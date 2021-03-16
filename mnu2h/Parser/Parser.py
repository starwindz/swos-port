import os
import re

import Util
import Constants

from Token import Token
from Menu import Menu
from Entry import Entry
from VariableStorage import VariableStorage
from Tokenizer import Tokenizer

from Parser.MenuParser import MenuParser
from Parser.EntryParser import EntryParser
from Parser.ExpressionParser import ExpressionParser
from Preprocessor.Preprocessor import Preprocessor

# Contains everything a growing parser needs
class Parser:
    def __init__(self, inputPath):
        assert isinstance(inputPath, str)

        self.inputPath = inputPath
        self.inputFilename = os.path.basename(inputPath)

        self.stringTableLengths = set() # set of lengths all string tables used in the program (number of their strings)
        self.entryDotReferences = []
        self.functions = set()      # this will contain names of all the functions declared
        self.menus = {}

        self.varStorage = VariableStorage()
        self.tokenizer = Tokenizer.fromFile(inputPath)
        self.defaultPropertyTokenizer = Tokenizer.empty()
        self.preprocessor = Preprocessor(self.tokenizer, inputPath, self.varStorage, self.parseExpression)
        self.expressionParser = ExpressionParser(self.tokenizer, self.preprocessor, self.varStorage)

        entryParser = EntryParser(self.tokenizer, self.preprocessor, self.expressionParser, self.varStorage)
        self.menuParser = MenuParser(self.menus, self.tokenizer, entryParser, self.preprocessor, self.expressionParser, self.varStorage)

        self.parse()
        self.checkForUnusedVariables()
        self.checkForConflictingExportedMenuVariables()
        self.checkForZeroWidthHeightEntries()
        self.resolveReferences()

    def getFunctions(self):
        return self.functions

    def getStringTableLengths(self):
        return self.stringTableLengths - {0}

    def getMenus(self):
        return self.menus

    def showWarnings(self):
        return self.preprocessor.showWarnings()

    def parseExpression(self, menu):
        return self.expressionParser.parse(menu)

    def parse(self):
        while not self.tokenizer.isAtEof():
            token = self.tokenizer.getNextToken()

            if token.string == '#':
                self.preprocessor.parsePreprocessorDirective()
            elif token.string == 'Menu':
                self.menuParser.parse()
                self.functions.update(self.menuParser.functions)
                self.stringTableLengths.update(self.menuParser.stringTableLengths)
                self.entryDotReferences += self.menuParser.entryDotReferences
            elif token.string == '}':
                Util.error('unmatched block end', token)
            elif token.string == 'Entry':
                Util.error('entries can only be used inside menus', token)
            else:
                self.parseAssignment(token)

    def parseAssignment(self, token):
        assert isinstance(token, Token)

        varToken = token
        varName = token.string

        if varName == 'export':
            Util.error('top-level exports not allowed', varToken)

        if token.string == ')':
            Util.error("unexpected `)'", token)

        token = self.tokenizer.expectIdentifier(token, "equal sign (`=')", fetchNextToken=True)

        if token.string != '=':
            Util.error(f"expected equal `=', got `{token.string}'", token)

        value = self.expressionParser.parse()

        self.varStorage.addGlobalVariable(varName, value, varToken)

    # checkForUnusedVariables
    #
    # Checks if there are unused variables and issues warnings if so.
    #
    def checkForUnusedVariables(self):
        if self.preprocessor.showWarnings():
            for varName, varToken in self.varStorage.getUnreferencedGlobalsInFile(self.inputPath):
                Util.warning(f"unused global variable `{varName}'", varToken)

            for menuName, varName, varToken in self.varStorage.getUnreferencedMenuVariablesInFile(self.inputPath):
                Util.warning(f"unused variable `{varName}' declared in menu `{menuName}'", varToken)

    def checkForConflictingExportedMenuVariables(self):
        for menu in self.menus.values():
            for property, (_, token) in menu.exportedVariables.items():
                entry = next((entry for entry in menu.entries.values() if entry.name == property), None)
                if entry:
                    Util.error(f"can't export constant `{property}' since there is an entry with same name", token)

    def checkForZeroWidthHeightEntries(self):
        if self.preprocessor.disallowZeroWidthHeightEntries():
            for menu in self.menus.values():
                for entry in menu.entries.values():
                    if isinstance(entry, Entry) and not entry.isTemplate():
                        noWidth = not entry.width or entry.width == '0'
                        noHeight = not entry.height or entry.height == '0'

                        if noWidth or noHeight:
                            name = '' if not entry.name else f" `{entry.name}'"
                            errorMessage = f"entry{name} has zero "
                            if noWidth:
                                errorMessage += 'width' if not noHeight else 'width and height'
                            else:
                                errorMessage += 'height'
                            Util.error(errorMessage, entry.token)

    # resolveEntryReferences
    #
    # in:
    #     text     - string possibly containing entry references that need to be resolved
    #     menu     - parent menu
    #     refToken - token that's referencing the entry (if we have a reference)
    #     entry    - parent entry, if applicable
    #
    # Returns the given text with all the entry references replaced. Displays an error and ends the program
    # if an undefined reference is encountered.
    #
    def resolveEntryReferences(self, text, menu, refToken, entry=None):
        assert isinstance(text, str)
        assert isinstance(menu, Menu)
        assert refToken is None or isinstance(refToken, Token)
        assert not entry or isinstance(entry, Entry)

        result = ''

        for tokenString in Tokenizer.splitLine(text):
            if entry:
                if tokenString == '>':
                    if menu.lastEntry == entry:
                        Util.error(f'referencing entry beyond the last one', refToken)
                    result = Util.formatToken(result, str(entry.ordinal + 1))
                    continue
                elif tokenString == '<':
                    if entry.ordinal == 0:
                        Util.error(f'referencing entry before the first one', refToken)
                    result = Util.formatToken(result, str(entry.ordinal - 1))
                    continue

            if tokenString.isidentifier() or Util.isString(tokenString):
                while Util.isString(tokenString):
                    tokenString = tokenString[1:-1]

                entry = menu.entries.get(tokenString)
                if entry is None:
                    # try parsing it as an expression before giving up
                    self.defaultPropertyTokenizer.resetToString(tokenString, refToken)
                    expandedValue = self.expressionParser.parse(menu, True, self.defaultPropertyTokenizer)
                    expandedValue = Util.unquotedString(expandedValue)

                    if expandedValue in menu.entryAliases:
                        expandedValue = menu.entryAliases[expandedValue]

                    entry = menu.entries.get(expandedValue)
                    if entry is None:
                        Util.error(f"undefined entry reference: `{expandedValue}'", refToken)
                result = Util.formatToken(result, str(entry.ordinal))
            else:
                result = Util.formatToken(result, tokenString)

        result = Util.removeEnclosingParentheses(result)

        try:
            return int(result, 0)
        except ValueError:
            try:
                return int(eval(result))
            except:
                result = re.sub(r'\B\s+|\s+\B', '', result)
                Util.error(f"unable to evaluate: `{result}'", refToken)

    # resolveEntryDotReferences
    #
    # Resolves all entry.property references.
    #
    def resolveEntryDotReferences(self):
        for menu, entryName, property, value, token in self.entryDotReferences:
            assert isinstance(value, str)
            assert isinstance(entryName, str)
            assert isinstance(property, str)
            assert isinstance(value, str)
            assert isinstance(token, Token)

            if entryName in menu.entryAliases:
                entryName = menu.entryAliases[entryName]

            # this shouldn't happen
            if entryName not in menu.entries:
                Util.error(f"menu `{menu.name}' does not have entry `{entryName}'", token)

            value = value.strip('()')

            entry = menu.entries[entryName]
            if not hasattr(entry, property):
                Util.error(f"entry `{entryName}' does not have property `{property}'", token)

            if property in Constants.kNextEntryProperties:
                if value != '-1' and value not in menu.entries:
                    Util.error(f"entry `{value}' undefined", token)

                value = -1 if value == '-1' else menu.entries[value].ordinal

            setattr(entry, property, value)

    # resolveReferences
    #
    # Resolves all forward references to entries in all of the menus.
    #
    def resolveReferences(self):
        for menu in self.menus.values():
            initialEntry = str(menu.properties[Constants.kInitialEntry])
            menu.properties[Constants.kInitialEntry] = self.resolveEntryReferences(initialEntry, menu, menu.initialEntryToken)

            for entry in menu.entries.values():
                for property in Constants.kNextEntryProperties:
                    token = getattr(entry, property + 'Token')
                    oldValue = str(getattr(entry, property))
                    newValue = self.resolveEntryReferences(oldValue, menu, token, entry)
                    setattr(entry, property, newValue)

        self.resolveEntryDotReferences()

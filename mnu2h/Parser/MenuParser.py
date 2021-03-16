import Util
import Constants
import Parser.Common

from Menu import Menu
from Entry import Entry
from Token import Token
from Tokenizer import Tokenizer
from Variable import Variable
from VariableStorage import VariableStorage

from Parser.ExpressionParser import ExpressionParser
from Parser.EntryParser import EntryParser
from Preprocessor.Preprocessor import Preprocessor

class MenuParser:
    def __init__(self, menus, tokenizer, entryParser, preprocessor, expressionParser, varStorage):
        assert isinstance(tokenizer, Tokenizer)
        assert isinstance(entryParser, EntryParser)
        assert isinstance(preprocessor, Preprocessor)
        assert isinstance(expressionParser, ExpressionParser)
        assert isinstance(varStorage, VariableStorage)

        self.menus = menus
        self.tokenizer = tokenizer
        self.entryParser = entryParser
        self.preprocessor = preprocessor
        self.expressionParser = expressionParser
        self.varStorage = varStorage

        self.menu = None
        self.functions = set()
        self.stringTableLengths = set()
        self.entryDotReferences = []

        self.fillAllowedAndDefaultProperties()

    def fillAllowedAndDefaultProperties(self):
        self.allowedProperties = set(Constants.kMenuProperties)
        self.defaultProperties = set()

        for prop in Constants.kMenuDefaults + tuple(Constants.kDefaultPropertyAliases):
            prop1 = 'def' + prop[0].upper() + prop[1:]
            prop2 = 'default' + prop[0].upper() + prop[1:]
            self.allowedProperties.add(prop1)
            self.allowedProperties.add(prop2)
            self.defaultProperties.add(prop1)
            self.defaultProperties.add(prop2)

        self.allowedProperties.add('onRestore')

    def parse(self):
        self.menu = self.parseMenuHeader()
        self.parseMenuBody()
        return self.menu

    def parseMenuHeader(self):
        token, menuName, nameToken = Parser.Common.getName(self.tokenizer, self.preprocessor, 'menu name')
        if not menuName:
            Util.error('menu name missing', token)

        if menuName in self.menus:
            Util.error(f"menu `{menuName}' redeclared", nameToken)

        Util.verifyIdentifier(menuName, nameToken)
        Util.verifyNonCppKeyword(menuName, nameToken, 'menu name')

        self.tokenizer.expectBlockStart(token)

        menu = Menu()
        menu.name = menuName
        self.menus[menuName] = menu

        return menu

    def parseMenuBody(self):
        while True:
            token = self.tokenizer.getNextToken()
            if token.string == '}':
                break

            if token.string == '#':
                self.preprocessor.parsePreprocessorDirective(self.menu)
            elif token.string == 'Entry':
                self.entryParser.parse(self.menu, False)
                self.stringTableLengths.add(self.entryParser.stringTableLength)
                self.functions.update(self.entryParser.functions)
            elif token.string == 'TemplateEntry':
                self.entryParser.parse(self.menu, True)
            elif token.string == 'ResetTemplate':
                self.handleResetTemplateEntry()
            elif token.string == 'entryAlias':
                self.parseEntryAlias()
            else:
                self.parseMenuVariablesAndProperties(token)

        if Constants.kInitialEntry not in self.menu.properties:
            self.menu.properties[Constants.kInitialEntry] = 0
            self.menu.initialEntryToken = None

        self.verifyMenuHeaderType()

    def handleResetTemplateEntry(self):
        self.menu.templateEntry = Entry()
        self.menu.templateActive = False

        entry, internalName = self.menu.createNewTemplateEntry(True)
        self.menu.entries[internalName] = entry

        nextToken = self.tokenizer.peekNextToken()
        if nextToken and nextToken.string == '{':
            self.tokenizer.getNextToken()
            token = self.tokenizer.getNextToken()
            self.tokenizer.expect('}', token, fetchNextToken=False)

    def parseEntryAlias(self):
        token, alias, aliasToken = Parser.Common.getName(self.tokenizer, self.preprocessor, 'entry alias', self.menu)
        if not alias or alias == '=':
            Util.error('entry alias missing', aliasToken)

        Util.verifyIdentifier(alias, aliasToken)
        Util.verifyNonCppKeyword(alias, aliasToken, 'entry alias')

        self.tokenizer.expect('=', token)

        token, entryName, entryToken = Parser.Common.getName(self.tokenizer, self.preprocessor, 'entry name', self.menu)
        if not entryName:
            Util.error('entry name missing', token)

        if alias in self.menu.entryAliases:
            Util.error(f"entry alias `{alias}' redefined", aliasToken)

        if alias in self.menu.entries:
            Util.error(f"entry `{alias}' already exists", aliasToken)

        if entryName not in self.menu.entries:
            Util.error(f"unknown entry: `{entryToken.string}'", entryToken)

        self.menu.entryAliases[alias] = entryName
        self.tokenizer.ungetToken()

    # We've detected a non-entry entity in the menu and it can be property or variable assignment.
    # Variables will be declared in menu scope.
    def parseMenuVariablesAndProperties(self, token):
        assert isinstance(token, Token)

        idToken, exported = self.getMenuAssignmentTarget(token)
        token = self.tokenizer.expectIdentifier(idToken, fetchNextToken=False)
        token, entryNameToken, dotProperty = self.handleMenuDotVariable(idToken, token)

        token = self.tokenizer.getNextToken("equal sign (`=') or colon (`:')")
        if token.string not in '=:':
            Util.error(f"expecting menu variable assignment or property, got `{token.string}'", token)

        isVariableAssignment = token.string == '='
        isDotVariable = entryNameToken is not None

        if exported and (not isVariableAssignment or isDotVariable):
            Util.error("menu properties can't be exported", idToken)

        if isVariableAssignment:
            self.parseMenuVariableAssignment(idToken, entryNameToken, dotProperty, exported)
        else:
            self.parseMenuPropertyAssignment(idToken)

    def verifyMenuHeaderType(self):
        presentFunctions = tuple(filter(lambda func: func in self.menu.properties, Constants.kMenuFunctions))
        functionNames = tuple(map(lambda func: self.menu.properties[func], presentFunctions))

        numSwosFuncs = sum(Variable.isSwos(func) for func in functionNames)
        numNativeFuncs = sum(not Variable.isSwos(func) for func in functionNames)

        if numSwosFuncs > 0 and numNativeFuncs > 0:
            Util.error(f"menu `{self.menu.name}' contains mixed type functions", self.menu.propertyTokens[presentFunctions[0]])

    def getMenuAssignmentTarget(self, token):
        assert isinstance(token, Token)

        exported = False

        if token.string == 'export':
            exported = True
            token = self.tokenizer.getNextToken()
            Util.verifyNonCppKeyword(token.string, token, 'exported variable name')

        return token, exported

    # Detects if the assignment target (given by idToken) is a "dot variable" in the form of `entry.field'.
    # Only one level of dot property is supported.
    def handleMenuDotVariable(self, idToken, token):
        assert isinstance(idToken, Token)
        assert isinstance(token, Token)

        entryNameToken = None
        dotProperty = ''

        nextToken = self.tokenizer.peekNextToken()
        if nextToken and nextToken.string == '.':
            entryNameToken = idToken

            self.tokenizer.getNextToken()
            token = self.tokenizer.getNextToken()

            dotProperty = token.string
            token = self.tokenizer.expectIdentifier(token, fetchNextToken=False)

        return token, entryNameToken, dotProperty

    def parseMenuVariableAssignment(self, idToken, entryNameToken, dotProperty, exported):
        assert isinstance(idToken, Token)
        assert isinstance(entryNameToken, (Token, type(None)))
        assert isinstance(dotProperty, str)

        varName = idToken.string
        isDotVariable = entryNameToken is not None

        if not isDotVariable and self.varStorage.hasMenuVariable(self.menu, varName):
            Util.error(f"variable `{varName}' redefined", idToken)

        value = self.expressionParser.parse(self.menu)

        if isDotVariable:
            self.entryDotReferences.append((self.menu, entryNameToken.string, dotProperty, value, entryNameToken))
        else:
            var = self.varStorage.addMenuVariable(self.menu, varName, value, idToken)
            if exported:
                self.menu.exportedVariables[varName] = (value, idToken)
                var.referenced = True   # treat exported variables as referenced to suppress unused warning

    def parseMenuPropertyAssignment(self, propertyToken):
        assert isinstance(propertyToken, Token)

        property = propertyToken.string

        if property not in self.allowedProperties:
            Util.error(f"unknown menu property: `{property}'", propertyToken)

        token = self.tokenizer.peekNextToken()
        if not token or token.string == '}':
            Util.error(f"missing property value for `{property}'", propertyToken)

        if property == 'onRestore':
            property = 'onReturn'

        isFunction = property in Constants.kMenuFunctions

        if isFunction:
            value, declareFunction = Parser.Common.registerFunction(self.tokenizer)
            if declareFunction:
                self.functions.add(value)
        else:
            expressionStartToken = token

            delayedExpansionProperty = property in self.defaultProperties
            value = self.expressionParser.parse(self.menu, not delayedExpansionProperty)

            if property in ('x', 'y'):
                value = Util.getNumericValue(value, expressionStartToken)

        property = self.expandMenuPropertyAliases(property)

        if property == Constants.kInitialEntry:
            self.menu.initialEntryToken = propertyToken

        self.menu.properties[property] = value
        self.menu.propertyTokens[property] = propertyToken

        self.handleInitialXYMenuProperties(property, value)

    def handleInitialXYMenuProperties(self, property, value):
        assert isinstance(property, str)

        if property in ('x', 'y') and not self.menu.entries:
            setattr(self.menu, 'initial' + property.upper(), value)

    @staticmethod
    def expandMenuPropertyAliases(prop):
        assert isinstance(prop, (str))

        if len(prop) > 3 and prop.startswith('def'):
            i = 7 if prop[3:].startswith('ault') else 3
            base = prop[i].lower() + prop[i + 1:]

            base = Constants.kDefaultPropertyAliases.get(base, base)

            if base in Constants.kMenuDefaults:
                return 'default' + base[0].upper() + base[1:]

        return prop

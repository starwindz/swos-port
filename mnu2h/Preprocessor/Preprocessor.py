"""Handler of preprocessor directives (starting with `#'). Home to many a token manipulation."""

import os
import copy

import Constants
import Util

from Token import Token
from Variable import Variable
from VariableStorage import VariableStorage
from Menu import Menu
from Tokenizer import Tokenizer

from Preprocessor.ExpressionHandler import ExpressionHandler

class Preprocessor:
    def __init__(self, tokenizer, inputPath, varStorage, parseExpression):
        assert isinstance(tokenizer, Tokenizer)
        assert isinstance(inputPath, str)
        assert isinstance(varStorage, VariableStorage)
        assert callable(parseExpression)

        self.tokenizer = tokenizer
        self.inputPath = inputPath
        self.varStorage = varStorage
        self.parseExpression = parseExpression

        self.warningsState = True
        self.disallowZeroWidthHeightEntriesState = True

        self.expressionHandler = ExpressionHandler(tokenizer, varStorage, self.showWarnings)

    def showWarnings(self):
        return self.warningsState

    def disallowZeroWidthHeightEntries(self):
        return self.disallowZeroWidthHeightEntriesState

    # parsePreprocessorDirective
    #
    # in:
    #     menu        - current menu we're in, if any
    #     onlyNonVoid - only allow directives that return values
    #
    # After the initial character of preprocessor directive has been encountered, recognizes which
    # directive it was and processes it. Returns the result of the invoked directive,
    # or None if it's a void directive.
    #
    def parsePreprocessorDirective(self, menu=None, onlyNonVoid=False):
        assert isinstance(menu, (Menu, type(None)))

        token = self.tokenizer.getNextToken()
        directive = token.string

        if not hasattr(self, 'kPreprocDirectives'):
            self.kPreprocDirectives = {
                'include':     (self.parseInclude, False),
                'textWidth':   (self.parseTextWidth, True),
                'textHeight':  (self.parseTextHeight, True),
                'repeat':      (self.parseRepeatDirective, False),
                'join':        (self.parseJoinDirective, True),
                'print':       (self.parsePrintDirective, False),
                'assert':      (self.parseAssertDirective, False),
                'eval':        (self.parseAndEvaluateEvalDirective, True),
                '{':           (self.parseAndEvaluateEvalDirective, True),
                'warningsOn':  (self.warningsOn, False),
                'warningsOff': (self.warningsOff, False),
                'allowZeroWidthHeightEntries': (self.zeroWidthHeightEntriesOn, False),
                'disallowZeroWidthHeightEntries': (self.zeroWidthHeightEntriesOff, False),
            }

        handler, returnsValue = self.kPreprocDirectives.get(directive, 2 * (None,))

        if not handler:
            assert directive not in Constants.kPreprocReservedWords
            Util.error(f"unknown preprocessor directive `{directive}'", token)

        if onlyNonVoid and not returnsValue:
            Util.error(f"directive `{directive}' not allowed in this context", token)

        if directive == 'eval':
            token = self.tokenizer.getNextToken("`{'")
            self.tokenizer.expect('{', token, fetchNextToken=False)
        elif directive != '{':
            self.tokenizer.expectIdentifier(token, fetchNextToken=False)

        result = handler(menu)

        if onlyNonVoid and directive == 'join':
            self.tokenizer.getNextToken()

        return result

    # parseInclude
    #
    # Parses include directive, opens the file and scoops all the tokens.
    #
    def parseInclude(self, _menu):
        token = self.tokenizer.getNextToken('include filename')
        filename = token.string

        if filename[0] != '"' or filename[-1] != '"':
            Util.error(f"expecting quoted file name, got `{filename}'", token)

        filename = filename[1:-1]
        path = os.path.join(os.path.dirname(self.inputPath), filename)

        if not os.path.isfile(path):
            path = os.path.abspath(filename)

        self.tokenizer.includeFile(path)

    # parseTextWidth
    #
    # Parses text width directive and returns width of the text in pixels.
    #
    def parseTextWidth(self, _menu):
        text, useBigFont = self.parseTextWidthAndHeight(True)
        width = self.textWidth(text, useBigFont)
        return width

    # parseTextHeight
    #
    # Parses text height directive and returns height of the text in pixels.
    #
    def parseTextHeight(self, _menu):
        _, useBigFont = self.parseTextWidthAndHeight()
        height = self.textHeight(useBigFont)
        return height

    def parseTextSize(self, token):
        assert isinstance(token, Token)

        fontSize = token.string.lower()
        if fontSize == 'big':
            fontSize = True
        elif fontSize == 'small':
            fontSize = False
        else:
            fontSize = Util.getBoolValue(token)

        return fontSize, self.tokenizer.getNextToken()

    # parseTextWidthAndHeight
    #
    # in:
    #     requireText - flag determining if we require text or it can be skipped
    #
    # Does the common part of the text width and height directives parsing, extracts the text
    # and font size and returns them for further processing.
    #
    def parseTextWidthAndHeight(self, requireText=False):
        token = self.tokenizer.getNextToken("`('")
        token = self.tokenizer.expect('(', token, fetchNextToken=True)

        text = ''
        fontSize = False

        if Util.isString(token.string):
            text = token.string[1:-1]
            token = self.tokenizer.getNextToken("comma `,' or closing parentheses `)'")
            if token.string == ',':
                token = self.tokenizer.getNextToken('font size')
                fontSize, token = self.parseTextSize(token)
        elif requireText:
            self.tokenizer.expect('a string', token, quote=False, fetchNextToken=True)
        elif token.string != ')':
            fontSize, token = self.parseTextSize(token)

        token = self.tokenizer.expect(')', token, fetchNextToken=False)
        return text, fontSize

    # parseRepeatDirective
    #
    # in:
    #     menu  - parent menu, if any
    #     level - level of depth of the loop, 0 is the topmost
    #
    # Scans for "#repeatEnd" marker and repeats all tokens in between by the given number of times.
    # Optional parameter is a name of loop variable, which will default to `i' if not given.
    #
    def parseRepeatDirective(self, menu, level=0):
        assert isinstance(menu, (Menu, type(None)))
        assert isinstance(level, int)

        repeatStart = self.tokenizer.getCurrentTokenIndex() - 2
        initialToken = copy.copy(self.tokenizer.peekTokenAt(repeatStart))

        if self.tokenizer.isAtEof():
            Util.error('repeat count is missing', initialToken)

        repeatCount, repeatCountToken = self.getRepeatCount(menu)
        if not isinstance(repeatCount, int):
            Util.error(f"repeat count must be integer (`{repeatCount}' given)", repeatCountToken)
        loopVariable = self.getLoopVariable(initialToken)

        self.removeOpeningRepeatDirectiveTokens(repeatStart)

        start = repeatStart
        isTerminated, end = self.parseRepeatLoop(menu, loopVariable, level)

        if not isTerminated:
            Util.error(f'unterminated repeat directive', initialToken)

        repeatBlock = self.tokenizer.getTokenRange(start, end)
        if not repeatBlock:
            Util.error(f'empty repeat block', initialToken)

        self.tokenizer.deleteTokens(repeatStart, end)

        self.unrollLoopAndResolveReferences(loopVariable, start, repeatBlock, repeatCount, level)
        self.tokenizer.setCurrentTokenIndex(start)

    def getRepeatCount(self, menu):
        assert isinstance(menu, (Menu, type(None)))

        initialExpressionToken = self.tokenizer.peekNextToken()
        nodes = self.expressionHandler.parse(menu)
        self.checkForUninitializedVariablesInRepeatCount(nodes, initialExpressionToken)
        repeatCount = self.expressionHandler.evaluate(nodes)[0]

        return repeatCount, initialExpressionToken

    @staticmethod
    def checkForUninitializedVariablesInRepeatCount(nodes, initialExpressionToken):
        assert isinstance(nodes, list)
        assert isinstance(initialExpressionToken, Token)

        uninitializedVariables = sorted({ f"`{node.var.name}'" for node in nodes if node.uninitialized })
        if uninitializedVariables:
            variableListString = ', '.join(uninitializedVariables)
            errorMessage = f'repeat count expression must not contain uninitialized variables ({variableListString})'
            Util.error(errorMessage, initialExpressionToken)

    def getLoopVariable(self, initialToken):
        assert isinstance(initialToken, Token)

        loopVariableName, loopVariableToken = self.parseExplicitLoopVariable(initialToken)
        loopVariableName = self.trySelectingImplicitLoopVariable(loopVariableName, loopVariableToken)
        loopVariable = self.assignFinalLoopVariable(loopVariableName, loopVariableToken)

        return loopVariable

    def parseExplicitLoopVariable(self, initialToken):
        assert isinstance(initialToken, Token)

        loopVariableName = ''
        loopVariableToken = None

        token = self.tokenizer.peekNextToken()

        if token and token.string == '=>':
            self.tokenizer.getNextToken()
            token = self.tokenizer.getNextToken('loop variable')
            self.tokenizer.expectIdentifier(token, fetchNextToken=False)

            loopVariableToken = token
            loopVariableName = token.string

            if loopVariableName in Constants.kPreprocReservedWords:
                Util.error(f"`{loopVariableName}' is a reserved preprocessor word", loopVariableToken)
        else:
            loopVariableToken = initialToken

        return loopVariableName, loopVariableToken

    # If no loop variable is specified, try assigning one of standard i, j, k. But only if they're not
    # already assigned. If they're all taken report an error.
    def trySelectingImplicitLoopVariable(self, loopVariableName, loopVariableToken):
        assert isinstance(loopVariableName, str)
        assert isinstance(loopVariableToken, Token)

        if not loopVariableName:
            for kLoopVar in ('i', 'j', 'k'):
                loopVariableName = kLoopVar
                if not self.varStorage.hasPreprocVariable(kLoopVar):
                    break
            else:
                Util.error('could not assign loop variable implicitly', loopVariableToken)

        return loopVariableName

    def assignFinalLoopVariable(self, loopVariableName, loopVariableToken):
        assert isinstance(loopVariableName, str)
        assert isinstance(loopVariableToken, Token)

        if self.varStorage.hasLoopVariable(loopVariableName):
            Util.error(f"`{loopVariableName}' already used as a loop variable", loopVariableToken)

        if not self.varStorage.hasPreprocVariable(loopVariableName):
            self.varStorage.addPreprocVariable(loopVariableName, 0, loopVariableToken)

        loopVariable = self.varStorage.addLoopVariable(loopVariableName)

        return loopVariable

    def removeOpeningRepeatDirectiveTokens(self, repeatStart):
        assert isinstance(repeatStart, int)

        repeatEnd = self.tokenizer.getCurrentTokenIndex()
        self.tokenizer.deleteTokens(repeatStart, repeatEnd)
        self.tokenizer.setCurrentTokenIndex(repeatStart)

    # parseRepeatLoop
    #
    # in:
    #     menu         - parent menu, if any
    #     loopVariable - reference to loop variable
    #     level        - nesting level of the loop
    #
    # Parses loop body, prepares loop variable references, parses expressions and decides how to evaluate
    # them later so the loop variables can get correct values. Returns last token belonging to the loop
    # and its index.
    #
    def parseRepeatLoop(self, menu, loopVariable, level):
        assert isinstance(menu, (Menu, type(None)))
        assert isinstance(loopVariable, Variable)
        assert isinstance(level, int)

        while self.tokenizer.gotTokensLeft():
            token = self.tokenizer.getNextToken('repeat loop contents')

            if token.string == '#':
                token = self.tokenizer.peekNextToken()

                if token.string == 'eval' or token.string == '{':
                    self.handleExpressionInLoopBody(menu)
                else:
                    self.tokenizer.expectIdentifier(token, fetchNextToken=False)

                    varName = token.string
                    varToken = token

                    if varName == 'repeat':
                        self.tokenizer.getNextToken()
                        self.parseRepeatDirective(menu, level + 1)
                    elif varName == 'endRepeat':
                        end = self.endRepeatLoop(loopVariable)
                        return True, end
                    elif varName == loopVariable.name:
                        self.convertLoopVariableRefToExpression(loopVariable, varToken, menu)
                    elif varName not in Constants.kPreprocReservedWords and not self.varStorage.hasPreprocVariable(varName):
                        Util.error(f"unknown preprocessor variable `{varName}'", varToken)

        return False, self.tokenizer.getCurrentTokenIndex() - 1

    def handleExpressionInLoopBody(self, menu):
        assert isinstance(menu, (Menu, type(None)))

        exprStart = self.tokenizer.getCurrentTokenIndex() - 1
        token = self.tokenizer.getNextToken()

        if token.string == 'eval':
            token = self.tokenizer.getNextToken("`{'")

        self.tokenizer.expect('{', token, fetchNextToken=False)

        self.varStorage.setLoopVariablesAsUnreferenced()
        expressionNodes, formatSpec = self.parseEvalDirective(menu)

        self.evaluateOrDeferLoopBodyExpression(exprStart, expressionNodes, formatSpec)
        self.removeExpressionFromLoopBody(exprStart)

    # Evaluates the expression immediately if it's not dependant on any loop variable. If it is then stores
    # the expression tree in the token stream so we at least don't have to parse it again.
    def evaluateOrDeferLoopBodyExpression(self, exprStart, nodes, formatSpec):
        assert isinstance(exprStart, int)
        assert isinstance(nodes, list)

        referencedLoopVars = self.varStorage.getReferencedLoopVariables()
        exprToken = self.tokenizer.peekTokenAt(exprStart)

        if not referencedLoopVars:
            value = self.expressionHandler.evaluate(nodes)[0]
            value = self.formatValueWithSpecification(value, formatSpec)
            exprToken.string = str(value)
        else:
            assert nodes
            exprToken.string = nodes
            exprToken.referencedLoopVars = referencedLoopVars
            exprToken.spec = formatSpec

    def removeExpressionFromLoopBody(self, exprStart):
        assert isinstance(exprStart, int)

        currentTokenIndex = self.tokenizer.getCurrentTokenIndex()
        self.tokenizer.deleteTokens(exprStart + 1, currentTokenIndex)
        self.tokenizer.setCurrentTokenIndex(exprStart + 1)

    # Wraps up the loop. #repeat/repeatEnd tokens are deleted from the stream and loop variables destroyed.
    # Returns ending index of token range (which at this point contains pure instructions).
    def endRepeatLoop(self, loopVariable):
        assert isinstance(loopVariable, Variable)

        end = self.tokenizer.getCurrentTokenIndex() - 1
        self.tokenizer.deleteTokens(end, end + 2)
        self.varStorage.destroyLoopVariable(loopVariable.name)

        return end

    # Turns variable references into single node expressions.
    def convertLoopVariableRefToExpression(self, loopVariable, varToken, menu):
        assert isinstance(loopVariable, Variable)
        assert isinstance(varToken, Token)
        assert isinstance(menu, (Menu, type(None)))

        opNodeList = self.expressionHandler.createSingleOperandExpression(varToken, menu, loopVariable, self.varStorage)
        hashIndex = self.tokenizer.getCurrentTokenIndex() - 1

        self.tokenizer.setTokenTextAt(hashIndex, opNodeList)
        loopVarToken = self.tokenizer.peekTokenAt(hashIndex)
        loopVarToken.referencedLoopVars = { loopVariable }
        loopVarToken.spec = None

        self.tokenizer.deleteToken(hashIndex + 1)
        self.tokenizer.setCurrentTokenIndex(hashIndex + 1)

    # unrollLoopAndResolveReferences
    #
    # in:
    #     loopVariable - loop variable for this loop
    #     start        - index of starting token
    #     repeatBlock  - list of tokens to repeat
    #     repeatCount  - number of times to repeat
    #     level        - nesting level of the loop
    #
    # Duplicates block as many times as specified and goes through gathered list of tokens forming the loop
    # resolving references to the loop variable, as well as expressions, which are expected in parsed form
    # (as a list of evaluation nodes).
    #
    def unrollLoopAndResolveReferences(self, loopVariable, start, repeatBlock, repeatCount, level):
        assert isinstance(loopVariable, Variable)
        assert isinstance(start, int)
        assert isinstance(repeatBlock, list)
        assert isinstance(repeatCount, int)
        assert isinstance(level, int)

        loopVariable.value = 0

        for i in range(0, repeatCount):
            insertIndex = start + i * len(repeatBlock)
            expressionCache = {}

            for j, token in enumerate(repeatBlock):
                destToken = token

                if isinstance(token.string, list):
                    destToken = self.tryEvaluatingExpressionReferencingLoopVariables(token, loopVariable, expressionCache, level)
                else:
                    assert isinstance(token, Token)

                self.tokenizer.insertTokenAt(insertIndex + j, destToken)

            loopVariable.value += 1

    def tryEvaluatingExpressionReferencingLoopVariables(self, token, loopVariable, expressionCache, level):
        assert isinstance(token, Token)
        assert isinstance(loopVariable, Variable)
        assert isinstance(expressionCache, dict)
        assert isinstance(level, int)

        assert hasattr(token, 'referencedLoopVars')
        assert hasattr(token, 'spec')
        assert isinstance(token.referencedLoopVars, set)
        assert token.referencedLoopVars

        destToken = copy.copy(token)
        nodes = token.string
        assert nodes
        spec = token.spec

        if not level or len(token.referencedLoopVars) == 1 and loopVariable in token.referencedLoopVars:
            self.evaluateStoredExpression(destToken, nodes, spec, expressionCache)
        elif loopVariable in token.referencedLoopVars:
            self.replaceLoopVariableWithConstant(destToken, nodes, loopVariable, expressionCache)

        return destToken

    def evaluateStoredExpression(self, destToken, nodes, formatSpec, expressionCache):
        assert isinstance(destToken,Token)
        assert isinstance(nodes, list)
        assert isinstance(formatSpec, (str, type(None)))
        assert isinstance(expressionCache, dict)

        value = expressionCache.get(id(nodes))
        if value is None:
            value = self.expressionHandler.evaluate(nodes)[0]
            value = self.formatValueWithSpecification(value, formatSpec)
        destToken.string = str(value)
        expressionCache[id(nodes)] = value

    @staticmethod
    def replaceLoopVariableWithConstant(destToken, nodes, loopVariable, expressionCache):
        assert isinstance(destToken,Token)
        assert isinstance(nodes, list)
        assert isinstance(loopVariable, Variable)
        assert isinstance(expressionCache, dict)

        nodesFork = expressionCache.get(id(nodes))
        if nodesFork is None:
            nodesFork = []
            for node in nodes:
                nodesFork.append(copy.copy(node))

            for k in range(0, len(nodes)):
                node = nodesFork[k]
                if node.isLvalue() and node.var.name == loopVariable.name:
                    constNode = copy.copy(node)
                    constNode.setConstant(loopVariable.value)
                    nodesFork[k] = constNode

            expressionCache[id(nodes)] = nodesFork

        assert nodesFork
        destToken.string = nodesFork

    # parseJoinDirective
    #
    # Creates a single token by concatenating a comma separated list of tokens. Whole construct is replaced by the
    # resulting token.
    #
    def parseJoinDirective(self, menu):
        start = self.tokenizer.getCurrentTokenIndex() - 2
        assert self.tokenizer.peekTokenAt(start).string == '#'

        token = self.tokenizer.getNextToken("`('")
        token = self.tokenizer.expect('(', token, fetchNextToken=True)

        reportMissingValueError = lambda: Util.error('value to join missing', token)
        joinedString = ''

        while token.string != ')':
            if token.string == ',':
                reportMissingValueError()

            if token.string == '#':
                value = self.parsePreprocessorDirective(menu, onlyNonVoid=True)
                value = str(value)
            else:
                value = token.string

            joinedString += Util.unquotedString(value)

            token = self.tokenizer.getNextToken("`)' or `,'")
            if token.string == ',':
                token = self.tokenizer.getNextToken()
                if token.string == ')':
                    reportMissingValueError()
            elif token.string != ')':
                Util.error(f"`{token.string}' unexpected, only `,' or `)' allowed", token)

        if not joinedString:
            reportMissingValueError()

        end = self.tokenizer.getCurrentTokenIndex()
        self.tokenizer.deleteTokens(start + 1, end)
        self.tokenizer.setCurrentTokenIndex(start)

        self.tokenizer.setTokenTextAt(start, joinedString)

        return joinedString

    # parsePrintDirective
    #
    # in:
    #     menu  - parent menu, if any
    #
    # Simply prints an expression that follows the directive. Useful for debugging.
    #
    def parsePrintDirective(self, menu):
        assert isinstance(menu, (Menu, type(None)))

        token = self.tokenizer.peekNextToken()
        if not token or token.string == ',':
            Util.error('missing expression to print', self.tokenizer.getPreviousToken())

        line = token.line
        filename = token.inputFilename

        values = []

        while True:
            value = self.parseExpression(menu)
            value = Util.unquotedString(value)
            values.append(value)

            token = self.tokenizer.peekNextToken()
            if not token or token.string != ',':
                break

            self.tokenizer.getNextToken()

        print(f'{filename}({line}):', ', '.join(f"`{value}'" for value in values))

    def parseAssertDirective(self, menu):
        assert isinstance(menu, (Menu, type(None)))

        token = self.tokenizer.peekNextToken()
        if not token:
            Util.error('missing assert expression', self.tokenizer.getPreviousToken())

        line = token.line
        filename = token.inputFilename

        nodes = self.expressionHandler.parse(menu)
        value = self.expressionHandler.evaluate(nodes)[0]

        if not value:
            Util.error('assertion failed', token)

    # textWidth
    #
    # in:
    #     text       - text to measure
    #     useBigFont - flag whether to use big or small font to measure
    #
    # Returns the width of the given text in pixels.
    #
    @staticmethod
    def textWidth(text, useBigFont):
        assert isinstance(text, str)

        spaceWidth = 4 if useBigFont else 3
        table = Constants.kBigFontWidths if useBigFont else Constants.kSmallFontWidths

        width = 0

        for c in text:
            c = ord(c.upper()) - 32
            if c == 0:
                width += spaceWidth
            elif c > 0 and c < len(table):
                charWidth = table[c]
                width += charWidth

        return width

    # textHeight
    #
    # in:
    #     useBigFont - flag whether to use big or small font to measure
    #
    # Returns the height of the text in pixels.
    #
    @staticmethod
    def textHeight(useBigFont):
        return Constants.kBigFontHeight if useBigFont else Constants.kSmallFontHeight

    # parseAndEvaluateEvalDirective
    #
    # in:
    #     menu  - parent menu, if any
    #
    # Parses and evaluates a compile time expression, and returns the result and the token that follows it.
    #
    def parseAndEvaluateEvalDirective(self, menu):
        assert isinstance(menu, (Menu, type(None)))

        nodes, formatSpec = self.parseEvalDirective(menu)
        value, nodes, _ = self.expressionHandler.evaluate(nodes)
        assert not nodes

        value = self.formatValueWithSpecification(value, formatSpec)
        return value

    # parseEvalDirective
    #
    # in:
    #     menu - parent menu, if any
    #
    # Does parsing-only of a given `eval' directive. Returns a complete evaluation tree of the expression and
    # its format specification (if present).
    #
    def parseEvalDirective(self, menu):
        assert isinstance(menu, (Menu, type(None)))

        nodes = self.expressionHandler.parse(menu)

        token = self.tokenizer.getNextToken("`}'")
        token = self.tokenizer.expect('}', token, fetchNextToken=False)

        formatSpecification = self.parseFormatSpecification()

        return nodes, formatSpecification

    def parseFormatSpecification(self):
        spec = None

        token = self.tokenizer.peekNextToken()
        if token and token.string == ':':
            self.tokenizer.getNextToken()
            token = self.tokenizer.getNextToken('format specification')
            spec = token.string

            if token.string in '+-#':
                token = self.tokenizer.getNextToken('format specification')
                spec += token.string

            if token.string != 'q':
                try:
                    self.formatValueWithSpecification(0, token.string)
                except ValueError:
                    Util.error(f"invalid format specifier: `{token.string}'", token)

        return spec

    @staticmethod
    def formatValueWithSpecification(value, formatSpec):
        assert isinstance(value, (int, str))
        assert isinstance(formatSpec, (str, type(None)))

        if formatSpec:
            if formatSpec == 'q':
                value = f'"{value}"'
            else:
                value = '{:{}}'.format(value, formatSpec)

        return value

    def warningsOn(self, _menu):
        self.warningsState = True

    def warningsOff(self, _menu):
        self.warningsState = False

    def zeroWidthHeightEntriesOn(self, _menu):
        self.disallowZeroWidthHeightEntriesState = False

    def zeroWidthHeightEntriesOff(self, _menu):
        self.disallowZeroWidthHeightEntriesState = True

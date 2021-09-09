import inspect

import Util
import Parser.Common

from Menu import Menu
from Token import Token
from Tokenizer import Tokenizer
from VariableStorage import VariableStorage

from Preprocessor.Preprocessor import Preprocessor

class ExpressionParser:
    def __init__(self, tokenizer, preprocessor, varStorage):
        assert isinstance(tokenizer, Tokenizer)
        assert isinstance(preprocessor, Preprocessor)
        assert isinstance(varStorage, VariableStorage)

        self.tokenizer = tokenizer
        self.preprocessor = preprocessor
        self.varStorage = varStorage

    # parse
    #
    # in:
    #     menu            - current menu (if any)
    #     expandVariables - if false do not expand variables, copy everything verbatim
    #     tokenizer       - custom tokenizer to use (if not given will use standard one from the instance)
    #
    # Parses an expression and returns the result (as a string). This isn't a real expression evaluator,
    # it will just make sure that the result is in the form a C++ compiler can evaluate.
    #
    def parse(self, menu=None, expandVariables=True, tokenizer=None):
        assert isinstance(menu, (Menu, type(None)))
        assert isinstance(tokenizer, (Tokenizer, type(None)))

        tokenizer = tokenizer or self.tokenizer

        result = ''

        kOperators = '+-*/%&|^()<>'
        isOperand = lambda op: op == '' or op not in kOperators

        if not tokenizer.gotTokensLeft():
            tokenizer.getNextToken('expression start')

        arrow = self.handleNextPrevEntryArrows(menu, tokenizer)
        if arrow:
            tokenizer.getNextToken()
            return arrow

        while True:
            token = tokenizer.getNextToken('expression operand')
            op = token.string

            while op == '#':
                op = self.preprocessor.parsePreprocessorDirective(menu, onlyNonVoid=True)
                assert op is not None
                op = str(op)

            if op == '(':
                value = self.handleParenSubexpression(menu, expandVariables, tokenizer)
                result = Util.formatToken(result, value)
            elif op == ')':
                Util.error('unmatched parenthesis', token)
            else:
                result = self.handleOperand(op, token, menu, result, isOperand, expandVariables, tokenizer)

            token = tokenizer.peekNextToken()
            if not token or isOperand(token.string) or token.string in (')', '}'):
                break

            tokenizer.getNextToken()
            result = Util.formatToken(result, token.string)

        if result:
            if result.endswith(' '):
                result = result[:-1]

            if result[0] == '(' and result[-1] == ')' and result.count('(') == 1:
                result = result[1:-1]

        return result

    @staticmethod
    def handleNextPrevEntryArrows(menu, tokenizer):
        assert isinstance(menu, (type(None), Menu))
        assert isinstance(tokenizer, Tokenizer)

        if menu:
            nextToken = tokenizer.peekNextToken()
            if nextToken and (nextToken.string == '<' or nextToken.string == '>'):
                return nextToken.string

    def handleParenSubexpression(self, menu, expandVariables, tokenizer):
        assert isinstance(menu, (Menu, type(None)))
        assert isinstance(tokenizer, Tokenizer)

        subResult = self.parse(menu, expandVariables, tokenizer)

        token = tokenizer.getNextToken("closing parenthesis `)'")
        if token.string != ')':
            Util.error('unmatched parenthesis', token)

        if subResult.endswith(' '):
            subResult = subResult[:-1]

        result = '(' + subResult + ')'

        return result

    def handleDotOperand(self, value, token, menu):
        assert isinstance(value, str)
        assert isinstance(token, Token)
        assert isinstance(menu, (Menu, type(None)))

        if value == 'swos':
            token = Parser.Common.getSwosVariable(self.tokenizer, skipDot=False)
            return token.string

        if menu is None:
            Util.error("dot operator (`.') can only be used inside menus", token)

        aliasValue = menu.entryAliases.get(value)
        if value not in menu.entries:
            if not aliasValue or aliasValue not in menu.entries:
                Util.error(f"`{value}' is not recognized as an entry name", token)
            else:
                value = aliasValue

        entry = menu.entries[value]
        token = self.tokenizer.getNextToken('menu property')

        if token.string == 'endY':
            value = f'({entry.y} + {entry.height})'
        elif token.string == 'endX':
            value = f'({entry.x} + {entry.width})'
        else:
            if not hasattr(entry, token.string):
                Util.error(f"entry `{value}' does not have property `{token.string}'", token)
            value = getattr(entry, token.string)

        return str(value)

    def handleOperand(self, op, token, menu, result, isOperand, expandVariables, tokenizer):
        assert isinstance(op, (str, int))
        assert isinstance(token, Token)
        assert isinstance(menu, (Menu, type(None)))
        assert isinstance(result, str)
        assert callable(isOperand)
        assert isinstance(tokenizer, Tokenizer)

        if op == '':
            return op

        unaryOp = None
        if op in ('+', '-', '~', '!'):
            unaryOp = token.string
            token = tokenizer.getNextToken()
            op = token.string

        if not isOperand(op):
            Util.error(f"operand expected, got `{op}'", token)

        expandIfNeeded = lambda value: self.varStorage.expandVariable(value, token, menu) if expandVariables else value

        value = expandIfNeeded(op)
        isString = Util.isString(value)

        nextToken = tokenizer.peekNextToken()
        if nextToken:
            if nextToken.string == '.':
                tokenizer.getNextToken()
                if isString:
                    while True:
                        value = self.handleStringFunction(value, tokenizer)
                        nextToken = tokenizer.peekNextToken()
                        if nextToken and nextToken.string == '.':
                            tokenizer.getNextToken()
                        else:
                            break
                else:
                    value = self.handleDotOperand(op, token, menu)
                    value = expandIfNeeded(value)
            elif nextToken.string == '[':
                tokenizer.getNextToken()
                value = self.handleStringIndexing(value, tokenizer)

        # must parenthesize in case value expands to an expression, e.g. a-b where b expands to 3+5
        if not isString and len(value.strip()) > 1:
            value = '(' + value + ')'

        result = Util.formatToken(result, value)

        if unaryOp:
            # add parentheses to prevent double minus from turning into increment
            result = '(' + unaryOp + result.rstrip() + ')'

        return result

    def handleStringFunction(self, value, tokenizer):
        assert isinstance(value, (int, str))
        assert not isinstance(value, str) or Util.isString(value)
        assert isinstance(tokenizer, Tokenizer)

        token = tokenizer.getNextToken('string function')
        params = self.getFunctionParameters(tokenizer)

        def verifyNumParams(expected):
            assert isinstance(expected, int)
            assert isinstance(params, list)

            actual = len(params)
            if actual != expected:
                Util.error(f'invalid number of parameters for {token.string}(), {expected} expected, got {actual}', token)

        if not hasattr(self, 'kStringFunctions'):
            self.kStringFunctions = {
                'first'      : lambda v: v[0] if v else '',
                'last'       : lambda v: v[-1] if v else '',
                'middle'     : lambda v: v[len(v) // 2] if v else '',
                'upper'      : lambda v: v.upper(),
                'lower'      : lambda v: v.lower(),
                'capitalize' : lambda v: v.capitalize(),
                'title'      : lambda v: v.title(),
                'zfill'      : lambda v, n: v.zfill(n),
                'left'       : lambda v, n: v[:n],
                'right'      : lambda v, n: v[-n:],
                'mid'        : lambda v, m, n: v[m:n],
            }

        handler = self.kStringFunctions.get(token.string)

        if not handler:
            Util.error(f"`{token.string}' is not a known string function", token)

        expectedNumParams = len(inspect.signature(handler).parameters) - 1
        verifyNumParams(expectedNumParams)

        quote = value[0]
        value = Util.unquotedString(value)

        result = handler(value, *params)
        return f'{quote}{result}{quote}'

    def handleStringIndexing(self, value, tokenizer):
        assert isinstance(value, str) and Util.isString(value)
        assert isinstance(tokenizer, Tokenizer)

        token = tokenizer.getNextToken('string index value')
        index = Util.getNumericValue(token.string, token)

        token = tokenizer.getNextToken("`]'")
        tokenizer.expect(']', token, fetchNextToken=False)

        quote = value[0]
        value = Util.unquotedString(value)

        try:
            c = value[index]
        except IndexError:
            Util.error(f"index {index} out of bounds for string `{value}'", token)

        return f'{quote}{c}{quote}'

    def getFunctionParameters(self, tokenizer):
        assert isinstance(tokenizer, Tokenizer)

        token = tokenizer.getNextToken("function parameter list start `('")
        token = tokenizer.expect('(', token, fetchNextToken=True)

        reportMissingParameterError = lambda: Util.error('function parameter missing', token)
        params = []

        while token.string != ')':
            if token.string == ',':
                reportMissingParameterError()

            if token.string == '#':
                value = self.preprocessor.parsePreprocessorDirective(menu=None, onlyNonVoid=True)
                assert value is not None
            else:
                value = token.string

            try:
                params.append(int(value))
            except ValueError:
                Util.error(f"function parameter `{token.string}' is not numeric", token)

            token = self.tokenizer.getNextToken("`)' or `,'")
            if token.string == ',':
                token = self.tokenizer.getNextToken()
                if token.string == ')':
                    reportMissingParameterError()
            elif token.string != ')':
                Util.error(f"`{token.string}' unexpected, only `,' or `)' allowed", token)

        return params

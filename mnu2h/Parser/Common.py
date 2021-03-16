import Util

from Menu import Menu
from Token import Token
from Tokenizer import Tokenizer
from Variable import Variable

from Preprocessor.Preprocessor import Preprocessor

# registerFunction
#
# Checks if a function inside the given token is an identifier and if it requires a forward declaration in C++ file.
# If so, checks it for uniqueness, if it's redefined reports an error and exits. Returns the function's name
# and declare flag.
#
def registerFunction(tokenizer):
    assert isinstance(tokenizer, Tokenizer)

    kNameOfHandlerFunction = 'name of a menu handler function'
    token = tokenizer.getNextToken(kNameOfHandlerFunction)
    Util.verifyNonCppKeyword(token.string, token, kNameOfHandlerFunction)

    functionName, declareFunction, isRawFunction = handlePredeclaredFunction(token, tokenizer)

    if not isRawFunction and not Variable.isSwos(functionName) and not functionName.isidentifier():
        Util.error(f"expected handler function, got `{functionName}'", token)

    return functionName, declareFunction

# If the function is prefixed with "swos." or tilde don't declare it (presumably it is declared elsewhere).
def handlePredeclaredFunction(token, tokenizer):
    assert isinstance(token, Token)
    assert isinstance(tokenizer, Tokenizer)

    functionName = token.string
    declareFunction = True
    isRawFunction = False

    if token.string == '~' or token.string == 'swos':
        if token.string == 'swos':
            token = getSwosVariable(tokenizer)
        else:
            token = tokenizer.getNextToken('handler function')

        functionName = token.string
        declareFunction = False

        # send a quoted function as is
        if Util.isString(functionName):
            functionName = functionName[1:-1]
            isRawFunction = True

    return functionName, declareFunction, isRawFunction

def getSwosVariable(tokenizer, skipDot=True):
    assert isinstance(tokenizer, Tokenizer)

    if skipDot:
        token = tokenizer.getNextToken('.')
        token = tokenizer.expect('.', token)

    token = tokenizer.getNextToken('SWOS symbol')
    token.string = Variable.makeSwos(token.string)
    return token

def getName(tokenizer, preprocessor, nameType, menu=None):
    assert isinstance(tokenizer, Tokenizer)
    assert isinstance(preprocessor, Preprocessor)
    assert isinstance(nameType, str)
    assert isinstance(menu, (Menu, type(None)))

    name = ''
    nameToken = None

    token = tokenizer.getNextToken(nameType)

    if token.string == '#' or token.string != '{':
        nameToken = token
        if token.string == '#':
            name = preprocessor.parsePreprocessorDirective(menu, onlyNonVoid=True)
            name = str(name)
        else:
            name = nameToken.string
        token = tokenizer.getNextToken()

    return token, name, nameToken

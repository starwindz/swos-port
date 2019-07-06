import os
import sys
import functools

from Entry import Entry
from Token import Token

# formatMessage
#
# Internal routine for having consistent output between error and warning reporting functions.
#
def formatMessage(text, pathOrToken, line, isWarning=False):
    assert isinstance(text, str) and text
    assert isinstance(pathOrToken, (str, Token, type(None)))
    assert isinstance(line, (int, type(None)))

    if pathOrToken is None:
        pathOrToken = ''

    if isinstance(pathOrToken, str):
        filename = pathOrToken
    else:
        filename = pathOrToken.inputFilename
        if not line:
            line = pathOrToken.line

    lineStr = f'({line})' if line and filename else ''
    warningStr = 'warning: ' if isWarning else ''
    colon = ': ' if filename else ''

    if filename:
        filename = os.path.basename(filename)

    return f'{filename}{lineStr}{colon}{warningStr}{text}'

# error
#
# in:
#     text        - text of the error to display
#     pathOrToken - path of the file we were processing, or token to get the info from (not present if tokenizing string)
#     line        - line number where error occurred (can be omitted if using token)
#
# Prints formatted error to stdout, with input filename and offending line, and exits the program.
#
def error(text, pathOrToken=None, line=None):
    errorMessage = formatMessage(text, pathOrToken, line)
    sys.exit(errorMessage)

# warning
#
# in:
#     text  - warning text
#     token - token associated with the warning
#     line     - line number of the warning
#
# Issues a warning to stdout, with input filename and line number.
#
def warning(text, tokenOrPath, line=None):
    warningMessage = formatMessage(text, tokenOrPath, line, True)
    print(warningMessage)

# isString
#
# in:
#     string - a string to check
#
# Returns true if the given string looks like a quoted string.
#
def isString(string):
    assert string and isinstance(string, str)

    return string[0] == '"' and string[-1] == '"' or string[0] == "'" and string[-1] == "'"

# unquotedString
#
# in:
#     string - a string to
#
# Returns new string with quotes removed.
#
def unquotedString(string):
    assert string and isinstance(string, str)

    if isString(string):
        string = string[1:-1]

    return string

# removeEnclosingParentheses
#
# in:
#    string - a string to remove enclosing parentheses from
#
# Returns a string with balanced, enclosing parentheses removed.
#
def removeEnclosingParentheses(string):
    while string and string[0] == '(' and string[-1] == ')':
        string = string[1:-1]

    return string

# formatToken
#
# in:
#     result - string that's being assembled
#     text   - text of the token that's to be added to result
#
# Concatenates text to result making sure the whitespace is inserted properly, and returns it.
#
def formatToken(result, text):
    assert isinstance(result, str)
    assert isinstance(text, str)

    if text == ')' and result.endswith(' '):
        result = result[:-1]

    result += text

    if text not in '(+-' and not isString(text):
        result += ' '

    return result

# getStringTableName
#
# in:
#     menuName - name of parent menu
#     entry    - entry for which we're deciding string table name
#
# Forms and returns a unique string table name for the entry.
#
def getStringTableName(menuName, entry):
    assert isinstance(menuName, str)
    assert isinstance(entry, Entry)

    name = menuName + '_' + entry.name

    if not entry.name:
        name += f'{entry.ordinal:02}'

    return name + '_stringTable'

# limitRotateTo32Bit
#
# Decorator in charge of normalizing input and output values of rotation routines,
# and keeping everything inside 32-bit boundaries.
#
def limitRotateTo32Bit(func):
    @functools.wraps(func)
    def rotateWrapper(n, r):
        n &= 0xffffffff

        if r < 0:
            f = (rotateLeft.__wrapped__, rotateRight.__wrapped__)[func == rotateLeft.__wrapped__]
            r = -r
        else:
            f = func

        r %= 32
        result = f(n, r) & 0xffffffff

        if result & 0x80000000:
            result = -((~result & 0xffffffff) + 1)

        return result

    return rotateWrapper

# rotateLeft
#
# in:
#     n - integer to rotate left
#     r - number of bits to rotate
#
# Rotates n left r bits, assumes n is 32-bit wide
#
@limitRotateTo32Bit
def rotateLeft(n, r):
    assert -2 ** 32 <= n < 2 ** 32 and 0 <= r < 32

    overflowBits = n >> (32 - r)
    n = ((n << r) | overflowBits) & 0xffffffff
    return n

# rotateLeft
#
# in:
#     n - integer to rotate left
#     r - number of bits to rotate
#
# Rotates n right r bits, assumes n is 32-bit wide
#
@limitRotateTo32Bit
def rotateRight(n, r):
    assert -2 ** 32 <= n < 2 ** 32 and 0 <= r < 32

    mask = 2 ** r - 1
    rotatedBits = n & mask
    return (n >> r) | (rotatedBits << (32 - r))

def withArticle(word):
    assert isinstance(word, str)

    if not word or not word[0].isalpha() or word[-1].lower() == 's':
        return word

    for article in ('a', 'an', 'the'):
        if word.startswith(article) and (len(word) == len(article) or word[len(article)].isspace()):
            return word

    article = 'an' if word[0].lower() in 'aeiou' else 'a'
    return f'{article} {word}'

# getNumericValue
#
# in:
#     text  - a string that should contain numeric value
#     token - token where the text originated from
#
# Extracts a numeric value from the string. If the value could not be converted, reports an error and exits.
#
def getNumericValue(text, token):
    assert isinstance(text, str)
    assert isinstance(token, Token)

    try:
        text = removeEnclosingParentheses(text)
        value = int(text, 0)
    except ValueError:
        error(f"`{text}' is not a numeric value'", token)

    return value

# getBoolValue
#
# in:
#     token - input token to get bool value from
#
# Extracts a boolean value from the given token and returns it
#
def getBoolValue(token):
    assert isinstance(token, Token)

    text = token.string.lower()
    if text == 'true':
        value = True
    elif text == 'false':
        value = False
    else:
        num = getNumericValue(token.string, token)
        value = num != 0

    return value

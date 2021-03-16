import unittest
from unittest import mock

from Parser.Parser import Parser

kTestFilename = 'InAGaddaDaVida'

def runWithMockedTokenizerOpen(callback, data='', ensureNewLine=True):
    assert callable(callback)
    assert isinstance(data, str)

    if ensureNewLine and not data.endswith('\n'):
        data += '\n'

    mockOpen = mock.mock_open(read_data=data)
    with mock.patch(f'Tokenizer.open', mockOpen):
        return callback()

def getParserWithData(data='', filePath=kTestFilename, ensureNewLine=True, allowDimensionlessEntries=True):
    if data and allowDimensionlessEntries:
        data = '#allowZeroWidthHeightEntries ' + data
    return runWithMockedTokenizerOpen(lambda: Parser(filePath), data, ensureNewLine)

def formatError(filename, line, text):
    assert isinstance(filename, str)
    assert isinstance(line, int)

    return f'{filename}({line}): {text}'

def assertExitWithError(test, func, errorMessage):
    assert isinstance(test, unittest.TestCase)
    assert callable(func)
    assert isinstance(errorMessage, str)

    with test.assertRaises(SystemExit) as cm:
        func()

    test.assertEqual(cm.exception.code, errorMessage)

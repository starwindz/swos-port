import os
import re
import unittest

from unittest import mock
from ddt import ddt, data

import TestHelper

from Token import Token
from Tokenizer import Tokenizer

kTemplateToken = Token('cambodia', 'axl', 23)

@ddt
class TestTokenizer(unittest.TestCase):
    def testEmptyFile(self):
        tokenizer = self.getTokenizerWithData()
        self.assertTrue(tokenizer.isAtEof())

    @data((('nothing', 'here'), 'File {} not found'),
        (('@#$*.%&', '*.*'), 'Error reading {}'))
    def testInvalidFilePaths(self, value):
        pathComponents, expected = value
        path = os.path.join(*pathComponents)
        TestHelper.assertExitWithError(self, lambda: Tokenizer(path), expected.format(path))

    @data('k r', '', 'b')
    def testResetToString(self, inputString):
        startingData = 'g a s t o n'
        tokenizer = self.getTokenizerWithData(startingData)

        token = tokenizer.getNextToken()
        self.assertEqual('g', token.string)

        tokenizer.resetToString(inputString, token)

        inputStringSquashed = inputString.replace(' ', '')
        expected = inputStringSquashed

        for c in expected:
            token = tokenizer.getNextToken()
            self.assertEqual(c, token.string)

        self.assertTrue(tokenizer.isAtEof())

    def testResetToString2(self):
        tokenizer = self.runWithMockedOpen(Tokenizer.empty)
        self.assertIsNone(tokenizer.peekNextToken())
        self.assertEqual(tokenizer.getCurrentTokenIndex(), 0)

        templateToken = kTemplateToken
        tokenizer.resetToString('Menu u{ Entry{} }', templateToken)
        self.assertIsNotNone(tokenizer.peekNextToken())

        def verifyTokens(*tokenStringList):
            self.assertEqual(tokenizer.getCurrentTokenIndex(), 0)
            for tokenString in tokenStringList:
                token = tokenizer.getNextToken()
                self.assertEqual(token.string, tokenString)
                self.assertEqual(token.line, templateToken.line)
                self.assertEqual(token.path, templateToken.path)
            self.assertTrue(tokenizer.isAtEof())

        verifyTokens('Menu', 'u', '{', 'Entry', '{', '}', '}')

        tokenizer.resetToString('bt=33', templateToken)
        verifyTokens('bt', '=', '33')

    @data(
        ('#include "recipe" fromage olala', 'Allumettes au'),
        ('#include "showMustGoOn.txt"', 'Empty spaces, what are we living for?'),
    )
    def testIncludeFile(self, testData):
        tokenizeData = lambda data: list(filter(None, re.split(r'(#)|\s+|\?|(,)', data)))

        inputData, includedData = testData
        tokenStrings = tokenizeData(inputData)

        tokenizer = self.getTokenizerWithData(inputData)

        inputPart1 = tokenStrings[:tokenStrings.index('include') + 2]
        inputPart2 = tokenStrings[len(inputPart1):]

        for expectedStr in inputPart1:
            token = tokenizer.getNextToken()
            self.assertEqual(token.string, expectedStr)

        includedTokenStrings = tokenizeData(includedData)

        def verifyInclude():
            tokenizer.includeFile('brmbrm')
            for tokenString in includedTokenStrings + inputPart2:
                token = tokenizer.getNextToken()
                self.assertEqual(token.string, tokenString)

        self.runWithMockedOpen(verifyInclude, includedData)

    def testInputPath(self):
        path = os.path.join('same', 'old', 'song')
        tokenizer = self.getTokenizerWithData(filePath=path)
        self.assertEqual(tokenizer.getInputPath(), path)

    @mock.patch('builtins.print')
    @data(
        ('', ()),   # empty file must give no warnings
        ('***', ((1, 'new line missing'),)),
        ("lEtS gO CRAAAZYY\n", ()),
        ('"NO"\nWARNINGS\n\'WHATSOEVER\'\n', ()),
        ('#print "IcanHAVEmixedCASE"\n', ()),
        ('~"MeeToo"\n', ()),
        ('what: "ohNoes!"\n', ((1, 'non upper case string detected'),)),
        ('SpeLLinGLeSzOnZ:\n"mIgHtYhaxxxORZ"', ((2, 'non upper case string detected'), (2, 'new line missing'))),
    )
    def testWarnings(self, testData, mockPrint):
        filename = 'HerrFlick'
        path = os.path.join('Helga', filename)
        input, warnings = testData
        tokenizer = self.getTokenizerWithData(input, ensureNewLine=False, filePath=path)

        def checkForWarnings():
            if warnings:
                calls = [TestHelper.formatError(filename, line, f'warning: {warning}') for line, warning in warnings]
                calls = map(mock.call, calls)
                self.assertEqual(mockPrint.call_count, len(warnings))
                mockPrint.assert_has_calls(calls)
            else:
                mockPrint.assert_not_called()

        checkForWarnings()
        mockPrint.reset_mock()

        tokenizer = Tokenizer.empty()
        noWarningsInput = 'blabla = 0 #warningsOff ' + input
        TestTokenizer.runWithMockedOpen(lambda: tokenizer.tokenizeFile(filename), noWarningsInput, ensureNewLine=False)
        mockPrint.assert_not_called()

        withWarningsInput = ('#warningsOn ' if input else '') + input
        TestTokenizer.runWithMockedOpen(lambda: tokenizer.tokenizeFile(filename), withWarningsInput, ensureNewLine=False)
        checkForWarnings()

    def testUnterminatedString(self):
        errorMessage = TestHelper.formatError(TestHelper.kTestFilename, 1, 'unterminated string')
        TestHelper.assertExitWithError(self,
            lambda: self.getTokenizerWithData('"Heey, oops', filePath=TestHelper.kTestFilename),
            errorMessage)

    @mock.patch('builtins.print')
    def testStringEscapes(self, mockPrint):
        inputData = r'''
            'What\'s this? He asked.'
            "It's \"them\"."
            'Old O\'Donnel turned all colors.'
        '''
        tokenizer = self.getTokenizerWithData(inputData)
        for expectedString in (r"'What\'s this? He asked.'", '"It\'s \\"them\\"."', r"'Old O\'Donnel turned all colors.'"):
            self.assertEqual(tokenizer.getNextToken().string, expectedString)

    @data('+5*4/3-2')
    def testGetPreviousToken(self, inputData):
        tokenizer = self.getTokenizerWithData(inputData)

        for c in inputData:
            self.assertEqual(tokenizer.getNextToken().string, c)
            self.assertEqual(tokenizer.getPreviousToken().string, c)

    @data('MaxCoveri')
    def testGotTokensLeft(self, inputData):
        tokenizer = self.getTokenizerWithData(inputData)
        self.assertTrue(tokenizer.gotTokensLeft())
        self.assertEqual(tokenizer.getNumTokens(), 1)
        self.assertEqual(tokenizer.getNextToken().string, inputData)
        self.assertFalse(tokenizer.gotTokensLeft())
        self.assertEqual(tokenizer.getNumTokens(), 1)

    @data('DeepBlueSea')
    def testPeekToken(self, inputData):
        tokenizer = self.getTokenizerWithData('\n'.join(list(inputData)))
        for line, c in enumerate(inputData.replace('\n', ''), 1):
            token = tokenizer.peekNextToken()
            self.assertEqual(token.string, c)
            self.assertEqual(token, tokenizer.getNextToken())
            self.assertEqual(token.line, line)

        self.assertIsNone(tokenizer.peekNextToken())

    @data(('', ''), ('rainbow', 'a'), ('adventure', 'an'))
    def testEofHandling(self, testData):
        inputData = ':-)'
        tokenizer = self.getTokenizerWithData(inputData)

        for c in inputData:
            self.assertFalse(tokenizer.isAtEof())
            self.assertEqual(tokenizer.getNextToken().string, c)

        self.assertTrue(tokenizer.isAtEof())

        errorMessage = TestHelper.formatError(TestHelper.kTestFilename, 1, 'unexpected end of file')
        expectedOutput = errorMessage

        lookingFor, article = testData
        if lookingFor:
            expectedOutput += f' while looking for {article} {lookingFor}'

        TestHelper.assertExitWithError(self, lambda: tokenizer.getNextToken(lookingFor), expectedOutput)

    @data(
        ('// some comment', '"STRING1"', "'STRING2'", '|>>=', '<<=|',  '>>=', '<<=', '*=', '/=', '%=',
        '==', '^=', '!=', '++', '+=', '--', '-=', '|=', '&&', '||', '&=', '<<', '>>', '=>', '(', ')', '{',
        '}', '~', '?', ':', ',', '#', '>=', '<=', '|>>', '<<|')
    )
    def testTokenSpliting(self, tokens):
        input = ' \n  '.join(tokens)
        tokenizer = self.getTokenizerWithData(input)

        for line, atom in enumerate(tokens[1:], 2):
            token = tokenizer.getNextToken()
            self.assertEqual(atom, token.string)
            self.assertEqual(line, token.line)

        self.assertTrue(tokenizer.isAtEof())

    @data(
        ('+*^', Tokenizer.expect, 3, "expected `+', but got `*' instead"),
        ('{+*', Tokenizer.expectBlockStart, 2, "block start `{' expected, got `+'"),
        (':@$', Tokenizer.expectColon, 2, "expected colon `:', got `@'"),
        ('I&2', Tokenizer.expectIdentifier, 2, "`&' is not a valid identifier"),
    )
    def testExpect(self, testData):
        input, function, numParams, expectedError = testData

        tokenizer = self.getTokenizerWithData(input)

        token = tokenizer.getNextToken()
        self.assertEqual(token.string, input[0])

        params = [tokenizer, token]
        if numParams > 2:
            params[1:1] = input[0]

        token = function(*params)
        self.assertEqual(token.string, input[0])
        token = function(*params, fetchNextToken=True)
        self.assertEqual(token.string, input[1])

        expectedError = TestHelper.formatError(TestHelper.kTestFilename, 1, expectedError)
        params[-1] = token
        TestHelper.assertExitWithError(self, lambda: function(*params), expectedError)

    @data('{abc1')
    def testExpectIdentifier(self, inputData):
        assert inputData[0] == '{'
        tokenizer = self.getTokenizerWithData(' '.join(list(inputData)))

        token = tokenizer.peekNextToken()
        self.assertEqual(token.string, inputData[0])

        tokenizer.expectBlockStart(token, fetchNextToken=False)
        token = tokenizer.expectBlockStart(token, fetchNextToken=True)

        self.assertEqual(token.string, inputData[0])
        token = tokenizer.getNextToken()
        self.assertEqual(token.string, inputData[1])

        for i in range(2, len(inputData)):
            shouldBeSameToken = tokenizer.expectIdentifier(token, fetchNextToken=False)
            self.assertEqual(shouldBeSameToken, token)
            self.assertEqual(token.string, inputData[i - 1])
            token = tokenizer.expectIdentifier(token, fetchNextToken=True)
            self.assertEqual(token.string, inputData[i])

        expectedError = TestHelper.formatError(TestHelper.kTestFilename, 1, 'unexpected end of file')
        TestHelper.assertExitWithError(self, tokenizer.getNextToken, expectedError)

    @data('4^1^2^3')
    def testEditTokens(self, inputData):
        assert ' ' not in inputData
        tokenizer = self.getTokenizerWithData(inputData)

        self.assertEqual(tokenizer.getCurrentTokenIndex(), 0)
        self.assertEqual(tokenizer.peekNextToken().string, inputData[0])

        self.assertIsNone(tokenizer.peekTokenAt(-1))
        self.assertIsNone(tokenizer.peekTokenAt(5 * len(inputData)))

        middleIndex = len(inputData) // 2
        tokenizer.setCurrentTokenIndex(middleIndex)
        self.assertEqual(tokenizer.getNextToken().string, inputData[middleIndex])
        self.assertEqual(tokenizer.peekNextToken().string, inputData[middleIndex + 1])

        tokenizer.setCurrentTokenIndex(0)
        tokenRange = tokenizer.getTokenRange(0, len(inputData))
        self.assertEqual(len(tokenRange), len(inputData))

        for i, c in enumerate(inputData):
            self.assertEqual(i, tokenizer.getCurrentTokenIndex())
            token = tokenizer.getNextToken()
            self.assertEqual(token, tokenizer.peekTokenAt(i))
            self.assertEqual(token, tokenRange[i])

        self.assertTrue(tokenizer.isAtEof())
        tokenizer.setCurrentTokenIndex(0)
        self.assertFalse(tokenizer.isAtEof())

        token = tokenizer.peekNextToken()
        self.assertEqual(token.string, inputData[0])
        self.assertEqual(tokenizer.peekTokenAt(0), token)
        tokenizer.setTokenAt(1, token)
        self.assertEqual(tokenizer.peekTokenAt(0), tokenizer.peekTokenAt(1))

        for _ in range(2):
            nextToken = tokenizer.getNextToken()
            self.assertEqual(token, nextToken)

        nextToken = tokenizer.getNextToken()
        self.assertEqual(nextToken.string, inputData[2])

        self.assertEqual(tokenizer.getNumTokens(), len(inputData))
        tokenizer.deleteToken(0)
        self.assertEqual(tokenizer.getNumTokens(), len(inputData) - 1)
        tokenizer.deleteTokens(1, 3)
        self.assertEqual(tokenizer.getNumTokens(), len(inputData) - 3)

        tokenizer.setCurrentTokenIndex(1_000_000)
        tokenizer.deleteTokens(len(inputData) - 4, len(inputData) + 1000)
        self.assertEqual(tokenizer.getNumTokens(), len(inputData) - 4)
        self.assertEqual(tokenizer.getCurrentTokenIndex(), len(inputData) - 4)

        tokenizer.setCurrentTokenIndex(0)
        for c in inputData[0] + inputData[4:-1]:
            self.assertEqual(tokenizer.getNextToken().string, c)

    @data('a b c')
    def testSetTokenText(self, inputData):
        tokenizer = self.getTokenizerWithData(inputData)
        token = tokenizer.peekNextToken()
        tokenizer.setTokenTextAt(0, 'yosh')
        modifiedToken = tokenizer.getNextToken()
        self.assertNotEqual(token, modifiedToken)

    @staticmethod
    def getTokenizerWithData(data='', ensureNewLine=True, filePath=TestHelper.kTestFilename):
        return TestTokenizer.runWithMockedOpen(lambda: Tokenizer.fromFile(filePath), data, ensureNewLine)

    @staticmethod
    def runWithMockedOpen(callback, data='', ensureNewLine=True):
        return TestHelper.runWithMockedTokenizerOpen(callback, data, ensureNewLine)

import unittest
import functools

from ddt import ddt, data

from Token import Token
import Util
import TestHelper

@ddt
class TestUtil(unittest.TestCase):
    @data(
        ('switch', 'a swimming pad', True),
        ('rabbit', 'a fox hunt companion', False),
        ('private', 'a barbecue stand', True),
        ('friend', 'a whipping cream', True),
        ('bubble', 'a form of air transport', False),
        ('catch', 'a throw', True),
        ('throw', 'a catch', True),
        ('whistle', 'a way to catch fish', False),
        ('int', 'a stamina', True),
        ('power', 'a fishing tool', False),
        ('auto', 'a bicycle', True),
        ('weather', 'an excuse', False),
        ('import', 'an export', True),
        ('default', 'a get-out-of-dept for free card', True),
        ('imaginary', 'an escapism', False),
        ('float', 'an ocean exploring tool', True),
        ('this', 'a that', True),
        ('landing gear', 'a propellant', False),
        ('module', 'a space station', True),
        ('case', 'a riot barricade', True),
        ('bucket', 'an anti-tank armour', False),
        ('register', 'a breaking and entering tool', True),
        ('double', 'an errand boy', True),
        ('union', 'a private enterprise', True),
    )
    def testVerifyNonCppKeyword(self, testData):
        word, context, isCppKeyword = testData

        expectedToken = Token(word, 'heaven', 22)
        errored = False

        def errorHandler(actualErrorStr, actualToken):
            self.assertEqual(actualToken, expectedToken)

            expectedErrorStr = f"can't use a C++ keyword `{expectedToken.string}'"
            if context:
                expectedErrorStr += f' as {context}'
            self.assertEqual(actualErrorStr, expectedErrorStr)

            nonlocal errored
            errored = True

        Util.error = errorHandler
        Util.verifyNonCppKeyword(expectedToken.string, expectedToken, context)
        self.assertEqual(isCppKeyword, errored)

    @data(
        (1, 1, -1, 2),
        (1, -1, -1, 0x80000000),
        (1, -1, 5, 32),
        (-1, -1, 10, -1),
        (1, 1, 1, 0x80000000),
        (1, -1, 1, 2),
        (0x12345678, 1, -1, 0x2468acf0),
        (0x12345678, 1, 1, 0x91a2b3c),
    )
    def testRotate(self, testData):
        value, leftOrRight, amount, expectedValue = testData
        if leftOrRight < 0:
            actualValue = Util.rotateLeft(value, amount)
        else:
            actualValue = Util.rotateRight(value, amount)

        self.assertEqual(actualValue & 0xffffffff, expectedValue & 0xffffffff)

    @data(
        ('empty', None, None, False, 'empty'),
        ('somebody stop me', None, None, True, 'warning: somebody stop me'),
        ("it's the way of the world", r'1\2\3\stop', None, False, "stop: it's the way of the world"),
        ("it's the way of the worlds", r'1\2\3\stop', None, True, "stop: warning: it's the way of the worlds"),
        ("it's the way of the word", r'1\2\3\stop', 25, False, "stop(25): it's the way of the word"),
        ("it's the way of the worded", r'1\2\3\stop', 19, True, "stop(19): warning: it's the way of the worded"),
        ("it's the way how it's worded", Token('bump', '18/and/life', 18), None, False, "life(18): it's the way how it's worded"),
        ("it's in the way how it's worded", Token('bump', "17/she's/only", 17), None, False, "only(17): it's in the way how it's worded"),
        ("it's in the way of the wording", Token('bump', "iii/override", 128), 256, False, "override(256): it's in the way of the wording"),
    )
    def testFormatMessage(self, testData):
        text, pathOrToken, line, isWarning, expectedOutput = testData
        actualOutput = Util.formatMessage(text, pathOrToken, line, isWarning)
        self.assertEqual(actualOutput, expectedOutput)

    @data(
        ('true', True, None),
        ('false', False, None),
        ('0', False, None),
        ('565', True, None),
        ('swoosh', True, "`swoosh' is not a numeric value'"),
        ('True', True, None),
        ('False', False, None),
        ('FaIse', False, "`FaIse' is not a numeric value'"),
    )
    def testGetBoolValue(self, testData):
        input, expectedValue, errorMessage = testData
        token = Token(input)

        if errorMessage:
            testFunction = functools.partial(Util.getBoolValue, token)
            TestHelper.assertExitWithError(self, testFunction, errorMessage)
        else:
            actualValue = Util.getBoolValue(token)
            self.assertEqual(actualValue, expectedValue)

    @data(
        ('balloon', 'a balloon'),
        ('a balloon', 'a balloon'),
        ('1 balloon', '1 balloon'),
        ('2 balloons', '2 balloons'),
        ('hands', 'hands'),
        ('air-glider', 'an air-glider'),
        ('the Flintstones', 'the Flintstones'),
        ('a robot', 'a robot'),
        ('crepes', 'crepes'),
        ('a', 'a'),
        ('bat mobile', 'a bat mobile'),
        ('Samurai', 'a Samurai'),
        ('entry alias', 'an entry alias'),
        ('canvas', 'a canvas'),
    )
    def testArticle(self, testData):
        self.doInputOuputTest(testData, Util.withArticle)

    @data(
        ('()', ''),
        ('(())', ''),
        ('((()))', ''),
        ('(unicorn)', 'unicorn'),
        ('((pooping))', 'pooping'),
        ('(((rainbows)))', 'rainbows'),
    )
    def testRemoveEnclosingParentheses(self, testData):
        self.doInputOuputTest(testData, Util.removeEnclosingParentheses)

    @data(
        ('""', ''),
        ("''", ''),
        ('"haug"', 'haug'),
        ('""moustache""', '"moustache"'),
        ('"One eyed Joe', '"One eyed Joe'),
        ("'Limpy Loody", "'Limpy Loody"),
        ('""""flying""""', '"""flying"""'),
        ('"""""grounded""""', '""""grounded"""'),
        ('"lazy eye\'', '"lazy eye\''),
    )
    def testUnquotedString(self, testData):
        self.doInputOuputTest(testData, Util.unquotedString)

    @data(
        ('"yes"', True),
        ('"no', False),
        ('no, really', False),
        ("'OK, yes'", True),
        ("''definitely''", True),
        ("''even so'", True),
        ('"a bit here, a bit there""', True),
        ("''", True),
        ('""', True),
    )
    def testIsString(self, testData):
        self.doInputOuputTest(testData, Util.isString)

    def doInputOuputTest(self, testData, transform):
        assert isinstance(testData, (list, tuple))
        assert callable(transform)

        input, expectedOutput = testData
        actualOutput = transform(input)
        self.assertEqual(actualOutput, expectedOutput)

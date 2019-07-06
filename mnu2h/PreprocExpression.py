import Util

from Token import Token
from Menu import Menu
from Variable import Variable
from Tokenizer import Tokenizer
from VariableStorage import VariableStorage

class PreprocExpression:
    def __init__(self, tokenizer, varStorage):
        assert isinstance(tokenizer, Tokenizer)
        assert isinstance(varStorage, VariableStorage)

        self.tokenizer = tokenizer
        self.varStorage = varStorage

    # parse
    #
    # in:
    #     menu  - parent menu, if any
    #
    # Parses preprocessor expression. Returns the top level parsed node list.
    # While evaluating expands all variables known until this moment.
    #
    def parse(self, menu=None):
        nodes = self.assignmentExpression(menu)

        while self.peekNextTokenString() == kComma.text:
            # unlike C, we will allow comma operator to return lvalues (simply if the right operand is lvalue)
            opToken = self.tokenizer.getNextToken()
            nextNodes = self.assignmentExpression(menu)
            commaOperator = OperatorNode(kComma, opToken, nextNodes[0].isLvalue())
            nodes = [ commaOperator ] + nodes + nextNodes

        return nodes

    # evaluate
    #
    # in:
    #     nodes - parsed list of nodes in Polish notation (assumed to be correct)
    #
    # Evaluates node list and returns the integer result.
    #
    @staticmethod
    def evaluate(nodes):
        assert nodes and isinstance(nodes, list)

        node = nodes[0]
        nodes = nodes[1:]

        if isinstance(node, OperandNode):
            return PreprocExpression.evaluateOperand(node, nodes)
        elif isinstance(node, OperatorNode):
            if node.type.numOperands == 1:
                return PreprocExpression.evaluateUnaryOperator(node, nodes)
            elif node.type.numOperands == 2:
                return PreprocExpression.evaluateBinaryOperator(node, nodes)
            elif node.type.numOperands == 3:
                return PreprocExpression.evaluateTernaryOperator(node, nodes)
            else:
                assert 0
        else:
            assert 0

        return 0, nodes, None

    def createSingleOperandExpression(self, token, menu, variable):
        assert isinstance(token, Token)
        assert isinstance(menu, (Menu, type(None)))
        assert isinstance(variable, Variable)

        node = OperandNode(token, menu, self.varStorage, variable)
        return [ node ]

    @staticmethod
    def evaluateOperand(node, nodes):
        assert isinstance(node, OperandNode)

        node.update()
        return node.value, nodes, node.var

    @staticmethod
    def evaluateUnaryOperator(node, nodes):
        assert isinstance(node, OperatorNode) and node.type.numOperands == 1

        value, nodes, var = PreprocExpression.evaluate(nodes)
        value = node.type.op(value)

        if node.type.expectsLvalue:
            assert not node.isLvalue()
            assert var

            var.value = value

            if node.type == kPostDecrement:
                var.value -= 1
            elif node.type == kPostIncrement:
                var.value += 1

        return value, nodes, var

    @staticmethod
    def evaluateBinaryOperator(node, nodes):
        assert isinstance(node, OperatorNode) and node.type.numOperands == 2

        val1, nodes, var = PreprocExpression.evaluate(nodes)

        val2Token = next(node.token for node in nodes if isinstance(node, OperandNode))
        val2, nodes, var2 = PreprocExpression.handleShortCircuitEvaluation(val1, node, nodes)

        PreprocExpression.checkForDivisionByZero(val2, val2Token, node, nodes)

        value = node.type.op(val1, val2)

        if node.type.expectsLvalue:
            assert var
            var.value = value
        elif node.type == kComma:
            var = var2

        return value, nodes, var

    @staticmethod
    def evaluateTernaryOperator(node, nodes):
        assert isinstance(node, OperatorNode) and node.type.numOperands == 3

        conditionValue, nodes, var = PreprocExpression.evaluate(nodes)
        if conditionValue:
            value, nodes, var = PreprocExpression.evaluate(nodes)
            nodes = PreprocExpression.skipExpression(nodes)
        else:
            nodes = PreprocExpression.skipExpression(nodes)
            value, nodes, var = PreprocExpression.evaluate(nodes)

        return value, nodes, var

    @staticmethod
    def handleShortCircuitEvaluation(val1, node, nodes):
        assert isinstance(val1, int)
        assert isinstance(node, OperatorNode)
        assert isinstance(nodes, list)

        var2 = None

        if (node.type == kLogicalAnd and not val1) or (node.type == kLogicalOr and val1):
            val2, nodes = 0, PreprocExpression.skipExpression(nodes)
        else:
            val2, nodes, var2 = PreprocExpression.evaluate(nodes)

        return val2, nodes, var2

    @staticmethod
    def checkForDivisionByZero(val2, val2Token, node, nodes):
        assert isinstance(val2, int)
        assert isinstance(val2Token, Token)
        assert isinstance(node, OperatorNode)
        assert isinstance(nodes, list)

        if not val2 and node.type in (kDivision, kDivEq, kModuo, kModEq):
            Util.error('division by zero', val2Token)

    @staticmethod
    def skipExpression(nodes):
        if isinstance(nodes[0], OperandNode):
            return nodes[1:]

        numOperands = nodes[0].type.numOperands
        nodes = nodes[1:]

        for _ in range(0, numOperands):
            nodes = PreprocExpression.skipExpression(nodes)

        return nodes

    #
    # Compile-time expressions recursive descent parser
    #

    def assignmentExpression(self, menu):
        nodes = self.conditionalExpression(menu)

        nextTokenString = self.peekNextTokenString()
        op = kAssignmentOperators.get(nextTokenString)

        if op:
            opToken = self.tokenizer.getNextToken()
            self.verifyLvalue(nodes[0], op.text, opToken)

            nextNodes = self.assignmentExpression(menu)
            nodes = [ OperatorNode(op, opToken) ] + nodes + nextNodes

        return nodes

    def conditionalExpression(self, menu):
        nodes = self.logicalOrExpression(menu)

        if self.peekNextTokenString() == kTernaryOp.text:
            opToken = self.tokenizer.getNextToken()
            nodes1 = self.parse(menu)

            token = self.tokenizer.getNextToken(':')
            self.tokenizer.expect(':', token, fetchNextToken=False)

            nodes2 = self.conditionalExpression(menu)

            op = OperatorNode(kTernaryOp, opToken, nodes1[0].isLvalue() and nodes2[0].isLvalue())
            nodes = [ op ] + nodes + nodes1 + nodes2

        return nodes

    def logicalOrExpression(self, menu):
        nodes = self.logicalAndExpression(menu)

        while self.peekNextTokenString() == kLogicalOr.text:
            opToken = self.tokenizer.getNextToken()
            nextNodes = self.logicalAndExpression(menu)
            nodes = [ OperatorNode(kLogicalOr, opToken) ] + nodes + nextNodes

        return nodes

    def logicalAndExpression(self, menu):
        nodes = self.inclusiveOrExpression(menu)

        while self.peekNextTokenString() == kLogicalAnd.text:
            opToken = self.tokenizer.getNextToken()
            nextNodes = self.inclusiveOrExpression(menu)
            nodes = [ OperatorNode(kLogicalAnd, opToken) ] + nodes + nextNodes

        return nodes

    def inclusiveOrExpression(self, menu):
        nodes = self.exclusiveOrExpression(menu)

        while self.peekNextTokenString() == kOr.text:
            opToken = self.tokenizer.getNextToken()
            nextNodes = self.exclusiveOrExpression(menu)
            nodes = [ OperatorNode(kOr, opToken) ] + nodes + nextNodes

        return nodes

    def exclusiveOrExpression(self, menu):
        nodes = self.andExpression(menu)

        while self.peekNextTokenString() == kXor.text:
            opToken = self.tokenizer.getNextToken()
            nextNodes = self.andExpression(menu)
            nodes = [ OperatorNode(kXor, opToken) ] + nodes + nextNodes

        return nodes

    def andExpression(self, menu):
        nodes = self.equalityExpression(menu)

        while self.peekNextTokenString() == kAnd.text:
            opToken = self.tokenizer.getNextToken()
            nextNodes = self.equalityExpression(menu)
            nodes = [ OperatorNode(kAnd, opToken) ] + nodes + nextNodes

        return nodes

    def equalityExpression(self, menu):
        nodes = self.relationalExpression(menu)

        while True:
            nextTokenString = self.peekNextTokenString()
            op = kEqualityOperators.get(nextTokenString)
            if not op:
                break

            opToken = self.tokenizer.getNextToken()
            nextNodes= self.relationalExpression(menu)
            nodes = [ OperatorNode(op, opToken) ] + nodes + nextNodes

        return nodes

    def relationalExpression(self, menu):
        nodes = self.shiftExpression(menu)

        while True:
            nextTokenString = self.peekNextTokenString()
            op = kRelationalOps.get(nextTokenString)
            if not op:
                break

            opToken = self.tokenizer.getNextToken()
            nextNodes = self.shiftExpression(menu)
            nodes = [ OperatorNode(op, opToken) ] + nodes + nextNodes

        return nodes

    def shiftExpression(self, menu):
        nodes = self.additiveExpression(menu)

        while True:
            nextTokenString = self.peekNextTokenString()
            op = kShiftOps.get(nextTokenString)
            if not op:
                break

            opToken = self.tokenizer.getNextToken()
            nextNodes = self.additiveExpression(menu)
            nodes = [ OperatorNode(op, opToken) ] + nodes + nextNodes

        return nodes

    def additiveExpression(self, menu):
        nodes = self.multiplicativeExpression(menu)

        while True:
            nextTokenString = self.peekNextTokenString()
            op = kAddOps.get(nextTokenString)
            if not op:
                break

            opToken = self.tokenizer.getNextToken()
            nextNodes = self.multiplicativeExpression(menu)
            nodes = [ OperatorNode(op, opToken) ] + nodes + nextNodes

        return nodes

    def multiplicativeExpression(self, menu):
        nodes = self.unaryExpression(menu)

        while True:
            nextTokenString = self.peekNextTokenString()
            op = kMulOps.get(nextTokenString)
            if not op:
                break

            opToken = self.tokenizer.getNextToken()
            nextNodes = self.unaryExpression(menu)
            nodes = [ OperatorNode(op, opToken) ] + nodes + nextNodes

        return nodes

    def unaryExpression(self, menu):
        nextTokenString = self.peekNextTokenString()
        op = kUnaryOps.get(nextTokenString)

        if op:
            opToken = self.tokenizer.getNextToken()
            nodes = self.unaryExpression(menu)
            if op in (kPreDecrement, kPostDecrement, kPreIncrement, kPostIncrement):
                self.verifyLvalue(nodes[0], op.text, opToken)
            nodes = [ OperatorNode(op, opToken) ] + nodes
        else:
            nodes = self.postfixExpression(menu)

        return nodes

    def postfixExpression(self, menu):
        nodes = self.primaryExpression(menu)

        while True:
            nextTokenString = self.peekNextTokenString()
            op = kPostfixOps.get(nextTokenString)
            if not op:
                break

            opToken = self.tokenizer.getNextToken()
            self.verifyLvalue(nodes[0], op.text, opToken)
            nodes = [ OperatorNode(op, opToken) ] + nodes

        return nodes

    def primaryExpression(self, menu):
        token = self.tokenizer.peekNextToken()
        if token and token.string == '(':
            self.tokenizer.getNextToken()
            nodes = self.parse(menu)
            token = self.tokenizer.getNextToken(')')
            self.tokenizer.expect(')', token, fetchNextToken=False)
        else:
            self.tokenizer.getNextToken('primary expression')
            try:
                nodes = [ OperandNode(token, menu, self.varStorage) ]
            except ValueError:
                tokenOrString = 'string' if Util.isString(token.string) else 'token'
                Util.error(f"unable to convert {tokenOrString} `{token.string}' to integer", token)

        return nodes

    def peekNextTokenString(self):
        if self.tokenizer.gotTokensLeft():
            return self.tokenizer.peekNextToken().string
        else:
            return ''

    @staticmethod
    def verifyLvalue(node, op, opToken):
        assert isinstance(node, (OperatorNode, OperandNode))
        assert isinstance(op, str)
        assert isinstance(opToken, Token)

        if not node.isLvalue():
            Util.error(f"operator `{op}' expects lvalue", opToken)

#
# Auxiliary classes for expression evaluation
#

class Operator:
    def __init__(self, text, numOperands, expectsLvalue, op):
        assert isinstance(text, str)
        assert isinstance(numOperands, int)
        assert callable(op)

        self.text = text
        self.numOperands = numOperands
        self.expectsLvalue = expectsLvalue
        self.op = op

    def __repr__(self):
        return f'Operator {self.text}, operands: {self.numOperands}'

kBitwiseNot     = Operator('~',    1, False, lambda v: ~v)
kLogicalNot     = Operator('!',    1, False, lambda v: int(not v))
kUnaryMinus     = Operator('-',    1, False, lambda v: -v)
kPreIncrement   = Operator('++',   1, True,  lambda v: v + 1)
kPostIncrement  = Operator('++',   1, True,  lambda v: v)
kPreDecrement   = Operator('--',   1, True,  lambda v: v - 1)
kPostDecrement  = Operator('--',   1, True,  lambda v: v)
kComma          = Operator(',',    2, False, lambda v1, v2: v2)
kTernaryOp      = Operator('?',    3, False, lambda v1, v2, v3: v2 if v1 else v3)
kLogicalOr      = Operator('||',   2, False, lambda v1, v2: int(v1 != 0 or v2 != 0))
kLogicalAnd     = Operator('&&',   2, False, lambda v1, v2: int(v1 != 0 and v2 != 0))
kOr             = Operator('|',    2, False, lambda v1, v2: v1 | v2)
kXor            = Operator('^',    2, False, lambda v1, v2: v1 ^ v2)
kAnd            = Operator('&',    2, False, lambda v1, v2: v1 & v2)
kEqual          = Operator('==',   2, False, lambda v1, v2: int(v1 == v2))
kNotEqual       = Operator('!=',   2, False, lambda v1, v2: int(v1 != v2))
kAddition       = Operator('+',    2, False, lambda v1, v2: v1 + v2)
kSubtraction    = Operator('-',    2, False, lambda v1, v2: v1 - v2)
kLessThan       = Operator('<',    2, False, lambda v1, v2: int(v1 < v2))
kGreaterThan    = Operator('>',    2, False, lambda v1, v2: int(v1 > v2))
kLessOrEqual    = Operator('<=',   2, False, lambda v1, v2: int(v1 <= v2))
kGreaterOrEqual = Operator('>=',   2, False, lambda v1, v2: int(v1 >= v2))
kLeftShift      = Operator('<<',   2, False, lambda v1, v2: v1 << v2)
kRightShift     = Operator('>>',   2, False, lambda v1, v2: v1 >> v2)
kLeftRotate     = Operator('<<|',  2, False, Util.rotateLeft)
kRightRotate    = Operator('|>>',  2, False, Util.rotateRight)
kMultiplication = Operator('*',    2, False, lambda v1, v2: v1 * v2)
kDivision       = Operator('/',    2, False, lambda v1, v2: v1 // v2)
kModuo          = Operator('%',    2, False, lambda v1, v2: v1 % v2)
kAssignment     = Operator('=',    2, True,  lambda v1, v2: v2)
kPlusEq         = Operator('+=',   2, True,  kAddition.op)
kMinusEq        = Operator('-=',   2, True,  kSubtraction.op)
kMulEq          = Operator('*=',   2, True,  kMultiplication.op)
kDivEq          = Operator('/=',   2, True,  kDivision.op)
kModEq          = Operator('%=',   2, True,  kModuo.op)
kLeftShiftEq    = Operator('<<=',  2, True,  kLeftShift.op)
kLeftRotateEq   = Operator('<<=|', 2, True,  kLeftRotate.op)
kRightShiftEq   = Operator('>>=',  2, True,  kRightShift.op)
kRightRotateEq  = Operator('|>>=', 2, True,  kRightRotate.op)
kAndEq          = Operator('&=',   2, True,  kAnd.op)
kXorEq          = Operator('^=',   2, True,  kXor.op)
kOrEq           = Operator('|=',   2, True,  kOr.op)

makeOpDict = lambda *opList: { op.text : op for op in opList }

kEqualityOperators = makeOpDict(kEqual, kNotEqual)
kRelationalOps = makeOpDict(kLessThan, kGreaterThan, kLessOrEqual, kGreaterOrEqual)
kAssignmentOperators = makeOpDict(kPlusEq, kMinusEq, kMulEq, kDivEq, kModEq, kLeftShiftEq,
    kLeftRotateEq, kRightShiftEq, kRightRotateEq, kAndEq, kXorEq, kOrEq, kAssignment)
kShiftOps = makeOpDict(kLeftShift, kRightShift, kLeftRotate, kRightRotate)
kAddOps = makeOpDict(kAddition, kSubtraction)
kMulOps = makeOpDict(kMultiplication, kDivision, kModuo)
kUnaryOps = makeOpDict(kBitwiseNot, kLogicalNot, kUnaryMinus, kPreIncrement, kPreDecrement)
kPostfixOps = makeOpDict(kPostDecrement, kPostIncrement)

class OperatorNode:
    def __init__(self, type, token, isLvalue=False):
        assert isinstance(type, Operator)
        assert isinstance(token, Token)

        self.type = type
        self.token = token
        self.lvalue = isLvalue
        self.uninitialized = False

    def __repr__(self):
        return f'<{self.type}, lvalue: {self.lvalue}>'

    def isLvalue(self):
        return self.lvalue

class OperandNode:
    def __init__(self, token, menu, varStorage, var=None):
        assert isinstance(token, Token)
        assert isinstance(menu, (Menu, type(None)))
        assert isinstance(varStorage, VariableStorage)

        self.token = token
        self.var = var
        self.uninitialized = False

        if not var:
            if token.string[0] == '@':
                self.value = varStorage.expandBuiltinVariable(token.string, token, menu)
                self.value = Util.removeEnclosingParentheses(self.value)
                self.value = int(self.value, 0)
            elif token.string.isidentifier():
                if menu:
                    var = varStorage.getMenuVariable(menu, token.string)
                if not var:
                    var = varStorage.getGlobalVariable(token.string)
                if not var:
                    var = varStorage.getPreprocVariable(token.string)

                if not var:
                    var = varStorage.addPreprocVariable(token.string, 0, token)
                    self.uninitialized = True
                else:
                    var.referenced = True

                self.var = var
                self.value = var.value

                if not isinstance(self.value, int):
                    if isinstance(self.value, str) and self.value.startswith('('):
                        tokenizer = Tokenizer.empty()
                        tokenizer.resetToString(self.value, token)
                        parser = PreprocExpression(tokenizer, varStorage)
                        nodes = parser.parse(menu)
                        self.value = PreprocExpression.evaluate(nodes)[0]
                        self.var = None

                    value = Util.unquotedString(str(self.value))
                    self.value = int(value, 0)
            else:
                self.value = Util.unquotedString(token.string)
                self.value = int(self.value, 0)
        else:
            self.value = var.value

    def __repr__(self):
        return self.var.__repr__() if self.var else f'<{self.value}, {self.token}>'

    def isLvalue(self):
        return bool(self.var)

    def setConstant(self, value):
        self.value = value
        self.var = None

    def update(self):
        if self.var and self.value != self.var.value:
            self.value = self.var.value
            if type(self.value) != int:
                try:
                    value = Util.unquotedString(self.var.value)
                    self.value = int(value, 0)
                except ValueError:
                    Util.error(f"`{self.var.value}' is not an integer value", self.token)

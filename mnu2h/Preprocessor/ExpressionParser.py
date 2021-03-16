"""Compile-time expressions recursive descent parser."""

import Util

from Menu import Menu
from Token import Token
from Tokenizer import Tokenizer
from VariableStorage import VariableStorage

from Preprocessor.Operator import *
from Preprocessor.OperandNode import OperandNode
from Preprocessor.OperandNode import InvalidOperandNode
from Preprocessor.OperatorNode import OperatorNode
from Preprocessor.ExpressionEvaluator import ExpressionEvaluator
from Preprocessor.ExpressionEvaluator import EvaluationError

class InvalidOperand(Exception):
    def __init__(self, message):
        super().__init__(message)

class ExpressionParser:
    def __init__(self, tokenizer, varStorage, showWarnings, resolving=False):
        assert isinstance(tokenizer, Tokenizer)
        assert isinstance(varStorage, VariableStorage)
        assert callable(showWarnings)

        self.tokenizer = tokenizer
        self.varStorage = varStorage
        self.showWarnings = showWarnings
        self.resolving = resolving

        self.menu = None
        self.varTokenizer = Tokenizer.empty()

    # parse
    #
    # in:
    #     menu  - parent menu, if any
    #
    # Parses preprocessor expression. Returns the top level parsed node list.
    # While evaluating expands all variables known until this moment.
    #
    def parse(self, menu=None):
        assert isinstance(menu, (Menu, type(None)))

        self.menu = menu
        nodes = self.assignmentExpression()

        while self.peekNextTokenString() == kComma.text:
            # unlike C, we will allow the comma operator to return l-values (simply if the rightmost operand is an l-value)
            opToken = self.tokenizer.getNextToken()
            nextNodes = self.assignmentExpression()
            commaOperator = OperatorNode(kComma, opToken, nextNodes[0].isLvalue())
            nodes = [ commaOperator ] + nodes + nextNodes

        return nodes

    def assignmentExpression(self):
        nodes = self.conditionalExpression()

        nextTokenString = self.peekNextTokenString()
        op = kAssignmentOperators.get(nextTokenString)

        if op:
            opToken = self.tokenizer.getNextToken()
            self.verifyLvalue(nodes[0], op.text, opToken)

            nextNodes = self.assignmentExpression()
            nodes = [ OperatorNode(op, opToken) ] + nodes + nextNodes

        return nodes

    def conditionalExpression(self):
        nodes = self.logicalOrExpression()

        if self.peekNextTokenString() == kTernaryOp.text:
            opToken = self.tokenizer.getNextToken()
            nodes1 = self.parse(self.menu)

            token = self.tokenizer.getNextToken(':')
            self.tokenizer.expect(':', token)

            nodes2 = self.conditionalExpression()

            op = OperatorNode(kTernaryOp, opToken, nodes1[0].isLvalue() and nodes2[0].isLvalue())
            nodes = [ op ] + nodes + nodes1 + nodes2

        return nodes

    def logicalOrExpression(self):
        nodes = self.logicalAndExpression()

        while self.peekNextTokenString() == kLogicalOr.text:
            opToken = self.tokenizer.getNextToken()
            nextNodes = self.logicalAndExpression()
            nodes = [ OperatorNode(kLogicalOr, opToken) ] + nodes + nextNodes

        return nodes

    def logicalAndExpression(self):
        nodes = self.inclusiveOrExpression()

        while self.peekNextTokenString() == kLogicalAnd.text:
            opToken = self.tokenizer.getNextToken()
            nextNodes = self.inclusiveOrExpression()
            nodes = [ OperatorNode(kLogicalAnd, opToken) ] + nodes + nextNodes

        return nodes

    def inclusiveOrExpression(self):
        nodes = self.exclusiveOrExpression()

        while self.peekNextTokenString() == kOr.text:
            opToken = self.tokenizer.getNextToken()
            nextNodes = self.exclusiveOrExpression()
            nodes = [ OperatorNode(kOr, opToken) ] + nodes + nextNodes

        return nodes

    def exclusiveOrExpression(self):
        nodes = self.andExpression()

        while self.peekNextTokenString() == kXor.text:
            opToken = self.tokenizer.getNextToken()
            nextNodes = self.andExpression()
            nodes = [ OperatorNode(kXor, opToken) ] + nodes + nextNodes

        return nodes

    def andExpression(self):
        nodes = self.equalityExpression()

        while self.peekNextTokenString() == kAnd.text:
            opToken = self.tokenizer.getNextToken()
            nextNodes = self.equalityExpression()
            nodes = [ OperatorNode(kAnd, opToken) ] + nodes + nextNodes

        return nodes

    def equalityExpression(self):
        nodes = self.relationalExpression()

        while True:
            nextTokenString = self.peekNextTokenString()
            op = kEqualityOperators.get(nextTokenString)
            if not op:
                break

            opToken = self.tokenizer.getNextToken()
            nextNodes= self.relationalExpression()
            nodes = [ OperatorNode(op, opToken) ] + nodes + nextNodes

        return nodes

    def relationalExpression(self):
        nodes = self.shiftExpression()

        while True:
            nextTokenString = self.peekNextTokenString()
            op = kRelationalOps.get(nextTokenString)
            if not op:
                break

            opToken = self.tokenizer.getNextToken()
            nextNodes = self.shiftExpression()
            nodes = [ OperatorNode(op, opToken) ] + nodes + nextNodes

        return nodes

    def shiftExpression(self):
        nodes = self.additiveExpression()

        while True:
            nextTokenString = self.peekNextTokenString()
            op = kShiftOps.get(nextTokenString)
            if not op:
                break

            opToken = self.tokenizer.getNextToken()
            nextNodes = self.additiveExpression()
            nodes = [ OperatorNode(op, opToken) ] + nodes + nextNodes

        return nodes

    def additiveExpression(self):
        nodes = self.multiplicativeExpression()

        while True:
            nextTokenString = self.peekNextTokenString()
            op = kAddOps.get(nextTokenString)
            if not op:
                break

            opToken = self.tokenizer.getNextToken()
            nextNodes = self.multiplicativeExpression()
            nodes = [ OperatorNode(op, opToken) ] + nodes + nextNodes

        return nodes

    def multiplicativeExpression(self):
        nodes = self.unaryExpression()

        while True:
            nextTokenString = self.peekNextTokenString()
            op = kMulOps.get(nextTokenString)
            if not op:
                break

            opToken = self.tokenizer.getNextToken()
            nextNodes = self.unaryExpression()
            nodes = [ OperatorNode(op, opToken) ] + nodes + nextNodes

        return nodes

    def unaryExpression(self):
        nextTokenString = self.peekNextTokenString()
        op = kUnaryOps.get(nextTokenString)

        if op:
            opToken = self.tokenizer.getNextToken()
            nodes = self.unaryExpression()
            if op.expectsLvalue:
                self.verifyLvalue(nodes[0], op.text, opToken)
            nodes = [ OperatorNode(op, opToken) ] + nodes
        else:
            if nextTokenString == ':':
                self.handleSymbol()
            nodes = self.postfixExpression()

        return nodes

    def handleSymbol(self):
        self.tokenizer.getNextToken()
        idToken = self.tokenizer.getNextToken()
        self.tokenizer.expectIdentifier(idToken)
        idToken.string = '"' + idToken.string + '"'
        self.tokenizer.ungetToken()

    def postfixExpression(self):
        nodes = self.primaryExpression()

        while True:
            nextTokenString = self.peekNextTokenString()
            op = kPostfixOps.get(nextTokenString)
            if not op:
                break

            opToken = self.tokenizer.getNextToken()
            if op.expectsLvalue:
                self.verifyLvalue(nodes[0], op.text, opToken)
            if op == kIndex:
                indexNodes = self.parse(self.menu)
                token = self.tokenizer.getNextToken(']')
                self.tokenizer.expect(']', token)
                nodes += indexNodes

            nodes = [ OperatorNode(op, opToken) ] + nodes

        return nodes

    def primaryExpression(self):
        token = self.tokenizer.peekNextToken()

        if token and token.string == '(':
            self.tokenizer.getNextToken()
            nodes = self.parse(self.menu)
            token = self.tokenizer.getNextToken(')')
            self.tokenizer.expect(')', token)
        else:
            self.tokenizer.getNextToken('primary expression')
            try:
                node = OperandNode(token, self.menu, self.varStorage)
            except InvalidOperandNode as invalidNode:
                if invalidNode.node.var:
                    try:
                        node = self.parseVariableContentsAsExpression(invalidNode.node)
                    except InvalidOperand as opError:
                        Util.error(f"in variable `{invalidNode.node.var.name}': {opError}", token)
                    except EvaluationError as evalError:
                        Util.error(f"in variable `{invalidNode.node.var.name}': {evalError}", token)
                else:
                    if token.string == '}':
                        errorMessage = 'unexpected end of expression'
                    else:
                        errorMessage = f"`{invalidNode.node.value}' is not a valid operand"
                    if not self.resolving:
                        Util.error(errorMessage, token)
                    else:
                        raise InvalidOperand(errorMessage)

            nodes = [ node ]

        return nodes

    def parseVariableContentsAsExpression(self, node):
        assert isinstance(node, OperandNode)
        assert node.var

        self.varTokenizer.resetToString(node.value, node.token)
        parser = ExpressionParser(self.varTokenizer, self.varStorage, self.showWarnings, True)
        nodes = parser.parse(self.menu)
        self.showUninitializedVariablesWarnings(nodes, node.var.name, node.token)
        value = ExpressionEvaluator.evaluate(nodes)[0]
        node.setConstant(value)
        return node

    def showUninitializedVariablesWarnings(self, nodes, varName, varToken):
        assert isinstance(nodes, (list, tuple))
        assert isinstance(varName, str)
        assert isinstance(varToken, Token)

        if not self.showWarnings():
            return

        uninitializedVariables = []

        for node in nodes:
            if node.uninitialized:
                uninitializedVariables.append('`' + node.var.name + "'")

        if uninitializedVariables:
            warningTextStart = f"from variable `{varName}': using uninitialized variable"
            if len(uninitializedVariables) == 1:
                Util.warning(warningTextStart + ' ' + uninitializedVariables[0], varToken)
            else:
                varNames = ', '.join(uninitializedVariables)
                Util.warning(warningTextStart + 's: ' + varNames, varToken)

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

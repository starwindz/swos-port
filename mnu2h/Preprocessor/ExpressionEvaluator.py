import Util

from Menu import Menu
from Token import Token
from Variable import Variable
from VariableStorage import VariableStorage

from Preprocessor.Operator import *
from Preprocessor.OperandNode import OperandNode
from Preprocessor.OperatorNode import OperatorNode

class EvaluationError(Exception):
    def __init__(self, message, token):
        super().__init__(message)
        self.token = token

class ExpressionEvaluator:
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
            return ExpressionEvaluator.evaluateOperand(node, nodes)
        elif isinstance(node, OperatorNode):
            if node.type.numOperands == 1:
                return ExpressionEvaluator.evaluateUnaryOperator(node, nodes)
            elif node.type.numOperands == 2:
                return ExpressionEvaluator.evaluateBinaryOperator(node, nodes)
            elif node.type.numOperands == 3:
                return ExpressionEvaluator.evaluateTernaryOperator(node, nodes)
            else:
                assert 0
        else:
            assert 0

        return 0, nodes, None

    @staticmethod
    def createSingleOperandExpression(token, menu, variable, varStorage):
        assert isinstance(token, Token)
        assert isinstance(menu, (Menu, type(None)))
        assert isinstance(variable, Variable)
        assert isinstance(varStorage, VariableStorage)

        node = OperandNode(token, menu, varStorage, variable)
        return [ node ]

    @staticmethod
    def evaluateOperand(node, nodes):
        assert isinstance(node, OperandNode)

        node.update()
        return node.value, nodes, node.var

    @staticmethod
    def evaluateUnaryOperator(node, nodes):
        assert isinstance(node, OperatorNode) and node.type.numOperands == 1

        value, nodes, var = ExpressionEvaluator.evaluate(nodes)

        if node.type.intOnly and isinstance(value, str):
            raise EvaluationError(f"operator `{node.type.printName}' cannot operate on strings", node.token)

        try:
            value = node.type.op(value)
        except ValueError:
            Util.error(f"unable to convert `{value}' to integer", node.token)

        if node.type.expectsLvalue:
            assert not node.isLvalue()
            assert var

            var.value = value

            if node.type == kPostDecrement:
                var.value -= 1
            elif node.type == kPostIncrement:
                var.value += 1

            node.lastSeenVarValue = var.value

        return value, nodes, var

    @staticmethod
    def evaluateBinaryOperator(node, nodes):
        assert isinstance(node, OperatorNode) and node.type.numOperands == 2

        val1, nodes, var = ExpressionEvaluator.evaluate(nodes)

        val2Token = next(node.token for node in nodes if isinstance(node, OperandNode))
        val2, nodes, var2 = ExpressionEvaluator.handleShortCircuitEvaluation(val1, node, nodes)

        ExpressionEvaluator.checkForDivisionByZero(val1, val2, val2Token, node, nodes)
        value = ExpressionEvaluator.evaluateOp(node, val1, val2)

        if node.type.expectsLvalue:
            assert var
            var.value = value
            node.lastSeenVarValue = value
        elif node.type == kComma:
            var = var2

        return value, nodes, var

    @staticmethod
    def evaluateOp(node, val1, val2):
        assert isinstance(node, OperatorNode)
        assert node.type.numOperands == 2
        assert isinstance(val1, (int, str))
        assert isinstance(val2, (int, str))

        kIndices = {
            (int, int): (0, 'integer and integer'),
            (int, str): (1, 'integer and string'),
            (str, int): (2, 'string and integer'),
            (str, str): (3, 'string and string'),
        }
        typesIndex, typesName = kIndices[(type(val1), type(val2))]

        if node.type.intOnly and typesIndex != 0:
            raise EvaluationError(f"operator `{node.type.printName}' only operates on integers", node.token)

        if callable(node.type.op):
            return node.type.op(val1, val2)
        else:
            assert isinstance(node.type.op, (list, tuple))
            assert len(node.type.op) == 4

            op = node.type.op[typesIndex]

            while isinstance(op, int):
                assert op != typesIndex
                op = node.type.op[op]

            if op is None:
                raise EvaluationError(f"invalid operand types ({typesName}) for operator `{node.type.printName}'", node.token)

            assert callable(op)
            return op(val1, val2)

    @staticmethod
    def evaluateTernaryOperator(node, nodes):
        assert isinstance(node, OperatorNode) and node.type.numOperands == 3

        conditionValue, nodes, var = ExpressionEvaluator.evaluate(nodes)
        if conditionValue:
            value, nodes, var = ExpressionEvaluator.evaluate(nodes)
            nodes = ExpressionEvaluator.skipExpression(nodes)
        else:
            nodes = ExpressionEvaluator.skipExpression(nodes)
            value, nodes, var = ExpressionEvaluator.evaluate(nodes)

        return value, nodes, var

    @staticmethod
    def handleShortCircuitEvaluation(val1, node, nodes):
        assert isinstance(val1, (int, str))
        assert isinstance(node, OperatorNode)
        assert isinstance(nodes, list)

        var2 = None

        if (node.type == kLogicalAnd and not val1) or (node.type == kLogicalOr and val1):
            val2, nodes = 0, ExpressionEvaluator.skipExpression(nodes)
        else:
            val2, nodes, var2 = ExpressionEvaluator.evaluate(nodes)

        return val2, nodes, var2

    @staticmethod
    def checkForDivisionByZero(val1, val2, val2Token, node, nodes):
        assert isinstance(val1, (int, str))
        assert isinstance(val2, (int, str))
        assert isinstance(val2Token, Token)
        assert isinstance(node, OperatorNode)
        assert isinstance(nodes, list)

        if isinstance(val1, int) and val2 == 0 and node.type in (kDivision, kDivEq, kModuo, kModEq):
            raise EvaluationError('division by zero', val2Token)

    @staticmethod
    def skipExpression(nodes):
        if isinstance(nodes[0], OperandNode):
            return nodes[1:]

        numOperands = nodes[0].type.numOperands
        nodes = nodes[1:]

        for _ in range(0, numOperands):
            nodes = ExpressionEvaluator.skipExpression(nodes)

        return nodes

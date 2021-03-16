import Util

from Menu import Menu
from Token import Token
from Tokenizer import Tokenizer
from Variable import Variable
from VariableStorage import VariableStorage

from Preprocessor.ExpressionParser import ExpressionParser
from Preprocessor.ExpressionEvaluator import ExpressionEvaluator
from Preprocessor.ExpressionEvaluator import EvaluationError

class ExpressionHandler:
    def __init__(self, tokenizer, varStorage, showWarnings, passExceptions=False):
        assert isinstance(tokenizer, Tokenizer)
        assert isinstance(varStorage, VariableStorage)
        assert callable(showWarnings)

        self.parser = ExpressionParser(tokenizer, varStorage, showWarnings, passExceptions)
        self.evaluator = ExpressionEvaluator()

    def parse(self, menu=None):
        assert isinstance(menu, (Menu, type(None)))

        return self.parser.parse(menu)

    def evaluate(self, nodes):
        assert nodes and isinstance(nodes, list)

        try:
            return self.evaluator.evaluate(nodes)
        except EvaluationError as e:
            Util.error(str(e), e.token)

    def parseAndEvaluate(self, menu=None):
        assert isinstance(menu, (Menu, type(None)))

        nodes = self.parse(menu)
        value = self.evaluate(nodes)[0]
        return value

    def createSingleOperandExpression(self, token, menu, variable, varStorage):
        assert isinstance(token, Token)
        assert isinstance(menu, (Menu, type(None)))
        assert isinstance(variable, Variable)
        assert isinstance(varStorage, VariableStorage)

        return self.evaluator.createSingleOperandExpression(token, menu, variable, varStorage)

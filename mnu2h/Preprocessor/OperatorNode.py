from Token import Token

from Preprocessor.Operator import Operator

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

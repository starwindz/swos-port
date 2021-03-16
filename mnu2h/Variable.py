"""Represents any kinds of variable that can be encountered in the menu source file."""

from Token import Token

class Variable:
    def __init__(self, name, value, token):
        assert isinstance(name, str)
        assert isinstance(value, (str, int))
        assert isinstance(token, Token)

        self.name = name
        self.value = value
        self.token = token
        self.referenced = False

    def __repr__(self):
        return (f'<{self.name} = "{self.value}", type: {"int" if isinstance(self.value, int) else "str"}, '
            f'referenced: {self.referenced}, {self.token}>')

    @staticmethod
    def isSwos(var):
        assert isinstance(var, str)
        return var.startswith('$')

    @staticmethod
    def makeSwos(var):
        assert isinstance(var, str)
        return '$' + var

    @staticmethod
    def extractSwosVar(var):
        assert isinstance(var, str) and Variable.isSwos(var)
        return var[1:]

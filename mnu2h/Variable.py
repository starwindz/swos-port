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
        return f'<{self.name} = {self.value}, referenced: {self.referenced}, {self.token}>'

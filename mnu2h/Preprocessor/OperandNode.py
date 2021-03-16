import Util

from Menu import Menu
from Token import Token
from Tokenizer import Tokenizer
from VariableStorage import VariableStorage

class InvalidOperandNode(Exception):
    def __init__(self, node):
        self.node = node
        super().__init__()

class OperandNode:
    def __init__(self, token, menu, varStorage, var=None):
        assert isinstance(token, Token)
        assert isinstance(menu, (Menu, type(None)))
        assert isinstance(varStorage, VariableStorage)

        self.token = token
        self.value = token.string
        self.var = var
        self.uninitialized = False
        self.lastSeenVarValue = None

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
                self.lastSeenVarValue = var.value

            if not isinstance(self.value, int):
                if not self.var and Util.isString(self.value):
                    self.value = Util.unquotedString(self.value)
                else:
                    try:
                        self.value = int(self.value, 0)
                    except ValueError:
                        raise InvalidOperandNode(self)

        else:
            self.value = var.value

    def __repr__(self):
        return self.var.__repr__() if self.var else f'<{self.value}, {self.token}>'

    def isLvalue(self):
        return bool(self.var)

    def setConstant(self, value):
        self.value = value
        self.var = None

    # Updates node value in case its variable has changed (it can also happen outside the parser/evaluator,
    # for example during repeat loop variable iterations -- node chains are kept separately so they don't
    # have to be re-parsed)
    def update(self):
        if self.var and self.lastSeenVarValue != self.var.value:
            self.value = self.var.value
            self.lastSeenVarValue = self.var.value

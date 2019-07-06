"""Basic parser unit, used heavily throughout the program"""

import os

class Token:
    def __init__(self, string, path='', line=0):
        assert isinstance(string, (str, int))
        assert isinstance(path, str)
        assert isinstance(line, int)

        self.string = str(string)
        self.line = line
        self.path = path
        self.inputFilename = os.path.basename(path)

    def __repr__(self):
        return f"Token['{self.string}', {self.inputFilename}, line {self.line}]"

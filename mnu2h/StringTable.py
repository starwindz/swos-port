class StringTable:
    def __init__(self, variable, values=(), initialValue=0):
        assert isinstance(variable, str)
        assert isinstance(values, (list, tuple))
        assert isinstance(initialValue, int)

        self.variable = variable
        self.values = values
        self.initialValue = initialValue

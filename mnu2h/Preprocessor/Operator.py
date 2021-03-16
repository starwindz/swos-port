import Util

class Operator:
    def __init__(self, text, numOperands, expectsLvalue, intOnly, op, printName=None):
        assert isinstance(text, str)
        assert isinstance(numOperands, int)
        assert callable(op) or isinstance(op, (tuple, list))

        self.text = text
        self.printName = printName if printName else text
        self.numOperands = numOperands
        self.expectsLvalue = expectsLvalue
        self.intOnly = intOnly
        self.op = op

    def __repr__(self):
        return f'Operator {self.text}, operands: {self.numOperands}'
                             # n.ops. lvalue int-only
kBitwiseNot     = Operator('~',    1, False, True,  lambda v: ~v)
kLogicalNot     = Operator('!',    1, False, True,  lambda v: int(not v))
kUnaryMinus     = Operator('-',    1, False, True,  lambda v: -v)
kUnaryPlus      = Operator('+',    1, False, True,  lambda v: v)
kPreIncrement   = Operator('++',   1, True,  True,  lambda v: v + 1)
kPostIncrement  = Operator('++',   1, True,  True,  lambda v: v)
kPreDecrement   = Operator('--',   1, True,  True,  lambda v: v - 1)
kPostDecrement  = Operator('--',   1, True,  True,  lambda v: v)
kInt            = Operator('int',  1, False, False, lambda v: int(str(v), 0))
kStr            = Operator('str',  1, False, False, str)
kIndex          = Operator('[',    2, False, False, (lambda v1, v2: str(v1)[v2] if -len(str(v1)) <= v2 < len(str(v1)) else '', None, 0, None), '[]')
kComma          = Operator(',',    2, False, False, lambda v1, v2: v2)
kTernaryOp      = Operator('?',    3, False, False, lambda v1, v2, v3: v2 if v1 else v3)
kLogicalOr      = Operator('||',   2, False, False, lambda v1, v2: int(bool(v1) or bool(v2)))
kLogicalAnd     = Operator('&&',   2, False, False, lambda v1, v2: int(bool(v1) and bool(v2)))
kOr             = Operator('|',    2, False, True,  lambda v1, v2: v1 | v2)
kXor            = Operator('^',    2, False, True,  lambda v1, v2: v1 ^ v2)
kAnd            = Operator('&',    2, False, True,  lambda v1, v2: v1 & v2)
kEqual          = Operator('==',   2, False, False, (lambda v1, v2: int(v1 == v2), None, None, 0))
kNotEqual       = Operator('!=',   2, False, False, (lambda v1, v2: int(v1 != v2), None, None, 0))
kAddition       = Operator('+',    2, False, False, (lambda v1, v2: v1 + v2, lambda v1, v2: str(v1) + str(v2), 1, 1))
kSubtraction    = Operator('-',    2, False, True,  lambda v1, v2: v1 - v2)
kLessThan       = Operator('<',    2, False, False, (lambda v1, v2: int(v1 < v2), lambda v1, v2: str(v1) < str(v2), 1, 1))
kGreaterThan    = Operator('>',    2, False, False, (lambda v1, v2: int(v1 > v2), lambda v1, v2: str(v1) > str(v2), 1, 1))
kLessOrEqual    = Operator('<=',   2, False, False, (lambda v1, v2: int(v1 <= v2), lambda v1, v2: str(v1) <= str(v2), 1, 1))
kGreaterOrEqual = Operator('>=',   2, False, False, (lambda v1, v2: int(v1 >= v2), lambda v1, v2: str(v1) >= str(v2), 1, 1))
kLeftShift      = Operator('<<',   2, False, False, (lambda v1, v2: v1 << v2, None, Util.shiftStringLeft, None))
kRightShift     = Operator('>>',   2, False, False, (lambda v1, v2: v1 >> v2, None, Util.shiftStringRight, None))
kLeftRotate     = Operator('<<|',  2, False, False, (Util.rotateLeft, None, Util.rotateStringLeft, None))
kRightRotate    = Operator('|>>',  2, False, False, (Util.rotateRight, None, Util.rotateStringRight, None))
kMultiplication = Operator('*',    2, False, False, (lambda v1, v2: v1 * v2, 0, 0, None))
kDivision       = Operator('/',    2, False, True,  lambda v1, v2: v1 // v2)
kModuo          = Operator('%',    2, False, True,  lambda v1, v2: v1 % v2)
kAssignment     = Operator('=',    2, True,  False, lambda v1, v2: v2)
kPlusEq         = Operator('+=',   2, True,  False, kAddition.op)
kMinusEq        = Operator('-=',   2, True,  True,  kSubtraction.op)
kMulEq          = Operator('*=',   2, True,  False, kMultiplication.op)
kDivEq          = Operator('/=',   2, True,  True,  kDivision.op)
kModEq          = Operator('%=',   2, True,  True,  kModuo.op)
kLeftShiftEq    = Operator('<<=',  2, True,  False, kLeftShift.op)
kLeftRotateEq   = Operator('<<=|', 2, True,  False, kLeftRotate.op)
kRightShiftEq   = Operator('>>=',  2, True,  False, kRightShift.op)
kRightRotateEq  = Operator('|>>=', 2, True,  False, kRightRotate.op)
kAndEq          = Operator('&=',   2, True,  True,  kAnd.op)
kXorEq          = Operator('^=',   2, True,  True,  kXor.op)
kOrEq           = Operator('|=',   2, True,  True,  kOr.op)

makeOpDict = lambda *opList: { op.text : op for op in opList }

kEqualityOperators = makeOpDict(kEqual, kNotEqual)
kRelationalOps = makeOpDict(kLessThan, kGreaterThan, kLessOrEqual, kGreaterOrEqual)
kAssignmentOperators = makeOpDict(kPlusEq, kMinusEq, kMulEq, kDivEq, kModEq, kLeftShiftEq,
    kLeftRotateEq, kRightShiftEq, kRightRotateEq, kAndEq, kXorEq, kOrEq, kAssignment)
kShiftOps = makeOpDict(kLeftShift, kRightShift, kLeftRotate, kRightRotate)
kAddOps = makeOpDict(kAddition, kSubtraction)
kMulOps = makeOpDict(kMultiplication, kDivision, kModuo)
kUnaryOps = makeOpDict(kBitwiseNot, kLogicalNot, kUnaryMinus, kUnaryPlus, kPreIncrement, kPreDecrement, kInt, kStr)
kPostfixOps = makeOpDict(kPostDecrement, kPostIncrement, kIndex)

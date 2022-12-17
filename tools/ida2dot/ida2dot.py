# Convert a graph in gdl format to the dot syntax, or convert directly from assembly file.
# When done convert to desired output format using the dot tool.

import re
import sys
import os.path
from enum import IntEnum, auto
from collections import defaultdict
from typing import Final

kFontSize: Final[int] = 26

class Node:
    def __init__(self, id, label=''):
        self.m_id = id
        self.m_text = ''
        self.m_label = label
        self.m_edges = {}
        self.m_hasConent = False

    def id(self):
        return self.m_id

    def text(self):
        return self.m_text

    def setText(self, text):
        self.m_text = text

    def hasConent(self):
        return self.m_hasConent

    def addLine(self, line='', content=True):
        self.m_text += line.rstrip() + '\\l'
        self.m_hasConent |= content

    def addEmptyLine(self):
        self.m_text += '\\l'

    def empty(self):
        return not self.m_text

    def __repr__(self):
        text = self.m_text[:75] + '...' if len(self.m_text) > 75 else self.m_text
        return f'#{self.m_id}: {text}'

class Edge:
    def __init__(self, target, label):
        self.m_target = target
        self.m_label = label

    def target(self):
        return self.m_target

    def label(self):
        return self.m_label

    def __repr__(self):
        return f"-> {self.m_target}, label: {self.m_label}"

class LineType(IntEnum):
    kProcStart = auto()
    kProcEnd = auto()
    kLabel = auto()
    kBranchInstruction = auto()
    kNonBranchInstruction = auto()
    kAlignDirective = auto()
    kNopInstruction = auto()
    kInlineComment = auto()
    kEmptyLine = auto()
    kEndOfFile = auto()
    kUnknown = auto()

def main():
    if len(sys.argv) != 2:
        printHelp()

    processInputFile(sys.argv[1])

def processInputFile(path):
    isAsmFile = path.lower().endswith('.asm')
    processFunc = processAsm if isAsmFile else processGdl
    nodes, edges = processFunc(path)
    writeDotFile(path, nodes, edges)

def processAsm(path):
    nodes = { 0: Node(0) }
    with open(path, 'r') as f:
        proc = findProcStart(f)
        if not proc:
            sys.exit(f"Couldn't find procedure start in {path}")

        currentNodeIndex = 0
        currentNode = nodes[0]
        def addCurentNode():
            if not currentNode.empty():
                nonlocal currentNodeIndex
                nodes[currentNodeIndex] = currentNode
                currentNodeIndex += 1

        currentNode.addLine(f'{proc}:')

        labelRefs = defaultdict(list)
        def addReference(branchTarget, label=''):
            labelRefs[currentNode.id()].append(Edge(branchTarget, label))

        nodeLabels = {}
        pendingInlineComments = []

        lastType = LineType.kUnknown
        lastCommentOffset = -1

        while True:
            type, id, line, tokens = breakDownAssemblyLine(f)
            line, commentOffset, comment = filterComment(line)

            if type != LineType.kEmptyLine and not comment and not line.strip():
                lastType = LineType.kUnknown
                continue

            if type in (LineType.kBranchInstruction, LineType.kLabel):
                nodeLabel = None
                if type == LineType.kBranchInstruction:
                    if branchInstructionHasTarget(id):
                        conditional = isConditionalBranchInstruction(id)
                        if conditional:
                            branchTarget = currentNodeIndex + 1
                            edgeLabel = 'false'
                            addReference(branchTarget, edgeLabel)
                        branchTarget = tokens[2] if tokens[1].lower() == 'short' else tokens[1]
                        edgeLabel = 'true' if conditional else ''
                        addReference(branchTarget, edgeLabel)
                elif currentNode.hasConent():
                    addReference(currentNodeIndex + 1)
            elif type in (LineType.kNopInstruction, LineType.kAlignDirective, LineType.kNonBranchInstruction):
                pass    # will be dealt with later
            elif type == LineType.kEmptyLine:
                if lastType == LineType.kInlineComment:
                    pendingInlineComments.append('\n')
                continue
            elif type == LineType.kEndOfFile:
                sys.exit(f'Procedure {proc} unterminated.')
            elif type == LineType.kProcEnd:
                type, *rest = breakDownAssemblyLine(f)
                if type != LineType.kEndOfFile:
                    print('Warning: trailing content after end of procedure')
                if not currentNode.empty():
                    addCurentNode()
                break
            elif type == LineType.kInlineComment:
                if comment:
                    pendingInlineComments.append(comment)
            else:
                assert False

            if type == LineType.kLabel:
                labelIndex = currentNodeIndex
                if currentNode.hasConent():
                    addCurentNode()
                    currentNode = Node(currentNodeIndex, nodeLabel)
                    labelIndex += 1
                nodeLabel = tokens[0][:-1]
                nodeLabels[nodeLabel] = labelIndex

            if commentOffset >= 0:
                commentOffset = max(commentOffset, lastCommentOffset)

            if type not in (LineType.kAlignDirective, LineType.kNopInstruction, LineType.kInlineComment):
                if comment and commentOffset >= 0:
                    line = line.ljust(commentOffset) + comment
                for comment in pendingInlineComments:
                    currentNode.addLine(comment)
                pendingInlineComments.clear()
                currentNode.addLine(line, type != LineType.kInlineComment)

            if type == LineType.kBranchInstruction:
                addCurentNode()
                currentNode = Node(currentNodeIndex)

            lastType = type
            lastCommentOffset = commentOffset

        edges = resolveReferences(nodes, nodeLabels, labelRefs)
        return nodes, edges

def resolveReferences(nodes, nodeLabels, labelRefs):
    edges = []
    for nodeId, refEdges in labelRefs.items():
        for edge in refEdges:
            target = edge.target()
            if type(target) is not int:
                target = nodeLabels[target]
            if not edges or edges[-1][0] != nodeId or edges[-1][1] != target:
                edges.append((nodeId, target, edge.label()))
    return sorted(edges)

def findProcStart(f):
    type = LineType.kUnknown
    while type != LineType.kEndOfFile:
        type, id, *rest = breakDownAssemblyLine(f)
        if type == LineType.kProcStart:
            return id

kBranchInstructions = {
    'ja', 'jae', 'jb', 'jbe', 'jc', 'jcxz', 'je', 'jecxz', 'jg', 'jge', 'jl', 'jle', 'jmp', 'jna', 'jnae', 'jnb',
    'jnbe', 'jnc', 'jne', 'jng', 'jnge', 'jnl', 'jnle', 'jno', 'jnp','jns', 'jnz', 'jo', 'jp', 'jpe', 'jpo', 'js',
    'jz', 'loop', 'loope', 'loopew', 'loopne', 'loopnew', 'loopnz', 'loopnzw', 'loopw', 'loopz', 'loopzw',
    'ret', 'retf', 'retfw', 'retn',
}

def breakDownAssemblyLine(f):
    line = f.readline()
    tokens = line.split(None, 4)

    id = None
    type = LineType.kEmptyLine

    if tokens:
        type = LineType.kNonBranchInstruction
        id = tokens[0]
        if id.endswith(':'):
            type = LineType.kLabel
        elif line.startswith(';'):
            type = LineType.kInlineComment
        elif id.lower() == 'nop':
            type = LineType.kNopInstruction
        elif id.lower() == 'jmp' and re.match(r'jmp\s*(?:short\s*)?\$\s*\+\s*2\b', line):
            type = LineType.kNopInstruction
        elif id.lower() == 'align':
            type = LineType.kAlignDirective
        elif id.lower() in kBranchInstructions:
            type = LineType.kBranchInstruction
        elif len(tokens) > 1:
            if tokens[1] == 'proc':
                type = LineType.kProcStart
            elif tokens[1] == 'endp':
                type = LineType.kProcEnd
    elif not line:
        type = LineType.kEndOfFile

    return type, id, line, tokens

def filterComment(line):
    try:
        commentStart = line.find(';')
        intro = line[:commentStart]
        comment = line[commentStart:].strip()
        if any(ord(c) < 32 or ord(c) > 127 for c in comment) or all(c == '-' for c in comment[1:].strip()):
            comment = None
        return intro, commentStart, comment
    except ValueError:
        return line, -1, None

def processGdl(path):
    nodes = {}
    edges = []
    with open(path, 'r') as f:
        while line := f.readline():
            if node := processGdlNode(line, f):
                nodeId, nodeString = node
                nodeString = processGdlNodeString(nodeString, f)
                node = Node(nodeId)
                node.setText(nodeString)
                nodes[nodeId] = node
            elif edge := processGdlEdge(line, f):
                srcNode, dstNode, label = edge
                edges.append((srcNode, dstNode, label))
    return nodes, edges

kNodeRe = re.compile(r'node:\s*{.*?title:\s*"([^"]+)".*?label:\s*"(.*)$')
def processGdlNode(line, f):
    if m := kNodeRe.match(line):
        id = m[1]
        text = None if any(ord(c) < 32 for c in m[2]) else m[2]
        return id, text

kEdgeRe = re.compile(r'edge:\s*{\s*sourcename:\s*"([^"]+)"\s*targetname:\s*"([^"]+)(?:"\s*label:\s*"([^"]+)")?')
def processGdlEdge(line, f):
    if m := kEdgeRe.match(line):
        return m[1], m[2], m[3]

def writeDotFile(path, nodes, edges):
    outputPath = (path[:-4] if path.lower().endswith(('.gdl', '.asm')) else path) + '.dot'
    with open(outputPath, 'w') as f:
        p = lambda *args, **kwargs: print(file=f, *args, **kwargs)
        p('digraph G {\n    node[shape=box, nodesep=0.5, ranksep=0.5, weight=1.6,')
        p(f'        fontname=Consolas, fontsize={kFontSize}]')
        p(f'    edge[fontname=Consolas, fontsize={kFontSize}]')
        for nodeIndex, node in nodes.items():
            p(f'    {nodeIndex} [label={quoteString(node.text())}]')
        p()
        for fromNode, toNode, label in edges:
            attributes = ''
            if label:
                color = 'forestgreen' if label.lower() == 'true' else 'red3'
                attributes = f' [label="{label.upper()}" color={color}]'
            p(f"    {fromNode} -> {toNode}{attributes};")
        p('}')

kEndOfStringRe = re.compile(r'(.*?[^\\])"')
def processGdlNodeString(nodeString, f):
    nodeString = nodeString or ''
    if m := re.match(kEndOfStringRe, nodeString):
        nodeString = m[1]
    else:
        if nodeString:
            nodeString += '\n'
        for line in f:
            if m := re.match(kEndOfStringRe, line):
                return nodeString + m[1]
            else:
                nodeString += line
    return nodeString

def isConditionalBranchInstruction(instruction):
    assert instruction.lower() in kBranchInstructions
    return not instruction.lower().startswith(('ret', 'jmp'))

def branchInstructionHasTarget(instruction):
    assert instruction.lower() in kBranchInstructions
    return not instruction.lower().startswith('ret')

def quoteString(s):
    s = re.sub('([^"]|^)"', r'\1\"', s)
    s = s.replace('\n', '\\l')
    if not s.endswith('\\l'):
        s += '\\l'
    return f'"{s}"'

def printHelp():
    sys.exit(f'Usage {os.path.basename(sys.argv[0])} <input gdl/asm path>')

if __name__ == '__main__':
    main()

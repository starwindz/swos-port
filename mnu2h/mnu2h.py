# Converter from mnu file to C++ header
#
# usage: python mnu2h.py <input.mnu> <output.h>
#

import sys
from Parser.Parser import Parser
from CodeGenerator import CodeGenerator

# getInputAndOutputFile
#
# Fetches input and output file parameters from the command line and returns them as a list.
# If they're not present, complains and exits the program.
#
def getInputAndOutputFile():
    if len(sys.argv) < 2:
        sys.exit('Missing input file path')

    if len(sys.argv) < 3:
        sys.exit('Missing output file path')

    return sys.argv[1], sys.argv[2]

def main():
    if sys.version_info < (3, 6):
        sys.exit('Is this the real life? Is this just Python older than 3.6?')

    inputPath, outputPath = getInputAndOutputFile()
    parser = Parser(inputPath)
    CodeGenerator.output(parser, inputPath, outputPath)

if __name__ == '__main__':
    main()

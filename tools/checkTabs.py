import os
import sys
from typing import Final, Tuple

kFilesToCheck: Tuple[str] = (
    'cpp', 'c', 'h', 'txt', 'md', 'mnu', 'mh', 'asm', 'inc', 'py', 'xml', 'pl',
)
#Makefile

def checkFile(path):
    badLines = []
    with open(path, 'r', errors='ignore') as f:
        lineNo = 0
        while line := f.readline():
            lineNo += 1
            if '\t' in line:
                badLines.append(str(lineNo))

    if badLines:
        print('**** CORRUPTED FILE DETECTED!!! ****')
        print(path)
        print(f'Lines: {",".join(badLines)}')
        return False
    else:
        return True

def scanDirectory(dir):
    ok = True
    for root, dirs, files in os.walk(dir):
        for file in files:
            if file.lower().endswith(kFilesToCheck):
                ok = checkFile(os.path.join(root, file)) and ok
    return ok

def main():
    ok = True
    if len(sys.argv) <= 1:
        ok = scanDirectory('.') and ok
    else:
        for dir in sys.argv[1:]:
            if not os.path.isdir(dir):
                sys.exit(f"Couldn't scan '{dir}'")
            ok = scanDirectory(dir) and ok

    if not ok:
        sys.exit(1)

if __name__ == '__main__':
    main()

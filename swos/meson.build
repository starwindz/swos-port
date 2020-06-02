kOutputFormat = 'masm'
kInputs = ['swos.asm', 'symbols.txt']   # the order is important
kSwosAsmFiles = [        # sadly, couldn't figure out a way to generate this programatically from a number
    'swos-01.asm',
    'swos-02.asm',
    'swos-03.asm',
    'swos-04.asm',
    'swos-05.asm',
    'swos-06.asm',
    'swos-07.asm',
    'swos-08.asm',
]

# Find and version check MASM
masm = find_program('ml')
mlRun = run_command(masm, '/?')

assert(mlRun.returncode() == 0, 'failed to obtain MASM version')

output = mlRun.stderr().strip().split()
assert(output[4].to_lower() == 'version', 'unexpected MASM version string: @0@'.format(mlRun.stderr()))
assert(output[5].version_compare('>=14'), 'MASM version 14 or later required')

# generate MASM SWOS files
swosAsm = custom_target('swos-asm',
    input: kInputs,
    output: ['swossym.h', kSwosAsmFiles,],  # swossym.h must be first
    command: [
        ida2asm,
        '@INPUT0@',
        '@OUTDIR@' / 'swos.asm',
        '@INPUT1@',
        '@OUTDIR@' / 'swossym.h',
        kOutputFormat,
        kSwosAsmFiles.length().to_string(),
    ],
)

# assemble generated SWOS ASM files into object files
i = 1
swosObjFiles = []

foreach asmFile : kSwosAsmFiles
    objFile = fs.replace_suffix(asmFile, '.obj')

    swosObjFiles += custom_target(objFile,
        input: swosAsm[i],
        output: objFile,
        command: [
            masm,
            '/Cp',
            '/coff',
            '/c',
            '/nologo',
            '/Fo', '@OUTPUT@',
            '@INPUT@',
        ],
    )
    i += 1
endforeach

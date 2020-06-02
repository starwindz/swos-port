project('swos-port', 'cpp',
    default_options: ['cpp_std=c++17'],
    version: '1.0',
    meson_version: '>= 0.54.2',
)

isReleaseBuild = get_option('buildtype').startswith('release')

if isReleaseBuild
    add_global_arguments('-DNDEBUG', language: 'cpp')
else
    add_global_arguments('-DDEBUG', '-MTd', language: 'cpp')
endif

add_global_arguments(
    '/Zc:__cplusplus',
    '-DNOMINMAX',
    '/wd4731',
    language: 'cpp'
)

fs = import('fs')

# find and version check Python
pymod = import('python')
python = pymod.find_installation('python3')
assert(python.language_version()[2].to_int() >= 6, 'Python 3.6 or higher required')

# Python one-liners for file globbing
getFilesCommand = 'import os;import pathlib;' + \
    'print(*(os.path.relpath(f).replace("\\\\","/") for f in pathlib.Path("@0@").rglob("@1@")),sep="\\n")'
getDirsCommand = 'import os;' + \
    'print(*(os.path.relpath(d).replace("\\\\","/") for d,_,_ in os.walk("@0@")),sep="\\n")'

# add 3rd party include directories
c = run_command([python, '-c', getDirsCommand.format('include')])
rawDirs = c.stdout().strip().split('\n')

includeDirs = include_directories(rawDirs, is_system: true)

subdir('ida2asm')
subdir('swos')
subdir('mnu2h')

# gather the sources
subdir('src')

# specify pre-built libs
if isReleaseBuild
    libDir = 'lib' / 'windows' / 'Release'
    libNames = [
        'ADLMIDI',
        'CrashRpt1403',
        'CrashRptProbe1403',
        'SDL2',
        'SDL2_image',
        'SDL2_mixer',
        'SDL2_net',
        'SDL2main',
    ]
else
    libDir = 'lib' / 'windows' / 'Debug'
    libNames = [
        'ADLMIDI',
        'CrashRpt1403d',
        'CrashRptProbe1403d',
        'SDL2_image',
        'SDL2_mixer',
        'SDL2_net',
        'SDL2d',
        'SDL2maind',
    ]
endif

# Meson demands an absolute path here
libDir = meson.source_root() / libDir

cppCompiler = meson.get_compiler('cpp')
libs = []

foreach lib : libNames
    libs += cppCompiler.find_library(lib, dirs: libDir)
endforeach

libs += cppCompiler.find_library('winmm')

# and finally build the damn thing ;)
executable('swos-port', sourceFiles, swosAsm[0], swosObjFiles, menuFiles,
    include_directories: [includeDirs, srcIncludeDirs],
    dependencies: libs,
    cpp_pch: 'src' / 'stdinc.h'
)

subdir('tests')

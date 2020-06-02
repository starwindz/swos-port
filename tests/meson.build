# generate fetcher DLL
subdir('sdl-address-table-fetcher')

# get test source files
c = run_command([python, '-c', getFilesCommand.format('src', '*.cpp')])
rawSources = c.stdout().strip().split('\n')

testSources = []
foreach source : rawSources
    if source != 'stdinc.cpp'
        testSources += source
    endif
endforeach

# get test dirs for include path
c = run_command([python, '-c', getDirsCommand.format('src')])
testIncludeDirs = c.stdout().strip().split('\n')

projectFilenames = [
    'audio' / 'chants.cpp',
    'audio' / 'comments.cpp',
    'audio' / 'sfx.cpp',
    'audio' / 'SoundSample.cpp',
    'controls' / 'controlOptionsMenu.cpp',
    'controls' / 'controls.cpp',
    'controls' / 'joypadConfigMenu.cpp',
    'controls' / 'joypads.cpp',
    'controls' / 'scanCodes.cpp',
    'files' / 'selectFilesMenu.cpp',
    'menus' / 'menu.cpp',
    'menus' / 'menuMouse.cpp',
    'replays' / 'ReplayData.cpp',
    'replays' / 'replayOptions.cpp',
    'replays' / 'replays.cpp',
    'replays' / 'replaysMenu.cpp',
    'substitutes' / 'substitutes.cpp',
    'video' / 'sprites.cpp',
    'video' / 'videoOptionsMenu.cpp',
    'game.cpp',
    'init.cpp',
    'pitch.cpp',
]

# extract source files from the prefilled dict
projectSources = []
foreach filename : projectFilenames
    projectSources += sourceFilenamesToFiles[filename]
endforeach

# we inherit a lot from main projects, such as include paths, libs and some of the sources
# make sure test include dirs come first so we can override dirent.h
# also make sure project sources come before test sources to avoid some weirdness (it was fixed but just in case)
tests = executable('tests', projectSources, testSources, swosAsm[0], swosObjFiles, menuFiles,
    include_directories: [testIncludeDirs, includeDirs, srcIncludeDirs, 'sdl-address-table-fetcher' / 'src'],
    link_with: sdlAddressTableFetcher,
    dependencies: libs,
    cpp_args: ['-DSWOS_TEST', '-DDIRENT_H', fetcherDefine],
    cpp_pch: '..' / 'src' / 'stdinc.h',
)
test('SWOS tester', tests)

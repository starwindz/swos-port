# gather all the source files
c = run_command([python, '-c', getFilesCommand.format('.', '*.cpp')])
rawSources = c.stdout().strip().split('\n')

currentSrcDir = meson.current_source_dir()
sourceFiles = []

# we'll need this dict later to pick & choose project files for the tester
sourceFilenamesToFiles = {}

foreach source : rawSources
    if not source.endswith('stdinc.cpp')
        file = files(source)
        sourceFiles += file
        sourceFilenamesToFiles += {source: file}
    endif
endforeach

# add all subdirectories as includes
c = run_command([python, '-c', getDirsCommand.format('.')])
rawDirs = c.stdout().strip().split('\n')

srcIncludeDirs = ['.']
foreach dir : rawDirs
    srcIncludeDirs += dir
endforeach

srcIncludeDirs = include_directories(srcIncludeDirs)

subdir('menus')

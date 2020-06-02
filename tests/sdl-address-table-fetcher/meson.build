fetcherBaseName = 'SdlAddressTableFetcher'
fetcherDllName = fetcherBaseName + '.dll'
fetcherDefine = '-DSDL_ADDRESS_FETCHER_DLL="@0@"'.format(fetcherDllName)

sdlAddressTableFetcher = shared_library('SdlAddressTableFetcher',
    'src' / 'SdlAddressTableFetcher.cpp',
    cpp_args: ['-D_WINDLL', '-DSDL_DYNAPI_FETCHER_EXPORTS', fetcherDefine],
)

# copy the DLL up to the tester

# must convert from backslashes to slashes to be able to pass it to Python at all
dllPath = fs.as_posix(sdlAddressTableFetcher.full_path())

custom_target('copy-fetcher-dll',
    depends: sdlAddressTableFetcher,
    build_by_default: true,
    build_always_stale: true,
    command: [python, '-c',
        'import shutil;shutil.copy("@0@", "@1@")'.format(dllPath, meson.build_root() / 'tests')
    ],
    output: 'none',
)

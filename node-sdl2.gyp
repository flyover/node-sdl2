{
    'variables': {
        'OS%': 'ios',
        'NODE_PATH': '../../node',
        'SDL2_PATH': '../../SDL2',
    },
    'xcode_settings': {
        'ALWAYS_SEARCH_USER_PATHS': 'NO',
        'USE_HEADERMAP': 'NO',
    },
    'conditions': [
        [ 'OS=="ios"', {
            'xcode_settings': {
                'SDKROOT': 'iphoneos',
                'ARCHS': '$(ARCHS_STANDARD)',
                'TARGETED_DEVICE_FAMILY': '1,2',
                'CODE_SIGN_IDENTITY': 'iPhone Developer',
            }
        }],
        [ 'OS=="osx"', {
            'xcode_settings': {
                'SDKROOT': 'macosx',
                'ARCHS': '$(ARCHS_STANDARD_32_64_BIT)',
            }
        }],
    ],
    'target_defaults': {
        'defines': [
            '__POSIX__',
            '_LARGEFILE_SOURCE',
            '_LARGEFILE64_SOURCE',
            '_FILE_OFFSET_BITS=64',
            '_DARWIN_USE_64_BIT_INODE=1',
        ],
        'configurations': {
            'Debug': {
                'defines': [ '_DEBUG', 'DEBUG=1' ],
                'cflags': [
                    '-g',
                    '-O0',
                    '-fno-strict-aliasing'
                    '-fwrapv'
                ],
            },
            'Release': {
                'defines': [ 'NDEBUG=1' ],
                'cflags': [
                    '-O3',
                    '-fstrict-aliasing',
                    '-fomit-frame-pointer',
                    '-fdata-sections',
                    '-ffunction-sections',
                ],
            },
        },
        'xcode_settings': {
            'GCC_C_LANGUAGE_STANDARD': 'c99',         # -std=c99
            'GCC_CW_ASM_SYNTAX': 'NO',                # No -fasm-blocks
            'GCC_DYNAMIC_NO_PIC': 'NO',               # No -mdynamic-no-pic (Equivalent to -fPIC)
            'GCC_ENABLE_CPP_EXCEPTIONS': 'NO',        # -fno-exceptions
            'GCC_ENABLE_CPP_RTTI': 'NO',              # -fno-rtti
            'GCC_ENABLE_PASCAL_STRINGS': 'NO',        # No -mpascal-strings
            'GCC_INLINES_ARE_PRIVATE_EXTERN': 'YES',  # -fvisibility-inlines-hidden
            'GCC_SYMBOLS_PRIVATE_EXTERN': 'YES',      # -fvisibility=hidden
            'GCC_THREADSAFE_STATICS': 'NO',           # -fno-threadsafe-statics
            'GCC_WARN_NON_VIRTUAL_DESTRUCTOR': 'YES', # -Wnon-virtual-dtor
            'PREBINDING': 'NO',                       # No -Wl,-prebind
            'WARNING_CFLAGS': [
                '-Wall',
                '-Wendif-labels',
                '-W',
                '-Wno-unused-parameter',
            ],
            'OTHER_CFLAGS[arch=armv7]': [ '-marm' ],
            'OTHER_CFLAGS[arch=armv7s]': [ '-marm' ],
            'OTHER_CFLAGS[arch=arm64]': [ '-marm' ],
        },
    },
    'targets': [
        {
            'target_name': 'libnode-sdl2-<(OS)',
            'type': 'static_library',
            'defines': [
                'NODE_WANT_INTERNALS=1',
            ],
            'include_dirs': [
                '.',
                '<!(node -e "require(\'nan\')")',
                '<(NODE_PATH)/src',
                '<(NODE_PATH)/deps/uv/include',
                '<(NODE_PATH)/deps/v8/include',
                '<(NODE_PATH)/deps/debugger-agent/include',
                '<(NODE_PATH)/deps/cares/include',
                '<(SDL2_PATH)/include',
            ],
            'direct_dependent_settings': {
                'include_dirs': [
                    '.',
                    '<!(node -e "require(\'nan\')")',
                    '<(SDL2_PATH)/include',
                ]
            },
            'dependencies': [
            ],
            'sources': [
				'node-sdl2.h',
				'node-sdl2.cc',
            ],
        },
    ],
}

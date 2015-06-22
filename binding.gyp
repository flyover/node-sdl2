{
    'targets':
    [
        {
            'target_name': "node-sdl2",
            'include_dirs':
            [
                "<(module_root_dir)",
                "<!(node -e \"require('nan')\")"
            ],
            'sources':
            [
                "node-sdl2.cc"
            ],
            'conditions':
            [
                [
                    'OS=="linux"',
                    {
                        'include_dirs': [ "/usr/local/include/SDL2" ],
                        'cflags': [ "<!@(sdl2-config --cflags)" ],
                        'ldflags': [ "<!@(sdl2-config --libs)" ],
                        'libraries': [ "-lSDL2" ]
                    }
                ],
                [
                    'OS=="mac"',
                    {
                        'include_dirs': [ "/usr/local/include/SDL2" ],
                        'cflags': [ "<!@(sdl2-config --cflags)" ],
                        'ldflags': [ "<!@(sdl2-config --libs)" ],
                        'libraries': [ "-L/usr/local/lib", "-lSDL2" ]
                    }
                ],
                [
                    'OS=="win"',
                    {
                        'include_dirs': [ "$(SDL2_ROOT)/include" ],
                        'libraries': [ "$(SDL2_ROOT)/lib/x64/SDL2.lib" ],
                        'copies':
                        [
                            {
                                'destination': "<!(echo %USERPROFILE%)/bin",
                                'files': [ "$(SDL2_ROOT)/lib/x64/SDL2.dll" ]
                            }
                        ]
                    }
                ]
            ]
        }
    ]
}

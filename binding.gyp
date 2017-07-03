{
  'targets': [{
    'target_name': 'K',
    'type': 'loadable_module',
    'sources': [ 'src/lib/K.cc' ],
    'libraries': [ '-lsqlite3', '-luWS' ],
    'conditions': [
      ['OS=="linux"', {
        'cflags_cc': [ '-std=c++11', '-DUSE_LIBUV' ],
        'cflags_cc!': [ '-fno-exceptions', '-std=gnu++0x', '-fno-rtti' ],
        'cflags!': [ '-fno-omit-frame-pointer' ],
        'ldflags!': [ '-rdynamic' ],
        'ldflags': [ '-s' ]
      }],
      ['OS=="mac"', {
        'xcode_settings': {
          'MACOSX_DEPLOYMENT_TARGET': '10.7',
          'CLANG_CXX_LANGUAGE_STANDARD': 'c++11',
          'CLANG_CXX_LIBRARY': 'libc++',
          'GCC_GENERATE_DEBUGGING_SYMBOLS': 'NO',
          'GCC_ENABLE_CPP_EXCEPTIONS': 'YES',
          'GCC_THREADSAFE_STATICS': 'YES',
          'GCC_OPTIMIZATION_LEVEL': '3',
          'GCC_ENABLE_CPP_RTTI': 'YES',
          'OTHER_CFLAGS!': [ '-fno-strict-aliasing' ],
          'OTHER_CPLUSPLUSFLAGS': [ '-DUSE_LIBUV' ]
        }
      }]
    ]
  }, {
    'target_name': 'build',
    'type': 'none',
    'dependencies': [ 'K' ],
    'conditions': [
      ['OS!="win"', {
        'actions': [{
          'action_name': 'K.platform.modules',
          'inputs': [ '<@(PRODUCT_DIR)/K.node' ],
          'outputs': [ 'K' ],
          'action': [ 'cp', '<@(PRODUCT_DIR)/K.node', 'build/K.<!@(node -p process.platform).<!@(node -p process.versions.modules).node' ]
        }]
      }]
    ]
  }, {
    'target_name': 'install',
    'type': 'none',
    'dependencies': [ 'K', 'build' ],
    'copies': [{
      'files': [ 'build/K.<!@(node -p process.platform).<!@(node -p process.versions.modules).node' ],
      'destination': 'app/server/lib'
    }]
  }]
}
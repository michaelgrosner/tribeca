{
  'targets': [{
    'target_name': 'K',
    'type': 'loadable_module',
    'sources': [
      'src/lib/K.cc',
      'src/lib/stdev.cc',
      'src/lib/sqlite.cc'
    ],
    'include_dirs': [ '<!(node -e "require(\'nan\')")' ],
    'libraries': [ '-lsqlite3' ],
    'conditions': [
      ['OS=="mac"', {
        'xcode_settings': {
          "OTHER_CFLAGS": [
            '-mmacosx-version-min=10.7',
            '-stdlib=libc++'
          ]
        }
      }]
    ]
  }, {
    'target_name': 'build',
    'type': 'none',
    'dependencies': [ 'K' ],
    'copies': [{
      'files': [ '<(PRODUCT_DIR)/K.node' ],
      'destination': 'app/lib'
    }]
  }]
}
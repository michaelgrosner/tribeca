{
  'targets': [{
    'target_name': 'K',
    'type': 'loadable_module',
    'sources': [
      'src/lib/K.cc',
      'src/lib/stdev.cc',
      'src/lib/sqlite.cc',
      'src/lib/ui.cc'
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
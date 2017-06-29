{
  'targets': [
    {
      'target_name': 'K',
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
            'MACOSX_DEPLOYMENT_TARGET': '10.8',
            'CLANG_CXX_LIBRARY': 'libc++',
            'CLANG_CSS_LANGUAGE_STANDARD': 'c++11'
          }
        }]
      ]
    }
  ]
}
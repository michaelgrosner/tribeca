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
    }
  ]
}
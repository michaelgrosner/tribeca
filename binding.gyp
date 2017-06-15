{
  'targets': [
    {
      'target_name': 'K',
      'sources': [
        'src/lib/K.cc',
        'src/lib/round.cc',
        'src/lib/stdev.cc'
      ],
      'include_dirs' : [ '<!(node -e "require(\'nan\')")' ]
    }
  ]
}
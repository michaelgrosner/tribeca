{
  'targets': [
    {
      'target_name': 'ctubio',
      'sources': [
        'src/lib/ctubio.cc',
        'src/lib/round.cc',
        'src/lib/stdev.cc'
      ],
      'include_dirs' : [ '<!(node -e "require(\'nan\')")' ]
    }
  ]
}
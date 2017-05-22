{
  'targets': [
    {
      'target_name': 'tribeca',
      'sources': [
        'src/lib/tribeca.cc',
        'src/lib/round.cc',
        'src/lib/stdev.cc'
      ],
      'include_dirs' : [ '<!(node -e "require(\'nan\')")' ]
    }
  ]
}
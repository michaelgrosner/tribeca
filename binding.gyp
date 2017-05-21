{
  'targets': [
    {
      'target_name': 'tribeca',
      'sources': [
        'src/lib/tribeca.cc',
        'src/lib/round.cc'
      ],
      'include_dirs' : [ '<!(node -e "require(\'nan\')")' ]
    }
  ]
}
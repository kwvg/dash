example_util_sources = files(
  '../../src/test/util/tempdir.cpp',
)

examples = [
  'element_types',
  'error_handling',
  'conditional_insert',
  'transaction_lifecycle',
  'operation_costs',
  'basic_crud',
  'tree_state',
]

foreach name : examples
  executable(name,
    '@0@.cpp'.format(name),
    example_util_sources,
    include_directories: libgrovedb_includes,
    dependencies: [libgrovedb_dep],
    build_by_default: true,
  )
endforeach

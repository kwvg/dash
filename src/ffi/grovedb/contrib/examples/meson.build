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
  'range_queries',
  'sum_trees',
  'multi_query',
  'hierarchical_query',
  'proof_generation',
  'absence_proofs',
  'chained_queries',
  'batch_operations',
  'batch_in_transaction',
  'auxiliary_storage',
  'checkpoint_snapshot',
  'subtree_hierarchy',
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

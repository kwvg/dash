fuzz_targets = [
  'fuzz_element',
  'fuzz_put_get',
  'fuzz_query',
  'fuzz_subtree',
  'fuzz_sum_tree',
  'fuzz_transaction',
  'fuzz_tree_state',
  'fuzz_wire',
]

fuzz_util_sources = files(
  'utils/tempdir.cpp',
)

fuzz_vendor_includes = include_directories('vendor')

foreach target : fuzz_targets
  executable(target,
    '@0@.cpp'.format(target),
    fuzz_util_sources,
    build_by_default: true,
    build_rpath: fuzz_build_rpath,
    cpp_args: fuzz_compile_args,
    dependencies: [libgrovedb_dep],
    include_directories: [libgrovedb_includes, fuzz_vendor_includes],
    link_args: fuzz_link_args,
    override_options: ['b_lundef=false'],
  )
endforeach

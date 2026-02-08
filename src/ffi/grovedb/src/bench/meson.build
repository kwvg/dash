bench_sources = files(
  'bench.cpp',
  'bench_element.cpp',
  'bench_crud.cpp',
  'bench_delete.cpp',
  'bench_query.cpp',
  'bench_proof.cpp',
  'bench_batch.cpp',
  'bench_aux.cpp',
  '../test/util/tempdir.cpp',
)

bench_includes = include_directories('vendor')

bench_exe = executable('bench_grovedb',
  bench_sources,
  include_directories: [libgrovedb_includes, bench_includes],
  dependencies: [libgrovedb_dep],
  build_by_default: true,
)

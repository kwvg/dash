example_util_sources = files(
  '../../src/test/util/tempdir.cpp',
)

examples = {
}

foreach name, source : examples
  executable(name,
    source,
    example_util_sources,
    include_directories: libgrovedb_includes,
    dependencies: [libgrovedb_dep],
    build_by_default: true,
  )
endforeach

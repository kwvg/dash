# Getting Started

## Prerequisites

- **C++20 compiler** — GCC 13+ or Clang 16+
- **Meson 1.9.0+** — build system
- **Rust toolchain** (stable) — builds the GroveDB FFI layer. Your application code is pure C++; the Rust toolchain is only needed at build time.

## Building

```bash
meson setup builddir
meson compile -C builddir
```

Key build options (set with `-D`):

| Option | Default | Description |
|--------|---------|-------------|
| `build_examples` | `false` | Build the 19 example programs in `contrib/examples/` |
| `build_tests` | `true` | Build the test suite (requires Boost.Test 1.73+) |
| `shared_library` | `true` | Build as shared library with pkg-config |
| `harden_build` | `true` | Enable compiler hardening flags |

Example with all options:

```bash
meson setup builddir -Dbuild_examples=true -Dbuild_tests=true
meson compile -C builddir
meson test -C builddir
```

## Linking

### As a Meson subproject

Add libgrovedb as a dependency in your `meson.build`:

```meson
grovedb_dep = dependency('libgrovedb')

executable('myapp',
  'main.cpp',
  dependencies: [grovedb_dep],
)
```

### With pkg-config

If libgrovedb is installed system-wide:

```bash
pkg-config --cflags --libs libgrovedb
```

Linking pulls in the Rust runtime automatically — no additional configuration needed.

## Opening a Database

```cpp
#include <grovedb/db.h>  // for Db, Path, Bytes, Element, Result, ...

auto result = grovedb::Db::Open("/path/to/db");
if (!result.has_value()) {
  // handle error: result.error().code(), result.error().message()
}
auto db = std::move(result).value();
```

[`Db::Open()`](@ref grovedb::Db::Open) creates a new database if the path doesn't exist, or opens an existing one. A fresh database starts with an empty root tree — similar to how `rocksdb::DB::Open` creates or opens a RocksDB instance, but with authenticated tree structures built in.

The path is a directory — GroveDB (via RocksDB) stores multiple files within it.

> **Namespace guidance**: All public types live under `grovedb::`. Prefer targeted `using` declarations over blanket imports:
> ```cpp
> using grovedb::Db;
> using grovedb::Bytes;
> using grovedb::Element;
> using grovedb::Path;
> ```
> Avoid `using namespace grovedb;` — it pulls in type aliases and helper functions that can collide with your own code or other libraries.

## Closing and Cleanup

The [`Db`](@ref grovedb::Db) destructor handles cleanup via RAII — it flushes pending writes and closes the database. No explicit close call needed.

For explicit control:

```cpp
// Force pending writes to persistent storage
auto flush_result = db.Flush();

// Delete all data from the database (destructive)
auto destroy_result = db.Destroy();
```

## First Program

Here's a minimal program that opens a database, inserts an element, and reads it back:

```cpp
#include <grovedb/db.h>       // for Db, Path, Bytes, Result, Costed, ...
#include <grovedb/element.h>  // for Element

#include <cstdlib>
#include <stdio.h>

int main()
{
  auto result = grovedb::Db::Open("/tmp/my_first_grovedb");
  if (!result.has_value()) {
    fprintf(stderr, "open failed: %s\n", result.error().message().c_str());
    return EXIT_FAILURE;
  }
  auto db = std::move(result).value();

  grovedb::Path root{};

  // Insert an item at the root
  auto put = grovedb::Element::Item(grovedb::Bytes::FromString("Hello GroveDB!"))
    .and_then([&](grovedb::Element e) {
      return db.Put(root, grovedb::Bytes::FromString("greeting"), e);
    });
  if (!put.has_value()) {
    fprintf(stderr, "put failed: %s\n", put.error().message().c_str());
    return EXIT_FAILURE;
  }
  fprintf(stdout, "inserted: %lu seeks, %lu bytes added\n", put->m_seek_count, put->m_storage_added_bytes);

  // Read it back
  auto get = db.Get(root, grovedb::Bytes::FromString("greeting"));
  if (!get.has_value()) {
    fprintf(stderr, "get failed: %s\n", get.error().message().c_str());
    return EXIT_FAILURE;
  }
  fprintf(stdout, "retrieved: %zu raw bytes\n", get->value().data().size());

  return EXIT_SUCCESS;
}
```

For the complete version with logging, see [`contrib/examples/basic_crud.cpp`](../libgrovedb/contrib/examples/basic_crud.cpp).

## Next Steps

- [Elements and Trees](04-elements-and-trees.md) — understand the four element types and how to build subtree hierarchies
- [Basic Operations](05-basic-operations.md) — Put, Get, Delete, and their variants

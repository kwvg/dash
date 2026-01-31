# grovedb_cxx

Rust crate used for [`grovedb`](../grovedb)'s C++ foreign function interface bindings using [`cxx`](https://crates.io/crates/cxx) for a limited subset of the public API meant to consumption using [`libgrovedb`](../../src/ffi/grovedb).

## Notes

* Requires Rust 1.88+

* The version of `cxx` used with this package must match with the version used in all other packages in the [`rust`](..) directory and with the [`native_cxxbridge`](../../depends/packages/native_cxxbridge.mk) package. Failure to ensure they match will result in symbol errors and unexpected behavior.

## License

grovedb_cxx is released under the terms of the MIT license. See the accompanying file [LICENSE](./LICENSE) or https://opensource.org/license/mit.

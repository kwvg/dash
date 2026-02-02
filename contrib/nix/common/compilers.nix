# Compiler version pinning overlay
# Pins specific GCC and Clang/LLVM versions for reproducible builds
{ nixpkgs, nixpkgs-unstable }:

final: prev:
let
  unstable = import nixpkgs-unstable {
    inherit (final) system;
    config.allowUnfree = false;
  };
in
{
  # GCC versions
  gcc11 = prev.gcc11; # From stable (lower bound testing)
  gcc13 = prev.gcc13; # From stable (default for most CI)
  gcc14 = prev.gcc14 or unstable.gcc14; # Fallback to unstable if not in stable
  gcc15 = unstable.gcc15; # Latest from unstable (upper bound testing)

  # Clang/LLVM 19 (for sanitizers, multiprocess, fuzz)
  clang_19 = unstable.clang_19;
  llvm_19 = unstable.llvm_19;
  llvmPackages_19 = unstable.llvmPackages_19;

  # LLD linker (for macOS cross-compilation)
  lld_19 = unstable.lld_19;
}

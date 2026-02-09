{ pkgs, aflplusplus }:

pkgs.mkShell {
  name = "grovedb-fuzz";

  packages = [
    pkgs.llvmPackages_20.clang
    pkgs.llvmPackages_20.clang-tools
    pkgs.llvmPackages_20.lld
    pkgs.llvmPackages_20.compiler-rt
    pkgs.meson
    pkgs.ninja
    pkgs.python3
    pkgs.zsh
    pkgs.rsync
    pkgs.nixfmt
    aflplusplus
  ];

  env.AFL_PATH = "${aflplusplus}/lib/afl";
  # Use clang's built-in SanitizerCoverage instead of AFL++'s LLVM pass
  # plugin, which is ABI-incompatible across different LLVM builds.
  env.AFL_LLVM_INSTRUMENT = "NATIVE";
  # Default shared-memory map to 64 KB; macOS shmmax (4 MB) rejects the
  # default AFL++ request.  Override with AFL_MAP_SIZE in the shell if needed.
  env.AFL_MAP_SIZE = "65536";

  shellHook = ''
    export CC=clang
    export CXX=clang++
    export LD=ld.lld
    echo "GroveDB fuzzing devenv active ($(clang --version | head -1))"
  '';
}

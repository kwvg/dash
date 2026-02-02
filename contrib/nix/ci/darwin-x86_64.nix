# ci_darwin-x86_64 - Clang 19 macOS cross-compilation
{ pkgs, helpers, testEnvShellHook }:

helpers.mkCIEnv {
  name = "dash-ci-darwin-x86_64";
  compiler = pkgs.clang_19;
  extraBuildInputs = [ pkgs.llvm_19 pkgs.lld_19 ];
  inherit testEnvShellHook;
  extraShellHook = ''
    export CI_TARGET="darwin-x86_64"
    export HOST="x86_64-apple-darwin"
    export CONFIGURE_FLAGS="--enable-reduce-exports --enable-werror"
    echo "CI Target: darwin-x86_64"
    echo "  Host: x86_64-apple-darwin"
    echo "  Compiler: Clang 19 (cross-compile)"
    echo "  Features: macOS cross-compilation with LLD linker"
    echo ""
    echo "Build commands:"
    echo "  ./autogen.sh"
    echo "  make -C depends HOST=\$HOST -j\$(nproc)"
    echo "  ./configure --prefix=\$(pwd)/depends/\$HOST \$CONFIGURE_FLAGS"
    echo "  make -j\$(nproc)"
  '';
}

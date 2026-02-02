# linux64_multiprocess - Clang 19 with multiprocess
{ pkgs, helpers, testEnvShellHook }:

helpers.mkCIEnv {
  name = "dash-ci-linux64-multiprocess";
  compiler = pkgs.clang_19;
  extraBuildInputs = [ pkgs.llvm_19 ];
  inherit testEnvShellHook;
  extraShellHook = ''
    export CI_TARGET="linux64_multiprocess"
    export HOST="x86_64-pc-linux-gnu"
    export CONFIGURE_FLAGS="--enable-reduce-exports --with-boost-process --enable-multiprocess CC=clang CXX=clang++"
    echo "CI Target: linux64_multiprocess"
    echo "  Host: x86_64-pc-linux-gnu"
    echo "  Compiler: Clang 19"
    echo "  Features: Multiprocess node/wallet separation"
    echo ""
    echo "Build commands:"
    echo "  ./autogen.sh"
    echo "  make -C depends HOST=\$HOST -j\$(nproc)"
    echo "  ./configure --prefix=\$(pwd)/depends/\$HOST \$CONFIGURE_FLAGS"
    echo "  make -j\$(nproc)"
  '';
}

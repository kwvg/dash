# linux64 - GCC 15, full build with wallet and GUI
# Matches CI_TARGET=linux64 in ci.Dockerfile
{ pkgs, helpers, testEnvShellHook }:

helpers.mkCIEnv {
  name = "dash-ci-linux64";
  compiler = pkgs.gcc15;
  inherit testEnvShellHook;
  extraShellHook = ''
    export CI_TARGET="linux64"
    export HOST="x86_64-pc-linux-gnu"
    export CONFIGURE_FLAGS="--enable-reduce-exports --with-boost-process"
    echo "CI Target: linux64"
    echo "  Host: x86_64-pc-linux-gnu"
    echo "  Compiler: GCC 15"
    echo "  Features: Wallet (BDB + SQLite), GUI"
    echo ""
    echo "Build commands:"
    echo "  ./autogen.sh"
    echo "  make -C depends HOST=\$HOST -j\$(nproc)"
    echo "  ./configure --prefix=\$(pwd)/depends/\$HOST \$CONFIGURE_FLAGS"
    echo "  make -j\$(nproc)"
  '';
}

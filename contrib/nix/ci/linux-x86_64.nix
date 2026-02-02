# linux-x86_64 - GCC 11, full build with wallet and GUI
# Targets: linux-x86_64 in ci.Dockerfile (native_qt5)
{
  pkgs,
  helpers,
  testEnvShellHook,
}:

helpers.mkCIEnv {
  name = "dash-ci-linux-x86_64";
  compiler = pkgs.gcc11;
  inherit testEnvShellHook;
  extraShellHook = ''
    export CI_TARGET="linux-x86_64"
    export HOST="x86_64-pc-linux-gnu"
    export CONFIGURE_FLAGS="--enable-reduce-exports --with-boost-process"
    echo "CI Target: linux-x86_64"
    echo "  Host: x86_64-pc-linux-gnu"
    echo "  Compiler: GCC 11"
    echo "  Features: Wallet (BDB + SQLite), GUI"
    echo ""
    echo "Build commands:"
    echo "  ./autogen.sh"
    echo "  make -C depends HOST=\$HOST -j\$(nproc)"
    echo "  ./configure --prefix=\$(pwd)/depends/\$HOST \$CONFIGURE_FLAGS"
    echo "  make -j\$(nproc)"
  '';
}

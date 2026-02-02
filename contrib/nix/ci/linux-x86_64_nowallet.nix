# linux-x86_64_nowallet - GCC 15, no wallet, no GUI
# Targets: linux-x86_64_nowallet in ci.Dockerfile
{ pkgs, helpers, testEnvShellHook }:

helpers.mkCIEnv {
  name = "dash-ci-linux-x86_64-nowallet";
  compiler = pkgs.gcc13;
  inherit testEnvShellHook;
  extraShellHook = ''
    export CI_TARGET="linux-x86_64_nowallet"
    export HOST="x86_64-pc-linux-gnu"
    export CONFIGURE_FLAGS="--disable-wallet --without-gui --without-bdb --without-sqlite --enable-reduce-exports"
    export MAKE_FLAGS="NO_WALLET=1"
    echo "CI Target: linux-x86_64_nowallet"
    echo "  Host: x86_64-pc-linux-gnu"
    echo "  Compiler: GCC 13"
    echo "  Features: NO wallet, NO GUI"
    echo ""
    echo "Build commands:"
    echo "  ./autogen.sh"
    echo "  make -C depends HOST=\$HOST \$MAKE_FLAGS -j\$(nproc)"
    echo "  ./configure --prefix=\$(pwd)/depends/\$HOST \$CONFIGURE_FLAGS"
    echo "  make -j\$(nproc)"
  '';
}

# linux64_sqlite - GCC 15 with SQLite wallet (no BDB)
{ pkgs, helpers, testEnvShellHook }:

helpers.mkCIEnv {
  name = "dash-ci-linux64-sqlite";
  compiler = pkgs.gcc15;
  inherit testEnvShellHook;
  extraShellHook = ''
    export CI_TARGET="linux64_sqlite"
    export HOST="x86_64-pc-linux-gnu"
    export CONFIGURE_FLAGS="--enable-reduce-exports --with-boost-process --with-sqlite --without-bdb"
    echo "CI Target: linux64_sqlite"
    echo "  Host: x86_64-pc-linux-gnu"
    echo "  Compiler: GCC 15"
    echo "  Features: SQLite wallet only (no BDB)"
    echo ""
    echo "Build commands:"
    echo "  ./autogen.sh"
    echo "  make -C depends HOST=\$HOST -j\$(nproc)"
    echo "  ./configure --prefix=\$(pwd)/depends/\$HOST \$CONFIGURE_FLAGS"
    echo "  make -j\$(nproc)"
  '';
}

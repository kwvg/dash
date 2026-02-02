# win64 - GCC 15 Windows cross-compilation with Wine
# Only available on x86_64-linux (mingw requires x86)
{ pkgs, helpers, testEnvShellHook }:

helpers.mkCIEnv {
  name = "dash-ci-win64";
  compiler = pkgs.gcc15;
  extraBuildInputs = [ pkgs.pkgsCross.mingwW64.stdenv.cc pkgs.wine ];
  inherit testEnvShellHook;
  extraShellHook = ''
    export CI_TARGET="win64"
    export HOST="x86_64-w64-mingw32"
    export CONFIGURE_FLAGS="--enable-reduce-exports"
    echo "CI Target: win64"
    echo "  Host: x86_64-w64-mingw32"
    echo "  Compiler: GCC 15 (mingw-w64 cross-compile)"
    echo "  Features: Windows cross-compilation + Wine for testing"
    echo ""
    echo "Build commands:"
    echo "  ./autogen.sh"
    echo "  make -C depends HOST=\$HOST -j\$(nproc)"
    echo "  ./configure --prefix=\$(pwd)/depends/\$HOST \$CONFIGURE_FLAGS"
    echo "  make -j\$(nproc)"
    echo ""
    echo "Run Windows binaries with Wine:"
    echo "  wine ./src/test/test_dash.exe"
  '';
}

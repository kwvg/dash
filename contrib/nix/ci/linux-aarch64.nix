# ci_linux-aarch64 - GCC 13 ARM64 cross-compilation
# Targets: aarch64-linux-gnu
{ pkgs, helpers, testEnvShellHook }:

helpers.mkCIEnv {
  name = "dash-ci-linux-aarch64";
  compiler = pkgs.gcc13;
  extraBuildInputs = [ pkgs.pkgsCross.aarch64-multiplatform.stdenv.cc ];
  inherit testEnvShellHook;
  extraShellHook = ''
    export CI_TARGET="linux-aarch64"
    export HOST="aarch64-linux-gnu"
    export CONFIGURE_FLAGS="--enable-reduce-exports --enable-glibc-back-compat"
    echo "CI Target: linux-aarch64"
    echo "  Host: aarch64-linux-gnu"
    echo "  Compiler: GCC 13 (cross-compile)"
    echo "  Features: ARM 64-bit cross-compilation"
    echo ""
    echo "Build commands:"
    echo "  ./autogen.sh"
    echo "  make -C depends HOST=\$HOST -j\$(nproc)"
    echo "  ./configure --prefix=\$(pwd)/depends/\$HOST \$CONFIGURE_FLAGS"
    echo "  make -j\$(nproc)"
  '';
}

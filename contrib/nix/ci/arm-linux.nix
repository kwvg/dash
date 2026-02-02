# arm-linux - GCC 11 ARM cross-compilation
{ pkgs, helpers, testEnvShellHook }:

helpers.mkCIEnv {
  name = "dash-ci-arm-linux";
  compiler = pkgs.gcc11;
  extraBuildInputs = [ pkgs.pkgsCross.armv7l-hf-multiplatform.stdenv.cc ];
  inherit testEnvShellHook;
  extraShellHook = ''
    export CI_TARGET="arm-linux"
    export HOST="arm-linux-gnueabihf"
    export CONFIGURE_FLAGS="--enable-reduce-exports --enable-glibc-back-compat"
    echo "CI Target: arm-linux"
    echo "  Host: arm-linux-gnueabihf"
    echo "  Compiler: GCC 11 (cross-compile)"
    echo "  Features: ARM 32-bit cross-compilation"
    echo ""
    echo "Build commands:"
    echo "  ./autogen.sh"
    echo "  make -C depends HOST=\$HOST -j\$(nproc)"
    echo "  ./configure --prefix=\$(pwd)/depends/\$HOST \$CONFIGURE_FLAGS"
    echo "  make -j\$(nproc)"
  '';
}

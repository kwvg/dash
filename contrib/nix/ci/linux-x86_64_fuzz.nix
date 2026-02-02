# linux-x86_64_fuzz - Clang 19, fuzzing with libFuzzer
# Targets: linux-x86_64_fuzz in ci.Dockerfile
{
  pkgs,
  helpers,
  testEnvShellHook,
}:

helpers.mkCIEnv {
  name = "dash-ci-linux-x86_64-fuzz";
  compiler = pkgs.clang_19;
  extraBuildInputs = [ pkgs.llvm_19 ];
  inherit testEnvShellHook;
  extraShellHook = ''
    export CI_TARGET="linux-x86_64_fuzz"
    export HOST="x86_64-pc-linux-gnu"
    export CONFIGURE_FLAGS="--enable-fuzz --with-sanitizers=fuzzer,address,undefined --disable-wallet --without-gui --without-bdb --without-sqlite CC=clang CXX=clang++"
    export MAKE_FLAGS="NO_WALLET=1"
    echo "CI Target: linux-x86_64_fuzz"
    echo "  Host: x86_64-pc-linux-gnu"
    echo "  Compiler: Clang 19"
    echo "  Features: Fuzz testing (libFuzzer + ASan + UBSan)"
    echo ""
    echo "Build commands:"
    echo "  ./autogen.sh"
    echo "  make -C depends HOST=\$HOST \$MAKE_FLAGS -j\$(nproc)"
    echo "  ./configure --prefix=\$(pwd)/depends/\$HOST \$CONFIGURE_FLAGS"
    echo "  make -j\$(nproc)"
  '';
}

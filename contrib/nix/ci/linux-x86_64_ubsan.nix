# linux-x86_64_ubsan - Clang 19 with UndefinedBehaviorSanitizer
{ pkgs, helpers, testEnvShellHook }:

helpers.mkCIEnv {
  name = "dash-ci-linux-x86_64-ubsan";
  compiler = pkgs.clang_19;
  extraBuildInputs = [ pkgs.llvm_19 ];
  inherit testEnvShellHook;
  extraShellHook = ''
    export CI_TARGET="linux-x86_64_ubsan"
    export HOST="x86_64-pc-linux-gnu"
    export CONFIGURE_FLAGS="--enable-reduce-exports --with-sanitizers=undefined --with-boost-process CC=clang CXX=clang++"
    echo "CI Target: linux-x86_64_ubsan"
    echo "  Host: x86_64-pc-linux-gnu"
    echo "  Compiler: Clang 19"
    echo "  Features: UndefinedBehaviorSanitizer (UBSan)"
    echo ""
    echo "Build commands:"
    echo "  ./autogen.sh"
    echo "  make -C depends HOST=\$HOST -j\$(nproc)"
    echo "  ./configure --prefix=\$(pwd)/depends/\$HOST \$CONFIGURE_FLAGS"
    echo "  make -j\$(nproc)"
  '';
}

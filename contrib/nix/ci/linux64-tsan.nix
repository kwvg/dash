# linux64_tsan - Clang 19 with ThreadSanitizer
{ pkgs, helpers, testEnvShellHook }:

helpers.mkCIEnv {
  name = "dash-ci-linux64-tsan";
  compiler = pkgs.clang_19;
  extraBuildInputs = [ pkgs.llvm_19 ];
  inherit testEnvShellHook;
  extraShellHook = ''
    export CI_TARGET="linux64_tsan"
    export HOST="x86_64-pc-linux-gnu"
    export CONFIGURE_FLAGS="--enable-reduce-exports --with-sanitizers=thread --with-boost-process CC=clang CXX=clang++"
    echo "CI Target: linux64_tsan"
    echo "  Host: x86_64-pc-linux-gnu"
    echo "  Compiler: Clang 19"
    echo "  Features: ThreadSanitizer (TSan) for race detection"
    echo ""
    echo "Build commands:"
    echo "  ./autogen.sh"
    echo "  make -C depends HOST=\$HOST -j\$(nproc)"
    echo "  ./configure --prefix=\$(pwd)/depends/\$HOST \$CONFIGURE_FLAGS"
    echo "  make -j\$(nproc)"
  '';
}

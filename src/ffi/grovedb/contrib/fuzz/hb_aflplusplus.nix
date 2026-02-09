# AFL++ pre-built Homebrew bottle for macOS.
#
# Fetches pinned bottles from GitHub Container Registry (GHCR) using an
# anonymous pull token.  No Homebrew installation required.
#
# Available osTag values (from https://formulae.brew.sh/api/formula/afl++.json):
#   arm64_tahoe    - macOS 26 Tahoe    (ARM64)
#   arm64_sequoia  - macOS 15 Sequoia  (ARM64)
#   arm64_sonoma   - macOS 14 Sonoma   (ARM64)
#   sonoma         - macOS 14 Sonoma   (x86_64)
#
# The default (arm64_sonoma) is forward-compatible with newer macOS versions.
{
  lib,
  stdenvNoCC,
  curl,
  cacert,
  jq,
  python314,
  darwin,
  osTag ? "arm64_sonoma",
}:

let
  version = "4.35c";
  repo = "homebrew/core/aflxx";

  bottles = {
    arm64_tahoe = {
      sha256 = "ea23bdd9186a4cfa39b5fbf8634d1e4d34dc1a2733417acb75a20fe1c5fbbb45";
    };
    arm64_sequoia = {
      sha256 = "b1e041792c9906ce815d937f71e05b230940b39bc99928f94fef6dbb3e67a743";
    };
    arm64_sonoma = {
      sha256 = "3e346420077002bb673198f9ed7a07d7a96aea31c70bf614479ae0cf1a871851";
    };
    sonoma = {
      sha256 = "57372ca3578b746aaebe19cd603fabd5a0e06ac5679a70184a670ab067bb7e63";
    };
  };

  bottle =
    bottles.${osTag}
      or (throw "hb_aflplusplus: unknown osTag '${osTag}'; valid: ${builtins.concatStringsSep ", " (builtins.attrNames bottles)}");

  src = stdenvNoCC.mkDerivation {
    name = "aflplusplus-bottle-${version}-${osTag}.tar.gz";

    nativeBuildInputs = [
      curl
      cacert
      jq
    ];

    phases = [ "installPhase" ];

    installPhase = ''
      token=$(curl -fsSL \
        "https://ghcr.io/token?scope=repository:${repo}:pull" \
        | jq -r .token)
      curl -fsSL \
        -H "Authorization: Bearer $token" \
        -o "$out" \
        "https://ghcr.io/v2/${repo}/blobs/sha256:${bottle.sha256}"
    '';

    outputHashMode = "flat";
    outputHashAlgo = "sha256";
    outputHash = bottle.sha256;

    SSL_CERT_FILE = "${cacert}/etc/ssl/certs/ca-bundle.crt";
  };

  cctools = darwin.cctools;
in
stdenvNoCC.mkDerivation {
  pname = "aflplusplus";
  inherit version;

  inherit src;

  nativeBuildInputs = [ cctools ];

  phases = [
    "installPhase"
    "fixupPhase"
  ];

  installPhase = ''
    mkdir -p "$out"
    tar xzf "$src" --strip-components=2 -C "$out"

    # Rewrite bin/ wrapper scripts: replace Homebrew placeholders with Nix
    # store paths.  The wrappers prepend Homebrew LLVM to PATH, but in the
    # Nix devShell LLVM is already in PATH, so we drop that prefix entirely.
    for f in "$out"/bin/*; do
      [ -f "$f" ] || continue
      head -1 "$f" | grep -q '^#!' || continue
      sed -i \
        -e 's|@@HOMEBREW_CELLAR@@/afl++/${version}|'"$out"'|g' \
        -e 's|@@HOMEBREW_PREFIX@@/opt/llvm/bin:||g' \
        -e 's|@@HOMEBREW_PREFIX@@|'"$out"'|g' \
        "$f"
    done

    # Fix Python 3.14 framework reference in Mach-O binaries.
    # Homebrew links against its Python.framework; we repoint to Nix's
    # libpython3.14.dylib instead.
    for bin in "$out"/libexec/afl-fuzz "$out"/libexec/afl-showmap "$out"/libexec/afl-tmin; do
      [ -f "$bin" ] || continue
      install_name_tool -change \
        '@@HOMEBREW_PREFIX@@/opt/python@3.14/Frameworks/Python.framework/Versions/3.14/Python' \
        '${python314}/lib/libpython3.14.dylib' \
        "$bin" 2>/dev/null || true
    done

    # Fix install names in LLVM instrumentation passes.
    for so in "$out"/lib/afl/*.so; do
      [ -f "$so" ] || continue
      install_name_tool -id "$out/lib/afl/$(basename "$so")" "$so" 2>/dev/null || true
    done

    # Re-sign all modified Mach-O binaries (install_name_tool invalidates the
    # original Homebrew ad-hoc signature; macOS kills unsigned arm64 binaries).
    for bin in "$out"/libexec/*; do
      [ -f "$bin" ] || continue
      if file "$bin" | grep -q 'Mach-O'; then
        /usr/bin/codesign --force --sign - "$bin"
      fi
    done
    for so in "$out"/lib/afl/*.so; do
      [ -f "$so" ] || continue
      /usr/bin/codesign --force --sign - "$so"
    done
  '';

  meta = {
    description = "AFL++ fuzzer (pre-built Homebrew bottle)";
    homepage = "https://aflplus.plus/";
    license = lib.licenses.asl20;
    platforms = [ "aarch64-darwin" ];
  };
}

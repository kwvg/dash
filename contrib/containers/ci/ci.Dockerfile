# syntax = devthefuture/dockerfile-x

FROM ./ci-slim.Dockerfile

# The inherited Dockerfile switches to non-privileged context and we've
# just started configuring this image, give us root access
USER root

# Install packages
RUN set -ex; \
    apt-get update && apt-get install ${APT_ARGS} \
    autoconf \
    automake \
    autotools-dev \
    bc \
    bear \
    bison \
    bsdmainutils \
    ccache \
    cmake \
    g++-11 \
    g++ \
    g++-15 \
    g++-arm-linux-gnueabihf \
    g++-mingw-w64-x86-64 \
    gawk \
    gettext \
    libtool \
    m4 \
    parallel \
    pkg-config \
    wine \
    wine64 \
    zip \
    && rm -rf /var/lib/apt/lists/*

# Install Clang/LLVM and set defaults
# Note: The slim container already added the GPG key and installed llvm-${LLVM_VERSION} but ${LLVM_VERSION_SECONDARY} is installed only in the full container
ARG LLVM_VERSION_SECONDARY=21
RUN set -ex; \
    . /etc/os-release; \
    echo "deb [signed-by=/etc/apt/trusted.gpg.d/apt.llvm.org.asc] http://apt.llvm.org/${UBUNTU_CODENAME}/  llvm-toolchain-${UBUNTU_CODENAME}-${LLVM_VERSION_SECONDARY} main" >> /etc/apt/sources.list.d/llvm.list; \
    apt-get update; \
    for llvmVersion in ${LLVM_VERSION} ${LLVM_VERSION_SECONDARY}; do \
      apt-get install ${APT_ARGS} \
        "clang-${llvmVersion}" \
        "libclang-${llvmVersion}-dev" \
        "libclang-rt-${llvmVersion}-dev" \
        "lld-${llvmVersion}" \
        "llvm-${llvmVersion}"; \
    done; \
    apt-get install ${APT_ARGS} \
    "clangd-${LLVM_VERSION}" \
    "clang-format-${LLVM_VERSION}" \
    "clang-tidy-${LLVM_VERSION}" \
    "lldb-${LLVM_VERSION}"; \
    rm -rf /var/lib/apt/lists/*; \
    echo "Setting defaults for Clang/LLVM ${LLVM_VERSION}..."; \
    llvmUpdAltArgs="update-alternatives --install /usr/bin/llvm-config llvm-config /usr/bin/llvm-config-${LLVM_VERSION} 100"; \
    for binName in clang clang++ clang-apply-replacements clang-format clang-tidy clangd dsymutil lld lldb lldb-server llvm-ar llvm-cov llvm-nm llvm-objdump llvm-ranlib llvm-strip run-clang-tidy; do \
        llvmUpdAltArgs="${llvmUpdAltArgs} --slave /usr/bin/${binName} ${binName} /usr/bin/${binName}-${LLVM_VERSION}"; \
    done; \
    for binName in ld64.lld ld.lld lld-link wasm-ld; do \
        llvmUpdAltArgs="${llvmUpdAltArgs} --slave /usr/bin/${binName} ${binName} /usr/bin/lld-${LLVM_VERSION}"; \
    done; \
    sh -c "${llvmUpdAltArgs}";
# LD_LIBRARY_PATH is empty by default, this is the first entry
ENV LD_LIBRARY_PATH="/usr/lib/llvm-${LLVM_VERSION}/lib"

RUN set -ex; \
    git clone --depth=1 "https://github.com/include-what-you-use/include-what-you-use" -b "clang_${LLVM_VERSION}" /opt/iwyu; \
    cd /opt/iwyu; \
    mkdir build && cd build; \
    cmake -G 'Unix Makefiles' -DCMAKE_PREFIX_PATH=/usr/lib/llvm-${LLVM_VERSION} ..; \
    make install -j "$(( $(nproc) - 1 ))"; \
    cd /opt && rm -rf /opt/iwyu;

RUN \
  mkdir -p /cache/ccache && \
  mkdir /cache/depends && \
  mkdir /cache/sdk-sources && \
  chown ${USER_ID}:${GROUP_ID} /cache && \
  chown ${USER_ID}:${GROUP_ID} -R /cache

# We're done, switch back to non-privileged user
USER dash

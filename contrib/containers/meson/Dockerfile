# Version must match with `depends/packages/native_rust.mk`
ARG RUST_VERSION=1.88.0
FROM rust:${RUST_VERSION}-slim-trixie

# Needed to prevent tzdata hanging while expecting user input
ENV DEBIAN_FRONTEND="noninteractive" TZ="Europe/London"

# Build and base stuff
ENV APT_ARGS="-y --no-install-recommends --no-upgrade"

# Install build dependencies
RUN set -ex; \
    apt-get update && apt-get install ${APT_ARGS} \
    clang \
    git \
    libboost-test-dev \
    libssl-dev \
    ninja-build \
    pkg-config \
    python3-pip \
    && rm -rf /var/lib/apt/lists/*; \
    pip install --break-system-packages meson

# Install Rust build dependencies
ARG CXXBRIDGE_VERSION=1.0.192
ARG SCCACHE_VERSION=0.12.0
RUN cargo install cxxbridge-cmd@${CXXBRIDGE_VERSION} sccache@${SCCACHE_VERSION} \
    && rm -rf /usr/local/cargo/registry

# Setup unprivileged user
ENV CC=clang \
    CXX=clang++
ARG USER_ID=1000 \
    GROUP_ID=1000
RUN set -ex; \
    groupadd -g ${GROUP_ID} build; \
    useradd -u ${USER_ID} -g ${GROUP_ID} -s /bin/bash -m -d /home/build build && \
    mkdir -p /cache/sccache /home/build /src && \
    chown ${USER_ID}:${GROUP_ID} /cache /src && \
    chown ${USER_ID}:${GROUP_ID} -R /cache /home/build

WORKDIR /src
USER build

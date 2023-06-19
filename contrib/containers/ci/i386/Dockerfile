# syntax = edrevo/dockerfile-plus

FROM ubuntu:focal

ENV APT_ARGS="-y --no-install-recommends --no-upgrade"

RUN dpkg --add-architecture i386

RUN apt-get update && apt-get install $APT_ARGS \
    g++-9-multilib \
    wine32  \
    && rm -rf /var/lib/apt/lists/*

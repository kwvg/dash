# syntax = edrevo/dockerfile-plus
FROM ubuntu:focal

RUN dpkg --add-architecture i386

INCLUDE+ common/Dockerfile
INCLUDE+ i386/Dockerfile

INCLUDE+ common/Dockerfile.finish

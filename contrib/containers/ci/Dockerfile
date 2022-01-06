# syntax = edrevo/dockerfile-plus
FROM ubuntu:focal

RUN dpkg --add-architecture i386

INCLUDE+ common/Dockerfile
INCLUDE+ 32bit/Dockerfile

INCLUDE+ common/Dockerfile.finish

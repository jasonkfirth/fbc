# Project: FreeBASIC Windows CE release workflow
# ------------------------------------------------
#
# File: build_scripts/wince/Dockerfile
#
# Purpose:
#
#     Provide a reproducible Linux-hosted ARM Windows CE build environment.
#
# Responsibilities:
#
#     - pin the maintained CeGCC 9.3 ARM toolchain by image digest
#     - provide the host tools needed to bootstrap FreeBASIC
#     - keep obsolete Windows CE cross tools off the Ubuntu host
#
# This file intentionally does NOT contain:
#
#     - FreeBASIC source construction
#     - Windows CE ROM images
#     - MIPS Windows CE toolchain construction

FROM ghcr.io/enlyze/windows-ce-build-environment-arm@sha256:22ea23d697accaab2b98fb12d81ccc1faade90f9fe490f554ae7c5c194dfe27c AS cegcc-arm

FROM ubuntu:24.04

ARG DEBIAN_FRONTEND=noninteractive

COPY --from=cegcc-arm /opt/cegcc-arm /opt/cegcc-arm

ENV PATH="/opt/cegcc-arm/bin:${PATH}"

RUN apt-get update \
 && apt-get install -y --no-install-recommends \
      bash \
      binutils \
      build-essential \
      ca-certificates \
      curl \
      autoconf \
      automake \
      file \
      gawk \
      git \
      libffi-dev \
      libtool \
      libncurses-dev \
      make \
      pkg-config \
      python3 \
      rsync \
      unzip \
      xz-utils \
      zip \
 && rm -rf /var/lib/apt/lists/*

RUN arm-mingw32ce-gcc -dumpmachine | grep -Fx arm-mingw32ce \
 && arm-mingw32ce-gcc -dumpversion | grep -Fx 9.3.0

WORKDIR /work

# end of build_scripts/wince/Dockerfile

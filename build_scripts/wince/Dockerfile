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

# Keep Ubuntu's default mirrors when they resolve; use an independent full
# archive mirror when the runner's Ubuntu mirror DNS is unavailable.
RUN set -eu; \
  mirror_dns_failed=0; \
  for mirror_host in azure.archive.ubuntu.com archive.ubuntu.com security.ubuntu.com; do \
    if ! getent ahosts "$mirror_host" >/dev/null 2>&1; then \
      mirror_dns_failed=1; \
      break; \
    fi; \
  done; \
  if [ "$mirror_dns_failed" -ne 0 ]; then \
    if getent ahosts mirrors.edge.kernel.org >/dev/null 2>&1; then \
      if [ -f /etc/apt/apt-mirrors.txt ]; then \
        printf '%s\n' 'https://mirrors.edge.kernel.org/ubuntu/' > /etc/apt/apt-mirrors.txt; \
      fi; \
      for source_file in /etc/apt/sources.list /etc/apt/sources.list.d/*.sources /etc/apt/sources.list.d/*.list; do \
        [ -f "$source_file" ] || continue; \
        sed -i -E 's#https?://(azure\.archive|archive|security)\.ubuntu\.com/ubuntu/?#https://mirrors.edge.kernel.org/ubuntu/#g' "$source_file"; \
      done; \
      echo "Ubuntu archive DNS failed; using the independent fallback mirror" >&2; \
    else \
      echo "WARNING: Ubuntu archive DNS and the fallback mirror are unavailable" >&2; \
    fi; \
  fi; \
  for attempt in 1 2 3; do \
    if apt-get -o Acquire::Retries=2 -o APT::Update::Error-Mode=any update \
      && apt-get -o Acquire::Retries=2 install -y --no-install-recommends \
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
      zip; then \
      rm -rf /var/lib/apt/lists/*; \
      exit 0; \
    fi; \
    if [ "$attempt" -lt 3 ]; then \
      sleep "$((attempt * 5))"; \
    fi; \
  done; \
  echo "ERROR: APT setup failed after 3 attempts" >&2; \
  exit 1

RUN arm-mingw32ce-gcc -dumpmachine | grep -Fx arm-mingw32ce \
 && arm-mingw32ce-gcc -dumpversion | grep -Fx 9.3.0

WORKDIR /work

# end of build_scripts/wince/Dockerfile

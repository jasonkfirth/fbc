# Project: FreeBASIC Windows CE emulator workflow
# ------------------------------------------------
#
# File: build_scripts/wince/emulator/Dockerfile
#
# Purpose:
#
#     Provide the Windows compatibility and virtual-display tools needed to
#     run the CERF Windows CE emulator on a Linux build host.
#
# Responsibilities:
#
#     - provide 32-bit Wine for the CERF Win32 executable
#     - provide an isolated X server for unattended emulator runs
#     - provide window and screenshot tools for runtime validation
#     - extract the public CERF archive and a user-supplied Microsoft ROM MSI
#
# This file intentionally does NOT contain:
#
#     - Windows CE ROM images or Microsoft redistributables
#     - the CERF release itself
#     - FreeBASIC compiler or target-library construction

FROM ubuntu@sha256:786a8b558f7be160c6c8c4a54f9a57274f3b4fb1491cf65146521ae77ff1dc54

ARG DEBIAN_FRONTEND=noninteractive

RUN dpkg --add-architecture i386 \
 && apt-get update \
 && apt-get install -y --no-install-recommends \
      ca-certificates \
      curl \
      fonts-dejavu-core \
      imagemagick \
      libgl1-mesa-dri:i386 \
      mesa-vulkan-drivers:i386 \
      msitools \
      procps \
      unzip \
      wine32:i386 \
      wine64 \
      xauth \
      xdotool \
      xvfb \
 && rm -rf /var/lib/apt/lists/*

ENV WINEARCH=win32
ENV WINEDEBUG=-all
ENV WINEPREFIX=/wine
ENV PATH="/usr/lib/wine:${PATH}"

WORKDIR /cerf

# end of build_scripts/wince/emulator/Dockerfile

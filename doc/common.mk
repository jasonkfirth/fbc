# Project: FreeBASIC documentation tools
# File: common.mk
# Purpose: Share host executable naming and compiler defaults.
# Responsibilities: Detect Windows shells and preserve caller tool overrides.
# This file intentionally does NOT contain application targets or build flags.

UNAME_S ?= $(shell uname -s 2>/dev/null || echo unknown)
ifneq (,$(filter MINGW% MSYS% CYGWIN%,$(UNAME_S)))
EXEEXT := .exe
else ifeq ($(OS),Windows_NT)
EXEEXT := .exe
else
EXEEXT :=
endif

FBC ?= fbc

# end of common.mk

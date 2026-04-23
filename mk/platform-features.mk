########################
# platform-features.mk #
########################
#
# Policy layer: defines build feature toggles derived strictly from
# TARGET_OS / TARGET_ARCH / ISA_FAMILY as resolved by platform.mk.
#
# This file must NOT:
#   - inspect distro identity
#   - inspect compiler support
#   - hardcode toolchain flags (except legacy compile-time feature macros)
#
# It defines feature booleans and policy modes which are later translated
# into concrete compiler/linker flags by toolchain-flags.mk.
#
# Defines:
#   ENABLE_PIC
#   ENABLE_NONPIC
#   DISABLE_MT
#   ENABLE_PIE
#   THREAD_MODEL
#
# Graphics policy:
#   ENABLE_X11
#   ENABLE_SDL
#
# Hardening policy:
#   ENABLE_STACK_PROTECTOR
#   ENABLE_FORTIFY
#   ENABLE_STACK_CLASH
#   ENABLE_FORMAT_SECURITY
#   ENABLE_RELRO
#   ENABLE_NOW
#   ENABLE_NOEXECSTACK
#   ENABLE_SEPARATE_CODE
#   ENABLE_CET
#   ENABLE_NO_PLT
#   ENABLE_AUTO_VAR_INIT
#   ENABLE_REPRODUCIBLE
#
# Legacy compile-time feature macros:
#   DISABLE_X11
#   DISABLE_OPENGL
#   DISABLE_FBDEV
#   DISABLE_D3D10
#   DISABLE_TCP
########################

# ---------------------------------------------------------------------------
# Reset to avoid stale values
# ---------------------------------------------------------------------------

ENABLE_PIC     :=
ENABLE_NONPIC  :=
DISABLE_MT     :=
ENABLE_PIE     :=
THREAD_MODEL   :=

ENABLE_X11     :=
ENABLE_SDL     :=

DISABLE_X11    :=
DISABLE_OPENGL :=
DISABLE_FBDEV  :=
DISABLE_D3D10  :=
DISABLE_TCP    :=

ENABLE_STACK_PROTECTOR :=
ENABLE_FORTIFY         :=
ENABLE_STACK_CLASH     :=
ENABLE_FORMAT_SECURITY :=
ENABLE_RELRO           :=
ENABLE_NOW             :=
ENABLE_NOEXECSTACK     :=
ENABLE_SEPARATE_CODE   :=
ENABLE_CET             :=
ENABLE_NO_PLT          :=
ENABLE_AUTO_VAR_INIT   :=
ENABLE_REPRODUCIBLE    :=

# ---------------------------------------------------------------------------
# Sanity guard
# ---------------------------------------------------------------------------

ifeq ($(strip $(TARGET_OS)),)
  $(error platform-features.mk: TARGET_OS not defined (include platform.mk first))
endif

# ---------------------------------------------------------------------------
# Platform families
# ---------------------------------------------------------------------------

ELF_UNIX_OS := linux freebsd netbsd openbsd dragonfly solaris haiku android
BSD_OS      := freebsd netbsd openbsd dragonfly
WINDOWS_OS  := win32 cygwin xbox
DOS_OS      := dos
JS_OS       := js

# ---------------------------------------------------------------------------
# PIC policy
# ---------------------------------------------------------------------------
#
# ELF hosted systems generally benefit from PIC support.
# Android is PIC-only in practice.
# Windows / DOS / JS do not use ELF-style PIC in this build model.
# Xbox is treated like win32 here.
# ---------------------------------------------------------------------------

ifneq ($(filter android openbsd,$(TARGET_OS)),)
 ENABLE_PIC    := YesPlease 
 ENABLE_NONPIC :=
else ifneq ($(filter $(ELF_UNIX_OS),$(TARGET_OS)),)
  ENABLE_PIC    := YesPlease
  ENABLE_NONPIC := YesPlease
else
  ENABLE_PIC    :=
  ENABLE_NONPIC := YesPlease
endif

ifneq ($(filter $(WINDOWS_OS) $(DOS_OS) $(JS_OS),$(TARGET_OS)),)
  ENABLE_PIC    :=
  ENABLE_NONPIC := YesPlease
endif

# ---------------------------------------------------------------------------
# PIE policy
# ---------------------------------------------------------------------------
#
# PIE is useful on major hosted ELF systems.
# Keep it disabled on DOS / JS / Windows-family / Xbox / Android by default.
# Cygwin is treated conservatively here.
# ---------------------------------------------------------------------------

ifneq ($(filter linux freebsd netbsd openbsd dragonfly solaris haiku,$(TARGET_OS)),)
  ENABLE_PIE := YesPlease
endif

# ---------------------------------------------------------------------------
# Threading policy
# ---------------------------------------------------------------------------

ifeq ($(TARGET_OS),win32)
  THREAD_MODEL := win32
else ifeq ($(TARGET_OS),xbox)
  THREAD_MODEL := win32
else ifeq ($(TARGET_OS),cygwin)
  THREAD_MODEL := posix
else ifneq ($(filter linux android freebsd netbsd openbsd dragonfly solaris haiku,$(TARGET_OS)),)
  THREAD_MODEL := posix
endif

ifeq ($(TARGET_OS),js)
  DISABLE_MT := YesPlease
  THREAD_MODEL :=
endif

# DOS stays single-threaded unless explicitly extended elsewhere
ifeq ($(TARGET_OS),dos)
  DISABLE_MT := YesPlease
  THREAD_MODEL :=
endif

# ---------------------------------------------------------------------------
# Graphics backend policy
# ---------------------------------------------------------------------------

# Linux / BSD / Solaris / Cygwin -> X11-oriented builds
ifneq ($(filter linux freebsd netbsd openbsd dragonfly solaris cygwin,$(TARGET_OS)),)
  ENABLE_X11 := YesPlease
endif

# Haiku -> native Haiku backend
ifeq ($(TARGET_OS),haiku)
  ENABLE_X11 :=
  ENABLE_SDL :=
  DISABLE_X11 := YesPlease
  DISABLE_OPENGL := YesPlease
  DISABLE_FBDEV := YesPlease
endif

# Android -> SDL-style portability layer; no X11
ifeq ($(TARGET_OS),android)
  ENABLE_X11 :=
  ENABLE_SDL := YesPlease
  DISABLE_X11 := YesPlease
  DISABLE_OPENGL := YesPlease
  DISABLE_FBDEV := YesPlease
endif

# Darwin -> native Cocoa/CoreGraphics path, not X11
ifeq ($(TARGET_OS),darwin)
  ENABLE_X11 :=
  DISABLE_X11 := YesPlease
  DISABLE_OPENGL := YesPlease
endif

# Windows / Xbox -> native backend, not X11
ifneq ($(filter win32 xbox,$(TARGET_OS)),)
  ENABLE_X11 :=
endif

# DOS -> VGA/VESA only
ifeq ($(TARGET_OS),dos)
  ENABLE_X11 :=
  ENABLE_SDL :=
  DISABLE_X11 := YesPlease
  DISABLE_OPENGL := YesPlease
endif

# JavaScript -> browser backend, not X11
ifeq ($(TARGET_OS),js)
  ENABLE_X11 :=
  ENABLE_SDL :=
  DISABLE_X11 := YesPlease
  DISABLE_TCP := YesPlease
endif

# DOS / Xbox -> no hosted sockets in the current runtime
ifneq ($(filter dos xbox,$(TARGET_OS)),)
  DISABLE_TCP := YesPlease
endif

# ---------------------------------------------------------------------------
# Hardening policy
# ---------------------------------------------------------------------------
#
# This section chooses a conservative baseline by OS family.
# It intentionally does NOT try to distinguish Debian/Fedora/Arch/Gentoo.
# For TARGET_OS=linux we choose a safe common baseline.
#
# More aggressive features like CET / no-plt / auto-var-init remain opt-in
# by platform/arch and can still be overridden by packagers.
# ---------------------------------------------------------------------------

# ---- Main hosted ELF baseline: Linux, each BSD, Solaris, Haiku ----
ifneq ($(filter linux netbsd freebsd haiku,$(TARGET_OS)),)

  ENABLE_STACK_PROTECTOR := YesPlease
  ENABLE_FORTIFY         := YesPlease
  ENABLE_STACK_CLASH     := YesPlease
  ENABLE_FORMAT_SECURITY := YesPlease

  ENABLE_RELRO           := YesPlease
  ENABLE_NOW             := YesPlease
  ENABLE_NOEXECSTACK     := YesPlease
  ENABLE_SEPARATE_CODE   := YesPlease

  ENABLE_REPRODUCIBLE    := YesPlease

endif

# ---- OpenBSD Specific Flags ----
ifneq ($(filter openbsd,$(TARGET_OS)),)

  ENABLE_STACK_PROTECTOR := 
  ENABLE_FORTIFY         := YesPlease
  ENABLE_STACK_CLASH     := YesPlease
  ENABLE_FORMAT_SECURITY := YesPlease

  ENABLE_RELRO           := YesPlease
  ENABLE_NOW             := YesPlease
  ENABLE_NOEXECSTACK     := YesPlease
  ENABLE_SEPARATE_CODE   := YesPlease

  ENABLE_REPRODUCIBLE    := YesPlease

endif

# ---- DragonFly BSD Specific Flags ----
ifneq ($(filter dragonfly,$(TARGET_OS)),)

  ENABLE_STACK_PROTECTOR := YesPlease
  ENABLE_FORTIFY         := YesPlease
  ENABLE_STACK_CLASH     := YesPlease
  ENABLE_FORMAT_SECURITY := YesPlease

  ENABLE_RELRO           := YesPlease
  ENABLE_NOW             := YesPlease
  ENABLE_NOEXECSTACK     := YesPlease
  ENABLE_SEPARATE_CODE   := 

  ENABLE_REPRODUCIBLE    := YesPlease

endif

# ---- Solaris Specific Flags ----
ifneq ($(filter solaris,$(TARGET_OS)),)

  ENABLE_STACK_PROTECTOR :=
  ENABLE_FORTIFY         :=
  ENABLE_STACK_CLASH     :=
  ENABLE_FORMAT_SECURITY :=

  ENABLE_RELRO           :=
  ENABLE_NOW             :=
  ENABLE_NOEXECSTACK     :=
  ENABLE_SEPARATE_CODE   :=

  ENABLE_REPRODUCIBLE    := YesPlease

endif

# ---- Android ----
#
# Android is ELF, but its libc/toolchain environment differs enough that
# fortify / stack-clash / z,now defaults should stay conservative here.
# Keep the policy smaller unless explicitly overridden.
ifeq ($(TARGET_OS),android)

  ENABLE_STACK_PROTECTOR := YesPlease
  ENABLE_FORMAT_SECURITY := YesPlease
  ENABLE_RELRO           := YesPlease
  ENABLE_NOEXECSTACK     := YesPlease
  ENABLE_REPRODUCIBLE    := YesPlease

  ENABLE_FORTIFY         :=
  ENABLE_STACK_CLASH     :=
  ENABLE_NOW             :=
  ENABLE_SEPARATE_CODE   :=

endif

# ---- Cygwin ----
#
# Cygwin behaves like POSIX userland on PE/COFF. Most ELF linker hardening
# flags do not apply. Keep reproducibility and format checks only.
ifeq ($(TARGET_OS),cygwin)

  ENABLE_STACK_PROTECTOR := YesPlease
  ENABLE_FORMAT_SECURITY := YesPlease
  ENABLE_REPRODUCIBLE    := YesPlease

endif

# ---- Native Windows / Xbox ----
#
# These do not use ELF hardening knobs. Keep reproducibility support only.
ifneq ($(filter win32 xbox,$(TARGET_OS)),)

  ENABLE_REPRODUCIBLE := YesPlease

endif

# ---- DOS / JavaScript ----
#
# No modern native hardening defaults here.
ifneq ($(filter dos js,$(TARGET_OS)),)

  ENABLE_STACK_PROTECTOR :=
  ENABLE_FORTIFY         :=
  ENABLE_STACK_CLASH     :=
  ENABLE_FORMAT_SECURITY :=
  ENABLE_RELRO           :=
  ENABLE_NOW             :=
  ENABLE_NOEXECSTACK     :=
  ENABLE_SEPARATE_CODE   :=
  ENABLE_REPRODUCIBLE    :=

endif

# ---------------------------------------------------------------------------
# Arch/CPU-specific hardening adjustments
# ---------------------------------------------------------------------------

# CET is meaningful on x86-family hosted targets where supported.
ifneq ($(filter x86 x86_64,$(TARGET_ARCH)),)
  ifneq ($(filter linux freebsd netbsd openbsd dragonfly solaris haiku,$(TARGET_OS)),)
    ENABLE_CET := YesPlease
  endif
endif

# -fno-plt is an ELF-centric optimization/hardening knob.
# Keep it enabled for mainstream hosted ELF platforms, but not Android.
ifneq ($(filter linux freebsd netbsd openbsd dragonfly solaris haiku,$(TARGET_OS)),)
  ENABLE_NO_PLT := YesPlease
endif

# Auto var init is valuable but somewhat more toolchain-sensitive.
# Restrict to mainstream hosted ELF targets; packagers can disable if needed.
ifneq ($(filter linux freebsd solaris haiku,$(TARGET_OS)),)
  ENABLE_AUTO_VAR_INIT := YesPlease
endif

# DOS / JS / Windows-family should never advertise CET / no-plt / auto-init.
ifneq ($(filter dos js win32 cygwin xbox android,$(TARGET_OS)),)
  ENABLE_CET           :=
  ENABLE_NO_PLT        :=
  ENABLE_AUTO_VAR_INIT :=
endif

# ---------------------------------------------------------------------------
# Translate graphics/backend policy into compile-time feature macros
# ---------------------------------------------------------------------------

ifdef DISABLE_X11
  ALLCFLAGS += -DDISABLE_X11
endif

ifdef DISABLE_OPENGL
  ALLCFLAGS += -DDISABLE_OPENGL
endif

ifdef DISABLE_FBDEV
  ALLCFLAGS += -DDISABLE_FBDEV
endif

ifdef DISABLE_D3D10
  ALLCFLAGS += -DDISABLE_D3D10
endif

ifdef DISABLE_TCP
  ALLCFLAGS += -DDISABLE_TCP
  FBFLAGS += -d DISABLE_TCP
endif

###############################
# end of platform-features.mk #
###############################

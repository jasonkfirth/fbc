##############################################################################
# bootstrap-emit.mk
##############################################################################

.PHONY: bootstrap-emit

##############################################################################
# Bootstrap directory identity
##############################################################################

BOOTSTRAP_DIR := $(if $(strip $(FBTARGET_DIR_OVERRIDE)),$(FBTARGET_DIR_OVERRIDE),$(FBTARGET))

BOOTSTRAP_OUT := bootstrap/$(BOOTSTRAP_DIR)

##############################################################################
# Stage0 compiler selection
##############################################################################

BOOT_FBC := $(AVAILABLE_FBC)

# Ensure bootstrap emission uses this tree's local bin/lib layout.
BOOT_FBC_BUILD_ROOT := $(rootdir)
ifneq ($(filter MSYS% MINGW%,$(shell uname -s 2>/dev/null)),)
BOOT_FBC_BUILD_ROOT := $(shell cygpath -m "$(rootdir)")
endif
BOOT_FBC_PREFIX_OPT := -prefix $(BOOT_FBC_BUILD_ROOT)
BOOT_TOOLCHAIN_BINDIR :=
ifneq ($(findstring /,$(CC))$(findstring \,$(CC)),)
BOOT_TOOLCHAIN_BINDIR := $(patsubst %/,%,$(dir $(CC)))
endif
BOOT_FBC_TOOL_ENV := env \
	PATH='$(if $(strip $(BOOT_TOOLCHAIN_BINDIR)),$(BOOT_TOOLCHAIN_BINDIR):)'"$$PATH" \
	AS='$(AS)' \
	AR='$(AR)' \
	LD='$(LD)' \
	GCC='$(CC)' \
	CLANG='$(CLANG)' \
	LLC='$(LLC)' \
	DLLTOOL='$(DLLTOOL)' \
	WINDRES='$(WINDRES)' \
	GORC='$(GORC)' \
	EMAS='$(EMAS)' \
	EMAR='$(EMAR)' \
	EMLD='$(EMLD)' \
	EMCC='$(EMCC)' \
	CXBE='$(CXBE)' \
	DXEGEN='$(DXEGEN)'

BOOTSTRAP_OS  := $(word 1,$(subst -, ,$(BOOTSTRAP_DIR)))

ifeq ($(BOOTSTRAP_OS),mingw)
BOOTSTRAP_OS := win32
endif

##############################################################################
# Canonical arch
##############################################################################

BOOTSTRAP_ARCH := $(TARGET_ARCH)

#
# The bootstrap emitter forwards this value directly to `fbc -arch`.
# The normalized build identity uses `x86`, but the compiler expects a
# concrete 32-bit sub-architecture such as 386/486/586/686 here.
# Use 686 as the stable default for generic 32-bit x86 bootstrap emission.
#
ifeq ($(BOOTSTRAP_ARCH),x86)
BOOTSTRAP_ARCH := 686
endif

#
# ARM targets also need a concrete CPU subtype for `fbc -arch`.
# Reuse the build's selected DEFAULT_CPUTYPE_ARM policy so emitted
# bootstrap sources match the current target configuration.
#
ifeq ($(BOOTSTRAP_ARCH),arm)
  ifeq ($(DEFAULT_CPUTYPE_ARM),FB_CPUTYPE_ARMV6)
    BOOTSTRAP_ARCH := armv6
  else ifeq ($(DEFAULT_CPUTYPE_ARM),FB_CPUTYPE_ARMV6_FP)
    BOOTSTRAP_ARCH := armv6+fp
  else ifeq ($(DEFAULT_CPUTYPE_ARM),FB_CPUTYPE_ARMV7A)
    BOOTSTRAP_ARCH := armv7-a
  else ifeq ($(DEFAULT_CPUTYPE_ARM),FB_CPUTYPE_ARMV7A_FP)
    BOOTSTRAP_ARCH := armv7-a+fp
  else ifeq ($(ARM_FLOAT_ABI),hf)
    BOOTSTRAP_ARCH := armv7-a+fp
  else
    BOOTSTRAP_ARCH := armv7-a
  endif
endif

#
# Older stage0 compilers may accept newer -arch values but fail to expose the
# corresponding __FB_* CPU family define while preprocessing compiler sources.
# Seed those defines here so bootstrap emission remains compatible.
#
BOOTSTRAP_COMPAT_DEFINES :=
ifeq ($(TARGET_ARCH),aarch64)
BOOTSTRAP_COMPAT_DEFINES += -d __FB_AARCH64__
endif
ifeq ($(TARGET_ARCH),riscv64)
BOOTSTRAP_COMPAT_DEFINES += -d __FB_RISCV64__
endif
ifeq ($(TARGET_ARCH),s390x)
BOOTSTRAP_COMPAT_DEFINES += -d __FB_S390X__
endif
ifeq ($(TARGET_ARCH),loongarch64)
BOOTSTRAP_COMPAT_DEFINES += -d __FB_LOONGARCH64__
endif

##############################################################################
# Determine emission target
##############################################################################

BOOT_EMIT_TARGET := $(BOOTSTRAP_OS)

##############################################################################
# Compiler sources
##############################################################################

BOOTSTRAP_COMPILER_SRC := $(FBC_SRC)

##############################################################################
# Bootstrap emission
##############################################################################

bootstrap-emit: bootstrap-check
	@echo "==> Emitting bootstrap sources"
	@echo "==> compiler : $(BOOT_FBC)"
	@echo "==> target   : $(BOOT_EMIT_TARGET)"
	@echo "==> arch     : $(if $(BOOTSTRAP_ARCH),$(BOOTSTRAP_ARCH),default)"
	@echo "==> output   : $(BOOTSTRAP_OUT)"

	@test -n "$(BOOT_FBC)" || { \
		echo "ERROR: no usable fbc found (system or built compiler)"; \
		exit 1; \
	}

	mkdir -p "$(BOOTSTRAP_OUT)"

	@echo "==> Cleaning previous bootstrap output"
	rm -f "$(BOOTSTRAP_OUT)"/*.c "$(BOOTSTRAP_OUT)"/*.asm

	@echo "==> Clearing temporary compiler emission"
	rm -f "$(srcdir)/compiler/"*.c "$(srcdir)/compiler/"*.asm

	$(BOOT_FBC_TOOL_ENV) $(BOOT_FBC) $(BOOT_FBC_PREFIX_OPT) $(BOOTSTRAP_COMPILER_SRC) \
		-m fbc \
		-gen gcc \
		-target $(BOOT_EMIT_TARGET) \
		$(if $(BOOTSTRAP_ARCH),-arch $(BOOTSTRAP_ARCH)) \
		$(BOOTSTRAP_COMPAT_DEFINES) \
		-i $(rootdir)/inc \
		-e -r -v \
		$(BOOTFBCFLAGS)

	@echo "==> Collecting emitted sources"

	@if ls $(srcdir)/compiler/*.c >/dev/null 2>&1; then \
		mv $(srcdir)/compiler/*.c "$(BOOTSTRAP_OUT)/"; \
	fi

	@if ls $(srcdir)/compiler/*.asm >/dev/null 2>&1; then \
		mv $(srcdir)/compiler/*.asm "$(BOOTSTRAP_OUT)/"; \
	fi

	@if ! ls "$(BOOTSTRAP_OUT)"/* >/dev/null 2>&1; then \
		echo "ERROR: bootstrap emission produced no sources"; \
		exit 1; \
	fi

	@dos2unix "$(BOOTSTRAP_OUT)"/* >/dev/null 2>&1 || true

##############################################################################
# End bootstrap-emit.mk
##############################################################################

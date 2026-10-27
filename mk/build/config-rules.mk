##############################################################################
# Project: FreeBASIC build system
# File: config-rules.mk
# Purpose: Rebuild normal build artifacts when their effective commands change.
# Responsibilities: Compare configuration and commit it after successful output.
# This file intentionally does NOT select sources or install toolchains.
##############################################################################

# Metadata queries must remain usable without a build toolchain. Other goals
# only read configuration here; all writes belong to successful output recipes.
BUILD_CONFIG_TRACKING := 1
build_config_query := $(if $(strip $(MAKECMDGOALS)),$(if $(filter-out print-% help,$(MAKECMDGOALS)),,yes))
ifneq ($(build_config_query),yes)

##############################################################################
# Command fingerprints
##############################################################################

build_config_empty :=
build_config_space := $(build_config_empty) $(build_config_empty)
define build_config_newline


endef
build_config_quote = '$(subst ','"'"',$(1))'
# Native Windows Make cannot reliably pass quoted multiline shell arguments.
# Encode backslashes first so a real newline differs from the literal text \n.
build_config_encode = $(subst $(build_config_newline),\n,$(subst \,\\,$(1)))
build_config_hash = $(or $(subst $(build_config_space),:,$(strip $(shell printf '%s\n' $(call build_config_quote,$(call build_config_encode,$(1))) | cksum))),$(error Cannot fingerprint the effective build command))

# Test/package recipes do not affect product compilation. Changes to the
# remaining included Make rules are conservative compile dependencies, since
# an edited per-file command can change code without changing a source file.
# Generated .d files describe source dependencies, not build commands. Including
# them would change the signature after compilation and overflow shell limits.
BUILD_CONFIG_RULE_FILES := $(filter-out \
%.d \
$(mkpath)/tests/% $(mkpath)/tests.mk $(mkpath)/maketests.mk \
$(mkpath)/dist.mk $(mkpath)/inst_uninst.mk $(mkpath)/bootstrap/%, \
$(MAKEFILE_LIST))
BUILD_CONFIG_RULES := $(subst $(build_config_space),:,$(strip $(shell config_rules="$$(cksum $(foreach path,$(BUILD_CONFIG_RULE_FILES),$(call build_config_quote,$(path))))" && printf '%s\n' "$$config_rules" | cksum)))
ifeq ($(BUILD_CONFIG_RULES),)
$(error Cannot fingerprint build rules; check that cksum and the included Makefiles are readable)
endif

# One shell invocation reads the small tool set. Content checksums avoid stale
# objects after an executable or wrapper is replaced at the same path. Unlike
# version probes, this does not launch SDK tools or depend on their CLI syntax.
BUILD_CONFIG_TOOLS := $(strip $(shell sh "$(mkpath)/../build_scripts/fingerprint-tools.sh" \
$(foreach tool,CC CXX AS AR RANLIB LD BUILD_FBC BOOT_FBC CLANG DARWIN_CLANG LLC WINDRES DLLTOOL,$(call build_config_quote,$(tool)=$($(tool)))) || printf '%s\n' BUILD_CONFIG_TOOLS_ERROR))
ifeq ($(BUILD_CONFIG_TOOLS),)
$(error Cannot fingerprint build tools)
endif
ifneq ($(filter BUILD_CONFIG_TOOLS_ERROR,$(BUILD_CONFIG_TOOLS)),)
$(error Cannot read build tool fingerprints)
endif
build_config_tool = $(filter $(1):%,$(BUILD_CONFIG_TOOLS))

# cksum supplies both a checksum and byte length. These are change detectors,
# not security identities. Preserve whitespace and shell metacharacters in
# flag values by quoting the complete value as data before hashing it.
define BUILD_CONFIG_C_INPUT
rules=$(BUILD_CONFIG_RULES)
target=$(TARGET_OS)/$(TARGET_ARCH)/$(FBTARGET)/$(THREAD_MODEL)
cc=$(RUN_CC)
tools=$(call build_config_tool,CC) $(call build_config_tool,AS) $(call build_config_tool,CLANG) $(call build_config_tool,DARWIN_CLANG)
cpp=$(CPPFLAGS)
cflags=$(ALLCFLAGS)
darwin_cc=$(RUN_DARWIN_CLANG)
darwin_flags=$(DARWIN_BLOCKS_CFLAGS)
endef
BUILD_CONFIG_C := $(call build_config_hash,$(BUILD_CONFIG_C_INPUT))

define BUILD_CONFIG_CXX_INPUT
rules=$(BUILD_CONFIG_RULES)
cxx=$(RUN_CXX)
tool=$(call build_config_tool,CXX)
cpp=$(CPPFLAGS)
cxxflags=$(ALLCXXFLAGS)
endef
BUILD_CONFIG_CXX := $(if $(USE_RUNTIME_CXX),$(call build_config_hash,$(BUILD_CONFIG_CXX_INPUT)),c-only)

define BUILD_CONFIG_FB_INPUT
rules=$(BUILD_CONFIG_RULES)
build_target=$(TARGET_OS)/$(TARGET_ARCH)/$(FBC_TARGET)/$(FBTARGET)
environment=$(FBC_TOOL_ENV)
compiler=$(BUILD_FBC)
tools=$(call build_config_tool,BUILD_FBC) $(call build_config_tool,CC) $(call build_config_tool,AS) $(call build_config_tool,LD) $(call build_config_tool,LLC) $(call build_config_tool,WINDRES) $(call build_config_tool,DLLTOOL)
target=$(BUILD_FBC_TARGET_OPT)
prefix=$(BUILD_FBC_BUILDPREFIX_OPT)
compatibility=$(BUILD_FBC_COMPAT_DEFINES)
build_flags=$(BUILD_FBCFLAGS)
source_prefix=$(FBC_PREFIX_OPT)
include_flags=$(FBC_INCLUDE_FLAGS)
endef
BUILD_CONFIG_FB := $(call build_config_hash,$(BUILD_CONFIG_FB_INPUT))
BUILD_CONFIG_COMPILER := $(call build_config_hash,$(BUILD_CONFIG_FB) $(ALLFBCFLAGS))
BUILD_CONFIG_COMPILER_JS := $(call build_config_hash,$(BUILD_CONFIG_COMPILER) $(FBC_JS_DEFINES))
BUILD_CONFIG_COMPILER_ANDROID := $(call build_config_hash,$(BUILD_CONFIG_COMPILER) $(FBC_ANDROID_DEFINES))
BUILD_CONFIG_COMPILER_WII := $(call build_config_hash,$(BUILD_CONFIG_COMPILER) $(FBC_WII_DEFINES))
BUILD_CONFIG_FBRT := $(call build_config_hash,$(BUILD_CONFIG_FB) $(ALLFBRTCFLAGS) $(DOS_THREAD_FBCFLAGS) $(FBC_INCLUDE_FLAGS))
BUILD_CONFIG_RTL := $(call build_config_hash,$(BUILD_CONFIG_C) $(RTLIB_GOSUB_CFLAGS)$(if $(RTLIB_ERROR_CFLAGS), errors=$(RTLIB_ERROR_CFLAGS)))
BUILD_CONFIG_GFX := $(call build_config_hash,$(BUILD_CONFIG_C) $(BUILD_CONFIG_CXX) $(GFXLIB2_ARCH_CFLAGS) $(GFXLIB2_SIMD_CFLAGS) $(GFXLIB2_OBJECT_CFLAGS))
BUILD_CONFIG_GFX3 := $(call build_config_hash,$(BUILD_CONFIG_C) $(BUILD_CONFIG_CXX) $(GFX3_LINUX_DRM_CFLAGS))
BUILD_CONFIG_SFX := $(call build_config_hash,$(BUILD_CONFIG_C) $(BUILD_CONFIG_CXX) $(SFXLIB_ARCH_CFLAGS) $(SFXLIB_NEON_CFLAGS) $(SFXLIB_OBJECT_CFLAGS))
BUILD_CONFIG_PIC := $(call build_config_hash,$(PIC_CFLAGS))
BUILD_CONFIG_MT := $(call build_config_hash,$(MT_CFLAGS))
BUILD_CONFIG_MTPIC := $(call build_config_hash,$(MTPIC_CFLAGS))
BUILD_CONFIG_AR := $(call build_config_hash,$(AR) $(ARFLAGS) $(RANLIB) $(call build_config_tool,AR) $(call build_config_tool,RANLIB))
BUILD_CONFIG_LINK := $(call build_config_hash,$(RUN_CC) $(call build_config_tool,CC) $(call build_config_tool,LD) $(ALLLDFLAGS) $(FBC_PIE_LDFLAGS) $(THREAD_FLAGS) $(TERM_LIB) $(COMPILER_PLATFORM_LIBS) $(COMPILER_RT0) $(COMPILER_RTL) $(BUILD_CONFIG_RTL) $(BUILD_CONFIG_AR) $(if $(filter YesPlease,$(ENABLE_PIE)),$(BUILD_CONFIG_PIC)))

##############################################################################
# Successful-output records
##############################################################################

# Each artifact owns one two-field record: compile signature, publish signature.
# Commit only after all inputs and the final link/archive have succeeded. A
# failed build leaves its old record, forcing retries independently of coarse
# filesystem timestamps. Separate variant records prevent a native build from
# marking old MT/PIC objects as configured without ever rebuilding them.
# Mark a compile attempt before changing any object. If that attempt fails,
# reverting to the previous flags must also rebuild its partially changed
# objects, even though the last successful record already names those flags.
.PHONY: build-config-force
build-config-force:

build_config_previous = $(if $(wildcard $(1)),$(strip $(file <$(1))))

define declare_build_config
ifneq ($(if $(wildcard $(5).pending),unfinished,$(3)),$(word 1,$(call build_config_previous,$(5))))
.PHONY: $(5).prepare
$(5).prepare:
	@mkdir -p "$(dir $(5))"
	@touch "$(5).pending"
$(if $(strip $(2)),$(2): $(5).prepare)
$(1): $(5).prepare
endif
ifneq ($(4),$(word 2,$(call build_config_previous,$(5))))
$(1): build-config-force
endif
$(1): private BUILD_CONFIG_RECORD := $(5)
$(1): private BUILD_CONFIG_SIGNATURE := $(3) $(4)
endef

define publish_build_config
$(if $(strip $(BUILD_CONFIG_RECORD)),@set -e; \
mkdir -p "$(dir $(BUILD_CONFIG_RECORD))"; \
trap 'rm -f "$(BUILD_CONFIG_RECORD).tmp"' 0; \
printf '%s\n' "$(BUILD_CONFIG_SIGNATURE)" >"$(BUILD_CONFIG_RECORD).tmp"; \
mv -f "$(BUILD_CONFIG_RECORD).tmp" "$(BUILD_CONFIG_RECORD)"; \
rm -f "$(BUILD_CONFIG_RECORD).pending")
endef

##############################################################################
# Compiler variants
##############################################################################

$(if $(FBC_EXE),$(eval $(call declare_build_config,$(FBC_EXE),$(FBC_OBJS),$(BUILD_CONFIG_COMPILER),$(BUILD_CONFIG_LINK),$(fbcobjdir)/.build-config)))
$(if $(FBC_JS_EXE),$(eval $(call declare_build_config,$(FBC_JS_EXE),$(FBC_JS_OBJS),$(BUILD_CONFIG_COMPILER_JS),$(BUILD_CONFIG_LINK),$(fbcjsobjdir)/.build-config)))
$(if $(FBC_ANDROID_EXE),$(eval $(call declare_build_config,$(FBC_ANDROID_EXE),$(FBC_ANDROID_OBJS),$(BUILD_CONFIG_COMPILER_ANDROID),$(BUILD_CONFIG_LINK),$(fbcandroidobjdir)/.build-config)))
$(if $(FBC_WII_EXE),$(eval $(call declare_build_config,$(FBC_WII_EXE),$(FBC_WII_OBJS),$(BUILD_CONFIG_COMPILER_WII),$(BUILD_CONFIG_LINK),$(fbcwiiobjdir)/.build-config)))

##############################################################################
# Compiler built from emitted bootstrap sources
##############################################################################

ifneq ($(strip $(BOOTSTRAP_FBC)),)
BUILD_CONFIG_BOOT_RULES := $(subst $(build_config_space),:,$(strip $(shell cksum $(foreach path,$(filter $(mkpath)/bootstrap/%,$(MAKEFILE_LIST)),$(call build_config_quote,$(path))) | cksum)))
BUILD_CONFIG_EMIT := $(call build_config_hash,$(BUILD_CONFIG_RULES) $(BUILD_CONFIG_BOOT_RULES) $(BOOT_FBC) $(call build_config_tool,BOOT_FBC) $(BOOT_FBC_TOOL_ENV) $(BOOT_EMIT_TARGET) $(BOOT_FBC_TARGET) $(BOOTSTRAP_ARCH) $(BOOTFBCGEN) $(BOOTFBCFLAGS) $(BOOTSTRAP_COMPAT_DEFINES))
BUILD_CONFIG_BOOT_MEMBERS := $(subst $(build_config_space),:,$(strip $(shell find "$(BOOTSTRAP_PATH)" -maxdepth 1 -type f \( -name '*.c' -o -name '*.asm' \) -print 2>/dev/null | LC_ALL=C sort | cksum)))
BUILD_CONFIG_BOOT := $(call build_config_hash,$(BUILD_CONFIG_C) $(BUILD_CONFIG_BOOT_RULES) $(BUILD_CONFIG_BOOT_MEMBERS) $(BOOTSTRAP_CFLAGS) $(BOOTSTRAP_WARN_SUPPRESS) $(BOOTSTRAP_ASM_SYNTAX_CFLAGS) $(PIC_CFLAGS) $(AS) $(ASFLAGS) $(call build_config_tool,AS) $(if $(wildcard $(BOOTSTRAP_PATH)/.emit-config),$(file <$(BOOTSTRAP_PATH)/.emit-config)))
BUILD_CONFIG_BOOT_LINK := $(call build_config_hash,$(BUILD_CONFIG_LINK) $(BOOTSTRAP_EARLY_LIBS) $(BOOTSTRAP_RT0) $(BOOTSTRAP_RTL) $(BOOTSTRAP_TERM_LIB) $(BOOTSTRAP_PLATFORM_LIBS))
$(eval $(call declare_build_config,$(BOOTSTRAP_FBC),$(BOOTSTRAP_OBJ),$(BUILD_CONFIG_BOOT),$(BUILD_CONFIG_BOOT_LINK),$(BOOTSTRAP_OBJDIR)/.build-config))
endif

##############################################################################
# Runtime archives and standalone startup objects
##############################################################################

BUILD_CONFIG_LIBDIR := $(libdir)/.build-config
$(eval $(call declare_build_config,$(libdir)/libfb.a,$(RTLIB_OBJ),$(BUILD_CONFIG_RTL),$(BUILD_CONFIG_AR),$(BUILD_CONFIG_LIBDIR)/libfb.a))
$(eval $(call declare_build_config,$(libdir)/libfbpic.a,$(RTLIB_PIC_OBJ),$(BUILD_CONFIG_RTL)-$(BUILD_CONFIG_PIC),$(BUILD_CONFIG_AR),$(BUILD_CONFIG_LIBDIR)/libfbpic.a))
$(eval $(call declare_build_config,$(libdir)/libfbmt.a,$(RTLIB_MT_OBJ),$(BUILD_CONFIG_RTL)-$(BUILD_CONFIG_MT),$(BUILD_CONFIG_AR),$(BUILD_CONFIG_LIBDIR)/libfbmt.a))
$(eval $(call declare_build_config,$(libdir)/libfbmtpic.a,$(RTLIB_MT_PIC_OBJ),$(BUILD_CONFIG_RTL)-$(BUILD_CONFIG_MTPIC),$(BUILD_CONFIG_AR),$(BUILD_CONFIG_LIBDIR)/libfbmtpic.a))
$(eval $(call declare_build_config,$(libdir)/libfbrt.a,$(FBRT_OBJ),$(BUILD_CONFIG_FBRT),$(BUILD_CONFIG_AR),$(BUILD_CONFIG_LIBDIR)/libfbrt.a))
$(eval $(call declare_build_config,$(libdir)/libfbrtpic.a,$(FBRT_PIC_OBJ),$(BUILD_CONFIG_FBRT)-$(BUILD_CONFIG_PIC),$(BUILD_CONFIG_AR),$(BUILD_CONFIG_LIBDIR)/libfbrtpic.a))
$(eval $(call declare_build_config,$(libdir)/libfbrtmt.a,$(FBRT_MT_OBJ),$(BUILD_CONFIG_FBRT)-$(BUILD_CONFIG_MT),$(BUILD_CONFIG_AR),$(BUILD_CONFIG_LIBDIR)/libfbrtmt.a))
$(eval $(call declare_build_config,$(libdir)/libfbrtmtpic.a,$(FBRT_MT_PIC_OBJ),$(BUILD_CONFIG_FBRT)-$(BUILD_CONFIG_MTPIC),$(BUILD_CONFIG_AR),$(BUILD_CONFIG_LIBDIR)/libfbrtmtpic.a))
$(eval $(call declare_build_config,$(libdir)/libfbgfx.a,$(GFX_OBJ),$(BUILD_CONFIG_GFX),$(BUILD_CONFIG_AR),$(BUILD_CONFIG_LIBDIR)/libfbgfx.a))
$(eval $(call declare_build_config,$(libdir)/libfbgfxpic.a,$(GFX_PIC_OBJ),$(BUILD_CONFIG_GFX)-$(BUILD_CONFIG_PIC),$(BUILD_CONFIG_AR),$(BUILD_CONFIG_LIBDIR)/libfbgfxpic.a))
$(eval $(call declare_build_config,$(libdir)/libfbgfxmt.a,$(GFX_MT_OBJ),$(BUILD_CONFIG_GFX)-$(BUILD_CONFIG_MT),$(BUILD_CONFIG_AR),$(BUILD_CONFIG_LIBDIR)/libfbgfxmt.a))
$(eval $(call declare_build_config,$(libdir)/libfbgfxmtpic.a,$(GFX_MT_PIC_OBJ),$(BUILD_CONFIG_GFX)-$(BUILD_CONFIG_MTPIC),$(BUILD_CONFIG_AR),$(BUILD_CONFIG_LIBDIR)/libfbgfxmtpic.a))
$(eval $(call declare_build_config,$(libdir)/libfbgfx3.a,$(GFX3_OBJ),$(BUILD_CONFIG_GFX3),$(BUILD_CONFIG_AR),$(BUILD_CONFIG_LIBDIR)/libfbgfx3.a))
$(eval $(call declare_build_config,$(libdir)/libfbgfx3pic.a,$(GFX3_PIC_OBJ),$(BUILD_CONFIG_GFX3)-$(BUILD_CONFIG_PIC),$(BUILD_CONFIG_AR),$(BUILD_CONFIG_LIBDIR)/libfbgfx3pic.a))
$(eval $(call declare_build_config,$(libdir)/libfbgfx3mt.a,$(GFX3_MT_OBJ),$(BUILD_CONFIG_GFX3)-$(BUILD_CONFIG_MT),$(BUILD_CONFIG_AR),$(BUILD_CONFIG_LIBDIR)/libfbgfx3mt.a))
$(eval $(call declare_build_config,$(libdir)/libfbgfx3mtpic.a,$(GFX3_MT_PIC_OBJ),$(BUILD_CONFIG_GFX3)-$(BUILD_CONFIG_MTPIC),$(BUILD_CONFIG_AR),$(BUILD_CONFIG_LIBDIR)/libfbgfx3mtpic.a))
$(eval $(call declare_build_config,$(libdir)/libsfx.a,$(SFX_OBJ),$(BUILD_CONFIG_SFX),$(BUILD_CONFIG_AR),$(BUILD_CONFIG_LIBDIR)/libsfx.a))
$(eval $(call declare_build_config,$(libdir)/libsfxpic.a,$(SFX_PIC_OBJ),$(BUILD_CONFIG_SFX)-$(BUILD_CONFIG_PIC),$(BUILD_CONFIG_AR),$(BUILD_CONFIG_LIBDIR)/libsfxpic.a))
$(eval $(call declare_build_config,$(libdir)/libsfxmt.a,$(SFX_MT_OBJ),$(BUILD_CONFIG_SFX)-$(BUILD_CONFIG_MT),$(BUILD_CONFIG_AR),$(BUILD_CONFIG_LIBDIR)/libsfxmt.a))
$(eval $(call declare_build_config,$(libdir)/libsfxmtpic.a,$(SFX_MT_PIC_OBJ),$(BUILD_CONFIG_SFX)-$(BUILD_CONFIG_MTPIC),$(BUILD_CONFIG_AR),$(BUILD_CONFIG_LIBDIR)/libsfxmtpic.a))

ifeq ($(THREAD_MODEL),pdmlwp)
$(eval $(call declare_build_config,$(libdir)/libfblwp.a,$(DOS_PDMLWP_OBJ),$(BUILD_CONFIG_C),$(BUILD_CONFIG_AR),$(BUILD_CONFIG_LIBDIR)/libfblwp.a))
endif

$(foreach object,fbrt0.o fbrt1.o fbrt2.o,$(eval $(call declare_build_config,$(libdir)/$(object),,$(BUILD_CONFIG_RTL),startup,$(BUILD_CONFIG_LIBDIR)/$(object))))
$(foreach object,fbrt0pic.o fbrt1pic.o fbrt2pic.o,$(eval $(call declare_build_config,$(libdir)/$(object),,$(BUILD_CONFIG_RTL)-$(BUILD_CONFIG_PIC),startup,$(BUILD_CONFIG_LIBDIR)/$(object))))

endif

# end of config-rules.mk

##############################################################################
# compile-rules.mk
#
# Object compilation rules for FreeBASIC build system
#
# Consumes canonical object lists from source-graph.mk
##############################################################################

##############################################################################
# Source search paths
##############################################################################

VPATH := \
$(srcdir)/compiler \
$(RTLIB_DIRS) \
$(FBRT_DIRS) \
$(GFXLIB2_DIRS) \
$(GFXLIB3_DIRS) \
$(SFXLIB_DIRS)

.SUFFIXES:

# Ensure bootstrap/self-hosted fbc uses this source tree's bin/lib paths
# instead of any compiled-in installation prefix.
FBC_BUILD_ROOT := $(rootdir)
ifneq ($(filter MSYS% MINGW% CYGWIN%,$(shell uname -s 2>/dev/null)),)
FBC_BUILD_ROOT := $(shell cygpath -m "$(rootdir)")
endif
FBC_PREFIX_OPT := -prefix $(FBC_BUILD_ROOT)
TOOLCHAIN_BINDIR := $(call tool_bindir,$(CC))
FBC_ENV_TOOLCHAIN_BINDIR := $(TOOLCHAIN_BINDIR)
FBC_ENV_PATH := $$PATH
FBC_ENV_PATH_SEP := :
FBC_ENV_AS := $(AS)
FBC_ENV_AR := $(AR)
FBC_ENV_LD := $(LD)
FBC_ENV_CC := $(CC)
FBC_ENV_CLANG := $(CLANG)
FBC_ENV_LLC := $(LLC)
FBC_ENV_DLLTOOL := $(DLLTOOL)
FBC_ENV_WINDRES := $(WINDRES)
FBC_ENV_GORC := $(GORC)
FBC_ENV_EMAS := $(EMAS)
FBC_ENV_EMAR := $(EMAR)
FBC_ENV_EMLD := $(EMLD)
FBC_ENV_EMCC := $(EMCC)
FBC_ENV_CXBE := $(CXBE)
FBC_ENV_DXEGEN := $(DXEGEN)
FBC_ENV_ELF2DOL := $(ELF2DOL)
ifneq ($(filter MSYS% MINGW% CYGWIN%,$(shell uname -s 2>/dev/null)),)
#
# BUILD_FBC is often a native Windows executable even when make is running
# from MSYS2 or Cygwin.  fbc consults tool-specific environment variables such
# as GCC and AS directly, so convert those to absolute Windows-style paths.
# Keep PATH in the shell's native POSIX form because the shell tools expect
# that environment after fbc launches them.
#
fbc_msys_tool = $(strip $(if $(strip $(1)),$(shell tool='$(call tool_cmd,$(1))'; args='$(call tool_args,$(1))'; if command -v "$$tool" >/dev/null 2>&1; then tool="$$(cygpath -m "$$(command -v "$$tool")")"; else tool="$$(cygpath -m "$$tool" 2>/dev/null || printf '%s' "$$tool")"; fi; if [ -n "$$args" ]; then printf '%s %s' "$$tool" "$$args"; else printf '%s' "$$tool"; fi)))
FBC_ENV_AS := $(call fbc_msys_tool,$(AS))
FBC_ENV_AR := $(call fbc_msys_tool,$(AR))
FBC_ENV_LD := $(call fbc_msys_tool,$(LD))
FBC_ENV_CC := $(call fbc_msys_tool,$(CC))
FBC_ENV_CLANG := $(call fbc_msys_tool,$(CLANG))
FBC_ENV_LLC := $(call fbc_msys_tool,$(LLC))
FBC_ENV_DLLTOOL := $(call fbc_msys_tool,$(DLLTOOL))
FBC_ENV_WINDRES := $(call fbc_msys_tool,$(WINDRES))
FBC_ENV_GORC := $(call fbc_msys_tool,$(GORC))
FBC_ENV_EMAS := $(call fbc_msys_tool,$(EMAS))
FBC_ENV_EMAR := $(call fbc_msys_tool,$(EMAR))
FBC_ENV_EMLD := $(call fbc_msys_tool,$(EMLD))
FBC_ENV_EMCC := $(call fbc_msys_tool,$(EMCC))
FBC_ENV_CXBE := $(call fbc_msys_tool,$(CXBE))
FBC_ENV_DXEGEN := $(call fbc_msys_tool,$(DXEGEN))
FBC_ENV_ELF2DOL := $(call fbc_msys_tool,$(ELF2DOL))
endif
FBC_TOOL_PATH_ENV := $(if $(strip $(FBC_ENV_TOOLCHAIN_BINDIR)),$(FBC_ENV_TOOLCHAIN_BINDIR)$(FBC_ENV_PATH_SEP),)$(FBC_ENV_PATH)
TOOLCHAIN_PATH_ENV :=
ifneq ($(strip $(TOOLCHAIN_BINDIR)),)
# Quote the runtime PATH separately. Some macOS applications install PATH
# entries containing spaces, and an unquoted $$PATH makes env treat the split
# path fragment as the command to execute.
TOOLCHAIN_PATH_ENV := env PATH="$(TOOLCHAIN_BINDIR):$$PATH"
endif
FBC_TOOL_ENV := env \
	$(TOOLCHAIN_FBC_ENV) \
	PATH="$(FBC_TOOL_PATH_ENV)" \
	AS='$(FBC_ENV_AS)' \
	AR='$(FBC_ENV_AR)' \
	LD='$(FBC_ENV_LD)' \
	GCC='$(FBC_ENV_CC)' \
	CLANG='$(FBC_ENV_CLANG)' \
	LLC='$(FBC_ENV_LLC)' \
	DLLTOOL='$(FBC_ENV_DLLTOOL)' \
	WINDRES='$(FBC_ENV_WINDRES)' \
	GORC='$(FBC_ENV_GORC)' \
	EMAS='$(FBC_ENV_EMAS)' \
	EMAR='$(FBC_ENV_EMAR)' \
	EMLD='$(FBC_ENV_EMLD)' \
	EMCC='$(FBC_ENV_EMCC)' \
	CXBE='$(FBC_ENV_CXBE)' \
	DXEGEN='$(FBC_ENV_DXEGEN)' \
	ELF2DOL='$(FBC_ENV_ELF2DOL)'
RUN_CC := $(TOOLCHAIN_PATH_ENV) $(CC)
RUN_CXX := $(TOOLCHAIN_PATH_ENV) $(CXX)
DARWIN_CLANG ?= clang
ifeq ($(TARGET_OS),darwin)
DARWIN_CLANG := $(strip $(shell xcrun --find clang 2>/dev/null || command -v clang 2>/dev/null || echo clang))
endif
RUN_DARWIN_CLANG := $(TOOLCHAIN_PATH_ENV) $(DARWIN_CLANG)
DARWIN_SDKROOT :=
ifeq ($(TARGET_OS),darwin)
DARWIN_SDKROOT := $(strip $(shell xcrun --show-sdk-path 2>/dev/null))
endif
DARWIN_BLOCKS_CFLAGS := -fblocks
ifneq ($(strip $(DARWIN_SDKROOT)),)
DARWIN_BLOCKS_CFLAGS += -isysroot $(DARWIN_SDKROOT)
endif
BUILD_FBC_TARGET_OPT :=
BUILD_FBC_BUILDPREFIX_OPT :=
BUILD_FBC_COMPAT_DEFINES :=
BUILD_FBC_COMPAT_TARGET := $(TARGET_ARCH) $(BUILD_FBC_TARGET)

ifneq ($(strip $(BUILD_FBC_TARGET)),)
BUILD_FBC_TARGET_OPT := -target $(BUILD_FBC_TARGET)
endif

ifneq ($(strip $(BUILD_FBC_BUILDPREFIX)),)
BUILD_FBC_BUILDPREFIX_OPT := -buildprefix $(BUILD_FBC_BUILDPREFIX)
endif

ifneq ($(filter aarch64 linux-aarch64,$(BUILD_FBC_COMPAT_TARGET)),)
BUILD_FBC_COMPAT_DEFINES += -d __FB_AARCH64__
endif
ifneq ($(filter riscv32 linux-riscv32,$(BUILD_FBC_COMPAT_TARGET)),)
BUILD_FBC_COMPAT_DEFINES += -d __FB_RISCV32__
endif
ifneq ($(filter riscv64 linux-riscv64,$(BUILD_FBC_COMPAT_TARGET)),)
BUILD_FBC_COMPAT_DEFINES += -d __FB_RISCV64__
endif
ifneq ($(filter s390x linux-s390x,$(BUILD_FBC_COMPAT_TARGET)),)
BUILD_FBC_COMPAT_DEFINES += -d __FB_S390X__
endif
ifneq ($(filter loongarch64 linux-loongarch64,$(BUILD_FBC_COMPAT_TARGET)),)
BUILD_FBC_COMPAT_DEFINES += -d __FB_LOONGARCH64__
endif

BUILD_FBCFLAGS ?=

$(fbcobjdir)/%.o: $(srcdir)/compiler/%.bas $(FBC_BI) | $(fbcobjdir)
	@mkdir -p "$(dir $@)"
	$(FBC_TOOL_ENV) $(BUILD_FBC) $(BUILD_FBC_TARGET_OPT) $(BUILD_FBC_BUILDPREFIX_OPT) $(BUILD_FBC_COMPAT_DEFINES) $(BUILD_FBCFLAGS) $(FBC_PREFIX_OPT) $(ALLFBCFLAGS) -i $(rootdir)/inc -c $< -o $@

FBC_JS_DEFINES := \
	-d BUILD_FB_DEFAULT_TARGET=FB_COMPTARGET_JS \
	-d BUILD_FB_DEFAULT_CPUTYPE=FB_CPUTYPE_ASMJS \
	-d ENABLE_SUFFIX=\"-js\"

$(fbcjsobjdir)/%.o: $(srcdir)/compiler/%.bas $(FBC_BI) | $(fbcjsobjdir)
	@mkdir -p "$(dir $@)"
	$(FBC_TOOL_ENV) $(BUILD_FBC) $(BUILD_FBC_TARGET_OPT) $(BUILD_FBC_BUILDPREFIX_OPT) $(BUILD_FBC_COMPAT_DEFINES) $(BUILD_FBCFLAGS) $(FBC_PREFIX_OPT) $(ALLFBCFLAGS) $(FBC_JS_DEFINES) -i $(rootdir)/inc -c $< -o $@

FBC_ANDROID_DEFINES := \
	-d BUILD_FB_DEFAULT_TARGET=FB_COMPTARGET_ANDROID \
	-d BUILD_FB_DEFAULT_CPUTYPE=FB_CPUTYPE_AARCH64 \
	-d ENABLE_SUFFIX=\"-android\"

$(fbcandroidobjdir)/%.o: $(srcdir)/compiler/%.bas $(FBC_BI) | $(fbcandroidobjdir)
	@mkdir -p "$(dir $@)"
	$(FBC_TOOL_ENV) $(BUILD_FBC) $(BUILD_FBC_TARGET_OPT) $(BUILD_FBC_BUILDPREFIX_OPT) $(BUILD_FBC_COMPAT_DEFINES) $(BUILD_FBCFLAGS) $(FBC_PREFIX_OPT) $(ALLFBCFLAGS) $(FBC_ANDROID_DEFINES) -i $(rootdir)/inc -c $< -o $@

FBC_WII_DEFINES := \
	-d BUILD_FB_DEFAULT_TARGET=FB_COMPTARGET_WII \
	-d BUILD_FB_DEFAULT_CPUTYPE=FB_CPUTYPE_PPC \
	-d ENABLE_SUFFIX=\"-wii\"

$(fbcwiiobjdir)/%.o: $(srcdir)/compiler/%.bas $(FBC_BI) | $(fbcwiiobjdir)
	@mkdir -p "$(dir $@)"
	$(FBC_TOOL_ENV) $(BUILD_FBC) $(BUILD_FBC_TARGET_OPT) $(BUILD_FBC_BUILDPREFIX_OPT) $(BUILD_FBC_COMPAT_DEFINES) $(BUILD_FBCFLAGS) $(FBC_PREFIX_OPT) $(ALLFBCFLAGS) $(FBC_WII_DEFINES) -i $(rootdir)/inc -c $< -o $@

##############################################################################
# rtlib (C runtime)
##############################################################################

RTLIB_GOSUB_CFLAGS :=
ifeq ($(TARGET_OS),win32)
  ifeq ($(TARGET_ARCH),x86_64)
    RTLIB_GOSUB_CFLAGS := -funwind-tables
  endif
endif

$(libfbobjdir)/%.o: $(srcdir)/rtlib/%.c $(LIBFB_H) | $(libfbobjdir)
	@mkdir -p "$(dir $@)"
	$(RUN_CC) $(CPPFLAGS) $(ALLCFLAGS) -MMD -MP -c $< -o $@

$(libfbpicobjdir)/%.o: $(srcdir)/rtlib/%.c $(LIBFB_H) | $(libfbpicobjdir)
	@mkdir -p "$(dir $@)"
	$(RUN_CC) $(CPPFLAGS) $(ALLCFLAGS) $(PIC_CFLAGS) -MMD -MP -c $< -o $@

$(libfbmtobjdir)/%.o: $(srcdir)/rtlib/%.c $(LIBFB_H) | $(libfbmtobjdir)
	@mkdir -p "$(dir $@)"
	$(RUN_CC) $(CPPFLAGS) $(ALLCFLAGS) $(MT_CFLAGS) -MMD -MP -c $< -o $@

$(libfbmtpicobjdir)/%.o: $(srcdir)/rtlib/%.c $(LIBFB_H) | $(libfbmtpicobjdir)
	@mkdir -p "$(dir $@)"
	$(RUN_CC) $(CPPFLAGS) $(ALLCFLAGS) $(MTPIC_CFLAGS) -MMD -MP -c $< -o $@

$(libfbobjdir)/gosub.o: $(srcdir)/rtlib/gosub.c $(LIBFB_H) | $(libfbobjdir)
	@mkdir -p "$(dir $@)"
	$(RUN_CC) $(CPPFLAGS) $(ALLCFLAGS) $(RTLIB_GOSUB_CFLAGS) -MMD -MP -c $< -o $@

$(libfbpicobjdir)/gosub.o: $(srcdir)/rtlib/gosub.c $(LIBFB_H) | $(libfbpicobjdir)
	@mkdir -p "$(dir $@)"
	$(RUN_CC) $(CPPFLAGS) $(ALLCFLAGS) $(PIC_CFLAGS) $(RTLIB_GOSUB_CFLAGS) -MMD -MP -c $< -o $@

$(libfbmtobjdir)/gosub.o: $(srcdir)/rtlib/gosub.c $(LIBFB_H) | $(libfbmtobjdir)
	@mkdir -p "$(dir $@)"
	$(RUN_CC) $(CPPFLAGS) $(ALLCFLAGS) $(MT_CFLAGS) $(RTLIB_GOSUB_CFLAGS) -MMD -MP -c $< -o $@

$(libfbmtpicobjdir)/gosub.o: $(srcdir)/rtlib/gosub.c $(LIBFB_H) | $(libfbmtpicobjdir)
	@mkdir -p "$(dir $@)"
	$(RUN_CC) $(CPPFLAGS) $(ALLCFLAGS) $(MTPIC_CFLAGS) $(RTLIB_GOSUB_CFLAGS) -MMD -MP -c $< -o $@

$(libfbobjdir)/%.o: $(srcdir)/rtlib/%.s $(LIBFB_H) | $(libfbobjdir)
	@mkdir -p "$(dir $@)"
	$(RUN_CC) $(CPPFLAGS) $(ALLCFLAGS) -x assembler-with-cpp -MMD -MP -c $< -o $@

$(libfbpicobjdir)/%.o: $(srcdir)/rtlib/%.s $(LIBFB_H) | $(libfbpicobjdir)
	@mkdir -p "$(dir $@)"
	$(RUN_CC) $(CPPFLAGS) $(ALLCFLAGS) $(PIC_CFLAGS) -x assembler-with-cpp -MMD -MP -c $< -o $@

$(libfbmtobjdir)/%.o: $(srcdir)/rtlib/%.s $(LIBFB_H) | $(libfbmtobjdir)
	@mkdir -p "$(dir $@)"
	$(RUN_CC) $(CPPFLAGS) $(ALLCFLAGS) $(MT_CFLAGS) -x assembler-with-cpp -MMD -MP -c $< -o $@

$(libfbmtpicobjdir)/%.o: $(srcdir)/rtlib/%.s $(LIBFB_H) | $(libfbmtpicobjdir)
	@mkdir -p "$(dir $@)"
	$(RUN_CC) $(CPPFLAGS) $(ALLCFLAGS) $(MTPIC_CFLAGS) -x assembler-with-cpp -MMD -MP -c $< -o $@

##############################################################################
# Static runtime startup
##############################################################################

$(libdir)/fbrt0.o: $(srcdir)/rtlib/static/fbrt0.c $(LIBFB_H) | $(libdir)
	@mkdir -p "$(dir $@)"
	$(RUN_CC) $(CPPFLAGS) $(ALLCFLAGS) -c $< -o $@

$(libdir)/fbrt0pic.o: $(srcdir)/rtlib/static/fbrt0.c $(LIBFB_H) | $(libdir)
	@mkdir -p "$(dir $@)"
	$(RUN_CC) $(CPPFLAGS) $(ALLCFLAGS) $(PIC_CFLAGS) -c $< -o $@

$(libdir)/fbrt1.o: $(srcdir)/rtlib/static/fbrt1.c $(LIBFB_H) | $(libdir)
	@mkdir -p "$(dir $@)"
	$(RUN_CC) $(CPPFLAGS) $(ALLCFLAGS) -c $< -o $@

$(libdir)/fbrt1pic.o: $(srcdir)/rtlib/static/fbrt1.c $(LIBFB_H) | $(libdir)
	@mkdir -p "$(dir $@)"
	$(RUN_CC) $(CPPFLAGS) $(ALLCFLAGS) $(PIC_CFLAGS) -c $< -o $@

$(libdir)/fbrt2.o: $(srcdir)/rtlib/static/fbrt2.c $(LIBFB_H) | $(libdir)
	@mkdir -p "$(dir $@)"
	$(RUN_CC) $(CPPFLAGS) $(ALLCFLAGS) -c $< -o $@

$(libdir)/fbrt2pic.o: $(srcdir)/rtlib/static/fbrt2.c $(LIBFB_H) | $(libdir)
	@mkdir -p "$(dir $@)"
	$(RUN_CC) $(CPPFLAGS) $(ALLCFLAGS) $(PIC_CFLAGS) -c $< -o $@

##############################################################################
# fbrt (FreeBASIC runtime layer)
##############################################################################

$(libfbrtobjdir)/%.o: $(srcdir)/fbrt/%.bas $(LIBFBRT_BI) | $(libfbrtobjdir)
	@mkdir -p "$(dir $@)"
	$(FBC_TOOL_ENV) $(BUILD_FBC) $(BUILD_FBC_TARGET_OPT) $(BUILD_FBC_BUILDPREFIX_OPT) $(BUILD_FBCFLAGS) $(FBC_PREFIX_OPT) $(ALLFBRTCFLAGS) -i $(rootdir)/inc -c $< -o $@

$(libfbrtpicobjdir)/%.o: $(srcdir)/fbrt/%.bas $(LIBFBRT_BI) | $(libfbrtpicobjdir)
	@mkdir -p "$(dir $@)"
	$(FBC_TOOL_ENV) $(BUILD_FBC) $(BUILD_FBC_TARGET_OPT) $(BUILD_FBC_BUILDPREFIX_OPT) $(BUILD_FBCFLAGS) $(FBC_PREFIX_OPT) $(ALLFBRTCFLAGS) -pic -i $(rootdir)/inc -c $< -o $@

$(libfbrtmtobjdir)/%.o: $(srcdir)/fbrt/%.bas $(LIBFBRT_BI) | $(libfbrtmtobjdir)
	@mkdir -p "$(dir $@)"
	$(FBC_TOOL_ENV) $(BUILD_FBC) $(BUILD_FBC_TARGET_OPT) $(BUILD_FBC_BUILDPREFIX_OPT) $(BUILD_FBCFLAGS) $(FBC_PREFIX_OPT) $(ALLFBRTCFLAGS) -mt -d ENABLE_MT -i $(rootdir)/inc -c $< -o $@

$(libfbrtmtpicobjdir)/%.o: $(srcdir)/fbrt/%.bas $(LIBFBRT_BI) | $(libfbrtmtpicobjdir)
	@mkdir -p "$(dir $@)"
	$(FBC_TOOL_ENV) $(BUILD_FBC) $(BUILD_FBC_TARGET_OPT) $(BUILD_FBC_BUILDPREFIX_OPT) $(BUILD_FBCFLAGS) $(FBC_PREFIX_OPT) $(ALLFBRTCFLAGS) -mt -pic -d ENABLE_MT -i $(rootdir)/inc -c $< -o $@

##############################################################################
# gfxlib2 (C sources)
##############################################################################

ifeq ($(TARGET_OS),darwin)

$(libfbgfxobjdir)/darwin/%.o: $(srcdir)/gfxlib2/darwin/%.c $(LIBFBGFX_H) | $(libfbgfxobjdir)
	@mkdir -p "$(dir $@)"
	$(RUN_DARWIN_CLANG) $(CPPFLAGS) $(ALLCFLAGS) $(DARWIN_BLOCKS_CFLAGS) -MMD -MP -c $< -o $@

$(libfbgfxpicobjdir)/darwin/%.o: $(srcdir)/gfxlib2/darwin/%.c $(LIBFBGFX_H) | $(libfbgfxpicobjdir)
	@mkdir -p "$(dir $@)"
	$(RUN_DARWIN_CLANG) $(CPPFLAGS) $(ALLCFLAGS) $(PIC_CFLAGS) $(DARWIN_BLOCKS_CFLAGS) -MMD -MP -c $< -o $@

$(libfbgfxmtobjdir)/darwin/%.o: $(srcdir)/gfxlib2/darwin/%.c $(LIBFBGFX_H) | $(libfbgfxmtobjdir)
	@mkdir -p "$(dir $@)"
	$(RUN_DARWIN_CLANG) $(CPPFLAGS) $(ALLCFLAGS) $(MT_CFLAGS) $(DARWIN_BLOCKS_CFLAGS) -MMD -MP -c $< -o $@

$(libfbgfxmtpicobjdir)/darwin/%.o: $(srcdir)/gfxlib2/darwin/%.c $(LIBFBGFX_H) | $(libfbgfxmtpicobjdir)
	@mkdir -p "$(dir $@)"
	$(RUN_DARWIN_CLANG) $(CPPFLAGS) $(ALLCFLAGS) $(MTPIC_CFLAGS) $(DARWIN_BLOCKS_CFLAGS) -MMD -MP -c $< -o $@

endif

$(libfbgfxobjdir)/%.o: $(srcdir)/gfxlib2/%.c $(LIBFBGFX_H) | $(libfbgfxobjdir)
	@mkdir -p "$(dir $@)"
	$(RUN_CC) $(CPPFLAGS) $(ALLCFLAGS) -MMD -MP -c $< -o $@

$(libfbgfxpicobjdir)/%.o: $(srcdir)/gfxlib2/%.c $(LIBFBGFX_H) | $(libfbgfxpicobjdir)
	@mkdir -p "$(dir $@)"
	$(RUN_CC) $(CPPFLAGS) $(ALLCFLAGS) $(PIC_CFLAGS) -MMD -MP -c $< -o $@

$(libfbgfxmtobjdir)/%.o: $(srcdir)/gfxlib2/%.c $(LIBFBGFX_H) | $(libfbgfxmtobjdir)
	@mkdir -p "$(dir $@)"
	$(RUN_CC) $(CPPFLAGS) $(ALLCFLAGS) $(MT_CFLAGS) -MMD -MP -c $< -o $@

$(libfbgfxmtpicobjdir)/%.o: $(srcdir)/gfxlib2/%.c $(LIBFBGFX_H) | $(libfbgfxmtpicobjdir)
	@mkdir -p "$(dir $@)"
	$(RUN_CC) $(CPPFLAGS) $(ALLCFLAGS) $(MTPIC_CFLAGS) -MMD -MP -c $< -o $@

##############################################################################
# gfxlib2 (C++ sources)
##############################################################################

ifneq ($(USE_RUNTIME_CXX),)

$(libfbgfxobjdir)/%.o: $(srcdir)/gfxlib2/%.cpp $(LIBFBGFX_H) | $(libfbgfxobjdir)
	@mkdir -p "$(dir $@)"
	$(RUN_CXX) $(ALLCXXFLAGS) -MMD -MP -c $< -o $@

$(libfbgfxpicobjdir)/%.o: $(srcdir)/gfxlib2/%.cpp $(LIBFBGFX_H) | $(libfbgfxpicobjdir)
	@mkdir -p "$(dir $@)"
	$(RUN_CXX) $(ALLCXXFLAGS) $(PIC_CFLAGS) -MMD -MP -c $< -o $@

$(libfbgfxmtobjdir)/%.o: $(srcdir)/gfxlib2/%.cpp $(LIBFBGFX_H) | $(libfbgfxmtobjdir)
	@mkdir -p "$(dir $@)"
	$(RUN_CXX) $(ALLCXXFLAGS) $(MT_CFLAGS) -MMD -MP -c $< -o $@

$(libfbgfxmtpicobjdir)/%.o: $(srcdir)/gfxlib2/%.cpp $(LIBFBGFX_H) | $(libfbgfxmtpicobjdir)
	@mkdir -p "$(dir $@)"
	$(RUN_CXX) $(ALLCXXFLAGS) $(MTPIC_CFLAGS) -MMD -MP -c $< -o $@

endif

$(libfbgfxobjdir)/%.o: $(srcdir)/gfxlib2/%.s $(LIBFBGFX_H) | $(libfbgfxobjdir)
	@mkdir -p "$(dir $@)"
	$(RUN_CC) $(ALLCFLAGS) -x assembler-with-cpp -MMD -MP -c $< -o $@

$(libfbgfxpicobjdir)/%.o: $(srcdir)/gfxlib2/%.s $(LIBFBGFX_H) | $(libfbgfxpicobjdir)
	@mkdir -p "$(dir $@)"
	$(RUN_CC) $(ALLCFLAGS) $(PIC_CFLAGS) -x assembler-with-cpp -MMD -MP -c $< -o $@

$(libfbgfxmtobjdir)/%.o: $(srcdir)/gfxlib2/%.s $(LIBFBGFX_H) | $(libfbgfxmtobjdir)
	@mkdir -p "$(dir $@)"
	$(RUN_CC) $(ALLCFLAGS) $(MT_CFLAGS) -x assembler-with-cpp -MMD -MP -c $< -o $@

$(libfbgfxmtpicobjdir)/%.o: $(srcdir)/gfxlib2/%.s $(LIBFBGFX_H) | $(libfbgfxmtpicobjdir)
	@mkdir -p "$(dir $@)"
	$(RUN_CC) $(ALLCFLAGS) $(MTPIC_CFLAGS) -x assembler-with-cpp -MMD -MP -c $< -o $@

##############################################################################
# gfxlib3 common core (C sources)
##############################################################################

$(libfbgfx3objdir)/%.o: $(srcdir)/gfxlib3/%.c $(LIBFBGFX3_H) | $(libfbgfx3objdir)
	@mkdir -p "$(dir $@)"
	$(RUN_CC) $(CPPFLAGS) $(ALLCFLAGS) -MMD -MP -c $< -o $@

$(libfbgfx3picobjdir)/%.o: $(srcdir)/gfxlib3/%.c $(LIBFBGFX3_H) | $(libfbgfx3picobjdir)
	@mkdir -p "$(dir $@)"
	$(RUN_CC) $(CPPFLAGS) $(ALLCFLAGS) $(PIC_CFLAGS) -MMD -MP -c $< -o $@

$(libfbgfx3mtobjdir)/%.o: $(srcdir)/gfxlib3/%.c $(LIBFBGFX3_H) | $(libfbgfx3mtobjdir)
	@mkdir -p "$(dir $@)"
	$(RUN_CC) $(CPPFLAGS) $(ALLCFLAGS) $(MT_CFLAGS) -MMD -MP -c $< -o $@

$(libfbgfx3mtpicobjdir)/%.o: $(srcdir)/gfxlib3/%.c $(LIBFBGFX3_H) | $(libfbgfx3mtpicobjdir)
	@mkdir -p "$(dir $@)"
	$(RUN_CC) $(CPPFLAGS) $(ALLCFLAGS) $(MTPIC_CFLAGS) -MMD -MP -c $< -o $@

##############################################################################
# sfxlib (C sources)
##############################################################################

ifeq ($(TARGET_OS),darwin)

$(libsfxobjdir)/darwin/%.o: $(srcdir)/sfxlib/darwin/%.c $(LIBSFX_H) | $(libsfxobjdir)
	@mkdir -p "$(dir $@)"
	$(RUN_DARWIN_CLANG) $(CPPFLAGS) $(ALLCFLAGS) $(DARWIN_BLOCKS_CFLAGS) -MMD -MP -c $< -o $@

$(libsfxpicobjdir)/darwin/%.o: $(srcdir)/sfxlib/darwin/%.c $(LIBSFX_H) | $(libsfxpicobjdir)
	@mkdir -p "$(dir $@)"
	$(RUN_DARWIN_CLANG) $(CPPFLAGS) $(ALLCFLAGS) $(PIC_CFLAGS) $(DARWIN_BLOCKS_CFLAGS) -MMD -MP -c $< -o $@

$(libsfxmtobjdir)/darwin/%.o: $(srcdir)/sfxlib/darwin/%.c $(LIBSFX_H) | $(libsfxmtobjdir)
	@mkdir -p "$(dir $@)"
	$(RUN_DARWIN_CLANG) $(CPPFLAGS) $(ALLCFLAGS) $(MT_CFLAGS) $(DARWIN_BLOCKS_CFLAGS) -MMD -MP -c $< -o $@

$(libsfxmtpicobjdir)/darwin/%.o: $(srcdir)/sfxlib/darwin/%.c $(LIBSFX_H) | $(libsfxmtpicobjdir)
	@mkdir -p "$(dir $@)"
	$(RUN_DARWIN_CLANG) $(CPPFLAGS) $(ALLCFLAGS) $(MTPIC_CFLAGS) $(DARWIN_BLOCKS_CFLAGS) -MMD -MP -c $< -o $@

endif

$(libsfxobjdir)/%.o: $(srcdir)/sfxlib/%.c $(LIBSFX_H) | $(libsfxobjdir)
	@mkdir -p "$(dir $@)"
	$(RUN_CC) $(CPPFLAGS) $(ALLCFLAGS) -MMD -MP -c $< -o $@

$(libsfxpicobjdir)/%.o: $(srcdir)/sfxlib/%.c $(LIBSFX_H) | $(libsfxpicobjdir)
	@mkdir -p "$(dir $@)"
	$(RUN_CC) $(CPPFLAGS) $(ALLCFLAGS) $(PIC_CFLAGS) -MMD -MP -c $< -o $@

$(libsfxmtobjdir)/%.o: $(srcdir)/sfxlib/%.c $(LIBSFX_H) | $(libsfxmtobjdir)
	@mkdir -p "$(dir $@)"
	$(RUN_CC) $(CPPFLAGS) $(ALLCFLAGS) $(MT_CFLAGS) -MMD -MP -c $< -o $@

$(libsfxmtpicobjdir)/%.o: $(srcdir)/sfxlib/%.c $(LIBSFX_H) | $(libsfxmtpicobjdir)
	@mkdir -p "$(dir $@)"
	$(RUN_CC) $(CPPFLAGS) $(ALLCFLAGS) $(MTPIC_CFLAGS) -MMD -MP -c $< -o $@

##############################################################################
# sfxlib (C++ sources)
##############################################################################

ifneq ($(USE_RUNTIME_CXX),)

$(libsfxobjdir)/%.o: $(srcdir)/sfxlib/%.cpp $(LIBSFX_H) | $(libsfxobjdir)
	@mkdir -p "$(dir $@)"
	$(RUN_CXX) $(ALLCXXFLAGS) -MMD -MP -c $< -o $@

$(libsfxpicobjdir)/%.o: $(srcdir)/sfxlib/%.cpp $(LIBSFX_H) | $(libsfxpicobjdir)
	@mkdir -p "$(dir $@)"
	$(RUN_CXX) $(ALLCXXFLAGS) $(PIC_CFLAGS) -MMD -MP -c $< -o $@

$(libsfxmtobjdir)/%.o: $(srcdir)/sfxlib/%.cpp $(LIBSFX_H) | $(libsfxmtobjdir)
	@mkdir -p "$(dir $@)"
	$(RUN_CXX) $(ALLCXXFLAGS) $(MT_CFLAGS) -MMD -MP -c $< -o $@

$(libsfxmtpicobjdir)/%.o: $(srcdir)/sfxlib/%.cpp $(LIBSFX_H) | $(libsfxmtpicobjdir)
	@mkdir -p "$(dir $@)"
	$(RUN_CXX) $(ALLCXXFLAGS) $(MTPIC_CFLAGS) -MMD -MP -c $< -o $@

endif

##############################################################################
# END compile-rules.mk
##############################################################################

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
$(SFXLIB_DIRS)

.SUFFIXES:

# Ensure bootstrap/self-hosted fbc uses this source tree's bin/lib paths
# instead of any compiled-in installation prefix.
FBC_BUILD_ROOT := $(rootdir)
ifneq ($(filter MSYS% MINGW%,$(shell uname -s 2>/dev/null)),)
FBC_BUILD_ROOT := $(shell cygpath -m "$(rootdir)")
endif
FBC_PREFIX_OPT := -prefix $(FBC_BUILD_ROOT)
TOOLCHAIN_BINDIR :=
ifneq ($(findstring /,$(CC))$(findstring \,$(CC)),)
TOOLCHAIN_BINDIR := $(patsubst %/,%,$(dir $(CC)))
endif
TOOLCHAIN_PATH_ENV :=
ifneq ($(strip $(TOOLCHAIN_BINDIR)),)
TOOLCHAIN_PATH_ENV := env PATH='$(TOOLCHAIN_BINDIR):'$$PATH
endif
FBC_TOOL_ENV := env \
	PATH='$(if $(strip $(TOOLCHAIN_BINDIR)),$(TOOLCHAIN_BINDIR):)'"$$PATH" \
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
RUN_CC := $(TOOLCHAIN_PATH_ENV) $(CC)
RUN_CXX := $(TOOLCHAIN_PATH_ENV) $(CXX)
DARWIN_CLANG ?= $(strip $(shell xcrun --find clang 2>/dev/null || command -v clang 2>/dev/null || echo clang))
RUN_DARWIN_CLANG := $(TOOLCHAIN_PATH_ENV) $(DARWIN_CLANG)
DARWIN_SDKROOT := $(strip $(shell xcrun --show-sdk-path 2>/dev/null))
DARWIN_BLOCKS_CFLAGS := -fblocks
ifneq ($(strip $(DARWIN_SDKROOT)),)
DARWIN_BLOCKS_CFLAGS += -isysroot $(DARWIN_SDKROOT)
endif
BUILD_FBC_TARGET_OPT :=
BUILD_FBC_BUILDPREFIX_OPT :=

ifneq ($(strip $(BUILD_FBC_TARGET)),)
BUILD_FBC_TARGET_OPT := -target $(BUILD_FBC_TARGET)
endif

ifneq ($(strip $(BUILD_FBC_BUILDPREFIX)),)
BUILD_FBC_BUILDPREFIX_OPT := -buildprefix $(BUILD_FBC_BUILDPREFIX)
endif

BUILD_FBCFLAGS ?=

$(fbcobjdir)/%.o: $(srcdir)/compiler/%.bas $(FBC_BI) | $(fbcobjdir)
	@mkdir -p "$(dir $@)"
	$(FBC_TOOL_ENV) $(BUILD_FBC) $(BUILD_FBC_TARGET_OPT) $(BUILD_FBC_BUILDPREFIX_OPT) $(BUILD_FBCFLAGS) $(FBC_PREFIX_OPT) $(ALLFBCFLAGS) -i $(rootdir)/inc -c $< -o $@

##############################################################################
# rtlib (C runtime)
##############################################################################

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

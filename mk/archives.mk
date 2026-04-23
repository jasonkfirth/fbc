##############################################################################
# archives.mk
#
# Final library + linker-script artifact graph
#
# Responsibilities:
#   - define runtime artifact sets
#   - generate linker scripts (runtime artifacts)
#
# Non-responsibilities:
#   - compilation rules          → build-rules.mk
#   - archive creation           → build-rules.mk
#   - directory infrastructure   → build-rules.mk
#   - runtime staging            → inst_uninst.mk / dist.mk
##############################################################################

##############################################################################
# Sanity: avoid accidental target collisions
##############################################################################

FBEXTRA_SCRIPT := fbextra.x

ifeq ($(strip $(FB_LDSCRIPT)),$(FBEXTRA_SCRIPT))
$(error FB_LDSCRIPT='$(FB_LDSCRIPT)' collides with $(FBEXTRA_SCRIPT))
endif

##############################################################################
# Runtime libraries (rtlib)
##############################################################################

RTL_LIBS := \
	$(libdir)/$(FB_LDSCRIPT) \
	$(libdir)/$(FBEXTRA_SCRIPT)

ifdef ENABLE_NONPIC
RTL_LIBS += \
	$(libdir)/fbrt0.o \
	$(libdir)/fbrt1.o \
	$(libdir)/fbrt2.o \
	$(libdir)/libfb.a
endif

ifdef ENABLE_PIC
RTL_LIBS += \
	$(libdir)/fbrt0pic.o \
	$(libdir)/fbrt1pic.o \
	$(libdir)/fbrt2pic.o \
	$(libdir)/libfbpic.a
endif

ifndef DISABLE_MT
ifdef ENABLE_NONPIC
RTL_LIBS += \
	$(libdir)/libfbmt.a
endif
ifdef ENABLE_PIC
RTL_LIBS += \
	$(libdir)/libfbmtpic.a
endif
endif

##############################################################################
# FreeBASIC runtime layer (fbrt)
##############################################################################

FBRTL_LIBS :=

ifneq ($(strip $(FBRT_OBJ)),)
ifdef ENABLE_NONPIC
FBRTL_LIBS += $(libdir)/libfbrt.a
endif

ifdef ENABLE_PIC
FBRTL_LIBS += $(libdir)/libfbrtpic.a
endif

ifndef DISABLE_MT
ifdef ENABLE_NONPIC
FBRTL_LIBS += $(libdir)/libfbrtmt.a
endif
ifdef ENABLE_PIC
FBRTL_LIBS += $(libdir)/libfbrtmtpic.a
endif
endif
endif

##############################################################################
# Graphics runtime (gfxlib2)
##############################################################################

GFX_LIBS :=

ifdef ENABLE_NONPIC
GFX_LIBS += $(libdir)/libfbgfx.a
endif

ifdef ENABLE_PIC
GFX_LIBS += $(libdir)/libfbgfxpic.a
endif

ifndef DISABLE_MT
ifdef ENABLE_NONPIC
GFX_LIBS += $(libdir)/libfbgfxmt.a
endif
ifdef ENABLE_PIC
GFX_LIBS += $(libdir)/libfbgfxmtpic.a
endif
endif

##############################################################################
# Sound runtime (sfxlib)
###############################################################################

SFX_LIBS :=

ifdef ENABLE_NONPIC
SFX_LIBS += $(libdir)/libsfx.a
endif

ifdef ENABLE_PIC
SFX_LIBS += $(libdir)/libsfxpic.a
endif

ifndef DISABLE_MT
ifdef ENABLE_NONPIC
SFX_LIBS += $(libdir)/libsfxmt.a
endif

ifdef ENABLE_PIC
SFX_LIBS += $(libdir)/libsfxmtpic.a
endif
endif

##############################################################################
# Primary runtime linker script
##############################################################################

UPSTREAM_LDSCRIPT := $(rootdir)/lib/$(FB_LDSCRIPT)

$(libdir)/$(FB_LDSCRIPT):
	@mkdir -p $(dir $@)
	@echo "Preparing linker script: $@"
	@if [ -f "$(UPSTREAM_LDSCRIPT)" ]; then \
		echo "Using upstream linker script"; \
		cp -f "$(UPSTREAM_LDSCRIPT)" "$@"; \
	else \
		echo "Synthesizing fallback linker script"; \
		printf "INPUT(libfb.a)\n" > "$@"; \
	fi

##############################################################################
# fbextra.x linker augmentation script
##############################################################################

UPSTREAM_FBEXTRA := $(rootdir)/lib/$(FBEXTRA_SCRIPT)

$(libdir)/$(FBEXTRA_SCRIPT):
	@mkdir -p $(dir $@)
	@echo "Preparing $(FBEXTRA_SCRIPT): $@"
	@if [ -f "$(UPSTREAM_FBEXTRA)" ]; then \
		echo "Using upstream $(FBEXTRA_SCRIPT)"; \
		cp -f "$(UPSTREAM_FBEXTRA)" "$@"; \
	else \
		echo "Synthesizing $(FBEXTRA_SCRIPT)"; \
		printf '%s\n' \
'/* FreeBASIC linker augmentation script */' \
'/* Discard compile-time metadata sections not needed in final binaries */' \
'SECTIONS' \
'{' \
'  /DISCARD/ :' \
'  {' \
'    *(.fbctinf)' \
'  }' \
'}' \
'INSERT AFTER .data;' \
		> "$@"; \
	fi

##############################################################################
# Export aggregate runtime target
##############################################################################

.PHONY: runtime-libs

runtime-libs: $(RTL_LIBS) $(FBRTL_LIBS) $(GFX_LIBS) $(SFX_LIBS)

##############################################################################
# END archives.mk
##############################################################################

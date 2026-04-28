##############################################################################
# build-dirs.mk
#
# Directory infrastructure
#
# Responsibilities:
#   - create all object and output directories required by the build
#
# Source of truth:
#   - directory variables defined in build-layout.mk
##############################################################################

##############################################################################
# Collect directories exported by build-layout.mk
##############################################################################

OBJDIRS := \
	$(fbcobjdir) \
	$(fbcjsobjdir) \
	$(libfbobjdir) \
	$(libfbpicobjdir) \
	$(libfbmtobjdir) \
	$(libfbmtpicobjdir) \
	$(libfbrtobjdir) \
	$(libfbrtpicobjdir) \
	$(libfbrtmtobjdir) \
	$(libfbrtmtpicobjdir) \
	$(libfbgfxobjdir) \
	$(libfbgfxpicobjdir) \
	$(libfbgfxmtobjdir) \
	$(libfbgfxmtpicobjdir) \
	$(libsfxobjdir) \
	$(libsfxpicobjdir) \
	$(libsfxmtobjdir) \
	$(libsfxmtpicobjdir)

OUTPUTDIRS := \
	$(libdir) \
	$(build_bindir)

BUILD_DIRS := $(OBJDIRS) $(OUTPUTDIRS)

##############################################################################
# Directory creation rule
##############################################################################

$(BUILD_DIRS):
	mkdir -p "$@"

##############################################################################
# END build-dirs.mk
##############################################################################

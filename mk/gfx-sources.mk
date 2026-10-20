##############################################################################
# FreeBASIC graphics libraries
# File: gfx-sources.mk
# Purpose: Define shared graphics subsystem directories.
# Responsibilities: Keep source discovery explicit and independent of obj/.
# This file intentionally does NOT contain target overrides or compiler flags.
##############################################################################

# Headers remain at each library root as the shared internal contracts. Native
# adapters live below drivers/ and override shared units by basename.
# Implementation directories are explicit so adding a subsystem is reviewable.
GFX2_SOURCE_GROUPS := core draw image input text pixels palette presentation data
GFX3_SOURCE_GROUPS := api compat core backend platform codec

GFX2_GENERIC_DIRS := $(addprefix $(srcdir)/gfxlib2/,$(GFX2_SOURCE_GROUPS))
GFX3_GENERIC_DIRS := $(addprefix $(srcdir)/gfxlib3/,$(GFX3_SOURCE_GROUPS))

# Native adapters and CPU kernels are separate from shared subsystem owners.
GFX2_DRIVER_DIR := $(srcdir)/gfxlib2/drivers
GFX3_DRIVER_DIR := $(srcdir)/gfxlib3/drivers

# end of gfx-sources.mk

# Project: FreeBASIC documentation tools
# File: GNUmakefile
# Purpose: Build the manual's compressed-help database generator.
# Responsibilities: Compile makefbhelp using shared host and compiler defaults.
# This file intentionally does NOT generate the manual itself.

include ../common.mk

MAKEFBHELP := makefbhelp$(EXEEXT)

################################################################################

$(MAKEFBHELP): makefbhelp.bas
	$(FBC) -exx -g $< -x $@

clean:
	-rm -f $(MAKEFBHELP)

.PHONY : clean

# end of GNUmakefile

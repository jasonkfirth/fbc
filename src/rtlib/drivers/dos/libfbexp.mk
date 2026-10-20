##############################################################################
# Project: FreeBASIC DOS runtime
# File: drivers/dos/libfbexp.mk
# Purpose: Rebuild the DOS dynamic-library export table.
# Responsibilities: Extract DXE exports and run the symbol-table generator.
# This file intentionally does NOT contain normal runtime archive recipes.
##############################################################################

# rebuild symb_reg.txt, needed by sys_dylib.c
# to build the export table for DOS dylib support
#
# expects standalone version
#
# running this makefile will touch sys_dylib.c
# which will cause libfb to be made next time
# fbc's top-level makefile is run.  Of course,
# that will cause libfb to be newer, again;
# which would trigger the build in this makefile
# if this makefile were to be run again.
#
# Currently, there is a cyclic dependency, between
# fbc's top-level makefile and this makefile that
# has not been solved out.  But that's OK.  Intent
# is that it would simply be used from a higher 
# level script once during build.
#
# From top-level fbc directory:
#    make TARGET_OS=dos
#    make -f src/rtlib/drivers/dos/libfbexp.mk
#    make TARGET_OS=dos
#
# Also, check for changes in 'symb_reg.txt' and commit
# changes to repository. 

FBC := fbc-dos.exe
DXE3GEN := dxe3gen
CP := cp
RM := rm -f
TOUCH := touch

SRCDIR := ./src/rtlib/drivers/dos/
LIBDIR := ./lib/dos/
BINDIR := ./bin/dos/

.PHONY: all
all: $(SRCDIR)symb_reg.txt                

$(SRCDIR)symb_reg.txt: $(LIBDIR)libfblst.txt $(BINDIR)maksymbr.exe
	$(BINDIR)maksymbr.exe -o $@ $<
	$(TOUCH) $(SRCDIR)sys_dylib.c

$(BINDIR)maksymbr.exe: $(SRCDIR)maksymbr.bas
	$(FBC) $< -exx -x $@ 

$(LIBDIR)libfblst.txt: $(LIBDIR)libfb.dxe 
	$(DXE3GEN) --show-exp $< > $@

$(LIBDIR)libfb.dxe: $(LIBDIR)libfb.a 
	$(DXE3GEN) -o $@ --whole-archive -U $<

.PHONY: clean
clean:
	$(RM) $(LIBDIR)libfb.dxe
	$(RM) $(LIBDIR)libfblst.txt
	$(RM) $(BINDIR)maksymbr.exe

# end of drivers/dos/libfbexp.mk

##############################################################################
# archive-rules.mk
#
# Final library archive creation rules
#
# Consumes object lists from source-graph.mk
##############################################################################

define DO_AR
	rm -f $@
	$(AR) $(ARFLAGS) $@ $^
	-$(RANLIB) $@
endef

##############################################################################
# Core runtime (rtlib)
##############################################################################

$(libdir)/libfb.a: $(RTLIB_OBJ) | $(libdir)
	$(call DO_AR)

$(libdir)/libfbpic.a: $(RTLIB_PIC_OBJ) | $(libdir)
	$(call DO_AR)

$(libdir)/libfbmt.a: $(RTLIB_MT_OBJ) | $(libdir)
	$(call DO_AR)

$(libdir)/libfbmtpic.a: $(RTLIB_MT_PIC_OBJ) | $(libdir)
	$(call DO_AR)

ifeq ($(THREAD_MODEL),pdmlwp)
DOS_PDMLWP_DIR := $(rootdir)/contrib/dos/pdmlwp
DOS_PDMLWP_OBJ := $(libfbmtobjdir)/pdmlwp/lwp.o $(libfbmtobjdir)/pdmlwp/lwpasm.o

# The imported scheduler locks regions bounded by source-order markers.
# Disable function/data reordering and LTO for these two objects only.
$(libfbmtobjdir)/pdmlwp/lwp.o: $(DOS_PDMLWP_DIR)/src/lwp.c $(DOS_PDMLWP_DIR)/include/lwp.h
	@mkdir -p "$(dir $@)"
	$(RUN_CC) -I$(DOS_PDMLWP_DIR)/include -std=gnu11 -O1 -march=i386 -mno-sse -mno-mmx -fno-lto -fno-toplevel-reorder -fno-reorder-functions -fno-strict-aliasing -fno-omit-frame-pointer -c $< -o $@

$(libfbmtobjdir)/pdmlwp/lwpasm.o: $(DOS_PDMLWP_DIR)/src/lwpasm.s
	@mkdir -p "$(dir $@)"
	$(RUN_CC) -x assembler-with-cpp -c $< -o $@

$(libdir)/libfbpdmlwp.a: $(DOS_PDMLWP_OBJ) | $(libdir)
	$(call DO_AR)
endif

##############################################################################
# FreeBASIC runtime layer (fbrt)
##############################################################################

$(libdir)/libfbrt.a: $(FBRT_OBJ) | $(libdir)
	$(call DO_AR)

$(libdir)/libfbrtpic.a: $(FBRT_PIC_OBJ) | $(libdir)
	$(call DO_AR)

$(libdir)/libfbrtmt.a: $(FBRT_MT_OBJ) | $(libdir)
	$(call DO_AR)

$(libdir)/libfbrtmtpic.a: $(FBRT_MT_PIC_OBJ) | $(libdir)
	$(call DO_AR)

##############################################################################
# Graphics runtime (gfxlib2)
##############################################################################

$(libdir)/libfbgfx.a: $(GFX_OBJ) | $(libdir)
	$(call DO_AR)

$(libdir)/libfbgfxpic.a: $(GFX_PIC_OBJ) | $(libdir)
	$(call DO_AR)

$(libdir)/libfbgfxmt.a: $(GFX_MT_OBJ) | $(libdir)
	$(call DO_AR)

$(libdir)/libfbgfxmtpic.a: $(GFX_MT_PIC_OBJ) | $(libdir)
	$(call DO_AR)

##############################################################################
# Experimental GPU graphics runtime (gfxlib3)
##############################################################################

$(libdir)/libfbgfx3.a: $(GFX3_OBJ) | $(libdir)
	$(call DO_AR)

$(libdir)/libfbgfx3pic.a: $(GFX3_PIC_OBJ) | $(libdir)
	$(call DO_AR)

$(libdir)/libfbgfx3mt.a: $(GFX3_MT_OBJ) | $(libdir)
	$(call DO_AR)

$(libdir)/libfbgfx3mtpic.a: $(GFX3_MT_PIC_OBJ) | $(libdir)
	$(call DO_AR)

##############################################################################
# Sound runtime (sfxlib)
##############################################################################

$(libdir)/libsfx.a: $(SFX_OBJ) | $(libdir)
	$(call DO_AR)

$(libdir)/libsfxpic.a: $(SFX_PIC_OBJ) | $(libdir)
	$(call DO_AR)

$(libdir)/libsfxmt.a: $(SFX_MT_OBJ) | $(libdir)
	$(call DO_AR)

$(libdir)/libsfxmtpic.a: $(SFX_MT_PIC_OBJ) | $(libdir)
	$(call DO_AR)

##############################################################################
# END archive-rules.mk
##############################################################################

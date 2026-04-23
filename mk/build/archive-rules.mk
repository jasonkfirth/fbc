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

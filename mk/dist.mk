################
# dist.mk
################

DISTROOT := dist
DISTDIR  := $(DISTROOT)/freebasic
DIST_BINDIR := $(prefixbindir)
DIST_INCDIR := $(prefixincdir)
DIST_RUNTIME_DIR := $(FBINSTALL_RUNTIME_DIR)

ifneq ($(filter win32 dos,$(TARGET_OS)),)
DIST_BINDIR := $(patsubst $(prefix)%,%,$(prefixbindir))
DIST_INCDIR := $(patsubst $(prefix)%,%,$(prefixincdir))
DIST_RUNTIME_DIR := $(patsubst $(prefix)%,%,$(FBINSTALL_RUNTIME_DIR))
endif

.PHONY: dist
dist: dist-clean dist-stage dist-package

.PHONY: dist-clean
dist-clean:
	rm -rf "$(DISTROOT)"

.PHONY: dist-stage
dist-stage: dist-bin dist-includes dist-runtime

.PHONY: dist-bin
dist-bin:
	mkdir -p "$(DISTDIR)$(DIST_BINDIR)"
	cp "$(FBC_EXE)" "$(DISTDIR)$(DIST_BINDIR)/fbc$(EXEEXT)"

.PHONY: dist-includes
dist-includes:
	mkdir -p "$(DISTDIR)$(DIST_INCDIR)"
	set -e; \
	for f in "$(rootdir)"/inc/*; do \
		[ -e "$$f" ] || continue; \
		if [ -f "$$f" ]; then \
			b=$$(basename "$$f"); \
			cp "$$f" "$(DISTDIR)$(DIST_INCDIR)/$$b"; \
		fi; \
	done

.PHONY: dist-runtime
dist-runtime:
	mkdir -p "$(DISTDIR)$(DIST_RUNTIME_DIR)"
	set -e; \
	for f in "$(libdir)"/*; do \
		[ -e "$$f" ] || continue; \
		if [ -f "$$f" ]; then \
			b=$$(basename "$$f"); \
			cp "$$f" "$(DISTDIR)$(DIST_RUNTIME_DIR)/$$b"; \
		fi; \
	done

.PHONY: dist-package
dist-package:
	tar -C "$(DISTROOT)" -cf freebasic-dist.tar freebasic

.PHONY: dist-zip
dist-zip:
	cd "$(DISTROOT)" && zip -r ../freebasic-dist.zip freebasic

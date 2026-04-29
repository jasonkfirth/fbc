##############################################################################
# inst_uninst.mk
#
# Install / uninstall rules for FreeBASIC
# Layout policy comes from layout.mk
##############################################################################

INSTALL_STAGE_BINDIR := $(prefixbindir)
INSTALL_STAGE_INCDIR := $(prefixincdir)
INSTALL_STAGE_LIBDIR := $(FBINSTALL_RUNTIME_DIR)

ifneq ($(strip $(DESTDIR)),)
ifneq ($(filter win32 dos,$(TARGET_OS)),)
INSTALL_STAGE_BINDIR := $(patsubst $(prefix)%,%,$(prefixbindir))
INSTALL_STAGE_INCDIR := $(patsubst $(prefix)%,%,$(prefixincdir))
INSTALL_STAGE_LIBDIR := $(patsubst $(prefix)%,%,$(FBINSTALL_RUNTIME_DIR))
endif
endif

INSTALL_BINDIR  := $(if $(strip $(DESTDIR)),$(DESTDIR)$(INSTALL_STAGE_BINDIR),$(prefixbindir))
INSTALL_INCDIR  := $(if $(strip $(DESTDIR)),$(DESTDIR)$(INSTALL_STAGE_INCDIR),$(prefixincdir))
INSTALL_LIBDIR  := $(if $(strip $(DESTDIR)),$(DESTDIR)$(INSTALL_STAGE_LIBDIR),$(FBINSTALL_RUNTIME_DIR))

INSTALL_MANIFEST := $(DESTDIR).install_manifest

INSTALL_FILE ?= install -m 644
INSTALL_EXE  ?= install -m 755
MKDIR_P      ?= mkdir -p

FBC_INSTALL_NAME ?= fbc$(EXEEXT)
FBC_JS_INSTALL_NAME ?= fbc-js$(EXEEXT)
FBC_ANDROID_INSTALL_NAME ?= fbc-android$(EXEEXT)
FBC_ANDROID_COMPILER_INSTALL_NAME ?= fbc-android-compiler$(EXEEXT)
FB_JS_NAME ?= freebasic-js
FB_JS_TARGET ?= js-asmjs
FB_JS_RUNTIME_ASSETS := fb_shell.html fb_rtlib.js termlib_min.js
FB_ANDROID_NAME ?= freebasic-android
FB_ANDROID_TARGET ?= android-aarch64

prefixjsincdir ?= $(prefix)/include/$(FB_JS_NAME)
prefixjsruntimedir ?= $(prefix)/$(libdirname)/$(FB_JS_NAME)
FBINSTALL_JS_RUNTIME_DIR := $(prefixjsruntimedir)/$(FB_JS_TARGET)
JS_BUILD_LIBDIR ?= $(rootdir)/$(libdirname)/freebasic/$(FB_JS_TARGET)

prefixandroidincdir ?= $(prefix)/include/$(FB_ANDROID_NAME)
prefixandroidrootdir ?= $(prefix)/$(libdirname)/$(FB_ANDROID_NAME)
prefixandroidsharedir ?= $(prefix)/share/$(FB_ANDROID_NAME)
prefixandroidbindir ?= $(prefixandroidrootdir)/bin
FBINSTALL_ANDROID_RUNTIME_DIR := $(prefixandroidrootdir)/$(FB_ANDROID_TARGET)
ANDROID_BUILD_LIBDIR ?= $(rootdir)/$(libdirname)/freebasic/$(FB_ANDROID_TARGET)
ANDROID_TOOLS_DIR ?= $(rootdir)/src/tools/android

INSTALL_STAGE_JS_INCDIR := $(prefixjsincdir)
INSTALL_STAGE_JS_LIBDIR := $(FBINSTALL_JS_RUNTIME_DIR)
INSTALL_STAGE_ANDROID_INCDIR := $(prefixandroidincdir)
INSTALL_STAGE_ANDROID_ROOTDIR := $(prefixandroidrootdir)
INSTALL_STAGE_ANDROID_BINDIR := $(prefixandroidbindir)
INSTALL_STAGE_ANDROID_LIBDIR := $(FBINSTALL_ANDROID_RUNTIME_DIR)
INSTALL_STAGE_ANDROID_SHAREDIR := $(prefixandroidsharedir)

ifneq ($(strip $(DESTDIR)),)
ifneq ($(filter win32 dos,$(TARGET_OS)),)
INSTALL_STAGE_JS_INCDIR := $(patsubst $(prefix)%,%,$(prefixjsincdir))
INSTALL_STAGE_JS_LIBDIR := $(patsubst $(prefix)%,%,$(FBINSTALL_JS_RUNTIME_DIR))
INSTALL_STAGE_ANDROID_INCDIR := $(patsubst $(prefix)%,%,$(prefixandroidincdir))
INSTALL_STAGE_ANDROID_ROOTDIR := $(patsubst $(prefix)%,%,$(prefixandroidrootdir))
INSTALL_STAGE_ANDROID_BINDIR := $(patsubst $(prefix)%,%,$(prefixandroidbindir))
INSTALL_STAGE_ANDROID_LIBDIR := $(patsubst $(prefix)%,%,$(FBINSTALL_ANDROID_RUNTIME_DIR))
INSTALL_STAGE_ANDROID_SHAREDIR := $(patsubst $(prefix)%,%,$(prefixandroidsharedir))
endif
endif

INSTALL_JS_INCDIR := $(if $(strip $(DESTDIR)),$(DESTDIR)$(INSTALL_STAGE_JS_INCDIR),$(prefixjsincdir))
INSTALL_JS_LIBDIR := $(if $(strip $(DESTDIR)),$(DESTDIR)$(INSTALL_STAGE_JS_LIBDIR),$(FBINSTALL_JS_RUNTIME_DIR))
INSTALL_ANDROID_INCDIR := $(if $(strip $(DESTDIR)),$(DESTDIR)$(INSTALL_STAGE_ANDROID_INCDIR),$(prefixandroidincdir))
INSTALL_ANDROID_ROOTDIR := $(if $(strip $(DESTDIR)),$(DESTDIR)$(INSTALL_STAGE_ANDROID_ROOTDIR),$(prefixandroidrootdir))
INSTALL_ANDROID_BINDIR := $(if $(strip $(DESTDIR)),$(DESTDIR)$(INSTALL_STAGE_ANDROID_BINDIR),$(prefixandroidbindir))
INSTALL_ANDROID_LIBDIR := $(if $(strip $(DESTDIR)),$(DESTDIR)$(INSTALL_STAGE_ANDROID_LIBDIR),$(FBINSTALL_ANDROID_RUNTIME_DIR))
INSTALL_ANDROID_SHAREDIR := $(if $(strip $(DESTDIR)),$(DESTDIR)$(INSTALL_STAGE_ANDROID_SHAREDIR),$(prefixandroidsharedir))

.PHONY: install install-bin install-includes install-runtime
install: install-bin install-includes install-runtime

.PHONY: install-bin
install-bin:
	mkdir -p "$(INSTALL_BINDIR)"
	install -m 755 "$(FBC_EXE)" "$(INSTALL_BINDIR)/$(FBC_INSTALL_NAME)"
	@echo "$(if $(strip $(DESTDIR)),$(INSTALL_STAGE_BINDIR),$(prefixbindir))/$(FBC_INSTALL_NAME)" >> "$(INSTALL_MANIFEST)"

.PHONY: install-includes
install-includes:
	$(MKDIR_P) "$(INSTALL_INCDIR)"
	cp -a "$(rootdir)/inc/." "$(INSTALL_INCDIR)/"
	@find "$(INSTALL_INCDIR)" -type f \
			| sed "s|^$(DESTDIR)||" \
				>> "$(INSTALL_MANIFEST)"

.PHONY: install-runtime
install-runtime:
	mkdir -p "$(INSTALL_LIBDIR)"
	set -e; \
	for f in "$(libdir)"/*; do \
		[ -e "$$f" ] || continue; \
		if [ -f "$$f" ]; then \
			b=$$(basename "$$f"); \
			install -m 644 "$$f" "$(INSTALL_LIBDIR)/$$b"; \
			echo "$(if $(strip $(DESTDIR)),$(INSTALL_STAGE_LIBDIR),$(FBINSTALL_RUNTIME_DIR))/$$b" >> "$(INSTALL_MANIFEST)"; \
		fi; \
	done

.PHONY: install-js install-js-bin install-js-includes install-js-runtime
install-js: install-js-bin install-js-includes install-js-runtime

.PHONY: install-js-bin
install-js-bin:
	mkdir -p "$(INSTALL_BINDIR)"
	install -m 755 "$(FBC_JS_EXE)" "$(INSTALL_BINDIR)/$(FBC_JS_INSTALL_NAME)"
	@echo "$(if $(strip $(DESTDIR)),$(INSTALL_STAGE_BINDIR),$(prefixbindir))/$(FBC_JS_INSTALL_NAME)" >> "$(INSTALL_MANIFEST)"

.PHONY: install-js-includes
install-js-includes:
	$(MKDIR_P) "$(INSTALL_JS_INCDIR)"
	cp -a "$(rootdir)/inc/." "$(INSTALL_JS_INCDIR)/"
	@find "$(INSTALL_JS_INCDIR)" -type f \
			| sed "s|^$(DESTDIR)||" \
				>> "$(INSTALL_MANIFEST)"

.PHONY: install-js-runtime
install-js-runtime:
	mkdir -p "$(INSTALL_JS_LIBDIR)"
	set -e; \
	for f in "$(JS_BUILD_LIBDIR)"/*; do \
		[ -e "$$f" ] || continue; \
		if [ -f "$$f" ]; then \
			b=$$(basename "$$f"); \
			install -m 644 "$$f" "$(INSTALL_JS_LIBDIR)/$$b"; \
			echo "$(if $(strip $(DESTDIR)),$(INSTALL_STAGE_JS_LIBDIR),$(FBINSTALL_JS_RUNTIME_DIR))/$$b" >> "$(INSTALL_MANIFEST)"; \
		fi; \
	done; \
	for b in $(FB_JS_RUNTIME_ASSETS); do \
		install -m 644 "$(rootdir)/lib/$$b" "$(INSTALL_JS_LIBDIR)/$$b"; \
		echo "$(if $(strip $(DESTDIR)),$(INSTALL_STAGE_JS_LIBDIR),$(FBINSTALL_JS_RUNTIME_DIR))/$$b" >> "$(INSTALL_MANIFEST)"; \
	done

.PHONY: install-android install-android-bin install-android-includes install-android-runtime install-android-tools
install-android: install-android-bin install-android-includes install-android-runtime install-android-tools

.PHONY: install-android-bin
install-android-bin:
	mkdir -p "$(INSTALL_BINDIR)" "$(INSTALL_ANDROID_BINDIR)"
	install -m 755 "$(FBC_ANDROID_EXE)" "$(INSTALL_ANDROID_BINDIR)/$(FBC_ANDROID_COMPILER_INSTALL_NAME)"
	install -m 755 "$(ANDROID_TOOLS_DIR)/fbc-android" "$(INSTALL_BINDIR)/$(FBC_ANDROID_INSTALL_NAME)"
	@echo "$(if $(strip $(DESTDIR)),$(INSTALL_STAGE_ANDROID_BINDIR),$(prefixandroidbindir))/$(FBC_ANDROID_COMPILER_INSTALL_NAME)" >> "$(INSTALL_MANIFEST)"
	@echo "$(if $(strip $(DESTDIR)),$(INSTALL_STAGE_BINDIR),$(prefixbindir))/$(FBC_ANDROID_INSTALL_NAME)" >> "$(INSTALL_MANIFEST)"

.PHONY: install-android-includes
install-android-includes:
	$(MKDIR_P) "$(INSTALL_ANDROID_INCDIR)"
	cp -a "$(rootdir)/inc/." "$(INSTALL_ANDROID_INCDIR)/"
	@find "$(INSTALL_ANDROID_INCDIR)" -type f \
			| sed "s|^$(DESTDIR)||" \
				>> "$(INSTALL_MANIFEST)"

.PHONY: install-android-runtime
install-android-runtime:
	@test -d "$(ANDROID_BUILD_LIBDIR)" || { echo "ERROR: Android runtime build directory missing: $(ANDROID_BUILD_LIBDIR)"; exit 1; }
	mkdir -p "$(INSTALL_ANDROID_LIBDIR)"
	set -e; \
	for f in "$(ANDROID_BUILD_LIBDIR)"/*; do \
		[ -e "$$f" ] || continue; \
		if [ -f "$$f" ]; then \
			b=$$(basename "$$f"); \
			install -m 644 "$$f" "$(INSTALL_ANDROID_LIBDIR)/$$b"; \
			echo "$(if $(strip $(DESTDIR)),$(INSTALL_STAGE_ANDROID_LIBDIR),$(FBINSTALL_ANDROID_RUNTIME_DIR))/$$b" >> "$(INSTALL_MANIFEST)"; \
		fi; \
	done

.PHONY: install-android-tools
install-android-tools:
	mkdir -p "$(INSTALL_ANDROID_SHAREDIR)/template"
	install -m 644 "$(ANDROID_TOOLS_DIR)/fb_android_app.c" "$(INSTALL_ANDROID_SHAREDIR)/template/fb_android_app.c"
	install -m 644 "$(ANDROID_TOOLS_DIR)/AndroidManifest.xml.in" "$(INSTALL_ANDROID_SHAREDIR)/template/AndroidManifest.xml.in"
	install -m 644 "$(ANDROID_TOOLS_DIR)/strings.xml" "$(INSTALL_ANDROID_SHAREDIR)/template/strings.xml"
	@echo "$(if $(strip $(DESTDIR)),$(INSTALL_STAGE_ANDROID_SHAREDIR),$(prefixandroidsharedir))/template/fb_android_app.c" >> "$(INSTALL_MANIFEST)"
	@echo "$(if $(strip $(DESTDIR)),$(INSTALL_STAGE_ANDROID_SHAREDIR),$(prefixandroidsharedir))/template/AndroidManifest.xml.in" >> "$(INSTALL_MANIFEST)"
	@echo "$(if $(strip $(DESTDIR)),$(INSTALL_STAGE_ANDROID_SHAREDIR),$(prefixandroidsharedir))/template/strings.xml" >> "$(INSTALL_MANIFEST)"

.PHONY: uninstall
uninstall:
	@if [ ! -f "$(INSTALL_MANIFEST)" ]; then \
		echo "No install manifest found."; \
		exit 0; \
	fi

	@echo "Removing installed files..."
	@set -e; \
	while IFS= read -r f; do \
		[ -n "$$f" ] || continue; \
		path="$(DESTDIR)$$f"; \
		rm -f "$$path" || true; \
	done < "$(INSTALL_MANIFEST)"

	@echo "Pruning empty directories..."
	@rmdir --ignore-fail-on-non-empty \
		"$(INSTALL_BINDIR)" \
		"$(INSTALL_INCDIR)" \
		"$(INSTALL_LIBDIR)" \
		2>/dev/null || true

	@rm -f "$(INSTALL_MANIFEST)"

.PHONY: pkg-tar pkg-zip
pkg-tar:
	rm -rf pkgroot
	$(MAKE) DESTDIR="$(SRC_ROOT)/pkgroot" install
	tar -C pkgroot -cf freebasic-package.tar .

pkg-zip:
	rm -rf pkgroot
	$(MAKE) DESTDIR="$(SRC_ROOT)/pkgroot" install
	cd pkgroot && zip -r ../freebasic-package.zip .

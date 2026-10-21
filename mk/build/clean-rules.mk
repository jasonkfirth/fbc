##############################################################################
# clean-rules.mk
##############################################################################
#
# Centralized cleaning policy
# Project: FreeBASIC build system
# Purpose: Remove explicitly requested generated artifact sets.
# Responsibilities: Keep component cleanup and bootstrap-source aliases together.
# This file intentionally does NOT create or install build products.
#
# clean                → normal developer clean
# distclean            → remove all build artifacts
#
# Component cleaning:
#   clean-compiler
#   clean-libs
#   clean-tests
#   clean-dist
#   clean-bootstrap
#   clean-bootstrap-src
#
# Bootstrap emitted sources are preserved unless explicitly removed.
#
##############################################################################

.PHONY: \
  clean \
  distclean \
  clean-build \
  clean-compiler \
  clean-all-objects \
  list-example-artifacts \
  clean-example-artifacts \
  clean-libs \
  clean-tests \
  clean-dist \
  clean-bootstrap \
  clean-bootstrap-src


##############################################################################
# Compiler objects
##############################################################################

clean-compiler:
	@if [ -n "$(fbcobjdir)" ]; then rm -rf "$(fbcobjdir)"; fi
	@if [ -n "$(fbcjsobjdir)" ]; then rm -rf "$(fbcjsobjdir)"; fi
	@if [ -n "$(fbcandroidobjdir)" ]; then rm -rf "$(fbcandroidobjdir)"; fi
	@if [ -n "$(fbcwiiobjdir)" ]; then rm -rf "$(fbcwiiobjdir)"; fi


##############################################################################
# All generated object roots
##############################################################################

clean-all-objects:
	@rm -rf $(BUILD_OUTPUT_PREFIX)src/compiler/obj
	@rm -rf $(BUILD_OUTPUT_PREFIX)src/rtlib/obj
	@rm -rf $(BUILD_OUTPUT_PREFIX)src/fbrt/obj
	@rm -rf $(BUILD_OUTPUT_PREFIX)src/gfxlib2/obj
	@rm -rf $(BUILD_OUTPUT_PREFIX)src/gfxlib3/obj
	@rm -rf $(BUILD_OUTPUT_PREFIX)src/sfxlib/obj


##############################################################################
# Runtime library objects
##############################################################################

clean-libs:
	@if [ -n "$(libfbobjdir)" ]; then rm -rf "$(libfbobjdir)"; fi
	@if [ -n "$(libfbpicobjdir)" ]; then rm -rf "$(libfbpicobjdir)"; fi
	@if [ -n "$(libfbmtobjdir)" ]; then rm -rf "$(libfbmtobjdir)"; fi
	@if [ -n "$(libfbmtpicobjdir)" ]; then rm -rf "$(libfbmtpicobjdir)"; fi

	@if [ -n "$(libfbrtobjdir)" ]; then rm -rf "$(libfbrtobjdir)"; fi
	@if [ -n "$(libfbrtpicobjdir)" ]; then rm -rf "$(libfbrtpicobjdir)"; fi
	@if [ -n "$(libfbrtmtobjdir)" ]; then rm -rf "$(libfbrtmtobjdir)"; fi
	@if [ -n "$(libfbrtmtpicobjdir)" ]; then rm -rf "$(libfbrtmtpicobjdir)"; fi

	@if [ -n "$(libfbgfxobjdir)" ]; then rm -rf "$(libfbgfxobjdir)"; fi
	@if [ -n "$(libfbgfxpicobjdir)" ]; then rm -rf "$(libfbgfxpicobjdir)"; fi
	@if [ -n "$(libfbgfxmtobjdir)" ]; then rm -rf "$(libfbgfxmtobjdir)"; fi
	@if [ -n "$(libfbgfxmtpicobjdir)" ]; then rm -rf "$(libfbgfxmtpicobjdir)"; fi

	@if [ -n "$(libfbgfx3objdir)" ]; then rm -rf "$(libfbgfx3objdir)"; fi
	@if [ -n "$(libfbgfx3picobjdir)" ]; then rm -rf "$(libfbgfx3picobjdir)"; fi
	@if [ -n "$(libfbgfx3mtobjdir)" ]; then rm -rf "$(libfbgfx3mtobjdir)"; fi
	@if [ -n "$(libfbgfx3mtpicobjdir)" ]; then rm -rf "$(libfbgfx3mtpicobjdir)"; fi

	@if [ -n "$(libsfxobjdir)" ]; then rm -rf "$(libsfxobjdir)"; fi
	@if [ -n "$(libsfxpicobjdir)" ]; then rm -rf "$(libsfxpicobjdir)"; fi
	@if [ -n "$(libsfxmtobjdir)" ]; then rm -rf "$(libsfxmtobjdir)"; fi
	@if [ -n "$(libsfxmtpicobjdir)" ]; then rm -rf "$(libsfxmtpicobjdir)"; fi

	@rm -f $(libdir)/libfb*.a
	@rm -f $(libdir)/libfbrt*.a
	@rm -f $(libdir)/libfbgfx*.a
	@rm -f $(libdir)/libsfx*.a
	@rm -f $(libdir)/.archive-members/*.a
	@rmdir "$(libdir)/.archive-members" 2>/dev/null || true
	@rm -f "$(libdir)/.build-config/"*.a "$(libdir)/.build-config/"*.o
	@rm -f "$(libdir)/.build-config/"*.a.pending "$(libdir)/.build-config/"*.o.pending
	@rmdir "$(libdir)/.build-config" 2>/dev/null || true
	@rm -f $(libdir)/fbrt0.o
	@rm -f $(libdir)/fbrt0pic.o
	@rm -f $(libdir)/fbrt1.o
	@rm -f $(libdir)/fbrt1pic.o
	@rm -f $(libdir)/fbrt2.o
	@rm -f $(libdir)/fbrt2pic.o


##############################################################################
# Build directories
##############################################################################

clean-build:
	@rm -rf $(BUILD_OUTPUT_PREFIX)bin
	@rm -rf $(BUILD_OUTPUT_PREFIX)obj
	@rm -rf $(BUILD_OUTPUT_PREFIX)stage
	@rm -rf $(BUILD_OUTPUT_PREFIX)dist


##############################################################################
# Test artifacts
##############################################################################

define _find_example_artifacts
if [ -d "$(rootdir)/examples" ]; then \
	find "$(rootdir)/examples" -type f \
		\( -name '*.asm' -o -name '*.c' -o -name '*.o' -o -name '*.obj' -o -name '*.a' -o -name '*.exe' -o -name '*.dll' -o -name '*.stdout.txt' -o -name '*.stderr.txt' \) \
		-print | while IFS= read -r f; do \
			case "$$f" in \
				*.stdout.txt) stem="$${f%.stdout.txt}" ;; \
				*.stderr.txt) stem="$${f%.stderr.txt}" ;; \
				*) stem="$${f%.*}" ;; \
			esac; \
			if [ -f "$$stem.bas" ] || [ -f "$$stem.bi" ]; then \
				printf '%s\n' "$$f"; \
			fi; \
		done | LC_ALL=C sort; \
fi
endef

.PHONY: list-example-artifacts
list-example-artifacts:
	@$(call _find_example_artifacts)

.PHONY: clean-example-artifacts
clean-example-artifacts:
	@if [ -f "$(rootdir)/examples/GNUmakefile" ]; then \
		$(MAKE) --no-print-directory -C "$(rootdir)/examples" clean >/dev/null; \
	fi
	@rm -f "examples/manual/proguide/hello.txt" "$(rootdir)/examples/manual/proguide/hello.txt"
	@$(MAKE) --no-print-directory -s list-example-artifacts | while IFS= read -r f; do \
		[ -n "$$f" ] || continue; \
		echo "==> Removing $$f"; \
		rm -f "$$f"; \
	done

clean-tests:
ifneq ($(strip $(BUILD_ROOT)),)
	@echo "ERROR: test cleanup uses shared source paths; use a separate worktree"
	@exit 1
else
	@rm -rf .maketests-tmp
	@rm -rf maketests-log
	@rm -rf test-run-log
	@$(MAKE) --no-print-directory clean-example-artifacts
	@cd tests && $(MAKE) --no-print-directory -f unit-tests.mk clean
	@cd tests && $(MAKE) --no-print-directory -f log-tests.mk FB_LANG=fb clean
	@cd tests && $(MAKE) --no-print-directory -f log-tests.mk FB_LANG=fblite clean
	@cd tests && $(MAKE) --no-print-directory -f log-tests.mk FB_LANG=qb clean
	@cd tests && $(MAKE) --no-print-directory -f log-tests.mk FB_LANG=deprecated clean
endif


##############################################################################
# Distribution artifacts
##############################################################################

clean-dist:
	@rm -f $(BUILD_OUTPUT_PREFIX)FreeBASIC-*.tar
	@rm -f $(BUILD_OUTPUT_PREFIX)FreeBASIC-*.tar.*
	@rm -f $(BUILD_OUTPUT_PREFIX)FreeBASIC-*.zip
	@rm -f $(BUILD_OUTPUT_PREFIX)FreeBASIC-*.7z


##############################################################################
# Bootstrap cleaning
##############################################################################

# Remove bootstrap compiler binaries and objects
clean-bootstrap:
	@rm -f $(BOOTSTRAP_FBC)
	@rm -rf $(BUILD_OUTPUT_PREFIX)bootstrap/obj
	@rm -f $(BUILD_OUTPUT_PREFIX)bootstrap/*/*.o


# Explicit removal of emitted bootstrap sources
clean-bootstrap-sources: clean-bootstrap-src

clean-bootstrap-src:
	@rm -f $(BUILD_OUTPUT_PREFIX)bootstrap/*/*.c
	@rm -f $(BUILD_OUTPUT_PREFIX)bootstrap/*/*.asm
	@rm -f $(BUILD_OUTPUT_PREFIX)bootstrap/*/*.ll
	@rm -f $(BUILD_OUTPUT_PREFIX)bootstrap/*/.emit-config*
	@rm -f $(BUILD_OUTPUT_PREFIX)bootstrap/*/.compiler-*.rsp
	@rm -rf $(BUILD_OUTPUT_PREFIX)bootstrap/*/.emit-work


##############################################################################
# Primary clean target
##############################################################################

clean: \
  clean-compiler \
  clean-libs \
  clean-build
ifeq ($(strip $(BUILD_ROOT)),)
clean: clean-tests
endif


##############################################################################
# Deep clean
##############################################################################

distclean: \
  clean \
  clean-dist \
  clean-bootstrap

	@if [ -n "$(libdir)" ]; then rm -rf "$(libdir)"; fi
	@rm -f $(FBC_EXE) $(FBC_JS_EXE)


##############################################################################
# End clean-rules.mk
##############################################################################

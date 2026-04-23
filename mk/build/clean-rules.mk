##############################################################################
# clean-rules.mk
##############################################################################
#
# Centralized cleaning policy
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

	@if [ -n "$(libsfxobjdir)" ]; then rm -rf "$(libsfxobjdir)"; fi
	@if [ -n "$(libsfxpicobjdir)" ]; then rm -rf "$(libsfxpicobjdir)"; fi
	@if [ -n "$(libsfxmtobjdir)" ]; then rm -rf "$(libsfxmtobjdir)"; fi
	@if [ -n "$(libsfxmtpicobjdir)" ]; then rm -rf "$(libsfxmtpicobjdir)"; fi

	@rm -f $(libdir)/libfb*.a
	@rm -f $(libdir)/libfbrt*.a
	@rm -f $(libdir)/libfbgfx*.a
	@rm -f $(libdir)/libsfx*.a
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
	@rm -rf bin
	@rm -rf obj
	@rm -rf stage
	@rm -rf dist


##############################################################################
# Test artifacts
##############################################################################

define _find_example_artifacts
if [ -d "$(rootdir)/examples" ]; then \
	find "$(rootdir)/examples" -type f \
		\( -name '*.asm' -o -name '*.c' -o -name '*.o' -o -name '*.obj' -o -name '*.exe' -o -name '*.stdout.txt' -o -name '*.stderr.txt' \) \
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
	@$(MAKE) --no-print-directory -s list-example-artifacts | while IFS= read -r f; do \
		[ -n "$$f" ] || continue; \
		echo "==> Removing $$f"; \
		rm -f "$$f"; \
	done

clean-tests:
	@rm -rf .maketests-tmp
	@rm -rf maketests-log
	@rm -rf test-run-log
	@$(MAKE) --no-print-directory clean-example-artifacts


##############################################################################
# Distribution artifacts
##############################################################################

clean-dist:
	@rm -f FreeBASIC-*.tar
	@rm -f FreeBASIC-*.tar.*
	@rm -f FreeBASIC-*.zip
	@rm -f FreeBASIC-*.7z


##############################################################################
# Bootstrap cleaning
##############################################################################

# Remove bootstrap compiler binaries and objects
clean-bootstrap:
	@rm -f bootstrap/fbc$(EXEEXT)
	@rm -f bootstrap/*/*.o


# Explicit removal of emitted bootstrap sources
clean-bootstrap-src:
	@rm -f bootstrap/*/*.c
	@rm -f bootstrap/*/*.asm


##############################################################################
# Primary clean target
##############################################################################

clean: \
  clean-compiler \
  clean-libs \
  clean-build \
  clean-tests


##############################################################################
# Deep clean
##############################################################################

distclean: \
  clean \
  clean-dist \
  clean-bootstrap

	@if [ -n "$(libdir)" ]; then rm -rf "$(libdir)"; fi
	@rm -f $(FBC_EXE)


##############################################################################
# End clean-rules.mk
##############################################################################

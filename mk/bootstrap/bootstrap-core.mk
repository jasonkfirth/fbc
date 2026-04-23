##############################################################################
# bootstrap-core.mk
#
# Deterministic bootstrap workflow
#
# Responsibilities
#   - build stage0 bootstrap compiler
#   - emit bootstrap sources
#   - construct minimal compiler using emitted sources
##############################################################################

BOOTSTRAP_FBC := bootstrap/fbc$(EXEEXT)

.PHONY: \
 bootstrap \
 bootstrap-check \
 bootstrap-minimal \
 bootstrap-seed-peer \
 bootstrap-emit \
 clean-bootstrap \
 clean-bootstrap-sources

##############################################################################
# Bootstrap directory identity
##############################################################################

BOOTSTRAP_DIR := $(if $(strip $(FBTARGET_DIR_OVERRIDE)),$(FBTARGET_DIR_OVERRIDE),$(FBTARGET))
BOOTSTRAP_PATH := bootstrap/$(BOOTSTRAP_DIR)

##############################################################################
# Bootstrap compiler selection
##############################################################################

BOOT_FBC := $(AVAILABLE_FBC)

##############################################################################
# Derive bootstrap OS
##############################################################################

BOOTSTRAP_OS := $(word 1,$(subst -, ,$(BOOTSTRAP_DIR)))

ifeq ($(BOOTSTRAP_OS),mingw)
BOOTSTRAP_OS := win32
endif

##############################################################################
# Canonical arch
##############################################################################

BOOTSTRAP_ARCH := $(TARGET_ARCH)

BOOTSTRAP_TERM_LIB := -lncurses

ifeq ($(TARGET_OS),haiku)
BOOTSTRAP_TERM_LIB := -lncursesw
endif

ifeq ($(TARGET_OS),dos)
BOOTSTRAP_TERM_LIB :=
endif

##############################################################################
# Bootstrap emission target
##############################################################################

BOOT_EMIT_TARGET := \
$(if $(strip $(FBC_TARGET)), \
 $(firstword $(subst -, ,$(FBC_TARGET))), \
 $(BOOTSTRAP_OS) \
)

##############################################################################
# Bootstrap workflow
##############################################################################

bootstrap: bootstrap-check rtlib bootstrap-minimal gfxlib2

$(BOOTSTRAP_PATH):
	mkdir -p "$@"

##############################################################################
# Bootstrap capability check
##############################################################################

bootstrap-check:
	@echo "==> Verifying bootstrap capability"
	@if [ -d "$(BOOTSTRAP_PATH)" ] && find "$(BOOTSTRAP_PATH)" -maxdepth 1 -type f \( -name '*.c' -o -name '*.asm' \) -print -quit | grep -q .; then \
		echo "==> Bootstrap sources detected"; \
	elif [ -n "$(BOOT_FBC)" ]; then \
		echo "==> Using available compiler: $(BOOT_FBC)"; \
	else \
		echo ""; \
		echo "ERROR: cannot run bootstrap."; \
		echo "No bootstrap sources or FreeBASIC compiler available."; \
		echo ""; \
		exit 1; \
	fi

##############################################################################
# Minimal bootstrap build
##############################################################################

bootstrap-minimal:

	@echo "==> Checking bootstrap sources"

	@if ! [ -d "$(BOOTSTRAP_PATH)" ] || ! find "$(BOOTSTRAP_PATH)" -maxdepth 1 -type f \( -name '*.c' -o -name '*.asm' \) -print -quit | grep -q .; then \
		echo ""; \
		echo "ERROR: bootstrap sources missing"; \
		echo "Run: make bootstrap-emit"; \
		echo ""; \
		exit 1; \
	fi

	@echo "==> Building bootstrap compiler"

	$(MAKE) BOOTSTRAP_MINIMAL=YesPlease $(BOOTSTRAP_FBC)

	@echo "==> Installing bootstrap compiler"

	mkdir -p "$(dir $(FBC_EXE))"
	cp $(BOOTSTRAP_FBC) $(FBC_EXE).new
	mv -f $(FBC_EXE).new $(FBC_EXE)

	mkdir -p "$(libdir)"

##############################################################################
# Last-resort peer bootstrap seeding
##############################################################################

BOOTSTRAP_DIRS := $(patsubst bootstrap/%/,%,$(wildcard bootstrap/*/))
BOOTSTRAP_BSD_DONORS := $(filter openbsd-$(TARGET_ARCH) netbsd-$(TARGET_ARCH) freebsd-$(TARGET_ARCH) dragonfly-$(TARGET_ARCH),$(BOOTSTRAP_DIRS))
BOOTSTRAP_PREFERRED_DONORS :=
ifneq ($(filter $(TARGET_OS),openbsd netbsd freebsd dragonfly),)
BOOTSTRAP_PREFERRED_DONORS := $(BOOTSTRAP_BSD_DONORS)
endif
BOOTSTRAP_SAME_ARCH_DONORS := $(filter %-$(TARGET_ARCH),$(BOOTSTRAP_DIRS))
BOOTSTRAP_DONOR_CANDIDATES := $(filter-out $(BOOTSTRAP_DIR),$(BOOTSTRAP_PREFERRED_DONORS) $(filter-out $(BOOTSTRAP_PREFERRED_DONORS),$(BOOTSTRAP_SAME_ARCH_DONORS)))

bootstrap-seed-peer:
	@echo "==> Last-resort bootstrap seeding for $(BOOTSTRAP_DIR)"
	@set -e; \
	if [ -d "$(BOOTSTRAP_PATH)" ] && find "$(BOOTSTRAP_PATH)" -maxdepth 1 -type f \( -name '*.c' -o -name '*.asm' \) -print -quit | grep -q .; then \
		echo ""; \
		echo "ERROR: bootstrap sources already exist for $(BOOTSTRAP_DIR)"; \
		echo "Refusing to overwrite existing sources."; \
		echo ""; \
		exit 1; \
	fi; \
	donor=""; \
	for cand in $(BOOTSTRAP_DONOR_CANDIDATES); do \
		if find "bootstrap/$$cand" -maxdepth 1 -type f \( -name '*.c' -o -name '*.asm' \) -print -quit | grep -q .; then \
			donor="$$cand"; \
			break; \
		fi; \
	done; \
	if [ -z "$$donor" ]; then \
		echo ""; \
		echo "ERROR: no peer bootstrap sources available for $(BOOTSTRAP_DIR)"; \
		echo "Candidates searched: $(BOOTSTRAP_DONOR_CANDIDATES)"; \
		echo ""; \
		exit 1; \
	fi; \
	echo "==> Using donor bootstrap: $$donor"; \
	mkdir -p "$(BOOTSTRAP_PATH)"; \
	for ext in c asm; do \
		for f in bootstrap/$$donor/*.$$ext; do \
			[ -e "$$f" ] || continue; \
			cp -f "$$f" "$(BOOTSTRAP_PATH)/"; \
		done; \
	done
	@$(MAKE) bootstrap-minimal
	@$(MAKE) bootstrap-emit
	@$(MAKE) bootstrap-minimal

##############################################################################
# Bootstrap object discovery
##############################################################################

BOOTSTRAP_C_SRC  := $(wildcard $(BOOTSTRAP_PATH)/*.c)
BOOTSTRAP_ASM_SRC := $(wildcard $(BOOTSTRAP_PATH)/*.asm)

BOOTSTRAP_OBJ := \
$(patsubst %.c,%.o,$(BOOTSTRAP_C_SRC)) \
$(patsubst %.asm,%.o,$(BOOTSTRAP_ASM_SRC))

##############################################################################
# Bootstrap object compilation
##############################################################################

#
# Emitted bootstrap C is machine-generated compiler output.
#
# It should still compile with the normal optimization, portability,
# feature, and hardening flags where practical, but the ordinary warning
# profile is too noisy for generated code and can turn benign emitter
# patterns into build failures on newer compilers.
#
# Keep this warning relaxation scoped to bootstrap-minimal so that normal
# source builds continue using the project's full warning policy.
#
BOOTSTRAP_CFLAGS := \
 $(filter-out \
  -Wall \
  -Wextra \
  -Werror=implicit-function-declaration \
  -Wfatal-errors \
  -Wformat \
  -Werror=format-security, \
  $(ALLCFLAGS))

BOOTSTRAP_WARN_SUPPRESS := \
 -Wno-format \
 -Wno-sign-compare \
 -Wno-parentheses \
 -Wno-unused-parameter \
 -Wno-unused-label \
 -Wno-unused-variable \
 -Wno-unused-function

# IMPORTANT: build bootstrap objects with PIC (required for PIE on OpenBSD)
%.o: %.c
	$(RUN_CC) $(CPPFLAGS) $(BOOTSTRAP_CFLAGS) $(BOOTSTRAP_WARN_SUPPRESS) $(PIC_CFLAGS) -c $< -o $@

%.o: %.asm
	$(AS) $(ASFLAGS) --strip-local-absolute $< -o $@

##############################################################################
# Bootstrap compiler link
##############################################################################

# Use PIC runtime when PIE is enabled
ifeq ($(ENABLE_PIE),YesPlease)
  BOOTSTRAP_RT0 := $(libdir)/fbrt0pic.o
  BOOTSTRAP_RTL := $(libdir)/libfbpic.a
else
  BOOTSTRAP_RT0 := $(libdir)/fbrt0.o
  BOOTSTRAP_RTL := $(libdir)/libfb.a
endif

$(BOOTSTRAP_FBC): rtlib $(BOOTSTRAP_OBJ) | $(BOOTSTRAP_PATH)

	$(RUN_CC) $(ALLLDFLAGS) -o $@ \
	$(BOOTSTRAP_RT0) \
	$(BOOTSTRAP_OBJ) \
	$(BOOTSTRAP_RTL) \
	-lm $(THREAD_FLAGS) $(BOOTSTRAP_TERM_LIB)

##############################################################################
# End bootstrap-core.mk
##############################################################################

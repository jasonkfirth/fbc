#################
# host-tools.mk #
#################
#
# Host/OS command behaviour and small portability shims.
#
# Responsibilities:
#   - EXEEXT selection (authoritative; platform.mk may also set it, but this is
#     the “host tools” view used by install/package rules)
#   - INSTALL_PROGRAM / INSTALL_FILE defaults that work on each OS family
#   - Avoid feature policy and toolchain flag policy (handled elsewhere)
#
# Haiku notes:
#   - There is no GNU coreutils-style `install` by default on many Haiku setups.
#   - Prefer `install` if present, otherwise fall back to `cp`.
#   - Use `mkdir -p` in install rules elsewhere (already done in your tree).
#################


# Choose an install tool:
# - Windows-like environments: use cp (consistent with existing behaviour)
# - Haiku: prefer install if available, else cp
# - Everything else: assume install exists
ifneq ($(filter cygwin dos win32 win64,$(TARGET_OS)),)

  INSTALL_PROGRAM := cp
  INSTALL_FILE    := cp

else ifeq ($(TARGET_OS),haiku)

  HAVE_INSTALL := $(shell command -v install >/dev/null 2>&1 && echo yes || echo no)
  ifeq ($(HAVE_INSTALL),yes)
    INSTALL_PROGRAM := install
    INSTALL_FILE    := install -m 644
  else
    $(warning host-tools.mk: 'install' not found on Haiku; falling back to 'cp')
    INSTALL_PROGRAM := cp
    INSTALL_FILE    := cp
  endif

else

  INSTALL_PROGRAM := install
  INSTALL_FILE    := install -m 644

endif

########################
# end of host-tools.mk #
########################

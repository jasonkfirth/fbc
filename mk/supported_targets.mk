##############################################################################
# supported_targets.mk
#
# Canonical FreeBASIC supported bootstrap targets
#
#
# Targets are defined as:
#
#   GENERAL_TARGETS = (GENERAL_OS × GENERAL_ARCH)
#   FINAL_TARGETS   = GENERAL_TARGETS + SPECIAL_TARGETS − INVALID_TARGETS
#
# This avoids generating a large hand-maintained matrix while still allowing
# explicit special-case targets such as DOS.
##############################################################################

##############################################################################
# General operating systems (cross-product)
##############################################################################

GENERAL_OS := \
	linux \
	freebsd \
	netbsd \
	openbsd \
	dragonfly \
	haiku \
	mingw \
	cygwin


##############################################################################
# General architectures (cross-product)
##############################################################################

GENERAL_ARCH := \
	amd64 \
	i386 \
	arm64 \
	armhf \
	armel \
	powerpc \
	powerpc64 \
	ppc64el \
	riscv64 \
	s390x \
	loongarch64


##############################################################################
# Special targets that do not participate in the cross product
##############################################################################

SPECIAL_TARGETS := \
	darwin-x86_64 \
	darwin-aarch64 \
	dos


##############################################################################
# Known invalid combinations
##############################################################################

INVALID_TARGETS :=


##############################################################################
# Cartesian product expansion
##############################################################################

define _fb_target_product
$(foreach os,$(GENERAL_OS),$(foreach arch,$(GENERAL_ARCH),$(os)-$(arch)))
endef

GENERAL_TARGET_MATRIX := $(call _fb_target_product)


##############################################################################
# Final supported bootstrap targets
##############################################################################

SUPPORTED_BOOTSTRAP_TARGETS := $(filter-out $(INVALID_TARGETS),$(GENERAL_TARGET_MATRIX) $(SPECIAL_TARGETS))


##############################################################################
# End supported_targets.mk
##############################################################################

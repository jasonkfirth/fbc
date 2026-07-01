##############################################################################
# supported_targets.mk
#
# Canonical FreeBASIC supported bootstrap emission matrix
#
# Matrix entries are defined as:
#
#   fbc-target:bootstrap-dir:target-triplet
#
# The fbc target is the value passed to `fbc -target`. The bootstrap directory
# is where emitted C/ASM sources are staged. The target triplet feeds the build
# identity layer so each matrix entry emits with the intended CPU family.
##############################################################################

##############################################################################
# CPU family helpers
##############################################################################

BOOTSTRAP_CPU_FAMILIES := \
	x86 \
	x86_64 \
	arm \
	aarch64 \
	powerpc \
	powerpc64 \
	powerpc64le \
	riscv32 \
	riscv64 \
	s390x \
	loongarch64

define _fb_triplet_arch
$(if $(filter x86,$(1)),i686,$(1))
endef

define _fb_bootstrap_spec
$(1):$(2):$(3)
endef

define _fb_os_arch_specs
$(foreach arch,$(BOOTSTRAP_CPU_FAMILIES),$(call _fb_bootstrap_spec,$(1)-$(arch),$(1)-$(arch),$(call _fb_triplet_arch,$(arch))-unknown-$(1)))
endef

##############################################################################
# Linux
##############################################################################

LINUX_BOOTSTRAP_TARGETS := \
	$(call _fb_bootstrap_spec,linux-x86,linux-x86,i686-linux-gnu) \
	$(call _fb_bootstrap_spec,linux-x86_64,linux-x86_64,x86_64-linux-gnu) \
	$(call _fb_bootstrap_spec,linux-arm,linux-arm,arm-linux-gnueabi) \
	$(call _fb_bootstrap_spec,linux-aarch64,linux-aarch64,aarch64-linux-gnu) \
	$(call _fb_bootstrap_spec,linux-powerpc,linux-powerpc,powerpc-linux-gnu) \
	$(call _fb_bootstrap_spec,linux-powerpc64,linux-powerpc64,powerpc64-linux-gnu) \
	$(call _fb_bootstrap_spec,linux-powerpc64le,linux-powerpc64le,powerpc64le-linux-gnu) \
	$(call _fb_bootstrap_spec,linux-riscv32,linux-riscv32,riscv32-linux-gnu) \
	$(call _fb_bootstrap_spec,linux-riscv64,linux-riscv64,riscv64-linux-gnu) \
	$(call _fb_bootstrap_spec,linux-s390x,linux-s390x,s390x-linux-gnu) \
	$(call _fb_bootstrap_spec,linux-loongarch64,linux-loongarch64,loongarch64-linux-gnu)

##############################################################################
# BSD family and Haiku
##############################################################################

BSD_BOOTSTRAP_TARGETS := \
	$(call _fb_os_arch_specs,freebsd) \
	$(call _fb_os_arch_specs,netbsd) \
	$(call _fb_os_arch_specs,openbsd) \
	$(call _fb_os_arch_specs,dragonfly)

HAIKU_BOOTSTRAP_TARGETS := \
	$(call _fb_os_arch_specs,haiku)

##############################################################################
# Windows
##############################################################################

WINDOWS_BOOTSTRAP_TARGETS := \
	$(call _fb_bootstrap_spec,win32,win32,i686-w64-mingw32) \
	$(call _fb_bootstrap_spec,win64,win64,x86_64-w64-mingw32) \
	$(call _fb_bootstrap_spec,win32-aarch64,win32-aarch64,aarch64-w64-mingw32)

##############################################################################
# Other operating systems
##############################################################################

CYGWIN_BOOTSTRAP_TARGETS := \
	$(call _fb_bootstrap_spec,cygwin-x86,cygwin-x86,i686-pc-cygwin) \
	$(call _fb_bootstrap_spec,cygwin-x86_64,cygwin-x86_64,x86_64-pc-cygwin)

DARWIN_BOOTSTRAP_TARGETS := \
	$(call _fb_bootstrap_spec,darwin-x86_64,darwin-x86_64,x86_64-apple-darwin) \
	$(call _fb_bootstrap_spec,darwin-aarch64,darwin-aarch64,aarch64-apple-darwin)

SOLARIS_BOOTSTRAP_TARGETS := \
	$(call _fb_bootstrap_spec,solaris-x86_64,solaris-x86_64,x86_64-pc-solaris)

ILLUMOS_BOOTSTRAP_TARGETS := \
	$(call _fb_bootstrap_spec,illumos-x86_64,illumos-x86_64,x86_64-pc-illumos)

DOS_BOOTSTRAP_TARGETS := \
	$(call _fb_bootstrap_spec,dos,dos,i586-pc-msdosdjgpp)

NUTTX_BOOTSTRAP_TARGETS := \
	$(call _fb_bootstrap_spec,nuttx-riscv32,nuttx-riscv32,riscv32-unknown-nuttx)

##############################################################################
# Final supported bootstrap targets
##############################################################################

SUPPORTED_BOOTSTRAP_TARGETS := \
	$(LINUX_BOOTSTRAP_TARGETS) \
	$(BSD_BOOTSTRAP_TARGETS) \
	$(HAIKU_BOOTSTRAP_TARGETS) \
	$(WINDOWS_BOOTSTRAP_TARGETS) \
	$(CYGWIN_BOOTSTRAP_TARGETS) \
	$(DARWIN_BOOTSTRAP_TARGETS) \
	$(SOLARIS_BOOTSTRAP_TARGETS) \
	$(ILLUMOS_BOOTSTRAP_TARGETS) \
	$(DOS_BOOTSTRAP_TARGETS) \
	$(NUTTX_BOOTSTRAP_TARGETS)

SUPPORTED_BOOTSTRAP_DIRS := $(foreach spec,$(SUPPORTED_BOOTSTRAP_TARGETS),$(word 2,$(subst :, ,$(spec))))


##############################################################################
# End supported_targets.mk
##############################################################################

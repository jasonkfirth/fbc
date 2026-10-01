##############################################################################
# FreeBASIC compiler build
#
# File: compiler-sources.mk
#
# Purpose:
#   Describe the compiler's subsystem directories and select host policies.
#
# Responsibilities:
#   - provide one source list for native, variant, and bootstrap builds
#   - replace generic host policies by basename for the compiler's build target
#   - reject ambiguous flat object names before a compilation starts
#
# This file intentionally does NOT contain:
#   - compiler flags, output directory policy, or runtime source discovery
##############################################################################

# Includes use paths relative to src/compiler. Keep this list explicit: adding
# a subsystem is a build graph change, while generated obj/ trees are never
# searched for sources. Basenames remain stable for bootstrap archives and
# release scripts that rebuild fbc.o when the version changes.
FBC_SOURCE_GROUPS := \
core \
diagnostics \
driver \
lexer \
ast \
ast/nodes \
ast/optimize \
parser \
parser/declarations \
parser/expressions \
parser/procedures \
parser/statements \
parser/intrinsics \
preprocessor \
symbols \
runtime \
backend \
backend/c \
backend/llvm \
backend/gas64 \
backend/x86 \
backend/debug \
support \
support/containers \
support/strings \
support/numeric \
tooling

FBC_GENERIC_DIRS := $(addprefix $(srcdir)/compiler/,$(FBC_SOURCE_GROUPS))
FBC_HOST_DIR := $(srcdir)/compiler/platform/$(TARGET_OS)

# Driver hooks describe all compilation targets. Host policies instead describe
# the machine on which the resulting compiler runs. SOURCE_OS belongs to runtime
# layering and aliases Cygwin/illumos, so it must not select compiler policies.
FBC_PLATFORM_DIRS := $(patsubst %/,%,$(sort $(dir \
$(wildcard $(srcdir)/compiler/driver/platforms/*/fbc-platform.bi))))
FBC_BACKEND_PLATFORM_DIRS := $(patsubst %/,%,$(sort $(dir \
$(wildcard $(srcdir)/compiler/backend/*/platforms/*/*.bi))))
FBC_HOST_POLICY_DIRS := $(patsubst %/,%,$(sort $(dir \
$(wildcard $(srcdir)/compiler/platform/*/*.bi))))

FBC_DIRS := $(FBC_HOST_DIR) $(FBC_GENERIC_DIRS)
# These runtime declarations are included by compiler sources, including the
# public headers' transitive includes. A changed ABI declaration must rebuild
# compiler objects just like a changed internal header.
FBC_RUNTIME_BI := $(wildcard $(addprefix $(abspath $(srcdir)/../inc)/,\
datetime.bi file.bi string.bi string-unicode.bi \
crt/mem.bi crt/string.bi crt/stddef.bi))
FBC_BI := $(sort $(foreach directory,\
$(FBC_GENERIC_DIRS) $(FBC_PLATFORM_DIRS) $(FBC_BACKEND_PLATFORM_DIRS) \
$(FBC_HOST_POLICY_DIRS),$(wildcard $(directory)/*.bi)) $(FBC_RUNTIME_BI))

FBC_SRC_GENERIC := $(sort $(foreach directory,$(FBC_GENERIC_DIRS),\
$(wildcard $(directory)/*.bas)))
FBC_SRC_TARGET := $(wildcard $(FBC_HOST_DIR)/*.bas)
FBC_BASE_TARGET := $(notdir $(FBC_SRC_TARGET))
FBC_SRC_GENERIC := $(filter-out \
$(foreach filename,$(FBC_BASE_TARGET),%/$(filename)),$(FBC_SRC_GENERIC))
FBC_SRC := $(FBC_SRC_GENERIC) $(FBC_SRC_TARGET)

# Host filesystem structures stay in C rather than duplicating stat and
# Windows file-identity layouts in BASIC declarations.
FBC_C_SRC := $(wildcard $(srcdir)/compiler/tooling/semantic-output.c)

# Both ordinary objects and emitted bootstrap sources are named by basename.
# Silently keeping the first duplicate would compile a different module after
# a directory move, so fail here instead of relying on VPATH order.
FBC_DUPLICATE_BASENAMES := $(strip $(foreach filename,\
$(sort $(notdir $(FBC_SRC))),\
$(if $(word 2,$(filter %/$(filename),$(FBC_SRC))),$(filename))))
ifneq ($(FBC_DUPLICATE_BASENAMES),)
$(error Duplicate compiler source basenames: $(FBC_DUPLICATE_BASENAMES))
endif

FBC_OBJNAMES := $(patsubst %.bas,%.o,$(notdir $(FBC_SRC))) $(patsubst %.c,%.o,$(notdir $(FBC_C_SRC)))
ifneq ($(filter $(patsubst %.c,%.o,$(notdir $(FBC_C_SRC))),$(patsubst %.bas,%.o,$(notdir $(FBC_SRC)))),)
$(error Compiler BASIC and C sources share an object basename)
endif
FBC_OBJS := $(addprefix $(fbcobjdir)/,$(FBC_OBJNAMES))
FBC_JS_OBJS := $(addprefix $(fbcjsobjdir)/,$(FBC_OBJNAMES))
FBC_ANDROID_OBJS := $(addprefix $(fbcandroidobjdir)/,$(FBC_OBJNAMES))
FBC_WII_OBJS := $(addprefix $(fbcwiiobjdir)/,$(FBC_OBJNAMES))

##############################################################################
# end of compiler-sources.mk
##############################################################################

#!/usr/bin/make -f
# Project: FreeBASIC documentation tools
# File: GNUmakefile
# Purpose: Coordinate the documentation tool subdirectories.
# Responsibilities: Run each child once and forward optional feature switches.
# This file intentionally does NOT duplicate child source dependency graphs.
#
# top level GNUmakefile that will run other makefiles for documentation
# and wiki tools
#   ./libfbdoc/GNUmakefile
#   ./fbdoc/GNUmakefile
#   ./fbhelp/GNUmakefile
#   ./fbchkdoc/GNUmakefile
#
# define's that can override build options
#
#   'HAVE_MYSQL=1' to indicate that mysql library is available
#   'HAVE_ASPELL=1' to indicate that spell check library is available
#   'USE_PCRE_STATIC=1' to indicate we should link with static version of libpcre
#   'FBC=d:/path/fbc to' to override path to execute fbc compiler 

include common.mk

ifndef MAKE
    MAKE := make
endif

# when we build in sub directories, make sure we pass on the options
MAKEOPTS :=
ifdef HAVE_MYSQL
    MAKEOPTS += HAVE_MYSQL=1
endif 
ifdef HAVE_ASPELL
    MAKEOPTS += HAVE_ASPELL=1
endif 
ifdef USE_PCRE_STATIC
    MAKEOPTS += USE_PCRE_STATIC=1
endif 

FBC ?= fbc

FBDOC_BINS := fbdoc/fbdoc$(EXEEXT)

LIBFBDOC_BINS := libfbdoc/libfbdoc.a

FBCHKDOC_BINS := fbchkdoc/getindex$(EXEEXT)  
FBCHKDOC_BINS += fbchkdoc/getpage$(EXEEXT)
FBCHKDOC_BINS += fbchkdoc/putpage$(EXEEXT)
FBCHKDOC_BINS += fbchkdoc/chkdocs$(EXEEXT)
FBCHKDOC_BINS += fbchkdoc/replace$(EXEEXT)
FBCHKDOC_BINS += fbchkdoc/rebuild$(EXEEXT)
FBCHKDOC_BINS += fbchkdoc/mkprntoc$(EXEEXT)
FBCHKDOC_BINS += fbchkdoc/mkerrlst$(EXEEXT)
FBCHKDOC_BINS += fbchkdoc/delextra$(EXEEXT)
FBCHKDOC_BINS += fbchkdoc/mkimglst$(EXEEXT)
FBCHKDOC_BINS += fbchkdoc/getimage$(EXEEXT)

ifdef HAVE_ASPELL
FBCHKDOC_BINS += fbchkdoc/spellit$(EXEEXT)
FBCHKDOC_BINS += fbchkdoc/spell$(EXEEXT)
endif

FBCHKDOC_BINS += fbchkdoc/samps$(EXEEXT)
FBCHKDOC_BINS += fbchkdoc/pretty$(EXEEXT)
FBCHKDOC_BINS += fbchkdoc/insert$(EXEEXT)

MAKEFBHELP_BINS := makefbhelp/makefbhelp$(EXEEXT) 

.PHONY: all fbdoc-tools libfbdoc-tools fbchkdoc-tools makefbhelp-tools
all: fbdoc-tools fbchkdoc-tools makefbhelp-tools

# Each child owns its incremental graph. A recipe attached separately to every
# child binary would start competing sub-makes in the same directory under -j.
fbdoc-tools: libfbdoc-tools
	cd fbdoc && $(MAKE) FBC="$(FBC)" $(MAKEOPTS)

libfbdoc-tools:
	cd libfbdoc && $(MAKE) FBC="$(FBC)" $(MAKEOPTS)

fbchkdoc-tools: libfbdoc-tools
	cd fbchkdoc && $(MAKE) FBC="$(FBC)" $(MAKEOPTS)

makefbhelp-tools:
	cd makefbhelp && $(MAKE) FBC="$(FBC)" $(MAKEOPTS)

$(FBDOC_BINS): | fbdoc-tools
	@test -f "$@"
$(LIBFBDOC_BINS): | libfbdoc-tools
	@test -f "$@"
$(FBCHKDOC_BINS): | fbchkdoc-tools
	@test -f "$@"
$(MAKEFBHELP_BINS): | makefbhelp-tools
	@test -f "$@"

.PHONY: clean
clean:
	cd fbdoc && $(MAKE) clean HAVE_ASPELL=1 HAVE_MYSQL=1
	cd fbchkdoc && $(MAKE) clean HAVE_ASPELL=1 HAVE_MYSQL=1
	cd makefbhelp && $(MAKE) clean HAVE_ASPELL=1 HAVE_MYSQL=1
	cd libfbdoc && $(MAKE) clean HAVE_ASPELL=1 HAVE_MYSQL=1

# end of GNUmakefile

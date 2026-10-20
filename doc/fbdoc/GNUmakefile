#!/usr/bin/make -f
# Project: FreeBASIC documentation tools
# File: GNUmakefile
# Purpose: Build the manual generator.
# Responsibilities: Link fbdoc with the shared documentation library.
# This file intentionally does NOT generate manual packages.
#
# makes the wiki tools and documentation support library
#
# define's that can override build options
#
#   'HAVE_MYSQL=1' to indicate that mysql library is available
#   'USE_PCRE_STATIC=1' to indicate we should link with static version of libpcre
#   'FBC=d:/path/fbc to' to override path to execute fbc compiler 


include ../common.mk

FBDOC := fbdoc$(EXEEXT)

ifndef MAKE
    MAKE := make
else
    export MAKE
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

FBFLAGS += -exx -g -i ../libfbdoc
ifdef HAVE_MYSQL
    FBFLAGS += -d HAVE_MYSQL -l mysql
endif
FBFLAGS += -p ../libfbdoc -l fbdoc

$(FBDOC): fbdoc.bas ../libfbdoc/libfbdoc.a
	$(FBC) $(FBFLAGS) $< -x $@

../libfbdoc/libfbdoc.a:
	cd ../libfbdoc && $(MAKE) FBC="$(FBC)" $(MAKEOPTS)

.PHONY: clean
clean:
	-rm -f $(FBDOC)

# end of GNUmakefile

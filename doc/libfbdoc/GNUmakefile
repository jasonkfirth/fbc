#!/usr/bin/make -f
# Project: FreeBASIC documentation tools
# File: GNUmakefile
# Purpose: Build the shared wiki and manual support library.
# Responsibilities: Select optional SQL modules and compile the library.
# This file intentionally does NOT build documentation applications.
#
# define's that can override build options
#
#   'HAVE_MYSQL=1' to indicate that mysql library is available
#   'USE_PCRE_STATIC=1' to indicate we should link with static version of libpcre
#   'FBC=d:/path/fbc to' to override path to execute fbc compiler 
#

include ../common.mk

FBFLAGS := -exx -g

INC := $(wildcard *.bi)
SRC := $(wildcard *.bas)

# if we don't have MYSQL libraries available, then
# remove modules from the source list that require mysql
ifndef HAVE_MYSQL
    SRC := $(filter-out CWikiConSql.bas,$(SRC)) 
    SRC := $(filter-out fbdoc_loader_sql.bas,$(SRC))
    MYSQL_SRC := CWikiConSql.bas fbdoc_loader_sql.bas
endif

OBJ := $(patsubst %.bas,%.o,$(SRC))
MYSQL_OBJ := $(patsubst %.bas,%.o,$(MYSQL_SRC)) 

# defining USE_PCRE_STATIC will cause PCRE_STATIC to be defined in sources 
ifdef USE_PCRE_STATIC
    #
    # FBFLAGS += -d USE_PCRE_STATIC
    #
    # CRegEx.bas will also accept USE_PCRE_STATIC, but we can simply
    # define PCRE_STATIC here and ensure that it is defined throughout the
    # sources.
    #   
    FBFLAGS += -d PCRE_STATIC=1
endif

libfbdoc.a: $(OBJ)
	$(FBC) -lib $^ -x $@

%.o: %.bas $(INC)
	$(FBC) $(FBFLAGS) -c $< -o $@

.PHONY: clean
clean:
	-rm -f $(OBJ) $(MYSQL_OBJ) libfbdoc.a

# end of GNUmakefile

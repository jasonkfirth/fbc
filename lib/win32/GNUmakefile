# Project: FreeBASIC Windows import libraries
# File: GNUmakefile
# Purpose: Generate import archives from the maintained DEF files.
# Responsibilities: Preserve library-specific dlltool options and cleanup aliases.
# This file intentionally does NOT compile runtime or compiler sources.

DLLTOOL := dlltool

libs := $(wildcard *.def)
libs := $(patsubst %.def,lib%.dll.a,$(libs))

.PHONY: all clean
all: $(libs)

# Some libraries need extra dlltool flags
libfmod.dll.a:         DTFLAGS := --add-stdcall-underscore
libjvm.dll.a:          DTFLAGS := --kill-at
libmySQL.dll.a:        DTFLAGS := --kill-at
libwx-c-0-9-0-2.dll.a: DTFLAGS := --add-stdcall-underscore

$(libs): lib%.dll.a: %.def
	$(DLLTOOL) -d $< -l $@ $(DTFLAGS)

clean:
	rm -f $(libs)

# end of GNUmakefile

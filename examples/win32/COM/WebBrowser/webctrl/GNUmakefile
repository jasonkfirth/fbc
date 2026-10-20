##########################
# Project: FreeBASIC COM examples
# File: GNUmakefile
# Purpose: Build the WebBrowser helper archive.
# Responsibilities: Track implementation and public interface dependencies.
# This file intentionally does NOT build the example application.

LIBNAME := webctrl
LIBDEST := ../

SRCS := webctrl.bas CBrowser.bas CClientSite.bas CInPlaceSite.bas CInPlaceFrame.bas
OBJS := $(patsubst %.bas,obj/%.o,$(SRCS))
HEADERS := $(wildcard *.bi ../*.bi)

##########################

FBC ?= fbc
FBCFLAGS :=
MKDIR_P ?= mkdir -p
RM ?= rm -f
.DEFAULT_GOAL := all

ifdef DEBUG
FBCFLAGS += -g
endif

LIBRARY := $(LIBDEST)lib$(LIBNAME).a

.SUFFIXES:
.SUFFIXES: .bas

VPATH = .

obj/%.o : %.bas $(HEADERS) | obj
	$(FBC) $(FBCFLAGS) -c $< -o $@

obj:
	$(MKDIR_P) obj

##########################

all: $(LIBRARY)

$(LIBRARY): $(OBJS)
	$(FBC) $(FBCFLAGS) -lib -x $@ $(OBJS)

.PHONY : clean
clean:
	-$(RM) $(OBJS) $(LIBRARY)

# end of GNUmakefile

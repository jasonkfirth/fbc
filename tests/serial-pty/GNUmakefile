# Project: FreeBASIC runtime serial integration tests
# ---------------------------------------------------
#
# File: GNUmakefile
#
# Purpose:
#
#     Build and run the POSIX OPEN COM pseudo-terminal integration test.
#
# Responsibilities:
#
#     - build the FreeBASIC testee against a selected runtime
#     - build the native pseudo-terminal controller
#     - run the paired integration test and remove generated executables
#
# This file intentionally does NOT contain:
#
#     - serial backend implementation details
#     - physical hardware tests

FBC ?= fbc
CXX ?= g++
RM ?= rm -f

FBFLAGS ?= -w all -exx
CXXFLAGS ?= -O2 -g

ALL_BINARIES := tester testee-open-com

.PHONY: all clean tests

all: $(ALL_BINARIES)

tester: tester.cpp
	$(CXX) $(CXXFLAGS) -Wall -Wextra -Werror $< -o $@

testee-open-com: testee-open-com.bas
	$(FBC) $(FBFLAGS) $< -x $@

tests: all
	./tester ./testee-open-com

clean:
	$(RM) $(ALL_BINARIES)

# end of GNUmakefile

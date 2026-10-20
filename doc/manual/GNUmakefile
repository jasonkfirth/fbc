# Project: FreeBASIC manual
# File: GNUmakefile
# Purpose: Generate and package each supported manual format.
# Responsibilities: Order shared HTML generation and verify fresh help output.
# This file intentionally does NOT build the documentation tools.
include ../common.mk
include ../../mk/version.mk
.DEFAULT_GOAL := all

HHC := hhc
FBDOC := ../fbdoc/fbdoc$(EXEEXT)
MAKEFBHELP := ../makefbhelp/makefbhelp$(EXEEXT)

# Nice names for the resulting .zip's and the important files
TITLE       = FB-manual-$(FBVERSION)
CHM         = $(TITLE).chm
CHMZIP      = $(TITLE)-chm.zip
FBHELP      = fbhelp.daz
FBHELPZIP   = $(TITLE)-fbhelp.zip
HTMLZIP     = $(TITLE)-html.zip
TXT         = $(TITLE).txt
TXTZIP      = $(TITLE)-txt.zip
WAKKAZIP    = $(TITLE)-wakka.zip

ifdef QUIET
  HIDE := > /dev/null
endif

# File dependencies detect edits; the cache directory detects page additions
# and removals, which otherwise leave an existing manual looking up to date.
MANUAL_INPUTS := $(wildcard cache cache/*.wakka)
html/fbdoc.hhp fbhelp/DocToc.txt txt/fbdoc.txt $(WAKKAZIP): $(MANUAL_INPUTS)

################################################################################

.PHONY: all refresh
.DELETE_ON_ERROR:
all: $(CHMZIP) $(FBHELPZIP) $(HTMLZIP) $(TXTZIP) $(WAKKAZIP)

################################################################################
# chm

html/fbdoc.hhp:
	rm -f html/*.html
	$(FBDOC) -chm $(HIDE)
	@test -f "$@" && test -f html/DocToc.html

html/fbdoc.chm: html/fbdoc.hhp
	rm -f "$@"
	-$(HHC) $< $(HIDE)
	@test -s "$@"

$(CHM): html/fbdoc.chm
	cp $< $@

$(CHMZIP): $(CHM)
	zip -q -9 $@ $<

################################################################################
# fbhelp

fbhelp/DocToc.txt:
	$(FBDOC) -fbhelp $(HIDE)

fbhelp/fbhelp.dat: fbhelp/DocToc.txt
	$(MAKEFBHELP)

$(FBHELP): fbhelp/fbhelp.dat
	gzip -c $< > $@

$(FBHELPZIP): $(FBHELP)
	zip -q -9 $@ $<

################################################################################
# html

# Both HTML entry points are produced by the same invocation. Order requests
# through the help project first so parallel manual formats cannot overwrite
# one another's generated files. Regenerate if just the table was removed.
html/DocToc.html: html/fbdoc.hhp
	@if [ ! -f "$@" ]; then $(FBDOC) -chm $(HIDE); fi
	@test -f "$@"

html/00index.html: html/DocToc.html
	cp $< $@

$(HTMLZIP): html/00index.html
	rm -f "$@"
	cd html && zip -q -r -9 ../$@ *.html style.css images

################################################################################
# txt

txt/fbdoc.txt:
	$(FBDOC) -txt $(HIDE)

$(TXT): txt/fbdoc.txt
	cp $< $@

$(TXTZIP): $(TXT)
	zip -q -9 $@ $<

################################################################################
# wakka

$(WAKKAZIP):
	rm -f "$@"
	cd cache && zip -q -9 ../$@ *.wakka

################################################################################

refresh:
	rm -rf cache
	mkdir cache
	cd ../fbchkdoc && ./getindex -web && ./getpage -auto -web @PageIndex.txt

clean:
	-rm -f html/*.html
	-rm -f html/fbdoc.hhc html/fbdoc.hhk html/fbdoc.hhp html/fbdoc.chm $(CHM)
	-rm -f fbhelp/*.txt fbhelp/fbhelp.dat $(FBHELP)
	-rm -f txt/*.txt $(TXT)
	-rm -f $(CHMZIP) $(FBHELPZIP) $(HTMLZIP) $(TXTZIP) $(WAKKAZIP)

.PHONY: clean

# end of GNUmakefile

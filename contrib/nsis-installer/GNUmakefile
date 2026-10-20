# Project: FreeBASIC Windows installer
# File: GNUmakefile
# Purpose: Build an NSIS installer from the prepared Windows ZIP.
# Responsibilities: Read the release version and generate the NSIS payload.
# This file intentionally does NOT build or publish the compiler distribution.
include ../../mk/version.mk

FBTITLE := FreeBASIC-$(FBVERSION)-win32
MAKENSIS := makensis

.SUFFIXES:

$(FBTITLE).exe: makescript.bas template.nsi $(FBTITLE).zip \
                  open-console.bas fblogo-header.bmp fblogo.ico
	rm -rf $(FBTITLE)
	unzip -q -o $(FBTITLE).zip
	$(FBTITLE)/fbc.exe -g -exx -d FBVERSION=$(FBVERSION) makescript.bas
	find $(FBTITLE) -type f | LC_ALL=C sort | sed -e 's,^$(FBTITLE)/,,g' > files.lst
	unix2dos files.lst
	./makescript.exe files.lst template.nsi installer.nsi
	rm files.lst
	$(FBTITLE)/fbc.exe open-console.bas
	cp installer.nsi open-console.exe fblogo.ico $(FBTITLE)
	$(MAKENSIS) -V2 $(FBTITLE)/installer.nsi
	rm -r installer.nsi open-console.exe makescript.exe $(FBTITLE)

.PHONY: clean
clean:
	-rm -f $(FBTITLE).exe

# end of GNUmakefile

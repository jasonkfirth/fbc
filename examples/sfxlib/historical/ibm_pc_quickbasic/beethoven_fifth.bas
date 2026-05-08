''
'' Microsoft QuickBASIC 4.5 Language Reference, PLAY statement example.
''
'' The body below is the manual's Beethoven's Fifth fragment.  It uses the
'' same PLAY string syntax supported by sfxlib, so no sound API translation is
'' needed.
''
'' Source:
''   https://www.pcjs.org/documents/books/mspl13/basic/qblang/
''

#lang "qb"

LISTEN$ = "T180 o2 P2 P8 L8 GGG L2 E-"
FATE$ = "P24 P8 L8 FFF L2 D"
PLAY LISTEN$ + FATE$

'' end of beethoven_fifth.bas

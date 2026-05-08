''
'' Microsoft QuickBASIC 4.5 Language Reference, PLAY scale example.
''
'' The original listing uses "X" + VARPTR$(SCALE$) to execute a PLAY substring.
'' sfxlib does not implement QuickBASIC's pointer-string indirection, so this
'' version makes the smallest source-level edit: the same SCALE$ string is
'' concatenated directly into each PLAY command.
''
'' Original shape:
''   SCALE$ = "CDEFGAB"
''   PLAY "o0 X" + VARPTR$(SCALE$)
''   FOR I = 1 TO 6
''       PLAY ">X" + VARPTR$(SCALE$)
''   NEXT
''   PLAY "o6 X" + VARPTR$(SCALE$)
''   FOR I = 1 TO 6
''       PLAY "<X" + VARPTR$(SCALE$)
''   NEXT
'
'' Source:
''   https://www.pcjs.org/documents/books/mspl13/basic/qblang/
''

#lang "qb"

SCALE$ = "CDEFGAB"
PLAY "o0 " + SCALE$
FOR I = 1 TO 6
	PLAY ">" + SCALE$
NEXT
PLAY "o6 " + SCALE$
FOR I = 1 TO 6
	PLAY "<" + SCALE$
NEXT

'' end of play_scale.bas

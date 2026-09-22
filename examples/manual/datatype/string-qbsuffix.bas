'' examples/manual/datatype/string-qbsuffix.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'STRING'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=KeyPgString
'' --------

'' QB-like $ suffixes
#lang "qb"

'' DIM based on $ suffix
'' The suffix is the subject of this QB compatibility example.
'' FB-LINTER: DISABLE-NEXT-LINE FBL-DECL-002
Dim a$
a$ = "Hello"

'' Implicit declaration based on $ suffix
b$ = ", world!"

Print a$ + b$

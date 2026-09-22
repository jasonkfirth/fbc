'' examples/manual/meta/dynamic.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic '$DYNAMIC'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=KeyPgMetaDynamic
'' --------

' compile with -lang fblite or qb

#lang "fblite"

'$DYNAMIC
Dim a(100)
'......
ReDim a(200)

'' Dynamic storage has the current ReDim bounds rather than fixed DIM bounds.
'' ReDim a(200) above establishes a nonempty zero-based array for this query.
'' FB-LINTER: DISABLE-NEXT-LINE FBL-ARR-004
Print LBound(a), UBound(a)

'' examples/manual/check/KeyPgDim_7.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'DIM'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=KeyPgDim
'' --------

'' Compile with -lang qb or fblite

'$lang: "qb"

'' A string variable using the $ type suffix
'' FB-LINTER: DISABLE-NEXT-LINE FBL-DECL-002
Dim strVariable$

'' An integer variable using the % type suffix
'' FB-LINTER: DISABLE-NEXT-LINE FBL-DECL-002
Dim intVariable%

'' A long variable using the & type suffix
'' FB-LINTER: DISABLE-NEXT-LINE FBL-DECL-002
Dim lngVariable&

'' A single precision floating point variable using the ! type suffix
'' FB-LINTER: DISABLE-NEXT-LINE FBL-DECL-002
Dim sngVariable!

'' A double precision floating point variable using the # type suffix
'' FB-LINTER: DISABLE-NEXT-LINE FBL-DECL-002
Dim dblVariable#

'' End of KeyPgDim_7.bas

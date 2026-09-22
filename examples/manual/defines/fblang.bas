'' examples/manual/defines/fblang.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic '__FB_LANG__'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=KeyPgDdfblang
'' --------

'' Set option explicit always on

#ifdef __FB_LANG__
  #if __FB_LANG__ <> "fb"
  '' This compatibility branch enables the pre-fb explicit-declaration rule.
  '' FB-LINTER: DISABLE-NEXT-LINE FBL-OPT-001
  Option Explicit
  #endif
#else
  '' Older version - before lang fb
  '' FB-LINTER: DISABLE-NEXT-LINE FBL-OPT-001 FBL-OPT-002
  Option Explicit
#endif

'' end of fblang.bas

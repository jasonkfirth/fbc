'' examples/manual/proguide/static-lib/mylib.bi
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'Static Libraries'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=ProPgStaticLibraries
'' --------

#ifndef FB_EXAMPLES_MANUAL_PROGUIDE_STATIC_LIB_MYLIB_BI
#define FB_EXAMPLES_MANUAL_PROGUIDE_STATIC_LIB_MYLIB_BI

'' mylib.bi
#inclib "mylib"
Declare Function Add2( ByVal x As Integer, ByVal y As Integer ) As Integer

#endif

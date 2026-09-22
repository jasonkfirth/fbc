'' examples/manual/proguide/shared-lib/mylib.bi
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'Shared Libraries'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=ProPgSharedLibraries
'' --------

#ifndef FB_EXAMPLES_MANUAL_PROGUIDE_SHARED_LIB_MYLIB_BI
#define FB_EXAMPLES_MANUAL_PROGUIDE_SHARED_LIB_MYLIB_BI

'' mylib.bi
#inclib "mylib"
Declare Function Add2( ByVal x As Integer, ByVal y As Integer ) As Integer

#endif

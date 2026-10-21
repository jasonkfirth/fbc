'' Project: FreeBASIC test suite
'' File: lib-declaration.bas
'' Purpose: Verify LIB on declarations and EXTERN blocks.
'' Responsibilities: Compile positive external library syntax without linking.
'' This file intentionally does NOT contain: a platform-specific library.
' TEST_MODE : COMPILE_ONLY_OK

Declare Function fbcTestsLibraryProbe Cdecl Lib "fbc-test-library" _
	Alias "fbc_tests_library_probe" (ByVal value As Long) As Long

Extern "C" Lib "fbc-test-library"
	Declare Sub fbcTestsBlockLibraryProbe Alias "fbc_tests_block_library_probe" ()
End Extern

'' end of lib-declaration.bas

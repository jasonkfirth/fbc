'' Project: FreeBASIC test suite
'' File: import-attribute.bas
'' Purpose: Verify Windows IMPORT data declaration syntax.
'' Responsibilities: Emit a referenced imported scalar and array declaration.
'' This file intentionally does NOT contain: a DLL implementation.
' TEST_MODE : COMPILE_ONLY_OK

#if defined( __FB_WIN32__ )
Extern "C"
	Extern Import fbcTestsImportedScalar Alias "fbc_tests_imported_scalar" As Long
	Extern Import fbcTestsImportedValues(0 To 1) Alias "fbc_tests_imported_values" As Long

	Private Function fbcTestsReadImportedScalar() As Long
		Return fbcTestsImportedScalar + fbcTestsImportedValues(0)
	End Function
End Extern
#endif

'' end of import-attribute.bas

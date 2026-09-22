'' FreeBASIC Compiler
'' ------------------
''
'' File: sfxlib-redefinition-errors.bas
''
'' Purpose:
''
''     Verify that duplicate declarations of sfxlib-owned names use the
''     sfxlib-specific diagnostic instead of the generic duplicate error.
''
'' Responsibilities:
''
''     - exercise variable, constant, procedure, keyword, and preprocessor
''       definition collisions with sfxlib names
''
'' This file intentionally does NOT contain:
''
''     - runtime sound playback
''     - FB_NO_SFXLIB success cases
''     - diagnostics for names owned by other runtime libraries
''
'' end of sfxlib-redefinition-errors.bas

' TEST_MODE : COMPILE_ONLY_FAIL

' These names are owned by sfxlib unless FB_NO_SFXLIB is defined.
dim note as integer
dim tempo as integer
const tone = 1
declare sub sound( byval frequency as long, byval duration as single )

' DEVICE is a sfxlib keyword rather than an identifier-style intrinsic.
dim device as integer

#define note 1

' end of sfxlib-redefinition-errors.bas

'' FreeBASIC Compiler
'' ------------------
''
'' File: gfxlib-redefinition-errors.bas
''
'' Purpose:
''
''     Verify that duplicate declarations of gfxlib-owned names use the
''     gfxlib-specific diagnostic instead of the generic duplicate error.
''
'' Responsibilities:
''
''     - exercise variable, constant, keyword, and preprocessor
''       definition collisions with gfxlib names
''
'' This file intentionally does NOT contain:
''
''     - graphics output
''     - FB_NO_GFXLIB success cases
''     - diagnostics for names owned by other runtime libraries
''
'' end of gfxlib-redefinition-errors.bas

' TEST_MODE : COMPILE_ONLY_FAIL

' These names are owned by gfxlib unless FB_NO_GFXLIB is defined.
dim pset as integer
dim screen as integer
const circle = 1
dim screenres as integer
dim bload as integer
const flip = 1

#define draw 1
#define bload 1

' end of gfxlib-redefinition-errors.bas

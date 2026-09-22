''
'' FreeBASIC Compiler
'' ------------------
''
'' File: no-sfx-intrinsic-defines.bas
''
'' Purpose:
''
''     Verify that FB_NO_SFXLIB permits preprocessor definitions to use
''     every sfxlib-specific public single-word intrinsic name.
''
'' Responsibilities:
''
''     - exercise the compiler's sfxlib intrinsic collision handling
''     - keep the no-sfxlib contract covered independently of variable tests
''
'' This file intentionally does NOT contain:
''
''     - runtime audio tests
''     - sfxlib linkage
''     - sound driver setup
''
'' end of no-sfx-intrinsic-defines.bas

' TEST_MODE : COMPILE_ONLY_OK

#define FB_NO_SFXLIB

' BEEP remains reserved because the always-enabled system runtime owns it too.
#define music 1
#define sfx 2
#define midi 3
#define device 4
#define capture 5
#define sound 6
#define noise 7
#define play 8
#define tempo 9
#define channel 10
#define octave 11
#define voice 12
#define vol 13
#define volume 14
#define balance 15
#define pan 16
#define note 17
#define wave 18
#define envelope 19
#define instrument 20
#define tone 21

' end of no-sfx-intrinsic-defines.bas

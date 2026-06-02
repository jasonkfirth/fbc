' TEST_MODE : COMPILE_AND_RUN_OK

''
'' FreeBASIC command sweep tests
'' -----------------------------
''
'' File: gfxlib-command-sweep.bas
''
'' Purpose:
''
''     Compile and run the QB-only gfxlib calls that cannot appear in
''     the -lang fb command sweep.
''
'' Responsibilities:
''
''     - exercise SCREEN mode syntax accepted only by the QB parser path
''     - exercise the stick and strig intrinsics registered as QB-only
''     - keep the run bounded and non-interactive
''
'' This file intentionally does NOT contain:
''
''     - -lang fb graphics command coverage
''     - direct hardware port I/O
''     - platform build orchestration
''

screen 13

dim as long stick_result
dim as long strig_result

stick_result = stick( 0 )
strig_result = strig( 0 )

pset (1, 1), 4
line (2, 2)-(10, 10), 2, b
circle (20, 20), 5, 3
paint (20, 20), 1, 3
draw "BM 4,30 C2 R5 D5 L5 U5"
screen , , 0, 0

screen 0

end 0

'' end of gfxlib-command-sweep.bas

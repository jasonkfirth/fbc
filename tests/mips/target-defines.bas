'' FreeBASIC MIPS compiler target smoke test
'' -----------------------------------------
''
'' File: tests/mips/target-defines.bas
''
'' Purpose:
''
''     Verify the common compile-time contract shared by every MIPS target.
''
'' Responsibilities:
''
''     - require the generic MIPS architecture define
''     - require the width-specific MIPS define
''     - permit the build harness to exercise both endian modes
''
'' This file intentionally does NOT contain:
''
''     - Linux runtime behavior checks
''     - emulator orchestration
''     - ABI-specific linker checks

#if not defined(__FB_MIPS__)
	#error expected generic MIPS target define
#endif

#if defined(__FB_64BIT__)
	#if not defined(__FB_MIPS64__)
		#error expected MIPS64 target define
	#endif
	#if defined(__FB_MIPS32__)
		#error unexpected MIPS32 target define
	#endif
#else
	#if not defined(__FB_MIPS32__)
		#error expected MIPS32 target define
	#endif
	#if defined(__FB_MIPS64__)
		#error unexpected MIPS64 target define
	#endif
#endif

print "mips target defines ok"

'' end of tests/mips/target-defines.bas

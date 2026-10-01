' TEST_MODE : COMPILE_ONLY_OK
#if (__FB_BACKEND__ = "gcc") or (__FB_BACKEND__ = "clang")
	#cmdline "-Wc -Wno-null-dereference"
#endif

dim as integer i = *cast(integer ptr, 0)

' TEST_MODE : COMPILE_ONLY_OK
#if (__FB_BACKEND__ = "gcc") or (__FB_BACKEND__ = "clang")
	#cmdline "-Wc -Wno-null-dereference"
#endif

type T
	as integer i
end type

dim as integer i = cast(T ptr, 0)->i

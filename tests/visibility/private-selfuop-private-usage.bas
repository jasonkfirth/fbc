' TEST_MODE : COMPILE_ONLY_OK
#if (__FB_BACKEND__ = "gcc") or (__FB_BACKEND__ = "clang")
	#cmdline "-Wc -Wno-infinite-recursion"
#endif

type T
	as integer i
	declare sub test( )
	private:
	declare operator @( ) as T ptr
end type

operator T.@( ) as T ptr
	operator = @this
end operator

sub T.test( )
	dim as T x
	dim as T ptr p = @x
end sub

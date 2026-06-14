' TEST_MODE : COMPILE_ONLY_OK
#cmdline "-Wc -Wno-null-dereference"

type T
	as integer i
end type

dim as integer i = cast(T ptr, 0)->i

' TEST_MODE : COMPILE_ONLY_OK
#cmdline "-Wc -Wno-null-dereference"

dim as integer i = *cast(integer ptr, 0)

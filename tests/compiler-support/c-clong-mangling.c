/*
 * FreeBASIC C++ ABI regression, c-clong-mangling.c
 * Supply C long operations under independently specified Itanium names.
 * The native compiler owns scalar widths and the platform label prefix.
 * Application entry and FreeBASIC declarations belong to the BASIC fixture.
 */

#define FB_TEST_STRINGIZE_INNER(value) #value
#define FB_TEST_STRINGIZE(value) FB_TEST_STRINGIZE_INNER(value)

/* Itanium names encode CPPABI (6), CHECK_LONG (10) or CHECK_ULONG (11).
 * The l/m suffixes denote long and unsigned long. The native label prefix
 * supplies the platform's ordinary symbol decoration.
 */
long fb_test_cpp_long(long value)
	__asm__(FB_TEST_STRINGIZE(__USER_LABEL_PREFIX__) "_ZN6CPPABI10CHECK_LONGEl");
unsigned long fb_test_cpp_ulong(unsigned long value)
	__asm__(FB_TEST_STRINGIZE(__USER_LABEL_PREFIX__) "_ZN6CPPABI11CHECK_ULONGEm");

long fb_test_cpp_long(long value)
{
	return value + 3L;
}

unsigned long fb_test_cpp_ulong(unsigned long value)
{
	return value ^ 0x55UL;
}

/* end of c-clong-mangling.c */

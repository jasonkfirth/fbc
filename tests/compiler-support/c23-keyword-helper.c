/*
    Project: FreeBASIC C backend regression tests
    File: c23-keyword-helper.c
    Purpose: Provide external objects named after C23 keywords.
    Responsibilities: Export simple functions and initialized variables.
    This fixture does not depend on FreeBASIC runtime internals.
*/

#if defined(_WIN32) && !defined(_WIN64)
#define FB_ABI_NAME(name) "_" name
#else
#define FB_ABI_NAME(name) name
#endif

#define KEYWORD_FUNCTION(identifier, object_name, increment) \
    int identifier(int value) __asm__(FB_ABI_NAME(object_name)); \
    int identifier(int value) { return value + increment; }

KEYWORD_FUNCTION(helper_alignas, "alignas", 1)
KEYWORD_FUNCTION(helper_alignof, "alignof", 2)
KEYWORD_FUNCTION(helper_bool, "bool", 3)
KEYWORD_FUNCTION(helper_static_assert, "static_assert", 4)
KEYWORD_FUNCTION(helper_thread_local, "thread_local", 5)
KEYWORD_FUNCTION(helper_true, "true", 6)
KEYWORD_FUNCTION(helper_typeof_unqual, "typeof_unqual", 7)

int helper_false __asm__(FB_ABI_NAME("false")) = 8;
int helper_nullptr __asm__(FB_ABI_NAME("nullptr")) = 9;

/* end of c23-keyword-helper.c */

/*
    Project: FreeBASIC C backend regression tests
    File: c23-keyword-variables-helper.c
    Purpose: Define external objects whose names are C23 keywords.
    Responsibilities: Supply fixed values to a separate FreeBASIC module.
    This fixture does not depend on FreeBASIC runtime internals.
*/

#if defined(_WIN32) && !defined(_WIN64)
#define FB_ABI_NAME(name) "_" name
#else
#define FB_ABI_NAME(name) name
#endif

#define KEYWORD_VARIABLE(identifier, object_name, value) \
    int identifier __asm__(FB_ABI_NAME(object_name)) = value;

KEYWORD_VARIABLE(helper_alignas, "alignas", 1)
KEYWORD_VARIABLE(helper_alignof, "alignof", 2)
KEYWORD_VARIABLE(helper_bool, "bool", 3)
KEYWORD_VARIABLE(helper_constexpr, "constexpr", 4)
KEYWORD_VARIABLE(helper_false, "false", 5)
KEYWORD_VARIABLE(helper_nullptr, "nullptr", 6)
KEYWORD_VARIABLE(helper_static_assert, "static_assert", 7)
KEYWORD_VARIABLE(helper_thread_local, "thread_local", 8)
KEYWORD_VARIABLE(helper_true, "true", 9)
KEYWORD_VARIABLE(helper_typeof_unqual, "typeof_unqual", 10)

/* end of c23-keyword-variables-helper.c */

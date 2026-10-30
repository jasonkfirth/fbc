/*
    Project: FreeBASIC C backend regression tests
    File: c-readonly-methods-caller.c
    Purpose: Call the original public method ABI from native C.
    Responsibilities: Check object layout, constructor state, method statics
    and a mutable input through the original callback type.
    This file intentionally does NOT contain compiler-specific warning controls.
*/

/* Match the generated tag and member names across the C translation units. */
struct fb_type_6READER {
    int VALUE;
};

_Static_assert(sizeof(int) == 4, "BASIC LONG must match the native reader");
_Static_assert(sizeof(struct fb_type_6READER) == 4, "Reader has one LONG and no vptr");

/* CDecl method aliases retain their original C++ style object names. */
#if defined(_WIN32) && defined(__i386__)
#define READER_ABI_PREFIX "_"
#else
#define READER_ABI_PREFIX ""
#endif
extern void ro_method_ctor(struct fb_type_6READER *, char *)
    __asm__(READER_ABI_PREFIX "_ZN6READER14ro_method_ctorEPc");
extern int ro_method_read(struct fb_type_6READER *, char *)
    __asm__(READER_ABI_PREFIX "_ZN6READER14ro_method_readEPc");
extern void ro_method_mutate(struct fb_type_6READER *, char *)
    __asm__(READER_ABI_PREFIX "_ZN6READER16ro_method_mutateEPc");
int check_native_methods(struct fb_type_6READER *);

int check_native_methods(struct fb_type_6READER *item)
{
    char text[] = "native";
    int (*callback)(struct fb_type_6READER *, char *) = ro_method_read;

    ro_method_ctor(item, text);
    if (item->VALUE != 6)
        return 1;
    if (callback(item, text) != 2)
        return 2;
    ro_method_mutate(item, text);
    if (text[0] != 'Z' || item->VALUE != 7)
        return 3;
    return 0;
}

/* end of c-readonly-methods-caller.c */

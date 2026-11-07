/*
    Project: FreeBASIC C backend regression tests
    File: c23-keyword-defined-helper.c
    Purpose: Access a FreeBASIC object named after a C23 keyword.
    Responsibilities: Read and update the separately defined object.
    This fixture does not provide the object storage.
*/

#if defined(_WIN32) && !defined(_WIN64)
#define FB_ABI_NAME(name) "_" name
#else
#define FB_ABI_NAME(name) name
#endif

extern int c_keyword_value __asm__(FB_ABI_NAME("thread_local"));
int c_keyword_read(void);
void c_keyword_write(int value);

int c_keyword_read(void) { return c_keyword_value; }
void c_keyword_write(int value) { c_keyword_value = value; }

/* end of c23-keyword-defined-helper.c */

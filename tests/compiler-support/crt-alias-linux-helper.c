/*
    Project: FreeBASIC compiler regression tests
    File: crt-alias-linux-helper.c
    Purpose: Supply CRT-style symbols for the Linux object-name check.
    Responsibilities: Return distinct values from four external functions.
    This fixture does not parse strings.
*/

long long narrow_signed(const char *, char **, int) __asm__("_strtoi64");
unsigned long long narrow_unsigned(const char *, char **, int) __asm__("_strtoui64");
long long wide_signed(const int *, int **, int) __asm__("_wcstoi64");
unsigned long long wide_unsigned(const int *, int **, int) __asm__("_wcstoui64");

long long narrow_signed(const char *text, char **end, int radix)
{
    (void)text;
    (void)end;
    (void)radix;
    return -101;
}

unsigned long long narrow_unsigned(const char *text, char **end, int radix)
{
    (void)text;
    (void)end;
    (void)radix;
    return 102;
}

long long wide_signed(const int *text, int **end, int radix)
{
    (void)text;
    (void)end;
    (void)radix;
    return -103;
}

unsigned long long wide_unsigned(const int *text, int **end, int radix)
{
    (void)text;
    (void)end;
    (void)radix;
    return 104;
}

/* end of crt-alias-linux-helper.c */

/*
 * Project: FreeBASIC runtime regression tests
 * File: longdouble-string-locale.c
 * Purpose: Verify decimal parsing without narrowing native long double.
 * Responsibilities: Exact values, extended precision and numeric-locale restoration.
 * This file does not implement parsing or change another process's locale.
 */
#include <assert.h>
#include <float.h>
#include <locale.h>
#include <stdio.h>
#include <string.h>

void fb_LongDoubleFromString(void *destination, const char *text,
                            unsigned int storage_size);

int main(void)
{
    const char *locales[] = {"fr-FR", "French_France.1252", "fr_FR.UTF-8", "de_DE.UTF-8"};
    long double value;
    char numeric_name[256];
    int non_c_locale = 0;
#if defined(__MINGW32__) && defined(_UCRT)
    int original_mode = _configthreadlocale(0);
#endif
    for (size_t i = 0; i < sizeof(locales) / sizeof(locales[0]); ++i) {
        if (setlocale(LC_NUMERIC, locales[i]) != NULL &&
            strcmp(localeconv()->decimal_point, ".") != 0) {
            non_c_locale = 1;
            break;
        }
    }
    const char *numeric = setlocale(LC_NUMERIC, NULL);
    assert(numeric != NULL && strlen(numeric) < sizeof(numeric_name));
    strcpy(numeric_name, numeric);
    fb_LongDoubleFromString(&value, "3.75", sizeof(value));
    assert(value == 3.75L);
#if LDBL_MANT_DIG > DBL_MANT_DIG
    fb_LongDoubleFromString(&value,
        "1.000000000000000000108420217248550443400745280086994171142578125",
        sizeof(value));
    assert(value > 1.0L && value == 1.0L + 0x1p-63L);
#endif
    assert(strcmp(setlocale(LC_NUMERIC, NULL), numeric_name) == 0);
#if defined(__MINGW32__) && defined(_UCRT)
    assert(_configthreadlocale(0) == original_mode);
    assert(non_c_locale);
#endif
    printf("LONGDOUBLE_STRING_LOCALE=PASS non_c=%d precision=%d\n",
           non_c_locale, LDBL_MANT_DIG);
    return 0;
}
/* end of longdouble-string-locale.c */

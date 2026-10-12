/*
    Project: FreeBASIC native regex qualification
    File: regex-abi.c
    Purpose: Report PCRE/TRE ABI facts and exercise real allocator exports.
    Responsibilities: Native SDK layouts, three PCRE widths and proper cleanup.
    This file contains no replacement regex engine or wrapper implementation.
*/
#include <stddef.h>
#include <stdio.h>
#include <pcre.h>
#include <tre/regex.h>

int main(void)
{
    const char *error = NULL;
    int offset = 0;
    int vector[9];
    pcre *code;
    pcre16 *code16;
    pcre32 *code32;
    const unsigned short text16[] = {'f','o','o',0};
    const unsigned int text32[] = {'f','o','o',0};
    printf("PCRE %zu %zu %zu %zu %zu %zu %zu %zu %zu %zu\n", sizeof(size_t), sizeof(unsigned long),
           sizeof(pcre_extra), offsetof(pcre_extra, study_data), offsetof(pcre_extra, match_limit),
           offsetof(pcre_extra, tables), offsetof(pcre_extra, mark), sizeof(pcre_callout_block),
           offsetof(pcre_callout_block, subject), offsetof(pcre_callout_block, mark));
    printf("TRE %zu %zu %zu %zu %zu %zu %zu\n", sizeof(regex_t), sizeof(regoff_t), sizeof(regmatch_t),
           offsetof(regex_t, re_nsub), offsetof(regex_t, value), sizeof(regaparams_t), sizeof(regamatch_t));
    printf("VERSIONS PCRE=%s TRE=%s\n", pcre_version(), tre_version());
    code = pcre_compile("foo", 0, &error, &offset, NULL);
    if (!code || pcre_exec(code, NULL, "foo", 3, 0, 0, vector, 9) != 1)
        return 1;
    pcre_free(code);
    code16 = pcre16_compile(text16, 0, &error, &offset, NULL);
    if (!code16 || pcre16_exec(code16, NULL, text16, 3, 0, 0, vector, 9) != 1)
        return 1;
    pcre16_free(code16);
    code32 = pcre32_compile(text32, 0, &error, &offset, NULL);
    if (!code32 || pcre32_exec(code32, NULL, text32, 3, 0, 0, vector, 9) != 1)
        return 1;
    pcre32_free(code32);
    printf("PCRE_NATIVE_WIDTHS=PASS 8 16 32\n");
    return 0;
}
/* end of regex-abi.c */

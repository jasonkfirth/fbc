/*
 * Project: FreeBASIC C backend regression tests
 * File: c-packed-descriptors.c
 * Purpose: Count recoverable alignment reports during ordinary descriptor use.
 * Responsibilities: Implement Clang's no-argument minimal-runtime test hook.
 * This file intentionally does NOT change descriptor values or release storage.
 */
void __ubsan_handle_type_mismatch_minimal( void );
unsigned int descriptor_alignment_errors( void );

static unsigned int errors;

void __ubsan_handle_type_mismatch_minimal( void )
{
    ++errors;
}

unsigned int descriptor_alignment_errors( void )
{
    return errors;
}

/* end of c-packed-descriptors.c */

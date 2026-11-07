/*
    Project: FreeBASIC C backend regression tests
    File: tls-keyword-c-helper.c
    Purpose: Read a keyword-named TLS object from separately compiled C.
    Responsibilities: Optionally provide the storage and its BASIC observer.
    This fixture does not create threads or depend on FreeBASIC runtime internals.

    Compile as GNU11, where bool is an ordinary identifier. This checks the
    exact ABI name independently of the emitter's C23 identifier encoding.
*/

#ifdef TLS_C_PROVIDER
__thread int bool = 17;
int tls_keyword_read(void);
int tls_keyword_read(void) { return bool; }
#else
extern __thread int bool;
#endif

int tls_keyword_c_read(void);
int tls_keyword_c_read(void) { return bool; }

/* end of tls-keyword-c-helper.c */

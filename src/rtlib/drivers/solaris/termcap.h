/*
 * Project: FreeBASIC runtime library
 * File: drivers/solaris/termcap.h
 * Purpose: Provide termcap runtime support.
 * Responsibilities: Target-specific runtime support for solaris.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#ifndef FB_SOLARIS_TERMCAP_SHIM_H
#define FB_SOLARIS_TERMCAP_SHIM_H

#ifdef __cplusplus
extern "C" {
#endif

int tgetent(char *bp, const char *name);
char *tgetstr(const char *id, char **area);
int tgetflag(const char *id);
int tgetnum(const char *id);

char *tgoto(const char *cap, int col, int row);
int tputs(const char *str, int affcnt, int (*putc_fn)(int));

/* globals expected by termcap users */
extern char PC;
extern char *BC;
extern char *UP;
extern short ospeed;

#ifdef __cplusplus
}
#endif

#endif

/* end of drivers/solaris/termcap.h */

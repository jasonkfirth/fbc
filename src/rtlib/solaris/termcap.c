/*
 * Solaris termcap shim backed by terminfo
 *
 * No <termcap.h>, <curses.h>, or <term.h>
 * Avoids Solaris header conflicts entirely
 */

#include "termcap.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* --------------------------------------------------------- */
/* Minimal terminfo declarations (no headers required)        */
/* --------------------------------------------------------- */

extern int setupterm(char *term, int fd, int *errret);
extern char *tigetstr(char *capname);
extern int tigetflag(char *capname);
extern int tigetnum(char *capname);
extern char *tparm(char *str, ...);
extern int baudrate(void);

/* --------------------------------------------------------- */
/* termcap globals                                           */
/* --------------------------------------------------------- */

char PC = 0;
char *BC = NULL;
char *UP = NULL;
short ospeed = 0;

/* --------------------------------------------------------- */
/* constants                                                 */
/* --------------------------------------------------------- */

#define OK 0
#define ERR -1

/* --------------------------------------------------------- */
/* termcap -> terminfo capability mapping                    */
/* --------------------------------------------------------- */

static const char *map_cap(const char *cap)
{
    if (!cap) return NULL;

    if (strcmp(cap, "cm") == 0) return "cup";
    if (strcmp(cap, "ho") == 0) return "home";
    if (strcmp(cap, "cl") == 0) return "clear";
    if (strcmp(cap, "ce") == 0) return "el";
    if (strcmp(cap, "cs") == 0) return "csr";
    if (strcmp(cap, "SF") == 0) return "ind";
    if (strcmp(cap, "ve") == 0) return "cnorm";
    if (strcmp(cap, "vi") == 0) return "civis";
    if (strcmp(cap, "bl") == 0) return "bel";
    if (strcmp(cap, "AF") == 0) return "setaf";
    if (strcmp(cap, "AB") == 0) return "setab";
    if (strcmp(cap, "me") == 0) return "sgr0";
    if (strcmp(cap, "md") == 0) return "bold";
    if (strcmp(cap, "dc") == 0) return "dch1";
    if (strcmp(cap, "ks") == 0) return "smkx";
    if (strcmp(cap, "ke") == 0) return "rmkx";

    return NULL;
}

/* --------------------------------------------------------- */

int tgetent(char *bp, const char *name)
{
    (void)bp;

    int err = 0;

    if (setupterm((char *)name, STDOUT_FILENO, &err) != OK)
        return -1;

    ospeed = (short)baudrate();
    return 1;
}

/* --------------------------------------------------------- */

char *tgetstr(const char *id, char **area)
{
    (void)area;

    const char *ti = map_cap(id);
    if (!ti)
        return NULL;

    char *s = tigetstr((char *)ti);

    if (s == (char *)-1 || s == NULL)
        return NULL;

    return s;
}

/* --------------------------------------------------------- */

int tgetflag(const char *id)
{
    const char *ti = map_cap(id);
    if (!ti)
        return 0;

    int v = tigetflag((char *)ti);
    return (v == -1) ? 0 : v;
}

/* --------------------------------------------------------- */

int tgetnum(const char *id)
{
    const char *ti = map_cap(id);
    if (!ti)
        return -1;

    int v = tigetnum((char *)ti);
    return (v == -1) ? -1 : v;
}

/* --------------------------------------------------------- */

char *tgoto(const char *cap, int col, int row)
{
    /* termcap uses (col,row), terminfo uses (row,col) */
    return tparm((char *)cap, row, col);
}

/* --------------------------------------------------------- */

int tputs(const char *str, int affcnt, int (*putc_fn)(int))
{
    (void)affcnt;

    if (!str || !putc_fn)
        return ERR;

    while (*str) {
        putc_fn(*str++);
    }

    return OK;
}

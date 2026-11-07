'' examples/manual/prepro/variadic.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic '... (Ellipsis)'
'' Purpose: Demonstrate forwarding variadic macro arguments to C stdio.
'' Responsibilities: Match the C argument widths and preserve macro expansion.
'' This file intentionally does NOT redefine the CRT calling convention.
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=KeyPgDots
'' --------

'' Using a variadic macro to wrap a variadic function
#include "crt.bi"
#define eprintf(Format, args...) fprintf(stderr, Format, args)
'' C's %i consumes a 32-bit int even when INTEGER is 64-bit.
eprintf(!"Hello from printf: %i %s %i\n", 5L, "test", 123L)

'' LISP-like accessors allowing to modify comma-separated lists
#define car(a, b...) a
#define cdr(a, b...) b

'' end of variadic.bas

/*
    Project: FreeBASIC C backend regression controls
    File: c-private-procedures-caller.c
    Purpose: Call a public BASIC entry without a BASIC-side reference.
    Responsibilities: Verify that private-body pruning preserves public ABI roots.
    This file intentionally does NOT contain: Program startup or platform APIs.
*/
extern int r90_public_entry(int value);
int r90_native_probe(void);

int r90_native_probe(void)
{
    return r90_public_entry(31);
}

/* end of c-private-procedures-caller.c */

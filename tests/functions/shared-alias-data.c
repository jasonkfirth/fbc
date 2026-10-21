/*
    Project: FreeBASIC compiler tests

    File: shared-alias-data.c

    Purpose:

        Access one FreeBASIC shared variable through its exported C symbol.

    Responsibilities:

        • read the FreeBASIC initializer
        • write the same storage from C

    This file intentionally does NOT contain:

        • FreeBASIC compiler implementation logic
        • an alternate copy of the exported variable
*/

extern int c2fb_shared_alias_data;

int c2fb_read_shared_alias_data(void)
{
    return c2fb_shared_alias_data;
}

void c2fb_write_shared_alias_data(int value)
{
    c2fb_shared_alias_data = value;
}

/* end of shared-alias-data.c */

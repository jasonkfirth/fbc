/*
    Project: FreeBASIC backend regression tests
    File: procedure-address-identity.c
    Purpose: Supply independent native procedure and data addresses.
    Responsibilities: Define native entries and return their typed addresses.
    This file intentionally does NOT implement BASIC comparison semantics.
*/
typedef int (*Callback)(int);

int fb_alias_target(int value);
int fb_alias_other(int value);
Callback fb_alias_reference(void);
int *fb_alias_data_reference(void);
extern int fb_alias_data;

int fb_alias_data = 23;

int fb_alias_target(int value)
{
    return value + 7;
}

int fb_alias_other(int value)
{
    return value - 3;
}

Callback fb_alias_reference(void)
{
    return &fb_alias_target;
}

int *fb_alias_data_reference(void)
{
    return &fb_alias_data;
}

/* end of procedure-address-identity.c */

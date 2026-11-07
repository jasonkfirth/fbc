/*
    Project: FreeBASIC C backend regressions
    File: c-external-prototypes.c
    Purpose: Observe retained C calls and a BASIC public entry point.
    Responsibilities: Supply external targets and count unexpected dead calls.
    This file intentionally does NOT own BASIC program startup.
*/

int prototype_dead(int value);
int prototype_live(int value);
int prototype_callback(int value);
int prototype_public_target(int value);
int prototype_call_public(void);
int prototype_dead_calls(void);
extern int prototype_public_entry(int value);

static int dead_calls;

int prototype_dead(int value)
{
    ++dead_calls;
    return value;
}

int prototype_live(int value)
{
    return value + 10;
}

int prototype_callback(int value)
{
    return value + 20;
}

int prototype_public_target(int value)
{
    return value + 30;
}

int prototype_call_public(void)
{
    return prototype_public_entry(13);
}

int prototype_dead_calls(void)
{
    return dead_calls;
}

/* end of c-external-prototypes.c */

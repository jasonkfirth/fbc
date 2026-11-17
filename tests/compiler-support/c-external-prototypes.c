/*
    Project: FreeBASIC C backend regressions
    File: c-external-prototypes.c
    Purpose: Observe retained C calls and a BASIC public entry point.
    Responsibilities: Supply external targets, shared objects and dead-call counts.
    This file intentionally does NOT own BASIC program startup.
*/

int prototype_dead(int value);
int prototype_live(int value);
int prototype_callback(int value);
int prototype_public_target(int value);
int prototype_call_public(void);
int prototype_dead_calls(void);
int prototype_shared_values(void);
int prototype_same_storage(const void *left_address, const void *right_address);
extern int prototype_public_entry(int value);
extern int prototype_defined_scalar;
extern int prototype_shared_scalar;
extern int prototype_shared_array[3];
extern int *prototype_shared_pointer;

int prototype_shared_scalar = 17;
int prototype_shared_array[3] = { 19, 29, 31 };
int *prototype_shared_pointer = &prototype_shared_array[1];

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

int prototype_shared_values(void)
{
    return prototype_shared_scalar + prototype_shared_array[1] + prototype_defined_scalar;
}

int prototype_same_storage(const void *left_address, const void *right_address)
{
    return left_address == right_address;
}

/* end of c-external-prototypes.c */

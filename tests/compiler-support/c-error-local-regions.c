/*
    Project: FreeBASIC C continuation qualification
    File: c-error-local-regions.c
    Purpose: Observe a scalar whose address leaves its original BASIC expression.
    Responsibilities: Retain one borrowed slot and clear it before owner retirement.
    This file intentionally does NOT own storage or support concurrent observers.
*/
#include <stdint.h>

void region_save_slot(intptr_t *slot);
intptr_t region_read_slot(void);
void region_clear_slot(void);
intptr_t region_identity(intptr_t number);

/* The single-thread test clears this borrow before its source owner returns. */
static intptr_t *saved_slot;

void region_save_slot(intptr_t *slot)
{
    saved_slot = slot;
}

intptr_t region_read_slot(void)
{
    return saved_slot != 0 ? *saved_slot : INTPTR_MIN;
}

void region_clear_slot(void)
{
    saved_slot = 0;
}

intptr_t region_identity(intptr_t number)
{
    return number;
}

/* end of c-error-local-regions.c */

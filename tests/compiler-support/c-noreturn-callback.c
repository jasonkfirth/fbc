/*
 * Project: FreeBASIC C backend regression
 * File: c-noreturn-callback.c
 * Purpose: Observe callback identity and invoke the original terminating body.
 * Responsibilities: Supply an ordinary C callback interface at both Intel widths.
 * This file intentionally does NOT adapt callbacks or own the supplied context.
 */

typedef void (*Callback)(void *context);

int callback_addresses_equal(Callback direct, Callback stored, Callback returned);
void callback_invoke(Callback value, void *context);

int callback_addresses_equal(Callback direct, Callback stored, Callback returned)
{
	return direct != 0 && direct == stored && direct == returned;
}

void callback_invoke(Callback value, void *context)
{
	value(context);
}

/* end of c-noreturn-callback.c */

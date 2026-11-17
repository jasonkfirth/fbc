/*
 * FreeBASIC variadic ABI regression, c-va-narrow-anchor.c
 * Call the BASIC functions through their original narrow C parameter types.
 * Check integer traversal after byte, short and float named arguments.
 * va_list initialization belongs to the BASIC definitions.
 */

long long fb_test_anchor_byte(signed char anchor, ...);
long long fb_test_anchor_short(short anchor, ...);
long long fb_test_anchor_single(float anchor, ...);
int fb_test_va_anchor_calls(void);

int fb_test_va_anchor_calls(void)
{
	if (fb_test_anchor_byte(-7, 101LL) != 101LL)
		return 1;
	if (fb_test_anchor_short(-513, 202LL) != 202LL)
		return 2;
	if (fb_test_anchor_single(1.25f, 303LL) != 303LL)
		return 3;
	return 0;
}

/* end of c-va-narrow-anchor.c */

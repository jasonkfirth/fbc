/*
    Project: FreeBASIC compiler backend regression tests
    -----------------------------------------

    File: incomplete-extern.c

    Purpose:

        Provide complete C storage for an object whose FreeBASIC declaration
        intentionally knows only its address.

    Responsibilities:

        - define the opaque object and its record layout
        - return its address for an independent identity check
        - inspect the object through a pointer passed by reference

    This file intentionally does NOT contain:

        - FreeBASIC type or parser tests
*/

struct c2fb_incomplete_object_type {
	int value;
};

struct c2fb_complete_later_type {
	int value;
};

struct c2fb_incomplete_object_type c2fb_incomplete_object = { 41 };
struct c2fb_complete_later_type c2fb_complete_later = { 73 };

struct c2fb_incomplete_object_type *c2fb_incomplete_object_address(void)
{
	return &c2fb_incomplete_object;
}

int c2fb_incomplete_object_matches(
	struct c2fb_incomplete_object_type *value)
{
	return (value == &c2fb_incomplete_object) && (value->value == 41);
}

/* end of incomplete-extern.c */

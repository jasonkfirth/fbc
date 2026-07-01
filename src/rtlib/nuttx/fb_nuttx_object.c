/*
    Project: FreeBASIC runtime library
    ----------------------------------

    File: fb_nuttx_object.c

    Purpose:

        Provide the smallest root Object ABI surface needed by generated-C
        NuttX tests.

    Responsibilities:

        - export the base Object RTTI record used by generated inheritance
          metadata
        - provide the base Object constructor symbol emitted by fbc

    This file intentionally does NOT contain:

        - a full object model
        - exception handling
        - dynamic type lookup helpers beyond the root RTTI record
*/

#if !defined(FB_NUTTX_USE_GENERIC_OBJECT) || \
    (FB_NUTTX_USE_GENERIC_OBJECT == 0)

struct $8fb_RTTI$ {
    void *STDLIBVTABLE;
    char *ID;
    struct $8fb_RTTI$ *RTTIBASE;
};

struct $10fb_Object$ {
    void *VPTR$;
};

struct $8fb_RTTI$ __fb_ZTS6Object = {
    (void *)0,
    (char *)"6Object",
    (struct $8fb_RTTI$ *)0
};

void _ZN10fb_Object$C1Ev(struct $10fb_Object$ *self)
{
    /*
        The generated child constructors immediately overwrite the vptr with
        their own vtable.  The root constructor still has to leave the base
        object in a known state for tests that instantiate Object directly.
    */
    if (self != (struct $10fb_Object$ *)0)
        self->VPTR$ = (void *)0;
}

int fb_IsTypeOf(struct $10fb_Object$ *object, struct $8fb_RTTI$ *type_rtti)
{
    void **vtable;
    struct $8fb_RTTI$ *object_rtti;

    if ((object == NULL) || (object->VPTR$ == NULL) || (type_rtti == NULL))
        return 0;

    vtable = (void **)object->VPTR$;
    object_rtti = (struct $8fb_RTTI$ *)vtable[-1];

    while (object_rtti != NULL) {
        if ((object_rtti->ID != NULL) && (type_rtti->ID != NULL) &&
            (strcmp(object_rtti->ID, type_rtti->ID) == 0))
            return -1;

        object_rtti = object_rtti->RTTIBASE;
    }

    return 0;
}

#endif

/* end of fb_nuttx_object.c */

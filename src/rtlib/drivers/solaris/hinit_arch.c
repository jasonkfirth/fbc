/*
    Project: FreeBASIC runtime library
    -------------------------

    File: drivers/solaris/hinit_arch.c

    Purpose:

        Provide the default Solaris architecture initialization hook.

    Responsibilities:

        - keep common Solaris initialization independent of CPU-specific code

    This file intentionally does NOT contain:

        - x86 floating-point setup, which lives in x86/hinit_arch.c
        - platform initialization shared by all Solaris targets
*/

void fb_hSolarisInitArch( void );

void fb_hSolarisInitArch( void )
{
}

/* end of drivers/solaris/hinit_arch.c */

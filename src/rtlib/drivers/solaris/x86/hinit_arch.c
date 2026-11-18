/*
    Project: FreeBASIC runtime library
    -------------------------

    File: drivers/solaris/x86/hinit_arch.c

    Purpose:

        Set the x87 control word for 32-bit Solaris x86.

    Responsibilities:

        - select 64-bit precision
        - select round-to-nearest

    This file intentionally does NOT contain:

        - Solaris console or thread initialization
        - floating-point setup for other architectures
*/

void fb_hSolarisInitArch( void );

void fb_hSolarisInitArch( void )
{
#if defined(__GNUC__)
    unsigned int control_word;

    /* Get FPU control word */
    __asm__ __volatile__( "fstcw %0" : "=m" (control_word) : );
    /* Set 64-bit and round to nearest */
    control_word = (control_word & 0xF0FF) | 0x300;
    /* Write back FPU control word */
    __asm__ __volatile__( "fldcw %0" : : "m" (control_word) );
#endif
}

/* end of drivers/solaris/x86/hinit_arch.c */

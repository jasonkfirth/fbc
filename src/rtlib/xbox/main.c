/*
    FreeBASIC Runtime Library
    -------------------------

    File: xbox/main.c

    Purpose:

        Provide the program entry bridge expected by nxdk's C runtime.

    Responsibilities:

        - export main() for nxdk's WinMainCRTStartup path
        - forward control to the FreeBASIC Xbox startup procedure

    This file intentionally does NOT contain:

        - runtime initialization
        - console, graphics, or audio setup
        - process termination policy
*/

/*
    The historical Xbox target emits the FreeBASIC program as XBoxStartup()
    instead of main().  nxdk's CRT still enters through main(), so this small
    bridge keeps the compiler-visible Xbox ABI separate from nxdk's C startup.
*/

extern int XBoxStartup( int argc, char **argv );

int main( int argc, char **argv )
{
	return XBoxStartup( argc, argv );
}

/* end of xbox/main.c */

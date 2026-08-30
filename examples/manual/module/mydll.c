/*
    Project: FreeBASIC manual examples
    ----------------------------------

    File: examples/manual/module/mydll.c

    Purpose:

        Provide the C DLL used by the manual's IMPORT example.

    Responsibilities:

        - export one integer data symbol from a Windows DLL

    This file intentionally does NOT contain:

        - executable startup code
        - FreeBASIC declarations for importing the symbol

    See also:

        https://www.freebasic.net/wiki/wikka.php?wakka=KeyPgImport
*/

/* mydll.c :
	compile with
	  gcc -shared -Wl,--strip-all -o mydll.dll mydll.c
*/
__declspec( dllexport ) int MyDll_Data = 0x1234;

/* end of examples/manual/module/mydll.c */

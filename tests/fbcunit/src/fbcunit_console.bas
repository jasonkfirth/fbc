''  fbcunit - FreeBASIC Compiler Unit Testing Component
''	Copyright (C) 2017-2020 Jeffery R. Marshall (coder[at]execulink[dot]com)
''
''  License: GNU Lesser General Public License
''           version 2.1 (or any later version) plus
''           linking exception, see license.txt

#include once "crt/stdio.bi"
#include once "fbcunit_console.bi"

'' chng: written [jeffm]

''
sub crt_print_output _
	( _
		byref s as const string _
	)

	''
	'' The compiler's current Windows runtime uses UCRT.  Calling the legacy
	'' __iob_func stdout bridge through crt/stdio.bi makes fbcunit archives built
	'' with an older CRT fail to link.  printf uses the active CRT's normal output
	'' path without borrowing that legacy stream accessor and preserves fbcunit's
	'' no-newline contract.
	printf( "%s", strptr(s) )

end sub

#pragma once

#ifdef __FB_BOOTSTRAP_COMPAT__
'' Older source-bootstrap compilers do not understand type aliases. These
'' declarations retain the same width during the temporary compiler build.
#if defined( __FB_64BIT__ ) and (not defined( __FB_WIN32__))
	type clong as integer
	type culong as uinteger
#else
	type clong as long
	type culong as ulong
#endif
#else
#if defined( __FB_64BIT__ ) and (not defined( __FB_WIN32__))
	'' On 64bit Linux/BSD systems (but not 64bit Windows), C's long is
	'' 64bit like FB's integer.
	type clong as integer alias "long"
	type culong as uinteger alias "long"
#else
	'' On 32bit systems and 64bit Windows, C's long is 32bit like FB's long.
	'' Note: Using 32bit Long here instead of 32bit/64bit Integer, because
	'' this is also used for 64bit Windows where Integer isn't 32bit.
	type clong as long alias "long"
	type culong as ulong alias "long"
#endif
#endif

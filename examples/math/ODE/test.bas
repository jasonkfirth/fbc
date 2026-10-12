'' Project: FreeBASIC ODE examples
'' File: test.bas
'' Purpose: Create and destroy one ODE physics world.
'' Responsibilities: Match the precision of the installed library.
'' This file contains no simulation loop or rendering.

'' Unix packages and the Windows 64-bit SDK use double precision. A caller
'' with a single-precision build can explicitly select it with -d dSINGLE.
#if not defined(dSINGLE) and not defined(dDOUBLE)
	#if not defined(__FB_WIN32__) or defined(__FB_64BIT__)
		#define dDOUBLE
	#endif
#endif
#include once "ode/ode.bi"

dim as dWorldID world = dWorldCreate()

dWorldDestroy(world)

'' end of test.bas

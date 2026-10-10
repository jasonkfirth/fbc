'' Project: FreeBASIC SDL3 examples
'' File: resolve-hostnames.bas
'' Purpose: Port upstream SDL3_net-3.2.0/examples/resolve-hostnames.c.
'' Responsibilities: Demonstrate the same SDL APIs and application lifecycle.
'' This file intentionally does NOT contain: compiler or library implementations.
''
'' Translated from the upstream C example; this is an altered source version.
'' This is just for demonstration purposes! This doesn't
'' do anything as complicated as, say, the `dig` utility.
''
'' All this to say: don't use this for anything serious!

#include once "SDL3/SDL.bi"
#include once "SDL3/SDL_net.bi"
#include once "crt.bi"

declare function example_main cdecl(byval argc as long, byval argv as zstring ptr ptr) as long

function example_main cdecl(byval argc as long, byval argv as zstring ptr ptr) as long
	scope
		if (NET_Init() = 0) then
			scope
				SDL_Log_(strptr("NET_Init() failed: %s"), SDL_GetError())
				return 1
			end scope
		end if
		'' NET_SimulateAddressResolutionLoss(3000, 30);
		dim addrs as NET_Address ptr ptr = cptr(NET_Address ptr ptr, SDL_calloc(argc, sizeof(NET_Address ptr)))
		scope
			dim i as long = 1
			do while (i < argc)
				scope
					addrs[i] = NET_ResolveHostname(argv[i])
				end scope
				i += 1
			loop
		end scope
		scope
			dim i as long = 1
			do while (i < argc)
				scope
					NET_WaitUntilResolved(addrs[i], (-1))
					if (NET_GetAddressStatus(addrs[i]) = NET_FAILURE) then
						scope
							SDL_Log_(strptr("%s: [FAILED TO RESOLVE: %s]"), argv[i], SDL_GetError())
						end scope
					else
						scope
							SDL_Log_(strptr("%s: %s"), argv[i], NET_GetAddressString(addrs[i]))
						end scope
					end if
					NET_UnrefAddress(addrs[i])
				end scope
				i += 1
			loop
		end scope
		NET_Quit()
		return 0
	end scope
end function

end SDL_RunApp(__FB_ARGC__, __FB_ARGV__, @example_main, 0)

'' end of resolve-hostnames.bas

'' Project: FreeBASIC SDL3 examples
'' File: get-local-addrs.bas
'' Purpose: Port upstream SDL3_net-3.2.0/examples/get-local-addrs.c.
'' Responsibilities: Demonstrate the same SDL APIs and application lifecycle.
'' This file intentionally does NOT contain: compiler or library implementations.
''
'' Translated from the upstream C example; this is an altered source version.
'' This is just for demonstration purposes! This doesn't
'' do anything as complicated as, say, the `ifconfig` utility.
''
'' All this to say: don't use this for anything serious!

#include once "SDL3/SDL.bi"
#include once "SDL3/SDL_net.bi"
#include once "crt.bi"

declare function example_main cdecl(byval argc as long, byval argv as zstring ptr ptr) as long

function example_main cdecl(byval argc as long, byval argv as zstring ptr ptr) as long
	scope
		dim addrs as NET_Address ptr ptr = cptr(NET_Address ptr ptr, 0)
		dim num_addrs as long = 0
		dim i as long
		if (NET_Init() = 0) then
			scope
				SDL_Log_(strptr("NET_Init() failed: %s"), SDL_GetError())
				return 1
			end scope
		end if
		addrs = NET_GetLocalAddresses(@(num_addrs))
		if (addrs = cptr(NET_Address ptr ptr, (cptr(any ptr, 0)))) then
			scope
				SDL_Log_(strptr("Failed to determine local addresses: %s"), SDL_GetError())
				NET_Quit()
				return 1
			end scope
		end if
		SDL_Log_(strptr("We saw %d local addresses:"), cast(long, num_addrs))
		scope
			i = 0
			do while (i < num_addrs)
				scope
					SDL_Log_(strptr("  - %s"), NET_GetAddressString(addrs[i]))
				end scope
				i += 1
			loop
		end scope
		NET_FreeLocalAddresses(addrs)
		NET_Quit()
		return 0
	end scope
end function

end SDL_RunApp(__FB_ARGC__, __FB_ARGV__, @example_main, 0)

'' end of get-local-addrs.bas

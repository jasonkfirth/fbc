''
''
'' netdb -- OpenBSD-specific network constants
''
''
#ifndef __crt_openbsd_netdb_bi__
#define __crt_openbsd_netdb_bi__

type netent
	n_name as zstring ptr
	n_aliases as zstring ptr ptr
	n_addrtype as long
	n_net as uint32_t
end type

#endif

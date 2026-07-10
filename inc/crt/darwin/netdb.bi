''
''
'' netdb -- Darwin-specific network database declarations
''
'' NOTICE: This file is part of the FreeBASIC Compiler package and can't
''         be included in other distributions without authorization.
''
''
#ifndef __crt_darwin_netdb_bi__
#define __crt_darwin_netdb_bi__

type netent
	n_name as zstring ptr
	n_aliases as zstring ptr ptr
	n_addrtype as long
	n_net as uint32_t
end type

#endif

'' end of crt/darwin/netdb.bi

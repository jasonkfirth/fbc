/*
	Xbox TCP device

	nxdk exposes TCP/IP through lwIP with BSD socket wrappers.  Keep the
	target-specific filename so source-graph.mk still records that Xbox has
	been considered explicitly, but use the shared TCP device implementation.
*/

#include "../dev_tcp.c"

/* end of xbox/dev_tcp.c */

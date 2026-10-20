/*
 * Project: FreeBASIC runtime library
 * File: drivers/xbox/dev_tcp.c
 * Purpose: Provide dev tcp runtime support.
 * Responsibilities: Target-specific runtime support for xbox.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
/*
	Xbox TCP device

	nxdk exposes TCP/IP through lwIP with BSD socket wrappers.  Keep the
	target-specific filename so source-graph.mk still records that Xbox has
	been considered explicitly, but use the shared TCP device implementation.
*/

#include "../../devices/dev_tcp.c"

/* end of drivers/xbox/dev_tcp.c */

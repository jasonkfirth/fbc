'' Project: FreeBASIC compiler - semantic source revisions
'' File: tooling/semantic-source-file.bas
'' Purpose: Identify the exact regular file opened by the compiler.
'' Responsibilities: Hash its bytes, preserve stream position, and verify closure.
'' This file intentionally does NOT contain: decoding, parsing, or path lookup.

#include once "tooling/semantic-source-file.bi"
#include once "fbc-int/file-info.bi"
#include once "crt/stdio.bi"
#include once "crt/mem.bi"

'' -------------------------------------------------------------------------
'' SHA-256: unsigned arithmetic and big-endian block words
'' -------------------------------------------------------------------------

const HASH_BLOCK_BYTES = 64
const SOURCE_READ_BYTES = 16384

type SEMANTIC_SOURCE_HASH
	words(0 to 7) as ulong
	bytes as ulongint
	block(0 to HASH_BLOCK_BYTES - 1) as ubyte
	used as uinteger
end type

'' FIPS 180-4 SHA-256 constants. A block is 512 bits and each round word is
'' 32 bits, independent of the compiler host's INTEGER size.
dim shared as const ulong hash_constants(0 to 63) = { _
	&h428a2f98UL,&h71374491UL,&hb5c0fbcfUL,&he9b5dba5UL,&h3956c25bUL,&h59f111f1UL,&h923f82a4UL,&hab1c5ed5UL, _
	&hd807aa98UL,&h12835b01UL,&h243185beUL,&h550c7dc3UL,&h72be5d74UL,&h80deb1feUL,&h9bdc06a7UL,&hc19bf174UL, _
	&he49b69c1UL,&hefbe4786UL,&h0fc19dc6UL,&h240ca1ccUL,&h2de92c6fUL,&h4a7484aaUL,&h5cb0a9dcUL,&h76f988daUL, _
	&h983e5152UL,&ha831c66dUL,&hb00327c8UL,&hbf597fc7UL,&hc6e00bf3UL,&hd5a79147UL,&h06ca6351UL,&h14292967UL, _
	&h27b70a85UL,&h2e1b2138UL,&h4d2c6dfcUL,&h53380d13UL,&h650a7354UL,&h766a0abbUL,&h81c2c92eUL,&h92722c85UL, _
	&ha2bfe8a1UL,&ha81a664bUL,&hc24b8b70UL,&hc76c51a3UL,&hd192e819UL,&hd6990624UL,&hf40e3585UL,&h106aa070UL, _
	&h19a4c116UL,&h1e376c08UL,&h2748774cUL,&h34b0bcb5UL,&h391c0cb3UL,&h4ed8aa4aUL,&h5b9cca4fUL,&h682e6ff3UL, _
	&h748f82eeUL,&h78a5636fUL,&h84c87814UL,&h8cc70208UL,&h90befffaUL,&ha4506cebUL,&hbef9a3f7UL,&hc67178f2UL }

private function hRotateRight( byval value as ulong, byval bits as uinteger ) as ulong
	return (value shr bits) or (value shl (32 - bits))
end function

private sub hHashBlock( byref hash as SEMANTIC_SOURCE_HASH )
	dim as ulong schedule(0 to 63)
	for index as integer = 0 to 15
		dim as ubyte ptr word = @hash.block(index * 4)
		schedule(index) = (culng(word[0]) shl 24) or (culng(word[1]) shl 16) or _
			(culng(word[2]) shl 8) or culng(word[3])
	next
	for index as integer = 16 to 63
		dim as ulong first = schedule(index - 15), second = schedule(index - 2)
		dim as ulong sigma0 = hRotateRight(first, 7) xor hRotateRight(first, 18) xor (first shr 3)
		dim as ulong sigma1 = hRotateRight(second, 17) xor hRotateRight(second, 19) xor (second shr 10)
		'' SHA additions are modulo 2^32. Add in 64 bits before explicitly
		'' reducing, so signed overflow never depends on an emission backend.
		schedule(index) = culng((culngint(schedule(index - 16)) + sigma0 + schedule(index - 7) + sigma1) and &hffffffffULL)
	next
	dim as ulong a = hash.words(0), b = hash.words(1), c = hash.words(2), d = hash.words(3)
	dim as ulong e = hash.words(4), f = hash.words(5), g = hash.words(6), h = hash.words(7)
	for index as integer = 0 to 63
		dim as ulong sigma1 = hRotateRight(e, 6) xor hRotateRight(e, 11) xor hRotateRight(e, 25)
		dim as ulong choose = (e and f) xor ((not e) and g)
		dim as ulong first = culng((culngint(h) + sigma1 + choose + hash_constants(index) + schedule(index)) and &hffffffffULL)
		dim as ulong sigma0 = hRotateRight(a, 2) xor hRotateRight(a, 13) xor hRotateRight(a, 22)
		dim as ulong majority = (a and b) xor (a and c) xor (b and c)
		dim as ulong second = culng((culngint(sigma0) + majority) and &hffffffffULL)
		h = g
		g = f
		f = e
		e = culng((culngint(d) + first) and &hffffffffULL)
		d = c
		c = b
		b = a
		a = culng((culngint(first) + second) and &hffffffffULL)
	next
	dim as ulong state(0 to 7) = { a, b, c, d, e, f, g, h }
	for index as integer = 0 to 7
		hash.words(index) = culng((culngint(hash.words(index)) + state(index)) and &hffffffffULL)
	next
end sub

private sub hHashInit( byref hash as SEMANTIC_SOURCE_HASH )
	dim as const ulong initial(0 to 7) = { _
		&h6a09e667UL,&hbb67ae85UL,&h3c6ef372UL,&ha54ff53aUL, _
		&h510e527fUL,&h9b05688cUL,&h1f83d9abUL,&h5be0cd19UL }
	memset(@hash, 0, sizeof(hash))
	'' SIZEOF(array) is the element size in BASIC, not the whole array.
	memcpy(@hash.words(0), @initial(0), 8 * sizeof(ulong))
end sub

private function hHashUpdate( byref hash as SEMANTIC_SOURCE_HASH, byval bytes as const ubyte ptr, byval count as uinteger ) as integer
	'' SHA-256 stores its bit length in 64 bits. Check before multiplying the
	'' total byte count by eight when the final block is constructed.
	if( count > &h1fffffffffffffffULL - hash.bytes ) then return FALSE
	hash.bytes += count
	while( count <> 0 )
		dim as uinteger take = HASH_BLOCK_BYTES - hash.used
		if( take > count ) then take = count
		memcpy(@hash.block(hash.used), bytes, take)
		hash.used += take
		bytes += take
		count -= take
		if( hash.used = HASH_BLOCK_BYTES ) then
			hHashBlock(hash)
			hash.used = 0
		end if
	wend
	return TRUE
end function

private sub hHashFinish( byref hash as SEMANTIC_SOURCE_HASH, byval digest as zstring ptr )
	dim as ulongint bits = hash.bytes * 8
	hash.block(hash.used) = &h80
	hash.used += 1
	if( hash.used > 56 ) then
		memset(@hash.block(0) + hash.used, 0, HASH_BLOCK_BYTES - hash.used)
		hHashBlock(hash)
		hash.used = 0
	end if
	memset(@hash.block(0) + hash.used, 0, 56 - hash.used)
	for index as integer = 0 to 7
		hash.block(63 - index) = cubyte((bits shr (index * 8)) and &hffULL)
	next
	hHashBlock(hash)
	dim as const zstring * 17 digits = "0123456789abcdef"
	for index as integer = 0 to 31
		dim as uinteger shift = (3 - index mod 4) * 8
		dim as ubyte value = cubyte((hash.words(index \ 4) shr shift) and &hffUL)
		digest[index * 2] = digits[value shr 4]
		digest[index * 2 + 1] = digits[value and 15]
	next
	digest[64] = 0
end sub

'' -------------------------------------------------------------------------
'' Open-stream snapshots
'' -------------------------------------------------------------------------

type SEMANTIC_SOURCE_REVISION
	stream as FILE ptr
	identity as FB_FILE_INFO
	digest as zstring * 65
	bytes as ulongint
	regular as integer
end type

private function hSameRevision( byref first as const FB_FILE_INFO, byref second as const FB_FILE_INFO ) as integer
	'' Only initialized fixed-width values cross the runtime interface.
	return memcmp(@first, @second, sizeof(first)) = 0
end function

private function hSnapshotStream _
	( byval stream as FILE ptr, byref identity as FB_FILE_INFO, byval digest as zstring ptr, byref bytes as ulongint ) as long
	if( fb_FileQueryStreamInfo(stream, @identity) = 0 ) then return 0
	if( (identity.flags and FB_FILE_INFO_REGULAR) = 0 ) then return 2
	if( ferror(stream) ) then return 0
	'' fpos_t contains host-specific conversion state as well as an offset.
	'' The runtime owns that layout; restoration consumes its position token.
	dim as any ptr position = fb_CrtFileSavePos(stream)
	if( position = NULL ) then return 0
	dim as integer ok = fseek(stream, 0, SEEK_SET) = 0
	dim as SEMANTIC_SOURCE_HASH hash
	hHashInit(hash)
	'' Bounded reads keep large source files out of compiler heap staging.
	dim as ubyte buffer(0 to SOURCE_READ_BYTES - 1)
	while( ok )
		dim as uinteger count = fread(@buffer(0), 1, SOURCE_READ_BYTES, stream)
		if( count = 0 ) then exit while
		ok = hHashUpdate(hash, @buffer(0), count)
	wend
	if( ferror(stream) ) then ok = FALSE
	if( fb_CrtFileRestorePos(stream, position) = 0 ) then ok = FALSE
	dim as FB_FILE_INFO after
	if( fb_FileQueryStreamInfo(stream, @after) = 0 ) then ok = FALSE
	if( hSameRevision(identity, after) = FALSE ) then ok = FALSE
	if( ok ) then
		bytes = hash.bytes
		hHashFinish(hash, digest)
	end if
	return abs(ok <> FALSE)
end function

function fbSemanticSourceOpen _
	( byval stream as any ptr, byval digest as zstring ptr, byval bytes as ulongint ptr, byval status as long ptr ) as any ptr
	if( (stream = NULL) or (digest = NULL) or (bytes = NULL) or (status = NULL) ) then return NULL
	digest[0] = 0
	*bytes = 0
	*status = 0
	dim as SEMANTIC_SOURCE_REVISION ptr revision = new SEMANTIC_SOURCE_REVISION
	if( revision = NULL ) then return NULL
	revision->stream = stream
	*status = hSnapshotStream(revision->stream, revision->identity, @revision->digest, revision->bytes)
	if( *status = 0 ) then
		delete revision
		return NULL
	end if
	revision->regular = *status = 1
	*digest = revision->digest
	*bytes = revision->bytes
	return revision
end function

function fbSemanticSourceClose( byval handle as any ptr ) as long
	dim as SEMANTIC_SOURCE_REVISION ptr revision = handle
	if( revision = NULL ) then return 0
	if( revision->regular = FALSE ) then
		delete revision
		return 2
	end if
	dim as FB_FILE_INFO identity
	dim as zstring * 65 digest
	dim as ulongint bytes
	dim as long status = hSnapshotStream(revision->stream, identity, @digest, bytes)
	if( (status <> 1) or (hSameRevision(revision->identity, identity) = FALSE) or _
	    (bytes <> revision->bytes) or (digest <> revision->digest) ) then status = 0
	delete revision
	return status
end function

'' end of tooling/semantic-source-file.bas

''
'' FreeBASIC runtime: fbrandom.bi
'' Checked cryptographic bytes from the target's system entropy provider.
'' The caller owns a writable buffer; this interface neither seeds nor uses RND.
'' Provider failures clear the complete valid output span before returning.
'' Invalid arguments leave memory unchanged. A zero-byte request succeeds,
'' including a NULL destination, but does not establish provider availability.
''
#ifndef __FB_RANDOM_BI__
#define __FB_RANDOM_BI__

#if __FB_LANG__ <> "fb"
	#error fbrandom.bi requires the fb dialect
#endif

namespace FB

	const SECURE_RANDOM_OK as long = 0
	const SECURE_RANDOM_UNAVAILABLE as long = 1
	const SECURE_RANDOM_IO_ERROR as long = 2
	const SECURE_RANDOM_INVALID_ARGUMENT as long = 3

	'' One MiB bounds each call, its failure wipe and platform conversions.
	const SECURE_RANDOM_MAX_BYTES as uinteger = 1048576

end namespace

namespace FBC

	extern "rtlib"
		'' Thread-safe for separate caller-owned buffers. DOS serializes its
		'' shared real-mode transfer area through the existing runtime lock.
		'' The status is returned directly; the BASIC ERR value is unchanged.
		declare function secureRandom alias "fb_SecureRandom" _
			( byval destination as any ptr, byval byte_count as uinteger ) as long
	end extern

end namespace

#endif
'' end of fbrandom.bi

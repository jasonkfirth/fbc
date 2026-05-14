' TEST_MODE : COMPILE_AND_RUN_OK

#define ASSERT(e) if (e) = 0 then fb_Assert(__FILE__, __LINE__, __FUNCTION__, #e)

#if defined( __FB_BIGENDIAN__ )
	#define MK16 "BA"
	#define MK32 "DCBA"
	#define CV16 &h4142
	#define CV32 &h41424344
#else
	#define MK16 "AB"
	#define MK32 "ABCD"
	#define CV16 &h4241
	#define CV32 &h44434241
#endif

dim i as integer, l as long, i32 as integer<32>
dim si as string, sl as string, si32 as string
dim s as string


'' constant:
i = cvi("ABCD")
l = cvl("ABCD")
i32 = cvi<32>("ABCD")

si = mki$(&h44434241)
sl = mkl$(&h44434241)
si32 = mki$<32>(&h44434241)

assert(i = CV16)
assert(l = CV32)
assert(i32 = CV32)

assert(si = MK16)
assert(sl = MK32)
assert(si32 = MK32)


'' variable:
si = "AB"
sl = "ABCD"
si32 = "ABCD"

i = cvi(si)
l = cvl(sl)
i32 = cvi<32>(si32)

assert(i = CV16)
assert(l = CV32)
assert(i32 = CV32)

si = mki$(i)
sl = mkl$(l)
si32 = mki$<32>(i32)

assert(si = "AB")
assert(sl = "ABCD")
assert(si32 = "ABCD")

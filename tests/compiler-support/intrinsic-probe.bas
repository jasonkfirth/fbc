/'
    Project: FreeBASIC compiler regression tests
    File: intrinsic-probe.bas
    Purpose: Check a compiler-recognized Windows intrinsic spelling.
    Responsibilities: Request processor identification through the intrinsic.
    This fixture does not inspect or modify machine configuration.
'/
extern "C"
    declare sub __cpuid( byval values as long ptr, byval leaf as long )
    declare function _interlockedbittestandset( byval base_ as long ptr, byval bit_ as long ) as ubyte
    declare function __rdtsc() as ulongint
    declare sub _mm_pause()
end extern
dim as long values(0 to 3)
__cpuid(@values(0), 0)
if values(0) = 0 then end 1
dim as long bits_ = 0
if _interlockedbittestandset(@bits_, 2) <> 0 orelse bits_ <> 4 then end 2
if __rdtsc() = 0 then end 3
_mm_pause()
print "INTRINSIC_PROBE_PASS"

/' end of intrinsic-probe.bas '/

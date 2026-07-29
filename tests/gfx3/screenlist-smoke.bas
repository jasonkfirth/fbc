''
'' Project: FreeBASIC gfxlib3 tests
'' --------------------------------
''
'' File: screenlist-smoke.bas
''
'' Purpose:
''
''     Verify the public SCREENLIST iterator remains stable for native display
''     modes and deterministic standard-mode fallbacks.
''
'' Responsibilities:
''
''     - restart the iterator for every supported depth
''     - check strictly sorted, unique packed-width/height results
''     - prove a restart returns exactly the same sequence
''     - reject unsupported depth requests without stale iteration state
''
'' This file intentionally does NOT contain:
''
''     - direct operating-system display-mode comparison
''     - fullscreen display-mode changes
''     - renderer or presentation testing
''
#ifndef __FB_GFXLIB3__
    #define __FB_GFXLIB3__
#endif

const maximum_modes = 128
dim depths(0 to 7) as integer = { 1, 2, 4, 8, 15, 16, 24, 32 }
dim as integer first_pass(0 to maximum_modes - 1)
dim as integer i, entry, previous, count, replay_count

for i = 0 to 7
    previous = 0
    count = 0
    entry = screenlist( depths(i) )

    while entry <> 0
        if entry <= previous then end 10 + i
        if (entry shr 16) <= 0 orelse (entry and &hFFFF) <= 0 then end 20 + i
        if count >= maximum_modes then end 30 + i
        first_pass(count) = entry
        previous = entry
        count += 1
        entry = screenlist()
    wend

    replay_count = 0
    entry = screenlist( depths(i) )
    while entry <> 0
        if replay_count >= count then end 40 + i
        if entry <> first_pass(replay_count) then end 50 + i
        replay_count += 1
        entry = screenlist()
    wend
    if replay_count <> count then end 60 + i
next i

if screenlist( 3 ) <> 0 then end 40
if screenlist() <> 0 then end 41

print "gfxlib screenlist PASS"
end 0

'' end of screenlist-smoke.bas

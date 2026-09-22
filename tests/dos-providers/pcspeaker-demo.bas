/'
    FreeBASIC DOS examples: pcspeaker-demo.bas

    Play three mixed voices through the PC speaker while the foreground
    computes. Build with -target dos -dos-threads pdmlwp. The runtime owns
    the worker and sample interrupt; this program does not touch hardware.
'/
#lang "fb"

#ifndef __FB_DOS__
    #error This example requires the DOS target.
#endif
#if __FB_MT__ = 0
    #error Build with -dos-threads pdmlwp.
#endif

'' Select the speaker even on machines with a configured Sound Blaster.
environ "SFXLIB_DRIVER=PCSpeaker"

dim frequencies(0 to 2) as integer = {220, 275, 330}
dim iterations as ulongint = 0
dim checksum as double = 0

for chord_index as integer = 0 to 2
    dim root_note as integer = frequencies(chord_index)
    sound 0, root_note, 0.8, 0.5
    sound 1, root_note * 5 \ 4, 0.8, 0.3
    sound 2, root_note * 3 \ 2, 0.8, 0.2

    '' No sleep, delay, yield or sound pumping in the computation loop.
    dim started as double = timer
    do
        for item as integer = 1 to 100
            checksum += sqr(cdbl(item))
        next
        iterations += 1
    loop while timer - started < 0.8
next

print "Computed batches:"; iterations; " checksum:"; checksum
if iterations = 0 or checksum <= 0 then end 1
print "PASS FreeBASIC PC speaker demo"
end 0

' end of pcspeaker-demo.bas

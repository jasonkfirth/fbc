'' FreeBASIC classic AmigaOS sound qualification
'' -------------------------------------------
'' File: sound.bas
'' Purpose: Record known stereo tones accepted by the native audio driver.
'' Responsibilities: Check initialization, streaming, capture, and shutdown.
'' This file intentionally does NOT contain driver or WAV encoding logic.

#include once "sfxlib_raw.bi"

extern "C"
	declare function amiga_test_sound_driver() as long
end extern

dim rate as integer = sfxlib.RawOpen()
if rate < 8000 or rate > 192000 then end 1
if amiga_test_sound_driver() = 0 then end 2
dim frames as integer = rate \ 4
dim samples(0 to frames * 2 - 1) as single
const tau = 6.2831853071795864769
for frame as integer = 0 to frames - 1
	samples(frame * 2) = sin(tau * 440 * frame / rate) * 0.5
	samples(frame * 2 + 1) = sin(tau * 880 * frame / rate) * 0.5
next
if sfxlib.OutputCaptureStart() <> 0 then end 3
if sfxlib.OutputCaptureReserve(frames) <> 0 then end 4
dim sent as integer = 0
while sent < frames
	dim accepted as integer = sfxlib.RawWrite(@samples(sent * 2), frames - sent, 2)
	if accepted < 0 then end 5
	sent += accepted
	if accepted = 0 then sleep 10, 1
wend
sleep 1200, 1
sfxlib.OutputCaptureStop()
if sfxlib.OutputCaptureSave("native-sound.wav") <> 0 then end 6
sfxlib.RawClose()
print "Native audio capture passed at "; rate; " Hz"
end 0

'' end of sound.bas

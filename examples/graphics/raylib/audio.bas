'' Project: FreeBASIC raylib 5.5 examples
'' File: audio.bas
'' Purpose: Load PCM from memory and play a short tone.
'' Responsibilities: Exercise Wave, Sound and AudioStream across the C ABI.
'' This file intentionally does not contain audio assets or a mixer.

#ifdef __FB_JS__
	#include once "raylib.bi"
#else
	#include once "raylib55.bi"
#endif

'' RIFF/WAVE uses little-endian fields. The 44-byte header describes 800
'' mono 16-bit samples at 8 kHz. Each 20-sample period gives a 400 Hz tone.
dim as string wave_bytes = "RIFF" & mkl(1636) & "WAVEfmt " & mkl(16) & _
	mkshort(1) & mkshort(1) & mkl(8000) & mkl(16000) & mkshort(2) & mkshort(16) & "data" & mkl(1600)
for sample_index as long = 0 to 799
	wave_bytes &= mkshort(8000 * sin(2 * PI * sample_index / 20))
next
dim as Wave tone_wave = LoadWaveFromMemory(".wav", cast(const ubyte ptr, strptr(wave_bytes)), len(wave_bytes))
if IsWaveValid(tone_wave) = 0 then end 1
if tone_wave.frameCount <> 800 orelse tone_wave.sampleRate <> 8000 orelse _
   tone_wave.sampleSize <> 16 orelse tone_wave.channels <> 1 then
	UnloadWave(tone_wave)
	end 1
end if

InitAudioDevice()
if IsAudioDeviceReady() = 0 then
	UnloadWave(tone_wave)
	end 1
end if
dim as Sound tone_sound = LoadSoundFromWave(tone_wave)
UnloadWave(tone_wave)
if IsSoundValid(tone_sound) = 0 then
	CloseAudioDevice()
	end 1
end if

PlaySound(tone_sound)
dim as integer status
if IsSoundPlaying(tone_sound) = 0 orelse tone_sound.frameCount = 0 orelse tone_sound.stream.channels = 0 then status = 1
sleep 150, 1
UnloadSound(tone_sound)
CloseAudioDevice()
if status = 0 then print "raylib audio passed"
end status

'' end of audio.bas

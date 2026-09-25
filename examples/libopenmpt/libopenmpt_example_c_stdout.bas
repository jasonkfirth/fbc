' Project: FreeBASIC libopenmpt examples
' File: libopenmpt_example_c_stdout.bas
' Purpose:
'   Render a tracker module as raw stereo PCM on standard output.
' Responsibilities:
'   - Exercise chunked libopenmpt rendering.
'   - Keep diagnostics on standard error while stdout carries binary audio.
' This file intentionally does NOT contain:
'   - Audio device output or format/container headers.
'   - A general-purpose stream-copy implementation.
' Adapted from the official libopenmpt C stdout example (BSD-3-Clause).

#include once "libopenmpt_example_common.bi"

#if defined(__FB_WIN32__)
#include once "crt/fcntl.bi"
#endif

const SAMPLE_RATE as long = 48000
const BUFFER_FRAMES as long = 480

dim as string input_path = command(1)
if input_path = "" then
	fputs(!"Usage: libopenmpt_example_c_stdout <module-file> > output.raw\n", stderr)
	end 1
end if

#if defined(__FB_WIN32__)
' The Microsoft CRT otherwise translates line-feed bytes in binary audio.
if _setmode(_fileno(stdout), _O_BINARY) = -1 then
	fputs(!"Error: could not set standard output to binary mode.\n", stderr)
	end 1
end if
#endif

dim as openmpt_module ptr mod_ = ExampleLoadModule(input_path)
if mod_ = 0 then end 1

if openmpt_module_set_repeat_count(mod_, 0) = 0 then
	fputs(!"Error: could not set the module to play once.\n", stderr)
	openmpt_module_destroy(mod_)
	end 1
end if

dim as short sample_buffer(0 to BUFFER_FRAMES * 2 - 1)
dim as size_t frames_rendered
dim as size_t samples_written
dim as integer failed = 0

do
	frames_rendered = openmpt_module_read_interleaved_stereo( _
		mod_, SAMPLE_RATE, BUFFER_FRAMES, @sample_buffer(0) _
	)
	if frames_rendered = 0 then exit do
	if frames_rendered > BUFFER_FRAMES then
		fputs(!"Error: libopenmpt returned more frames than the supplied buffer can hold.\n", stderr)
		failed = 1
		exit do
	end if

	' Raw PCM is native-endian signed 16-bit stereo: left, then right.
	samples_written = fwrite(@sample_buffer(0), sizeof(short), frames_rendered * 2, stdout)
	if samples_written <> frames_rendered * 2 then
		fputs(!"Error: could not write all rendered audio to standard output.\n", stderr)
		failed = 1
		exit do
	end if
loop

if fflush(stdout) <> 0 then
	fputs(!"Error: could not flush standard output.\n", stderr)
	failed = 1
end if

openmpt_module_destroy(mod_)
if failed then end 1
end 0

' end of libopenmpt_example_c_stdout.bas

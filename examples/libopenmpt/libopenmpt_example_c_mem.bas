' Project: FreeBASIC libopenmpt examples
' File: libopenmpt_example_c_mem.bas
' Purpose:
'   Load a tracker module from memory and render it to a PCM WAVE file.
' Responsibilities:
'   - Exercise libopenmpt's memory-based module constructor.
'   - Render bounded stereo chunks and write a valid PCM WAVE header.
' This file intentionally does NOT contain:
'   - Real-time audio device output.
'   - A general-purpose WAVE or audio conversion library.
' Adapted from the official libopenmpt C in-memory example (BSD-3-Clause).

#include once "libopenmpt_example_common.bi"

const SAMPLE_RATE as long = 48000
const BUFFER_FRAMES as long = 480
const WAV_HEADER_SIZE as long = 44
const MAX_WAV_DATA_SIZE as ulongint = 4294967259

private sub StoreLittleEndian16(byref buffer as string, byval offset as long, byval value as ulongint)
	mid(buffer, offset, 1) = chr(cint(value and &hFF))
	mid(buffer, offset + 1, 1) = chr(cint((value shr 8) and &hFF))
end sub

private sub StoreLittleEndian32(byref buffer as string, byval offset as long, byval value as ulongint)
	mid(buffer, offset, 1) = chr(cint(value and &hFF))
	mid(buffer, offset + 1, 1) = chr(cint((value shr 8) and &hFF))
	mid(buffer, offset + 2, 1) = chr(cint((value shr 16) and &hFF))
	mid(buffer, offset + 3, 1) = chr(cint((value shr 24) and &hFF))
end sub

private sub BuildWaveHeader(byref header as string, byval data_size as ulongint)
	header = space(WAV_HEADER_SIZE)
	mid(header, 1, 4) = "RIFF"
	StoreLittleEndian32(header, 5, data_size + 36)
	mid(header, 9, 4) = "WAVE"
	mid(header, 13, 4) = "fmt "
	StoreLittleEndian32(header, 17, 16)
	StoreLittleEndian16(header, 21, 1)
	StoreLittleEndian16(header, 23, 2)
	StoreLittleEndian32(header, 25, SAMPLE_RATE)
	StoreLittleEndian32(header, 29, SAMPLE_RATE * 4)
	StoreLittleEndian16(header, 33, 4)
	StoreLittleEndian16(header, 35, 16)
	mid(header, 37, 4) = "data"
	StoreLittleEndian32(header, 41, data_size)
end sub

dim as string input_path = command(1)
dim as string output_path = command(2)
if input_path = "" then
	print "Usage: libopenmpt_example_c_mem <module-file> [output.wav]"
	end 1
end if
if output_path = "" then output_path = "module.wav"

dim as openmpt_module ptr mod_ = ExampleLoadModule(input_path)
if mod_ = 0 then end 1

if openmpt_module_set_repeat_count(mod_, 0) = 0 then
	fputs(!"Error: could not set the module to play once.\n", stderr)
	openmpt_module_destroy(mod_)
	end 1
end if

dim as FILE ptr output_file = fopen(strptr(output_path), "wb+")
if output_file = 0 then
	fprintf(stderr, !"Error: could not open output file '%s'.\n", strptr(output_path))
	openmpt_module_destroy(mod_)
	end 1
end if

dim as string wave_header
BuildWaveHeader(wave_header, 0)
if fwrite(strptr(wave_header), 1, WAV_HEADER_SIZE, output_file) <> WAV_HEADER_SIZE then
	fputs(!"Error: could not write WAVE header.\n", stderr)
	fclose(output_file)
	openmpt_module_destroy(mod_)
	end 1
end if

dim as short sample_buffer(0 to BUFFER_FRAMES * 2 - 1)
dim as ubyte pcm_buffer(0 to BUFFER_FRAMES * 4 - 1)
dim as size_t frames_rendered
dim as ulongint data_size = 0
dim as ulongint chunk_size
dim as long sample_index
dim as ushort sample_bits
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

	chunk_size = cast(ulongint, frames_rendered) * 4
	if chunk_size > MAX_WAV_DATA_SIZE - data_size then
		fputs(!"Error: rendered audio exceeds the 4 GiB RIFF/WAVE size limit.\n", stderr)
		failed = 1
		exit do
	end if

	' WAVE samples are little-endian even when the host uses another byte order.
	for sample_index = 0 to cast(long, frames_rendered * 2) - 1
		sample_bits = cast(ushort, sample_buffer(sample_index))
		pcm_buffer(sample_index * 2) = cast(ubyte, sample_bits and &hFF)
		pcm_buffer(sample_index * 2 + 1) = cast(ubyte, (sample_bits shr 8) and &hFF)
	next

	if fwrite(@pcm_buffer(0), 1, cast(size_t, chunk_size), output_file) <> cast(size_t, chunk_size) then
		fputs(!"Error: could not write all rendered audio data.\n", stderr)
		failed = 1
		exit do
	end if
	data_size += chunk_size
loop

BuildWaveHeader(wave_header, data_size)
if failed = 0 then
	if fseek(output_file, 0, SEEK_SET) <> 0 then
		fputs(!"Error: could not seek back to the WAVE header.\n", stderr)
		failed = 1
	elseif fwrite(strptr(wave_header), 1, WAV_HEADER_SIZE, output_file) <> WAV_HEADER_SIZE then
		fputs(!"Error: could not update WAVE header.\n", stderr)
		failed = 1
	end if
end if

if fclose(output_file) <> 0 then
	fputs(!"Error: could not close the output file cleanly.\n", stderr)
	failed = 1
end if
openmpt_module_destroy(mod_)

if failed then end 1
print "Wrote "; data_size; " bytes of PCM audio to "; output_path
end 0

' end of libopenmpt_example_c_mem.bas

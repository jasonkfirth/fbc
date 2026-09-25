' Project: FreeBASIC libopenmpt examples
' File: libopenmpt_example_common.bi
' Purpose:
'   Provide the shared, checked file-loading path used by the examples.
' Responsibilities:
'   - Read a module file into a bounded memory buffer.
'   - Construct a libopenmpt module and release temporary input storage.
' This file intentionally does NOT contain:
'   - Audio rendering or output handling.
'   - A general-purpose file or playback abstraction.
' Ported from the loading pattern in the official libopenmpt C examples.
' SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include once "crt/stdio.bi"
#include once "libopenmpt.bi"

private function ExampleLoadModule(byref filename as string) as openmpt_module ptr
	dim as FILE ptr input_file = fopen(strptr(filename), "rb")
	dim as clong input_length
	dim as size_t input_size
	dim as ubyte ptr input_data = 0
	dim as openmpt_module ptr mod_ = 0
	dim as long error_code = 0

	if input_file = 0 then
		fprintf(stderr, !"Error: could not open module file '%s'.\n", strptr(filename))
		return 0
	end if

	if fseek(input_file, 0, SEEK_END) <> 0 then
		fputs(!"Error: could not seek in module file.\n", stderr)
		goto cleanup
	end if

	input_length = ftell(input_file)
	if input_length <= 0 then
		fputs(!"Error: module file is empty or its size could not be read.\n", stderr)
		goto cleanup
	end if

	' Keep the file size representable by FreeBASIC's binary I/O length argument.
	if input_length > 2147483647 then
		fputs(!"Error: module files larger than 2 GiB are not supported by this example.\n", stderr)
		goto cleanup
	end if

	if fseek(input_file, 0, SEEK_SET) <> 0 then
		fputs(!"Error: could not rewind module file.\n", stderr)
		goto cleanup
	end if

	input_size = cast(size_t, input_length)
	input_data = allocate(input_size)
	if input_data = 0 then
		fputs(!"Error: could not allocate module input buffer.\n", stderr)
		goto cleanup
	end if

	if fread(input_data, 1, input_size, input_file) <> input_size then
		fputs(!"Error: could not read the complete module file.\n", stderr)
		goto cleanup
	end if

	if fclose(input_file) <> 0 then
		input_file = 0
		fputs(!"Error: could not close module file after reading.\n", stderr)
		goto cleanup
	end if
	input_file = 0

	' The memory-based constructor copies/parses the input before it returns,
	' so the temporary file buffer can be released immediately afterward.
	mod_ = openmpt_module_create_from_memory2( _
		input_data, input_size, 0, 0, 0, 0, @error_code, 0, 0 _
	)
	deallocate(input_data)
	input_data = 0

	if mod_ = 0 then
		fprintf(stderr, !"Error: libopenmpt could not load the module (error %d).\n", error_code)
	end if

cleanup:
	if input_data <> 0 then deallocate(input_data)
	if input_file <> 0 then fclose(input_file)
	return mod_
end function

' end of libopenmpt_example_common.bi

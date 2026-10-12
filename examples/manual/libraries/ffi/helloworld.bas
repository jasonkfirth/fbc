'' examples/manual/libraries/ffi/helloworld.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'libffi'
'' Purpose: Call a typed C function through a libffi call interface.
'' Responsibilities: Match its C return type and provide ffi_arg result storage.
'' This example contains no closure allocation or persistent argument storage.
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=ExtLibFfi
'' --------

#include "ffi.bi"

' Simple "puts" equivalent function
Function printer cdecl (ByVal s As ZString Ptr) As ULong
	If s = 0 Then Return 0
	Print *s
	Return 42
End Function

' Initialize the argument info vectors
Dim s As ZString Ptr
Dim args(0 To 0) As ffi_type Ptr = {@ffi_type_pointer}
Dim values(0 To 0) As Any Ptr = {@s}

' Initialize the cif
Dim cif As ffi_cif
Dim result As ffi_status
result = ffi_prep_cif( _
	@cif,              _ ' call interface object
	FFI_DEFAULT_ABI,   _ ' binary interface type
	1,                 _ ' number of arguments
	@ffi_type_uint,    _ ' return type
	@args(0)           _ ' arguments
)

' Call function
' libffi widens small integer results to ffi_arg when storing them.
Dim return_value As ffi_arg
If result = FFI_OK Then
	s = @"Hello world"
	ffi_call(@cif, FFI_FN(@printer), @return_value, @values(0))

	' values holds a pointer to the function's arg, so to
	' call puts() again all we need to do is change the
	' value of s */
	s = @"This is cool!"
	ffi_call(@cif, FFI_FN(@printer), @return_value, @values(0))
	Print Using "Function returned &"; return_value
Else
	Print "Could not prepare the libffi call interface"
	End 1
End If
'' end of helloworld.bas

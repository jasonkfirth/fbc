'' Project: FreeBASIC semantic sidecar tests
'' File: pointer-storage-inputs.bas
'' Purpose: Preserve selected address operations and storage families.
'' Responsibilities: Typed inputs, NEW forms and original DELETE targets.
'' This file intentionally does NOT execute unsafe ownership combinations.
#lang "fb"
type Item
	value as integer
	declare destructor( )
end type
destructor Item( )
end destructor
type TextHolder
	text as string
	declare constructor(byref value as const string)
end type
constructor TextHolder(byref value as const string)
	text = value
end constructor
sub Observe( )
	dim text as string = "value"
	dim descriptor as any ptr = varptr(text)
	dim buffer as zstring ptr = strptr(text)
	dim address as string ptr = @text
	dim literal_buffer as zstring ptr = strptr("literal")
	dim temporary_address as Item ptr = @type<Item>(5)
	dim temporary_buffer as zstring ptr = strptr(type<TextHolder>("value").text)
	dim indirect_buffer as zstring ptr = strptr(*@text)
	dim scalar as Item ptr = new Item
	dim vector as Item ptr = new Item[3]
	dim uninitialized as integer ptr = new integer[4]{any}
	dim placement as Item ptr = new (descriptor) Item
	delete scalar
	delete[] vector
	delete[] uninitialized
	placement->destructor( )
end sub
'' end of pointer-storage-inputs.bas

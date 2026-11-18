'' Project: FreeBASIC compiler regression tests
'' File: threadcall-temporaries.bas
'' Purpose: Reject references to storage that expires during thread launch.
'' Responsibilities: Exercise scalar, object, descriptor and string conversions.
'' This file intentionally does NOT contain: Executable thread tests.

' TEST_MODE : COMPILE_ONLY_FAIL

type Sample
	value as long
end type

type Holder
	values(1 to 2) as long
end type

declare sub integer_ref(byref n as long)
declare sub record_ref(byref n as Sample)
declare sub array_ref(values() as long)
declare sub string_ref(byref text as string)
declare sub pointer_value(byval text as zstring ptr)
declare function make_string() as string

dim box as Holder
dim fixed_text as string * 12
dim thread_id as any ptr
thread_id = threadcall integer_ref(42L)
thread_id = threadcall record_ref(type<Sample>(42))
thread_id = threadcall array_ref(box.values())
thread_id = threadcall string_ref(fixed_text)
thread_id = threadcall pointer_value(make_string())

'' end of threadcall-temporaries.bas

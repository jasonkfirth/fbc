' TEST_MODE : COMPILE_AND_RUN_OK

''
'' FreeBASIC command sweep tests
'' -----------------------------
''
'' File: command-sweep.bas
''
'' Purpose:
''
''     Compile and run one program that touches FBLite-specific language
''     compatibility behavior.
''
'' Responsibilities:
''
''     - exercise implicit variables, dotted identifiers, namespace lookups,
''       nested WITH blocks, scoped LEN, and FBLite integer defaults
''     - keep the run bounded and non-interactive
''
'' This file intentionally does NOT contain:
''
''     - gfxlib coverage
''     - sfxlib coverage
''     - deprecated-mode shadowing coverage
''

#define CHECK(e) if (e) = 0 then fb_Assert(__FILE__, __LINE__, __FUNCTION__, #e)

type inner_t
	member as string
end type

type outer_t
	inner as inner_t
	value as integer
end type

namespace sweep_ns
	type box_t
		inner as inner_t
	end type

	dim shared box as box_t

	sub set_box( byref text as string )
		box.inner.member = text
	end sub
end namespace

dim outer as outer_t
outer.inner.member = "fblite"
outer.value = 42

CHECK( len( outer.inner.member ) = 6 )
CHECK( len( inner_t ) = sizeof( string ) )
CHECK( len( outer_t ) >= sizeof( string ) )

with outer
	CHECK( .value = 42 )
	CHECK( len( .inner.member ) = 6 )
end with

with outer.inner
	CHECK( .member = "fblite" )
end with

dim variable.string as string
variable.string = "dotted"
CHECK( variable.string = "dotted" )

implicit_value = 5
CHECK( implicit_value = 5 )

sweep_ns.set_box "namespace"
CHECK( sweep_ns.box.inner.member = "namespace" )
CHECK( len( sweep_ns.box.inner.member ) = 9 )

#ifdef __FB_64BIT__
	CHECK( sizeof( integer ) = 8 )
#else
	CHECK( sizeof( integer ) = 4 )
#endif

end 0

'' end of command-sweep.bas

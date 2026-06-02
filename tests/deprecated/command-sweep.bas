' TEST_MODE : COMPILE_AND_RUN_OK

''
'' FreeBASIC command sweep tests
'' -----------------------------
''
'' File: command-sweep.bas
''
'' Purpose:
''
''     Compile and run one program that touches deprecated language-mode
''     compatibility behavior that is still intentionally supported.
''
'' Responsibilities:
''
''     - exercise implicit variables before and after OPTION EXPLICIT
''     - exercise legacy shadowing behavior in nested control-flow scopes
''     - exercise string-suffix variables and dotted global names
''
'' This file intentionally does NOT contain:
''
''     - modern -lang fb command coverage
''     - gfxlib coverage
''     - sfxlib coverage
''

#define CHECK(e) if (e) = 0 then fb_Assert(__FILE__, __LINE__, __FUNCTION__, #e)

const dotted.name.value = 123

implicit_before = 10
CHECK( implicit_before = 10 )

scope
	dim as integer shadow_value = 1
	CHECK( shadow_value = 1 )

	scope
		dim as integer shadow_value = 2
		CHECK( shadow_value = 2 )
	end scope

	CHECK( shadow_value = 1 )
end scope

global_text$ = "global"

sub change_local_text( byval text$ )
	text$ = "local"
	CHECK( text$ = "local" )
end sub

dim as string temp = space( 4 )
change_local_text temp
CHECK( global_text$ = "global" )

namespace ns1
	type payload_t
		value as integer
	end type
end namespace

namespace ns2
	using ns1

	type holder_t
		payload as payload_t
	end type
end namespace

using ns2

dim holder as holder_t
holder.payload.value = dotted.name.value
CHECK( holder.payload.value = 123 )

option explicit

dim as integer explicit_value
explicit_value = 55
CHECK( explicit_value = 55 )

end 0

'' end of command-sweep.bas

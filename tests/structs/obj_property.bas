# include "fbcu.bi"

namespace fbc_tests.structs.obj_property

type bar
    declare property v as integer
    declare property v ( byval new_v as integer )
    declare property v ( byval new_v as zstring ptr )

    as integer p_v = any
end type

property bar.v as integer
	property = p_v
end property

property bar.v ( byval new_v as integer)
	p_v = new_v
end property

property bar.v ( byval new_v as zstring ptr )
	p_v = cint( *new_v )
end property

sub test cdecl
	dim as bar b1
	
	b1.v = 1
	CU_ASSERT_EQUAL( b1.v, 1 )

	b1.v = "2"
	CU_ASSERT_EQUAL( b1.v, 2 )
	
end sub

private sub ctor () constructor

	fbcu.add_suite("fb-tests-structs:property")
	fbcu.add_test( "test", @test)

end sub
	
end namespace	
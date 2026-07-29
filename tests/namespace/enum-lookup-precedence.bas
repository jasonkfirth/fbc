''
'' FreeBASIC Compiler Test Suite
''
'' File: enum-lookup-precedence.bas
''
'' Verify lookup precedence for implicitly imported enum elements in nested
'' namespaces and in UDT inheritance hierarchies.
''

#include once "fbcunit.bi"

const SHADOWED_VALUE = 10

namespace OUTER_SCOPE
	enum LOCAL_VALUES
		SHADOWED_VALUE = 20
	end enum

	namespace INNER_SCOPE
		function readValue() as integer
			return SHADOWED_VALUE
		end function
	end namespace
end namespace

type BASE_TYPE
	payload as integer

	enum MEMBER_VALUES
		BASE_VALUE = 30
	end enum

	declare function readBaseValue() as integer
end type

function BASE_TYPE.readBaseValue() as integer
	return BASE_VALUE
end function

type DERIVED_TYPE extends BASE_TYPE
	declare function readInheritedValue() as integer
end type

function DERIVED_TYPE.readInheritedValue() as integer
	return BASE_VALUE
end function

SUITE( fbc_tests.namespace_.enum_lookup_precedence )

	TEST( nested_namespace_enum )
		CU_ASSERT_EQUAL( 20, OUTER_SCOPE.INNER_SCOPE.readValue() )
	END_TEST

	TEST( udt_enum_members )
		dim base_object as BASE_TYPE
		dim derived as DERIVED_TYPE

		CU_ASSERT_EQUAL( 30, base_object.readBaseValue() )
		CU_ASSERT_EQUAL( 30, derived.readInheritedValue() )
	END_TEST

END_SUITE

'' end of enum-lookup-precedence.bas

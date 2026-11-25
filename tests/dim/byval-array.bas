'' Project: FreeBASIC compiler tests
'' File: dim/byval-array.bas
'' Purpose: Check bounds, element copies and lifetimes for BYVAL arrays.
'' Scalar value arguments are covered in functions/byval-arguments.bas.

#include "fbcunit.bi"

SUITE( fbc_tests.dim_.byval_array )

	declare function editGrid( byval values(any, any) as integer ) as integer

	function editGrid( byval values(any, any) as integer ) as integer
		CU_ASSERT( lbound( values, 1 ) = -2 )
		CU_ASSERT( ubound( values, 1 ) = -1 )
		CU_ASSERT( lbound( values, 2 ) = 3 )
		CU_ASSERT( ubound( values, 2 ) = 5 )
		dim as integer first = values(-2, 3)
		values(-2, 3) = -1
		redim preserve values(-2 to -1, 3 to 6)
		CU_ASSERT( values(-2, 3) = -1 )
		CU_ASSERT( ubound( values, 2 ) = 6 )
		return first
	end function

	function forwardGrid( values() as integer ) as integer
		return editGrid( byval values() )
	end function

	declare sub editOriginal( byref values(any) as integer )

	sub editOriginal( values(any) as integer )
		values(-1) = 7
		redim preserve values(-1 to 2)
	end sub

	type ArrayFields
		fixed(-2 to -1, 3 to 5) as integer
		resizable(any, any) as integer
	end type

	sub editStrings overload( byval values() as string )
		CU_ASSERT( values(1) = "caller" )
		values(1) = "local"
		erase values
		CU_ASSERT( ubound( values ) < lbound( values ) )
	end sub

	sub editStrings overload( byval value as integer )
		CU_FAIL( "expected string array overload" )
	end sub

	sub editEmpty( byval values() as string )
		CU_ASSERT( ubound( values ) < lbound( values ) )
		redim values(7 to 8)
		values(7) = "new"
		CU_ASSERT( values(7) = "new" )
	end sub

	dim shared as integer copies, destructions

	type Owner
		value as integer ptr
		declare constructor( )
		declare constructor( byref rhs as const Owner )
		declare destructor( )
	end type

	constructor Owner( )
		value = new integer( 0 )
	end constructor

	constructor Owner( byref rhs as const Owner )
		value = new integer( *rhs.value )
		copies += 1
	end constructor

	destructor Owner( )
		delete value
		destructions += 1
	end destructor

	type Envelope
		member as Owner
		fixed(0 to 0) as Owner
		dynamic(any) as Owner
	end type

	sub assignEnvelope( byref lhs as Envelope, byref rhs as Envelope )
		lhs = rhs
	end sub

	function editEnvelope( byval item as Envelope ) as integer
		*item.member.value += 10
		*item.fixed(0).value += 20
		*item.dynamic(0).value += 30
		return *item.member.value + *item.fixed(0).value + *item.dynamic(0).value
	end function

	function editEnvelopes( byval items() as Envelope ) as integer
		*items(0).member.value += 10
		*items(0).fixed(0).value += 20
		*items(0).dynamic(0).value += 30
		return *items(0).member.value + *items(0).fixed(0).value + *items(0).dynamic(0).value
	end function

	type PrivateItem
	private:
		declare constructor( byref rhs as const PrivateItem )
	public:
		number as integer
		declare constructor( )
		declare destructor( )
	end type

	constructor PrivateItem( )
		number = 17
	end constructor

	constructor PrivateItem( byref rhs as const PrivateItem )
		number = rhs.number
	end constructor

	destructor PrivateItem( )
	end destructor

	type PrivateMember
		item as PrivateItem
	end type

	sub editOwners( byval values() as Owner, byval other() as Owner )
		*values(0).value = 20
		CU_ASSERT( *other(0).value = 10 )
		*other(1).value = 30
		exit sub
	end sub

	function editOwner( byval item as Owner ) as integer
		*item.value += 1
		return *item.value
	end function

	type MutableCopy
		value as integer
		declare constructor( )
		declare constructor( byref rhs as MutableCopy )
	end type

	constructor MutableCopy( )
	end constructor

	constructor MutableCopy( byref rhs as MutableCopy )
		value = rhs.value + 1
	end constructor

	function readMutableCopies( byval items() as MutableCopy ) as integer
		return items(0).value
	end function

	function editConstSource( byval values(any) as integer ) as integer
		values(0) += 1
		return values(0)
	end function

	type BaseItem extends object
		title as string
		declare virtual function describe( ) as string
	end type

	function BaseItem.describe( ) as string
		return title
	end function

	type DerivedItem extends BaseItem
		scores(any) as integer
		declare function describe( ) as string override
	end type

	function DerivedItem.describe( ) as string
		return title & str( scores(-1) )
	end function

	function editDerived( byval items() as DerivedItem ) as string
		items(0).title = "copy"
		items(0).scores(-1) += 1
		dim as BaseItem ptr dispatch = @items(0)
		return dispatch->describe()
	end function

	TEST( boundsAndIsolation )
		dim fixed(-2 to -1, 3 to 5) as integer
		fixed(-2, 3) = 42
		CU_ASSERT( editGrid( fixed() ) = 42 )
		CU_ASSERT( fixed(-2, 3) = 42 )

		redim values(-2 to -1, 3 to 5) as integer
		values(-2, 3) = 51
		dim as function( byval (any, any) as integer ) as integer edit = @editGrid
		CU_ASSERT( edit( byval values() ) = 51 )
		CU_ASSERT( forwardGrid( values() ) = 51 )
		CU_ASSERT( values(-2, 3) = 51 )
		CU_ASSERT( ubound( values, 2 ) = 5 )

		dim as ArrayFields fields
		fields.fixed(-2, 3) = 61
		redim fields.resizable(-2 to -1, 3 to 5)
		fields.resizable(-2, 3) = 62
		CU_ASSERT( editGrid( fields.fixed() ) = 61 )
		CU_ASSERT( editGrid( fields.resizable() ) = 62 )
		CU_ASSERT( fields.fixed(-2, 3) = 61 )
		CU_ASSERT( fields.resizable(-2, 3) = 62 )
	END_TEST

	TEST( stringsAndEmptyArrays )
		redim words(1 to 2) as string
		words(1) = left( "caller text", 6 )
		editStrings( byval words() )
		CU_ASSERT( words(1) = "caller" )
		CU_ASSERT( ubound( words ) = 2 )

		dim empty() as string
		editEmpty( empty() )
		CU_ASSERT( ubound( empty ) < lbound( empty ) )
	END_TEST

	TEST( explicitByref )
		redim values(-1 to 1) as integer
		dim as sub( byref (any) as integer ) edit = @editOriginal
		edit( values() )
		CU_ASSERT( values(-1) = 7 )
		CU_ASSERT( ubound( values ) = 2 )
	END_TEST

	TEST( elementCopyModes )
		dim items(0 to 0) as MutableCopy
		items(0).value = 6
		CU_ASSERT( readMutableCopies( items() ) = 7 )
		CU_ASSERT( items(0).value = 6 )

		dim readonly(0 to 0) as const integer = { 18 }
		CU_ASSERT( editConstSource( readonly() ) = 19 )
		CU_ASSERT( readonly(0) = 18 )
	END_TEST

	TEST( derivedElements )
		dim items(0 to 0) as DerivedItem
		dim as DerivedItem ptr item = @items(0)
		item->title = "original"
		redim item->scores(-1 to 0)
		item->scores(-1) = 9
		CU_ASSERT( editDerived( items() ) = "copy10" )
		CU_ASSERT( items(0).title = "original" )
		CU_ASSERT( items(0).scores(-1) = 9 )
	END_TEST

	TEST( nestedOwnership )
		'' A private copy constructor must not prevent default construction
		'' of a containing type.
		dim as PrivateMember holder
		CU_ASSERT( holder.item.number = 17 )

		dim samples(0 to 0) as Envelope
		dim as Envelope ptr original = @samples(0)
		redim original->dynamic(0 to 0)
		*original->member.value = 1
		*original->fixed(0).value = 2
		*original->dynamic(0).value = 3

		dim as integer beforeCopies = copies, beforeDestructions = destructions
		assignEnvelope( *original, *original )
		CU_ASSERT( copies = beforeCopies )
		CU_ASSERT( destructions = beforeDestructions )
		CU_ASSERT( editEnvelope( (byval *original) ) = 66 )
		CU_ASSERT( editEnvelopes( samples() ) = 66 )
		CU_ASSERT( *original->member.value = 1 )
		CU_ASSERT( *original->fixed(0).value = 2 )
		CU_ASSERT( *original->dynamic(0).value = 3 )
		CU_ASSERT( copies = beforeCopies + 6 )
	END_TEST

	TEST( owningElements )
		copies = 0
		destructions = 0
		scope
			dim items(0 to 1) as Owner
			*items(0).value = 10
			*items(1).value = 11
			editOwners( items(), byval items() )
			CU_ASSERT( copies = 4 )
			CU_ASSERT( destructions = 4 )
			CU_ASSERT( *items(0).value = 10 )
			CU_ASSERT( *items(1).value = 11 )

			CU_ASSERT( editOwner( (byval items(0)) ) = 11 )
			CU_ASSERT( *items(0).value = 10 )
			CU_ASSERT( copies = 5 )
			CU_ASSERT( destructions = 5 )
		end scope
		CU_ASSERT( destructions = 7 )
	END_TEST

END_SUITE

'' end of dim/byval-array.bas

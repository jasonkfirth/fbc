'' Project: FreeBASIC semantic sidecar fixtures
'' File: field-groups.bas
'' Purpose: Separate accepted source fields from compiler storage and members.
'' Responsibilities: Final counts, anonymous promotion, descriptors and nesting.
'' This file intentionally does NOT instantiate types or call prototypes.

type FieldGroup
	As Long FieldGroupValue, FieldGroupCount
	FieldGroupNumbers(0 to 2) as Long
	FieldGroupCallback as Function(byval Value as Long) as Long
	static FieldGroupCounter as Long
	declare sub FieldGroupMethod()
end type

type OuterFields
	OuterFieldsValue as Long
	union
		OuterFieldsSmall as Byte
		OuterFieldsLarge as Long
		type
			OuterFieldsInner as Long
		end type
	end union
end type

type BaseFields extends Object
	BaseFieldsValue as Long
end type
type DerivedFields extends BaseFields
	DerivedFieldsValue as Long
	DerivedFieldsCount as Long
	DerivedFieldsFlags as Long
end type

type DynamicFields
	DynamicFieldsValues(any) as Long
	DynamicFieldsCounts(any) as Long
	DynamicFieldsFlags(any) as Long
end type

type NestedOwner
	type InnerGroup
		InnerGroupValue as Long
		InnerGroupCount as Long
		InnerGroupFlags as Long
	end type
	NestedOwnerValue as Long
end type

type MethodOnly extends Object
	declare sub MethodOnlyCall()
end type

'' end of field-groups.bas

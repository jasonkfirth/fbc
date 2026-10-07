'' Project: FreeBASIC semantic-sidecar tests
'' File: macro-reference-origins.bas
'' Purpose: Preserve compiler-selected macro references without fake source spans.
'' Responsibilities: Fields, overload selection, nested/repeated expansion and modes.
'' This file intentionally does NOT infer references from replacement text.

namespace reference_policy
    type packet
        value as integer
    end type
    declare function pick overload(byval value as integer) as integer
    declare function pick overload(byref value as string) as integer
    function pick(byval value as integer) as integer
        return value
    end function
    function pick(byref value as string) as integer
        return len(value)
    end function
end namespace
#define READ_FIELD(item) item.value
#define CALL_PICK(item) reference_policy.pick(item)
#define NESTED_READ(item) READ_FIELD(item)
#define TWO_READS(item) (READ_FIELD(item) + READ_FIELD(item))
dim item as reference_policy.packet
dim number as integer = READ_FIELD(item)
number = NESTED_READ(item)
number = TWO_READS(item)
number = CALL_PICK(number)
dim text_value as string = "text"
number = CALL_PICK(text_value)
number = item.value
number = reference_policy.pick(number)
#if 0
number = CALL_PICK(number)
#endif

'' end of macro-reference-origins.bas

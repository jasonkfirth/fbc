'' Project: FreeBASIC semantic sidecar tests
'' File: procedure-typing.bas
'' Purpose: Preserve original named header grammar across canonical reuse.
'' Responsibilities: Visibility defaults, AS/suffix/implicit results and macros.
'' This file intentionally does NOT execute procedure bodies.
#lang "fblite"
defint a-z
option private
declare function OriginalPrototype%(byval original_name as integer)
function OriginalPrototype(byval renamed_value as integer) as integer
    return renamed_value
end function
declare function TypedPrototype(byval original_name as integer) as integer
function TypedPrototype(byval renamed_value as integer)
    return renamed_value
end function
public function PublicImplicit()
    return 3
end function
private function PrivateTyped() as integer
    return 4
end function
function SuffixResult%()
    return 5
end function
public sub ExportedHeader() export
end sub
function ContinuedResult( _
    byval value as integer)
    return value
end function
#define MakeFunction(proc_name) function proc_name()
MakeFunction(MacroImplicit)
    return 6
end function
function InlineResult(): function = 7: end function
'' end of procedure-typing.bas

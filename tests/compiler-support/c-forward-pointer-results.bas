'' FreeBASIC C backend regression, c-forward-pointer-results.bas
'' Checks pointer returns whose source aliases resolve after the function body.
'' Covers null, non-null, qualified and nested pointer results.
'' Native type spelling belongs to the C emitter.

type LateValue as DefinedValue

function emptyValue() as LateValue ptr
	function = 0
end function

function echoValue(byval value as LateValue ptr) as LateValue ptr
	function = value
end function

function echoConstValue(byval value as const LateValue ptr) as const LateValue ptr
	function = value
end function

function echoSlot(byval value as LateValue ptr ptr) as LateValue ptr ptr
	function = value
end function

type DefinedValue as integer

dim as DefinedValue value = 73
dim as DefinedValue ptr address = @value
if emptyValue() <> 0 then end 1
if echoValue(@value) <> @value then end 2
if *echoConstValue(@value) <> 73 then end 3
if echoSlot(@address) <> @address then end 4
if **echoSlot(@address) <> 73 then end 5
print "FORWARD_POINTER_RESULTS_PASS"

'' end of c-forward-pointer-results.bas

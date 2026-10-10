'' Project: FreeBASIC compiler regression tests
'' File: string/fixed-eight-byte-fill.bas
'' Purpose: Verify fixed-string initialization across sizes and storage origins.
'' Responsibilities: Complete space fills, adjacent fields and repeated local setup.
'' This file intentionally does NOT inspect generated instruction spelling.
'' TEST_MODE : COMPILE_AND_RUN_OK

#lang "fb"

'' Local fixed strings and empty string initializers produce space fills.
'' Read their bytes so a later assignment cannot hide an incomplete fill.
sub checkSpaces(byval storage as const ubyte ptr, byval byteCount as integer)

	for byteIndex as integer = 0 to byteCount - 1
		if storage[byteIndex] <> asc(" ") then
			print "FIXED_FILL_FAILURE", byteCount, byteIndex
			end 1
		end if
	next

end sub

type GuardedStorage
	beforeValue as ulong
	textValue as string * 8 = ""
	afterValue as ulong
end type

dim shared sharedText as string * 8 = ""
checkSpaces(cast(const ubyte ptr, @sharedText), len(sharedText))

for repetition as integer = 0 to 2

	dim firstText as string * 8, secondText as string * 8
	checkSpaces(cast(const ubyte ptr, @firstText), len(firstText))
	checkSpaces(cast(const ubyte ptr, @secondText), len(secondText))

	dim oneByte as string * 1, twoBytes as string * 2, threeBytes as string * 3
	dim fourBytes as string * 4, fiveBytes as string * 5, sixBytes as string * 6
	dim sevenBytes as string * 7, nineBytes as string * 9
	dim sixteenBytes as string * 16, sixtyFourBytes as string * 64
	checkSpaces(cast(const ubyte ptr, @oneByte), len(oneByte))
	checkSpaces(cast(const ubyte ptr, @twoBytes), len(twoBytes))
	checkSpaces(cast(const ubyte ptr, @threeBytes), len(threeBytes))
	checkSpaces(cast(const ubyte ptr, @fourBytes), len(fourBytes))
	checkSpaces(cast(const ubyte ptr, @fiveBytes), len(fiveBytes))
	checkSpaces(cast(const ubyte ptr, @sixBytes), len(sixBytes))
	checkSpaces(cast(const ubyte ptr, @sevenBytes), len(sevenBytes))
	checkSpaces(cast(const ubyte ptr, @nineBytes), len(nineBytes))
	checkSpaces(cast(const ubyte ptr, @sixteenBytes), len(sixteenBytes))
	checkSpaces(cast(const ubyte ptr, @sixtyFourBytes), len(sixtyFourBytes))

	dim recordValue as GuardedStorage
	checkSpaces(cast(const ubyte ptr, @recordValue.textValue), len(recordValue.textValue))
	if recordValue.beforeValue <> 0 or recordValue.afterValue <> 0 then end 2

	dim heapValue as GuardedStorage ptr = new GuardedStorage
	if heapValue = 0 then end 3
	checkSpaces(cast(const ubyte ptr, @heapValue->textValue), len(heapValue->textValue))
	if heapValue->beforeValue <> 0 or heapValue->afterValue <> 0 then end 4
	delete heapValue

	'' Assignment must still use the same fixed storage after initialization.
	firstText = "alpha": secondText = "beta"
	if not (firstText < secondText) then end 5

next

print "FIXED_FILL_PASS"

'' end of string/fixed-eight-byte-fill.bas

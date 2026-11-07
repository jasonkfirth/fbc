/'
Project: FreeBASIC C error-owner lowering
File: backend/c/c-error-capture-layout.bi
Purpose: Group private capture fields without changing logical capture order.
Responsibilities: Preserve the runtime prefix and emit naturally aligned fields.
This file intentionally does NOT reorder initialization, cleanup or public records.
'/

private function hCErrCaptureAlignment(byval sym as FBSYMBOL ptr) as integer
	dim as integer dtype
	dim as FBSYMBOL ptr subtype
	symbGetRealType(sym, dtype, subtype)
	'' The native va_list replacement can have a different representation.
	'' Keep the whole declaration order when its layout is not owned here.
	if( symbIsBuiltinVaListType(dtype, subtype) ) then return 0
	if( typeIsPtr(dtype) ) then return env.pointersize
	var alignment = typeCalcNaturalAlign(dtype, subtype)
	if( typeGetDtAndPtrOnly(dtype) = FB_DATATYPE_STRUCT ) then
		alignment = hCRecordAlignment(subtype)
		if( symbGetUdtHasBitfield(subtype) andalso env.clopt.target <> FB_COMPTARGET_RISCOS ) then alignment = 1
	end if
	'' The grouping loop uses the target's power-of-two field alignments.
	select case as const alignment
	case 1, 2, 4, 8, 16
		return alignment
	end select
	return 0
end function

private sub hCErrCaptureField(byval index as integer)
	var sym = errorplan->captures[index]
	dim as string dimensions
	if( symbIsCArray(sym) ) then dimensions = hEmitArrayDecl(sym)
	if( errorplan->heap ) then
		hWriteLine(hCErrType(sym) + " value_" + str(index) + dimensions + ";")
	else
		var reference_name = "*reference_" + str(index)
		if( len(dimensions) ) then reference_name = "(" + reference_name + ")"
		hWriteLine(hCErrType(sym) + " " + reference_name + dimensions + ";")
	end if
end sub

private sub hCErrCaptureFields()
	'' Borrowed references all have pointer alignment. The owner prefix and
	'' resource flags also end at a pointer-aligned offset; keep them fixed.
	var grouped = errorplan->heap
	for i as integer = 0 to errorplan->capturecount - 1
		if( hCErrCaptureAlignment(errorplan->captures[i]) = 0 ) then grouped = FALSE
	next
	if( grouped = FALSE ) then
		for i as integer = 0 to errorplan->capturecount - 1
			hCErrCaptureField(i)
		next
		exit sub
	end if

	'' Descending alignment removes gaps in each run. A wider field is a
	'' barrier: moving it before a smaller field could grow a record whose
	'' fixed prefix is not aligned to that wider boundary. Its C sizeof is
	'' a multiple of its alignment, so the following run starts aligned again.
	'' Capture indices, initializers and cleanup still use their source order.
	var first = 0
	while( first < errorplan->capturecount )
		var limit = first
		while( limit < errorplan->capturecount )
			if( hCErrCaptureAlignment(errorplan->captures[limit]) > env.pointersize ) then exit while
			limit += 1
		wend
		var alignment = env.pointersize
		while( alignment > 0 )
			for i as integer = first to limit - 1
				if( hCErrCaptureAlignment(errorplan->captures[i]) = alignment ) then hCErrCaptureField(i)
			next
			alignment \= 2
		wend
		if( limit < errorplan->capturecount ) then hCErrCaptureField(limit)
		first = limit + 1
	wend
end sub

'' end of backend/c/c-error-capture-layout.bi

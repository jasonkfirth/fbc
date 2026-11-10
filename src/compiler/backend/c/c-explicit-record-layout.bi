/'
Project: FreeBASIC C backend record layout
File: backend/c/c-explicit-record-layout.bi
Purpose: Preserve exact field offsets and container extents in explicit C layouts.
Responsibilities: Walk promoted anonymous members and retain named field guards.
This file intentionally does NOT change BASIC layout, initialization or ownership.
'/

private function hCExplicitRecordLayout( byval udt as FBSYMBOL ptr ) as integer
	if( symbGetUdtHasBitfield( udt ) ) then return FALSE
	var packing = symbGetUDTAlign( udt )
	var managed = FALSE
	var fld = symbUdtGetFirstField( udt )
	while( fld <> NULL )
		managed or= hFieldNeedsRuntimeAlignment( fld )
		var parent = fld->parent
		while( parent <> udt )
			'' Anonymous members retain their own FIELD=N. The old field
			'' writer applies the outer packing to every promoted leaf.
			if( symbGetUDTAlign( parent ) <> packing ) then return TRUE
			parent = parent->parent
			assert( parent <> NULL )
		wend
		fld = symbUdtGetNextField( fld )
	wend
	return managed andalso (packing > 0) andalso _
	       (packing < typeCalcNaturalAlign( FB_DATATYPE_STRUCT, udt ))
end function

private sub hEmitExplicitRecordEnd( byval alignment as integer, byval naturalcalignment as integer )
	var attributes = "aligned(" + str( alignment ) + ")"
	'' A packed anonymous child can have lower C alignment than its BASIC
	'' natural alignment. Only decrease alignment when an emitted member
	'' actually requires it; aligned(N) alone also rounds the final size.
	if( env.clopt.target = FB_COMPTARGET_RISCOS orelse naturalcalignment > alignment ) then
		attributes = "packed, " + attributes
	end if
	hWriteLine( "} __attribute__((" + attributes + "));", TRUE )
end sub

private function hCImmediateRecordMember _
	( byval owner as FBSYMBOL ptr, byval fld as FBSYMBOL ptr ) as FBSYMBOL ptr
	'' Promoted leaves keep their original parent links after their symbol
	'' tables have moved. Find the direct field or anonymous child of owner.
	var member = fld
	while( member <> NULL )
		if( member->parent = owner ) then return member
		member = member->parent
	wend
	return NULL
end function

private function hCEmittedFieldAlignment( byval fld as FBSYMBOL ptr ) as integer
	var dtype = symbGetType( fld )
	var subtype = symbGetSubtype( fld )
	var alignment = typeCalcNaturalAlign( dtype, subtype )
	if( typeGetDtAndPtrOnly( dtype ) = FB_DATATYPE_STRUCT ) then
		alignment = hCRecordAlignment( subtype )
		if( symbGetUdtHasBitfield( subtype ) andalso env.clopt.target <> FB_COMPTARGET_RISCOS ) then
			'' Bitfield records use a byte-array C body.
			alignment = 1
		end if
	end if
	if( fld->var_.align > 0 ) then alignment = fld->var_.align
	return alignment
end function

private function hCEmittedRecordAlignment _
	( byval owner as FBSYMBOL ptr, byval fld as FBSYMBOL ptr ) as integer
	'' Anonymous tables have been flattened. Their emitted types receive
	'' their own native alignment; ordinary fields retain their C type's
	'' alignment. Determine whether this container actually needs packing
	'' before deciding which of its gaps C can supply implicitly.
	var alignment = 1
	var previouschild = cptr( FBSYMBOL ptr, NULL )
	while( fld <> NULL )
		var member = hCImmediateRecordMember( owner, fld )
		if( member = NULL ) then exit while
		if( symbIsDynamic( fld ) = FALSE ) then
			var memberalignment = 1
			if( member = fld ) then
				memberalignment = hCEmittedFieldAlignment( fld )
			elseif( member <> previouschild ) then
				memberalignment = hCRecordAlignment( member )
				previouschild = member
			end if
			if( memberalignment > alignment ) then alignment = memberalignment
		end if
		fld = symbUdtGetNextField( fld )
	wend
	return alignment
end function

private function hEmitExplicitRecordBody _
	( byval root as FBSYMBOL ptr, byval owner as FBSYMBOL ptr, _
	  byref fld as FBSYMBOL ptr, byval baseoffset as longint, byref paddingid as integer ) as integer
	'' The flattened list records absolute leaf offsets. Every anonymous
	'' container starts with a leaf at local offset zero, and its saved size
	'' includes its own trailing padding. Count the whole child extent before
	'' placing the next sibling, including inactive branches of a union.
	dim as longint fieldend = 0
	var naturalcalignment = hCEmittedRecordAlignment( owner, fld )
	var packedbody = (env.clopt.target = FB_COMPTARGET_RISCOS) orelse _
	                 (naturalcalignment > hCRecordAlignment( owner ))
	while( fld <> NULL )
		var member = hCImmediateRecordMember( owner, fld )
		if( member = NULL ) then exit while
		if( symbIsDynamic( fld ) ) then
			fld = symbUdtGetNextField( fld )
			continue while
		end if

		dim as integer memberalignment = any
		if( member = fld ) then
			memberalignment = hCEmittedFieldAlignment( fld )
		else
			memberalignment = hCRecordAlignment( member )
		end if
		var offset = symbGetOfs( fld ) - baseoffset
		if( symbGetUDTIsUnion( owner ) = FALSE ) then
			var gap = offset - fieldend
			assert( gap >= 0 )
			var calignment = iif( packedbody, 1, memberalignment )
			var implicitoffset = (fieldend + calignment - 1) and -clngint( calignment )
			assert( implicitoffset <= offset )
			if( gap > 0 andalso implicitoffset <> offset ) then
				'' Unnecessary named pads make otherwise compatible C record
				'' declarations differ under LTO. Keep ordinary C padding when
				'' it already places the next member at the required offset.
				var identifier = "fb_c_descriptor_pad_" + str( fieldend )
				if( owner <> root ) then
					'' Anonymous names are promoted too. Separate pads from
					'' different branches even when their local offsets match.
					paddingid += 1
					identifier += "_anon_" + str( paddingid )
				end if
				hWriteLine( "uint8 " + identifier + "[" + str( gap ) + "];", TRUE )
			end if
		end if

		dim as longint extent = any
		if( member = fld ) then
			extent = symbGetRealSize( fld )
			hEmitCRecordField( fld, symbGetUDTAlign( owner ), TRUE )
			fld = symbUdtGetNextField( fld )
		else
			assert( symbIsStruct( member ) )
			extent = symbGetSizeOf( member )
			if( symbGetUDTIsUnion( member ) ) then
				hWriteLine( "union {", TRUE )
			else
				hWriteLine( "struct {", TRUE )
			end if
			sectionIndent( )
			var childcalignment = hEmitExplicitRecordBody( root, member, fld, baseoffset + offset, paddingid )
			sectionUnindent( )
			hEmitExplicitRecordEnd( memberalignment, childcalignment )
		end if
		var memberend = offset + extent
		assert( memberend <= symbGetSizeOf( owner ) )
		if( memberend > fieldend ) then fieldend = memberend
	wend
	return naturalcalignment
end function

'' end of backend/c/c-explicit-record-layout.bi

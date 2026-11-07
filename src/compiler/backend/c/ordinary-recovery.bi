'' Project: FreeBASIC C ERROR owner recovery
'' File: backend/c/ordinary-recovery.bi
'' Purpose: Return proved recovery edges to the original owner activation.
'' Responsibilities: Own pending boundaries and keep resumable paths in callbacks.
'' This file intentionally does NOT infer destinations of opaque transfers.

sub irhlcErrorRegisterRecoveryBoundary( byval proc as FBSYMBOL ptr, _
	byval label as FBSYMBOL ptr, byval branch as ASTNODE ptr, byval target as FBSYMBOL ptr )
	var entry = cptr(C_ERROR_RECOVERY ptr, callocate(sizeof(C_ERROR_RECOVERY)))
	if( entry = NULL ) then
		errReportEx(FB_ERRMSG_INTERNAL, "C recovery boundary allocation")
		exit sub
	end if
	entry->owner = proc
	entry->label = label
	entry->branch = branch
	entry->target = target
	entry->next = errorrecoveries
	errorrecoveries = entry
end sub

private sub hCErrDropRecoveries( byref first as C_ERROR_RECOVERY ptr )
	while( first <> NULL )
		var entry = first
		first = entry->next
		deallocate entry
	wend
end sub

private function hCErrTakeRecoveries( byval proc as FBSYMBOL ptr ) as C_ERROR_RECOVERY ptr
	dim as C_ERROR_RECOVERY ptr first = NULL
	var link = @errorrecoveries
	while( *link <> NULL )
		var entry = *link
		if( entry->owner = proc ) then
			*link = entry->next
			entry->next = first
			first = entry
		else
			link = @entry->next
		end if
	wend
	return first
end function

private function hCErrRecoveryTarget( byval label as FBSYMBOL ptr ) as integer
	if( label = errorplan->exit_label andalso errorplan->exit_target_id > 0 ) then return errorplan->exit_target_id
	var entry = errorplan->recoveries
	while( entry <> NULL )
		if( entry->label = label ) then return entry->target_id
		entry = entry->next
	wend
	return 0
end function

private function hCErrRecoveryTreeSafe(byval n as ASTNODE ptr) as integer
	if( n = NULL ) then return TRUE
	if( n->class = AST_NODECLASS_CALL ) then
		'' A source or foreign callee can RESUME an active error, directly or
		'' through another call. Without a callee summary it must stay outlined.
		if( n->sym = NULL orelse n->l <> NULL orelse symbGetIsRTL(n->sym) = FALSE ) then return FALSE
		var identifier = hCErrName(n)
		'' String primitives have no user callbacks. Keep the remaining list
		'' explicit rather than assume every runtime entry is callback-free.
		if( left(identifier, 6) <> "fb_Str" and left(identifier, 7) <> "fb_Wstr" ) then
			select case identifier
			case "fb_End", "fb_ErrorGetNum", "fb_ErrorSetNum", "fb_ErrorSetHandler", _
				"fb_ErrorThrowAt", "fb_ErrorThrowEx", "fb_ErrorSetModName", "fb_ErrorSetFuncName"
			case else
				return FALSE
			end select
		end if
	end if
	return hCErrRecoveryTreeSafe(n->l) andalso hCErrRecoveryTreeSafe(n->r)
end function

private function hCErrRecoveryEntries() as integer
	if( errorplan->recoveries = NULL and errorplan->handlercount = 0 ) then return TRUE
	'' Runtime targets are signed 32-bit IDs; target 1 remains final cleanup.
	if( errorplan->count > &h7FFFFFFD ) then return FALSE
	errorplan->recovery_ids = callocate(errorplan->count * sizeof(integer))
	var known = cptr(ubyte ptr, callocate(errorplan->count))
	dim as C_ERROR_HANDLER proof
	proof.live = callocate(errorplan->count)
	if( errorplan->recovery_ids = NULL or known = NULL or proof.live = NULL ) then
		deallocate known
		deallocate proof.live
		return FALSE
	end if
	dim as integer selected = FALSE
	var entry = errorplan->recoveries
	while( entry <> NULL )
		var boundary_index = hCErrLabelIndex(entry->label)
		var target_index = hCErrLabelIndex(entry->target)
		dim as integer source_index = -1
		for i as integer = 0 to errorplan->count - 1
			if( errorplan->statements[i] = entry->branch ) then source_index = i: exit for
		next
		dim as integer candidate = (boundary_index >= 0 and target_index >= 0 and source_index >= 0)
		if( candidate ) then
			candidate = (entry->target = errorplan->exit_label orelse (errorplan->original_live <> NULL andalso errorplan->original_live[target_index] <> 0))
			candidate and= (entry->branch->class = AST_NODECLASS_BRANCH andalso _
				entry->branch->op.op = AST_OP_JMP andalso entry->branch->op.ex = entry->target)
			candidate and= (entry->target <> errorplan->cleanup_label andalso entry->target->lbl.gosub = FALSE)
			for i as integer = 0 to errorplan->handlercount - 1
				if( errorplan->handlers[i].label = entry->target ) then candidate = FALSE
			next
			dim as integer from_handler = FALSE
			for i as integer = 0 to errorplan->handlercount - 1
				from_handler or= errorplan->handlers[i].live[source_index] <> 0
			next
			candidate and= from_handler
		end if
		if( candidate ) then
			'' The edge's own cleanup executes after the checkpoint transfer.
			'' A user destructor could still RESUME, so it needs a callee contract too.
			if( boundary_index >= source_index ) then
				candidate = FALSE
			else
				for i as integer = boundary_index + 1 to source_index - 1
					if( hCErrRecoveryTreeSafe(errorplan->statements[i]) = FALSE ) then candidate = FALSE: exit for
				next
			end if
		end if
		if( candidate ) then
			if( entry->target = errorplan->exit_label ) then known[target_index] = 1
			if( known[target_index] = 0 ) then
				for i as integer = 0 to errorplan->count - 1
					proof.live[i] = 0
				next
				proof.label = entry->target
				var safe = hCErrMark(@proof)
				if( safe ) then
					for i as integer = 0 to errorplan->count - 1
						if( proof.live[i] = 0 ) then continue for
						var n = errorplan->statements[i]
						if( hCErrResume(n) <> 0 ) then safe = FALSE: exit for
						'' A dynamic GOSUB return could reach another caller's RESUME.
						if( n->class = AST_NODECLASS_BRANCH andalso n->op.op = AST_OP_RET ) then safe = FALSE: exit for
						if( hCErrRecoveryTreeSafe(n) = FALSE ) then safe = FALSE: exit for
					next
				end if
				known[target_index] = iif(safe, 1, 2)
			end if
			if( known[target_index] = 1 ) then
				entry->target_id = boundary_index + 2
				errorplan->recovery_ids[boundary_index] = entry->target_id
				symbSetIsAccessed(entry->label)
				selected = TRUE
			end if
		end if
		if( entry->target_id = 0 and boundary_index >= 0 ) then
			'' Unselected synthetic boundaries have no emitted source entry.
			var n = errorplan->statements[boundary_index]
			n->class = AST_NODECLASS_NOP
			n->sym = NULL
		end if
		entry = entry->next
	wend
	deallocate proof.live
	deallocate known
	if( selected ) then
		for i as integer = 0 to errorplan->handlercount - 1
			var h = @errorplan->handlers[i]
			for j as integer = 0 to errorplan->count - 1
				h->live[j] = 0
			next
			if( hCErrMark(h) = FALSE ) then return FALSE
		next
		deallocate errorplan->original_live
		errorplan->original_live = NULL
		hCErrMarkOriginal()
	end if
	'' Unsafe early exits remain outlined so a destructor/callee can RESUME.
	'' Once that source cleanup has returned, leave through the owner's actual
	'' return label. Replaying its earlier cleanup boundary would destroy twice.
	var exit_index = hCErrLabelIndex(errorplan->exit_label)
	if( exit_index >= 0 ) then
		dim as integer late_exit = FALSE
		for i as integer = 0 to errorplan->handlercount - 1
			late_exit or= errorplan->handlers[i].live[exit_index] <> 0
		next
		if( late_exit ) then
			errorplan->exit_target_id = exit_index + 2
			errorplan->recovery_ids[exit_index] = errorplan->exit_target_id
			symbSetIsAccessed(errorplan->exit_label)
			deallocate errorplan->original_live
			errorplan->original_live = NULL
			hCErrMarkOriginal()
		end if
	end if
	return TRUE
end function

'' end of backend/c/ordinary-recovery.bi

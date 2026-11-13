'' Project: FreeBASIC C backend regression tests
'' File: c-debug-local-storage.bas
'' Purpose: Preserve unread source debug values without materializing TYPEOF scratch.
'' Responsibilities: Check effects, named locals and real record lifetimes.
'' This file intentionally does NOT read the locals inspected by the debugger.

#lang "fb"

extern "C"
	declare sub debugLocalCheckpoint alias "debug_local_checkpoint"()
end extern

dim shared as long valueCalls, factoryCalls, constructed, destroyed, liveObjects

type LocalOwner
	value as long
	declare constructor()
	declare destructor()
end type

constructor LocalOwner()
	constructed += 1
	liveObjects += 1
	this.value = 71
end constructor

destructor LocalOwner()
	destroyed += 1
	liveObjects -= 1
end destructor

private function makeLocalOwner() as LocalOwner
	factoryCalls += 1
	dim result as LocalOwner
	result.value = 83
	return result
end function

private function localValue() as long
	valueCalls += 1
	return 10 + valueCalls
end function

private sub exerciseDebugLocals()
	dim localInteger as long = localValue()
	dim localFloat as double = cdbl(localValue()) + 0.5
	dim localPointer as long ptr = 0
	dim localFlag as boolean = true
	dim owned as LocalOwner
	'' TYPEOF must neither call the factory nor construct its return scratch.
	'' Only this named instance has a constructor and destructor to execute.
	dim queriedOwner as typeof(makeLocalOwner)
	localInteger = localValue()
#ifdef DEBUG_OPAQUE_LOCAL_CONTROL
	'' Non-x86 C targets use GCC-style quoted assembly rather than Intel syntax.
	#ifdef __FB_X86__
		#if __FB_ASM__ = "intel"
	asm
		nop
	end asm
		#else
	asm "nop"
		#endif
	#else
	asm "nop"
	#endif
#endif
	debugLocalCheckpoint()
end sub

exerciseDebugLocals()
if valueCalls <> 3 or factoryCalls <> 0 then end 1
if constructed <> 2 or destroyed <> 2 or liveObjects <> 0 then end 2
print "C_DEBUG_LOCAL_STORAGE_PASS"

'' end of c-debug-local-storage.bas

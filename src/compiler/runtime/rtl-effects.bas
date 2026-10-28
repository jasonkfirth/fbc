'' Project: FreeBASIC compiler - runtime semantic contracts
'' File: runtime/rtl-effects.bas
'' Purpose: Describe effects of compiler-owned runtime registrations.
'' Responsibilities: Native family provenance and explicit conservative contracts.
'' This file intentionally does NOT contain: optimization policy or external name guesses.

#include once "core/fbint.bi"
#include once "runtime/rtl.bi"

function rtlSemanticEffects( byval domain as FB_RTL_SEMANTIC_DOMAIN, _
	byval procdef as const FB_RTL_PROCDEF ptr ) as ulongint
	if( (procdef = NULL) orElse (procdef->name = NULL) ) then return FB_RTL_EFFECT_UNKNOWN

	'' Names here select entries inside an actual compiler registration table.
	'' User declarations with the same spelling or alias never call this API.
	'' Unknown remains explicit where the compiler has no narrower contract.
	dim as string registration_name = *procdef->name
	select case domain
	case FB_RTL_DOMAIN_MEMORY
		select case registration_name
		case "allocate", "callocate": return FB_RTL_EFFECT_ALLOCATE
		case "reallocate": return FB_RTL_EFFECT_ALLOCATE or FB_RTL_EFFECT_RELEASE or FB_RTL_EFFECT_ARGUMENT_READ
		case "deallocate": return FB_RTL_EFFECT_RELEASE
		case FB_RTL_NULLPTRCHK
			return FB_RTL_EFFECT_ERROR_HANDLING or FB_RTL_EFFECT_RUNTIME_READ or FB_RTL_EFFECT_CONTROL_TRANSFER
		case FB_RTL_MEMSWAP, FB_RTL_MEMCOPYCLEAR, FB_RTL_MEMMOVE, FB_RTL_MEMCOPY
			return FB_RTL_EFFECT_ARGUMENT_READ or FB_RTL_EFFECT_ARGUMENT_WRITE
		case else: return FB_RTL_EFFECT_UNKNOWN
		end select
	case FB_RTL_DOMAIN_MATH
		'' These selected numeric declarations compute values without changing
		'' BASIC state. This contract is independent of CANBECLONED. Native
		'' floating environment/errno behavior is outside its language scope.
		select case registration_name
		case FB_RTL_ABS, FB_RTL_SGN, FB_RTL_FIX, FB_RTL_FRAC, FB_RTL_ACOS, FB_RTL_ASIN, FB_RTL_ATAN, _
			FB_RTL_ATAN2, FB_RTL_TAN, FB_RTL_POW, FB_RTL_DBL2ULONGINT, FB_RTL_LONGINTDIV, FB_RTL_LONGINTMOD, _
			FB_RTL_ULONGINTDIV, FB_RTL_ULONGINTMOD
			return FB_RTL_EFFECT_COMPUTATION
		case "rnd"
			return FB_RTL_EFFECT_COMPUTATION or FB_RTL_EFFECT_RUNTIME_READ or FB_RTL_EFFECT_RUNTIME_WRITE
		case "randomize"
			return FB_RTL_EFFECT_RUNTIME_WRITE
		case else: return FB_RTL_EFFECT_UNKNOWN
		end select
	case FB_RTL_DOMAIN_FILE
		return FB_RTL_EFFECT_EXTERNAL_IO or FB_RTL_EFFECT_UNKNOWN
	case FB_RTL_DOMAIN_PRINT, FB_RTL_DOMAIN_CONSOLE
		return FB_RTL_EFFECT_EXTERNAL_IO or FB_RTL_EFFECT_RUNTIME_WRITE or FB_RTL_EFFECT_UNKNOWN
	case FB_RTL_DOMAIN_GRAPHICS, FB_RTL_DOMAIN_SOUND
		return FB_RTL_EFFECT_EXTERNAL_IO or FB_RTL_EFFECT_UNKNOWN
	case FB_RTL_DOMAIN_ERROR
		return FB_RTL_EFFECT_ERROR_HANDLING or FB_RTL_EFFECT_UNKNOWN
	case FB_RTL_DOMAIN_PROFILE
		return FB_RTL_EFFECT_PROFILING or FB_RTL_EFFECT_RUNTIME_WRITE or FB_RTL_EFFECT_UNKNOWN
	case FB_RTL_DOMAIN_GOSUB
		return FB_RTL_EFFECT_CONTROL_TRANSFER or FB_RTL_EFFECT_RUNTIME_READ or FB_RTL_EFFECT_RUNTIME_WRITE or FB_RTL_EFFECT_UNKNOWN
	case FB_RTL_DOMAIN_DATA
		return FB_RTL_EFFECT_RUNTIME_READ or FB_RTL_EFFECT_RUNTIME_WRITE or FB_RTL_EFFECT_UNKNOWN
	case FB_RTL_DOMAIN_STRING
		select case registration_name
		case FB_RTL_STRINIT, FB_RTL_WSTRASSIGNAW_INIT, FB_RTL_STRASSIGN, FB_RTL_WSTRASSIGN, _
			FB_RTL_WSTRASSIGNWA, FB_RTL_WSTRASSIGNAW, FB_RTL_STRCONCATASSIGN, FB_RTL_WSTRCONCATASSIGN, _
			FB_RTL_STRASSIGNMID, FB_RTL_WSTRASSIGNMID, FB_RTL_STRLSET, FB_RTL_STRLSETANA, FB_RTL_STRLSETEX, _
			FB_RTL_WSTRLSET, FB_RTL_WSTRLSETEX, FB_RTL_STRRSET, FB_RTL_STRRSETANA, FB_RTL_STRRSETEX, _
			FB_RTL_WSTRRSET, FB_RTL_WSTRRSETEX, FB_RTL_STRSWAP, FB_RTL_WSTRSWAP
			return FB_RTL_EFFECT_ARGUMENT_READ or FB_RTL_EFFECT_ARGUMENT_WRITE or FB_RTL_EFFECT_ALLOCATE or FB_RTL_EFFECT_RELEASE or FB_RTL_EFFECT_UNKNOWN
		case FB_RTL_STRDELETE, FB_RTL_HSTRDELTEMP, FB_RTL_WSTRDELETE
			return FB_RTL_EFFECT_RELEASE or FB_RTL_EFFECT_ARGUMENT_WRITE or FB_RTL_EFFECT_UNKNOWN
		case else
			'' String helpers may consume temporary descriptors or allocate
			'' results even when their BASIC value operation is read-only.
			return FB_RTL_EFFECT_ARGUMENT_READ or FB_RTL_EFFECT_UNKNOWN
		end select
	case FB_RTL_DOMAIN_UNICODE
		select case registration_name
		case FB_RTL_USTRINIT, FB_RTL_USTRASSIGN, FB_RTL_USTRCONCATASSIGN, FB_RTL_USTRASSIGNMID, _
			FB_RTL_USTRLSET, FB_RTL_USTRRSET, FB_RTL_USTRINDEXSET
			return FB_RTL_EFFECT_ARGUMENT_READ or FB_RTL_EFFECT_ARGUMENT_WRITE or FB_RTL_EFFECT_ALLOCATE or FB_RTL_EFFECT_RELEASE or FB_RTL_EFFECT_UNKNOWN
		case FB_RTL_USTRFILELINEINPUT, FB_RTL_USTRINPUT
			return FB_RTL_EFFECT_EXTERNAL_IO or FB_RTL_EFFECT_ARGUMENT_WRITE or FB_RTL_EFFECT_UNKNOWN
		case FB_RTL_USTRPRINT, FB_RTL_USTRWRITE, FB_RTL_USTRPRINTUSGINIT, FB_RTL_USTRLPRINTUSGINIT, FB_RTL_USTRPRINTUSG
			return FB_RTL_EFFECT_EXTERNAL_IO or FB_RTL_EFFECT_ARGUMENT_READ or FB_RTL_EFFECT_UNKNOWN
		case else: return FB_RTL_EFFECT_ARGUMENT_READ or FB_RTL_EFFECT_UNKNOWN
		end select
	case FB_RTL_DOMAIN_ARRAY
		select case registration_name
		case FB_RTL_ARRAYSNGBOUNDCHK, FB_RTL_ARRAYBOUNDCHK, FB_RTL_ARRAYDIMENSIONCHK
			return FB_RTL_EFFECT_ARGUMENT_READ or FB_RTL_EFFECT_ERROR_HANDLING or FB_RTL_EFFECT_CONTROL_TRANSFER
		case FB_RTL_ARRAYLBOUND, FB_RTL_ARRAYUBOUND
			return FB_RTL_EFFECT_ARGUMENT_READ or FB_RTL_EFFECT_COMPUTATION
		case FB_RTL_ARRAYREDIM, FB_RTL_ARRAYREDIMPRESV, FB_RTL_ARRAYREDIM_OBJ, FB_RTL_ARRAYREDIMPRESV_OBJ, FB_RTL_ARRAYREDIMTO
			return FB_RTL_EFFECT_ARGUMENT_READ or FB_RTL_EFFECT_ARGUMENT_WRITE or FB_RTL_EFFECT_ALLOCATE or FB_RTL_EFFECT_RELEASE or FB_RTL_EFFECT_CODE_EXECUTION or FB_RTL_EFFECT_UNKNOWN
		case FB_RTL_ARRAYDESTRUCTOBJ, FB_RTL_ARRAYDESTRUCTSTR, FB_RTL_ARRAYERASE, FB_RTL_ARRAYERASEOBJ, FB_RTL_ARRAYERASESTR
			return FB_RTL_EFFECT_ARGUMENT_WRITE or FB_RTL_EFFECT_RELEASE or FB_RTL_EFFECT_CODE_EXECUTION or FB_RTL_EFFECT_UNKNOWN
		case else: return FB_RTL_EFFECT_ARGUMENT_READ or FB_RTL_EFFECT_ARGUMENT_WRITE or FB_RTL_EFFECT_UNKNOWN
		end select
	case FB_RTL_DOMAIN_OBJECT
		return FB_RTL_EFFECT_ARGUMENT_READ or FB_RTL_EFFECT_COMPUTATION
	case FB_RTL_DOMAIN_SERIALIZATION
		return FB_RTL_EFFECT_ARGUMENT_READ or FB_RTL_EFFECT_ARGUMENT_WRITE or FB_RTL_EFFECT_UNKNOWN
	case FB_RTL_DOMAIN_SYSTEM
		select case registration_name
		case FB_RTL_END, "system", "stop": return FB_RTL_EFFECT_PROCESS_EXIT
		case "threadcreate", "threadwait", "fb_ThreadCall", "mutexcreate", "mutexdestroy", "mutexlock", "mutexunlock", _
			"condcreate", "conddestroy", "condsignal", "condbroadcast", "condwait"
			return FB_RTL_EFFECT_SYNCHRONIZATION or FB_RTL_EFFECT_UNKNOWN
		case "shell", "run", "chain", "exec"
			return FB_RTL_EFFECT_CODE_EXECUTION or FB_RTL_EFFECT_EXTERNAL_IO or FB_RTL_EFFECT_UNKNOWN
		case "timer", "time", "date", "environ", "command", "curdir", "exepath"
			return FB_RTL_EFFECT_RUNTIME_READ or FB_RTL_EFFECT_UNKNOWN
		case "settime", "setdate", "setenviron"
			return FB_RTL_EFFECT_RUNTIME_WRITE or FB_RTL_EFFECT_UNKNOWN
		case else: return FB_RTL_EFFECT_UNKNOWN
		end select
	end select
	return FB_RTL_EFFECT_UNKNOWN
end function

'' end of runtime/rtl-effects.bas

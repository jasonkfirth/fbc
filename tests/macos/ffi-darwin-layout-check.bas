''
'' FreeBASIC macOS libffi tests
'' ----------------------------
''
'' File: ffi-darwin-layout-check.bas
''
'' Purpose:
''
''     Verify that ffi.bi selects the SDK libffi ABI and data layouts for
''     each supported 64-bit Darwin architecture.
''
'' Responsibilities:
''
''     - reject the x86 libffi target on Darwin AArch64
''     - check ffi_abi values against the matching SDK target header
''     - check public call-interface and closure structure layouts
''     - support an AArch64 translation check from an Intel Mac
''
'' This file intentionally does NOT contain:
''
''     - calls into libffi
''     - non-Darwin libffi layout assumptions
''     - test-runner orchestration
''

#include once "ffi.bi"

#ifndef __FB_DARWIN__
	#error "ffi-darwin-layout-check.bas requires the Darwin target"
#endif

#ifndef __FB_64BIT__
	#error "ffi-darwin-layout-check.bas requires a 64-bit target"
#endif

#assert FFI_BAD_ARGTYPE = 3

#ifdef FFI_GO_CLOSURES
	#error "Apple's system libffi does not export the Go-closure API"
#endif

#ifdef __FB_ARM__
	#ifndef AARCH64
		#error "Darwin AArch64 must select the libffi AARCH64 target"
	#endif
	#ifdef X86_DARWIN
		#error "Darwin AArch64 must not select the libffi X86_DARWIN target"
	#endif
	#ifdef X86_ANY
		#error "Darwin AArch64 must not enable x86-only libffi definitions"
	#endif
	#ifdef FFI_TARGET_SPECIFIC_STACK_SPACE_ALLOCATION
		#error "Darwin AArch64 must not enable x86 stack-space definitions"
	#endif
	#ifndef FFI_TARGET_SPECIFIC_VARIADIC
		#error "Darwin AArch64 requires the target-specific variadic ABI"
	#endif

	#assert FFI_FIRST_ABI = 0
	#assert FFI_SYSV = 1
	#assert FFI_WIN64 = 2
	#assert FFI_LAST_ABI = 3
	#assert FFI_DEFAULT_ABI = FFI_SYSV
	#assert FFI_CLOSURES = 1
	#assert FFI_LEGACY_CLOSURE_API = 0
	#assert FFI_NATIVE_RAW_API = 0
	#assert FFI_TRAMPOLINE_SIZE = 24
	#assert sizeof( ffi_arg ) = 8

	#assert sizeof( ffi_cif ) = 40
	#assert offsetof( ffi_cif, aarch64_nfixedargs ) = 32
	#assert sizeof( ffi_closure ) = 40
	#assert offsetof( ffi_closure, trampoline_table_entry ) = 8
	#assert offsetof( ffi_closure, cif ) = 16
	#assert offsetof( ffi_closure, user_data ) = 32
	#assert sizeof( ffi_raw_closure ) = 56
	#assert sizeof( ffi_java_raw_closure ) = 56
#else
	#ifndef X86_DARWIN
		#error "Darwin x86 must select the libffi X86_DARWIN target"
	#endif

	#assert FFI_FIRST_ABI = 1
	#assert FFI_UNIX64 = 2
	#assert FFI_DEFAULT_ABI = FFI_UNIX64
	#assert FFI_LEGACY_CLOSURE_API = 1
	#assert FFI_TRAMPOLINE_SIZE = 24
	#assert sizeof( ffi_arg ) = 8

	#assert sizeof( ffi_cif ) = 32
	#assert sizeof( ffi_closure ) = 48
	#assert offsetof( ffi_closure, cif ) = 24
	#assert offsetof( ffi_closure, user_data ) = 40
	#assert sizeof( ffi_raw_closure ) = 64
	#assert sizeof( ffi_java_raw_closure ) = 64
#endif

end 0

'' end of ffi-darwin-layout-check.bas

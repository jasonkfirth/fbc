' Project: FreeBASIC compiler tests
' --------------------------------
'
' File: cmdline-option-reflection.bas
'
' Purpose:
'
'     Verify intrinsic defines for compiler code-generation and ABI options.
'
' Responsibilities:
'
'     * check reflection of architecture, export, PIC, and entry settings
'     * check reflection of ABI and preprocessor command-line switches
'
' This test intentionally does NOT contain:
'
'     * a linked program, because -entry customizes its startup symbol
'
' TEST_MODE : COMPILE_ONLY_OK

#if defined(__FB_OPENBSD__) or defined(__FB_ANDROID__)
#cmdline "-pic"
#if __FB_PIC__ = 0
#error __FB_PIC__ should reflect -pic
#endif
#endif

#cmdline "-arch native -entry custom_entry -export -z gosub-setjmp -z no-thiscall -z no-fastcall -z valist-as-ptr -z retinflts -z nobuiltins -z optabstract -z nocmdline"

#if defined(__FB_OPENBSD__) or defined(__FB_ANDROID__)
#if __CMDLINE__ <> "-pic -arch native -entry custom_entry -export -z gosub-setjmp -z no-thiscall -z no-fastcall -z valist-as-ptr -z retinflts -z nobuiltins -z optabstract -z nocmdline"
#error __CMDLINE__ should survive an FBC restart
#endif
#else
#if __CMDLINE__ <> "-arch native -entry custom_entry -export -z gosub-setjmp -z no-thiscall -z no-fastcall -z valist-as-ptr -z retinflts -z nobuiltins -z optabstract -z nocmdline"
#error __CMDLINE__ should survive an FBC restart
#endif
#endif

#if __FB_ARCH__ = ""
#error __FB_ARCH__ should identify the target architecture
#endif

#if __FB_ARCH__ = "native"
#error __FB_ARCH__ should use the canonical architecture name
#endif

#if __FB_ENTRY__ <> "custom_entry"
#error __FB_ENTRY__ should reflect -entry
#endif

#if __FB_EXPORT__ = 0
#error __FB_EXPORT__ should reflect -export
#endif

#if __FB_GOSUB_SETJMP__ = 0
#error __FB_GOSUB_SETJMP__ should reflect -z gosub-setjmp
#endif

#if __FB_NO_THISCALL__ = 0
#error __FB_NO_THISCALL__ should reflect -z no-thiscall
#endif

#if __FB_NO_FASTCALL__ = 0
#error __FB_NO_FASTCALL__ should reflect -z no-fastcall
#endif

#if __FB_VALIST_AS_PTR__ = 0
#error __FB_VALIST_AS_PTR__ should reflect -z valist-as-ptr
#endif

#if __FB_RETURN_IN_FLTS__ = 0
#error __FB_RETURN_IN_FLTS__ should reflect -z retinflts
#endif

#if __FB_NOBUILTINS__ = 0
#error __FB_NOBUILTINS__ should reflect -z nobuiltins
#endif

#if __FB_OPTABSTRACT__ = 0
#error __FB_OPTABSTRACT__ should reflect -z optabstract
#endif

#if __FB_NOCMDLINE__ = 0
#error __FB_NOCMDLINE__ should reflect -z nocmdline
#endif

' end of cmdline-option-reflection.bas

'' Project: FreeBASIC C backend regression tests
'' File: c-opaque-nop.bi
'' Purpose: Keep a real ASM boundary without imposing an x86 instruction.
'' Responsibilities: Match the C emitter's raw and GNU string input dialects.
'' This file intentionally does NOT access registers or alter program values.
#ifndef FBC_TEST_OPAQUE_NOP_BI
#define FBC_TEST_OPAQUE_NOP_BI

'' __FB_ASM__ describes x86 syntax only. Other C targets use raw instruction
'' tokens by default; Darwin instead expects a GNU assembly string expression.
#if defined(__FB_X86__)
    #if __FB_ASM__ = "att"
        #define FBC_TEST_GNU_ASM_TEXT
    #endif
#elseif defined(__FB_DARWIN__)
    #define FBC_TEST_GNU_ASM_TEXT
#endif

#macro compiler_test_opaque_nop()
    asm
        #ifdef FBC_TEST_GNU_ASM_TEXT
            "nop" : : : "memory"
        #else
            nop
        #endif
    end asm
#endmacro

#endif
'' end of c-opaque-nop.bi

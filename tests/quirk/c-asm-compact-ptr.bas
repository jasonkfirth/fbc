' TEST_MODE : COMPILE_AND_RUN_OK

'' Project: FreeBASIC C assembly qualification
'' File: c-asm-compact-ptr.bas
'' Purpose: Exercise memory operands written without a space after PTR.
'' Responsibilities: Check operand widths, repeated references and both cases.
'' This file intentionally does NOT depend on application libraries.
#if defined(__FB_X86__)
#cmdline "-asm intel"

dim as ubyte source_byte = &h5A, result_byte = 0
dim as ushort source_word = &h3456, result_word = 0
dim as ulong source_dword = &h12345678, result_dword = 0
#ifdef __FB_64BIT__
    dim as ulongint source_qword = &h1122334455667788ull, result_qword = 0
#endif

asm
    mov al, byte ptr[source_byte]
    mov byte ptr[result_byte], al
    mov ax, word PTR[source_word]
    mov word PTR[result_word], ax
    mov eax, dword Ptr[source_dword]
    mov dword ptr[result_dword], eax
    mov eax, dword ptr [source_dword]
#ifdef __FB_64BIT__
    mov rax, qword ptr[source_qword]
    mov qword ptr[result_qword], rax
#endif
end asm

if result_byte <> source_byte then end 1
if result_word <> source_word then end 2
if result_dword <> source_dword then end 3
#ifdef __FB_64BIT__
    if result_qword <> source_qword then end 4
#endif
#endif
print "C_ASM_COMPACT_PTR_PASS"
'' end of c-asm-compact-ptr.bas

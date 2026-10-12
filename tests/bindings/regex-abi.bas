'' FreeBASIC native regex qualification
'' File: regex-abi.bas
'' Purpose: Compare binding layouts and execute the real SDK's three widths.
'' Responsibilities: Exercise imported allocator data and release native patterns.
'' This file contains no substitute SDK or guessed C layout.
#lang "fb"
#include once "pcre.bi"
#include once "pcre16.bi"
#include once "pcre32.bi"
#include once "regex.bi"

Dim extra_layout As pcre_extra
Print "PCRE"; SizeOf(UInteger); SizeOf(CULong); SizeOf(extra_layout); _
      OffsetOf(pcre_extra, study_data); OffsetOf(pcre_extra, match_limit); _
      OffsetOf(pcre_extra, tables); OffsetOf(pcre_extra, mark); SizeOf(pcre_callout_block); _
      OffsetOf(pcre_callout_block, subject); OffsetOf(pcre_callout_block, mark)
Print "TRE"; SizeOf(regex_t); SizeOf(regoff_t); SizeOf(regmatch_t); _
      OffsetOf(regex_t, re_nsub); OffsetOf(regex_t, value); SizeOf(regaparams_t); SizeOf(regamatch_t)

Dim message_text As Const ZString Ptr
Dim offset As Long
Dim vector(0 To 8) As Long
Dim code As pcre Ptr = pcre_compile("foo", 0, @message_text, @offset, NULL)
If code = NULL Then End 1
If pcre_exec(code, NULL, "foo", 3, 0, 0, @vector(0), 9) <> 1 Then End 1
pcre_free(code)
Dim text16(0 To 3) As UShort = {Asc("f"), Asc("o"), Asc("o"), 0}
Dim code16 As pcre16 Ptr = pcre16_compile(@text16(0), 0, @message_text, @offset, NULL)
If code16 = NULL Then End 1
If pcre16_exec(code16, NULL, @text16(0), 3, 0, 0, @vector(0), 9) <> 1 Then End 1
pcre16_free(code16)
Dim text32(0 To 3) As ULong = {Asc("f"), Asc("o"), Asc("o"), 0}
Dim code32 As pcre32 Ptr = pcre32_compile(@text32(0), 0, @message_text, @offset, NULL)
If code32 = NULL Then End 1
If pcre32_exec(code32, NULL, @text32(0), 3, 0, 0, @vector(0), 9) <> 1 Then End 1
pcre32_free(code32)
Print "PCRE_NATIVE_WIDTHS=PASS 8 16 32"

'' end of regex-abi.bas

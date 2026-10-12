'' FreeBASIC Aspell qualification
'' File: aspell-abi.bas
'' Purpose: Describe the binding's public layouts before checking words.
'' Responsibilities: C size and offset comparison on both pointer widths.
'' This file contains no dictionary or spelling engine implementation.
#lang "fb"
#include once "aspell.bi"
Print "ABI"; SizeOf(Any Ptr); SizeOf(AspellToken); OffsetOf(AspellToken,len); _
    SizeOf(AspellKeyInfo); OffsetOf(AspellKeyInfo,def); OffsetOf(AspellKeyInfo,desc); _
    OffsetOf(AspellKeyInfo,flags); OffsetOf(AspellKeyInfo,other_data); _
    SizeOf(AspellError); SizeOf(AspellErrorInfo); OffsetOf(AspellErrorInfo,num_parms); _
    OffsetOf(AspellErrorInfo,parms(0))
'' end of aspell-abi.bas

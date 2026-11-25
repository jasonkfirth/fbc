'' Project: FreeBASIC browser corpus
'' File: fb-corpus-lbasic-llvm-probe.bas
'' Purpose: Exercise the LLVM 14 C ABI used by L-BASIC on a Wasm host.
'' The program writes LLVM IR and bitcode for a call through a typed pointer.
'' The compiler frontend and the final linker are separate from this probe.

#lang "fb"

Extern "C"
    Declare Function LLVMModuleCreateWithName(ByVal moduleName As ZString Ptr) As Any Ptr
    Declare Sub LLVMDisposeModule(ByVal irModule As Any Ptr)
    Declare Function LLVMInt32Type() As Any Ptr
    Declare Function LLVMStructType(ByVal fields As Any Ptr Ptr, ByVal fieldCount As ULong, _
                                   ByVal packed As Long) As Any Ptr
    Declare Sub LLVMSetTarget(ByVal irModule As Any Ptr, ByVal triple As ZString Ptr)
    Declare Function LLVMCreateTargetData(ByVal layout As ZString Ptr) As Any Ptr
    Declare Sub LLVMDisposeTargetData(ByVal layout As Any Ptr)
    Declare Sub LLVMSetModuleDataLayout(ByVal irModule As Any Ptr, ByVal layout As Any Ptr)
    Declare Function LLVMOffsetOfElement(ByVal layout As Any Ptr, ByVal structureType As Any Ptr, _
                                        ByVal fieldIndex As ULong) As ULongInt
    '' size_t follows the host pointer width. The layout offset API above is
    '' explicitly unsigned long long even on a 32-bit host.
    Declare Function LLVMCreateMemoryBufferWithMemoryRangeCopy(ByVal contents As ZString Ptr, _
                                     ByVal byteCount As UInteger, ByVal bufferName As ZString Ptr) As Any Ptr
    Declare Function LLVMGetBufferSize(ByVal buffer As Any Ptr) As UInteger
    Declare Sub LLVMDisposeMemoryBuffer(ByVal buffer As Any Ptr)
    Declare Function LLVMFunctionType(ByVal resultType As Any Ptr, ByVal parameterTypes As Any Ptr Ptr, _
                                     ByVal parameterCount As ULong, ByVal variadic As Long) As Any Ptr
    Declare Function LLVMAddFunction(ByVal irModule As Any Ptr, ByVal functionName As ZString Ptr, _
                                     ByVal functionType As Any Ptr) As Any Ptr
    Declare Function LLVMAppendBasicBlock(ByVal procedureValue As Any Ptr, ByVal blockName As ZString Ptr) As Any Ptr
    Declare Function LLVMCreateBuilder() As Any Ptr
    Declare Sub LLVMPositionBuilderAtEnd(ByVal builder As Any Ptr, ByVal block As Any Ptr)
    Declare Sub LLVMDisposeBuilder(ByVal builder As Any Ptr)
    Declare Function LLVMGetParam(ByVal procedureValue As Any Ptr, ByVal parameterIndex As ULong) As Any Ptr
    Declare Function LLVMBuildAdd(ByVal builder As Any Ptr, ByVal leftValue As Any Ptr, ByVal rightValue As Any Ptr, _
                                  ByVal valueName As ZString Ptr) As Any Ptr
    Declare Function LLVMBuildRet(ByVal builder As Any Ptr, ByVal resultValue As Any Ptr) As Any Ptr
    Declare Function LLVMConstInt(ByVal integerType As Any Ptr, ByVal value As ULongInt, _
                                  ByVal signExtend As Long) As Any Ptr
    Declare Function LLVMBuildCall(ByVal builder As Any Ptr, ByVal callee As Any Ptr, ByVal arguments As Any Ptr Ptr, _
                                   ByVal argumentCount As ULong, ByVal valueName As ZString Ptr) As Any Ptr
    Declare Function LLVMVerifyModule(ByVal irModule As Any Ptr, ByVal failureAction As Long, _
                                      ByVal diagnostic As ZString Ptr Ptr) As Long
    Declare Function LLVMPrintModuleToFile(ByVal irModule As Any Ptr, ByVal filename As ZString Ptr, _
                                           ByVal diagnostic As ZString Ptr Ptr) As Long
    Declare Sub LLVMDisposeMessage(ByVal diagnostic As ZString Ptr)
    Declare Function LLVMWriteBitcodeToFile(ByVal irModule As Any Ptr, ByVal filename As ZString Ptr) As Long
End Extern

Private Function WriteProbe() As Long
    Dim As Any Ptr irModule = LLVMModuleCreateWithName("lbasic_llvm_probe")
    Dim As Any Ptr builder = LLVMCreateBuilder()
    Dim As Any Ptr integerType = LLVMInt32Type()
    Dim As Any Ptr parameterTypes(0 To 1) = {integerType, integerType}

    '' LLVM 14's WebAssemblyTargetMachine layout for wasm32 Emscripten.
    Dim As Any Ptr layout = LLVMCreateTargetData("e-m:e-p:32:32-p10:8:8-p20:8:8-i64:64-f128:64-n32:64-S128-ni:1:10:20")
    LLVMSetTarget(irModule, "wasm32-unknown-emscripten")
    LLVMSetModuleDataLayout(irModule, layout)
    Dim As Any Ptr pairType = LLVMStructType(@parameterTypes(0), 2, 0)
    Dim As ULongInt secondField = LLVMOffsetOfElement(layout, pairType, 1)
    LLVMDisposeTargetData(layout)

    Const sample = "llvm"
    Dim As Any Ptr buffer = LLVMCreateMemoryBufferWithMemoryRangeCopy(sample, Len(sample), "probe")
    Dim As UInteger copiedBytes = LLVMGetBufferSize(buffer)
    LLVMDisposeMemoryBuffer(buffer)
    If secondField <> SizeOf(Long) OrElse copiedBytes <> Len(sample) Then
        LLVMDisposeBuilder(builder)
        LLVMDisposeModule(irModule)
        Return 1
    End If

    Dim As Any Ptr sumType = LLVMFunctionType(integerType, @parameterTypes(0), 2, 0)
    Dim As Any Ptr sumProcedure = LLVMAddFunction(irModule, "probe_add", sumType)
    LLVMPositionBuilderAtEnd(builder, LLVMAppendBasicBlock(sumProcedure, "entry"))
    Dim As Any Ptr total = LLVMBuildAdd(builder, LLVMGetParam(sumProcedure, 0), LLVMGetParam(sumProcedure, 1), "sum")
    LLVMBuildRet(builder, total)

    '' L-BASIC's call builder passes an array of opaque value references.
    '' Exercise LLVMBuildCall so the probe also covers the typed-pointer API.
    Dim As Any Ptr entryType = LLVMFunctionType(integerType, 0, 0, 0)
    Dim As Any Ptr entryProcedure = LLVMAddFunction(irModule, "probe_answer", entryType)
    LLVMPositionBuilderAtEnd(builder, LLVMAppendBasicBlock(entryProcedure, "entry"))
    Dim As Any Ptr arguments(0 To 1) = {LLVMConstInt(integerType, 19, 0), LLVMConstInt(integerType, 23, 0)}
    total = LLVMBuildCall(builder, sumProcedure, @arguments(0), 2, "answer")
    LLVMBuildRet(builder, total)

    Const returnStatus As Long = 2  '' LLVMReturnStatusAction
    Dim As ZString Ptr diagnostic = 0
    Dim As Long status = LLVMVerifyModule(irModule, returnStatus, @diagnostic)
    If diagnostic <> 0 Then
        If status <> 0 Then Print *diagnostic
        LLVMDisposeMessage(diagnostic)
        diagnostic = 0
    End If
    If status = 0 Then status = LLVMPrintModuleToFile(irModule, "probe.ll", @diagnostic)
    If diagnostic <> 0 Then
        Print *diagnostic
        LLVMDisposeMessage(diagnostic)
    End If
    If status = 0 Then status = LLVMWriteBitcodeToFile(irModule, "probe.bc")
    LLVMDisposeBuilder(builder)
    LLVMDisposeModule(irModule)
    Return status
End Function

If WriteProbe() <> 0 Then End 1
Print "LBASIC_LLVM_PROBE_OK"

'' end of fb-corpus-lbasic-llvm-probe.bas

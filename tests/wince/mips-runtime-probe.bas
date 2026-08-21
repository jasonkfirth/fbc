''
'' FreeBASIC Windows CE MIPS validation
'' ------------------------------------
''
'' File: mips-runtime-probe.bas
''
'' Purpose:
''
''     Isolate runtime operations that do not return under MIPS Windows CE.
''
'' Responsibilities:
''
''     - select one runtime operation from the command line
''     - record durable checkpoints in the shared storage folder
''     - let the persistent campaign supervisor enforce process timeouts
''
'' This file intentionally does NOT contain:
''
''     - fbctests assertions
''     - emulator lifecycle control
''     - platform-runtime replacements
''

#Include "vbcompat.bi"

Dim Shared assignmentGlobal As Integer

Function AssignmentByRef() ByRef As Integer
	Function = assignmentGlobal
End Function

Function AssignmentDefault(ByVal value As Integer => 456) As Integer
	Function => value
End Function

Type AssignmentUDT Field => 1
	value As Integer => 789
	byteValue As Byte

	Declare Property ValueProperty() As Integer
	Declare Operator Cast() As Integer
End Type

Property AssignmentUDT.ValueProperty() As Integer
	Property => value
End Property

Operator AssignmentUDT.Cast() As Integer
	Operator => value
End Operator

Type PackedPlainUDT Field = 1
	value As Integer
	byteValue As Byte
End Type

Type AlignedInitializedUDT
	value As Integer = 789
	byteValue As Byte
End Type

Sub RecordCheckpoint(ByRef operation As Const String, ByRef checkpoint As Const String)
	Dim As Integer fileNumber = FreeFile()
	Dim As String path = "\Storage Card\mips-runtime-probe-" & operation & ".log"

	Open path For Append As #fileNumber
	Print #fileNumber, checkpoint
	Close #fileNumber
End Sub

Dim As String operation = Command(1)

If Len(operation) = 0 Then
	End 87
End If

RecordCheckpoint(operation, "start")

Select Case operation
	Case "baseline"
		Dim As Integer value = 10
		value += 1

	Case "val"
		Dim As Double value = Val("10")

	Case "valint"
		Dim As Integer value = ValInt("10")

	Case "vallng"
		Dim As LongInt value = ValLng("10")

	Case "valwstring"
		Dim As WString * 16 text = WStr("10")
		Dim As Double value = Val(text)

	Case "str"
		Dim As String text = Str(10)

	Case "format"
		Dim As String text = Format(1.5, "0.0")

	Case "acos"
		Dim As Double argument = -0.5
		Dim As Double runtimeValue = ACos(argument)
		Dim As Double constantValue = ACos(-0.5)
		Dim As Double identityValue = 2.0 * Atn(Sqr((1.0 - argument) / (1.0 + argument)))
		Dim As Double atan2Value = Atan2(Sqr(1.0 - (argument * argument)), argument)
		Dim As ULongInt runtimeBits = *CPtr(ULongInt Ptr, @runtimeValue)
		Dim As ULongInt constantBits = *CPtr(ULongInt Ptr, @constantValue)
		Dim As ULongInt identityBits = *CPtr(ULongInt Ptr, @identityValue)
		Dim As ULongInt atan2Bits = *CPtr(ULongInt Ptr, @atan2Value)
		RecordCheckpoint(operation, "runtime=" & Hex(runtimeBits, 16))
		RecordCheckpoint(operation, "constant=" & Hex(constantBits, 16))
		RecordCheckpoint(operation, "identity=" & Hex(identityBits, 16))
		RecordCheckpoint(operation, "atan2=" & Hex(atan2Bits, 16))

	Case "data"
		Dim As Integer value
		Restore ProbeData
		Read value

	Case "mkshort"
		Dim As String text = MKShort(10)

	Case "print"
		Print 10

	Case "openempty"
		Dim As Integer result = Open("", For Random, As #1, Len = 1)
		If result = 0 Then
			Close #1
		End If

	Case "swap-ss"
		Dim As String a = "1", b = "0"
		Swap a, b

	Case "swap-sf"
		Dim As String a = "1"
		Dim As String * 4 b = "0"
		Swap a, b

	Case "swap-sz"
		Dim As String a = "1"
		Dim As ZString * 5 b = "0"
		Swap a, b

	Case "swap-fs"
		Dim As String * 4 a = "1"
		Dim As String b = "0"
		Swap a, b

	Case "swap-ff"
		Dim As String * 4 a = "1", b = "0"
		Swap a, b

	Case "swap-fz"
		Dim As String * 4 a = "1"
		Dim As ZString * 5 b = "0"
		Swap a, b

	Case "swap-zs"
		Dim As ZString * 5 a = "1"
		Dim As String b = "0"
		Swap a, b

	Case "swap-zf"
		Dim As ZString * 5 a = "1"
		Dim As String * 4 b = "0"
		Swap a, b

	Case "swap-zz"
		Dim As ZString * 5 a = "1", b = "0"
		Swap a, b

	Case "swap-ww"
		Dim As WString * 5 a = WStr("1"), b = WStr("0")
		Swap a, b

	Case "assign-byref"
		assignmentGlobal = 0
		AssignmentByRef() => 1

	Case "assign-string"
		Dim As String text
		text => "abc"
		text +=> "abc"
		text + => "abc"

	Case "assign-loop"
		Dim As Integer total
		Dim As Integer outer
		For outer => 1 To 2
			For inner As Integer => 1 To 2
				total += 1
			Next
		Next

	Case "assign-fixed"
		Dim fixedText As String * 5 => "abc"

	Case "assign-default"
		Dim As Integer value = AssignmentDefault()

	Case "assign-udt"
		Dim item As AssignmentUDT
		Dim As Integer value = item.ValueProperty + CInt(item)

	Case "assign-udt-init"
		Dim item As AssignmentUDT

	Case "assign-udt-property"
		Dim item As AssignmentUDT
		Dim As Integer value = item.ValueProperty

	Case "assign-udt-cast"
		Dim item As AssignmentUDT
		Dim As Integer value = CInt(item)

	Case "assign-udt-packed-plain"
		Dim item As PackedPlainUDT

	Case "assign-udt-aligned-init"
		Dim item As AlignedInitializedUDT

	Case "assign-err"
		Err => 0

	Case "assign-mid"
		Dim As String text = "abc"
		Mid(text, 2) => "x"

	Case "assign-lrset"
		Dim As String text = "   "
		LSet text => "x"
		RSet text => "x"

	Case Else
		RecordCheckpoint(operation, "unknown operation")
		End 87
End Select

RecordCheckpoint(operation, "done")
End 0

ProbeData:
Data 10

'' end of mips-runtime-probe.bas

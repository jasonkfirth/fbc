/'
    FreeBASIC procedure optimization regression
    File c-procedure-noinline.bas
    Check contextual modifiers, prototype merging and procedure pointer calls.
    Native diagnostic policy belongs to the test runner.
'/

Declare Function audit_decl CDecl Alias "audit_decl" (ByVal value As Long) As Long NoInline
Declare Function audit_body CDecl Alias "audit_body" (ByVal value As Long) As Long

Function audit_decl CDecl Alias "audit_decl" (ByVal value As Long) As Long
	Return value + 3
End Function

Function audit_body CDecl Alias "audit_body" (ByVal value As Long) As Long NoInline
	Return value + 5
End Function

Namespace Ordinary
Function NoInline(ByVal value As Long) As Long
	Return value + 7
End Function
End Namespace

Function audit_identifier(ByVal NoInline As Long) As Long
	Return NoInline + 11
End Function

Sub audit_sub(ByRef value As Long) NoInline
	value += 13
End Sub

Type AuditMethods
	state As Long
	Declare Function value(ByVal sample As Long) As Long NoInline
End Type

Function AuditMethods.value(ByVal sample As Long) As Long
	Return sample + 17
End Function

Function audit_ref(ByRef value As Long) ByRef As Long NoInline
	Return value
End Function

Dim callback As Function CDecl(ByVal As Long) As Long = @audit_decl
Dim subject As AuditMethods
For sample As Long = -32 To 32
	If callback(sample) <> sample + 3 Then End 1
	callback = @audit_body
	If callback(sample) <> sample + 5 Then End 2
	callback = @audit_decl
	If Ordinary.NoInline(sample) <> sample + 7 Then End 3
	If audit_identifier(sample) <> sample + 11 Then End 4
	Dim value As Long = sample
	audit_sub(value)
	If value <> sample + 13 Then End 5
	If subject.value(sample) <> sample + 17 Then End 6
	Dim ByRef reference As Long = audit_ref(value)
	reference = sample + 19
	If value <> sample + 19 Then End 7
Next

Print "PROCEDURE_NOINLINE_PASS 455"

/' end of c-procedure-noinline.bas '/

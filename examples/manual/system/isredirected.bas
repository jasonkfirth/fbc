'' examples/manual/system/isredirected.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'ISREDIRECTED'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=KeyPgIsredirected
'' --------

'' A Windows based example, just for the use principle
'' Self-sufficient example, using his own .exe file as dummy input file for stdin redirection

#include "fbio.bi"

'' Quotation marks wrapping for compatibility with spaces in path name
Dim As String pathExe = """" & ExePath & """"
Dim As String executablePath = Command(0)
If Len(executablePath) = 0 Then
  Print "The executable path is unavailable"
  Sleep
  End 1
End If
If LCase(Right(executablePath, 4)) <> ".exe" Then executablePath &= ".exe"
Dim As String quotedExecutable = """" & executablePath & """"
Dim As String launchCommand = "start """" /d " & pathExe & " /b " & _
  quotedExecutable & " < " & quotedExecutable & " secondprocess"

If Command() = "" Then  '' First process without stdin redirection
  '' Check stdin redirection
  Print "First process without stdin redirection: IsRedirected(-1) = "; IsRedirected(-1)
  '' Creation of asynchronous second process with stdin redirected from file.exe
  '' Both interpolated paths are derived from the current executable and are quoted.
  '' FB-LINTER: DISABLE-NEXT-LINE FBL-SEC-001 FBL-SEC-002
  Dim As Long launchResult = Shell(launchCommand)
  If launchResult <> 0 Then
    Print "Unable to start the redirected process: "; launchResult
    Sleep
    End 1
  End If
  '' Waiting for termination of asynchronous second process
  Sleep
ElseIf Command() = "secondprocess" Then  '' Second process with stdin redirection
  '' Check stdin redirection
  Print "Second process with stdin redirection  : IsRedirected(-1) = "; IsRedirected(-1)
End If

'' end of isredirected.bas

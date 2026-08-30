/'
    Project: FreeBASIC DOS Runtime
    --------------------------------

    File: maksymbr.bas

    Purpose:

        Generate the C export table used by the DOS dynamic-library runtime.

    Responsibilities:

        - parse the generator command line
        - read one or more symbol-list files
        - emit declarations and DXE export-table entries

    This file intentionally does NOT contain:

        - DXE binary generation
        - symbol discovery from libraries
        - runtime dynamic-library loading

    The build rule compiles this tool with -exx. Read and write failures after
    a successful Open therefore terminate with the runtime's file-I/O error.

    Ownership policy: every successfully opened input or output file is closed
    by this program before normal return or a handled error exit.
'/

'$LANG: "fblite"

Const MAX_INPUT_FILES As Integer = 100

Dim As String InputFiles(1 To MAX_INPUT_FILES)
Dim As Integer ArgumentIndex = 1
Dim As Integer NextOutput = FALSE
Dim As String OutputFile = ""
Dim As Integer InputCount = 0
Dim As Integer ShowHelp = FALSE
Dim As Integer OutBasic = FALSE

Do
    Dim As String Argument = Trim(Command(ArgumentIndex))
    If Len(Argument) = 0 Then Exit Do

    Select Case UCase(Argument)
    Case "-H", "-HELP"
        ShowHelp = TRUE
    Case "-O"
        NextOutput = TRUE
    Case "-B"
        OutBasic = TRUE
    Case Else
        If NextOutput Then
            OutputFile = Argument
            NextOutput = FALSE
        Else
            If InputCount >= MAX_INPUT_FILES Then
                Print "Too many input files; the maximum is "; MAX_INPUT_FILES
                End 254
            End If
            InputCount += 1
            InputFiles(InputCount) = Argument
        End If
    End Select

    ArgumentIndex += 1
Loop

If ShowHelp Then
    Print "Command format: MAKSYMBR -o output.c input.txt [input2.txt ...]"
    Print "output.c - output filename, input.txt - input file names"
    End 0
End If

If (Len(OutputFile) = 0) Or (InputCount <= 0) Or NextOutput Then
    Print "Invalid program call format."
    Print "For assistance, use the option -h"
    End 254
End If

Dim As Integer OutputHandle = FreeFile()
If Open(OutputFile, For Output, As #OutputHandle) <> 0 Then
    Print "Error creating output file: "; OutputFile
    End 253
End If

'' First pass: emit assembler symbol declarations.
For InputIndex As Integer = 1 To InputCount
    Dim As Integer InputHandle = FreeFile()
    If Open(InputFiles(InputIndex), For Input, As #InputHandle) <> 0 Then
        Print "Error opening input file "; InputFiles(InputIndex); " in the first pass"
        Close #OutputHandle
        End 252
    End If

    While Not Eof(InputHandle)
        Dim As String InputLine
        Line Input #InputHandle, InputLine
        InputLine = Trim(InputLine)
        Print #OutputHandle, "extern_asm(" + InputLine + ");"
    Wend

    Close #InputHandle
Next

Print #OutputHandle,
Print #OutputHandle, "DXE_EXPORT_TABLE (libfb_symbol_table)"

'' Second pass: emit the export table entries.
For InputIndex2 As Integer = 1 To InputCount
    Dim As Integer InputHandle2 = FreeFile()
    If Open(InputFiles(InputIndex2), For Input, As #InputHandle2) <> 0 Then
        Print "Error opening input file "; InputFiles(InputIndex2); " in the second pass"
        Close #OutputHandle
        End 248
    End If

    While Not Eof(InputHandle2)
        Dim As String InputLine2
        Line Input #InputHandle2, InputLine2
        InputLine2 = Trim(InputLine2)
        Print #OutputHandle, "    DXE_EXPORT_ASM (" + InputLine2 + ")"
    Wend

    Close #InputHandle2
Next

Print #OutputHandle, "DXE_EXPORT_END"
Close #OutputHandle

End 0

'' end of maksymbr.bas

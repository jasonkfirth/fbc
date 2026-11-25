' Project: FreeBASIC browser corpus
' File: fb-corpus-azdecrypt.bas
' Purpose: Run one English four-gram substitution job with AZdecrypt.
' Responsibilities: Validate ciphertext, load the native model and write the best result.
' The extracted AZdecrypt units own the search. The browser owns the worker lifecycle.

#lang "fb"
#include once "crt/string.bi"

Const constcip = 4000
Const constfrq = 100
Const ngram_size = 4
Const ngram_alphabet_size = 26
Const modelBytes = ngram_alphabet_size * ngram_alphabet_size * ngram_alphabet_size * ngram_alphabet_size
Const maxInputBytes = 32768

' Native table order is AAAA, AAAB, and so on through ZZZZ. The last
' index varies fastest in the original FreeBASIC array.
Dim Shared As UByte g4(25,25,25,25), g4b()
Dim Shared As Long ioctable(constcip)

#include "solver_random.bi"
#include "solver_statistics.bi"

Private Function LoadEnglishModel() As Boolean
    Dim model As Integer = FreeFile()
    If Open("/english-fourgrams.bin" For Binary Access Read As #model) <> 0 Then Return False
    If Lof(model) <> modelBytes Then
        Close #model
        Return False
    End If
    Dim As Integer readStatus = Get(#model, 1, g4())
    Close #model
    If readStatus <> 0 Then Return False

    Dim As Integer i, x1, x2, x3
    #include "thread_load_best_ngram.bi"
    For i = 0 To UBound(ioctable)
        ioctable(i) = i * (i - 1)
    Next i
    Return True
End Function

Private Function SolveSubstitution(cipherSymbols() As Long, ByVal l As Integer, ByVal s As Integer, _
                                   ByVal random_restarts As Integer, ByVal seed As ULongInt, _
                                   ByRef plaintext As String, ByRef tested As UInteger) As Double
    Const abc_size = ngram_alphabet_size, abc_sizem1 = abc_size - 1
    Const temp1 = 325, ngramfactor = 68.0
    Const lvmax = 3, iterations_total = 600000, tempdiv = 3
    Dim As Byte sr(3) = {0, 3, 2, 1}

    Dim As Integer e, h, i, j, k, lv, lr, rr, mi, mj, accept, frcmax
    Dim As Integer new_letter, old_letter, curr_symbol, new_ngram_score, old_ngram_score
    Dim As UInteger it, iterations
    ' Wasm32 has a 32-bit UInteger. The native generator needs the upper
    ' bits of both products before it masks or shifts them.
    Dim As ULongInt state = 2 * seed + 1
    Dim As Double d, mc, mc_minus, entropy, old_entropy, ent_score_norm
    Dim As Double new_score, old_score, best_score, start_temp, temp, temp_min, curr_temp, ngf, ngfal, onesixl
    Dim As Integer al = l - (ngram_size - 1)
    Dim As UByte key1(constfrq), key2(constfrq), stl(constfrq), sol(constcip), bsol(constcip), ngrams(constcip)
    Dim As Long frq(constfrq)
    Dim As Short nba(constcip), frc(constfrq), maps(constfrq), maps2(constfrq)
    Dim As Short mape1(constcip), mape2(constfrq)
    Dim As Double enttable(constcip * ngram_size)

    For i = 1 To l * ngram_size
        Dim probability As Double = i / (l * ngram_size)
        enttable(i) = Abs(logbx(probability, 2) * probability)
    Next i
    #include "solver_search.bi"

    If best_score > 0 Then
        plaintext = Space(l)
        For i = 1 To l
            plaintext[i - 1] = Asc("A") + bsol(i)
        Next i
    End If
    Return best_score
End Function

Dim As Integer source = FreeFile()
If Open("/cipher.txt" For Binary Access Read As #source) <> 0 Then
    Print "Could not open the ciphertext."
    End 2
End If
Dim As LongInt lengthInBytes = Lof(source)
If lengthInBytes < 40 OrElse lengthInBytes > maxInputBytes Then
    Close #source
    End 3
End If
Dim As String encrypted = Input(CInt(lengthInBytes), #source)
Close #source
If Len(encrypted) <> lengthInBytes Then
    Print "Could not read the complete ciphertext."
    End 2
End If

Dim As Long cipherSymbols(constcip)
Dim As Integer symbolNumbers(25), letterCount, symbolCount
For offset As Integer = 0 To Len(encrypted) - 1
    Dim As Integer character = encrypted[offset]
    If character >= Asc("a") AndAlso character <= Asc("z") Then character -= Asc("a") - Asc("A")
    If character < Asc("A") OrElse character > Asc("Z") Then Continue For
    If letterCount = constcip Then End 3
    Dim As Integer slot = character - Asc("A")
    If symbolNumbers(slot) = 0 Then
        symbolCount += 1
        symbolNumbers(slot) = symbolCount
    End If
    letterCount += 1
    cipherSymbols(letterCount) = symbolNumbers(slot)
Next offset
If letterCount < 40 OrElse symbolCount < 2 Then End 3
If Not LoadEnglishModel() Then
    Print "Could not load the English four-gram table."
    End 2
End If

Dim As Integer restarts = ValInt(Command(1))
Dim As ULongInt seed = ValInt(Command(2))
If restarts < 1 OrElse restarts > 4 OrElse seed > 1000000 Then
    Print "The solver settings are out of range."
    End 2
End If
Dim As UInteger tested
Dim As String plaintext
Dim score As Double = SolveSubstitution(cipherSymbols(), letterCount, symbolCount, restarts, seed, plaintext, tested)
If Len(plaintext) <> letterCount Then End 4

' AZdecrypt scores a continuous stream of letters. Restore the supplied
' spacing, letter case and punctuation after the search.
Dim As Integer nextLetter
For offset As Integer = 0 To Len(encrypted) - 1
    Dim As Integer character = encrypted[offset]
    Select Case character
    Case Asc("A") To Asc("Z")
        encrypted[offset] = plaintext[nextLetter]
        nextLetter += 1
    Case Asc("a") To Asc("z")
        encrypted[offset] = plaintext[nextLetter] + Asc("a") - Asc("A")
        nextLetter += 1
    End Select
Next offset

Dim As Integer result = FreeFile()
If Open("/solution.txt" For Binary Access Write As #result) <> 0 Then End 2
Dim As Integer writeStatus = Put(#result, 1, encrypted)
Close #result
If writeStatus <> 0 Then End 2

result = FreeFile()
If Open("/score.txt" For Output As #result) <> 0 Then End 2
Print #result, LTrim(Str(score))
Print #result, LTrim(Str(tested))
Close #result
End 0

' end of fb-corpus-azdecrypt.bas

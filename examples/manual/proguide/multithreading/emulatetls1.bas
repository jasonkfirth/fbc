'' examples/manual/proguide/multithreading/emulatetls1.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'Emulate a TLS (Thread Local Storage) and a TP (Thread Pooling) feature'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=ProPgEmulateTlsTp
'' --------

#include Once "crt/string.bi"
#include Once "fberror.bi"

'' Thread and resource ownership:
''
'' Each generated TLS accessor owns matching thread-handle and data slots.  The
'' zero slot remains a sentinel, while positive indices are live thread values.
'' Old runtime emulation serializes ThreadSelf refresh, access, and destruction
'' with threadData.mutex; current runtimes use their native ThreadSelf support.

#if __FB_VERSION__ < "1.08"
	' Emulation of the function Threadself() of FreeBASIC
	' Before each use, the thread must refresh this function value with its own thread handle,
	' and all of this (refreshing + use) protected by a mutex.
	Function ThreadSelf () ByRef As Any Ptr
		Static As Any Ptr handle
		Return handle
	End Function
#else
	#include Once "fbthread.bi"
#endif

#macro CreateTLSdatatypeVariableFunction (variable_function_name, variable_datatype)
' Creation of a "variable_function_name" function to emulate a static datatype variable (not an array),
' with a value depending on the thread using it.
	Namespace TLS
		Function variable_function_name (ByVal cd As Boolean = True) ByRef As variable_datatype
		' Function emulating (creation/access/destruction) a static datatype variable with value depending on thread using it:
			' If calling without parameter (or with 'True') parameter, this allows to [create and] access the static datatype variable.
			' If calling with the 'False' parameter, this allows to destroy the static datatype variable.
			Dim As Integer bound = 0
			Static As Any Ptr TLSindex(bound)
			Static As variable_datatype TLSdata(bound)
			Dim As Any Ptr Threadhandle = ThreadSelf()
			Dim As Integer index = 0
			For I As Integer = LBound(TLSindex) + 1 To UBound(TLSindex)  ' search existing TLS variable (existing array element) for the running thread
				If TLSindex(I) = Threadhandle Then
					index = I
					Exit For
				End If
			Next I
			If index = 0 And cd = True Then  ' create a new TLS variable (new array element) for a new thread
				'' The static zero slot is a sentinel, so this is capacity growth rather than traversal.
				'' FB-LINTER: DISABLE-NEXT-LINE FBL-ARR-004
				index = UBound(TLSindex) + 1
				ReDim Preserve TLSindex(index)
				TLSindex(index) = Threadhandle
				ReDim Preserve TLSdata(index)
			ElseIf index > 0 And cd = False Then  ' destroy a TLS variable (array element) and compact the array
				'' index is a live positive slot, and the sentinel keeps the dynamic arrays nonempty.
				'' FB-LINTER: DISABLE-NEXT-LINE FBL-ARR-004
				If index < UBound(TLSindex) Then  ' reorder the array elements
					'' The live TLS slot bounds the handle-table compaction move.
					'' FB-LINTER: DISABLE-NEXT-LINE FBL-ARR-004 FBL-PTR-013
					memmove(@TLSindex(index), @TLSindex(index + 1), (UBound(TLSindex) - index) * SizeOf(Any Ptr))
					Dim As variable_datatype Ptr p = Allocate(SizeOf(variable_datatype))  ' for compatibility to object with destructor
					If p = 0 Then Error FB.FB_RTERROR_OUTOFMEM
					'' The live TLS slot bounds all three byte moves below.
					'' FB-LINTER: DISABLE-NEXT-LINE FBL-ARR-004 FBL-PTR-013
					memmove(p, @TLSdata(index), SizeOf(variable_datatype))                ' for compatibility to object with destructor
					'' FB-LINTER: DISABLE-NEXT-LINE FBL-ARR-004
					memmove(@TLSdata(index), @TLSdata(index + 1), (UBound(TLSdata) - index) * SizeOf(variable_datatype))
					'' FB-LINTER: DISABLE-NEXT-LINE FBL-ARR-004
					memmove(@TLSdata(UBound(TLSdata)), p, SizeOf(variable_datatype))      ' for compatibility to object with destructor
					Deallocate(p)                                                         ' for compatibility to object with destructor
				End If
				'' The sentinel leaves one valid slot after removing a thread value.
				'' FB-LINTER: DISABLE-NEXT-LINE FBL-ARR-004
				ReDim Preserve TLSindex(UBound(TLSindex) - 1)
				'' FB-LINTER: DISABLE-NEXT-LINE FBL-ARR-004
				ReDim Preserve TLSdata(UBound(TLSdata) - 1)
				index = 0
			End If
			Return TLSdata(index)
		End Function
	End Namespace
#endmacro

'------------------------------------------------------------------------------

Type threadData
	Dim As Any Ptr handle
	Dim As String prefix
	Dim As String suffix
	Dim As Double tempo
	#if __FB_VERSION__ < "1.08"
		Static As Any Ptr mutex
	#endif
End Type
#if __FB_VERSION__ < "1.08"
	Dim As Any Ptr threadData.mutex
#endif

Namespace TLS
	'' The macro below defines this accessor; the declaration exposes its generated namespace to tools.
	Declare Function count (ByVal cd As Boolean = True) ByRef As Integer
End Namespace

CreateTLSdatatypeVariableFunction (count, Integer)  ' create a TLS static integer function

Function counter() As Integer  ' definition of a generic counter with counting depending on thread calling it
	TLS.count() += 1            ' increment the TLS static integer
	Return TLS.count()          ' return the TLS static integer
End Function

Sub Thread(ByVal p As Any Ptr)
	Dim As threadData Ptr ptd = p
	Dim As UInteger c
	Do
		#if __FB_VERSION__ < "1.08"
			MutexLock(threadData.mutex)
			ThreadSelf() = ptd->handle
		#endif
			c = counter()
		#if __FB_VERSION__ < "1.08"
			MutexUnlock(threadData.mutex)
		#endif
		Print ptd->prefix & c & ptd->suffix & " ";  ' single print with concatenated string avoids using a mutex
		Sleep ptd->tempo, 1
	Loop Until c = 12
	#if __FB_VERSION__ < "1.08"
		MutexLock(threadData.mutex)
		ThreadSelf() = ptd->handle
	#endif
	TLS.count(False)  ' destroy the TLS static integer
	#if __FB_VERSION__ < "1.08"
		MutexUnlock(threadData.mutex)
	#endif
End Sub

'------------------------------------------------------------------------------

Print "|x| : counting from thread a"
Print "(x) : counting from thread b"
Print "[x] : counting from thread c"
Print

#if __FB_VERSION__ < "1.08"
	threadData.mutex = MutexCreate()
#endif

Dim As threadData mtlsa
mtlsa.prefix = "|"
mtlsa.suffix = "|"
mtlsa.tempo = 100
#if __FB_VERSION__ < "1.08"
	MutexLock(threadData.mutex)
#endif
mtlsa.handle = ThreadCreate(@Thread, @mtlsa)
#if __FB_VERSION__ < "1.08"
	MutexUnlock(threadData.mutex)
#endif

Dim As threadData mtlsb
mtlsb.prefix = "("
mtlsb.suffix = ")"
mtlsb.tempo = 150
#if __FB_VERSION__ < "1.08"
	MutexLock(threadData.mutex)
#endif
mtlsb.handle = ThreadCreate(@Thread, @mtlsb)
#if __FB_VERSION__ < "1.08"
	MutexUnlock(threadData.mutex)
#endif

Dim As threadData mtlsc
mtlsc.prefix = "["
mtlsc.suffix = "]"
mtlsc.tempo = 250
#if __FB_VERSION__ < "1.08"
	MutexLock(threadData.mutex)
#endif
mtlsc.handle = ThreadCreate(@Thread, @mtlsc)
#if __FB_VERSION__ < "1.08"
	MutexUnlock(threadData.mutex)
#endif

ThreadWait(mtlsa.handle)
ThreadWait(mtlsb.handle)
ThreadWait(mtlsc.handle)
#if __FB_VERSION__ < "1.08"
	MutexDestroy(threadData.mutex)
#endif

Print
Print
Print "end of threads"

Sleep

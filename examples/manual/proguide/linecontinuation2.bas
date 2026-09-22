'' examples/manual/proguide/linecontinuation2.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'Line Continuation'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=ProPgLineContinuation
'' --------

'' Here's an example:

Declare Sub drawRectangle( ByVal x As Integer, ByVal y As Integer, ByVal w As Integer, ByVal h As Integer )

'' which can also be written as:

#if 0
'' The alternative layouts remain in source for the manual, but only one
'' declaration may participate in this compilable example.
Declare Sub drawRectangle( ByVal x As Integer, ByVal y As Integer, _
						   ByVal w As Integer, ByVal h As Integer )

'' or:

Declare Sub drawRectangle _
	( _
		ByVal x As Integer, _
		ByVal y As Integer, _
		ByVal w As Integer, _
		ByVal h As Integer _
	)
#endif

'' (or any other formatting you like)

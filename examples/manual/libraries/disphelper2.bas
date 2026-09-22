'' examples/manual/libraries/disphelper2.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'disphelper'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=ExtLibdisphelper
'' --------

'' IExplorer example

#define UNICODE
#include "disphelper/disphelper.bi"

Sub navigate(ByRef url As String)
	'' DISPATCH_OBJ expands to this nullable IDispatch declaration.  Spelling
	'' it out keeps the ownership and each HRESULT boundary visible.
	Dim As IDispatch Ptr ieApp = NULL

	If SUCCEEDED( dhInitialize( True ) ) Then
		If SUCCEEDED( dhToggleExceptions( True ) ) Then
			If SUCCEEDED( dhCreateObject( "InternetExplorer.Application", NULL, @ieApp ) ) Then
				If SUCCEEDED( dhPutValue( ieApp, "Visible = %b", True ) ) Then
					dhCallMethod( ieApp, ".Navigate(%s)", url )
				End If
			End If
		End If

		SAFE_RELEASE( ieApp )
		dhUninitialize( True )
	End If
End Sub

	navigate("www.freebasic.net")

' end of disphelper2.bas

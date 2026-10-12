'' FreeBASIC wx-c examples
'' File: fonts_and_colours.bas
'' Purpose: Apply fonts and colours to native wxWidgets controls.
'' Responsibilities: Widget construction and temporary resource ownership.
'' This file contains no widget implementation or custom event dispatcher.
#Include "wx-c/wx.bi"

Declare Function App_OnInit WXCALL ( ) As wxBool
Declare Function App_OnExit WXCALL ( ) As wxInt

' wxWidgets callbacks own and use this application widget graph.
' FB-LINTER: DISABLE-NEXT-LINE FBL301
Dim Shared As wxApp Ptr wx_app
Dim Shared As wxFrame Ptr wx_frame
Dim Shared As wxPanel Ptr wx_panel

Function App_OnInit WXCALL ( ) As wxBool

	Dim As wxString Ptr textArg = wxString_ctorUTF8( "Fonts and Colours" )
	wx_frame = wxFrame_ctor( )
	If wx_frame = 0 OrElse textArg = 0 Then Return WX_FALSE
	Dim As wxChar created = wxFrame_Create( wx_frame, WX_NULL, -1, textArg, -1, -1, 250, 100, wxFRAME_DEFAULT_STYLE Or wxCLOSE_BOX Xor wxMAXIMIZE_BOX Xor wxRESIZE_BORDER, WX_NULL )
	wxString_dtor( textArg )
	If created = 0 Then Return WX_FALSE
	wx_panel = wxPanel_ctor2( wx_frame, -1, -1, -1, -1, -1, 0, WX_NULL )
	If wx_panel = 0 Then Return WX_FALSE

	'' create global font
	Dim As wxFont Ptr globalFont = wxFont_ctor( 8, wxMODERN, wxNORMAL, wxNORMAL, 0, 0, 0 )
	If globalFont = 0 Then Return WX_FALSE
	wxFont_SetPointSize( globalFont, 12 )

	'' apply font for all widgets on wx_panel
	wxWindow_SetFont( wx_panel, globalFont )
	'' SetFont copies the font; our temporary wrapper is no longer needed.
	wxFont_dtor( globalFont )


	'' widget stattext
	Dim As wxStaticText Ptr stattext
	stattext = wxStaticText_ctor( )
	textArg = wxString_ctorUTF8( "Label:" )
	If stattext = 0 OrElse textArg = 0 Then Return WX_FALSE
	'' Let wxWidgets choose a height that fits the inherited 12-point font.
	created = wxStaticText_Create( stattext, wx_panel, -1, textArg, 10, 10, 80, -1, 0, 0 )
	wxString_dtor( textArg )
	If created = 0 Then Return WX_FALSE

	'' set colour for stattext
	Dim As wxColour Ptr sCol = wxColour_ctor( )
	If sCol = 0 Then Return WX_FALSE
	wxColour_Set( sCol, 255, 0, 0, 0 )
	wxWindow_SetForegroundColour( stattext, sCol )
	wxColour_dtor( sCol )


	'' widget textctrl
	Dim As wxTextCtrl Ptr textctrl
	textctrl = wxTextCtrl_ctor( )
	textArg = wxString_ctorUTF8( "Text" )
	If textctrl = 0 OrElse textArg = 0 Then Return WX_FALSE
	created = wxTextCtrl_Create( textctrl, wx_panel, -1, textArg, 10, 30, -1, -1, 0, 0, 0 )
	wxString_dtor( textArg )
	If created = 0 Then Return WX_FALSE

	'' wxFont_SMALL_FONT is borrowed from the library; do not destroy it.
	wxTextCtrl_SetFont( textctrl, wxFont_SMALL_FONT )


	'' set focus to wx_panel
	wxWindow_SetFocus( wx_panel )

	wxWindow_Show( wx_frame, 1 )
	wxApp_OnInit( wx_app )
	Return 1

End Function

Function App_OnExit WXCALL ( ) As wxInt

	Return wxApp_OnExit( wx_app )

End Function


''main
wx_app = wxApp_ctor( )
If wx_app = 0 Then End 1
wxApp_RegisterVirtual ( wx_app, @App_OnInit, @App_OnExit )
wxApp_Run( 0, 0 )

'' end of fonts_and_colours.bas

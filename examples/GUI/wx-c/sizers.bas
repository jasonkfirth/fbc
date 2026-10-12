'' FreeBASIC wx-c examples, by dumbledore
'' File: sizers.bas
'' Purpose: Lay out two buttons with a vertical box sizer.
'' Responsibilities: Layout callbacks and temporary title ownership.
'' This file contains no custom layout engine or event loop.

#Include "wx-c/wx.bi"

'' wxWidgets owns the attached sizer until its parent frame is destroyed.
	Dim Shared As wxApp Ptr app
	Dim Shared As wxSizer Ptr sizer

''
Sub sizer_OnDispose WXCALL ( )

  '' The parent has released the owned sizer; do not retain a usable handle.
  sizer = 0
End Sub

''
Sub sizer_OnRecalcSizes WXCALL ( )

  wxBoxSizer_RecalcSizes( sizer )

End Sub

''
Function App_OnInit WXCALL ( ) As wxBool

  Dim As wxString Ptr textArg = wxString_ctorUTF8("Welcome to WX-C")
    '' create the main window
  Dim As wxFrame Ptr frame = wxFrame_ctor( )
  If frame = 0 OrElse textArg = 0 Then Return WX_FALSE
  Dim As wxChar created = wxFrame_Create( frame, 0, 1, textArg, -1, -1, -1, -1, _
                    wxFRAME_DEFAULT_STYLE Or wxCLOSE_BOX, WX_NULL )
  wxString_dtor( textArg )
  If created = 0 Then Return WX_FALSE

    '' create a box sizer and set as the new layout
  sizer = wxBoxSizer_ctor( wxVERTICAL )
  If sizer = 0 Then Return WX_FALSE
  wxBoxSizer_RegisterVirtual( sizer, @sizer_OnRecalcSizes )
  wxBoxSizer_RegisterDisposable( sizer, @sizer_OnDispose )
  wxWindow_SetAutoLayout( frame, WX_TRUE )

    '' create button 1 and add to sizer
  Dim As wxButton Ptr button1 = wxButton_ctor( )
  textArg = wxString_ctorUTF8("Button1")
  If button1 = 0 OrElse textArg = 0 Then Return WX_FALSE
  created = wxButton_Create( button1, frame, 1, textArg, -1, -1, -1, -1, 0, 0, 0 )
  wxString_dtor( textArg )
  If created = 0 Then Return WX_FALSE
  '' Equal positive proportions share spare height; wxEXPAND fills the width.
  wxSizer_AddWindow( sizer, button1, 1, wxSizerStyle.wxEXPAND, 0, 0 )

    '' create button 2 and add to sizer
  Dim As wxButton Ptr button2 = wxButton_ctor( )
  textArg = wxString_ctorUTF8("Button2")
  If button2 = 0 OrElse textArg = 0 Then Return WX_FALSE
  created = wxButton_Create( button2, frame, 2, textArg, -1, -1, -1, -1, 0, 0, 0 )
  wxString_dtor( textArg )
  If created = 0 Then Return WX_FALSE
  wxSizer_AddWindow( sizer, button2, 1, wxSizerStyle.wxEXPAND, 0, 0 )

    ''
  wxWindow_SetSizerAndFit( frame, sizer, 0 )

    ''
  wxWindow_CenterOnScreen( frame, wxBOTH )

  wxWindow_Show( frame, 1 )

  Function = wxApp_OnInit( app )

End Function

''
Function App_OnExit WXCALL ( ) As wxInt

  Function = wxApp_OnExit( app )

End Function

'' main
  app = wxApp_ctor( )
  If app = 0 Then End 1
  wxApp_RegisterVirtual( app, @App_OnInit, @App_OnExit )
  wxApp_Run( 0, 0 )

'' end of sizers.bas

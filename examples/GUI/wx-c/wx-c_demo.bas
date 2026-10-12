'' FreeBASIC wx-c examples, by dumbledore
'' File: wx-c_demo.bas
'' Purpose: Open a modal dialog from a native button event.
'' Responsibilities: Callback ABI, parent-owned widgets and temporary values.
'' This file contains no widget implementation or custom event dispatcher.


#Include "wx-c/wx.bi"

Const FRAME_W = 500
Const FRAME_H = 400

''
'' ids
''
Enum
    ID_FRAME
    ID_PANEL
    ID_CLICKMEBUTTON
    ID_EXITBUTTON
    ID_DIAG_EXITBUTTON
End Enum

Declare Function app_oninit_cb WXCALL () As wxBool
Declare Function app_onexit_cb WXCALL () As wxInt

''
'' The GUI thread owns these borrowed handles throughout its event loop.
'' wxWidgets destroys the dialog and child controls with the parent frame.
''
Dim Shared As wxApp Ptr app
Dim Shared As wxFrame Ptr frame
Dim Shared As wxDialog Ptr dialog

'':::::
''
'' main
''
app = wxApp_ctor( )
If app = 0 Then End 1
wxApp_RegisterVirtual( app, @app_oninit_cb, @app_onexit_cb )
wxApp_Run( 0, 0 )

End 0

'':::::
''
'' dialog's exit button callback
''
Sub diag_exitbutton_cb WXCALL (ByVal event As wxEvent Ptr, ByVal iListener As wxInt)

    wxDialog_EndModal( dialog, 0 )

End Sub

'':::::
''
'' panel's click-me button callback
''
Sub panel_clickmebutton_cb WXCALL (ByVal event As wxEvent Ptr, ByVal iListener As wxInt)

    wxDialog_ShowModal( dialog )

End Sub

'':::::
''
'' panel's exit button callback
''
Sub panel_exitbutton_cb WXCALL (ByVal event As wxEvent Ptr, ByVal iListener As wxInt)

    wxWindow_Close( frame, 0 )

End Sub

'':::::
''
'' on init callback
''
Function app_oninit_cb WXCALL () As wxBool
    Dim As Integer ypos, size
    Dim As wxString Ptr textArg = wxString_ctorUTF8("Welcome to WX-C")

    '' create the main window
    frame = wxFrame_ctor( )
    If frame = 0 OrElse textArg = 0 Then Return WX_FALSE
    Dim As wxChar created = wxFrame_Create( frame, 0, ID_FRAME, textArg, _
        200, 200, FRAME_W, FRAME_H, _
        wxFRAME_DEFAULT_STYLE, WX_NULL )
    '' Create copies the label; the caller still owns its string wrapper.
    wxString_dtor( textArg )
    If created = 0 Then Return WX_FALSE

    '' make a dialog
    dialog = wxDialog_ctor( )
    textArg = wxString_ctorUTF8("Press Exit")
    If dialog = 0 OrElse textArg = 0 Then Return WX_FALSE
    created = wxDialog_Create( dialog, frame, -1, textArg, _
        10, 10, 500, 400, _
        wxFRAME_DEFAULT_STYLE, 0 )
    wxString_dtor( textArg )
    If created = 0 Then Return WX_FALSE

    '' add widgets to the dialog
    size = 27
    ypos=1

    textArg = wxString_ctorUTF8("&Goto")
    wxButton_Create( wxButton_ctor( ), dialog, -1, textArg, _
        405, ypos, 85, -1, 0, 0, 0 )
    wxString_dtor( textArg )
    ypos += size

    textArg = wxString_ctorUTF8("&Select")
    wxButton_Create( wxButton_ctor( ), dialog, -1, textArg, _
        405, ypos, 85, -1, 0, 0, 0 )
    wxString_dtor( textArg )
    ypos += size

    textArg = wxString_ctorUTF8("E&dit")
    wxButton_Create( wxButton_ctor( ), dialog, -1, textArg, _
        405, ypos, 85, -1, 0, 0, 0 )
    wxString_dtor( textArg )
    ypos += size

    textArg = wxString_ctorUTF8("&Add")
    wxButton_Create( wxButton_ctor( ), dialog, -1, textArg, _
        405, ypos, 85, -1, 0, 0, 0 )
    wxString_dtor( textArg )
    ypos += size

    Dim As wxButton Ptr diag_exitbutton = wxButton_ctor( )
    textArg = wxString_ctorUTF8("&Exit")
    wxButton_Create( diag_exitbutton, dialog, ID_DIAG_EXITBUTTON, textArg, _
        405, ypos, 85, -1, 0, 0, 0 )
    wxString_dtor( textArg )

    wxEvtHandler_proxy( diag_exitbutton, @diag_exitbutton_cb )
    wxEvtHandler_Connect( diag_exitbutton, wxEvent_EVT_COMMAND_BUTTON_CLICKED( ), ID_DIAG_EXITBUTTON, -1, 0 )

    Dim As wxListBox Ptr selector = wxListBox_ctor( )
    Dim As wxSize Ptr listSize = wxSize_ctor(400, 350)
    If selector = 0 OrElse listSize = 0 Then Return WX_FALSE
    created = wxListBox_Create( selector, dialog, -1, _
        0, listSize, 0, _
        wxLB_SINGLE Or wxLB_NEED_SB Or wxLB_HSCROLL, 0, 0 )
    '' A null point selects the default position without allocating a wrapper.
    wxSize_dtor( listSize )
    If created = 0 Then Return WX_FALSE

    Dim As wxFont Ptr font = wxFont_ctor( 10, wxMODERN, wxNORMAL, wxNORMAL, 0, 0, 0 )
    If font = 0 Then Return WX_FALSE
    wxWindow_SetFont( selector, font )

    Dim As Integer i
    Dim As wxArrayString Ptr tempString = wxArrayString_ctor( )
    If tempString = 0 Then Return WX_FALSE
    For i = 0 To 15
        textArg = wxString_ctorUTF8("list item " + Str( i+1 ))
        If textArg = 0 Then Return WX_FALSE
        wxArrayString_Add( tempString, textArg )
        wxString_dtor( textArg )
        wxListBox_InsertItems( selector, tempString, i )
        wxArrayString_Clear( tempString )
    Next i
    wxArrayString_dtor( tempString )

    '' create the main panel
    Dim As wxPanel Ptr panel = wxPanel_ctor2( frame, ID_PANEL, -1, -1, -1, -1, 0, 0 )
    If panel = 0 Then Return WX_FALSE

    '' create a text widget
    '' we're not using this thing for any events so it gets an id of -1
    Dim As wxTextCtrl Ptr text = wxTextCtrl_ctor( )
    textArg = wxString_ctorUTF8("Hello from wx-c in fb!")
    If text = 0 OrElse textArg = 0 Then Return WX_FALSE
    created = wxTextCtrl_Create( text, panel, -1, textArg, _
        8, 8, FRAME_W - 24, FRAME_H - 200, _
        wxVSCROLL, 0, 0 )
    wxString_dtor( textArg )
    If created = 0 Then Return WX_FALSE

    wxTextCtrl_SetFont( text, font )
    '' Both controls copied the font. The application releases its wrapper.
    wxFont_dtor( font )

    '' make a button (sending a wxsize(-1,-1) means take the default size -
    '' you can specify just x or y as -1 if you want the other one to be a fixed size
    Dim As wxButton Ptr panel_clickmebutton = wxButton_ctor( )
    textArg = wxString_ctorUTF8("Click me!")
    wxButton_Create( panel_clickmebutton, panel, ID_CLICKMEBUTTON, textArg, _
        FRAME_W\2 - 100, FRAME_H - 60, -1, -1, 0, 0, 0 )
    wxString_dtor( textArg )

    '' connect the button to the button event handler sub
    wxEvtHandler_proxy( panel_clickmebutton, @panel_clickmebutton_cb )
    wxEvtHandler_Connect( panel_clickmebutton, wxEvent_EVT_COMMAND_BUTTON_CLICKED( ), ID_CLICKMEBUTTON, -1, 0 )

    ''
    Dim As wxButton Ptr panel_exitbutton = wxButton_ctor( )
    textArg = wxString_ctorUTF8("Exit")
    wxButton_Create( panel_exitbutton, panel, ID_EXITBUTTON, textArg, _
        FRAME_W\2 + 40, FRAME_H - 60, -1, -1, 0, 0, 0 )
    wxString_dtor( textArg )

    '' connect the button to the button event handler sub
    wxEvtHandler_proxy( panel_exitbutton, @panel_exitbutton_cb )
    wxEvtHandler_Connect( panel_exitbutton, wxEvent_EVT_COMMAND_BUTTON_CLICKED( ), ID_EXITBUTTON, -1, 0 )

    '' center it
    wxWindow_CenterOnScreen( frame, wxBOTH )

    '' show window
    wxWindow_Show( frame, 1 )

    Function = wxApp_OnInit( app )

End Function

'':::::
''
'' on exit callback
''
Function app_onexit_cb WXCALL () As wxInt

    Dim As wxString Ptr messageArg = wxString_ctorUTF8("Bye Bye...")
    Dim As wxString Ptr captionArg = wxString_ctorUTF8("Window Closed!")
    wxMsgBox( 0, messageArg, captionArg, wxOK, -1, -1 )
    wxString_dtor( captionArg )
    wxString_dtor( messageArg )

    Function = wxApp_OnExit( app )

End Function

'' end of wx-c_demo.bas

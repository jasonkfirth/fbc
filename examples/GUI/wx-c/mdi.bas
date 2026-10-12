'' FreeBASIC wx-c examples, by dumbledore
'' File: mdi.bas
'' Purpose: Create an MDI parent, child frames and a menu bar.
'' Responsibilities: Native callbacks and temporary label ownership.
'' This file contains no window implementation or custom message loop.

#Include "wx-c/wx.bi"

Const CHILDREN = 4

'' The application owns this parent and its children until normal shutdown.
	Dim Shared As wxApp Ptr app
	Dim Shared As wxMDIParentFrame Ptr mdiparent


'' obligatory callback when creating parent MDI frames
Function mdiparent_OnCreateClient WXCALL (  ) As wxMDIClientWindow Ptr

    Function = wxMDIParentFrame_OnCreateClient( mdiparent )

End Function

''
Function init_frames( ) As wxBool

    Dim As wxString Ptr textArg = wxString_ctorUTF8("parent")
    '' create the parent MDI frame
    mdiparent = wxMDIParentFrame_ctor( )
    If mdiparent = 0 OrElse textArg = 0 Then Return WX_FALSE
    wxMDIParentFrame_RegisterVirtual( mdiparent, @mdiparent_OnCreateClient )
    Dim As wxChar created = wxMDIParentFrame_Create( mdiparent, 0, -1, textArg, -1, -1, 500, 500, _
                             wxFRAME_DEFAULT_STYLE Or wxCLOSE_BOX Or wxVSCROLL Or wxHSCROLL, 0 )
    '' Frame creation copies the title, rather than adopting the wrapper.
    wxString_dtor( textArg )
    If created = 0 Then Return WX_FALSE

    '' now the child ones
    Dim As wxMDIChildFrame Ptr childframe
    Dim As Integer i
    For i = 1 To CHILDREN
        childframe = wxMDIChildFrame_ctor( )
        textArg = wxString_ctorUTF8("child_" + Str(i))
        If childframe = 0 OrElse textArg = 0 Then Return WX_FALSE
        created = wxMDIChildFrame_Create( childframe, mdiparent, -1, textArg, _
                                10*i, 10*i, 120, 100, _
                                wxFRAME_DEFAULT_STYLE Or wxCLOSE_BOX, 0 )
        wxString_dtor( textArg )
        If created = 0 Then Return WX_FALSE
    Next

    Return WX_TRUE
End Function

''
Function init_menus( ) As wxBool

    '' add a menu bar
    Dim As wxMenuBar Ptr mbar = wxMenuBar_ctor( )
    If mbar = 0 Then Return WX_FALSE

    '' and the menus
    Dim As wxString Ptr textArg = wxString_ctorUTF8("")
    If textArg = 0 Then Return WX_FALSE
    Dim As wxMenu Ptr menu = wxMenu_ctor( textArg, 0 )
    wxString_dtor( textArg )
    If menu = 0 Then Return WX_FALSE
    textArg = wxString_ctorUTF8("&Nothing")
    Dim As wxString Ptr helpArg = wxString_ctorUTF8("This does squat.")
    If textArg = 0 OrElse helpArg = 0 Then Return WX_FALSE
    wxMenuBase_Append( menu, -1, textArg, helpArg, 0 )
    wxString_dtor( helpArg )
    wxString_dtor( textArg )
    textArg = wxString_ctorUTF8("&File")
    If textArg = 0 Then Return WX_FALSE
    wxMenuBar_Append( mbar, menu, textArg )
    wxString_dtor( textArg )
    wxFrame_SetMenuBar( mdiparent, mbar )
    Return WX_TRUE
End Function

''
Function App_OnInit WXCALL ( ) As wxBool

    ''
    If init_frames( ) = WX_FALSE Then Return WX_FALSE

    ''
    If init_menus( ) = WX_FALSE Then Return WX_FALSE

    ''
    wxWindow_Show( mdiparent, WX_TRUE )

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

'' end of mdi.bas

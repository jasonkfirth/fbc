'' FreeBASIC native CGUI qualification
'' File: cgui-abi.bas
'' Purpose: Compare native ABI facts and exercise real event callbacks.
'' Responsibilities: Guarded LONG outputs, data identity and orderly cleanup.
'' This file contains no substitute CGUI implementation or desktop input.
#include once "cgui.bi"

Type GuardedInt
    before_value As ULong
    value As Long
    after_value As ULong
End Type

'' CGUI invokes this callback from its event loop. Keep the payload alive
'' until that loop returns, and check that the native API preserves its address.
Dim Shared As ULong cookie = &h71234567
Dim Shared As Long callbacks

Sub finish Cdecl(ByVal callback_data As Any Ptr)
    If callback_data <> @cookie OrElse cookie <> &h71234567 Then End 10
    callbacks += 1
    cookie = &h12345678
    StopProcessEvents()
    CloseWin(NULL)
End Sub

Print "ABI "; SizeOf(Long); " "; SizeOf(ULong); " "; SizeOf(Any Ptr); " "; _
      SizeOf(t_cgui_colors); " "; SizeOf(cgui_ver); " "; _
      SizeOf(event_message_buffer_size); " "; SizeOf(CLong); " "; SizeOf(GuardedInt)
Print "CONSTANTS "; CGUI_AUTOINDICATOR; " "; CGUI_ALIGNCENTRE; " "; CGUI_INIT_WINDOWED; " "; CGUI_W_FLOATING
Print "VERSION "; cgui_ver; " "; cgui_rev; " "; cgui_minor_rev; " "; cgui_release_date
If Command(1) = "--abi" Then End 0

If InitCguiWindowedMode() = 0 Then End 1
Dim As Long dialogue = MkDialogue(CGUI_ADAPTIVE, "FreeBASIC CGUI callback", 0)
AddTextBox(CGUI_TOPLEFT, "Native callback and integer pointer checks", 220, 0, 0)
DisplayWin()
Dim As GuardedInt x = (&hA1B2C3D4, -1, &hE5F60718)
Dim As GuardedInt y = x, w = x, h = x
GetWinInfo(dialogue, @x.value, @y.value, @w.value, @h.value)
If x.before_value <> &hA1B2C3D4 OrElse x.after_value <> &hE5F60718 Then End 2
If y.before_value <> x.before_value OrElse y.after_value <> x.after_value Then End 2
If w.before_value <> x.before_value OrElse w.after_value <> x.after_value Then End 2
If h.before_value <> x.before_value OrElse h.after_value <> x.after_value Then End 2
If w.value <= 0 OrElse h.value <= 0 Then End 2
GenEvent(@finish, @cookie, 100, dialogue)
ProcessEvents()
DeInitCgui()
If callbacks <> 1 OrElse cookie <> &h12345678 Then End 3
Print "CGUI_FB_RUNTIME=PASS"
'' end of cgui-abi.bas

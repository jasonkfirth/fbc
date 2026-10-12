'' CGUI Example: hello.bas
'' Purpose: Open a dialogue and create further dialogues from its buttons.
'' Responsibilities: Native CGUI event processing and close callbacks.
'' This example contains no custom renderer or application data persistence.

#include once "allegro.bi"
#include once "cgui.bi"

declare sub make_a_window ()

sub new_window cdecl (byval userdata as any ptr)
   make_a_window()
end sub

sub stop_program cdecl (byval userdata as any ptr)
   end 0
end sub

sub close_window cdecl (byval userdata as any ptr)
   '' This terminates the top level of event processing.
   StopProcessEvents()
   CloseWin(NULL)
end sub

sub make_a_window( )

   MkDialogue(CGUI_ADAPTIVE, "Hello world", 0)

   AddButton(CGUI_TOPLEFT, "A new_window", @new_window, NULL)

   AddTextBox(CGUI_RIGHT, "A simple window showing some buttons. Press the button to the left and you will get a new identical window.", 200, 0, 0)

   AddButton(CGUI_DOWNLEFT, "E~xit", @stop_program, NULL)

   AddButton(CGUI_RIGHT, "Close", @close_window, NULL)
   DisplayWin()

   '' Check for events - like mouse clicks and key-presses
   ProcessEvents()
end sub

   '' CGUI defaults to fullscreen on Windows. Request a window explicitly
   '' so this introductory example can run beside other desktop programs.
   If InitCguiWindowedMode() = 0 Then End 1
   make_a_window()
   DeInitCgui()
   END_OF_MAIN()
'' end of hello.bas

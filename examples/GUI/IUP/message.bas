'' FreeBASIC IUP examples
'' File: message.bas
'' Purpose: Show a native IUP message dialog.
'' Responsibilities: Native dialog behavior, callbacks and checked toolkit lifetime.
'' This file contains no widget implementation or secondary event dispatcher.
#include once "IUP/iup.bi"
#define NULL 0

  '' Initializes IUP
  if IupOpen( NULL, NULL ) = IUP_ERROR then
    print "Error: Could not initialize IUP."
    end 1
  end if

  '' Shows a Message Box
  IupMessage("IupMessage Example", "Hi! Press the button:")

  '' Finishes IUP
  IupClose ()

'' end of message.bas

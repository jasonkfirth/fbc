/'
    Project: FreeBASIC console qualification
    File: emulator-arguments.bas
    Purpose: Check the guest argument adapter before running example programs.
    Responsibilities: Verify a real argv[0], whitespace and empty arguments.
    This file does not supply its own arguments or generate an exit receipt.
'/
#lang "fb"
#include once "file.bi"

if not fileexists(command(0)) then end 21
if command(1) <> "two words" then end 22
if command(2) <> "" then end 23
if command(3) <> "saved-state.bin" then end 24
if command(4) <> "" then end 25
end 0

'' end of emulator-arguments.bas

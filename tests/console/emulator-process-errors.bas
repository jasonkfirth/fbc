/'
    Project: FreeBASIC console qualification
    File: emulator-process-errors.bas
    Purpose: Check that failed process launches return to the current title.
    Responsibilities: Exercise missing, empty and oversized executable paths.
    This file does not launch another executable or replace the current title.
'/
#lang "fb"
#include once "file.bi"

const missing_program = "E:/fbq-missing-executable.xbe"
if fileexists(missing_program) then end 81
for index as long = 1 to 16
    if exec(missing_program, "unused argument") <> -1 then end 82
    if chain(missing_program) <> -1 then end 83
    if shell(missing_program) <> -1 then end 84
next
if exec("", "") <> -1 then end 85
if chain(string(300, "x")) <> -1 then end 86
if shell(string(300, "x")) <> -1 then end 87
'' The SDK only supports its fixed DVD root as a working directory.
if chdir("D:/") <> 0 then end 88
if chdir("X:/fbq-missing-directory") = 0 then end 89
end 0

'' end of emulator-process-errors.bas

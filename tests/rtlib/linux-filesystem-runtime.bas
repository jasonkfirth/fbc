'' Project: FreeBASIC runtime tests
'' File: linux-filesystem-runtime.bas
'' Purpose: Check ordinary filesystem and memory-query entry points.
'' Responsibilities: Verify complete copied bytes, directory names and queries.
'' Synthetic count boundaries belong to linux-filesystem-sizes.c.
#lang "fb"
#include once "vbcompat.bi"
#include once "file.bi"

const rootPath = "filesystem-basic-values"
dim contents as string = "header" & chr(0) & string(33000, "x")
dim fileNumber as integer = freefile
dim firstName as string, secondName as string

if len(curdir()) = 0 or len(exepath()) = 0 then end 10
if len(monthname(1)) = 0 or len(weekdayname(1)) = 0 then end 11
if fre() = 0 or fre(-1) = 0 then end 12
if mkdir(rootPath) <> 0 then end 13
if open(rootPath & "/a.dat" for binary as #fileNumber) <> 0 then end 14
if put(#fileNumber, , contents) <> 0 then end 15
if close(#fileNumber) <> 0 then end 16
if filecopy(rootPath & "/a.dat", rootPath & "/b.dat") <> 0 then end 17
if open(rootPath & "/b.dat" for binary as #fileNumber) <> 0 then end 18
if input(lof(fileNumber), fileNumber) <> contents then end 19
if close(#fileNumber) <> 0 then end 20
firstName = dir(rootPath & "/*.dat")
secondName = dir()
if firstName = secondName then end 21
if firstName <> "a.dat" and firstName <> "b.dat" then end 22
if secondName <> "a.dat" and secondName <> "b.dat" then end 23
if dir() <> "" then end 24
if kill(rootPath & "/a.dat") <> 0 then end 25
if kill(rootPath & "/b.dat") <> 0 then end 26
if rmdir(rootPath) <> 0 then end 27
print "LINUX_FILESYSTEM_RUNTIME_PASS"

'' end of linux-filesystem-runtime.bas

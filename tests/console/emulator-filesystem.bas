/'
    Project: FreeBASIC console qualification
    File: emulator-filesystem.bas
    Purpose: Exercise native directory iteration and relative file renaming.
    Responsibilities: Own fixture files and verify per-thread enumeration state.
    This file does not enumerate user directories or relax runtime assertions.
'/
#lang "fb"
#include once "file.bi"

const fixture_root = "fbq-filesystem"
dim shared worker_result as long

sub enumerate_worker(byval unused as any ptr)
    if dir(fixture_root & "/*.bin") = "two.bin" then worker_result = 1
    '' Leave this iterator open so thread cleanup also exercises its destructor.
end sub

if mkdir(fixture_root) <> 0 then end 31
dim handle as integer = freefile
if open(fixture_root & "/one.txt" for output as #handle) <> 0 then end 32
print #handle, "one"
close #handle
if open(fixture_root & "/two.bin" for output as #handle) <> 0 then end 33
print #handle, "two"
close #handle
if open(fixture_root & "/abc" for output as #handle) <> 0 then end 51
print #handle, "plain"
close #handle
if dir(fixture_root & "/abc") <> "abc" then end 52
if dir(fixture_root & "/ONE.TXT") <> "one.txt" then end 53
if dir(fixture_root & "/o?e.txt") <> "one.txt" then end 54
if dir(fixture_root & "/*.txt") <> "one.txt" then end 34
if dir() <> "" then end 35
name fixture_root & "/one.txt" as fixture_root & "/renamed.txt"
if not fileexists(fixture_root & "/renamed.txt") then end 36
if fileexists(fixture_root & "/one.txt") then end 37

dim entry_name as string = dir(fixture_root & "/*.*")
if entry_name = "" then end 38
dim worker as any ptr = threadcreate(@enumerate_worker)
if worker = 0 then end 39
threadwait(worker)
if worker_result <> 1 then end 40
dim entries as long = 1
do
    entry_name = dir()
    if entry_name = "" then exit do
    entries += 1
    if entries > 3 then end 41
loop
if entries <> 3 then end 42
if dir(fixture_root & "/absent*") <> "" then end 43
if kill(fixture_root & "/renamed.txt") <> 0 then end 44
if kill(fixture_root & "/two.bin") <> 0 then end 45
if kill(fixture_root & "/abc") <> 0 then end 55
if rmdir(fixture_root) <> 0 then end 46
'' A root filename has no directory separator until the Xbox drive adapter
'' qualifies it. Keep its original UTF-8 bytes through native enumeration.
dim root_file as string = "fbq-root-" & chr(&hC3, &hA9, &hE4, &hB8, &hAD) & ".txt"
if open(root_file for output as #handle) <> 0 then end 47
print #handle, "root"
close #handle
entry_name = dir(root_file)
if entry_name <> root_file then
    handle = freefile
    if open("E:\FBQFSERR.TXT" for output as #handle) = 0 then
        print #handle, "expected: "; root_file
        print #handle, "actual: "; entry_name
        entry_name = dir("E:/*", &hff)
        while entry_name <> ""
            print #handle, "entry: "; entry_name
            entry_name = dir()
        wend
        close #handle
    end if
    end 48
end if
if dir() <> "" then end 49
if kill(root_file) <> 0 then end 50
end 0

'' end of emulator-filesystem.bas

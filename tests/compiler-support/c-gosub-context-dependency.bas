/'
    Project: FreeBASIC C backend regression reduction
    File: c-gosub-context-dependency.bas
    Purpose: Keep a GOSUB context used only by generated runtime calls.
    Responsibilities: Execute a GOSUB whose target ends the program without RETURN.
    This file contains no graphics, file operations or native assembly.
'/
#lang "fblite"
#if (__FB_BACKEND__ = "gcc") or (__FB_BACKEND__ = "clang")
    #cmdline "-Wc -Werror=implicit-function-declaration"
#endif
option gosub

gosub work
end 2

work:
print "GOSUB_STORAGE_DEPENDENCY_PASS"
end 0

' end of c-gosub-context-dependency.bas

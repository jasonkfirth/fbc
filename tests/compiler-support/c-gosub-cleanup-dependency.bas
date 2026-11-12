/'
    Project: FreeBASIC generated cleanup regression reduction
    File: c-gosub-cleanup-dependency.bas
    Purpose: Retain a GOSUB cleanup runtime dependency owned by an error callback.
    Responsibilities: Use a local GOSUB and an installed legacy error handler.
    This file contains no graphics, external assets or failing runtime operations.
'/
#lang "fblite"
#if (__FB_BACKEND__ = "gcc") or (__FB_BACKEND__ = "clang")
    #cmdline "-Wc -Werror=implicit-function-declaration"
#endif
option gosub

dim as integer visits
on error goto failed
gosub work
if visits <> 1 then end 2
print "GOSUB_ERROR_CLEANUP_DEPENDENCY_PASS"
end 0

work:
visits += 1
return

failed:
end 1

' end of c-gosub-cleanup-dependency.bas

/' Project: FreeBASIC C emitter audit
   File: c-procedure-reachability-copyback.bas
   Purpose: Compare fixed-string BYREF copy-back between backends.
   Responsibilities: Check a copied argument with a live index function.
   This fixture intentionally does NOT use destructors. '/
private function string_index() as integer
    return 0
end function
private function unused_index_caller() as integer
    return string_index()
end function
private sub append_marker(byref value as string)
    value = trim(value) + "B"
end sub
dim values(0 to 0) as string * 8
values(0) = "A"
append_marker(values(string_index()))
if trim(values(0)) <> "AB" then end 1
end 0
' end of c-procedure-reachability-copyback.bas

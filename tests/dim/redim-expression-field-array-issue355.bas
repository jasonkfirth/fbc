/'
    Project: FreeBASIC compiler regression tests
    ---------------------------------------------

    File: redim-expression-field-array-issue355.bas

    Purpose:

        Verify that REDIM can identify a dynamic array field after
        indexing the containing dynamic array.

    Responsibilities:

        • exercise the parser ambiguity reported by GitHub issue #355
        • verify that the resulting field can be indexed after REDIM

    This file intentionally does NOT contain:

        • compiler implementation logic
        • platform-specific runtime checks
 '/

type AType
    array(any) as integer
end type

dim test(any, any) as AType
redim test(0, 0)
redim test(0, 0).array(0 to 2)

test(0, 0).array(0) = 11
test(0, 0).array(1) = 22
test(0, 0).array(2) = 33

if test(0, 0).array(0) <> 11 then end 1
if test(0, 0).array(1) <> 22 then end 1
if test(0, 0).array(2) <> 33 then end 1

/' end of redim-expression-field-array-issue355.bas '/

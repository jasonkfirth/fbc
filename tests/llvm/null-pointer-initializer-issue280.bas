/'
    Project: FreeBASIC compiler regression tests
    ---------------------------------------------

    File: null-pointer-initializer-issue280.bas

    Purpose:

        Verify that the LLVM backend emits a typed null pointer when an
        integer pointer is initialized from CPtr(..., 0).

    Responsibilities:

        • cover the invalid LLVM IR reported by GitHub issue #280
        • provide input suitable for both textual IR and llc validation

    This file intentionally does NOT contain:

        • LLVM implementation code
        • platform-specific linker behavior
 '/

function EntryPoint alias "EntryPoint"() as integer
    dim pValue as integer ptr = cptr(integer ptr, 0)
    return 0
end function

/' end of null-pointer-initializer-issue280.bas '/

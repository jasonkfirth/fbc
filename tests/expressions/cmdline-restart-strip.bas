/'
    FreeBASIC compiler regression test
    ----------------------------------

    File: cmdline-restart-strip.bas

    Purpose:

        Verify that a source-level #cmdline which retains intermediate files
        still lets the restarted compiler consume the stage-one output.

    Responsibilities:

        - request the strip and retain-intermediate options
        - keep the source small so the test isolates the restart path

    This file intentionally does NOT contain:

        - linker-specific behavior
        - target-specific runtime calls
'/

' TEST_MODE : COMPILE_ONLY_OK

#cmdline "-strip -R"

dim as integer value = 42
if value <> 42 then end 1

/' end of cmdline-restart-strip.bas '/

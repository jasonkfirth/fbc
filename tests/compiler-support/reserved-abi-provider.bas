/'
    Project: FreeBASIC compiler regression tests
    File: reserved-abi-provider.bas
    Purpose: Supply the identifier fixture from a FreeBASIC module.
    Responsibilities: Define the external function and global variable.
    This fixture does not provide the test driver.
'/
extern "C"
    dim shared external_value alias "__fb_test_external_value" as long = 17

    function _fb_test_external_add cdecl ( byval value as long ) as long
        return value + 5
    end function
end extern

/' end of reserved-abi-provider.bas '/

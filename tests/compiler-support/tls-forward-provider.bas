/'
    Project: FreeBASIC C backend regression tests
    File: tls-forward-provider.bas
    Purpose: Check a forward declaration of keyword-named TLS storage.
    Responsibilities: Define and read the same object after an extern declaration.
    This fixture does not define the consumer module.
'/

extern "C"
    thread local extern exported_value alias "bool" as long
    public thread local dim shared exported_value as long = 17

    function tls_keyword_read cdecl () as long
        return exported_value
    end function
end extern

/' end of tls-forward-provider.bas '/

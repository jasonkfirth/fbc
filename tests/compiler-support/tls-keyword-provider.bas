/'
    Project: FreeBASIC compiler regression tests
    File: tls-keyword-provider.bas
    Purpose: Define a thread-local variable with a fixed external name.
    Responsibilities: Supply storage for a separately compiled consumer.
    This fixture does not create worker threads.
'/

extern "C"
    public thread local dim shared exported_value alias "bool" as long = 17
    function tls_keyword_read cdecl () as long
        return exported_value
    end function
end extern

/' end of tls-keyword-provider.bas '/

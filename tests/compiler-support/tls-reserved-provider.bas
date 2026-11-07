/'
    Project: FreeBASIC compiler regression tests
    File: tls-reserved-provider.bas
    Purpose: Define a thread-local variable with a fixed external name.
    Responsibilities: Supply storage for a separately compiled consumer.
    This fixture does not create worker threads.
'/

extern "C"
    public thread local dim shared exported_value alias "__fb_test_tls_value" as long = 17
end extern

/' end of tls-reserved-provider.bas '/

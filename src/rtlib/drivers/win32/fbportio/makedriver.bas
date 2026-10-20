/'
    Project: FreeBASIC Win32 runtime
    -------------------------------

    File: drivers/win32/fbportio/makedriver.bas

    Purpose:

        Convert the fbportio.sys image into a C byte array for the driver
        resource module.

    Responsibilities:

        - read and size-check the driver image
        - encode its bytes as a C initializer
        - write the generated source module

    This file intentionally does NOT contain:

        - the kernel driver implementation or its device protocol
        - service installation and port-I/O test policy

    Targets: Win32 build-host utility for the matching fbportio driver.
    Module API: standalone build tool; helper procedures are not runtime ABI.

    Ownership: fileLoad transfers its allocated buffer to the caller, which
    releases it after the generated module has been written.
'/

#define NULL 0

sub fatalCantAccessFile(byref filename as string)
    print "Error: Could not access file: '" + filename + "'"
    end 1
end sub

function fileLoad(byref filename as string, byval psize as integer ptr) as ubyte ptr
    dim as integer f = freefile()
    if (open(filename, for binary, access read, as #f)) then
        fatalCantAccessFile(filename)
    end if

    dim as ubyte ptr p = NULL
    '' Keep LOF wide, then reject sizes outside the INTEGER range before conversion.
    '' fblint: disable-next-line FBL423 REASON: LongInt is fixed-width and bounded before CInt.
    dim as longint file_size = lof(f)
    if (file_size < 0) orelse (file_size > clngint(&h7FFFFFFF)) then
        close #f
        fatalCantAccessFile(filename)
    end if
    dim as integer size = cint(file_size)

    if (size > 0) then
        p = allocate(size)
        if( p <> NULL ) then
            dim as integer result = get(#f, , *p, size, size)
            if (result or (size <= 0)) then
                deallocate(p)
                p = NULL
                close #f
                fatalCantAccessFile(filename)
                return NULL
            end if
        else
            close #f
            fatalCantAccessFile(filename)
            return NULL
        end if
    end if

    close #f

    *psize = size
    return p
end function

sub writeOut(byref filename as string, byref text as string)
    dim as integer f = freefile()
    if (open(filename, for output, as #f)) then
        fatalCantAccessFile(filename)
    end if
    print #f, text;
    close #f
end sub

    if (__FB_ARGC__ <> 3) then
        print "Usage: makedriver fbportio.sys fbportio-driver.h"
        end 1
    end if

    dim as string inputfile = *__FB_ARGV__[1]
    dim as string outputfile = *__FB_ARGV__[2]

    dim as integer size = 0
    dim as ubyte ptr p = fileLoad(inputfile, @size)

    dim as string emit
    emit += "#define FBPORTIO_DRIVER_SIZE " + str(size) + !"\n"
    emit += !"const unsigned char fbportio_driver[FBPORTIO_DRIVER_SIZE] = {\n"

    for i as integer = 0 to (size - 1)
        if ((i mod 16) = 0) then
            '' Indent at line begin
            '' The driver is a small build-time input kept in one output buffer.
            emit += "    "
        end if

        emit += "0x" + hex(p[i], 2)

        if (i < (size - 1)) then
            emit += ","
            '' Emit a newline every 16 bytes
            if (((i + 1) mod 16) = 0) then
                emit += !"\n"
            else
                emit += " "
            end if
        else
            emit += !"\n"
        end if
    next

    emit += !"};\n"

    writeOut(outputfile, emit)

    if (p <> NULL) then
        deallocate(p)
        p = NULL
    end if
'' end of makedriver.bas

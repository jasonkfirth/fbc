/'
    Small tool to test the fbportio driver. You can
'/

#include "windows.bi"
#include "win/winioctl.bi"
#include "../fbportio.h"

#define INFO(message) print __FUNCTION__ & "(): " & message
#define APIFAILED(func) print_winapi_error(__FUNCTION__, func)

private function command_line_argument _
    ( _
        byval index as integer, _
        byval argument_count as integer, _
        byval arguments as zstring ptr ptr _
    ) as zstring ptr

    if (arguments = NULL) or (index < 0) or (index >= argument_count) then
        return NULL
    end if

    '' The runtime guarantees one non-null argument pointer for each index below argc.
    '' fblint: disable-next-line FBL525
    return arguments[index]
end function

private sub print_winapi_error(byval parent as zstring ptr, byval func as zstring ptr)
    dim as DWORD e = GetLastError()
    print *parent & "(): " & *func & " failed, GetLastError(): &h" & hex(e) & " / " & e

    dim as zstring ptr p = NULL
    if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER or FORMAT_MESSAGE_FROM_SYSTEM, _
                      NULL, _
                      e, _
                      0, _
                      cptr(LPTSTR, @p), _
                      0, _
                      NULL)) then
        if (p <> NULL) then
            '' FORMAT_MESSAGE_ALLOCATE_BUFFER transfers this buffer to the caller.
            '' fblint: disable-next-line FBL-PTR-001
            print *p
            LocalFree(p)
        end if
    end if

    end(1)
end sub

private sub delete_service(byval manager as SC_HANDLE)
    dim as SC_HANDLE service = OpenService(manager, "fbportio", SERVICE_ALL_ACCESS)
    if (service = NULL) then
        APIFAILED("OpenService()")
    end if

    INFO("Marking the fbportio service for deletion...")
    INFO("(This will fail if the service is already marked for deletion)")
    INFO("(If the service is running, it will be deleted after it stops)")

    if (DeleteService(service) = 0) then
        APIFAILED("DeleteService()")
    end if

    CloseServiceHandle(service)
end sub

private sub stop_service(byval manager as SC_HANDLE)
    dim as SC_HANDLE service = OpenService(manager, "fbportio", SERVICE_ALL_ACCESS)
    if (service = NULL) then
        APIFAILED("OpenService()")
    end if

    INFO("Stopping the fbportio service...")

    dim as SERVICE_STATUS status
    if (ControlService(service, SERVICE_CONTROL_STOP, @status) = 0) then
        APIFAILED("ControlService(SERVICE_CONTROL_STOP)")
    end if

    CloseServiceHandle(service)
end sub

private sub create_fbportio_sys()
    INFO("Copying fbportio.sys into the system's driver directory...")

    '' A fixed-size ZSTRING has inline storage and no configurable array lower bound.
    '' fblint: disable-next-line FBL-OPT-003
    dim as zstring * (MAX_PATH+1) filename
    dim as integer needed = GetSystemDirectory(@filename, MAX_PATH)
    if ((needed = 0) or (needed > MAX_PATH)) then
        APIFAILED("GetSystemDirectory()")
    end if

    filename += "\Drivers\fbportio.sys"

    if (CopyFile("fbportio.sys", filename, FALSE) = 0) then
        APIFAILED("CopyFile()")
    end if
end sub

private sub create_service(byval manager as SC_HANDLE)
    INFO("Creating fbportio service...")

    dim as SC_HANDLE service = _
            CreateService(manager, _
                          "fbportio", _
                          "fbportio", _
                          SERVICE_ALL_ACCESS, _
                          SERVICE_KERNEL_DRIVER, _
                          SERVICE_DEMAND_START, _
                          SERVICE_ERROR_NORMAL, _
                          "system32\drivers\fbportio.sys", _
                          NULL, _
                          NULL, _
                          NULL, _
                          NULL, _
                          NULL)

    if (service = NULL) then
        APIFAILED("CreateService()")
    end if

    CloseServiceHandle(service)
end sub

private sub start_service(byval manager as SC_HANDLE)
    dim as SC_HANDLE service = OpenService(manager, "fbportio", SERVICE_ALL_ACCESS)
    if (service = NULL) then
        APIFAILED("OpenService()")
    end if

    INFO("Starting fbportio service...")

    if (StartService(service, 0, NULL) = 0) then
        APIFAILED("StartService()")
    end if

    CloseServiceHandle(service)
end sub

private sub access_driver _
    ( _
        byval h as HANDLE, _
        byval code as DWORD, _
        byval pin as any ptr, _
        byval insize as uinteger, _
        byval pout as any ptr, _
        byval outsize as uinteger _
    )

    INFO("Accessing driver...")

    dim as DWORD outwritten = 0
    if (DeviceIoControl(h, code, pin, insize, pout, outsize, @outwritten, NULL) = 0) then
        APIFAILED("DeviceIoControl()")
    end if
end sub

private sub init_driver()
    INFO("Initializing driver...")

    static as integer initialized = FALSE
    if (initialized) then
        return
    end if

    INFO("Opening '\\.\fbportio'...")

    dim as HANDLE driver = _
            CreateFile("\\.\fbportio", _
                       GENERIC_READ or GENERIC_WRITE, _
                       0, _
                       NULL, _
                       OPEN_EXISTING, _
                       FILE_ATTRIBUTE_NORMAL, _
                       NULL)
    if (driver = INVALID_HANDLE_VALUE) then
        APIFAILED("CreateFile()")
    end if

    INFO("Asking driver for version number...")
    dim as WORD version = 0
    access_driver(driver, IOCTL_GET_VERSION, NULL, 0, @version, 2)
    if (version <> FBPORTIO_VERSION) then
        INFO("Driver version mismatch, aborting...")
        CloseHandle(driver)
        end(1)
    end if

    INFO("Asking driver to grant our pid i/o ports access...")
    dim as DWORD pid = GetCurrentProcessId()
    access_driver(driver, IOCTL_GRANT_IOPM, @pid, 4, NULL, 0)

    CloseHandle(driver)

    '' Give up our timeslice to ensure process kernel state is updated
    Sleep(1)
    initialized = TRUE
end sub

private sub write_driver(byval port as short, byval dat as byte)
    INFO("Writing...")
    '' __asm__ volatile("outb %0, %1" : : "a"(dat), "d"(port));
    '' This test tool is built only for the 32-bit x86 fbportio backend.
    '' fblint: disable-next-line FBL972
    asm
        mov al, [dat]
        mov dx, [port]
        out dx, al
    end asm
end sub

private sub read_driver(byval port as short, byval pdat as byte ptr)
    INFO("Reading...")
    dim as byte dat = 0

    '' __asm__ volatile("inb %1, %0" : "=a"(value) : "d"(port));
    '' This test tool is built only for the 32-bit x86 fbportio backend.
    '' fblint: disable-next-line FBL972
    asm
        mov dx, [port]
        in al, dx
        mov [dat], al
    end asm

    if (pdat) then
        *pdat = dat
    end if
end sub


    INFO("Opening service control manager...")
    dim as SC_HANDLE manager = OpenSCManager(NULL, NULL, GENERIC_ALL)
    if (manager = NULL) then
        APIFAILED("OpenSCManager()")
        INFO("Opening service control manager in read only mode...")
        manager = OpenSCManager(NULL, NULL, GENERIC_READ)
        if (manager = NULL) then
            APIFAILED("OpenSCManager()")
            end(1)
        end if
    end if

    dim as integer show_help = (__FB_ARGC__ < 2)

    for i as integer = 1 to (__FB_ARGC__ - 1)
        dim as zstring ptr arg = command_line_argument(i, __FB_ARGC__, __FB_ARGV__)
        if (arg = NULL) then
            show_help = TRUE
            exit for
        end if

        select case (*arg)
        case "-h", "--help"
            show_help = TRUE
            exit for

        case "-i", "--install"
            create_fbportio_sys()
            create_service(manager)

        case "-u", "--uninstall"
            delete_service(manager)

        case "-s", "--start"
            start_service(manager)

        case "-e", "--stop"
            stop_service(manager)

        case "-w", "--write"
            dim as zstring ptr port = command_line_argument(i + 1, __FB_ARGC__, __FB_ARGV__)
            dim as zstring ptr dat = command_line_argument(i + 2, __FB_ARGC__, __FB_ARGV__)
            if (port = NULL) or (dat = NULL) then
                show_help = TRUE
                exit for
            end if
            i += 2
            init_driver()
            write_driver(valuint(*port), valuint(*dat))

        case "-r", "--read"
            dim as zstring ptr port = command_line_argument(i + 1, __FB_ARGC__, __FB_ARGV__)
            if (port = NULL) then
                show_help = TRUE
                exit for
            end if
            i += 1
            dim as byte dat = 0
            init_driver()
            read_driver(valuint(*port), @dat)
            print "Data received: " + str(dat)

        case else
            print "Unexpected command line argument: '" + *arg + "'"
            end(1)
        end select
    next

    if (manager) then
        CloseServiceHandle(manager)
    end if

    if (show_help) then
        print "Options:"
        print "-h, --help"
        print "-i, --install"
        print "-u, --uninstall"
        print "-s, --start"
        print "-e, --stop"
        print "{-w|--write} <port> <data>"
        print "{-r|--read} <port>"
        end(1)
    end if

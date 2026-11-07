/'
    Project: FreeBASIC C backend regression tests
    File: c-long-literals.bas
    Purpose: Preserve long literal bytes and their persistent addresses.
    Responsibilities: Check boundary lengths, embedded zeros, signed char views,
    static aggregate initialization, UTF-16 pairs and writes to pooled storage.
    This file intentionally does NOT use a native diagnostic suppression.
'/
#lang "fb"

#define TEXT1 "A"
#define TEXT2 (TEXT1 + TEXT1)
#define TEXT4 (TEXT2 + TEXT2)
#define TEXT8 (TEXT4 + TEXT4)
#define TEXT16 (TEXT8 + TEXT8)
#define TEXT32 (TEXT16 + TEXT16)
#define TEXT64 (TEXT32 + TEXT32)
#define TEXT128 (TEXT64 + TEXT64)
#define TEXT256 (TEXT128 + TEXT128)
#define TEXT512 (TEXT256 + TEXT256)
#define TEXT1024 (TEXT512 + TEXT512)
#define TEXT2048 (TEXT1024 + TEXT1024)
#define TEXT4096 (TEXT2048 + TEXT2048)
#define TEXT4095 (TEXT2048 + TEXT1024 + TEXT512 + TEXT256 + TEXT128 + TEXT64 + TEXT32 + TEXT16 + TEXT8 + TEXT4 + TEXT2 + TEXT1)

#define BYTES16 (!"A\0B\x80\xFF\x1B\"\\?09az~\nZ")
#define BYTES32 (BYTES16 + BYTES16)
#define BYTES64 (BYTES32 + BYTES32)
#define BYTES128 (BYTES64 + BYTES64)
#define BYTES256 (BYTES128 + BYTES128)
#define BYTES512 (BYTES256 + BYTES256)
#define BYTES1024 (BYTES512 + BYTES512)
#define BYTES2048 (BYTES1024 + BYTES1024)
#define BYTES4096 (BYTES2048 + BYTES2048)

#define WIDE1 wstr(!"\u20AC\U0001F600X")
#define WIDE2 (WIDE1 + WIDE1)
#define WIDE4 (WIDE2 + WIDE2)
#define WIDE8 (WIDE4 + WIDE4)
#define WIDE16 (WIDE8 + WIDE8)
#define WIDE32 (WIDE16 + WIDE16)
#define WIDE64 (WIDE32 + WIDE32)
#define WIDE128 (WIDE64 + WIDE64)
#define WIDE256 (WIDE128 + WIDE128)
#define WIDE512 (WIDE256 + WIDE256)
#define WIDE1024 (WIDE512 + WIDE512)
#define WIDE2048 (WIDE1024 + WIDE1024)

type LiteralRecord
    narrow_value as zstring * 4098
    wide_value as wstring * 8194
end type

dim shared as LiteralRecord saved = (BYTES4096, WIDE2048)
dim shared as const zstring ptr global_address = @BYTES4096
dim shared as const wstring ptr global_wide = @WIDE2048

private function NarrowAddress() as zstring ptr
    return @BYTES4096
end function

private function WideAddress() as wstring ptr
    return @WIDE2048
end function

dim as const ubyte expected(0 to 15) = {65, 0, 66, 128, 255, 27, 34, 92, 63, 48, 57, 97, 122, 126, 10, 90}
dim as ubyte ptr value = cast(ubyte ptr, NarrowAddress())
if( NarrowAddress() <> cast(zstring ptr, value) ) then end __LINE__
for index as integer = 0 to 4095
    if( value[index] <> expected(index mod 16) ) then end __LINE__
    if( saved.narrow_value[index] <> expected(index mod 16) ) then end __LINE__
    if( (*global_address)[index] <> expected(index mod 16) ) then end __LINE__
next
if( value[4096] <> 0 or saved.narrow_value[4096] <> 0 or saved.narrow_value[4097] <> 0 ) then end __LINE__

dim as string dynamic_value = TEXT4096
if( len(dynamic_value) <> 4096 ) then end __LINE__
for index as integer = 0 to 4095
    if( dynamic_value[index] <> 65 ) then end __LINE__
next
if( len(TEXT4095) <> 4095 or len(TEXT4096) <> 4096 ) then end __LINE__
dim as zstring ptr boundary = @TEXT4095
if( boundary[4094] <> 65 or boundary[4095] <> 0 ) then end __LINE__

dim as wstring ptr wide_value = WideAddress()
if( WideAddress() <> wide_value ) then end __LINE__
#if sizeof(wstring) = 2
    const WIDE_UNITS = 4
    dim as const uinteger expected_wide(0 to 3) = {&h20AC, &hD83D, &hDE00, 88}
#else
    const WIDE_UNITS = 3
    dim as const uinteger expected_wide(0 to 2) = {&h20AC, &h1F600, 88}
#endif
for index as integer = 0 to WIDE_UNITS * 2048 - 1
    if( (*wide_value)[index] <> expected_wide(index mod WIDE_UNITS) ) then end __LINE__
    if( saved.wide_value[index] <> expected_wide(index mod WIDE_UNITS) ) then end __LINE__
    if( (*global_wide)[index] <> expected_wide(index mod WIDE_UNITS) ) then end __LINE__
next
if( (*wide_value)[WIDE_UNITS * 2048] <> 0 or saved.wide_value[WIDE_UNITS * 2048] <> 0 ) then end __LINE__

'' Both calls return the same module-owned writable storage. A returned
'' address must remain usable after its producing procedure has returned.
value[4095] = 67
if( NarrowAddress()[4095] <> 67 ) then end __LINE__
(*wide_value)[WIDE_UNITS * 2048 - 1] = 89
if( (*WideAddress())[WIDE_UNITS * 2048 - 1] <> 89 ) then end __LINE__

/' end of c-long-literals.bas '/

/'
    Project: FreeBASIC console qualification
    File: emulator-native-filesystem.bas
    Purpose: Report the independent native C filesystem probe's result.
    Responsibilities: Call the C ABI and retain its unchanged return code.
    This file does not create filenames or implement directory matching.
'/
#lang "fb"

declare function native_filesystem cdecl alias "fbqual_native_filesystem" () as long
end native_filesystem()

'' end of emulator-native-filesystem.bas

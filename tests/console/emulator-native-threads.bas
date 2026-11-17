/'
    Project: FreeBASIC console qualification
    File: emulator-native-threads.bas
    Purpose: Report the independent native C thread probe's result.
    Responsibilities: Call the native ABI and retain its result unchanged.
    This file does not create workers or implement their synchronization.
'/
#lang "fb"

declare function native_threads cdecl alias "fbqual_native_threads" () as long
end native_threads()

'' end of emulator-native-threads.bas

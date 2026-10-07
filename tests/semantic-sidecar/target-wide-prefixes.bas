'' Project: FreeBASIC compiler semantic-model fixtures
'' File: target-wide-prefixes.bas
'' Purpose: Retain emitted literal WCHAR units across C and assembly backends.
'' Responsibilities: ASCII, supplementary scalars, explicit surrogates and nulls.
'' This file intentionally does NOT assume that producer and target units match.

#lang "fb"

print wstr("A"), wstr(!"\U0001F600"), wstr(!"\uD83D\uDE00")
print wstr(!"A\x00B"), wstr(""), wstr(!"\u20AC"), wstr(!"\UFFFFFFFF")

'' end of target-wide-prefixes.bas

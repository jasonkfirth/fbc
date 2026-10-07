'' Project: FreeBASIC semantic sidecar tests
'' File: initializer-copy-contracts.bas
'' Purpose: Preserve selected fixed-character initializer copy contracts.
'' Responsibilities: Static, runtime, counted, wide and aggregate inputs.
'' This file intentionally does NOT execute the truncated initializer values.
#lang "fb"
dim shared as string * 4 StaticEmbedded = !"A\0BCDEF"
dim shared as string * 4 StaticWide = wstr("ABCDE")
dim shared as string * 4 UnusedStatic = "ABCDE"
type CopyRecord
	text as string * 4
end type
sub ReviewCopyContracts()
	print StaticEmbedded, StaticWide
	dim as string * 4 LocalEmbedded = !"A\0BCDEF"
	dim as string * 4 LocalOverflow = "ABCDE"
	dim as string CountedSource = !"A\0BCDEF"
	dim as string * 4 LocalCounted = CountedSource
	dim as string * 4 LocalWide = wstr("ABCDE")
	dim as CopyRecord LocalRecord = ("ABCDE")
	dim LocalArray(0 to 1) as string * 4 = {"ABCDE", !"A\0BCDEF"}
end sub
#define COPY_LITERAL "ABCDE"
sub ReviewMacroCopy()
	dim as string * 4 MacroTarget = COPY_LITERAL
end sub
'' end of initializer-copy-contracts.bas

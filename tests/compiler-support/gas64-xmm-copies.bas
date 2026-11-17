'' Project: FreeBASIC GAS64 optimizer regression
'' File: gas64-xmm-copies.bas
'' Purpose: Exercise consecutive scalar copies through the production rewrite.
'' Responsibilities: Report register and memory copies for both scalar widths.
'' This file intentionally does NOT duplicate optimizer instruction selection.

#lang "fb"

'' The controller extracts the production helper into this generated include.
'' Its nondebug path only writes the supplied length and position variables;
'' no instruction buffer is needed to observe the replacement instructions.
dim shared as string prevpart1, prevpart2, previnstruc
dim shared as integer prevwpos

#include "gas64-xmm-rewrite.bi"

private sub runCase( byref mnemonic as const string, byval toMemory as boolean )

	dim as integer textLength = 0, writepos = 1
	dim as string part1 = "xmm2", part2 = "-8[rbp]"
	dim as string instruc = "movss"
	if( mnemonic = "movsd" or mnemonic = "movq" ) then
		instruc = "movsd"
	end if
	dim as string code = instruc + " " + part1 + ", " + part2

	prevpart1 = part2
	prevpart2 = "xmm0"
	previnstruc = mnemonic
	prevwpos = writepos
	hOptimRewriteFromXmm( code, @textLength, part1, part2, instruc, writepos )
	dim as string firstCopy = code

	'' check_optim remembers the rewritten instruction before handling the
	'' next one. Preserve that transition so a lost DOUBLE width is visible.
	prevpart1 = part1
	prevpart2 = part2
	previnstruc = instruc
	prevwpos = writepos
	part1 = iif( toMemory, "-16[rbp]", "xmm3" )
	part2 = "xmm2"
	instruc = iif( mnemonic = "movsd" or mnemonic = "movq", "movsd", "movss" )
	code = instruc + " " + part1 + ", " + part2
	hOptimRewriteFromXmm( code, @textLength, part1, part2, instruc, writepos )

	print mnemonic + "|" + iif( toMemory, "memory", "register" ) + _
	      "|" + firstCopy + "|" + code
end sub

runCase( "movss", false )
runCase( "movss", true )
runCase( "movd", false )
runCase( "movd", true )
runCase( "movsd", false )
runCase( "movsd", true )
runCase( "movq", false )
runCase( "movq", true )

'' end of gas64-xmm-copies.bas

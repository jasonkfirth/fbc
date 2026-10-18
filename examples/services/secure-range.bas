'' FreeBASIC runtime examples: secure-range.bas
'' Purpose: Select uniform bounded values from checked cryptographic bytes.
'' Responsibilities: Reject modulo bias, bad bounds and provider failures.
'' This example does not change RND state or implement an entropy source.
#lang "fb"
#include once "fbrandom.bi"

function secure_below( byval bound as ulong, byref result as ulong ) as long
	if bound = 0 then return FB.SECURE_RANDOM_INVALID_ARGUMENT
	'' There are 2^32 possible ULONG values. Discard the incomplete low range
	'' so every residue modulo bound has exactly the same number of inputs.
	dim threshold as ulong = culng(4294967296ull mod culngint(bound))
	for attempt as long = 1 to 128
		dim sample as ulong
		dim status as long = FBC.secureRandom(@sample, sizeof(sample))
		if status <> FB.SECURE_RANDOM_OK then return status
		if sample >= threshold then
			result = sample mod bound
			return FB.SECURE_RANDOM_OK
		end if
	next
	'' Rejection probability is below one half per attempt. Bound retries
	'' nevertheless so an unexpectedly stuck provider cannot hang the caller.
	return FB.SECURE_RANDOM_IO_ERROR
end function

for roll as long = 1 to 12
	dim face as ulong
	dim status as long = secure_below(6, face)
	if status <> FB.SECURE_RANDOM_OK then
		print "No roll was generated, status "; status
		end 1
	end if
	print face + 1;
next
print
'' end of secure-range.bas

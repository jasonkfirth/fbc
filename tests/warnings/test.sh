#!/usr/bin/env bash
#
# FreeBASIC test suite: test.sh
# Purpose: Generate warning diagnostics for the maintained target profiles.
# Responsibilities: Invoke one compiler and normalize diagnostic locations.
# This file intentionally does NOT compile or link graphics implementations.
#
# This runs fbc on all the .bas files in this directory and captures fbc's
# output into corresponding .txt files. This is done for multiple compilation
# targets, which can produce different test results (e.g. due to 32bit vs 64bit
# differences or because sizeof(wstring) depends on the OS).
#
# fbc's output is being run through a sed replace to strip out the
# filename.bas(linenumber) part. This way we get the plain warning message only,
# and there won't be unnecessary changes due to changing line numbers when
# adding test cases, etc.
#
# The *.txt files are included in the Git repo, so Git can be used to check for
# problems such as missing or duplicate warnings.
#
# The test cases themselves use #print to indicate where warnings should appear,
# which allows checking the test results in detail.
#

FBC=`printenv FBC`
FBC=${FBC:-fbc}
FBC_FLAGS=${FBC_FLAGS:-}
TEST_FILTER=${TEST_FILTER:-*.bas}

# FBC names one executable. The Make caller supplies the toolchain environment
# separately; expanding a quoted shell command stored in FBC loses its quoting.
command -v "$FBC" >/dev/null 2>&1 || {
	echo "ERROR: warning-test compiler not found: $FBC" >&2
	exit 1
}

function run_tests() {
	fbtarget="$1"
	txtdir=r/$fbtarget
	test_count=0

	mkdir -p $txtdir
	rm -f $txtdir/*

	for i in $TEST_FILTER; do
		[ -f "$i" ] || continue
		test_count=$((test_count + 1))
		echo "TEST $fbtarget $i"
		withoutext=${i%.bas}

		"$FBC" $FBC_FLAGS -maxerr inf -w constness -target $fbtarget $i -r -m $withoutext | \
			sed -e 's,^.*\.\(bas\|bi\)(.*) warning .*(.*): ,\t,g' > \
			$txtdir/$withoutext.txt
		pipeline_result=("${PIPESTATUS[@]}")
		# A normal compiler error is part of some diagnostic fixtures. Tool
		# launch failures, crashes and output failures must fail this runner.
		if [ "${pipeline_result[0]}" -gt 1 ] || [ "${pipeline_result[1]}" -ne 0 ]; then
			echo "ERROR: warning-test command failed for $fbtarget $i" >&2
			exit 1
		fi
	done
	if [ "$test_count" -eq 0 ]; then
		echo "ERROR: warning-test filter matched no sources: $TEST_FILTER" >&2
		exit 1
	fi

	rm -f *.asm *.c

	# Change *.txt files over to CRLF on Windows (MSYS sed produces LF...)
	case `uname` in
	*MINGW*)
		unix2dos -q $txtdir/*
	esac
}

run_tests dos
run_tests linux-x86
run_tests linux-x86_64
run_tests win32
run_tests win64

# end of test.sh

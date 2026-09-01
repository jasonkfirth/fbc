#!/bin/sh

##############################################################################
# FreeBASIC Haiku fbctests grep compatibility helper
##############################################################################
#
# Purpose:
#
#   Provide the small grep option subset used by the fbctests makefiles on
#   Haiku nightlies where GNU grep cannot open files on a populated BFS volume.
#
# Responsibilities:
#
#   * match POSIX basic or extended regular expressions with case folding
#   * support normal, quiet, and file-name-only output
#   * retain grep-compatible success, no-match, and error exit statuses
#
# This script intentionally does NOT contain:
#
#   * recursive directory traversal
#   * GNU grep options not used by the fbctests makefiles
#
##############################################################################

set -u

ignore_case=0
list_only=0
quiet=0
exact_line=0
extended=0
pattern=
pattern_set=0

while [ "$#" -gt 0 ]; do
	case "$1" in
		-i) ignore_case=1 ;;
		-l) list_only=1 ;;
		-q) quiet=1 ;;
		-x) exact_line=1 ;;
		-E) extended=1 ;;
		-e)
			shift
			[ "$#" -gt 0 ] || {
				echo "grep.sh: -e requires a pattern" >&2
				exit 2
			}
			pattern="$1"
			pattern_set=1
			;;
		--)
			shift
			break
			;;
		-*)
			echo "grep.sh: unsupported option: $1" >&2
			exit 2
			;;
		*)
			if [ "$pattern_set" -eq 0 ]; then
				pattern="$1"
				pattern_set=1
				shift
				break
			fi
			break
			;;
	esac
	shift
done

if [ "$pattern_set" -eq 0 ] && [ "$#" -gt 0 ]; then
	pattern="$1"
	pattern_set=1
	shift
fi

[ "$pattern_set" -eq 1 ] || {
	echo "grep.sh: missing pattern" >&2
	exit 2
}

# The supported Haiku images provide GNU awk.  IGNORECASE and nextfile let the
# helper preserve grep's case-folding and one-name-per-matching-file behavior
# without opening the files through the failing GNU grep code path.
FB_TEST_GREP_PATTERN="$pattern" awk \
	-v ignore_case="$ignore_case" \
	-v list_only="$list_only" \
	-v quiet="$quiet" \
	-v exact_line="$exact_line" \
	-v extended="$extended" '
function basic_to_extended(source, result, escaped, i, ch) {
	result = ""
	escaped = 0

	for (i = 1; i <= length(source); i++) {
		ch = substr(source, i, 1)

		if (escaped != 0) {
			# These operators require a backslash in a basic expression,
			# but are operators without one in an extended expression.
			if (index("+?|(){}", ch) != 0)
				result = result ch
			else
				result = result "\\" ch

			escaped = 0
		} else if (ch == "\\") {
			escaped = 1
		} else if (index("+?|(){}", ch) != 0) {
			# The same characters are literals when they are not escaped in
			# a basic expression.
			result = result "\\" ch
		} else {
			result = result ch
		}
	}

	if (escaped != 0)
		result = result "\\\\"

	return result
}

BEGIN {
	pattern = ENVIRON["FB_TEST_GREP_PATTERN"]

	if (extended == 0)
		pattern = basic_to_extended(pattern)

	if (ignore_case != 0)
		IGNORECASE = 1

	file_count = ARGC - 1
	found = 0
}

function line_matches(line) {
	if (exact_line != 0)
		return line ~ ("^(" pattern ")$")

	return line ~ pattern
}

line_matches($0) {
	found = 1

	if (quiet != 0)
		exit 0

	if (list_only != 0) {
		print FILENAME
		nextfile
	}

	if (file_count > 1)
		print FILENAME ":" $0
	else
		print $0
}

END {
	if (found != 0)
		exit 0

	exit 1
}
' "$@"

exit $?

# end of grep.sh

'' FreeBASIC PCRE examples
'' File: preg_class.bas
'' Purpose: Demonstrate an owning match and replacement class.
'' Responsibilities: Bound result access, release captures, and advance empty matches.
'' This file contains no PCRE engine or concurrent object ownership.

/'
'' PCRE example?
'' Copyright (C) 2007  MindlessXD
''
'' This program is free software; you can redistribute it and/or
'' modify it under the terms of the GNU General Public License
'' as published by the Free Software Foundation; either version 2
'' of the License, or (at your option) any later version.
''
'' This program is distributed in the hope that it will be useful,
'' but WITHOUT ANY WARRANTY; without even the implied warranty of
'' MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
'' GNU General Public License for more details.
''
'' You should have received a copy of the GNU General Public License
'' along with this program; if not, write to the Free Software
'' Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
'/

/'
    Resource ownership:

        strings owns an array of match arrays.  Each match array owns the
        PCRE substring pointers returned for that match.  clean_up() releases
        every acquired level before clearing the owning fields.
'/

#include "pcre.bi"
#include once "match-progress.bi"

type preg_t
	as integer             substrings_limit '' the maximum number of substrings that may be matched, -1 is infinite

	as zstring ptr ptr ptr strings          '' read-only, access via result() and result_all()
	as integer             matches          '' read-only, number of resulting pattern matches
	as integer             substrings       '' read-only, number of resulting pattern substrings matches

	as long        errorcode                '' read-only
	as const zstring ptr err_               '' read-only
	as long        erroffset                '' read-only

	'' use match() to match only one instance, use match_all() to match all instances
	declare function match(pattern as zstring ptr, subject as zstring ptr, byval offset as long = 0, byval flags as long = 0) as integer
	declare function match_all(pattern as zstring ptr, subject as zstring ptr, byval offset as long = 0, byval flags as long = 0) as integer

	'' use result() and result_all() corresponding with use of match() and match_all() respectively
	'' result(0) is the whole match, result(n) is the nth substring
	declare function result(byval substring as integer = 0) as string
	declare function result_all(byval match_ as integer, byval substring as integer = 0) as string

	'' replacments use '$n' for substring replacements
	'' there is an optional terminating $ (i.e. '$n$') to allow numbers to directly follow replacments
	'' to insert a literal '$' use '$$'
	declare function replace(pattern as zstring ptr, replacement as zstring ptr, subject as zstring ptr, byval offset as long = 0, byval flags as long = 0) as integer

	declare function append_match() as integer
	declare function clean_up() as integer

	declare constructor (byval substrings_limit_ as integer = -1)
	declare destructor ()
private:
	'' Match arrays have one owner; copying their pointers would double-free them.
	declare constructor(byref other as const preg_t)
	declare operator let(byref other as const preg_t)
end type

function preg_t.append_match() as integer
	dim as zstring ptr ptr ptr resized_strings
	dim as zstring ptr ptr new_match

	if this.matches < 0 or this.matches >= (cast(uinteger, -1) shr 1) \ sizeof(zstring ptr ptr) then return -1
	resized_strings = reallocate(this.strings, (this.matches + 1) * sizeof(zstring ptr ptr))
	if resized_strings = 0 then return -1

	this.strings = resized_strings
	new_match = callocate((this.substrings + 1) * sizeof(zstring ptr))
	if new_match = 0 then return -1

	'' append_match allocated this exact match slot. FB-LINTER: DISABLE-NEXT-LINE FBL525
	this.strings[this.matches] = new_match
	function = this.matches
	this.matches += 1
end function

function preg_t.match(pattern as zstring ptr, subject as zstring ptr, byval offset as long = 0, byval flags as long = 0) as integer
	this.clean_up()

	this.errorcode = 0
	if pattern = NULL or subject = NULL then
		this.errorcode = PCRE_ERROR_NULL
		return this.errorcode
	end if
	if len(*subject) > &h7fffffff then
		this.errorcode = PCRE_ERROR_BADLENGTH
		return this.errorcode
	end if
	if offset < 0 or offset > len(*subject) then
		this.errorcode = PCRE_ERROR_BADOFFSET
		return this.errorcode
	end if

	dim as pcre ptr temp_c = pcre_compile2(pattern, flags, @this.errorcode, @this.err_, @this.erroffset, 0)
	if temp_c = 0 then return this.errorcode

	dim capture_count as long
	this.errorcode = pcre_fullinfo(temp_c, 0, PCRE_INFO_CAPTURECOUNT, @capture_count)
	if this.errorcode <> 0 then
		pcre_free(temp_c)
		return this.errorcode
	end if
	this.substrings = capture_count
	if (this.substrings_limit >= 0 and this.substrings_limit < this.substrings) then this.substrings = this.substrings_limit

	dim as long ptr vector = allocate((this.substrings + 1) * 3 * sizeof(long))
	if vector = 0 then
		pcre_free(temp_c)
		this.errorcode = PCRE_ERROR_NOMEMORY
		return this.errorcode
	end if

	dim as long temp_e = pcre_exec(temp_c, 0, subject, len(*subject), offset, 0 /'flags'/, vector, (this.substrings + 1) * 3)
	if temp_e = 0 then temp_e = this.substrings + 1
	if temp_e < 0 then
		this.errorcode = temp_e
	else
		dim as integer match_index = this.append_match()
		if match_index < 0 then
			this.clean_up()
			this.errorcode = PCRE_ERROR_NOMEMORY
		else
		for i as integer = 0 to temp_e - 1
			dim as const zstring ptr substring
			dim as integer substring_result = pcre_get_substring(subject, vector, temp_e, i, @substring)
			if substring_result < 0 then
				this.errorcode = substring_result
				exit for
			end if
			'' PCRE returned substring i within the newly allocated match array. FB-LINTER: DISABLE-NEXT-LINE FBL525
			this.strings[match_index][i] = cast(zstring ptr, substring)
		next
			if this.errorcode < 0 then this.clean_up()
		end if
	end if
	if this.errorcode = PCRE_ERROR_NOMATCH then this.errorcode = 0

	deallocate(vector)
	vector = 0

	pcre_free(temp_c)

	return this.errorcode
end function

function preg_t.match_all(pattern as zstring ptr, subject as zstring ptr, byval offset as long = 0, byval flags as long = 0) as integer
	this.clean_up()

	this.errorcode = 0
	if pattern = NULL or subject = NULL then
		this.errorcode = PCRE_ERROR_NULL
		return this.errorcode
	end if
	if len(*subject) > &h7fffffff then
		this.errorcode = PCRE_ERROR_BADLENGTH
		return this.errorcode
	end if
	if offset < 0 or offset > len(*subject) then
		this.errorcode = PCRE_ERROR_BADOFFSET
		return this.errorcode
	end if

	dim as pcre ptr temp_c = pcre_compile2(pattern, flags, @this.errorcode, @this.err_, @this.erroffset, 0)
	if temp_c = 0 then return this.errorcode

	dim capture_count as long
	this.errorcode = pcre_fullinfo(temp_c, 0, PCRE_INFO_CAPTURECOUNT, @capture_count)
	if this.errorcode <> 0 then
		pcre_free(temp_c)
		return this.errorcode
	end if
	this.substrings = capture_count
	if (this.substrings_limit >= 0 and this.substrings_limit < this.substrings) then this.substrings = this.substrings_limit

	dim as long ptr vector = allocate((this.substrings + 1) * 3 * sizeof(long))
	if vector = 0 then
		pcre_free(temp_c)
		this.errorcode = PCRE_ERROR_NOMEMORY
		return this.errorcode
	end if

	dim as long temp_l = len(*subject)
	dim as long temp_o = offset
	do
		dim as long temp_e
		if this.matches = 0 then
			temp_e = pcre_exec(temp_c, 0, subject, temp_l, temp_o, 0, vector, (this.substrings + 1) * 3)
		else
			temp_e = pcre_next_match(temp_c, 0, subject, temp_l, vector[0], vector[1], 0, vector, (this.substrings + 1) * 3)
		end if
		if temp_e = 0 then temp_e = this.substrings + 1
		if temp_e < 0 then
			this.errorcode = temp_e
		else
			dim as integer match_index = this.append_match()
			if match_index < 0 then
				this.clean_up()
				this.errorcode = PCRE_ERROR_NOMEMORY
			else
			for i as integer = 0 to temp_e - 1
				dim as const zstring ptr substring
				dim as integer substring_result = pcre_get_substring(subject, vector, temp_e, i, @substring)
				if substring_result < 0 then
					this.errorcode = substring_result
					exit for
				end if
				'' PCRE returned substring i within the newly allocated match array. FB-LINTER: DISABLE-NEXT-LINE FBL525
				this.strings[match_index][i] = cast(zstring ptr, substring)
			next
				if this.errorcode < 0 then
					this.clean_up()
					exit do
				end if
				'' PCRE writes the next subject offset into vector slot 1. FB-LINTER: DISABLE-NEXT-LINE FBL525
				temp_o = vector[1]
			end if
		end if
	loop while this.errorcode = 0
	if this.errorcode = PCRE_ERROR_NOMATCH then this.errorcode = 0

	deallocate(vector)
	vector = 0

	pcre_free(temp_c)

	return this.errorcode
end function

function preg_t.replace(pattern as zstring ptr, replacement as zstring ptr, subject as zstring ptr, byval offset as long = 0, byval flags as long = 0) as integer
	this.clean_up()

	this.errorcode = 0
	if pattern = NULL or subject = NULL then
		this.errorcode = PCRE_ERROR_NULL
		return this.errorcode
	end if
	if len(*subject) > &h7fffffff then
		this.errorcode = PCRE_ERROR_BADLENGTH
		return this.errorcode
	end if
	if offset < 0 or offset > len(*subject) then
		this.errorcode = PCRE_ERROR_BADOFFSET
		return this.errorcode
	end if

	if replacement = NULL then
		this.errorcode = PCRE_ERROR_NULL
		return this.errorcode
	end if

	dim as pcre ptr temp_c = pcre_compile2(pattern, flags, @this.errorcode, @this.err_, @this.erroffset, 0)
	if temp_c = 0 then return this.errorcode

	dim capture_count as long
	this.errorcode = pcre_fullinfo(temp_c, 0, PCRE_INFO_CAPTURECOUNT, @capture_count)
	if this.errorcode <> 0 then
		pcre_free(temp_c)
		return this.errorcode
	end if
	this.substrings = capture_count
	if (this.substrings_limit >= 0 and this.substrings_limit < this.substrings) then this.substrings = this.substrings_limit

	dim as string result_ = left(*subject, offset)

	dim as long ptr vector = allocate((this.substrings + 1) * 3 * sizeof(long))
	if vector = 0 then
		pcre_free(temp_c)
		this.errorcode = PCRE_ERROR_NOMEMORY
		return this.errorcode
	end if

	dim as long temp_s = len(*subject)
	dim as long temp_o = offset
	dim first_match as boolean = true
	do
		dim as long temp_e
		if first_match then
			temp_e = pcre_exec(temp_c, 0, subject, temp_s, temp_o, 0, vector, (this.substrings + 1) * 3)
			first_match = false
		else
			temp_e = pcre_next_match(temp_c, 0, subject, temp_s, vector[0], vector[1], 0, vector, (this.substrings + 1) * 3)
		end if
		if temp_e = 0 then temp_e = this.substrings + 1
		if temp_e < 0 then
			this.errorcode = temp_e
		else
			'' PCRE replacement output has variable-length matches. FB-LINTER: DISABLE-NEXT-LINE FBL503
			result_ &= left(subject[temp_o], vector[0] - temp_o)

			dim replacement_text as string = *replacement
			dim token as integer = 0
			do while token < len(replacement_text)
				if replacement_text[token] <> asc("$") then
					result_ &= chr(replacement_text[token])
					token += 1
				elseif token+1 < len(replacement_text) andalso replacement_text[token+1] = asc("$") then
					result_ &= "$"
					token += 2
				else
					dim digit_position as integer = token+1
					dim capture_index as long = 0
					dim overflowed as boolean = false
					do while digit_position < len(replacement_text)
						dim digit as long = replacement_text[digit_position]-asc("0")
						if digit < 0 or digit > 9 then exit do
						if capture_index > (&h7fffffff-digit) \ 10 then
							overflowed = true
						elseif not overflowed then
							capture_index = capture_index*10+digit
						end if
						digit_position += 1
					loop
					if digit_position = token+1 then
						result_ &= "$"
						token += 1
					else
						if not overflowed andalso capture_index < temp_e then
							if vector[2*capture_index] >= 0 then
								result_ &= mid(*subject, vector[2*capture_index]+1, vector[2*capture_index+1]-vector[2*capture_index])
							end if
						end if
						if digit_position < len(replacement_text) andalso replacement_text[digit_position] = asc("$") then digit_position += 1
						token = digit_position
					end if
				end if
			loop

			'' PCRE writes the next subject offset into vector slot 1. FB-LINTER: DISABLE-NEXT-LINE FBL525
			temp_o = vector[1]
		end if
	loop while this.errorcode = 0
	if this.errorcode = PCRE_ERROR_NOMATCH then
		result_ &= subject[temp_o]
		this.errorcode = 0
	end if

	if this.errorcode = 0 then
		dim as integer match_index = this.append_match()
		if match_index < 0 then
			this.clean_up()
			this.errorcode = PCRE_ERROR_NOMEMORY
		else
			this.strings[match_index][0] = pcre_malloc(len(result_) + 1)
			if this.strings[match_index][0] = 0 then
				this.clean_up()
				this.errorcode = PCRE_ERROR_NOMEMORY
			else
				*this.strings[match_index][0] = result_
				this.substrings = 0
			end if
		end if
	end if

	deallocate(vector)
	vector = 0

	pcre_free(temp_c)

	return this.errorcode
end function

function preg_t.result(byval substring as integer = 0) as string
	if (0 < matches and substring >= 0 and substring <= this.substrings) then
		if (this.strings[0][substring]) then
			return *this.strings[0][substring]
		end if
	end if
	return ""
end function

function preg_t.result_all(byval match_ as integer, byval substring as integer = 0) as string
	if (match_ >= 0 and match_ < matches and substring >= 0 and substring <= this.substrings) then
		if (this.strings[match_][substring]) then
			return *this.strings[match_][substring]
		end if
	end if
	return ""
end function

function preg_t.clean_up() as integer
	this.err_ = 0

	if this.strings then
		for i as integer = 0 to this.matches - 1
			for j as integer = 0 to this.substrings
				if this.strings[i][j] then
					pcre_free_substring(this.strings[i][j])
					this.strings[i][j] = 0
				end if
			next
			deallocate(this.strings[i])
			this.strings[i] = 0
		next
		deallocate(this.strings)
		this.strings = 0
	end if

	this.matches = 0
	this.substrings = 0

	return 0
end function

constructor preg_t(byval substrings_limit_ as integer = -1)
	this.substrings_limit = substrings_limit_
	this.strings = 0
	this.matches = 0
	this.substrings = 0
	this.errorcode = 0
	this.err_ = 0
	this.erroffset = 0
end constructor

destructor preg_t()
	clean_up()
end destructor

function preg_match_simple(pattern as zstring ptr, subject as zstring ptr, byval offset as long = 0, byval flags as long = 0) as string
	dim as preg_t preg = 0
	preg.match(pattern, subject, offset, flags)
	return preg.result()
end function

function preg_replace_simple(pattern as zstring ptr, replacement as zstring ptr, subject as zstring ptr, byval offset as long = 0, byval flags as long = 0) as string
	dim as preg_t preg
	preg.replace(pattern, replacement, subject, offset, flags)
	return preg.result()
end function

#ifndef REGEX_EXAMPLE_NO_MAIN
'' a few tests
scope
	dim as preg_t foo

	if foo.match("(foo)?ba(r|z)?", "foo foobar bar foobaz baz") then
		? "ERROR: " & foo.errorcode
	else
		? "match:" & foo.result()
		for j as integer = 1 to foo.substrings
			? "substring: " & foo.result(j)
		next
	end if
	?

	if foo.match_all("(foo)?ba(r|z)?", "foo foobar bar foobaz baz") then
		? "ERROR: " & foo.errorcode
	else
		for i as integer = 0 to foo.matches - 1
			? "match: " & foo.result_all(i)
			for j as integer = 1 to foo.substrings
				? "substring: " & foo.result_all(i, j)
			next
		next
	end if
	?
end scope

? preg_match_simple("ba[y-z]", "foo bar baz bay")
? preg_match_simple("ba.", "bar baz", 2)
? preg_match_simple(".OO", "foo", , PCRE_CASELESS)
?

? preg_replace_simple("(foo)?ba(r|z)", "//$0//$1$1//$2$$$2//$9//", "foo--foobaz--bar--baz--foo")
? preg_replace_simple("(foo)?ba(r|z)", "//$$//$0//", "foo--bar--foo")
? preg_replace_simple("(foo)?ba(r|z)", "//$-//$0//", "foo--bar--foo")
? preg_replace_simple("(foo)?ba(r|z)", "//$999//$0//", "foo--bar--foo")
?
#endif

'' end of preg_class.bas

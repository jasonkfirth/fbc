'' Project: FreeBASIC semantic binary transport
'' File: fbc-semantic-binary.bi
'' Purpose: Encode and read compact, self-describing compiler observations.
'' Responsibilities: Bound framing, integer coding, string interning and headers.
'' This file intentionally does NOT contain semantic validation or lint rules.
'' Targets: Native FreeBASIC compiler and linter host builds; linted targets use
'' the compiler's target options.
'' Module API: Internal include-only codec shared by the compiler and linter.
'' This file intentionally does NOT contain:
''     - language parser behavior or lint policy, which remain in the callers.
''
'' Ownership: A writer dictionary belongs to one serial export transaction. A
'' reader owns its compact bytes and strings; indexed reads are immutable after
'' the sequential load. No native structure or pointer is written to disk.

#ifndef FBC_SEMANTIC_BINARY_BI
#define FBC_SEMANTIC_BINARY_BI
#include once "fbc-semantic-elements.bi"

private const FBCSEM_BINARY_MAGIC = "FBCSEMB1" + chr(10)
private const FBCSEM_BINARY_MAX_BYTES = 1792 * 1024 * 1024
private const FBCSEM_BINARY_MAX_RECORD = 64 * 1024 * 1024
private const FBCSEM_BINARY_MAX_HEADER = 64 * 1024
private const FBCSEM_BINARY_MAX_STRINGS = 262144
private const FBCSEM_BINARY_MAX_STRING_BYTES = 64 * 1024 * 1024
private const FBCSEM_BINARY_MAX_TUPLE_ITEMS = 256
private const FBCSEM_BINARY_HASH_BUCKETS = 65536
'' Three low token bits describe the value; the remaining bits hold its value,
'' byte length or dictionary identity. Values too large for 61 bits stay text.
private const FBCSEM_BINARY_MAX_INTEGER as ulongint = &h1fffffffffffffffULL

type FbcSemDictionary
	values(any) as string
	links(any) as long
	heads(0 to FBCSEM_BINARY_HASH_BUCKETS - 1) as long
	count as long
	capacity as long
	bytes as long
end type

type FbcSemReader
	bytes_text as string
	tags(0 to 255) as string
	arities(0 to 255) as integer
	strings(any) as string
	string_count as long
	string_capacity as long
	string_bytes as long
	position as long
	row_start as long
	row_length as long
	capabilities as integer
	first as integer
	binary as integer
	header as string
	mode as string
	failure as string
end type

'' -------------------------------------------------------------------------
'' Portable integers and writer dictionary
'' -------------------------------------------------------------------------

private function FbcSemVarint(byval number as ulongint) as string
	dim as string result
	do
		dim as ubyte part = number and 127
		number shr= 7
		if number <> 0 then part or= 128
		result += chr(part)
	loop while number <> 0
	return result
end function

private function FbcSemGetVarint(byref bytes_text as const string, byref position as long, _
	byval end_position as long, byref number as ulongint) as integer
	number = 0
	for shift as integer = 0 to 63 step 7
		if position < 0 orelse position >= end_position then return 0
		dim as ubyte part = bytes_text[position]
		position += 1
		if shift = 63 andalso part > 1 then return 0
		number or= culngint(part and 127) shl shift
		if (part and 128) = 0 then
			'' A canonical encoding has no redundant leading zero groups.
			if shift > 0 andalso part = 0 then return 0
			return -1
		end if
	next
	return 0
end function

private function FbcSemHash(byref text as const string) as ulong
	'' FNV-1a's 32-bit offset basis and prime; buckets have a power-of-two count.
	dim as ulong result = 2166136261U
	for index as integer = 0 to len(text) - 1
		result = culng((culngint(result xor text[index]) * 16777619ULL) and &hffffffffULL)
	next
	return result and (FBCSEM_BINARY_HASH_BUCKETS - 1)
end function

private sub FbcSemRollback(byref dictionary as FbcSemDictionary, byval count as long)
	if count < 0 orelse count > dictionary.count then exit sub
	while dictionary.count > count
		dim as long slot = dictionary.count
		dim as ulong bucket = FbcSemHash(dictionary.values(slot))
		dictionary.heads(bucket) = dictionary.links(slot)
		dictionary.bytes -= len(dictionary.values(slot))
		dictionary.values(slot) = ""
		dictionary.count -= 1
	wend
end sub

private function FbcSemEncodeString(byref dictionary as FbcSemDictionary, _
	byref text as const string) as string
	dim as ulong bucket = FbcSemHash(text)
	dim as long slot = dictionary.heads(bucket)
	while slot > 0
		if dictionary.values(slot) = text then return FbcSemVarint((culngint(slot) shl 3) or 3)
		slot = dictionary.links(slot)
	wend
	if dictionary.count >= FBCSEM_BINARY_MAX_STRINGS orelse _
	   len(text) > FBCSEM_BINARY_MAX_STRING_BYTES - dictionary.bytes then
		return FbcSemVarint((culngint(len(text)) shl 3) or 5) + text
	end if
	if dictionary.count + 1 >= dictionary.capacity then
		dim as long capacity = dictionary.capacity * 2
		if capacity = 0 then capacity = 256
		if capacity > FBCSEM_BINARY_MAX_STRINGS + 1 then capacity = FBCSEM_BINARY_MAX_STRINGS + 1
		redim preserve dictionary.values(0 to capacity - 1)
		redim preserve dictionary.links(0 to capacity - 1)
		dictionary.capacity = capacity
	end if
	dictionary.count += 1
	slot = dictionary.count
	dictionary.values(slot) = text
	dictionary.links(slot) = dictionary.heads(bucket)
	dictionary.heads(bucket) = slot
	dictionary.bytes += len(text)
	return FbcSemVarint((culngint(len(text)) shl 3) or 4) + text
end function

private function FbcSemDecimal(byref text as const string, byref magnitude as ulongint, _
	byref negative as integer) as integer
	magnitude = 0
	negative = 0
	dim as integer start = 0
	if len(text) = 0 then return 0
	if text[0] = asc("-") then
		negative = -1
		start = 1
	end if
	if start >= len(text) then return 0
	if text[start] = asc("0") andalso len(text) - start > 1 then return 0
	if negative andalso mid(text, start + 1) = "0" then return 0
	for index as integer = start to len(text) - 1
		dim as integer digit = text[index] - asc("0")
		if digit < 0 orelse digit > 9 then return 0
		if magnitude > (FBCSEM_BINARY_MAX_INTEGER - digit) \ 10 then return 0
		magnitude = magnitude * 10 + digit
	next
	return -1
end function

private function FbcSemEncodeField(byref dictionary as FbcSemDictionary, _
	byref text as const string) as string
	if len(text) = 0 then return chr(0)
	'' Many observation receipts contain their own tab-separated fields, escaped
	'' once by the outer record. Encode those components too, rather than keeping
	'' a unique, long textual tuple for every expression or declaration.
	if instr(text, "%09") > 0 then
		dim as integer count = 1, cursor = 1
		do
			dim as integer separator = instr(cursor, text, "%09")
			if separator = 0 then exit do
			count += 1
			cursor = separator + 3
		loop while count <= FBCSEM_BINARY_MAX_TUPLE_ITEMS
		if count <= FBCSEM_BINARY_MAX_TUPLE_ITEMS then
			dim as string result = FbcSemVarint((culngint(count) shl 3) or 7)
			cursor = 1
			for index as integer = 1 to count
				dim as integer separator = instr(cursor, text, "%09")
				if separator = 0 then separator = len(text) + 1
				result += FbcSemEncodeField(dictionary, mid(text, cursor, separator - cursor))
				cursor = separator + 3
			next
			return result
		end if
	end if
	dim as ulongint magnitude
	dim as integer negative
	if FbcSemDecimal(text, magnitude, negative) then
		return FbcSemVarint((magnitude shl 3) or iif(negative, 2, 1))
	end if
	'' Observation keys frequently append a source identity to a stable name.
	'' Intern the name once rather than allocating one string for every ID.
	dim as integer separator = instrrev(text, ":")
	if separator > 1 andalso separator < len(text) then
		dim as string suffix = mid(text, separator + 1)
		if FbcSemDecimal(suffix, magnitude, negative) andalso negative = 0 then
			dim as string prefix = left(text, separator)
			return chr(6) + FbcSemEncodeString(dictionary, prefix) + FbcSemVarint(magnitude)
		end if
	end if
	return FbcSemEncodeString(dictionary, text)
end function

private function FbcSemPart(byref text as const string, byval field_index as integer) as string
	dim as integer start = 1
	for index as integer = 0 to field_index
		dim as integer ending = instr(start, text, chr(9))
		if index = field_index then
			if ending = 0 then return mid(text, start)
			return mid(text, start, ending - start)
		end if
		if ending = 0 then return ""
		start = ending + 1
	next
	return ""
end function

private function FbcSemPartCount(byref text as const string) as integer
	dim as integer count = 1
	for index as integer = 0 to len(text) - 1
		if text[index] = 9 then count += 1
	next
	return count
end function

private function FbcSemTagId(byref tag as const string) as integer
	return FbcSemElementId(tag)
end function

private function FbcSemEncodeRecord(byref dictionary as FbcSemDictionary, _
	byref line_text as const string) as string
	dim as integer tag_end = instr(line_text, chr(9))
	if tag_end = 0 then return ""
	dim as integer identity = FbcSemTagId(left(line_text, tag_end - 1))
	if identity < 1 orelse identity > 255 then return ""
	dim as integer count = 1
	for index as integer = tag_end to len(line_text) - 1
		if line_text[index] = 9 then count += 1
	next
	if count > 254 then return ""
	dim as string result = chr(identity) + chr(count)
	dim as integer start = tag_end + 1
	for index as integer = 1 to count
		dim as integer ending = instr(start, line_text, chr(9))
		if ending = 0 then ending = len(line_text) + 1
		result += FbcSemEncodeField(dictionary, mid(line_text, start, ending - start))
		start = ending + 1
	next
	if len(result) > FBCSEM_BINARY_MAX_RECORD then return ""
	'' Frames use an explicitly little-endian 32-bit byte length.
	dim as ulong bytes = len(result)
	return chr(bytes and 255, (bytes shr 8) and 255, (bytes shr 16) and 255, (bytes shr 24) and 255) + result
end function

private function FbcSemHeader(byref producer as const string, byref mode as const string, _
	byval macros as integer) as string
	dim as string result = FBCSEM_BINARY_MAGIC
	result += "schema" + chr(9) + "27" + chr(10)
	result += "producer" + chr(9) + producer + chr(10)
	result += "mode" + chr(9) + mode + chr(10)
	result += "layout" + chr(9) + "u32le frame bytes; u8 record ID; u8 value count; canonical unsigned LEB128 tokens" + chr(10)
	result += "values" + chr(9) + "low 3 bits: 0 empty, 1 unsigned, 2 negative magnitude, 3 string ID, 4 new string bytes, 5 literal bytes, 6 prefix token then numeric suffix, 7 tuple joined by %09; upper bits value/length/count; strings use schema-27 byte escapes" + chr(10)
	result += "dictionary" + chr(9) + "IDs start at 1 in encounter order; definitions survive committed modules; no forward references" + chr(10)
	result += "cap" + chr(9) + "core" + chr(9) + "1" + chr(9) + "1" + chr(10)
	result += "cap" + chr(9) + "source-revisions" + chr(9) + "1" + chr(9) + "1" + chr(10)
	result += "cap" + chr(9) + "structured-values" + chr(9) + "1" + chr(9) + "1" + chr(10)
	if mode = "full" then
		result += "cap" + chr(9) + "native-types" + chr(9) + "1" + chr(9) + "1" + chr(10)
		result += "cap" + chr(9) + "control-flow" + chr(9) + "1" + chr(9) + "1" + chr(10)
	end if
	if macros then result += "cap" + chr(9) + "macro-provenance" + chr(9) + "1" + chr(9) + "0" + chr(10)
	result += FbcSemElements() + "DATA" + chr(10)
	return result
end function

'' -------------------------------------------------------------------------
'' Bounded file reader and immutable indexed values
'' -------------------------------------------------------------------------

private sub FbcSemReset(byref reader as FbcSemReader)
	erase reader.strings
	erase reader.tags
	erase reader.arities
	reader.string_count = 0
	reader.string_capacity = 0
	reader.string_bytes = 0
	reader.capabilities = 0
	reader.failure = ""
	reader.bytes_text = ""
	reader.header = ""
	reader.mode = ""
	reader.binary = 0
	reader.position = 0
	reader.row_start = 0
	reader.row_length = 0
	reader.first = 0
end sub

'' The caller transfers ownership of one bounded input buffer. Native threaded
'' consumers can read through private OS handles instead of FreeBASIC file slots.
private function FbcSemStart(byref reader as FbcSemReader, byref input_bytes as string) as integer
	FbcSemReset reader
	if len(input_bytes) < 1 orelse len(input_bytes) > FBCSEM_BINARY_MAX_BYTES then return 0
	swap reader.bytes_text, input_bytes
	reader.first = -1
	reader.binary = (left(reader.bytes_text, len(FBCSEM_BINARY_MAGIC)) = FBCSEM_BINARY_MAGIC)
	if reader.binary = 0 then return -1
	reader.position = len(FBCSEM_BINARY_MAGIC)
	dim as string schema, producer, known = FbcSemElements()
	dim as integer ended, seen_capabilities, seen_metadata
	dim as string capability_names = chr(9)
	while reader.position < len(reader.bytes_text) andalso reader.position < FBCSEM_BINARY_MAX_HEADER
		dim as integer ending = instr(reader.position + 1, reader.bytes_text, chr(10))
		if ending = 0 orelse ending > FBCSEM_BINARY_MAX_HEADER then exit while
		dim as string line_text = mid(reader.bytes_text, reader.position + 1, ending - reader.position - 1)
		reader.position = ending
		'' Header descriptions are ASCII wire data too. Controls must not forge
		'' boundaries when a consumer projects the header as a logical TSV row.
		for index as integer = 0 to len(line_text) - 1
			if (line_text[index] < 32 andalso line_text[index] <> 9) orelse line_text[index] > 126 then
				reader.failure = "invalid-header-byte"
				return 0
			end if
		next
		dim as string element = FbcSemPart(line_text, 0)
		dim as integer metadata_mask
		select case element
		case "schema": metadata_mask = 1
		case "producer": metadata_mask = 2
		case "mode": metadata_mask = 4
		case "layout": metadata_mask = 8
		case "values": metadata_mask = 16
		case "dictionary": metadata_mask = 32
		end select
		if metadata_mask then
			if FbcSemPartCount(line_text) <> 2 orelse len(FbcSemPart(line_text, 1)) = 0 orelse _
			   (seen_metadata and metadata_mask) <> 0 then
				reader.failure = "duplicate-or-invalid-header-metadata"
				return 0
			end if
			seen_metadata or= metadata_mask
		end if
		select case element
		case "schema": schema = FbcSemPart(line_text, 1)
		case "producer": producer = FbcSemPart(line_text, 1)
		case "mode"
			reader.mode = FbcSemPart(line_text, 1)
			if reader.mode <> "full" andalso reader.mode <> "bindings" andalso reader.mode <> "expressions" then
				reader.failure = "unsupported-export-mode:" + reader.mode
				return 0
			end if
		case "record"
			if FbcSemPartCount(line_text) <> 5 orelse len(FbcSemPart(line_text, 4)) = 0 then
				reader.failure = "invalid-record-definition"
				return 0
			end if
			dim as ulongint identity, arity
			dim as integer negative
			if FbcSemDecimal(FbcSemPart(line_text, 1), identity, negative) = 0 orelse negative then exit while
			if identity < 1 orelse identity > 255 then exit while
			if FbcSemDecimal(FbcSemPart(line_text, 3), arity, negative) = 0 orelse negative then exit while
			if arity < 2 orelse arity > 255 orelse len(reader.tags(identity)) <> 0 then exit while
			dim as string tag = FbcSemPart(line_text, 2)
			dim as integer ordinal = FbcSemTagId(tag)
			if ordinal = 0 then reader.failure = "unsupported-record:" + tag: return 0
			dim as string expected = "record" + chr(9) + ltrim(str(ordinal)) + chr(9) + tag + chr(9) + ltrim(str(arity)) + chr(9)
			if instr(known, expected) = 0 then reader.failure = "record-contract:" + tag: return 0
			for index as integer = 1 to 255
				if reader.tags(index) = tag then reader.failure = "duplicate-record:" + tag: return 0
			next
			reader.tags(identity) = tag
			reader.arities(identity) = arity
		case "cap"
			if FbcSemPartCount(line_text) <> 4 orelse _
			   (FbcSemPart(line_text, 3) <> "0" andalso FbcSemPart(line_text, 3) <> "1") then
				reader.failure = "invalid-capability-definition"
				return 0
			end if
			dim as integer mask
			dim as string capability = FbcSemPart(line_text, 1)
			if len(capability) = 0 orelse instr(capability_names, chr(9) + capability + chr(9)) <> 0 then
				reader.failure = "duplicate-or-empty-capability"
				return 0
			end if
			capability_names += capability + chr(9)
			dim as ulongint version_number
			dim as integer negative
			if FbcSemDecimal(FbcSemPart(line_text, 2), version_number, negative) = 0 orelse _
			   negative orelse version_number = 0 then
				reader.failure = "invalid-capability-version:" + capability
				return 0
			end if
			select case capability
			case "core": mask = 1
			case "source-revisions": mask = 2
			case "native-types": mask = 4
			case "control-flow": mask = 8
			case "macro-provenance": mask = 16
			case "structured-values": mask = 32
			end select
			if FbcSemPart(line_text, 2) <> "1" then mask = 0
			if mask = 0 andalso FbcSemPart(line_text, 3) <> "0" then
				reader.failure = "unsupported-capability:" + capability
				return 0
			end if
			if (seen_capabilities and mask) <> 0 then reader.failure = "duplicate-capability": return 0
			seen_capabilities or= mask
			reader.capabilities or= mask
		case "DATA"
			if line_text <> "DATA" then exit while
			ended = -1
			exit while
		case "layout", "values", "dictionary"
			'' Human-readable descriptions do not override transport version 1.
		case else
			reader.failure = "unknown-header-element"
			return 0
		end select
	wend
	'' The six required metadata fields occupy the low six bits of this mask.
	if ended = 0 orelse seen_metadata <> 63 orelse schema <> "27" orelse len(producer) = 0 then
		reader.failure = "invalid-binary-header"
		return 0
	end if
	if (reader.capabilities and 3) <> 3 then
		reader.failure = "missing-capability: core, source-revisions"
		return 0
	end if
	reader.header = "FBCSEM" + chr(9) + schema + chr(9) + producer
	return -1
end function

private function FbcSemLoad(byref reader as FbcSemReader, byref filename as const string) as integer
	FbcSemReset reader
	dim as integer file_number = freefile
	if open(filename for binary access read as #file_number) <> 0 then return 0
	dim as longint bytes = lof(file_number)
	if bytes < 1 orelse bytes > FBCSEM_BINARY_MAX_BYTES then close #file_number: return 0
	dim as string input_bytes = space(bytes)
	dim as integer status = get(#file_number, , input_bytes)
	dim as integer stable = (lof(file_number) = bytes)
	close #file_number
	if status <> 0 orelse stable = 0 then return 0
	return FbcSemStart(reader, input_bytes)
end function

'' Index construction needs byte boundaries, not allocated string values. The
'' sequential reader already validated references and escapes before this runs.
private function FbcSemSkipValue(byref reader as FbcSemReader, byref position as long, _
	byval ending as long, byval prefix as integer = 0, byval grouped as integer = 0) as integer
	dim as ulongint token, suffix
	if FbcSemGetVarint(reader.bytes_text, position, ending, token) = 0 then return 0
	select case token and 7
	case 0, 1, 2, 3: return -1
	case 4, 5
		dim as ulongint bytes = token shr 3
		if bytes > culngint(ending - position) then return 0
		position += bytes
	case 6
		if prefix then return 0
		if FbcSemSkipValue(reader, position, ending, -1, grouped) = 0 then return 0
		if FbcSemGetVarint(reader.bytes_text, position, ending, suffix) = 0 then return 0
	case 7
		dim as ulongint count = token shr 3
		if prefix orelse grouped orelse count < 2 orelse count > FBCSEM_BINARY_MAX_TUPLE_ITEMS then return 0
		for index as integer = 1 to count
			if FbcSemSkipValue(reader, position, ending, 0, -1) = 0 then return 0
		next
	case else: return 0
	end select
	return -1
end function

private function FbcSemReadValue(byref reader as FbcSemReader, byref position as long, _
	byval ending as long, byref value as string, byval loading as integer, _
	byval prefix as integer = 0, byval grouped as integer = 0) as integer
	dim as ulongint token
	value = ""
	if FbcSemGetVarint(reader.bytes_text, position, ending, token) = 0 then return 0
	dim as integer kind = token and 7
	dim as ulongint payload = token shr 3
	select case kind
	case 0
		return (payload = 0)
	case 1, 2
		value = ltrim(str(payload))
		if kind = 2 then
			if payload = 0 then return 0
			value = "-" + value
		end if
	case 3
		if payload < 1 orelse payload > reader.string_count then return 0
		value = reader.strings(payload)
	case 4, 5
		if payload = 0 orelse payload > culngint(ending - position) then return 0
		if kind = 4 andalso loading then
			if reader.string_count >= FBCSEM_BINARY_MAX_STRINGS orelse _
			   payload > FBCSEM_BINARY_MAX_STRING_BYTES - reader.string_bytes then return 0
		end if
		value = mid(reader.bytes_text, position + 1, payload)
		position += payload
		if kind = 4 andalso loading then
			if reader.string_count + 1 >= reader.string_capacity then
				dim as long capacity = reader.string_capacity * 2
				if capacity = 0 then capacity = 256
				if capacity > FBCSEM_BINARY_MAX_STRINGS + 1 then capacity = FBCSEM_BINARY_MAX_STRINGS + 1
				redim preserve reader.strings(0 to capacity - 1)
				reader.string_capacity = capacity
			end if
			reader.string_count += 1
			reader.strings(reader.string_count) = value
			reader.string_bytes += payload
		end if
	case 6
		if payload <> 0 orelse prefix then return 0
		if FbcSemReadValue(reader, position, ending, value, loading, -1, grouped) = 0 then return 0
		if len(value) = 0 orelse right(value, 1) <> ":" then return 0
		if FbcSemGetVarint(reader.bytes_text, position, ending, payload) = 0 orelse _
		   payload > FBCSEM_BINARY_MAX_INTEGER then return 0
		value += ltrim(str(payload))
	case 7
		if prefix orelse grouped orelse payload < 2 orelse payload > FBCSEM_BINARY_MAX_TUPLE_ITEMS orelse _
		   (reader.capabilities and 32) = 0 then return 0
		for index as integer = 1 to payload
			dim as string part
			if FbcSemReadValue(reader, position, ending, part, loading, 0, -1) = 0 then return 0
			if len(value) > FBCSEM_BINARY_MAX_RECORD - len(part) - 3 then return 0
			if index > 1 then value += "%09"
			value += part
		next
	case else: return 0
	end select
	'' Binary string payloads retain schema-27 wire spelling. Raw controls or
	'' high bytes could forge field_index boundaries when a legacy API requests text.
	if kind = 4 orelse kind = 5 then
		for index as integer = 0 to len(value) - 1
			if value[index] < 32 orelse value[index] > 126 then return 0
		next
	end if
	return -1
end function

'' Both sequential APIs share framing and dictionary decoding. The field API
'' keeps schema-27 wire escapes intact, just like FbcSemNext, but does not join
'' values into TSV only for the consumer to split them again. Semantic field
'' validation and percent decoding remain the consumer's responsibility.
private function FbcSemBeginBinaryRecord(byref reader as FbcSemReader, _
	byref ending as long, byref count as integer) as integer
	reader.row_start = 0
	reader.row_length = 0
	if reader.position = len(reader.bytes_text) then return 0
	reader.failure = "invalid-binary-record"
	if reader.position < 0 orelse len(reader.bytes_text) - reader.position < 4 then return 0
	dim as ulong bytes
	for index as integer = 0 to 3
		bytes or= culng(reader.bytes_text[reader.position + index]) shl (index * 8)
	next
	reader.position += 4
	if bytes < 2 orelse bytes > FBCSEM_BINARY_MAX_RECORD orelse _
	   bytes > culng(len(reader.bytes_text) - reader.position) then return 0
	ending = reader.position + bytes
	dim as integer identity = reader.bytes_text[reader.position]
	count = reader.bytes_text[reader.position + 1]
	if len(reader.tags(identity)) = 0 orelse count + 1 <> reader.arities(identity) then return 0
	reader.row_start = reader.position
	reader.row_length = bytes
	reader.position += 2
	return -1
end function

private function FbcSemNext(byref reader as FbcSemReader, byref line_text as string) as integer
	line_text = ""
	reader.row_start = 0
	reader.row_length = 0
	if reader.binary = 0 then
		if reader.position >= len(reader.bytes_text) then return 0
		dim as integer ending = instr(reader.position + 1, reader.bytes_text, chr(10))
		if ending = 0 then reader.failure = "unterminated-text-record": return 0
		line_text = mid(reader.bytes_text, reader.position + 1, ending - reader.position - 1)
		if right(line_text, 1) = chr(13) then line_text = left(line_text, len(line_text) - 1)
		reader.position = ending
		return -1
	end if
	if reader.first then
		reader.first = 0
		line_text = reader.header
		return -1
	end if
	dim as long ending
	dim as integer count
	if FbcSemBeginBinaryRecord(reader, ending, count) = 0 then return 0
	line_text = reader.tags(reader.bytes_text[reader.row_start])
	for index as integer = 1 to count
		dim as string value
		if FbcSemReadValue(reader, reader.position, ending, value, -1) = 0 then return 0
		if len(line_text) > FBCSEM_BINARY_MAX_RECORD - len(value) - 1 then return 0
		line_text += chr(9) + value
	next
	if reader.position <> ending then return 0
	reader.failure = ""
	return -1
end function

'' Return the number of encoded fields, including the tag, or zero for EOF or
'' failure. This binary-only API requires a zero-based caller-owned array.
'' Strings are copied into that array; no field borrows reader storage. A
'' failure can leave partial fields, which must never be published as a row.
private function FbcSemNextFields(byref reader as FbcSemReader, fields() as string) as integer
	reader.row_start = 0
	reader.row_length = 0
	if reader.binary = 0 orelse lbound(fields) <> 0 orelse ubound(fields) < 2 then
		reader.failure = "binary-field-reader-requires-zero-based-array"
		return 0
	end if
	if reader.first then
		reader.first = 0
		for index as integer = 0 to 2
			fields(index) = FbcSemPart(reader.header, index)
		next
		return 3
	end if
	dim as long ending
	dim as integer count
	if FbcSemBeginBinaryRecord(reader, ending, count) = 0 then return 0
	if count > ubound(fields) then return 0
	fields(0) = reader.tags(reader.bytes_text[reader.row_start])
	dim as integer wire_length = len(fields(0))
	for index as integer = 1 to count
		if FbcSemReadValue(reader, reader.position, ending, fields(index), -1) = 0 then return 0
		if wire_length > FBCSEM_BINARY_MAX_RECORD - len(fields(index)) - 1 then return 0
		wire_length += len(fields(index)) + 1
	next
	if reader.position <> ending then return 0
	reader.failure = ""
	return count + 1
end function

private function FbcSemField(byref reader as FbcSemReader, byval start as long, _
	byval bytes as long, byval field_index as integer, byref value as string) as integer
	if start < 0 orelse bytes < 2 orelse bytes > len(reader.bytes_text) - start then return 0
	dim as integer count = reader.bytes_text[start + 1]
	if field_index < 0 orelse field_index > count then return 0
	if field_index = 0 then value = reader.tags(reader.bytes_text[start]): return -1
	dim as long position = start + 2
	for index as integer = 1 to field_index
		if FbcSemReadValue(reader, position, start + bytes, value, 0) = 0 then return 0
	next
	return -1
end function

private function FbcSemLine(byref reader as FbcSemReader, byval start as long, _
	byval bytes as long) as string
	if bytes = 0 then return reader.header
	if start < 0 orelse bytes < 2 orelse bytes > len(reader.bytes_text) - start then return ""
	dim as string result = reader.tags(reader.bytes_text[start])
	dim as long position = start + 2
	for index as integer = 1 to reader.bytes_text[start + 1]
		dim as string value
		if FbcSemReadValue(reader, position, start + bytes, value, 0) = 0 then return ""
		result += chr(9) + value
	next
	return result
end function

#endif
'' end of fbc-semantic-binary.bi

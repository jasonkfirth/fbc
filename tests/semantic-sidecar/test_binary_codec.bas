'' Project: FreeBASIC semantic transport tests
'' File: test_binary_codec.bas
'' Purpose: Exercise the shared codec independently of semantic graph validation.
'' Responsibilities: Exact values, transaction rollback, limits and reader reuse.
'' This file intentionally does NOT infer compiler facts or install tools.

#lang "fb"
#include once "fbc-semantic-binary.bi"

sub requireCodec(byval accepted as integer, byref operation as const string)
	if accepted then exit sub
	print "Semantic binary codec failed: "; operation
	end 1
end sub

dim as string modelPath = command(1)
requireCodec(len(modelPath) > 0, "output path")
dim as FbcSemDictionary dictionary
dim as FbcSemReader reader
dim as FbcSemReader fieldReader
dim as string fields(0 to 18)
dim as string values(0 to 19) = { _
	"", "0", "-1", "1", "01", "-0", "+1", _
	"2305843009213693951", "-2305843009213693951", _
	"2305843009213693952", "18446744073709551615", _
	"prefix:0", "prefix:12345", "prefix:01", _
	"%00%09%25%FF", "first%09second%09", "%09", _
	"prefix:2%09-17%09", "plain text", "plain text" }
dim as string lines(0 to 20), encoded, producer = "codec-test", exportMode = "full"
encoded = FbcSemHeader(producer, exportMode, 0)
for index as integer = 0 to ubound(values)
	lines(index) = "K" + chr(9) + "module" + chr(9) + "1" + chr(9) + _
		"codec:" + ltrim(str(index)) + chr(9) + values(index)
	encoded += FbcSemEncodeRecord(dictionary, lines(index))
next

'' Discarded transactions must not leave dictionary references to bytes which
'' were never published. This is also the adjacent-expression replacement rule.
dim as long checkpoint = dictionary.count
dim as string discarded = "K" + chr(9) + "module" + chr(9) + "1" + chr(9) + _
	"discarded:key" + chr(9) + "discarded-value"
dim as string ignored = FbcSemEncodeRecord(dictionary, discarded)
FbcSemRollback dictionary, checkpoint
lines(20) = discarded
encoded += FbcSemEncodeRecord(dictionary, discarded)

dim as integer fileNumber = freefile
requireCodec(open(modelPath for binary access write as #fileNumber) = 0, "create model")
requireCodec(put(#fileNumber, , encoded) = 0, "write model")
requireCodec(close(fileNumber) = 0, "close model")
requireCodec(FbcSemLoad(reader, modelPath), "load model")
requireCodec(FbcSemLoad(fieldReader, modelPath), "load field reader")
dim as string row
requireCodec(FbcSemNext(reader, row), "logical header")
requireCodec(row = "FBCSEM" + chr(9) + "27" + chr(9) + producer, "header value")
requireCodec(FbcSemNextFields(fieldReader, fields()) = 3, "logical field header")
requireCodec(fields(0) = "FBCSEM" andalso fields(1) = "27" andalso fields(2) = producer, "header fields")
for index as integer = 0 to ubound(lines)
	requireCodec(FbcSemNext(reader, row), "read record")
	requireCodec(row = lines(index), "exact value")
	requireCodec(FbcSemNextFields(fieldReader, fields()) = 5, "field record arity")
	requireCodec(fields(0) = "K" andalso fields(1) = "module" andalso fields(2) = "1", "field identities")
	if index <= ubound(values) then
		requireCodec(fields(3) = "codec:" + ltrim(str(index)) andalso fields(4) = values(index), "exact field value")
	else
		requireCodec(fields(3) = "discarded:key" andalso fields(4) = "discarded-value", "field dictionary rollback")
	end if
	requireCodec(fieldReader.row_start = reader.row_start andalso _
		fieldReader.row_length = reader.row_length, "identical indexed frame bounds")
	for fieldIndex as integer = 0 to 4
		dim as string fieldValue
		requireCodec(FbcSemField(reader, reader.row_start, reader.row_length, fieldIndex, fieldValue), "indexed read")
		requireCodec(fieldValue = FbcSemPart(row, fieldIndex), "indexed value")
	next
next
requireCodec(FbcSemNext(reader, row) = 0 andalso len(reader.failure) = 0, "end of records")
requireCodec(FbcSemNextFields(fieldReader, fields()) = 0 andalso len(fieldReader.failure) = 0, "end of field records")

'' The API must refuse incompatible caller storage before indexing a field.
'' Runtime array checks remain enabled for these negative controls.
dim as string smallFields(0 to 3), shiftedFields(1 to 19)
requireCodec(FbcSemLoad(fieldReader, modelPath), "reload bounded field reader")
requireCodec(FbcSemNextFields(fieldReader, smallFields()) = 3, "small array header")
requireCodec(FbcSemNextFields(fieldReader, smallFields()) = 0 andalso len(fieldReader.failure) > 0, "small array refusal")
requireCodec(FbcSemLoad(fieldReader, modelPath), "reload shifted field reader")
requireCodec(FbcSemNextFields(fieldReader, shiftedFields()) = 0 andalso len(fieldReader.failure) > 0, "array origin refusal")

'' Once either dictionary ceiling is reached, new strings remain literal.
'' Existing bucket links still refer only to real, allocated entries.
dim as long originalCount = dictionary.count, originalBytes = dictionary.bytes
dim as string uniqueValue = "literal beyond dictionary limit"
dictionary.count = FBCSEM_BINARY_MAX_STRINGS
dim as string literalValue = FbcSemEncodeString(dictionary, uniqueValue)
requireCodec((literalValue[0] and 7) = 5, "dictionary entry limit")
dictionary.count = originalCount
dictionary.bytes = FBCSEM_BINARY_MAX_STRING_BYTES
literalValue = FbcSemEncodeString(dictionary, uniqueValue)
requireCodec((literalValue[0] and 7) = 5, "dictionary byte limit")
dictionary.bytes = originalBytes
requireCodec(FbcSemLoad(reader, modelPath + ".missing") = 0, "missing input")
requireCodec(len(reader.bytes_text) = 0 andalso len(reader.header) = 0 andalso _
	reader.string_count = 0 andalso reader.binary = 0, "failed reload clears ownership")
print "SEMANTIC_BINARY_CODEC_PASS"

'' end of test_binary_codec.bas

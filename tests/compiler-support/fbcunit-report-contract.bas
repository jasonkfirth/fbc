'' Project: FreeBASIC compiler unit testing regressions
'' File: fbcunit-report-contract.bas
'' Purpose: Check real report text, status and stream lifecycle.
'' Responsibilities: Include one implementation and exercise its private helpers.
'' This file intentionally does NOT replace report code or inspect FILE storage.

#ifndef REPORT_IMPLEMENTATION
#define REPORT_IMPLEMENTATION "../fbcunit/src/fbcunit_report.bas"
#define REPORT_REPAIRED
#endif
#include once REPORT_IMPLEMENTATION

dim as string first_path = command(1)
dim as string next_path = command(2)
dim as string xml_path = command(3)
if len(first_path) = 0 then end 1
if report_open(first_path) = false then end 2

dim as string lines(0 to 2) = { !"alpha\n", !"100% beta\n", !"gamma\n" }
for i as integer = 0 to 2
	indent = i * 4
	dim wrote as boolean = report_write(lines(i))
#ifdef REPORT_REPAIRED
	if wrote = false then end 3
#else
	'' The original helper writes normally but never sets its Boolean result.
	if wrote <> false then end 4
#endif
next

#ifdef REPORT_REPAIRED
	if len(next_path) = 0 orelse len(xml_path) = 0 then end 5
	'' Opening another report closes the first and resets indentation.
	indent = 12
	if report_open(next_path) = false then end 6
	if indent <> 0 then end 7
	if report_write("") = false then end 8
	if report_write(!"fresh\n") = false then end 9
	report_indent()
	if report_write(!"indented\n") = false then end 10
	report_outdent()
	if report_close() = false then end 11
	if f <> NULL then end 12
	if report_close() = false then end 13
	if report_write("closed") <> false then end 14

	if fbcu.report_init_file(xml_path) = false then end 15
	fbcu.report_exit_file()
	if f <> NULL then end 16
#else
	'' Close the original stream once. Repeated-close behavior is tested only
	'' after the repair, because the old helper keeps its closed handle.
	if report_close() = false then end 17
#endif

'' end of fbcunit-report-contract.bas

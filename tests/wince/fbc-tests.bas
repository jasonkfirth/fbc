''
'' FreeBASIC Windows CE fbcunit entry point
'' -----------------------------------------
''
'' File: tests/wince/fbc-tests.bas
''
'' Purpose:
''
''     Run one aggregate fbcunit executable inside the Windows CE guest.
''
'' Responsibilities:
''
''     - establish CERF shared storage as the logical test directory
''     - parse the standard fbcunit command-line options
''     - run registered suites and write the requested XML report
''     - return failure when tests or report creation fail
''
'' This file intentionally does NOT contain:
''
''     - test selection or compilation
''     - emulator startup and shutdown
''     - host-side result interpretation
''     - retry or timeout policy
''

#include once "fbcunit.bi"

'' Windows CE has no process current-directory API.  The target runtime
'' emulates CHDIR, while CERF exposes all staged tests under this mount.
if chdir( "\Storage Card" ) <> 0 then end 2

'' -------------------------------------------------------------------------
'' Command-line options                                                      
'' -------------------------------------------------------------------------

dim opt_help as boolean = false
dim opt_verbose as boolean = false
dim opt_show_summary as boolean = true
dim opt_brief_summary as boolean = false
dim opt_hide_cases as boolean = false
dim opt_show_console as boolean = false
dim opt_xml_report as boolean = false
dim opt_xml_filename as string = ""
dim opt_no_error as boolean = false

dim i as integer = 1

while command(i) > ""

	select case lcase(command(i))
	case "-h", "-help", "--help"
		opt_help = true

	case "-v", "--verbose"
		opt_verbose = true

	case "--no-error"
		opt_no_error = true

	case "--xml"
		i += 1
		opt_xml_report = true
		opt_xml_filename = command(i)

		if( opt_xml_filename = "" ) then
			print "expected filename after '" & command(i-1) & "'"
			end 1
		end if

	case "--no-summary"
		opt_show_summary = false

	case "--brief-summary"
		opt_brief_summary = true

	case "--hide-cases"
		opt_hide_cases = true

	case "--show-console"
		opt_show_console = true

	case else
		print "Unrecognized option '" & command(i) & "'"
		end 1

	end select

	i += 1

wend

if( opt_help ) then
	print "usage: fbc-tests [options]"
	print
	print "options:"
	print "   -h, -help, --help    show this help information"
	print "   -v, --verbose        be verbose"
	print "   --xml filename       write test results to xml format for filename"
	print "   --no-summary         don't show the summary (default is to show it)"
	print "   --no-error           don't exit with error code even if tests failed"
	print "   --brief-summary      only show failures in the summary"
	print "   --hide-cases         don't show the failed cases"
	print "   --show-console       show console output"
	print

	'' exit with an error code
	end 1
end if

'' -------------------------------------------------------------------------
'' Suite execution and reporting                                             
'' -------------------------------------------------------------------------

dim passed as boolean = false

'' check the fbcunit internal state
'' at this point all the module constructors should have been
'' called and fbcunit should know about all the suites & tests
'' it can run

if( fbcu.check_internal_state() = false ) then
	print "fbc-tests: fbcu.check_internal_state() failed"
	end 1
end if

'' set extra options
fbcu.setBriefSummary( opt_brief_summary )
fbcu.setHideCases( opt_hide_cases )
fbcu.setShowConsole( opt_show_console )

'' run the tests
passed = fbcu.run_tests( opt_show_summary, opt_verbose )


'' write xml report
if( opt_xml_report ) then
	if( fbcu.write_report_xml( opt_xml_filename ) = false ) then
		'' even if the tests passed, but writing the report
		'' failed, end with an exit code
		end 1
	end if
end if


'' Only return exit code zero if every test passed and reporting succeeded,
'' or if the caller explicitly requested suppression through --no-error.
if( passed or opt_no_error ) then
	end 0
end if

'' failed
end 1

'' end of tests/wince/fbc-tests.bas

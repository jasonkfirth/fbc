''
'' "Hello World!" test, in UTF-8 format
''

#ifdef __FB_WIN32__
# define unicode
# include once "windows.bi"
#endif

const LANG = "Chinese"
'' FB-LINTER: DISABLE-NEXT-LINE FBL-SEC-014 REASON: The fullwidth comma is the intended Chinese punctuation in this Unicode example.
	dim helloworld as wstring * 20 => "你好，世界!"

	print """Hello World!"" in "; LANG; ": "; helloworld

#ifdef __FB_WIN32__
	messagebox( 0, helloworld, """Hello World!"" in " & LANG & ":", MB_OK )
#endif

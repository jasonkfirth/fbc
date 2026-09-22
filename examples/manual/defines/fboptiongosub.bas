'' examples/manual/defines/fboptiongosub.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic '__FB_OPTION_GOSUB__'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=KeyPgDdfboptiongosub
'' --------

#if( __FB_OPTION_GOSUB__ <> 0 )
	'' turn off gosub support
	'' This conditional definition lesson deliberately changes the option after inspection.
	'' FB-LINTER: DISABLE-NEXT-LINE FBL-OPT-001
	Option nogosub
#endif

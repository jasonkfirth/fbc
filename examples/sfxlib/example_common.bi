''
'' Shared helper routines for the sfxlib example programs.
''

#if defined( __FB_WIN32__ ) or defined( __FB_DOS__ )
	const SFX_EXAMPLE_PATHSEP = "\"
#else
	const SFX_EXAMPLE_PATHSEP = "/"
#endif

function SfxExampleMedia( byref leaf as string ) as string
	return exepath() & SFX_EXAMPLE_PATHSEP & "media" & SFX_EXAMPLE_PATHSEP & leaf
end function

sub SfxExampleBanner( byref title as string )
	print
	print "========================================"
	print title
	print "========================================"
end sub

sub SfxExampleWait( byval milliseconds as integer )
	if( milliseconds > 0 ) then
		sleep milliseconds, 1
	end if
end sub

sub SfxExampleResult( byref label as string, byval value as long )
	print label; value
end sub

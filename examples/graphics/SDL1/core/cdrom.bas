'' Project: FreeBASIC SDL examples
'' File: cdrom.bas
'' Purpose:
''     List CD track lengths when a drive and disc are present.
'' Responsibilities:
''     Enumerate drives, validate status, and release the drive.
'' This file intentionally does NOT contain:
''     CD playback or simulated CD hardware.
''
' SDL_cdrom example adapted to freeBasic from:
' http://www.libsdl.org/cgi/docwiki.cgi/SDL_20CDROM_20Listing_20Track
'
' Lists all the tracks available on a CD.

#include  "SDL\SDL.bi"

	dim cdrom as SDL_CD ptr
	dim i as integer
	dim m as long, s as long, f as long

	' initiate SDL with cdrom support
	if (SDL_Init(SDL_INIT_CDROM) < 0) then
		print "Unable to initiate SDL: "; *SDL_GetError()
		end 1
	end if

	' SDL1 can run on systems with no CD drive. SDL12-compat also reports
	' no drives unless its optional virtual CD support is configured.
	dim drives as long = SDL_CDNumDrives()
	if drives < 0 then
		print "Unable to enumerate CD-ROM drives: "; *SDL_GetError()
		SDL_Quit
		end 1
	elseif drives = 0 then
		print "No CD-ROM drives available"
		SDL_Quit
		end 0
	end if

	' open up the default cdrom drive
	cdrom = SDL_CDOpen(0)
	if cdrom = NULL then
		print "Unable to open CD-ROM drive: "; *SDL_GetError()
		SDL_Quit
		end 1
	end if

	' obtain the status of the cdrom
	dim status as CDstatus = SDL_CDStatus(cdrom)
	if status = CD_ERROR then
		print "Unable to read CD-ROM status: "; *SDL_GetError()
		SDL_CDClose(cdrom)
		SDL_Quit
		end 1
	elseif status = CD_TRAYEMPTY then
		print "No disc in CD-ROM drive"
		SDL_CDClose(cdrom)
		SDL_Quit
		end 0
	end if
	if cdrom->numtracks < 0 or cdrom->numtracks > SDL_MAX_TRACKS then
		print "Invalid CD-ROM track count"
		SDL_CDClose(cdrom)
		SDL_Quit
		end 1
	end if

	' print the number of available tracks on the cd
	print "Drive tracks:"; cdrom->numtracks

	' we now print info about each track
	for i = 0 to cdrom->numtracks - 1
		FRAMES_TO_MSF(cdrom->track(i).length, @m, @s, @f)
		if (f > 0) then s = s + 1
		print chr(9); "Track (index"; i; ") "; cdrom->track(i).id; ":"; m; ":"; string(2 - len(str(s)), "0"); trim(str(s))
	next

	SDL_CDClose(cdrom)
	SDL_Quit

	' end when the user presses a key
	sleep

'' End of cdrom.bas

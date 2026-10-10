'' Project: FreeBASIC SDL examples
'' File: music_test1.bas
'' Purpose:
''     Play and stop an Ogg song using SDL1_mixer.
'' Responsibilities:
''     Own music on the main thread and transfer callback notifications.
'' This file intentionally does NOT contain:
''     Music cleanup on the audio thread.
''
' SDL_music example adapted to freeBasic from:
' http://www.kekkai.org/roger/sdl/mixer/
'
' music.ogg is a freely distributed song by "Honest Bob and the
' Factory-to-Dealer Incentives" called "Another person you had sex with" for
' people who wanted an ogg file to test their programs with.
'
' Press any to start/stop the music, ESC to exit.

#include once "SDL\SDL.bi"
#include once "SDL\SDL_mixer.bi"

' Mix_Music actually holds the music information.
'' Only the main thread loads and frees the song. The audio callback posts
'' a semaphore, which remains alive until Mix_CloseAudio joins that thread.
'' FB-LINTER: DISABLE-NEXT-LINE FBL301
dim shared song as Mix_Music ptr = NULL
dim shared songFinished as SDL_sem ptr

declare function handlekey(byval key as SDL_KeyboardEvent ptr) as integer
declare sub musicDone cdecl ()
declare sub stopMusic()

sub main()
	dim video as SDL_Surface ptr
	dim event as SDL_Event
	dim done as integer
	dim exitStatus as integer
	done = 0

	dim audio_rate as long
	dim audio_format as Uint16
	dim audio_channels as long
	dim audio_buffers as long

	' We're going to be requesting certain things from our audio
	' device, so we set them up beforehand
	audio_rate = 44100
	audio_format = AUDIO_S16
	audio_channels = 2
	audio_buffers = 4096

	if SDL_Init(SDL_INIT_VIDEO or SDL_INIT_AUDIO) <> 0 then
		print "SDL_Init: "; *SDL_GetError()
		end 1
	end if

	' This is where we open up our audio device.  Mix_OpenAudio takes
	' as its parameters the audio format we'd /like/ to have.
	if (Mix_OpenAudio(audio_rate, audio_format, audio_channels, audio_buffers)) then
		print "Unable to open audio!"
		SDL_Quit
		end 1
	end if

	' If we actually care about what we got, we can ask here.  In this
	' program we don't, but I'm showing the function call here anyway
	' in case we'd want to know later.
	if Mix_QuerySpec(@audio_rate, @audio_format, @audio_channels) = 0 then
		print "Unable to query audio format!"
		Mix_CloseAudio
		SDL_Quit
		end 1
	end if

	' We're going to be using a window onscreen to register keypresses
	' in.  We don't really care what it has in it, since we're not
	' doing graphics, so we'll just throw something up there.

	video = SDL_SetVideoMode(320, 240, 0, 0)
	if video = NULL then
		print "Unable to create video surface!"
		Mix_CloseAudio
		SDL_Quit
		end 1
	end if
	songFinished = SDL_CreateSemaphore(0)
	if songFinished = NULL then
		print "Unable to create music completion semaphore!"
		Mix_CloseAudio
		SDL_Quit
		end 1
	end if
	Mix_HookMusicFinished(@musicDone)

	if( environ( "FB_EXAMPLE_AUTOPLAY" ) <> "" ) then
		event.key.keysym.sym = SDLK_m
		if handleKey(@event.key) = 0 then
			exitStatus = 1
		else
			SDL_Delay(1500)
		end if
		done = 1
	end if

	do while (done = 0)
		do while (SDL_PollEvent(@event))
			select case (event.type)
			case SDL_QUIT_
				done = 1
			case SDL_KEYDOWN
				if( event.key.keysym.sym = SDLK_ESCAPE ) then
					done = -1
				else
					if handleKey(@event.key) = 0 then
						exitStatus = 1
						done = 1
					end if
				end if
			end select
		loop
		if SDL_SemTryWait(songFinished) = 0 then
			print "Music finished"
			stopMusic()
		end if

		' So we don't hog the CPU
		SDL_Delay(50)
	loop

	' This is the cleaning up part
	Mix_HookMusicFinished(NULL)
	stopMusic()
	Mix_CloseAudio
	SDL_DestroySemaphore(songFinished)
	SDL_Quit
	if exitStatus then end 1
end sub

function handleKey (byval key as SDL_KeyboardEvent ptr) as integer
    ' Any non-ESC key toggles the music on and off.
    ' When it's on, it'll be loaded and 'song' will point to
    ' something valid.  If it's off, song will be NULL.

    if (song = NULL) then
		' Actually loads up the music
        song = Mix_LoadMUS("data/music.ogg")
        if song = NULL then
            print "Unable to load music: "; *Mix_GetError()
            return 0
        end if

        ' This begins playing the music - the first argument is a
        ' pointer to Mix_Music structure, and the second is how many
        ' times you want it to loop (use -1 for infinite, and 0 to
        ' have it just play once)
        if Mix_PlayMusic(song, 0) <> 0 then
            print "Unable to play music: "; *Mix_GetError()
            stopMusic()
            return 0
        end if
        print "Playing music.ogg"

        ' We want to know when our music has stopped playing so we
        ' can free it up and set 'song' back to NULL.  SDL_Mixer
        ' provides us with a callback routine we can use to do
        ' exactly that
        ' The hook was registered after creating its semaphore in main().
    else
        stopMusic()
    end if
    return 1
end function

' This is the function that we told SDL_Mixer to call when the music
' was finished. SDL_mixer holds its audio lock here, so calling
' Mix_HaltMusic or Mix_FreeMusic in this callback can deadlock.
' Leave those operations to the main thread.
sub musicDone cdecl ()
	SDL_SemPost(songFinished)
end sub

sub stopMusic()
	if song <> NULL then
		Mix_HaltMusic
		Mix_FreeMusic(song)
		song = NULL
		print "Stopped music.ogg"
	end if
	' A manual halt can post a completion signal too. Discard it before
	' another song starts, so an old signal cannot stop the new song.
	while SDL_SemTryWait(songFinished) = 0
	wend
end sub

main()

'' End of music_test1.bas

' TEST_MODE : COMPILE_ONLY_OK

#define FB_NO_GFXLIB
#define FB_NO_SFXLIB

#define bload 1

scope
	dim screenres as integer = 24
	dim flip as integer = 25
end scope

scope
	dim screen as integer = 1
	dim pset as integer = 2
	dim preset as integer = 3
	dim point as integer = 4
	dim circle as integer = 5
	dim window as integer = 6
	dim palette as integer = 7
	dim paint as integer = 8
	dim draw as integer = 9
	dim imagecreate as integer = 10

	screen = screen + pset + preset + point + circle
	window = window + palette + paint + draw + imagecreate
end scope

scope
	dim sound as integer = 8
	dim noise as integer = 9
	dim play as integer = 10
	dim tempo as integer = 11
	dim channel as integer = 12
	dim octave as integer = 13
	dim voice as integer = 14
	dim vol as integer = 15
	dim volume as integer = 16
	dim balance as integer = 17
	dim pan as integer = 18
	dim note as integer = 19
	dim wave as integer = 20
	dim envelope as integer = 21
	dim instrument as integer = 22
	dim tone as integer = 23

	dim music as integer = 1
	dim sfx as integer = 2
	dim audio as integer = 3
	dim stream as integer = 4
	dim midi as integer = 5
	dim device as integer = 6
	dim capture as integer = 7

	music = music + sfx + audio + stream
	midi = midi + device + capture
end scope

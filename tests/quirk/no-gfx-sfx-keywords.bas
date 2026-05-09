' TEST_MODE : COMPILE_ONLY_OK

#define FB_NO_GFXLIB
#define FB_NO_SFXLIB

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

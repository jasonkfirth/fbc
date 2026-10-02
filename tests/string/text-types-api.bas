'' Project: FreeBASIC text argument coverage
'' File: text-types-api.bas
'' Purpose: Compile every built-in text consumer with all three string types.
'' Responsibilities: Check compiler routing without invoking destructive APIs.
'' This file intentionally does NOT execute shell, graphics, or system changes.

' TEST_MODE : COMPILE_ONLY_OK
#include once "string.bi"
#include once "datetime.bi"
#include once "file.bi"
#include once "dir.bi"
#include once "fbgfx.bi"
#include once "fbc-int/string.bi"

#if __FB_LANG__ = "fb"
	#define textSize fb.DrawStringSize
	#define paintBytes fb.PaintPattern
#else
	#define textSize DrawStringSize
	#define paintBytes PaintPattern
#endif

'' All OPEN variants share the same text argument conversion contract.
#macro check_file_arguments(value, h)
	print #(h), value
	write #(h), value
	print #(h), using value; value
	open value for input encoding value as #(h)
	open value, #(h), value
	open cons for output encoding value as #(h)
	open err for output encoding value as #(h)
	open pipe value for input as #(h)
	open scrn for output encoding value as #(h)
	open lpt value for output as #(h)
	open com value for binary as #(h)
	open tcp value for binary as #(h)
	open tcp server value for binary as #(h)
#endmacro

private sub check_s( byref value as string )
	print value
	write value
	print using "&"; value
#if __FB_LANG__ <> "fb"
	lprint value
	lprint using "&"; value
#endif
	line input value
	input value
	read value
	dim as integer h
	check_file_arguments(value, h)
	open value for input as #h
	name value as value
	kill value
	mkdir value
	rmdir value
	chdir value
	dim as string answer
	answer = environ(value)
	h = setenviron(value)
	answer = dir(value)
	h = shell(value)
	h = exec(value, value)
	run value, value
	chain value
	h = settime(value)
	h = setdate(value)
	dim as any ptr moduleHandle = dylibload(value)
	dim as any ptr entry = dylibsymbol(moduleHandle,value)
	h = len(value)
	h = asc(value)
	answer = left(value,1)
	answer = right(value,1)
	answer = mid(value,1,1)
	mid(value,1,1) = value
	answer = string(2,value)
	h = instr(value,value)
	h = instr(value,any value)
	h = instrrev(value,value)
	h = instrrev(value,any value)
	answer = trim(value)
	answer = ltrim(value)
	answer = rtrim(value)
	answer = trim(value,any value)
	answer = ltrim(value,value)
	answer = rtrim(value,value)
	answer = ucase(value)
	answer = lcase(value)
	lset value = value
	rset value = value
	h = cvshort(value)
	h = cvl(value)
	dim as double d = cvd(value)
	d = cvs(value)
	h = cvlongint(value)
	d = val(value)
	h = valint(value)
	h = valuint(value)
	h = vallng(value)
	h = valulng(value)
	h = cbool(value)
	h = cint(value)
	d = cdbl(value)
	h = StrComp(value,value)
	answer = Replace(value,value,value)
	answer = StrReverse(value)
	answer = Format(1,value)
	d = DateValue(value)
	d = TimeValue(value)
	h = IsDate(value)
	d = DateAdd(value,1,1)
	d = DatePart(value,1)
	d = DateDiff(value,1,2)
	h = FileExists(value)
	h = GetAttr(value)
	h = SetAttr(value,0)
	h = FileLen(value)
	d = FileDateTime(value)
	h = FileCopy(value,value)
	dim as long textWidth, textHeight
	h = textSize(value,textWidth,textHeight)
	windowtitle value
	screeninfo ,,,,,,value
	screencontrol 1,value
	draw value
	draw string (0,0),value
	entry = screenglproc(value)
	h = paintBytes(0, 0, 0, value)
	h = bload(value)
	h = bsave(value,cast(any ptr,0),0)
	width value,80
	FBC.LeftSelf(value, 1)
	play value
	play 1, value
	play value, value
	play value, value, value
	note value, 4, 0.01
	note 1, value, 4, 0.01
	music load value
	music play value
	music loop value
	sfx load 1, value
	midi play value
	h = capture save(value)
end sub

private sub check_w( byref value as wstring )
	print value
	write value
	print using "&"; value
#if __FB_LANG__ <> "fb"
	lprint value
	lprint using "&"; value
#endif
	line input "", value, 32
	input value
	read value
	dim as integer h
	check_file_arguments(value, h)
	open value for input as #h
	name value as value
	kill value
	mkdir value
	rmdir value
	chdir value
	dim as string answer
	answer = environ(value)
	h = setenviron(value)
	answer = dir(value)
	h = shell(value)
	h = exec(value, value)
	run value, value
	chain value
	h = settime(value)
	h = setdate(value)
	dim as any ptr moduleHandle = dylibload(value)
	dim as any ptr entry = dylibsymbol(moduleHandle,value)
	h = len(value)
	h = asc(value)
	answer = left(value,1)
	answer = right(value,1)
	answer = mid(value,1,1)
	mid(value,1,1) = value
	answer = string(2,value)
	h = instr(value,value)
	h = instr(value,any value)
	h = instrrev(value,value)
	h = instrrev(value,any value)
	answer = trim(value)
	answer = ltrim(value)
	answer = rtrim(value)
	answer = trim(value,any value)
	answer = ltrim(value,value)
	answer = rtrim(value,value)
	answer = ucase(value)
	answer = lcase(value)
	lset value = value
	rset value = value
	h = cvshort(value)
	h = cvl(value)
	dim as double d = cvd(value)
	d = cvs(value)
	h = cvlongint(value)
	d = val(value)
	h = valint(value)
	h = valuint(value)
	h = vallng(value)
	h = valulng(value)
	h = cbool(value)
	h = cint(value)
	d = cdbl(value)
	h = StrComp(value,value)
	answer = Replace(value,value,value)
	answer = StrReverse(value)
	answer = Format(1,value)
	d = DateValue(value)
	d = TimeValue(value)
	h = IsDate(value)
	d = DateAdd(value,1,1)
	d = DatePart(value,1)
	d = DateDiff(value,1,2)
	h = FileExists(value)
	h = GetAttr(value)
	h = SetAttr(value,0)
	h = FileLen(value)
	d = FileDateTime(value)
	h = FileCopy(value,value)
	dim as long textWidth, textHeight
	h = textSize(value,textWidth,textHeight)
	windowtitle value
	screeninfo ,,,,,,value
	screencontrol 1,value
	draw value
	draw string (0,0),value
	entry = screenglproc(value)
	h = paintBytes(0, 0, 0, value)
	h = bload(value)
	h = bsave(value,cast(any ptr,0),0)
	width value,80
	FBC.LeftSelf(value, 1)
	play value
	play 1, value
	play value, value
	play value, value, value
	note value, 4, 0.01
	note 1, value, 4, 0.01
	music load value
	music play value
	music loop value
	sfx load 1, value
	midi play value
	h = capture save(value)
end sub

private sub check_u( byref value as ustring )
	print value
	write value
	print using "&"; value
#if __FB_LANG__ <> "fb"
	lprint value
	lprint using "&"; value
#endif
	line input value
	input value
	read value
	dim as integer h
	check_file_arguments(value, h)
	open value for input as #h
	name value as value
	kill value
	mkdir value
	rmdir value
	chdir value
	dim as string answer
	answer = environ(value)
	h = setenviron(value)
	answer = dir(value)
	h = shell(value)
	h = exec(value, value)
	run value, value
	chain value
	h = settime(value)
	h = setdate(value)
	dim as any ptr moduleHandle = dylibload(value)
	dim as any ptr entry = dylibsymbol(moduleHandle,value)
	h = len(value)
	h = asc(value)
	answer = left(value,1)
	answer = right(value,1)
	answer = mid(value,1,1)
	mid(value,1,1) = value
	answer = string(2,value)
	h = instr(value,value)
	h = instr(value,any value)
	h = instrrev(value,value)
	h = instrrev(value,any value)
	answer = trim(value)
	answer = ltrim(value)
	answer = rtrim(value)
	answer = trim(value,any value)
	answer = ltrim(value,value)
	answer = rtrim(value,value)
	answer = ucase(value)
	answer = lcase(value)
	lset value = value
	rset value = value
	h = cvshort(value)
	h = cvl(value)
	dim as double d = cvd(value)
	d = cvs(value)
	h = cvlongint(value)
	d = val(value)
	h = valint(value)
	h = valuint(value)
	h = vallng(value)
	h = valulng(value)
	h = cbool(value)
	h = cint(value)
	d = cdbl(value)
	h = StrComp(value,value)
	answer = Replace(value,value,value)
	answer = StrReverse(value)
	answer = Format(1,value)
	d = DateValue(value)
	d = TimeValue(value)
	h = IsDate(value)
	d = DateAdd(value,1,1)
	d = DatePart(value,1)
	d = DateDiff(value,1,2)
	h = FileExists(value)
	h = GetAttr(value)
	h = SetAttr(value,0)
	h = FileLen(value)
	d = FileDateTime(value)
	h = FileCopy(value,value)
	dim as long textWidth, textHeight
	h = textSize(value,textWidth,textHeight)
	windowtitle value
	screeninfo ,,,,,,value
	screencontrol 1,value
	draw value
	draw string (0,0),value
	entry = screenglproc(value)
	h = paintBytes(0, 0, 0, value)
	h = bload(value)
	h = bsave(value,cast(any ptr,0),0)
	width value,80
	FBC.LeftSelf(value, 1)
	play value
	play 1, value
	play value, value
	play value, value, value
	note value, 4, 0.01
	note 1, value, 4, 0.01
	music load value
	music play value
	music loop value
	sfx load 1, value
	midi play value
	h = capture save(value)
end sub

'' Mixed optional signatures must resolve without ambiguity.
private sub mixed_sss( byref s as string, byref w as wstring, byref u as ustring )
	dim as string resultBytes = Replace(s, s, s)
end sub

private sub mixed_ssw( byref s as string, byref w as wstring, byref u as ustring )
	dim as string resultBytes = Replace(s, s, w)
end sub

private sub mixed_ssu( byref s as string, byref w as wstring, byref u as ustring )
	dim as string resultBytes = Replace(s, s, u)
end sub

private sub mixed_sws( byref s as string, byref w as wstring, byref u as ustring )
	dim as string resultBytes = Replace(s, w, s)
end sub

private sub mixed_sww( byref s as string, byref w as wstring, byref u as ustring )
	dim as string resultBytes = Replace(s, w, w)
end sub

private sub mixed_swu( byref s as string, byref w as wstring, byref u as ustring )
	dim as string resultBytes = Replace(s, w, u)
end sub

private sub mixed_sus( byref s as string, byref w as wstring, byref u as ustring )
	dim as string resultBytes = Replace(s, u, s)
end sub

private sub mixed_suw( byref s as string, byref w as wstring, byref u as ustring )
	dim as string resultBytes = Replace(s, u, w)
end sub

private sub mixed_suu( byref s as string, byref w as wstring, byref u as ustring )
	dim as string resultBytes = Replace(s, u, u)
end sub

private sub mixed_wss( byref s as string, byref w as wstring, byref u as ustring )
	dim as string resultBytes = Replace(w, s, s)
end sub

private sub mixed_wsw( byref s as string, byref w as wstring, byref u as ustring )
	dim as string resultBytes = Replace(w, s, w)
end sub

private sub mixed_wsu( byref s as string, byref w as wstring, byref u as ustring )
	dim as string resultBytes = Replace(w, s, u)
end sub

private sub mixed_wws( byref s as string, byref w as wstring, byref u as ustring )
	dim as string resultBytes = Replace(w, w, s)
end sub

private sub mixed_www( byref s as string, byref w as wstring, byref u as ustring )
	dim as string resultBytes = Replace(w, w, w)
end sub

private sub mixed_wwu( byref s as string, byref w as wstring, byref u as ustring )
	dim as string resultBytes = Replace(w, w, u)
end sub

private sub mixed_wus( byref s as string, byref w as wstring, byref u as ustring )
	dim as string resultBytes = Replace(w, u, s)
end sub

private sub mixed_wuw( byref s as string, byref w as wstring, byref u as ustring )
	dim as string resultBytes = Replace(w, u, w)
end sub

private sub mixed_wuu( byref s as string, byref w as wstring, byref u as ustring )
	dim as string resultBytes = Replace(w, u, u)
end sub

private sub mixed_uss( byref s as string, byref w as wstring, byref u as ustring )
	dim as string resultBytes = Replace(u, s, s)
end sub

private sub mixed_usw( byref s as string, byref w as wstring, byref u as ustring )
	dim as string resultBytes = Replace(u, s, w)
end sub

private sub mixed_usu( byref s as string, byref w as wstring, byref u as ustring )
	dim as string resultBytes = Replace(u, s, u)
end sub

private sub mixed_uws( byref s as string, byref w as wstring, byref u as ustring )
	dim as string resultBytes = Replace(u, w, s)
end sub

private sub mixed_uww( byref s as string, byref w as wstring, byref u as ustring )
	dim as string resultBytes = Replace(u, w, w)
end sub

private sub mixed_uwu( byref s as string, byref w as wstring, byref u as ustring )
	dim as string resultBytes = Replace(u, w, u)
end sub

private sub mixed_uus( byref s as string, byref w as wstring, byref u as ustring )
	dim as string resultBytes = Replace(u, u, s)
end sub

private sub mixed_uuw( byref s as string, byref w as wstring, byref u as ustring )
	dim as string resultBytes = Replace(u, u, w)
end sub

private sub mixed_uuu( byref s as string, byref w as wstring, byref u as ustring )
	dim as string resultBytes = Replace(u, u, u)
end sub

'' end of text-types-api.bas

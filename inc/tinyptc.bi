'' Project: FreeBASIC TinyPTC bindings
'' File: tinyptc.bi
'' Purpose: Declare the C framebuffer interface for supported targets.
'' Responsibilities: Library selection and C-compatible parameter types.
'' This file contains no display implementation or framebuffer allocation.
''
'' TinyPTC by Gaffer
'' www.gaffer.org/tinyptc
''

''
'' add the -d PTC_WIN option to fbc's command-line to use the windowed version
''
#if defined(PTC_WIN) and not defined(__FB_DOS__)
# inclib "tinyptc-win"
#else
# inclib "tinyptc"
#endif

'' tinyptc api
'' TinyPTC uses C int dimensions/results and a void pointer to packed 32-bit
'' 0x00RRGGBB pixels. FreeBASIC Integer is wider on 64-bit targets.
declare function ptc_open	cdecl alias "ptc_open"		(byval title as zstring ptr, _
								 byval width as long, _
								 byval height as long ) as long

declare function ptc_update	cdecl alias "ptc_update"	(byval buffer as any ptr) as long

declare sub ptc_close	cdecl alias "ptc_close"		()

#ifdef __FB_WIN32__
# inclib "ddraw"
# inclib "user32"
# inclib "gdi32"
#elseif defined(__FB_LINUX__)
# inclib "X11"
# libpath "/usr/X11R6/lib"
#elseif defined(__FB_DOS__)
'''
#else
# error Platform not supported!
#endif

'' end of tinyptc.bi

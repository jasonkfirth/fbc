''
'' Project: FreeBASIC graphics runtime
'' -----------------------------------
''
'' File: fbgfx3-option.bi
''
'' Purpose:
''
''     Provide the public gfxlib3 selection define for the compiler's -gfx3
''     option before user source is parsed.
''
'' Responsibilities:
''
''     - define __FB_GFXLIB3__ using the documented source-level spelling
''     - preserve an application's existing gfxlib3 selection define
''
'' This file intentionally does NOT contain:
''
''     - gfxlib3 declarations
''     - library selection pragmas
''     - Vulkan, OpenGL, or platform declarations
''

#ifndef __FB_GFXLIB3__
    #define __FB_GFXLIB3__
#endif

'' end of fbgfx3-option.bi

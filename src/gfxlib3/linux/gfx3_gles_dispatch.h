/*
    Project: FreeBASIC gfxlib3
    --------------------------

    File: linux/gfx3_gles_dispatch.h

    Purpose:

        Declare the GLES 3.0 procedures loaded from a Linux GL context.

    Responsibilities:

        - give the common GLES renderer correctly typed function pointers
        - keep Linux GLES calls out of the application's direct link table

    This file intentionally does NOT contain:

        - context creation or symbol lookup
        - Android dynamic-link behavior
        - renderer state or shader declarations
*/

#ifndef __FB_GFX3_LINUX_GLES_DISPATCH_H__
#define __FB_GFX3_LINUX_GLES_DISPATCH_H__

#include <GLES3/gl3.h>

#define FB_GFX3_GLES_DISPATCH_FUNCTIONS(X) \
	X(glActiveTexture) \
	X(glAttachShader) \
	X(glBeginQuery) \
	X(glBindBuffer) \
	X(glBindFramebuffer) \
	X(glBindTexture) \
	X(glBindVertexArray) \
	X(glBlitFramebuffer) \
	X(glBufferData) \
	X(glCheckFramebufferStatus) \
	X(glClientWaitSync) \
	X(glCompileShader) \
	X(glCreateProgram) \
	X(glCreateShader) \
	X(glDeleteBuffers) \
	X(glDeleteFramebuffers) \
	X(glDeleteProgram) \
	X(glDeleteQueries) \
	X(glDeleteShader) \
	X(glDeleteSync) \
	X(glDeleteTextures) \
	X(glDeleteVertexArrays) \
	X(glDisable) \
	X(glDisableVertexAttribArray) \
	X(glDrawArrays) \
	X(glDrawArraysInstanced) \
	X(glEnable) \
	X(glEnableVertexAttribArray) \
	X(glEndQuery) \
	X(glFenceSync) \
	X(glFinish) \
	X(glFlush) \
	X(glFramebufferTexture2D) \
	X(glGenBuffers) \
	X(glGenFramebuffers) \
	X(glGenQueries) \
	X(glGenTextures) \
	X(glGenVertexArrays) \
	X(glGetError) \
	X(glGetIntegerv) \
	X(glGetProgramInfoLog) \
	X(glGetProgramiv) \
	X(glGetQueryObjectuiv) \
	X(glGetShaderInfoLog) \
	X(glGetShaderiv) \
	X(glGetString) \
	X(glGetUniformLocation) \
	X(glLinkProgram) \
	X(glPixelStorei) \
	X(glReadBuffer) \
	X(glReadPixels) \
	X(glScissor) \
	X(glShaderSource) \
	X(glTexParameteri) \
	X(glTexStorage2D) \
	X(glTexSubImage2D) \
	X(glUniform1i) \
	X(glUniform1ui) \
	X(glUniform2f) \
	X(glUniform2i) \
	X(glUniform2ui) \
	X(glUniform4i) \
	X(glUniform4iv) \
	X(glUniformMatrix3fv) \
	X(glUseProgram) \
	X(glVertexAttribDivisor) \
	X(glVertexAttribIPointer) \
	X(glVertexAttribPointer) \
	X(glViewport)

typedef struct FB_GFX3_GLES_DISPATCH {
#define FB_GFX3_GLES_DECLARE_FUNCTION(name) __typeof__(&name) p_##name;
	FB_GFX3_GLES_DISPATCH_FUNCTIONS(FB_GFX3_GLES_DECLARE_FUNCTION)
#undef FB_GFX3_GLES_DECLARE_FUNCTION
} FB_GFX3_GLES_DISPATCH;

#define glActiveTexture (gles_dispatch.p_glActiveTexture)
#define glAttachShader (gles_dispatch.p_glAttachShader)
#define glBeginQuery (gles_dispatch.p_glBeginQuery)
#define glBindBuffer (gles_dispatch.p_glBindBuffer)
#define glBindFramebuffer (gles_dispatch.p_glBindFramebuffer)
#define glBindTexture (gles_dispatch.p_glBindTexture)
#define glBindVertexArray (gles_dispatch.p_glBindVertexArray)
#define glBlitFramebuffer (gles_dispatch.p_glBlitFramebuffer)
#define glBufferData (gles_dispatch.p_glBufferData)
#define glCheckFramebufferStatus (gles_dispatch.p_glCheckFramebufferStatus)
#define glClientWaitSync (gles_dispatch.p_glClientWaitSync)
#define glCompileShader (gles_dispatch.p_glCompileShader)
#define glCreateProgram (gles_dispatch.p_glCreateProgram)
#define glCreateShader (gles_dispatch.p_glCreateShader)
#define glDeleteBuffers (gles_dispatch.p_glDeleteBuffers)
#define glDeleteFramebuffers (gles_dispatch.p_glDeleteFramebuffers)
#define glDeleteProgram (gles_dispatch.p_glDeleteProgram)
#define glDeleteQueries (gles_dispatch.p_glDeleteQueries)
#define glDeleteShader (gles_dispatch.p_glDeleteShader)
#define glDeleteSync (gles_dispatch.p_glDeleteSync)
#define glDeleteTextures (gles_dispatch.p_glDeleteTextures)
#define glDeleteVertexArrays (gles_dispatch.p_glDeleteVertexArrays)
#define glDisable (gles_dispatch.p_glDisable)
#define glDisableVertexAttribArray (gles_dispatch.p_glDisableVertexAttribArray)
#define glDrawArrays (gles_dispatch.p_glDrawArrays)
#define glDrawArraysInstanced (gles_dispatch.p_glDrawArraysInstanced)
#define glEnable (gles_dispatch.p_glEnable)
#define glEnableVertexAttribArray (gles_dispatch.p_glEnableVertexAttribArray)
#define glEndQuery (gles_dispatch.p_glEndQuery)
#define glFenceSync (gles_dispatch.p_glFenceSync)
#define glFinish (gles_dispatch.p_glFinish)
#define glFlush (gles_dispatch.p_glFlush)
#define glFramebufferTexture2D (gles_dispatch.p_glFramebufferTexture2D)
#define glGenBuffers (gles_dispatch.p_glGenBuffers)
#define glGenFramebuffers (gles_dispatch.p_glGenFramebuffers)
#define glGenQueries (gles_dispatch.p_glGenQueries)
#define glGenTextures (gles_dispatch.p_glGenTextures)
#define glGenVertexArrays (gles_dispatch.p_glGenVertexArrays)
#define glGetError (gles_dispatch.p_glGetError)
#define glGetIntegerv (gles_dispatch.p_glGetIntegerv)
#define glGetProgramInfoLog (gles_dispatch.p_glGetProgramInfoLog)
#define glGetProgramiv (gles_dispatch.p_glGetProgramiv)
#define glGetQueryObjectuiv (gles_dispatch.p_glGetQueryObjectuiv)
#define glGetShaderInfoLog (gles_dispatch.p_glGetShaderInfoLog)
#define glGetShaderiv (gles_dispatch.p_glGetShaderiv)
#define glGetString (gles_dispatch.p_glGetString)
#define glGetUniformLocation (gles_dispatch.p_glGetUniformLocation)
#define glLinkProgram (gles_dispatch.p_glLinkProgram)
#define glPixelStorei (gles_dispatch.p_glPixelStorei)
#define glReadBuffer (gles_dispatch.p_glReadBuffer)
#define glReadPixels (gles_dispatch.p_glReadPixels)
#define glScissor (gles_dispatch.p_glScissor)
#define glShaderSource (gles_dispatch.p_glShaderSource)
#define glTexParameteri (gles_dispatch.p_glTexParameteri)
#define glTexStorage2D (gles_dispatch.p_glTexStorage2D)
#define glTexSubImage2D (gles_dispatch.p_glTexSubImage2D)
#define glUniform1i (gles_dispatch.p_glUniform1i)
#define glUniform1ui (gles_dispatch.p_glUniform1ui)
#define glUniform2f (gles_dispatch.p_glUniform2f)
#define glUniform2i (gles_dispatch.p_glUniform2i)
#define glUniform2ui (gles_dispatch.p_glUniform2ui)
#define glUniform4i (gles_dispatch.p_glUniform4i)
#define glUniform4iv (gles_dispatch.p_glUniform4iv)
#define glUniformMatrix3fv (gles_dispatch.p_glUniformMatrix3fv)
#define glUseProgram (gles_dispatch.p_glUseProgram)
#define glVertexAttribDivisor (gles_dispatch.p_glVertexAttribDivisor)
#define glVertexAttribIPointer (gles_dispatch.p_glVertexAttribIPointer)
#define glVertexAttribPointer (gles_dispatch.p_glVertexAttribPointer)
#define glViewport (gles_dispatch.p_glViewport)

#endif

/* end of linux/gfx3_gles_dispatch.h */

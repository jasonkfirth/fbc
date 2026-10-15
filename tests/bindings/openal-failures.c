/*
    FreeBASIC OpenAL example failure regression
    File: openal-failures.c
    Purpose: Check source/buffer ownership and error cleanup independently.
    Responsibilities: Native SDK declarations and controlled failure results.
    This file contains no audio renderer or playback qualification.
*/
#include <AL/al.h>
#include <AL/alut.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *mode;
static int source_live,buffer_live,exits,source_deletes,buffer_deletes;
static ALenum error;
static int is(const char *name) { return !strcmp(mode,name); }

static void verify(void)
{
    assert(exits==(is("init") ? 0 : 1));
    if(!is("init") && !is("buffer")) assert(buffer_deletes==1);
    if(!is("init") && !is("buffer") && !is("source")) assert(source_deletes==1);
    assert(!source_live && !buffer_live);
    printf("OPENAL_FAILURE=PASS mode=%s sources=%d buffers=%d exits=%d\n",mode,source_deletes,buffer_deletes,exits);
}

__attribute__((constructor)) static void setup(void)
{
    mode=getenv("AL_TEST_FAILURE");assert(mode);atexit(verify);
}

ALboolean __wrap_alutInit(int *argc,char **argv) { (void)argc;(void)argv;return is("init") ? AL_FALSE : AL_TRUE; }
ALboolean __wrap_alutExit(void) { exits++;source_live=buffer_live=0;return is("exit") ? AL_FALSE : AL_TRUE; }
ALenum __wrap_alutGetError(void) { return ALUT_ERROR_NO_ERROR; }
ALuint __wrap_alutCreateBufferFromFile(const char *filename)
{
    assert(filename && *filename);
    if(is("buffer")) return 0;
    buffer_live=1;return 1;
}

ALenum __wrap_alGetError(void) { ALenum result=error;error=AL_NO_ERROR;return result; }
void __wrap_alGenSources(ALsizei count,ALuint *sources)
{
    assert(count==1);
    if(is("source")) { *sources=0;error=AL_OUT_OF_MEMORY;return; }
    *sources=2;source_live=1;
}
void __wrap_alSourcei(ALuint source,ALenum key,ALint value)
{
    assert(source==2 && source_live);
    if(key==AL_BUFFER) assert(value==1 && buffer_live);
    if(key==AL_LOOPING && is("configure")) error=AL_INVALID_OPERATION;
}
void __wrap_alSourcef(ALuint source,ALenum key,ALfloat value) { assert(source==2 && source_live);(void)key;(void)value; }
void __wrap_alSourcefv(ALuint source,ALenum key,const ALfloat *values) { assert(source==2 && source_live && values);(void)key; }
void __wrap_alListenerfv(ALenum key,const ALfloat *values)
{
    assert(values);
    if(key==AL_ORIENTATION) assert(values[0]==0 && values[1]==0 && values[2]==-1 && values[3]==0 && values[4]==1 && values[5]==0);
}
void __wrap_alSourcePlay(ALuint source) { assert(source==2 && source_live);if(is("play")) error=AL_INVALID_OPERATION; }
void __wrap_alSourcePause(ALuint source) { assert(source==2 && source_live); }
void __wrap_alSourceStop(ALuint source) { assert(source==2 && source_live);if(is("stop")) error=AL_INVALID_OPERATION; }
void __wrap_alDeleteSources(ALsizei count,const ALuint *sources)
{
    assert(count==1 && *sources==2 && source_live);source_deletes++;
    if(is("delete-source")) { error=AL_INVALID_OPERATION;return; }
    source_live=0;
}
void __wrap_alDeleteBuffers(ALsizei count,const ALuint *buffers)
{
    assert(count==1 && *buffers==1 && buffer_live);buffer_deletes++;
    if(source_live || is("delete-buffer")) { error=AL_INVALID_OPERATION;return; }
    buffer_live=0;
}
/* end of openal-failures.c */

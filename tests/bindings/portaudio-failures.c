/*
    FreeBASIC PortAudio example failure regression
    File: portaudio-failures.c
    Purpose: Check original-example cleanup with controlled native API results.
    Responsibilities: SDK signatures, stream lifetime and callback buffer guards.
    This file contains no playback replacement or hardware acceptance.
*/
#include <portaudio.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *mode;
static int closes,terminations,opens;
static PaStreamCallback *callback;
static void *payload;
static PaDeviceInfo device={1,"Controlled device",7,0,1,0,0,0,0,44100};
static PaHostApiInfo api={1,paALSA,"Controlled API",1,paNoDevice,0};

static int is(const char *name) { return !strcmp(mode,name); }

static void verify(void)
{
    int expected_closes=(is("start") || is("stop") || is("close")) ? 1 : (is("normal") || is("terminate")) ? 3 : 0;
    assert(closes==expected_closes);
    assert(terminations==(is("init") ? 0 : 1));
    printf("PORTAUDIO_FAILURE=PASS mode=%s closes=%d terminations=%d\n",mode,closes,terminations);
}

__attribute__((constructor)) static void setup(void)
{
    mode=getenv("PA_TEST_FAILURE");
    assert(mode);
    atexit(verify);
}

PaError __wrap_Pa_Initialize(void) { return is("init") ? paUnanticipatedHostError : paNoError; }
PaError __wrap_Pa_Terminate(void) { terminations++; return is("terminate") ? paUnanticipatedHostError : paNoError; }
PaDeviceIndex __wrap_Pa_GetDefaultOutputDevice(void) { return is("device") ? paNoDevice : 0; }
const PaDeviceInfo *__wrap_Pa_GetDeviceInfo(PaDeviceIndex index) { assert(index==0); return is("info") ? NULL : &device; }
const PaHostApiInfo *__wrap_Pa_GetHostApiInfo(PaHostApiIndex index) { assert(index==device.hostApi); return is("host") ? NULL : &api; }

PaError __wrap_Pa_OpenDefaultStream(PaStream **stream,int input,int output,PaSampleFormat format,
                                   double rate,unsigned long frames,PaStreamCallback *fn,void *user)
{
    assert(input==0 && output==1 && format==paInt32 && rate==44100 && frames==4410);
    opens++;
    if(is("open")) return paUnanticipatedHostError;
    *stream=(PaStream *)(uintptr_t)1;
    callback=fn;
    payload=user;
    return paNoError;
}

PaError __wrap_Pa_StartStream(PaStream *stream)
{
    struct { uint32_t before; int32_t samples[32]; uint32_t after; } guarded;
    PaStreamCallbackTimeInfo time={0,0,0};
    assert(stream==(PaStream *)(uintptr_t)1);
    if(is("start")) return paUnanticipatedHostError;
    guarded.before=0x12345678u;guarded.after=0x87654321u;
    memset(guarded.samples,0,sizeof(guarded.samples));
    assert(callback(NULL,NULL,32,&time,0,payload)==paAbort);
    assert(callback(NULL,guarded.samples,32,&time,0,NULL)==paAbort);
    assert(callback(NULL,guarded.samples,0,&time,0,payload)==paContinue);
    assert(callback(NULL,guarded.samples,32,&time,0,payload)==paContinue);
    assert(guarded.before==0x12345678u && guarded.after==0x87654321u);
    assert(guarded.samples[1]!=0);
    return paNoError;
}

PaError __wrap_Pa_StopStream(PaStream *stream) { assert(stream==(PaStream *)(uintptr_t)1); return is("stop") ? paUnanticipatedHostError : paNoError; }
PaError __wrap_Pa_CloseStream(PaStream *stream) { assert(stream==(PaStream *)(uintptr_t)1); closes++; return is("close") ? paUnanticipatedHostError : paNoError; }
void __wrap_Pa_Sleep(long milliseconds) { assert(milliseconds==440 || milliseconds==330 || milliseconds==220); }
/* end of portaudio-failures.c */

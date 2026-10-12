/*
    FreeBASIC Linux sound driver regression
    File: sfx-linux-write.c
    Purpose: Check bounded writes and accepted-sample diagnostics.
    Responsibilities: Controlled API failures, partial writes and input bounds.
    This file contains no playback emulator or physical-device acceptance.
*/
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include "../../src/sfxlib/linux/sfx_driver_alsa.c"
#include "../../src/sfxlib/linux/sfx_driver_pulse.c"

/* These unit-only link wrappers control the SDK's write results. Native
   playback is tested separately against real ALSA and PulseAudio services.
   Section garbage collection drops the unused device-opening routines. */
FB_SFX_LINUX_STATE fb_sfx_linux={1,44100,2,512,1};
static int responses[8],response_count,response_index,write_calls,wait_calls;
static int diagnostic_frames,diagnostic_calls,pulse_failure;
static const float *expected_samples;

snd_pcm_sframes_t __wrap_snd_pcm_writei(snd_pcm_t *pcm,const void *samples,snd_pcm_uframes_t frames)
{
    (void)pcm;
    assert(samples && frames>0);
    write_calls++;
    if(response_index<response_count) return responses[response_index++];
    return 0;
}

int __wrap_snd_pcm_wait(snd_pcm_t *pcm,int milliseconds)
{
    (void)pcm;
    assert(milliseconds==ALSA_WRITE_WAIT_MS);
    wait_calls++;
    return 0;
}

int __wrap_snd_pcm_prepare(snd_pcm_t *pcm) { (void)pcm; return 0; }
int __wrap_nanosleep(const struct timespec *request,struct timespec *remaining)
{
    (void)remaining;
    assert(request->tv_sec==0 && request->tv_nsec==ALSA_WRITE_WAIT_MS*1000000L);
    return 0;
}

int __wrap_pa_simple_write(pa_simple *stream,const void *samples,size_t bytes,int *error)
{
    (void)stream;
    assert(samples && bytes==16);
    write_calls++;
    if(pulse_failure) { *error=PA_ERR_IO; return -1; }
    return 0;
}

void fb_sfxDriverDiagnostics(const char *name,const float *samples,int frames,int channels)
{
    assert(!strcmp(name,"ALSA") || !strcmp(name,"PulseAudio"));
    assert(channels==2 && frames>0 && diagnostic_frames+frames<=4);
    assert(samples==expected_samples+diagnostic_frames*channels);
    diagnostic_frames+=frames;
    diagnostic_calls++;
}

void fb_sfxDriverStatsRecordUnderrun(const char *name) { assert(!strcmp(name,"ALSA")); }
void fb_sfxDriverStatsRecordRecovery(const char *name) { assert(!strcmp(name,"ALSA")); }

static void reset(const float *samples,const int *script,int count)
{
    memset(responses,0,sizeof(responses));
    if(count) memcpy(responses,script,(size_t)count*sizeof(int));
    response_count=count;
    response_index=write_calls=wait_calls=diagnostic_frames=diagnostic_calls=0;
    expected_samples=samples;
}

int main(void)
{
    const float samples[]={.1f,.2f,.3f,.4f,.5f,.6f,.7f,.8f};
    const int partial[]={2,-EAGAIN,2},zero_then_full[]={0,4};
    const int error_after_prefix[]={2,-EIO},underrun[]={2,-EPIPE,-EPIPE},overreported[]={5};
    alsa_pcm=(snd_pcm_t *)(uintptr_t)1;
    pulse_stream=(pa_simple *)(uintptr_t)1;
    reset(samples,NULL,0);
    assert(alsa_driver_write(samples,4)==0);
    assert(write_calls==ALSA_WRITE_RETRIES+1 && wait_calls==ALSA_WRITE_RETRIES && diagnostic_frames==0);
    reset(samples,zero_then_full,2);
    assert(alsa_driver_write(samples,4)==4 && wait_calls==1 && diagnostic_frames==4);
    reset(samples,partial,3);
    assert(alsa_driver_write(samples,4)==4 && diagnostic_calls==2 && diagnostic_frames==4);
    reset(samples,error_after_prefix,2);
    assert(alsa_driver_write(samples,4)==2 && diagnostic_frames==2);
    reset(samples,underrun,3);
    assert(alsa_driver_write(samples,4)==2 && diagnostic_frames==2);
    reset(samples,overreported,1);
    assert(alsa_driver_write(samples,4)==-1 && diagnostic_frames==0);
    reset(samples,NULL,0);
    assert(alsa_driver_write(samples,INT_MAX)==-1 && write_calls==0);
    assert(alsa_driver_write(NULL,4)==-1 && write_calls==0);
    reset(samples,NULL,0);
    pulse_failure=0;
    assert(pulse_driver_write(samples,4)==4 && diagnostic_frames==4);
    reset(samples,NULL,0);
    pulse_failure=1;
    assert(pulse_driver_write(samples,4)==-1 && diagnostic_frames==0);
    assert(pulse_driver_write(samples,INT_MAX)==-1 && write_calls==1);
    free(alsa_pcm_buffer);
    free(pulse_pcm_buffer);
    puts("SFX_LINUX_WRITE=PASS zero-retries partial-prefix errors input-bounds pulse-acceptance");
    return 0;
}
/* end of sfx-linux-write.c */

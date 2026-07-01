/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_driver_audio.c

    Purpose:

        Implement the NuttX audio-framework playback driver.

    Responsibilities:

        - open the NuttX PCM playback device
        - configure signed 16-bit stereo PCM output
        - convert sfxlib mixer samples into NuttX audio pipeline buffers
        - enqueue buffers through the public AUDIOIOC interface

    This file intentionally does NOT contain:

        - mixer logic
        - HDMI, I2S, or board clock setup
        - MIDI support
        - capture support
*/

#include <nuttx/config.h>

#include "../fb_sfx_driver.h"
#include "../fb_sfx_driver_diag.h"
#include "../fb_sfx_internal.h"

#ifdef CONFIG_AUDIO
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <mqueue.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <nuttx/audio/audio.h>
#endif

/* ------------------------------------------------------------------------- */
/* Driver constants                                                          */
/* ------------------------------------------------------------------------- */

#define FB_SFX_NUTTX_MAX_BUFFERS 4
#define FB_SFX_NUTTX_MIN_BUFFER_BYTES 512
#define FB_SFX_NUTTX_DEFAULT_DEVICE "/dev/audio/pcm0p"

/* ------------------------------------------------------------------------- */
/* Driver state                                                              */
/* ------------------------------------------------------------------------- */

#ifdef CONFIG_AUDIO
typedef struct FB_SFX_NUTTX_BUFFER
{
    FAR struct ap_buffer_s *apb;
    int queued;

} FB_SFX_NUTTX_BUFFER;

static int g_nuttx_audio_fd = -1;
static mqd_t g_nuttx_audio_mq = (mqd_t)-1;
static char g_nuttx_audio_mqname[32];
static FB_SFX_NUTTX_BUFFER g_nuttx_audio_buffers[FB_SFX_NUTTX_MAX_BUFFERS];
static int g_nuttx_audio_buffer_count = 0;
static int g_nuttx_audio_rate = FB_SFX_DEFAULT_RATE;
static int g_nuttx_audio_channels = FB_SFX_DEFAULT_CHANNELS;
static int g_nuttx_audio_started = 0;
static unsigned int g_nuttx_audio_write_count = 0;
#endif

/* ------------------------------------------------------------------------- */
/* NuttX audio helpers                                                       */
/* ------------------------------------------------------------------------- */

#ifdef CONFIG_AUDIO
static const char *nuttx_audio_device_path(void)
{
    const char *path;

    path = getenv("FB_SFX_NUTTX_AUDIO_DEVICE");
    if ((path != NULL) && (*path != '\0'))
        return path;

    return FB_SFX_NUTTX_DEFAULT_DEVICE;
}

static int nuttx_audio_keep_open_on_exit(void)
{
    const char *value;

    /*
        QEMU virtio-sound can deliver a completion interrupt after the last
        user close.  The NuttX audio upper half frees its shared status block
        on close, so the late interrupt can fault before the emulator exits.
        Keep this behind an environment switch so hardware runs still exercise
        the normal close path.
    */
    value = getenv("FB_SFX_NUTTX_KEEP_AUDIO_OPEN");
    return (value != NULL) && (strcmp(value, "1") == 0);
}

static int nuttx_audio_configure(int fd, int rate, int channels)
{
    struct audio_caps_desc_s cap;

    memset(&cap, 0, sizeof(cap));

    /*
        NuttX audio applications configure PCM playback through
        AUDIOIOC_CONFIGURE.  The public nxaudio helper uses this same layout:
        ac_controls.hw[0] carries the sample rate and ac_controls.b[2] carries
        the bits per sample.
    */
    cap.caps.ac_len = sizeof(struct audio_caps_s);
    cap.caps.ac_type = AUDIO_TYPE_OUTPUT;
    cap.caps.ac_channels = (uint8_t)channels;
    cap.caps.ac_format.hw = AUDIO_FMT_PCM;
    cap.caps.ac_controls.hw[0] = (uint16_t)rate;
    cap.caps.ac_controls.b[2] = 16;
    cap.caps.ac_controls.b[3] = 0;

    return ioctl(fd, AUDIOIOC_CONFIGURE, (unsigned long)(uintptr_t)&cap);
}

static int nuttx_audio_create_mq(int fd, int message_count)
{
    struct mq_attr attr;
    int result;

    snprintf(g_nuttx_audio_mqname, sizeof(g_nuttx_audio_mqname),
        "/fbsfx%ld", (long)getpid());

    memset(&attr, 0, sizeof(attr));
    attr.mq_maxmsg = message_count;
    attr.mq_msgsize = sizeof(struct audio_msg_s);
    attr.mq_flags = O_NONBLOCK;

    mq_unlink(g_nuttx_audio_mqname);

    g_nuttx_audio_mq = mq_open(g_nuttx_audio_mqname,
        O_RDWR | O_CREAT | O_NONBLOCK, 0644, &attr);
    if (g_nuttx_audio_mq == (mqd_t)-1)
        return -1;

    result = ioctl(fd, AUDIOIOC_REGISTERMQ, (unsigned long)g_nuttx_audio_mq);
    if (result < 0)
    {
        mq_close(g_nuttx_audio_mq);
        mq_unlink(g_nuttx_audio_mqname);
        g_nuttx_audio_mq = (mqd_t)-1;
        return -1;
    }

    return 0;
}

static void nuttx_audio_poll_messages(void)
{
    struct audio_msg_s msg;
    unsigned int prio;
    ssize_t nread;
    int i;

    if (g_nuttx_audio_mq == (mqd_t)-1)
        return;

    for (;;)
    {
        nread = mq_receive(g_nuttx_audio_mq, (char *)&msg,
            sizeof(msg), &prio);
        if (nread != sizeof(msg))
            break;

        if (msg.msg_id != AUDIO_MSG_DEQUEUE)
            continue;

        for (i = 0; i < g_nuttx_audio_buffer_count; i++)
        {
            if (g_nuttx_audio_buffers[i].apb == msg.u.ptr)
            {
                g_nuttx_audio_buffers[i].queued = 0;
                break;
            }
        }
    }
}

static FB_SFX_NUTTX_BUFFER *nuttx_audio_find_buffer(void)
{
    int retry;
    int i;

    for (retry = 0; retry < 50; retry++)
    {
        nuttx_audio_poll_messages();

        for (i = 0; i < g_nuttx_audio_buffer_count; i++)
        {
            if (!g_nuttx_audio_buffers[i].queued)
                return &g_nuttx_audio_buffers[i];
        }

        usleep(1000);
    }

    return NULL;
}

static int nuttx_audio_queued_count(void)
{
    int queued;
    int i;

    nuttx_audio_poll_messages();

    queued = 0;

    for (i = 0; i < g_nuttx_audio_buffer_count; i++)
    {
        if (g_nuttx_audio_buffers[i].queued)
            queued++;
    }

    return queued;
}

static int nuttx_audio_wait_until_idle(void)
{
    int retry;

    for (retry = 0; retry < 200; retry++)
    {
        if (nuttx_audio_queued_count() == 0)
            return 0;

        usleep(10000);
    }

    return -1;
}

static int nuttx_audio_alloc_buffers(int fd)
{
    struct ap_buffer_info_s info;
    int desired_count;
    int i;

    memset(&info, 0, sizeof(info));
    if (ioctl(fd, AUDIOIOC_GETBUFFERINFO, (unsigned long)(uintptr_t)&info) < 0)
        return -1;

    if (info.buffer_size < FB_SFX_NUTTX_MIN_BUFFER_BYTES)
        return -1;

    desired_count = (int)info.nbuffers;
    if (desired_count <= 0)
        desired_count = 2;

    if (desired_count > FB_SFX_NUTTX_MAX_BUFFERS)
        desired_count = FB_SFX_NUTTX_MAX_BUFFERS;

    for (i = 0; i < desired_count; i++)
    {
        struct audio_buf_desc_s desc;

        memset(&desc, 0, sizeof(desc));
        desc.numbytes = info.buffer_size;
        desc.u.pbuffer = &g_nuttx_audio_buffers[i].apb;

        if (ioctl(fd, AUDIOIOC_ALLOCBUFFER,
            (unsigned long)(uintptr_t)&desc) < 0)
        {
            return -1;
        }

        if (g_nuttx_audio_buffers[i].apb == NULL)
            return -1;

        g_nuttx_audio_buffers[i].queued = 0;
        g_nuttx_audio_buffer_count++;
    }

    return 0;
}

static void nuttx_audio_free_buffers(void)
{
    int i;

    for (i = 0; i < g_nuttx_audio_buffer_count; i++)
    {
        if (g_nuttx_audio_buffers[i].apb != NULL)
        {
            struct audio_buf_desc_s desc;

            memset(&desc, 0, sizeof(desc));
            desc.u.buffer = g_nuttx_audio_buffers[i].apb;
            ioctl(g_nuttx_audio_fd, AUDIOIOC_FREEBUFFER,
                (unsigned long)(uintptr_t)&desc);
        }

        g_nuttx_audio_buffers[i].apb = NULL;
        g_nuttx_audio_buffers[i].queued = 0;
    }

    g_nuttx_audio_buffer_count = 0;
}

static int nuttx_audio_enqueue(FB_SFX_NUTTX_BUFFER *buffer, int bytes)
{
    struct audio_buf_desc_s desc;

    if ((buffer == NULL) || (buffer->apb == NULL) || (bytes <= 0))
        return -1;

    buffer->apb->nbytes = (apb_samp_t)bytes;
    buffer->apb->curbyte = 0;
    buffer->apb->nsamples = (apb_samp_t)(bytes / sizeof(short));
    buffer->apb->flags = 0;

    memset(&desc, 0, sizeof(desc));
    desc.numbytes = (apb_samp_t)bytes;
    desc.u.buffer = buffer->apb;

    if (ioctl(g_nuttx_audio_fd, AUDIOIOC_ENQUEUEBUFFER,
        (unsigned long)(uintptr_t)&desc) < 0)
    {
        return -1;
    }

    buffer->queued = 1;

    if (!g_nuttx_audio_started)
    {
        if (ioctl(g_nuttx_audio_fd, AUDIOIOC_START, 0) < 0)
            return -1;

        g_nuttx_audio_started = 1;
    }

    return 0;
}
#endif

/* ------------------------------------------------------------------------- */
/* Driver lifecycle                                                          */
/* ------------------------------------------------------------------------- */

static int nuttx_audio_init(int rate, int channels, int buffer, int flags)
{
#ifdef CONFIG_AUDIO
    const char *path;
    int fd;
    int mq_messages;

    (void)buffer;
    (void)flags;

    if (g_nuttx_audio_fd >= 0)
        return 0;

    g_nuttx_audio_rate = (rate > 0) ? rate : FB_SFX_DEFAULT_RATE;
    g_nuttx_audio_channels = (channels > 0) ? channels : FB_SFX_DEFAULT_CHANNELS;

    if ((g_nuttx_audio_channels <= 0) || (g_nuttx_audio_channels > 2))
        return -1;

    path = nuttx_audio_device_path();
    fd = open(path, O_RDWR | O_CLOEXEC);
    if (fd < 0)
        return -1;

    if (ioctl(fd, AUDIOIOC_RESERVE, 0) < 0)
    {
        close(fd);
        return -1;
    }

    if (nuttx_audio_configure(fd, g_nuttx_audio_rate,
        g_nuttx_audio_channels) < 0)
    {
        ioctl(fd, AUDIOIOC_RELEASE, 0);
        close(fd);
        return -1;
    }

    g_nuttx_audio_fd = fd;

    if (nuttx_audio_alloc_buffers(fd) < 0)
    {
        nuttx_audio_free_buffers();
        ioctl(fd, AUDIOIOC_RELEASE, 0);
        close(fd);
        g_nuttx_audio_fd = -1;
        return -1;
    }

    mq_messages = g_nuttx_audio_buffer_count + 8;
    if (nuttx_audio_create_mq(fd, mq_messages) < 0)
    {
        nuttx_audio_free_buffers();
        ioctl(fd, AUDIOIOC_RELEASE, 0);
        close(fd);
        g_nuttx_audio_fd = -1;
        return -1;
    }

    g_nuttx_audio_started = 0;
    g_nuttx_audio_write_count = 0;

    printf("FB_NUTTX_QEMU_SFX_AUDIO_OPEN device=%s buffers=%d rate=%d channels=%d\n",
        path, g_nuttx_audio_buffer_count, g_nuttx_audio_rate,
        g_nuttx_audio_channels);
    fflush(stdout);

    return 0;
#else
    (void)rate;
    (void)channels;
    (void)buffer;
    (void)flags;

    return -1;
#endif
}

static void nuttx_audio_exit(void)
{
#ifdef CONFIG_AUDIO
    if (g_nuttx_audio_fd < 0)
        return;

    if (nuttx_audio_keep_open_on_exit())
    {
        printf("FB_NUTTX_QEMU_SFX_AUDIO_KEEP_OPEN queued=%d\n",
            nuttx_audio_queued_count());
        fflush(stdout);
        return;
    }

    /*
        The NuttX audio lower half owns queued APBs until it sends
        AUDIO_MSG_DEQUEUE back through the registered message queue.  Do not
        tear down or free those buffers early.  QEMU virtio-sound is quick
        enough to prove the path, but it can still have buffers in flight when
        a short BASIC program exits.
    */
    if (nuttx_audio_wait_until_idle() != 0)
    {
        printf("FB_NUTTX_QEMU_SFX_AUDIO_LEAVE_OPEN queued=%d\n",
            nuttx_audio_queued_count());
        fflush(stdout);
        return;
    }

    ioctl(g_nuttx_audio_fd, AUDIOIOC_STOP, 0);
    ioctl(g_nuttx_audio_fd, AUDIOIOC_SHUTDOWN, 0);

    if (g_nuttx_audio_mq != (mqd_t)-1)
    {
        ioctl(g_nuttx_audio_fd, AUDIOIOC_UNREGISTERMQ,
            (unsigned long)g_nuttx_audio_mq);
        mq_close(g_nuttx_audio_mq);
        mq_unlink(g_nuttx_audio_mqname);
        g_nuttx_audio_mq = (mqd_t)-1;
    }

    nuttx_audio_free_buffers();

    ioctl(g_nuttx_audio_fd, AUDIOIOC_RELEASE, 0);
    close(g_nuttx_audio_fd);

    g_nuttx_audio_fd = -1;
    g_nuttx_audio_started = 0;
    g_nuttx_audio_write_count = 0;
#endif
}

/* ------------------------------------------------------------------------- */
/* Driver write                                                              */
/* ------------------------------------------------------------------------- */

static int nuttx_audio_write(const float *samples, int frames)
{
#ifdef CONFIG_AUDIO
    FB_SFX_NUTTX_BUFFER *buffer;
    int max_frames;
    int accepted_frames;
    int sample_count;
    int bytes;

    if ((g_nuttx_audio_fd < 0) || (samples == NULL) || (frames <= 0))
        return -1;

    buffer = nuttx_audio_find_buffer();
    if (buffer == NULL)
        return 0;

    max_frames = (int)(buffer->apb->nmaxbytes /
        (sizeof(short) * (size_t)g_nuttx_audio_channels));
    if (max_frames <= 0)
        return -1;

    accepted_frames = frames;
    if (accepted_frames > max_frames)
        accepted_frames = max_frames;

    sample_count = accepted_frames * g_nuttx_audio_channels;
    bytes = sample_count * (int)sizeof(short);

    fb_sfxConvertFloatToS16(samples, (short *)buffer->apb->samp,
        sample_count);

    if (nuttx_audio_enqueue(buffer, bytes) < 0)
        return -1;

    g_nuttx_audio_write_count++;
    fb_sfxDriverDiagnostics("NuttX audio", samples, accepted_frames,
        g_nuttx_audio_channels);

    printf("FB_NUTTX_QEMU_SFX_AUDIO_ENQUEUE write=%u frames=%d bytes=%d\n",
        g_nuttx_audio_write_count, accepted_frames, bytes);
    fflush(stdout);

    return accepted_frames;
#else
    (void)samples;
    (void)frames;

    return -1;
#endif
}

static int nuttx_audio_device_list(void)
{
#ifdef CONFIG_AUDIO
    return 1;
#else
    return 0;
#endif
}

const FB_SFX_DRIVER fb_sfxDriverNuttXAudio =
{
    "NuttX audio",
    FB_SFX_DRIVER_CAP_BLOCKING,
    nuttx_audio_init,
    nuttx_audio_exit,
    nuttx_audio_write,
    NULL,
    NULL,
    nuttx_audio_device_list,
    NULL
};

/* end of sfx_driver_audio.c */

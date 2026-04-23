/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: fb_sfx_driver.h

    Purpose:

        Define the platform audio driver interface used by the
        FreeBASIC sound subsystem.

        This header establishes the contract between the portable
        audio engine and platform-specific audio backends.

        Each operating system backend implements a driver structure
        that conforms to this interface.

    Responsibilities:

        • define the SFXDRIVER interface
        • document expectations of platform backends
        • define driver capability flags
        • expose the global driver list

    This file intentionally does NOT contain:

        • mixer implementation
        • command layer logic
        • runtime state definitions
        • platform-specific code

    Platform drivers are implemented in platform directories such as:

        sfxlib/linux
        sfxlib/win32
        sfxlib/darwin
        sfxlib/haiku
*/

#ifndef __FB_SFX_DRIVER_H__
#define __FB_SFX_DRIVER_H__

#include "fb_sfx.h"

#ifdef __cplusplus
extern "C" {
#endif


/* ------------------------------------------------------------------------- */
/* Driver capability flags                                                   */
/* ------------------------------------------------------------------------- */

/*
    Drivers may advertise optional capabilities.

    The runtime may adjust behavior depending on the features
    available from the backend.
*/

#define FB_SFX_DRIVER_CAP_CAPTURE     0x0001
#define FB_SFX_DRIVER_CAP_MIDI        0x0002
#define FB_SFX_DRIVER_CAP_STREAM      0x0004
#define FB_SFX_DRIVER_CAP_LOOPBACK    0x0008



/* ------------------------------------------------------------------------- */
/* Driver initialization flags                                               */
/* ------------------------------------------------------------------------- */

/*
    Initialization flags passed to the driver init() routine.

    These flags allow the runtime to request specific behaviors
    from the backend when possible.
*/

#define FB_SFX_INIT_DEFAULT      0x0000
#define FB_SFX_INIT_LOW_LATENCY  0x0001
#define FB_SFX_INIT_EXCLUSIVE    0x0002



/* ------------------------------------------------------------------------- */
/* Platform driver interface                                                 */
/* ------------------------------------------------------------------------- */

/*
    SFXDRIVER

    Platform audio backend interface.

    Each operating system implements one or more drivers that conform
    to this interface.  Drivers are registered in a global driver list
    and attempted in order during subsystem initialization.

    Initialization model:

        runtime → driver.init()

    If initialization succeeds, the driver becomes the active backend.

    Audio flow model:

        mixer → driver.write() → OS audio system

    Capture flow model:

        OS audio system → driver.capture_read() → runtime capture buffer
*/

typedef struct SFXDRIVER
{
    const char *name;

    /*
        Capability flags.

        Indicates optional functionality supported by the backend.
    */

    unsigned int capabilities;


    /* ------------------------------------------------------------------ */
    /* Driver lifecycle                                                   */
    /* ------------------------------------------------------------------ */

    /*
        Initialize the audio device.

        Parameters:

            rate        requested sample rate
            channels    requested number of channels
            buffer      requested buffer size (frames)
            flags       initialization behavior flags

        Returns:

            0 on success
            non-zero on failure
    */

    int (*init)(
        int rate,
        int channels,
        int buffer,
        int flags
    );


    /*
        Shutdown the audio device.

        Called when the runtime exits or switches drivers.
    */

    void (*exit)(void);



    /* ------------------------------------------------------------------ */
    /* Audio output                                                       */
    /* ------------------------------------------------------------------ */

    /*
        Write audio samples to the device.

        Samples are provided in the internal mixer format.

        Drivers may convert samples to the format required by the
        operating system audio API.
    */

    int (*write)(
        const float *samples,
        int frames
    );



    /* ------------------------------------------------------------------ */
    /* Audio capture                                                      */
    /* ------------------------------------------------------------------ */

    /*
        Read captured audio samples from the device.

        Not all drivers implement capture.  Drivers that do not support
        capture should set this pointer to NULL.
    */

    int (*capture_read)(
        short *samples,
        int frames
    );



    /* ------------------------------------------------------------------ */
    /* Polling                                                            */
    /* ------------------------------------------------------------------ */

    /*
        Driver poll routine.

        Some platforms require periodic polling to process events,
        refill buffers, or maintain streaming playback.

        Drivers that do not require polling may set this pointer to NULL.
    */

    void (*poll)(void);



    /* ------------------------------------------------------------------ */
    /* Device management                                                  */
    /* ------------------------------------------------------------------ */

    /*
        Enumerate available audio devices.

        Returns the number of devices detected.

        Drivers may return zero if enumeration is not supported.
    */

    int (*device_list)(void);


    /*
        Select an audio device by index.

        Drivers that do not support device selection may ignore this
        call or return an error code.
    */

    int (*device_select)(int device_id);


} SFXDRIVER;



/* ------------------------------------------------------------------------- */
/* Driver list                                                               */
/* ------------------------------------------------------------------------- */

/*
    Global list of available drivers.

    The runtime attempts to initialize drivers in order until one
    successfully starts.

    Example:

        ALSA → PulseAudio → Null driver
*/

extern const SFXDRIVER *__fb_sfx_drivers_list[];



/* ------------------------------------------------------------------------- */
/* Null driver                                                               */
/* ------------------------------------------------------------------------- */

/*
    The null driver is always available and ensures that the sound
    subsystem can operate even when no audio hardware is present.

    The null driver consumes audio samples but does not send them
    to any device.
*/

extern const SFXDRIVER __fb_sfxDriverNull;



#ifdef __cplusplus
}
#endif

#endif

/* end of fb_sfx_driver.h */

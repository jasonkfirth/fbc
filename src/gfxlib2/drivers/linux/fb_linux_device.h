/*
 * Project: FreeBASIC Linux graphics support
 * File: fb_linux_device.h
 * Purpose: Expose the Linux device-form-factor query shared by both gfxlibs.
 * Responsibilities: Declare the platform-owned device-class detector.
 * This file intentionally does NOT contain session or hardware probing.
 */
#ifndef FB_LINUX_DEVICE_H
#define FB_LINUX_DEVICE_H

int fb_hLinuxDeviceClass(void);

#endif
/* end of fb_linux_device.h */

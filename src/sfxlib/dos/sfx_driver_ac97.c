/* FreeBASIC Sound Library: dos/sfx_driver_ac97.c
 *
 * Play signed 16-bit stereo PCM through Intel ICH AC'97 bus-master DMA.
 * This file owns PCI BIOS discovery, codec setup and a conventional-memory
 * descriptor list. It contains neither software mixing nor an IRQ handler.
 *
 * AC'97 describes the codec link, not a universal PCI controller interface.
 * Only the explicitly listed Intel controllers with assigned legacy I/O BARs
 * are accepted. PCI BIOS discovery precedes every access to device ports.
 *
 * The sound core serializes writes and joins its optional DOS worker before
 * exit. Completion is polled with a deadline; the thread profile yields while
 * waiting. PCI interrupts stay disabled, avoiding ownership of a shared IRQ.
 */

#ifndef DISABLE_MSDOS

#include "../fb_sfx.h"
#include "../fb_sfx_internal.h"
#include "../fb_sfx_driver.h"
#if FB_SFX_DOS_THREADS
#include "../../rtlib/dos/fb_dos_thread.h"
#endif

#include <dpmi.h>
#include <go32.h>
#include <limits.h>
#include <pc.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/movedata.h>
#include <time.h>

/* Intel native audio bus-master registers, relative to NABMBAR. */
#define AC97_PO_BDBAR       0x10
#define AC97_PO_CIV         0x14
#define AC97_PO_LVI         0x15
#define AC97_PO_SR          0x16
#define AC97_PO_PICB        0x18
#define AC97_PO_CR          0x1B
#define AC97_GLOBAL_CONTROL 0x2C
#define AC97_GLOBAL_STATUS  0x30
#define AC97_CODEC_ACCESS   0x34

#define AC97_SR_HALTED      0x01
#define AC97_SR_LAST_DONE   0x04
#define AC97_SR_FIFO_ERROR  0x10
#define AC97_SR_CLEAR       0x1C
#define AC97_CR_RUN         0x01
#define AC97_CR_RESET       0x02
#define AC97_COLD_RELEASE   0x02
#define AC97_CODEC_READY    0x100
#define AC97_PCI_IO         0x01
#define AC97_PCI_MASTER     0x04
#define AC97_BDL_ENTRIES    32
#define AC97_MAX_FRAMES     4096
#define AC97_RATE           48000

/* One descriptor is eight little-endian bytes: physical buffer address,
 * then a 16-bit SAMPLE count and flags. Stereo frames each occupy two
 * samples. IOC and BUP remain clear (no IRQ, output silence after underrun).
 * The descriptor list must be eight-byte aligned; samples four-byte aligned.
 * DOS paragraphs satisfy both and are physically contiguous below 1 MiB.
 */
typedef struct AC97_DESCRIPTOR
{
    uint32_t address;
    uint32_t samples;
} AC97_DESCRIPTOR;

/* Only one controller is owned. All state is private to the serialized
 * driver lifecycle; the hardware sees only the DOS allocation, never heap
 * addresses or pageable protected-mode memory.
 */
static struct
{
    unsigned short pci;
    unsigned short command;
    unsigned short mixer;
    unsigned short busmaster;
    unsigned long global_control;
    unsigned short codec_saved[5];
    int codec_count;
    int pci_owned;
    int engine_owned;
    int selector;
    unsigned long physical;
    short *pcm;
    int frames;
    int channels;
    unsigned int descriptor;
    int started;
    int active;
    int quarantined;
} ac97;

/* Save only the codec registers this backend changes: master/PCM volume,
 * power control, extended audio control and front DAC sample rate.
 */
static const unsigned char codec_registers[] = { 0x02, 0x18, 0x26, 0x2A, 0x2C };

/* ------------------------------------------------------------------------- */
/* PCI BIOS and bounded register access                                      */
/* ------------------------------------------------------------------------- */

static int ac97_pci_call(__dpmi_regs *regs)
{
    /* INT 1Ah, AH=B1h. Both Carry and AH report failure. The DOS thread
     * provider serializes this real-mode bridge through its DPMI wrapper.
     */
    return __dpmi_int(0x1A, regs) == 0 &&
        !(regs->x.flags & 1) && regs->h.ah == 0;
}

static int ac97_pci_read(unsigned short device, unsigned short offset,
                         unsigned long *value)
{
    __dpmi_regs regs;
    memset(&regs, 0, sizeof(regs));
    regs.x.ax = 0xB10A; /* Read configuration DWORD; BX is bus/devfn. */
    regs.x.bx = device;
    regs.x.di = offset;
    if (!ac97_pci_call(&regs))
        return 0;
    *value = regs.d.ecx;
    return 1;
}

static int ac97_pci_command(unsigned short value)
{
    __dpmi_regs regs;
    unsigned long readback;
    memset(&regs, 0, sizeof(regs));
    /* Write only the command WORD. A DWORD write would also acknowledge
     * write-one-to-clear PCI status bits belonging to the system.
     */
    regs.x.ax = 0xB10C;
    regs.x.bx = ac97.pci;
    regs.x.di = 4;
    regs.x.cx = value;
    return ac97_pci_call(&regs) && ac97_pci_read(ac97.pci, 4, &readback) &&
        (readback & (AC97_PCI_IO | AC97_PCI_MASTER)) ==
        (value & (AC97_PCI_IO | AC97_PCI_MASTER));
}

static int ac97_supported(unsigned long identity)
{
    if ((identity & 0xFFFF) != 0x8086)
        return 0;
    switch (identity >> 16)
    {
        case 0x2415: /* 82801AA, ICH */
        case 0x2425: /* 82801AB, ICH0 */
        case 0x2445: /* 82801BA, ICH2 */
        case 0x2485: /* ICH3 */
            return 1;
        default:
            return 0;
    }
}

static int ac97_find(void)
{
    __dpmi_regs regs;
    unsigned int index;
    memset(&regs, 0, sizeof(regs));
    regs.x.ax = 0xB101; /* Installation check: EDX must contain "PCI ". */
    if (!ac97_pci_call(&regs) || regs.d.edx != 0x20494350UL || regs.h.bh < 2)
        return 0;

    /* A bounded enumeration also protects against a broken BIOS returning
     * the same class match indefinitely. No raw CF8/CFC probing is used.
     */
    for (index = 0; index < 256; ++index)
    {
        unsigned short device;
        unsigned long identity, class_code, mixer, busmaster, command;
        memset(&regs, 0, sizeof(regs));
        regs.x.ax = 0xB103; /* Find PCI class: multimedia/audio, interface 0. */
        regs.d.ecx = 0x040100;
        regs.x.si = index;
        if (!ac97_pci_call(&regs))
            break;
        device = regs.x.bx;
        if (!ac97_pci_read(device, 0, &identity) || !ac97_supported(identity) ||
            !ac97_pci_read(device, 8, &class_code) ||
            (class_code >> 8) != 0x040100 ||
            !ac97_pci_read(device, 0x10, &mixer) ||
            !ac97_pci_read(device, 0x14, &busmaster) ||
            !ac97_pci_read(device, 4, &command))
            continue;
        /* The BIOS must assign nonoverlapping I/O windows: 256 bytes for
         * NAMBAR and 64 for NABMBAR. Reject MMIO and unassigned/invalid BARs;
         * allocating PCI resources is the firmware's responsibility.
         */
        if ((mixer & 0xFF) != 1 || (busmaster & 0x3F) != 1)
            continue;
        mixer &= ~3UL;
        busmaster &= ~3UL;
        if (mixer < 0x100 || mixer > 0xFF00 ||
            busmaster < 0x100 || busmaster > 0xFFC0 ||
            (mixer < busmaster + 64 && busmaster < mixer + 256))
            continue;
        ac97.pci = device;
        ac97.command = (unsigned short)command;
        ac97.mixer = (unsigned short)mixer;
        ac97.busmaster = (unsigned short)busmaster;
        return 1;
    }
    return 0;
}

static void ac97_wait(void)
{
#if FB_SFX_DOS_THREADS
    /* This helper does not pump sound recursively. */
    fb_DosThreadDelay(1);
#endif
}

static int ac97_codec_access(void)
{
    uclock_t start = uclock();
    do
    {
        /* Reading CAS claims the semaphore; the following mixer access
         * releases it. A permanently busy or absent controller times out.
         */
        if (!(inportb(ac97.busmaster + AC97_CODEC_ACCESS) & 1))
            return 1;
        ac97_wait();
    } while (uclock() - start < UCLOCKS_PER_SEC / 10);
    return 0;
}

static int ac97_codec_read(unsigned int reg, unsigned short *value)
{
    if (!ac97_codec_access())
        return 0;
    *value = inportw(ac97.mixer + reg);
    return *value != 0xFFFF;
}

static int ac97_codec_write(unsigned int reg, unsigned short value)
{
    if (!ac97_codec_access())
        return 0;
    outportw(ac97.mixer + reg, value);
    return 1;
}

static int ac97_wait_engine(unsigned int reg, unsigned int mask,
                             unsigned int expected)
{
    uclock_t start = uclock();
    do
    {
        if ((inportb(ac97.busmaster + reg) & mask) == expected)
            return 1;
        ac97_wait();
    } while (uclock() - start < UCLOCKS_PER_SEC / 10);
    return 0;
}

/* ------------------------------------------------------------------------- */
/* Device lifecycle                                                         */
/* ------------------------------------------------------------------------- */

static void msdos_ac97_exit(void)
{
    int stopped = 1;
    if (ac97.quarantined)
        return;
    if (ac97.engine_owned)
    {
        outportb(ac97.busmaster + AC97_PO_CR, 0);
        stopped = ac97_wait_engine(AC97_PO_SR, AC97_SR_HALTED, AC97_SR_HALTED);
        /* Disable PCI bus mastering before releasing any physical memory,
         * even on a completion timeout. If BIOS refuses and DMA cannot be
         * stopped, retain the allocation rather than risk memory corruption.
         */
        if (!ac97_pci_command((ac97.command | AC97_PCI_IO) & ~AC97_PCI_MASTER) && !stopped)
        {
            SFX_DEBUG("msdos_ac97: cannot stop DMA; retaining DOS allocation");
            /* Keep the live descriptor address and allocation intact. This
             * process must not reuse an engine it could not safely stop.
             */
            ac97.quarantined = 1;
            ac97.active = 0;
            free(ac97.pcm);
            ac97.pcm = NULL;
            return;
        }
        outportb(ac97.busmaster + AC97_PO_CR, AC97_CR_RESET);
        if (!ac97_wait_engine(AC97_PO_CR, AC97_CR_RESET, 0))
            stopped = 0;
        outportl(ac97.busmaster + AC97_PO_BDBAR, 0);
        outportw(ac97.busmaster + AC97_PO_SR, AC97_SR_CLEAR);
        /* Restore rate before extended control, and power after volumes. */
        if (ac97.codec_count == 5)
        {
            /* A variable-rate codec accepts the saved DAC rate only while
             * VRA is enabled. Restore that enable before restoring its rate.
             */
            ac97_codec_write(0x2A, ac97.codec_saved[3]);
            ac97_codec_write(0x2C, ac97.codec_saved[4]);
            ac97_codec_write(0x02, ac97.codec_saved[0]);
            ac97_codec_write(0x18, ac97.codec_saved[1]);
            ac97_codec_write(0x26, ac97.codec_saved[2]);
        }
        outportl(ac97.busmaster + AC97_GLOBAL_CONTROL, ac97.global_control);
    }
    if (ac97.pci_owned)
        ac97_pci_command(stopped ? ac97.command : (ac97.command & ~AC97_PCI_MASTER));
    if (ac97.selector)
        __dpmi_free_dos_memory(ac97.selector);
    free(ac97.pcm);
    memset(&ac97, 0, sizeof(ac97));
}

static int msdos_ac97_init(int rate, int channels, int buffer, int flags)
{
    AC97_DESCRIPTOR descriptors[AC97_BDL_ENTRIES] = {{ 0, 0 }};
    unsigned short vendor1, vendor2, value;
    unsigned long command;
    uclock_t start;
    int segment, bytes, i;
    (void)rate;
    (void)flags;

    if (ac97.pci_owned || (channels != 1 && channels != 2))
        return -1;
    memset(&ac97, 0, sizeof(ac97));
    if (!ac97_find())
        return -1;
    ac97.pci_owned = 1;
    if (!ac97_pci_command(ac97.command | AC97_PCI_IO) ||
        !ac97_pci_read(ac97.pci, 4, &command) || !(command & AC97_PCI_IO))
        goto failed;
    /* Do not take a controller from another active DOS audio owner. The
     * three CR registers correspond to PCM input, PCM output and mic input.
     */
    if ((inportb(ac97.busmaster + 0x0B) |
         inportb(ac97.busmaster + AC97_PO_CR) |
         inportb(ac97.busmaster + 0x2B)) & AC97_CR_RUN)
        goto failed;
    ac97.global_control = inportl(ac97.busmaster + AC97_GLOBAL_CONTROL);
    if (ac97.global_control == 0xFFFFFFFFUL)
        goto failed;
    ac97.engine_owned = 1;
    /* Release cold reset, select two-channel/16-bit PCM. Avoid a whole-link
     * reset when firmware has already brought up the codec.
     */
    outportl(ac97.busmaster + AC97_GLOBAL_CONTROL,
             (ac97.global_control & ~0x00700000UL) | AC97_COLD_RELEASE);
    start = uclock();
    while (!(inportl(ac97.busmaster + AC97_GLOBAL_STATUS) & AC97_CODEC_READY))
    {
        if (uclock() - start >= UCLOCKS_PER_SEC)
            goto failed;
        ac97_wait();
    }
    if (!ac97_codec_read(0x7C, &vendor1) || !ac97_codec_read(0x7E, &vendor2) ||
        (!vendor1 && !vendor2))
        goto failed;
    for (i = 0; i < 5; ++i)
    {
        if (!ac97_codec_read(codec_registers[i], &ac97.codec_saved[i]))
            goto failed;
        ac97.codec_count++;
    }
    /* 48 kHz is mandatory on AC'97, including codecs without variable rate
     * support. Disable VRA/DRA so pitch never depends on firmware settings.
     */
    if (!ac97_codec_write(0x26, 0) ||
        !ac97_codec_write(0x2A, ac97.codec_saved[3] & ~3) ||
        !ac97_codec_write(0x2C, AC97_RATE) ||
        !ac97_codec_read(0x2A, &value) || (value & 3))
        goto failed;
    start = uclock();
    do
    {
        if (!ac97_codec_read(0x26, &value))
            goto failed;
        /* DAC, analog mixer and reference must all be powered and ready. */
        if ((value & 0x0E) == 0x0E)
            break;
        if (uclock() - start >= UCLOCKS_PER_SEC)
            goto failed;
        ac97_wait();
    } while (1);
    if (!ac97_codec_write(0x02, 0) || !ac97_codec_write(0x18, 0x0808))
        goto failed;

    ac97.frames = buffer > 0 ? buffer : 1024;
    if (ac97.frames > AC97_MAX_FRAMES)
        ac97.frames = AC97_MAX_FRAMES;
    ac97.channels = channels;
    bytes = AC97_BDL_ENTRIES * sizeof(AC97_DESCRIPTOR) + ac97.frames * 4;
    segment = __dpmi_allocate_dos_memory((bytes + 15) / 16, &ac97.selector);
    if (segment < 0)
    {
        ac97.selector = 0;
        goto failed;
    }
    ac97.physical = (unsigned long)segment << 4;
    ac97.pcm = malloc(ac97.frames * 4);
    if (!ac97.pcm)
        goto failed;
    /* Inactive descriptors carry zero length rather than stale DOS memory,
     * including entries adjacent to the current hardware prefetch window.
     */
    movedata(_go32_my_ds(), (unsigned)descriptors, ac97.selector, 0, sizeof(descriptors));
    outportb(ac97.busmaster + AC97_PO_CR, AC97_CR_RESET);
    if (!ac97_wait_engine(AC97_PO_CR, AC97_CR_RESET, 0))
        goto failed;
    outportw(ac97.busmaster + AC97_PO_SR, AC97_SR_CLEAR);
    outportl(ac97.busmaster + AC97_PO_BDBAR, ac97.physical);
    if (!ac97_pci_command(ac97.command | AC97_PCI_IO | AC97_PCI_MASTER) ||
        !ac97_pci_read(ac97.pci, 4, &command) || !(command & AC97_PCI_MASTER))
        goto failed;
    ac97.active = 1;
    if (__fb_sfx)
        __fb_sfx->samplerate = AC97_RATE;
    SFX_DEBUG("msdos_ac97: PCI %04X codec %04X:%04X, 48000 Hz stereo",
              ac97.pci, vendor1, vendor2);
    return 0;

failed:
    msdos_ac97_exit();
    return -1;
}

/* ------------------------------------------------------------------------- */
/* Blocking DMA output                                                      */
/* ------------------------------------------------------------------------- */

static short ac97_sample(float value)
{
    /* Reject NaN before the float-to-integer conversion; clamp infinities. */
    if (value != value)
        return 0;
    if (value >= 1.0f)
        return 32767;
    if (value <= -1.0f)
        return -32768;
    return (short)(value * 32767.0f);
}

static int msdos_ac97_write(const float *samples, int frames)
{
    int written = 0;
    if (!ac97.active || !samples || frames <= 0 || frames > INT_MAX / (int)sizeof(float) / ac97.channels)
        return -1;
    while (written < frames)
    {
        AC97_DESCRIPTOR descriptor;
        uclock_t start;
        int i, count = frames - written;
        unsigned short status;
        if (count > ac97.frames)
            count = ac97.frames;
        for (i = 0; i < count; ++i)
        {
            const float *source = samples + (written + i) * ac97.channels;
            ac97.pcm[i * 2] = ac97_sample(source[0]);
            ac97.pcm[i * 2 + 1] = ac97_sample(source[ac97.channels - 1]);
        }
        descriptor.address = ac97.physical + AC97_BDL_ENTRIES * sizeof(descriptor);
        descriptor.samples = count * 2;
        movedata(_go32_my_ds(), (unsigned)ac97.pcm, ac97.selector,
                 AC97_BDL_ENTRIES * sizeof(descriptor), count * 4);
        movedata(_go32_my_ds(), (unsigned)&descriptor, ac97.selector,
                 ac97.descriptor * sizeof(descriptor), sizeof(descriptor));
        /* LVI publishes the completed descriptor. Keep RUN set between
         * writes: extending LVI resumes an engine halted at its previous
         * last entry, without resetting the codec or DMA for every block.
         */
        outportw(ac97.busmaster + AC97_PO_SR, AC97_SR_CLEAR);
        outportb(ac97.busmaster + AC97_PO_LVI, ac97.descriptor);
        if (!ac97.started)
        {
            outportb(ac97.busmaster + AC97_PO_CR, AC97_CR_RUN);
            ac97.started = 1;
        }
        start = uclock();
        do
        {
            status = inportw(ac97.busmaster + AC97_PO_SR);
            if ((status & AC97_SR_FIFO_ERROR) || uclock() - start >= UCLOCKS_PER_SEC)
            {
                /* Stop immediately; the core will detach us and run exit
                 * before selecting the next driver. No fictitious success.
                 */
                outportb(ac97.busmaster + AC97_PO_CR, 0);
                ac97.active = 0;
                return -1;
            }
            if ((status & (AC97_SR_HALTED | AC97_SR_LAST_DONE)) ==
                (AC97_SR_HALTED | AC97_SR_LAST_DONE) &&
                inportb(ac97.busmaster + AC97_PO_CIV) == ac97.descriptor &&
                inportw(ac97.busmaster + AC97_PO_PICB) == 0)
                break;
            ac97_wait();
        } while (1);
        written += count;
        ac97.descriptor++;
        if (ac97.descriptor == AC97_BDL_ENTRIES)
            ac97.descriptor = 0;
    }
    return written;
}

const FB_SFX_DRIVER fb_sfxDriverAc97 =
{
    "AC97",
    FB_SFX_DRIVER_CAP_BLOCKING,
    msdos_ac97_init,
    msdos_ac97_exit,
    msdos_ac97_write,
    NULL,
    NULL,
    NULL,
    NULL
};

#endif

/* end of sfx_driver_ac97.c */

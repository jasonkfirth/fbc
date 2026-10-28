/*
    FreeBASIC gfxlib2 for classic AmigaOS
    -------------------------------------

    File: gfx_display.c

    Purpose:
        Present gfxlib2's software framebuffer through native Amiga graphics.

    Responsibilities:
        - own Intuition, graphics, and optional CyberGraphX library bases
        - use an RTG public screen when available or a private chipset screen
        - convert dirty rows to ARGB or planar-compatible palette indices
        - skip unchanged planar rows and invalidate them after window damage
        - release windows and screens before closing their supporting libraries

    This file intentionally does NOT contain:
        - input translation, drawing primitives, or direct chipset programming

    The RTG conversion follows the AROS backend. The chipset path uses the
    WritePixelLine8 scratch-RastPort model from FreeBASIC-NG's Amiga backend.
    A private screen owns its palette; changing the desktop palette would
    change unrelated applications. True-colour BASIC framebuffers are reduced
    to an exact private palette when possible, with a fixed RGB fallback for
    richer frames, without changing BASIC's pixels.
*/

#include "fb_gfx_amiga.h"

#include <cybergraphx/cybergraphics.h>
#include <exec/memory.h>
#include <graphics/displayinfo.h>
#include <graphics/gfxbase.h>
#include <graphics/modeid.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/cybergraphics.h>
#include <proto/graphics.h>
#include <proto/intuition.h>

#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* Classic library-vector inlines obtain their base from these globals.
   This module owns them, and every display/input call stays inside their
   open lifetime. gfxlib2 serializes foreground rendering through its lock. */
struct IntuitionBase *IntuitionBase;
struct GfxBase *GfxBase;
struct Library *CyberGfxBase;
FB_AMIGA_GFX_STATE fb_amiga_gfx;

static int amiga_open_libraries(void)
{
    IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library", 39);
    GfxBase = (struct GfxBase *)OpenLibrary("graphics.library", 39);
    CyberGfxBase = OpenLibrary("cybergraphics.library", 41);
    fb_amigaGfxDebug("libraries intuition=%p graphics=%p cyber=%p", IntuitionBase, GfxBase, CyberGfxBase);
    return IntuitionBase != NULL && GfxBase != NULL;
}

static void amiga_close_libraries(void)
{
    if (CyberGfxBase != NULL) CloseLibrary(CyberGfxBase);
    if (GfxBase != NULL) CloseLibrary((struct Library *)GfxBase);
    if (IntuitionBase != NULL) CloseLibrary((struct Library *)IntuitionBase);
    CyberGfxBase = NULL;
    GfxBase = NULL;
    IntuitionBase = NULL;
}

static void amiga_source_rgb(const unsigned char *source, int x,
    unsigned char *red, unsigned char *green, unsigned char *blue)
{
    unsigned int pixel;

    if (__fb_gfx->depth <= 8) {
        pixel = __fb_gfx->device_palette[source[x]];
        *red = (unsigned char)(pixel & 255U);
        *green = (unsigned char)((pixel >> 8) & 255U);
        *blue = (unsigned char)((pixel >> 16) & 255U);
    } else if (__fb_gfx->depth == 16) {
        pixel = ((const unsigned short *)source)[x];
        *red = (unsigned char)(((pixel >> 11) & 31U) * 255U / 31U);
        *green = (unsigned char)(((pixel >> 5) & 63U) * 255U / 63U);
        *blue = (unsigned char)((pixel & 31U) * 255U / 31U);
    } else {
        pixel = ((const unsigned int *)source)[x];
        *red = (unsigned char)((pixel >> 16) & 255U);
        *green = (unsigned char)((pixel >> 8) & 255U);
        *blue = (unsigned char)(pixel & 255U);
    }
}

void fb_amigaGfxSetPalette(int index, int red, int green, int blue)
{
    if (!fb_amiga_gfx.private_screen || fb_amiga_gfx.screen == NULL ||
        __fb_gfx == NULL || __fb_gfx->depth > 8 ||
        index < 0 || index >= (1 << fb_amiga_gfx.screen_depth))
        return;

    /* SetRGB32 uses the most significant bits of each component. */
    SetRGB32(&fb_amiga_gfx.screen->ViewPort, (ULONG)index,
        (ULONG)(red & 255) * 0x01010101UL,
        (ULONG)(green & 255) * 0x01010101UL,
        (ULONG)(blue & 255) * 0x01010101UL);
}

static unsigned char amiga_nearest_component(unsigned int value, int bits)
{
    unsigned int maximum, step, index;
    int best = 0, best_distance = 256;

    if (bits < 1 || bits > 3) return 0;
    maximum = (1U << bits) - 1U;
    step = 0xffffffffUL / maximum;
    for (index = 0; index <= maximum; ++index) {
        int component = (int)((index * step) >> 24);
        int distance = (int)value - component;
        if (distance < 0) distance = -distance;
        if (distance < best_distance) {
            best = (int)index;
            best_distance = distance;
        }
    }
    return (unsigned char)best;
}

static void amiga_set_truecolour_palette(void)
{
    int index;
    int red_bits = fb_amiga_gfx.screen_depth == 8 ? 3 : 2;
    int green_bits = red_bits;
    int blue_bits = fb_amiga_gfx.screen_depth == 8 ? 2 : 1;
    unsigned int green_mask = (1U << green_bits) - 1U;
    unsigned int blue_mask = (1U << blue_bits) - 1U;

    for (index = 0; index < (1 << fb_amiga_gfx.screen_depth); ++index) {
        ULONG red = (ULONG)(index >> (green_bits + blue_bits));
        ULONG green = (ULONG)((index >> blue_bits) & green_mask);
        ULONG blue = (ULONG)(index & blue_mask);

        SetRGB32(&fb_amiga_gfx.screen->ViewPort, (ULONG)index,
            red * (0xffffffffUL / ((1U << red_bits) - 1U)),
            green * (0xffffffffUL / green_mask),
            blue * (0xffffffffUL / blue_mask));
    }

    /* High-bit truncation chooses the wrong entry near palette boundaries.
       Select the closest actual level once, without divisions or allocation
       in the per-pixel presentation loop. Indexed images keep their own palette. */
    for (index = 0; index < 256; ++index) {
        fb_amiga_gfx.red_pen[index] =
            amiga_nearest_component((unsigned int)index, red_bits) << (green_bits + blue_bits);
        fb_amiga_gfx.green_pen[index] =
            amiga_nearest_component((unsigned int)index, green_bits) << blue_bits;
        fb_amiga_gfx.blue_pen[index] =
            amiga_nearest_component((unsigned int)index, blue_bits);
    }
}

/* ------------------------------------------------------------------------- */
/* True-colour private palettes                                              */
/* ------------------------------------------------------------------------- */

/* The graphics lock protects this display-owned state. At most 256 colours
   occupy the 1024-slot table, so a bounded linear probe always finds room.
   Slots store pen+1 because RGB black and pen zero are both valid values. */
#define AMIGA_EXACT_COLOURS 256
#define AMIGA_COLOUR_SLOTS 1024
static unsigned int amiga_exact_colours[AMIGA_EXACT_COLOURS];
static UWORD amiga_colour_slots[AMIGA_COLOUR_SLOTS];
static unsigned char *amiga_colour_pixels;
static int amiga_colour_count;
enum { AMIGA_PALETTE_FIXED, AMIGA_PALETTE_EXACT, AMIGA_PALETTE_DOMINANT };
static int amiga_palette_mode;

static int amiga_colour_pen(unsigned int colour)
{
    unsigned int slot = (colour ^ (colour >> 10) ^ (colour >> 20)) &
        (AMIGA_COLOUR_SLOTS - 1U);
    unsigned int probe;

    for (probe = 0; probe < AMIGA_COLOUR_SLOTS; ++probe) {
        unsigned int entry = amiga_colour_slots[slot];
        if (entry == 0) {
            int pen = amiga_colour_count;
            if (pen >= (1 << fb_amiga_gfx.screen_depth)) return -1;
            amiga_exact_colours[pen] = colour;
            amiga_colour_slots[slot] = (UWORD)(pen + 1);
            ++amiga_colour_count;
            return pen;
        }
        if (amiga_exact_colours[entry - 1U] == colour) return (int)entry - 1;
        slot = (slot + 1U) & (AMIGA_COLOUR_SLOTS - 1U);
    }
    return -1;
}

static void amiga_reset_colour_table(void)
{
    /* Intuition's default draw pens use the first four entries. Keep their
       light background, black detail, white shine and grey shadow stable.
       Source pixels may share these colours; indexed BASIC modes retain
       complete ownership of their original palette and never enter here. */
    static const unsigned int ui_colours[] = {
        0xaaaaaaU, 0x000000U, 0xffffffU, 0x555555U
    };
    unsigned int index;

    memset(amiga_colour_slots, 0, sizeof(amiga_colour_slots));
    amiga_colour_count = 0;
    /* Borderless displays have no window decorations. Reserve only black
       for the private screen's area outside the BASIC client framebuffer. */
    if (fb_amiga_gfx.window != NULL &&
        (fb_amiga_gfx.window->Flags & WFLG_BORDERLESS)) {
        amiga_colour_pen(0);
        return;
    }
    for (index = 0; index < sizeof(ui_colours) / sizeof(ui_colours[0]); ++index)
        amiga_colour_pen(ui_colours[index]);
}

static int amiga_colour_row_changed(int row)
{
    const unsigned char *source;
    if (__fb_gfx->dirty != NULL && !__fb_gfx->dirty[row]) return FALSE;
    if (fb_amiga_gfx.present_cache == NULL ||
        !fb_amiga_gfx.present_cache_valid[row]) return TRUE;
    source = __fb_gfx->framebuffer + (size_t)row * (size_t)__fb_gfx->pitch;
    return memcmp(source, fb_amiga_gfx.present_cache +
        (size_t)row * fb_amiga_gfx.present_cache_row_bytes,
        fb_amiga_gfx.present_cache_row_bytes) != 0;
}

static int amiga_collect_colour_row(int row)
{
    const unsigned char *source = __fb_gfx->framebuffer +
        (size_t)row * (size_t)__fb_gfx->pitch;
    unsigned char *pixels = amiga_colour_pixels +
        (size_t)row * (size_t)fb_amiga_gfx.width;
    int x;

    for (x = 0; x < fb_amiga_gfx.width; ++x) {
        unsigned char red, green, blue;
        unsigned int colour;
        int pen;
        amiga_source_rgb(source, x, &red, &green, &blue);
        colour = ((unsigned int)red << 16) | ((unsigned int)green << 8) | blue;
        pen = amiga_colour_pen(colour);
        if (pen < 0) return FALSE;
        pixels[x] = (unsigned char)pen;
    }
    return TRUE;
}

static void amiga_upload_exact_colours(int first)
{
    int index;
    for (index = first; index < amiga_colour_count; ++index) {
        unsigned int colour = amiga_exact_colours[index];
        SetRGB32(&fb_amiga_gfx.screen->ViewPort, (ULONG)index,
            (ULONG)((colour >> 16) & 255U) * 0x01010101UL,
            (ULONG)((colour >> 8) & 255U) * 0x01010101UL,
            (ULONG)(colour & 255U) * 0x01010101UL);
    }
}

/* Sparse antialiasing can introduce hundreds of shades around a few large
   flat regions. Keep those regions exact instead of reducing the entire UI
   to RGB332. Rich images still use the existing fixed palette: this path
   admits at most 512 colours, with four covering at least three quarters of
   the canvas. The 1024-slot hash remains at most half full. */
#define AMIGA_FRAME_COLOURS 512
struct amiga_frame_colour {
    unsigned int colour;
    size_t count;
    int distance;
    int pen;
};
static struct amiga_frame_colour amiga_frame_colours[AMIGA_FRAME_COLOURS];
static UWORD amiga_frame_slots[AMIGA_COLOUR_SLOTS];
static int amiga_frame_count;

static int amiga_frame_colour_index(unsigned int colour, int insert)
{
    unsigned int slot = (colour ^ (colour >> 10) ^ (colour >> 20)) &
        (AMIGA_COLOUR_SLOTS - 1U);
    unsigned int probe;

    for (probe = 0; probe < AMIGA_COLOUR_SLOTS; ++probe) {
        unsigned int entry = amiga_frame_slots[slot];
        if (entry == 0) {
            int index = amiga_frame_count;
            if (!insert || index >= AMIGA_FRAME_COLOURS) return -1;
            amiga_frame_colours[index].colour = colour;
            amiga_frame_colours[index].count = 0;
            amiga_frame_slots[slot] = (UWORD)(index + 1);
            ++amiga_frame_count;
            return index;
        }
        if (amiga_frame_colours[entry - 1U].colour == colour)
            return (int)entry - 1;
        slot = (slot + 1U) & (AMIGA_COLOUR_SLOTS - 1U);
    }
    return -1;
}

static int amiga_compare_colour_counts(const void *left, const void *right)
{
    const struct amiga_frame_colour *a = &amiga_frame_colours[*(const UWORD *)left];
    const struct amiga_frame_colour *b = &amiga_frame_colours[*(const UWORD *)right];
    if (a->count > b->count) return -1;
    if (a->count < b->count) return 1;
    /* RGB breaks frequency ties independently of libc's equal-key order. */
    if (a->colour < b->colour) return -1;
    if (a->colour > b->colour) return 1;
    return 0;
}

static int amiga_nearest_colour_pen(unsigned int colour, int *distance)
{
    /* Three squared 8-bit component differences fit a signed 32-bit int. */
    int best_distance = 3 * 255 * 255 + 1;
    int best = 0, pen;
    for (pen = 0; pen < amiga_colour_count; ++pen) {
        unsigned int candidate = amiga_exact_colours[pen];
        int red = (int)((colour >> 16) & 255U) -
            (int)((candidate >> 16) & 255U);
        int green = (int)((colour >> 8) & 255U) -
            (int)((candidate >> 8) & 255U);
        int blue = (int)(colour & 255U) - (int)(candidate & 255U);
        int distance = red * red + green * green + blue * blue;
        if (distance < best_distance) {
            best = pen;
            best_distance = distance;
        }
    }
    *distance = best_distance;
    return best;
}

static int amiga_prepare_dominant_colours(void)
{
    UWORD order[AMIGA_FRAME_COLOURS];
    size_t covered = 0;
    size_t total = (size_t)fb_amiga_gfx.width * (size_t)fb_amiga_gfx.height;
    int row, x, index;

    memset(amiga_frame_slots, 0, sizeof(amiga_frame_slots));
    amiga_frame_count = 0;
    /* The framebuffer allocation already bounds width*height to size_t.
       Each pixel belongs to one bucket, so bucket counts cannot overflow. */
    for (row = 0; row < fb_amiga_gfx.height; ++row) {
        const unsigned char *source = __fb_gfx->framebuffer +
            (size_t)row * (size_t)__fb_gfx->pitch;
        for (x = 0; x < fb_amiga_gfx.width; ++x) {
            unsigned char red, green, blue;
            unsigned int colour;
            amiga_source_rgb(source, x, &red, &green, &blue);
            colour = ((unsigned int)red << 16) | ((unsigned int)green << 8) | blue;
            index = amiga_frame_colour_index(colour, TRUE);
            if (index < 0) return FALSE;
            ++amiga_frame_colours[index].count;
        }
    }
    for (index = 0; index < amiga_frame_count; ++index) order[index] = (UWORD)index;
    qsort(order, (size_t)amiga_frame_count, sizeof(order[0]), amiga_compare_colour_counts);
    for (index = 0; index < amiga_frame_count && index < 4; ++index)
        covered += amiga_frame_colours[order[index]].count;
    if (covered < total - total / 4) return FALSE;

    amiga_reset_colour_table();
    for (index = 0; index < amiga_frame_count && index < 4; ++index)
        amiga_colour_pen(amiga_frame_colours[order[index]].colour);
    for (index = 0; index < amiga_frame_count; ++index)
        amiga_frame_colours[index].pen = amiga_nearest_colour_pen(
            amiga_frame_colours[index].colour, &amiga_frame_colours[index].distance);

    /* The four dominant regions are exact. Spend remaining pens on the
       largest remaining squared RGB error, weighted by pixel frequency.
       This also represents rare coloured text edges instead of consuming
       every pen with nearly identical grey shades. The checked 32-bit
       framebuffer size and 8-bit RGB distances keep the score in uint64_t. */
    while (amiga_colour_count < (1 << fb_amiga_gfx.screen_depth)) {
        uint64_t best_score = 0;
        int chosen = -1, pen;
        unsigned int colour;
        for (index = 0; index < amiga_frame_count; ++index) {
            uint64_t score = (uint64_t)amiga_frame_colours[index].distance *
                amiga_frame_colours[index].count;
            if (score > best_score || (score != 0 && score == best_score &&
                amiga_frame_colours[index].colour < amiga_frame_colours[chosen].colour)) {
                chosen = index;
                best_score = score;
            }
        }
        if (chosen < 0) break;
        colour = amiga_frame_colours[chosen].colour;
        pen = amiga_colour_pen(colour);
        if (pen < 0) return FALSE;
        for (index = 0; index < amiga_frame_count; ++index) {
            unsigned int candidate = amiga_frame_colours[index].colour;
            int red = (int)((colour >> 16) & 255U) -
                (int)((candidate >> 16) & 255U);
            int green = (int)((colour >> 8) & 255U) -
                (int)((candidate >> 8) & 255U);
            int blue = (int)(colour & 255U) - (int)(candidate & 255U);
            int distance = red * red + green * green + blue * blue;
            if (distance < amiga_frame_colours[index].distance) {
                amiga_frame_colours[index].distance = distance;
                amiga_frame_colours[index].pen = pen;
            }
        }
    }
    for (row = 0; row < fb_amiga_gfx.height; ++row) {
        const unsigned char *source = __fb_gfx->framebuffer +
            (size_t)row * (size_t)__fb_gfx->pitch;
        unsigned char *pixels = amiga_colour_pixels +
            (size_t)row * (size_t)fb_amiga_gfx.width;
        for (x = 0; x < fb_amiga_gfx.width; ++x) {
            unsigned char red, green, blue;
            unsigned int colour;
            amiga_source_rgb(source, x, &red, &green, &blue);
            colour = ((unsigned int)red << 16) | ((unsigned int)green << 8) | blue;
            index = amiga_frame_colour_index(colour, FALSE);
            if (index < 0) return FALSE;
            pixels[x] = (unsigned char)amiga_frame_colours[index].pen;
        }
    }
    return TRUE;
}


/* Convert changed rows once, before uploading any palette-dependent pixels.
   Append-only additions preserve old pen meanings. If that table fills, a
   full-frame scan discards unused old colours. A richer frame retains the
   existing nearest RGB fallback. Only a remapping forces unchanged rows to
   be uploaded again; the BASIC framebuffer and dirty/cache contracts stay
   separate from this native pen cache. */
static int amiga_prepare_colour_frame(void)
{
    int row, changed = FALSE;
    int was_private = amiga_palette_mode != AMIGA_PALETTE_FIXED;
    int first_new = amiga_colour_count;

    for (row = 0; row < fb_amiga_gfx.height; ++row)
        if (amiga_colour_row_changed(row)) { changed = TRUE; break; }
    if (!changed) return FALSE;

    if (amiga_palette_mode == AMIGA_PALETTE_EXACT) {
        for (row = 0; row < fb_amiga_gfx.height; ++row) {
            if (amiga_colour_row_changed(row) && !amiga_collect_colour_row(row))
                break;
        }
        if (row == fb_amiga_gfx.height) {
            amiga_upload_exact_colours(first_new);
            return FALSE;
        }
    }

    amiga_reset_colour_table();
    for (row = 0; row < fb_amiga_gfx.height; ++row) {
        if (!amiga_collect_colour_row(row)) {
            if (amiga_prepare_dominant_colours()) {
                amiga_upload_exact_colours(0);
                amiga_palette_mode = AMIGA_PALETTE_DOMINANT;
                return TRUE;
            }
            if (was_private) amiga_set_truecolour_palette();
            amiga_palette_mode = AMIGA_PALETTE_FIXED;
            return was_private;
        }
    }
    amiga_upload_exact_colours(0);
    amiga_palette_mode = AMIGA_PALETTE_EXACT;
    return TRUE;
}


static ULONG amiga_select_mode(int width, int height, int depth)
{
    struct DimensionInfo dimensions;
    struct DisplayInfo display;
    struct TagItem tags[] = {
        { BIDTAG_NominalWidth, (ULONG)width },
        { BIDTAG_NominalHeight, (ULONG)height },
        { BIDTAG_DesiredWidth, (ULONG)width },
        { BIDTAG_DesiredHeight, (ULONG)height },
        { BIDTAG_Depth, (ULONG)depth },
        { TAG_IGNORE, 0 },
        { TAG_DONE, 0 }
    };
    ULONG mode = BestModeIDA(tags), candidate = (ULONG)INVALID_ID;
    ULONG monitor = mode & MONITOR_ID_MASK, length;
    int max_width = 0, max_height = 0;
    unsigned int count;

    if (mode == (ULONG)INVALID_ID) return mode;
    length = GetDisplayInfoData(NULL, (UBYTE *)&dimensions,
        sizeof(dimensions), DTAG_DIMS, mode);
    if (length >= offsetof(struct DimensionInfo, Nominal) + sizeof(dimensions.Nominal) &&
        dimensions.Nominal.MaxX - dimensions.Nominal.MinX + 1 >= width &&
        dimensions.Nominal.MaxY - dimensions.Nominal.MinY + 1 >= height)
        return mode;

    /* BestModeID can select a tiny viewport when the desired dimensions
       exceed every physical mode. Keep the requested virtual bitmap, but
       bound the desired display dimensions to this monitor's actual modes.
       Nominal dimensions continue to describe the original aspect ratio. */
    for (count = 0; count < 65536U; ++count) {
        /* Normal display databases are much smaller; bound a corrupt or
           repeating enumeration without keeping an allocation-sized list. */
        ULONG next = NextDisplayInfo(candidate);
        int nominal_width, nominal_height;
        if (next == (ULONG)INVALID_ID || next == candidate) break;
        candidate = next;
        if ((candidate & MONITOR_ID_MASK) != monitor) continue;
        length = GetDisplayInfoData(NULL, (UBYTE *)&display,
            sizeof(display), DTAG_DISP, candidate);
        if (length < offsetof(struct DisplayInfo, PropertyFlags) + sizeof(display.PropertyFlags) ||
            display.NotAvailable != 0 || (display.PropertyFlags & SPECIAL_FLAGS) != 0)
            continue;
        length = GetDisplayInfoData(NULL, (UBYTE *)&dimensions,
            sizeof(dimensions), DTAG_DIMS, candidate);
        if (length < offsetof(struct DimensionInfo, Nominal) + sizeof(dimensions.Nominal) ||
            dimensions.MaxDepth < depth)
            continue;
        nominal_width = dimensions.Nominal.MaxX - dimensions.Nominal.MinX + 1;
        nominal_height = dimensions.Nominal.MaxY - dimensions.Nominal.MinY + 1;
        if (nominal_width > max_width) max_width = nominal_width;
        if (nominal_height > max_height) max_height = nominal_height;
    }
    if (max_width <= 0 || max_height <= 0 ||
        (width <= max_width && height <= max_height))
        return mode;
    tags[2].ti_Data = (ULONG)MIN(width, max_width);
    tags[3].ti_Data = (ULONG)MIN(height, max_height);
    tags[5].ti_Tag = BIDTAG_MonitorID;
    tags[5].ti_Data = monitor;
    candidate = BestModeIDA(tags);
    return candidate != (ULONG)INVALID_ID ? candidate : mode;
}

/* Palette ownership does not imply a borderless window. Intuition needs
   space for decorations around a requested client area on a private screen. */
static struct Screen *amiga_open_private_screen(ULONG mode_id, int width,
    int height, const char *title, ULONG *screen_error)
{
    return OpenScreenTags(NULL,
        SA_DisplayID, mode_id, SA_ErrorCode, (ULONG)screen_error,
        SA_Width, (ULONG)width, SA_Height, (ULONG)height,
        SA_Depth, (ULONG)fb_amiga_gfx.screen_depth,
        SA_Title, (ULONG)(title != NULL ? title : "FreeBASIC"),
        SA_ShowTitle, FALSE, SA_Quiet, TRUE, TAG_DONE);
}

int fb_amigaGfxDisplayInit(const char *title, int width, int height,
    int refresh_rate, int flags)
{
    struct Screen *desktop;
    int maximum_depth;
    int borderless = (flags & (DRIVER_FULLSCREEN | DRIVER_NO_FRAME)) != 0;
    ULONG mode_id = (ULONG)INVALID_ID, screen_error = 0;

    (void)refresh_rate;
    fb_amigaGfxDebug("opening %dx%d depth=%d", width, height, __fb_gfx != NULL ? __fb_gfx->depth : 0);
    memset(&fb_amiga_gfx, 0, sizeof(fb_amiga_gfx));

    /* Both pixel-transfer APIs use unsigned 16-bit dimensions and pitch.
       Conversion needs one scratch row. The optional planar cache below
       retains source bytes only to avoid uploading unchanged rows. */
    if (width <= 0 || height <= 0 || width > 16383 || height > 32767 ||
        __fb_gfx == NULL || !amiga_open_libraries())
        goto fail;

    desktop = LockPubScreen(NULL);
    if (desktop != NULL && CyberGfxBase != NULL &&
        GetCyberMapAttr(desktop->RastPort.BitMap, CYBRMATTR_ISCYBERGFX) &&
        !(flags & DRIVER_FULLSCREEN)) {
        fb_amiga_gfx.screen = desktop;
        fb_amiga_gfx.rtg = TRUE;
    } else {
        if (desktop != NULL) UnlockPubScreen(NULL, desktop);
        /* AA_LISA describes enabled AGA modes. The internal MLISA bit may
           be set before SetPatch enables those modes, so it cannot establish
           the depth accepted by graphics.library's display database. */
        maximum_depth = (GfxBase->ChipRevBits0 & GFXF_AA_LISA) ? 8 : 5;
        fb_amigaGfxDebug("chip revision=%u maximum depth=%d", GfxBase->ChipRevBits0, maximum_depth);
        fb_amiga_gfx.screen_depth = __fb_gfx->depth <= 8
            ? MIN(__fb_gfx->depth, maximum_depth) : maximum_depth;
        if (__fb_gfx->depth <= 8 && __fb_gfx->depth > maximum_depth)
            goto fail;
        mode_id = amiga_select_mode(width, height, fb_amiga_gfx.screen_depth);
        fb_amigaGfxDebug("selected mode=%lx", mode_id);
        if (mode_id == (ULONG)INVALID_ID) goto fail;
        fb_amiga_gfx.screen = amiga_open_private_screen(mode_id, width,
            height, title, &screen_error);
        fb_amigaGfxDebug("mode=%lx screen error=%lu", mode_id, screen_error);
        fb_amiga_gfx.private_screen = TRUE;
    }
    if (fb_amiga_gfx.screen == NULL) goto fail;
    fb_amigaGfxDebug("screen opened");

    if (fb_amiga_gfx.private_screen && !borderless) {
        struct Screen *screen = fb_amiga_gfx.screen;
        int border_width, border_height;

        /* screens.h defines a window title's height as WBorTop plus the
           screen font height plus one. Use the opened screen's actual
           metrics, then reopen the same mode with a larger virtual bitmap.
           Keeping the mode preserves its font and border geometry. */
        if (screen->WBorLeft < 0 || screen->WBorRight < 0 ||
            screen->WBorTop < 0 || screen->WBorBottom < 0 ||
            screen->RastPort.TxHeight <= 0)
            goto fail;
        border_width = screen->WBorLeft + screen->WBorRight;
        border_height = screen->WBorTop + screen->RastPort.TxHeight + 1 +
            screen->WBorBottom;
        /* Intuition stores screen and window dimensions in signed WORDs. */
        if (width > SHRT_MAX - border_width ||
            height > SHRT_MAX - border_height)
            goto fail;
        if (!CloseScreen(screen)) goto fail;
        fb_amiga_gfx.screen = amiga_open_private_screen(mode_id,
            width + border_width, height + border_height, title, &screen_error);
        if (fb_amiga_gfx.screen == NULL) goto fail;
    }

    fb_amiga_gfx.window = OpenWindowTags(NULL,
        WA_CustomScreen, (ULONG)fb_amiga_gfx.screen,
        WA_InnerWidth, (ULONG)width, WA_InnerHeight, (ULONG)height,
        /* A title alone reserves a top border even with WA_Borderless.
           Omit it for borderless modes so their input and pixel origins agree. */
        WA_Title, (ULONG)(borderless ? NULL : (title != NULL ? title : "FreeBASIC")),
        WA_Borderless, (ULONG)borderless,
        WA_DragBar, (ULONG)!borderless,
        WA_DepthGadget, (ULONG)!borderless,
        WA_CloseGadget, (ULONG)!borderless,
        WA_Activate, TRUE, WA_RMBTrap, TRUE, WA_ReportMouse, TRUE,
        WA_IDCMP, IDCMP_CLOSEWINDOW | IDCMP_RAWKEY | IDCMP_MOUSEBUTTONS |
            IDCMP_MOUSEMOVE | IDCMP_ACTIVEWINDOW | IDCMP_INACTIVEWINDOW |
            IDCMP_REFRESHWINDOW, TAG_DONE);
    if (fb_amiga_gfx.window == NULL) goto fail;
    /* Auto-adjusted windows must not silently clip the BASIC client area. */
    if (fb_amiga_gfx.window->Width - fb_amiga_gfx.window->BorderLeft -
            fb_amiga_gfx.window->BorderRight < width ||
        fb_amiga_gfx.window->Height - fb_amiga_gfx.window->BorderTop -
            fb_amiga_gfx.window->BorderBottom < height)
        goto fail;
    fb_amigaGfxDebug("window opened");

    /* One transparent sprite row needs two control words, two image words,
       and two terminating words. Intuition requires DMA-accessible chip RAM
       and may retain the data while the window is inactive. */
    fb_amiga_gfx.blank_pointer = AllocMem(6 * sizeof(UWORD), MEMF_CHIP | MEMF_CLEAR);
    if (fb_amiga_gfx.blank_pointer == NULL) goto fail;

    /* WritePixelLine8 needs a one-row temporary bitmap with a word-aligned
       planar pitch. AllocBitMap performs that alignment, including odd widths. */
    fb_amiga_gfx.present_buffer_size = ((size_t)width + 15U) & ~(size_t)15U;
    if (fb_amiga_gfx.rtg)
        fb_amiga_gfx.present_buffer_size *= 4U;
    else {
        fb_amiga_gfx.line_bitmap = AllocBitMap((ULONG)width, 1,
            (ULONG)fb_amiga_gfx.screen_depth, BMF_CLEAR | BMF_DISPLAYABLE, NULL);
        if (fb_amiga_gfx.line_bitmap == NULL) goto fail;
        InitRastPort(&fb_amiga_gfx.line_rastport);
        fb_amiga_gfx.line_rastport.BitMap = fb_amiga_gfx.line_bitmap;
        if (__fb_gfx->depth > 8) amiga_set_truecolour_palette();
    }
    fb_amiga_gfx.present_buffer = malloc(fb_amiga_gfx.present_buffer_size);
    if (fb_amiga_gfx.present_buffer == NULL) goto fail;
    memset(fb_amiga_gfx.present_buffer, 0, fb_amiga_gfx.present_buffer_size);
    fb_amiga_gfx.width = width;
    fb_amiga_gfx.height = height;
    if (!fb_amiga_gfx.rtg && __fb_gfx->depth > 8) {
        /* This optional byte-per-pixel cache avoids a second RGB conversion.
           Allocation failure keeps the existing fixed-palette renderer. */
        amiga_colour_pixels = malloc((size_t)width * (size_t)height);
    }
    if (!fb_amiga_gfx.rtg) {
        size_t row_bytes = (size_t)width * (size_t)__fb_gfx->bpp;

        /* PALETTE marks every software row dirty even when SetRGB32 has
           already changed the displayed colours. Compare actual source
           bytes so palette updates and repeated frames need no planar
           transfer. Allocation failure only disables this optimization.
           The mode bounds above keep this product within 32-bit size_t. */
        fb_amiga_gfx.present_cache = malloc((row_bytes + 1U) * (size_t)height);
        if (fb_amiga_gfx.present_cache != NULL) {
            fb_amiga_gfx.present_cache_row_bytes = row_bytes;
            fb_amiga_gfx.present_cache_valid = fb_amiga_gfx.present_cache +
                row_bytes * (size_t)height;
            memset(fb_amiga_gfx.present_cache_valid, FALSE, (size_t)height);
        }
    }
    fb_amiga_gfx.refresh_rate = GfxBase->DisplayFlags & PAL ? 50 : 60;
    fb_amiga_gfx.cursor_visible = TRUE;
    fb_amiga_gfx.active = TRUE;
    fb_amigaGfxDebug("display ready");
    ScreenToFront(fb_amiga_gfx.screen);
    return 0;

fail:
    fb_amigaGfxDebug("display setup failed, DOS error=%ld", IoErr());
    fb_amigaGfxDisplayExit();
    return -1;
}

void fb_amigaGfxDisplayExit(void)
{
    fb_amigaGfxDebug("closing display");
    fb_amiga_gfx.active = FALSE;
    if (fb_amiga_gfx.window != NULL) CloseWindow(fb_amiga_gfx.window);
    if (fb_amiga_gfx.blank_pointer != NULL)
        FreeMem(fb_amiga_gfx.blank_pointer, 6 * sizeof(UWORD));
    if (fb_amiga_gfx.line_bitmap != NULL) FreeBitMap(fb_amiga_gfx.line_bitmap);
    if (fb_amiga_gfx.screen != NULL) {
        if (fb_amiga_gfx.private_screen) CloseScreen(fb_amiga_gfx.screen);
        else UnlockPubScreen(NULL, fb_amiga_gfx.screen);
    }
    free(fb_amiga_gfx.present_buffer);
    free(fb_amiga_gfx.present_cache);
    free(amiga_colour_pixels);
    amiga_colour_pixels = NULL;
    amiga_colour_count = 0;
    amiga_palette_mode = AMIGA_PALETTE_FIXED;
    memset(&fb_amiga_gfx, 0, sizeof(fb_amiga_gfx));
    amiga_close_libraries();
}

void fb_amigaGfxInvalidate(void)
{
    if (fb_amiga_gfx.present_cache_valid != NULL)
        memset(fb_amiga_gfx.present_cache_valid, FALSE, (size_t)fb_amiga_gfx.height);
}

void fb_amigaGfxPresent(void)
{
    int row, force_all = FALSE;

    if (!fb_amiga_gfx.active || __fb_gfx == NULL ||
        __fb_gfx->framebuffer == NULL || fb_amiga_gfx.present_buffer == NULL)
        return;

    fb_amigaGfxDebug("present begin");
    if (!fb_amiga_gfx.rtg && __fb_gfx->depth > 8 && amiga_colour_pixels != NULL)
        force_all = amiga_prepare_colour_frame();

    for (row = 0; row < fb_amiga_gfx.height; ++row) {
        const unsigned char *source;
        unsigned char *cached = NULL;
        unsigned char *destination = fb_amiga_gfx.present_buffer;
        int x;

        if (!force_all && __fb_gfx->dirty != NULL && !__fb_gfx->dirty[row]) continue;
        source = __fb_gfx->framebuffer + (size_t)row * (size_t)__fb_gfx->pitch;
        if (fb_amiga_gfx.present_cache != NULL) {
            cached = fb_amiga_gfx.present_cache +
                (size_t)row * fb_amiga_gfx.present_cache_row_bytes;
            if (!force_all && fb_amiga_gfx.present_cache_valid[row] &&
                memcmp(cached, source, fb_amiga_gfx.present_cache_row_bytes) == 0) {
                if (__fb_gfx->dirty != NULL) __fb_gfx->dirty[row] = FALSE;
                continue;
            }
        }
        if (amiga_palette_mode != AMIGA_PALETTE_FIXED) {
            memcpy(destination, amiga_colour_pixels +
                (size_t)row * (size_t)fb_amiga_gfx.width,
                (size_t)fb_amiga_gfx.width);
        } else {
            for (x = 0; x < fb_amiga_gfx.width; ++x) {
                unsigned char red, green, blue;

                if (!fb_amiga_gfx.rtg && __fb_gfx->depth <= 8) {
                    destination[x] = source[x];
                    continue;
                }
                amiga_source_rgb(source, x, &red, &green, &blue);
                if (fb_amiga_gfx.rtg) {
                    *destination++ = 255;
                    *destination++ = red;
                    *destination++ = green;
                    *destination++ = blue;
                } else
                    destination[x] = fb_amiga_gfx.red_pen[red] |
                        fb_amiga_gfx.green_pen[green] | fb_amiga_gfx.blue_pen[blue];
            }
        }
        if (fb_amiga_gfx.rtg)
            WritePixelArray(fb_amiga_gfx.present_buffer, 0, 0,
                (UWORD)(fb_amiga_gfx.width * 4), fb_amiga_gfx.window->RPort,
                fb_amiga_gfx.window->BorderLeft,
                (UWORD)(fb_amiga_gfx.window->BorderTop + row),
                (UWORD)fb_amiga_gfx.width, 1, RECTFMT_ARGB);
        else {
            LONG written = WritePixelLine8(fb_amiga_gfx.window->RPort,
                (ULONG)fb_amiga_gfx.window->BorderLeft,
                (ULONG)(fb_amiga_gfx.window->BorderTop + row),
                (ULONG)fb_amiga_gfx.width, fb_amiga_gfx.present_buffer,
                &fb_amiga_gfx.line_rastport);
            /* An incomplete native transfer must remain dirty for retry. */
            if (written != fb_amiga_gfx.width) {
                /* A palette remap may also upload rows that were not dirty. */
                if (__fb_gfx->dirty != NULL) __fb_gfx->dirty[row] = TRUE;
                if (fb_amiga_gfx.present_cache_valid != NULL)
                    fb_amiga_gfx.present_cache_valid[row] = FALSE;
                continue;
            }
        }
        if (cached != NULL) {
            memcpy(cached, source, fb_amiga_gfx.present_cache_row_bytes);
            fb_amiga_gfx.present_cache_valid[row] = TRUE;
        }
        if (__fb_gfx->dirty != NULL) __fb_gfx->dirty[row] = FALSE;
    }
    fb_amigaGfxDebug("present end");
}

void fb_amigaGfxWaitVSync(void)
{
    if (GfxBase != NULL) WaitTOF();
}

void fb_amigaGfxSetWindowTitle(char *title)
{
    if (fb_amiga_gfx.window != NULL && title != NULL) {
        SetWindowTitles(fb_amiga_gfx.window, title, (UBYTE *)~0UL);
        /* Intuition can repaint a title strip even on our borderless window.
           Its pixels belong to the BASIC canvas on a private screen. Restore
           them immediately, including rows whose source bytes did not change.
           RTG public-screen windows retain their ordinary native decorations. */
        if (fb_amiga_gfx.private_screen) {
            fb_amigaGfxInvalidate();
            if (__fb_gfx != NULL && __fb_gfx->dirty != NULL)
                memset(__fb_gfx->dirty, TRUE, (size_t)fb_amiga_gfx.height);
            fb_amigaGfxPresent();
        }
    }
}

int fb_amigaGfxSetWindowPosition(int x, int y)
{
    struct Window *window = fb_amiga_gfx.window;

    if (window == NULL) return 0;
    if (!fb_amiga_gfx.private_screen)
        MoveWindow(window, x == INT_MIN ? 0 : x - window->LeftEdge,
            y == INT_MIN ? 0 : y - window->TopEdge);
    /* Cast before shifting: off-screen negative coordinates are valid. */
    return (int)((unsigned int)(UWORD)window->LeftEdge |
        ((unsigned int)(UWORD)window->TopEdge << 16));
}

void fb_amigaGfxReadScreenInfo(ssize_t *width, ssize_t *height,
    ssize_t *depth, ssize_t *refresh)
{
    struct Screen *screen;
    int opened = IntuitionBase == NULL;

    *width = *height = *depth = *refresh = 0;
    if (opened && !amiga_open_libraries()) {
        amiga_close_libraries();
        return;
    }
    screen = fb_amiga_gfx.screen != NULL ? fb_amiga_gfx.screen : LockPubScreen(NULL);
    if (screen != NULL) {
        *width = screen->Width;
        *height = screen->Height;
        *depth = GetBitMapAttr(screen->RastPort.BitMap, BMA_DEPTH);
        *refresh = GfxBase->DisplayFlags & PAL ? 50 : 60;
        if (screen != fb_amiga_gfx.screen) UnlockPubScreen(NULL, screen);
    }
    if (opened) amiga_close_libraries();
}

/* end of gfx_display.c */

/*
 * FreeBASIC external-library qualification
 * File: pdflib-native.c
 * Purpose: Exercise PDFlib 4.0.2 through its original C declarations.
 * Responsibilities: Native widths, allocator ownership and PDF output APIs.
 * This file contains no substitute PDF generator or FreeBASIC binding.
 */
#include "pdflib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* The test is single-threaded. PDFlib calls the first allocator with a null
 * object, before opaque state exists; this counter belongs only to this test.
 */
static size_t live_blocks, allocations, output_bytes;
static FILE *sink;
static int marker;

static void error_handler(PDF *p, int type, const char *message)
{
    (void)p;
    if (type == PDF_NonfatalError) return;
    fprintf(stderr, "PDFlib error %d: %s\n", type, message);
    /* The vendor requires fatal handlers to unwind or terminate, not return. */
    exit(2);
}

static void *allocate_block(PDF *p, size_t bytes, const char *caller)
{
    void *block;
    (void)p; (void)caller;
    block = malloc(bytes);
    if (!block) exit(3);
    ++live_blocks;
    ++allocations;
    return block;
}

static void *resize_block(PDF *p, void *old, size_t bytes, const char *caller)
{
    void *block;
    (void)p; (void)caller;
    if (!old) return allocate_block(p, bytes, caller);
    if (!bytes) exit(4);
    block = realloc(old, bytes);
    if (!block) exit(3);
    return block;
}

static void release_block(PDF *p, void *block)
{
    (void)p;
    if (!block) return;
    if (!live_blocks) exit(5);
    --live_blocks;
    free(block);
}

static size_t write_output(PDF *p, void *data, size_t bytes)
{
    size_t written;
    if (PDF_get_opaque(p) != &marker || !sink) exit(6);
    written = fwrite(data, 1, bytes, sink);
    output_bytes += written;
    return written;
}

static PDF *new_document(void)
{
    PDF *p = PDF_new2(error_handler, allocate_block, resize_block,
                      release_block, &marker);
    if (!p || PDF_get_opaque(p) != &marker) exit(7);
    return p;
}

static void page_contents(PDF *p, const char *logo)
{
    int font, image;
    PDF_set_value(p, "compress", 0);
    PDF_begin_page(p, 320, 240);
    font = PDF_findfont(p, "Helvetica", "winansi", 0);
    if (font < 0) exit(8);
    PDF_setfont(p, font, 14);
    PDF_show_xy(p, "PDFlib native ABI", 20, 200);
    PDF_rect(p, 20, 20, 280, 200);
    PDF_stroke(p);
    if (PDF_open_image_file(p, "gif", "absent-pdflib-image.gif", "", 0) != -1) exit(9);
    image = PDF_open_image_file(p, "gif", logo, "", 0);
    if (image < 0 || PDF_get_value(p, "imagewidth", image) <= 0) exit(10);
    PDF_place_image(p, image, 20, 80, 1);
    PDF_close_image(p, image);
    PDF_end_page(p);
    PDF_close(p);
}

int main(int argc, char **argv)
{
    PDF *p;
    int mode;
    printf("PDFLIB_ABI=%zu,%zu,%zu,%zu,%zu,%d,%d\n", sizeof(void *),
           sizeof(int), sizeof(long), sizeof(size_t), sizeof(float),
           PDF_get_majorversion(), PDF_get_minorversion());
    if (argc == 1) return 0;
    if (argc != 4) return 1;
    mode = atoi(argv[3]);
    p = new_document();
    if (PDF_open_file(p, "missing-pdflib-parent/denied.pdf") != -1) return 11;
    PDF_delete(p);
    if (live_blocks) return 12;
    p = new_document();
    /* A failed open also permits retrying the same object. */
    if (PDF_open_file(p, "missing-pdflib-parent/retry.pdf") != -1) return 11;
    if (mode == 1) {
        sink = fopen(argv[1], "wb");
        if (!sink) return 13;
        PDF_open_mem(p, write_output);
    } else if (PDF_open_file(p, mode == 2 ? "" : argv[1]) != 1) {
        return 14;
    }
    page_contents(p, argv[2]);
    if (mode == 2) {
        /* The buffer belongs to p and expires on the next PDFlib operation. */
        long bytes = 0;
        const char *data = PDF_get_buffer(p, &bytes);
        FILE *file = fopen(argv[1], "wb");
        if (!file || !data || bytes < 8 || memcmp(data, "%PDF-", 5)) return 15;
        if (fwrite(data, 1, (size_t)bytes, file) != (size_t)bytes || fclose(file)) return 16;
    }
    if (mode == 1 && (fclose(sink) || output_bytes < 8)) return 17;
    PDF_delete(p);
    if (live_blocks || !allocations) return 18;
    printf("PDFLIB_DOCUMENT=PASS mode=%d owned_blocks=0\n", mode);
    return 0;
}
/* end of pdflib-native.c */

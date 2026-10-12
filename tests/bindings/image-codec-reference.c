/*
    Project: FreeBASIC native image qualification
    File: image-codec-reference.c
    Purpose: Create small JPEG fixtures and export the SDK's native decoded pixels.
    Responsibilities: Real encoder/decoder calls, odd-width rows and cleanup.
    This file contains no substitute codec or FreeBASIC wrapper implementation.
*/
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include <jpeglib.h>

struct decode_state {
    struct jpeg_decompress_struct info;
    struct jpeg_error_mgr error;
    jmp_buf jump;
    FILE *input, *output;
    JSAMPROW row;
    int created;
};

static void decode_error(j_common_ptr info)
{
    struct decode_state *state = info->client_data;
    longjmp(state->jump, 1);
}

static int decode(const char *filename, const char *destination)
{
    struct decode_state *state = calloc(1, sizeof(*state));
    int result = 1;
    uint32_t width, height;
    if (!state) return 1;
    state->input = fopen(filename, "rb");
    if (!state->input) goto cleanup;
    state->info.err = jpeg_std_error(&state->error);
    state->error.error_exit = decode_error;
    state->info.client_data = state;
    if (setjmp(state->jump)) goto cleanup;
    state->created = 1;
    jpeg_create_decompress(&state->info);
    jpeg_stdio_src(&state->info, state->input);
    if (jpeg_read_header(&state->info, TRUE) != JPEG_HEADER_OK) goto cleanup;
    if ((uint64_t)state->info.image_width * state->info.image_height > 16*1024*1024) goto cleanup;
    if (state->info.jpeg_color_space == JCS_CMYK || state->info.jpeg_color_space == JCS_YCCK) goto cleanup;
    state->info.out_color_space = JCS_RGB;
    if (!jpeg_start_decompress(&state->info) || state->info.output_components != 3) goto cleanup;
    width = state->info.output_width;
    height = state->info.output_height;
    state->row = malloc((size_t)width*3);
    if (!state->row) goto cleanup;
    state->output = fopen(destination, "wb");
    if (!state->output) goto cleanup;
    /* Qualification pixels: little-endian uint32 width/height, then top-down BGRA. */
    if (fwrite(&width, 4, 1, state->output) != 1 || fwrite(&height, 4, 1, state->output) != 1) goto cleanup;
    while (state->info.output_scanline < height) {
        uint32_t x;
        if (jpeg_read_scanlines(&state->info, &state->row, 1) != 1) goto cleanup;
        for (x=0; x<width; x++) {
            unsigned char pixel[4] = {state->row[x*3+2],state->row[x*3+1],state->row[x*3],255};
            if (fwrite(pixel, 4, 1, state->output) != 1) goto cleanup;
        }
    }
    if (!jpeg_finish_decompress(&state->info)) goto cleanup;
    result = 0;
cleanup:
    free(state->row);
    if (state->created) jpeg_destroy_decompress(&state->info);
    if (state->input && fclose(state->input)) result = 1;
    if (state->output && fclose(state->output)) result = 1;
    free(state);
    return result;
}

static int fixture(const char *filename, char mode)
{
    struct jpeg_compress_struct info;
    struct jpeg_error_mgr error;
    unsigned char row[17*4];
    JSAMPROW rows[1] = {row};
    FILE *file;
    int channels, x, y;
    channels = mode == 'g' ? 1 : (mode == 'c' ? 4 : 3);
    file = fopen(filename, "wb");
    if (!file) return 1;
    info.err = jpeg_std_error(&error);
    jpeg_create_compress(&info);
    jpeg_stdio_dest(&info, file);
    info.image_width = 17;
    info.image_height = 9;
    info.input_components = channels;
    info.in_color_space = channels == 1 ? JCS_GRAYSCALE : (channels == 4 ? JCS_CMYK : JCS_RGB);
    jpeg_set_defaults(&info);
    jpeg_set_quality(&info, 100, TRUE);
    for (x=0; x<info.num_components; x++) {
        info.comp_info[x].h_samp_factor = 1;
        info.comp_info[x].v_samp_factor = 1;
    }
    jpeg_start_compress(&info, TRUE);
    for (y=0; y<9; y++) {
        for (x=0; x<17; x++) {
            if (channels == 1) row[x] = (unsigned char)(x*17+y*29);
            else {
                row[x*channels] = (unsigned char)(x*11+y*19);
                row[x*channels+1] = (unsigned char)(x*7+y*37);
                row[x*channels+2] = (unsigned char)(x*29+y*3);
                if (channels==4) row[x*channels+3] = 17;
            }
        }
        if (jpeg_write_scanlines(&info, rows, 1) != 1) {
            jpeg_destroy_compress(&info);
            fclose(file);
            return 1;
        }
    }
    jpeg_finish_compress(&info);
    jpeg_destroy_compress(&info);
    return fclose(file) != 0;
}

int main(int argc, char **argv)
{
    if (argc == 4 && !strcmp(argv[2], "decode")) return decode(argv[1], argv[3]);
    if (argc == 3 && strchr("grc", argv[2][0])) return fixture(argv[1], argv[2][0]);
    return 1;
}
/* end of image-codec-reference.c */

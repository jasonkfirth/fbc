/*
    Project: FreeBASIC native text/2D qualification
    File: freetype-cairo.c
    Purpose: Compare native layouts and generate real SDK raster references.
    Responsibilities: Genuine FreeType glyphs, Cairo drawing and owned buffers.
    This file contains no substitute font engine or renderer.
*/
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <cairo.h>

static int save(const char *filename, unsigned char *pixels, uint32_t width, uint32_t height)
{
    FILE *file = fopen(filename, "wb");
    int failed;
    if (!file) return 1;
    failed = fwrite(&width,4,1,file)!=1 || fwrite(&height,4,1,file)!=1 ||
             fwrite(pixels,4,(size_t)width*height,file)!=(size_t)width*height;
    return fclose(file)!=0 || failed;
}
int main(int argc, char **argv)
{
    FT_Library library = NULL;
    FT_Face face = NULL;
    unsigned char *pixels = NULL;
    cairo_surface_t *surface = NULL;
    cairo_t *context = NULL;
    char filename[4096];
    int major, minor, patch, result = 1;
    if (argc != 3) return 1;
    if (FT_Init_FreeType(&library)) goto cleanup;
    FT_Library_Version(library,&major,&minor,&patch);
    printf("VERSIONS FT=%d.%d.%d CAIRO=%s\n",major,minor,patch,cairo_version_string());
    printf("FT %zu %zu %zu %zu %zu %zu %zu %zu %zu %zu %zu %zu %zu\n",
        sizeof(FT_Long),sizeof(FT_Pos),sizeof(FT_Bitmap),offsetof(FT_Bitmap,buffer),
        offsetof(FT_Bitmap,pitch),sizeof(FT_FaceRec),offsetof(FT_FaceRec,glyph),
        sizeof(FT_GlyphSlotRec),offsetof(FT_GlyphSlotRec,bitmap),
        offsetof(FT_GlyphSlotRec,bitmap_left),offsetof(FT_GlyphSlotRec,bitmap_top),
        offsetof(FT_GlyphSlotRec,advance),sizeof(FT_SizeRec));
    printf("CAIRO %zu %zu %zu\n",sizeof(cairo_matrix_t),sizeof(cairo_rectangle_int_t),sizeof(cairo_glyph_t));
    if (FT_New_Face(library,argv[1],0,&face) || FT_Set_Pixel_Sizes(face,0,200) ||
        FT_Load_Char(face,'@',FT_LOAD_DEFAULT) || FT_Render_Glyph(face->glyph,FT_RENDER_MODE_NORMAL)) goto cleanup;
    pixels = calloc(320*200,4);
    if (!pixels) goto cleanup;
    for (unsigned int y=0;y<face->glyph->bitmap.rows && y<200;y++)
        for (unsigned int x=0;x<face->glyph->bitmap.width && x<320;x++) {
            unsigned char value=face->glyph->bitmap.buffer[y*face->glyph->bitmap.pitch+x];
            size_t position=((size_t)y*320+x)*4;
            pixels[position]=pixels[position+1]=pixels[position+2]=value;
        }
    snprintf(filename,sizeof(filename),"%s/glyph.bgra",argv[2]);
    if (save(filename,pixels,320,200)) goto cleanup;
    free(pixels); pixels=calloc(400*300,4);
    if (!pixels) goto cleanup;
    surface=cairo_image_surface_create_for_data(pixels,CAIRO_FORMAT_ARGB32,400,300,1600);
    if (cairo_surface_status(surface)) goto cleanup;
    context=cairo_create(surface);
    if (cairo_status(context)) goto cleanup;
    cairo_set_source_rgba(context,1,1,1,1); cairo_paint(context);
    cairo_set_line_width(context,1); cairo_set_source_rgba(context,1,0,0,1);
    cairo_move_to(context,0,0); cairo_line_to(context,399,299); cairo_stroke(context);
    cairo_surface_flush(surface);
    if (cairo_status(context)) goto cleanup;
    snprintf(filename,sizeof(filename),"%s/cairo.bgra",argv[2]);
    if (save(filename,pixels,400,300)) goto cleanup;
    result=0;
cleanup:
    if (context) cairo_destroy(context);
    if (surface) cairo_surface_destroy(surface);
    free(pixels);
    if (face) FT_Done_Face(face);
    if (library) FT_Done_FreeType(library);
    return result;
}
/* end of freetype-cairo.c */

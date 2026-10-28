/*
    FreeBASIC file-size regression bridge
    File: file-size-bridge.c
    Purpose: Count real seek callbacks and append through an independent stream.
    Responsibilities: Forward every original hook and report actual counts.
    This file does not fake lengths, alter cursors or return replacement data.
*/
#include "../../src/rtlib/fb.h"
#include <stdio.h>
static int seek_calls;
extern int __real_fb_DevFileSeek(FB_FILE *,fb_off_t,int);
int __wrap_fb_DevFileSeek(FB_FILE *file,fb_off_t position,int origin)
{
    ++seek_calls;
    return __real_fb_DevFileSeek(file,position,origin);
}
void audit_reset(void) { seek_calls=0; }
int audit_moves(void) { return seek_calls; }
int append_byte(const char *name)
{
    FILE *stream=fopen(name,"ab");
    int result;
    if(stream==NULL)return 1;
    result=fputc('Z',stream)==EOF;
    if(fclose(stream)!=0)result=1;
    return result;
}
/* end of file-size-bridge.c */

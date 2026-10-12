/*
    FreeBASIC FMOD metadata qualification
    File: fmod-tags.c
    Purpose: Inspect bounded tag values using the publisher's native C API.
    Responsibilities: Exact integer outputs, returned byte ranges and cleanup.
    This metadata-only probe does not claim audio output qualification.
*/
#include <fmod.h>
#include <stdio.h>
#include <stdint.h>

int main(int argc, char **argv)
{
    const char *keys[] = {"TITLE", "ARTIST", "ALBUM", "TIT2", "TPE1", "TALB"};
    FSOUND_STREAM *stream;
    int index, count = 0, result = 0;
    if(argc != 2) return 1;
    if(!FSOUND_SetOutput(FSOUND_OUTPUT_NOSOUND) || !FSOUND_Init(44100,4,0)) return 2;
    stream=FSOUND_Stream_Open(argv[1],FSOUND_MPEGACCURATE,0,0);
    if(!stream) { FSOUND_Close(); return 1; }
    if(!FSOUND_Stream_GetNumTagFields(stream,&count) || count<0 || count>1024) result=1;
    printf("COUNT %d\n",count);
    for(index=0;result==0 && index<6;index++) {
        struct { uint32_t before; int length; uint32_t after; } field={0xA1B2C3D4,0,0xE5F60718};
        void *value=NULL;
        int found=FSOUND_Stream_FindTagField(stream,index<3?FSOUND_TAGFIELD_ID3V1:FSOUND_TAGFIELD_ID3V2,
                                           keys[index],&value,&field.length);
        if(field.before!=0xA1B2C3D4 || field.after!=0xE5F60718 || field.length<0 || field.length>4096) {result=1;break;}
        printf("TAG %s %d %d ",keys[index],found,field.length);
        if(found && field.length>0) {
            int offset;
            if(!value) {result=1;break;}
            for(offset=0;offset<field.length && offset<128;offset++) printf("%02x",((const unsigned char *)value)[offset]);
        }
        putchar('\n');
    }
    if(!FSOUND_Stream_Close(stream)) result=1;
    FSOUND_Close();
    return result;
}
/* end of fmod-tags.c */

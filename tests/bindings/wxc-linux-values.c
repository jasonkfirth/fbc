/*
    FreeBASIC wx-c Linux qualification
    File: wxc-linux-values.c
    Purpose: Check the genuine wrapper's C exports and Unicode values.
    Responsibilities: Native widths, string conversion and owned objects.
    This file contains no widget implementation or output-device checks.
*/
#include <dlfcn.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

/* The publisher's GNU build exports Cdecl functions with undecorated names.
   These signatures come from wxstring.cxx and colour.cxx. POSIX dlsym
   represents function addresses in void pointers; copy into matching slots. */
static void resolve(void *library, const char *name, void *slot, size_t size)
{
    void *address=dlsym(library,name);
    if(!address || size!=sizeof(address)) {
        fprintf(stderr,"Missing wx-c export: %s\n",name);
        _Exit(2);
    }
    memcpy(slot,&address,size);
}

int main(int argc, char **argv)
{
    void *library, *text=NULL, *empty=NULL, *colour=NULL;
    void *(*string_create)(const char *);
    void (*string_delete)(void *);
    size_t (*string_length)(const void *);
    wchar_t (*string_character)(const void *,size_t);
    void *(*colour_create)(unsigned char,unsigned char,unsigned char);
    void (*colour_delete)(void *);
    unsigned char (*colour_red)(const void *), (*colour_green)(const void *), (*colour_blue)(const void *);
    const wchar_t expected[]={65,233,937};
    int index,result=1;
    if(argc!=2) return 1;
    library=dlopen(argv[1],RTLD_NOW|RTLD_LOCAL);
    if(!library) {fprintf(stderr,"%s\n",dlerror());return 3;}
#define RESOLVE(name,slot) resolve(library,name,&slot,sizeof(slot))
    RESOLVE("wxString_ctorUTF8",string_create);
    RESOLVE("wxString_dtor",string_delete);
    RESOLVE("wxString_GetLength",string_length);
    RESOLVE("wxString_CharAtUTF16",string_character);
    RESOLVE("wxColour_ctorByParts",colour_create);
    RESOLVE("wxColour_dtor",colour_delete);
    RESOLVE("wxColour_Red",colour_red);
    RESOLVE("wxColour_Green",colour_green);
    RESOLVE("wxColour_Blue",colour_blue);
    printf("WXC_ABI=%zu,%zu,%zu,%zu,%zu\n",sizeof(void *),sizeof(int),sizeof(long),sizeof(size_t),sizeof(wchar_t));
    text=string_create("A\xc3\xa9\xce\xa9");
    empty=string_create("");
    if(!text || !empty || string_length(text)!=3 || string_length(empty)!=0) goto cleanup;
    for(index=0;index<3;index++) if(string_character(text,(size_t)index)!=expected[index]) goto cleanup;
    colour=colour_create(17,129,250);
    if(!colour || colour_red(colour)!=17 || colour_green(colour)!=129 || colour_blue(colour)!=250) goto cleanup;
    result=0;
cleanup:
    if(colour) colour_delete(colour);
    if(empty) string_delete(empty);
    if(text) string_delete(text);
    if(dlclose(library)) result=1;
    if(result==0) puts("WXC_VALUES=PASS unicode=3 empty=0 rgb=17,129,250");
    return result;
}
/* end of wxc-linux-values.c */

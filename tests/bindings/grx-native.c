/*
    FreeBASIC GRX 2.4.9 native qualification
    File: grx-native.c
    Purpose: Inspect the publisher ABI and validate a real graphics window.
    Responsibilities: Context layouts, palette colors and bounded pixel checks.
    This file contains no replacement renderer or keyboard injection.
*/
#include <grx20.h>
#include <grxkeys.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#ifdef _WIN32
#include <windows.h>
extern HWND hGRXWnd;
#endif

int main(int argc, char **argv)
{
    int row, column, width, height, result=0;
    printf("ABI %u %u %u %u %u %u %u %u %u %u\n",
           (unsigned)sizeof(void *),(unsigned)sizeof(GrColor),(unsigned)sizeof(GrKeyType),
           (unsigned)sizeof(GrFrame),(unsigned)offsetof(GrFrame,gf_lineoffset),
           (unsigned)offsetof(GrFrame,gf_driver),(unsigned)sizeof(GrContext),
           (unsigned)offsetof(GrContext,gc_xmax),(unsigned)sizeof(GrMouseEvent),GRX_VERSION_API);
    if(argc==1) return 0;
    (void)argv;
    if(!GrSetMode(GR_width_height_graphics,320,240)) return 2;
#ifdef _WIN32
    if(!hGRXWnd || !SetWindowTextA(hGRXWnd,"GRX native C qualification")) result=1;
#endif
    width=(GrSizeX()-10)/32;
    height=(GrSizeY()-10)/8;
    if(width<=0 || height<=0) result=1;
    GrSetRGBcolorMode();
    for(row=0;result==0 && row<8;row++) {
        for(column=0;column<32;column++) {
            GrColor color=(GrColor)(row*32+column);
            int x=5+column*width, y=5+row*height;
            GrFilledBox(x,y,x+width-1,y+height-1,color);
            if(GrPixel(x+width/2,y+height/2)!=color) {result=1;break;}
        }
    }
    GrSleep(200);
#ifdef _WIN32
    if(result==0) {
        HDC dc=GetDC(hGRXWnd);
        if(!dc) result=1;
        else {
            /* Compare the displayed GDI pixels with GRX's logical palette.
               These are independent APIs; reading back only GrPixel would
               not demonstrate that the native window was updated. */
            for(row=0;result==0 && row<8;row++) {
                for(column=0;column<32;column++) {
                    int red,green,blue;
                    COLORREF pixel;
                    GrQueryColor((GrColor)(row*32+column),&red,&green,&blue);
                    pixel=GetPixel(dc,5+column*width+width/2,5+row*height+height/2);
                    if(pixel==CLR_INVALID || pixel!=RGB(red,green,blue)) {
                        printf("GDI_PIXEL_ERROR=%d,%d expected=%d,%d,%d actual=%lu\n",row,column,red,green,blue,(unsigned long)pixel);
                        result=1;break;
                    }
                }
            }
            ReleaseDC(hGRXWnd,dc);
        }
    }
#endif
    if(!GrSetMode(GR_default_text)) result=1;
    if(result==0) printf("GRX_NATIVE_WINDOW=PASS colors=256\n");
    return result;
}
/* end of grx-native.c */

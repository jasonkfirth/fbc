/*
 * Project: FreeBASIC runtime library
 * File: fb_data.h
 * Purpose: Provide fb data runtime support.
 * Responsibilities: Shared runtime types, ABI declarations and internal contracts.
 * This file intentionally does NOT contain: Platform implementations or compiler parsing.
 */
#include "fb_abi.h"
#ifndef FB_DATADESC_PACKED
#define FB_DATADESC_PACKED FBPACKED
#endif

struct FB_DATADESC_ {
	short 					len;
	union {
		char 				*zstr;
		FB_WCHAR			*wstr;
		void   				*ofs;
		struct FB_DATADESC_ *next;
	};
} FB_DATADESC_PACKED;

typedef struct FB_DATADESC_ FB_DATADESC;

extern FB_DATADESC *fb_data_ptr FB_RTLIB_SYMBOL("__fb_data_ptr");

#define FB_DATATYPE_LINK -1
#define FB_DATATYPE_OFS  -2
#define FB_DATATYPE_WSTR 0x8000

FBCALL void         fb_DataRestore      ( FB_DATADESC *labeladdr );
       void         fb_DataNext         ( void );
FBCALL void         fb_DataReadStr      ( void *dst, ssize_t dst_size, int fillrem );
FBCALL void         fb_DataReadWstr     ( FB_WCHAR *dst, ssize_t dst_size );
FBCALL void         fb_DataReadBool     ( char *dst );
FBCALL void         fb_DataReadByte     ( char *dst );
FBCALL void         fb_DataReadUByte    ( unsigned char *dst );
FBCALL void         fb_DataReadShort    ( short *dst );
FBCALL void         fb_DataReadUShort   ( unsigned short *dst );
FBCALL void         fb_DataReadInt      ( int *dst );
FBCALL void         fb_DataReadUInt     ( unsigned int *dst );
FBCALL void         fb_DataReadLongint  ( long long *dst );
FBCALL void         fb_DataReadULongint ( unsigned long long *dst );
FBCALL void         fb_DataReadSingle   ( float *dst );
FBCALL void         fb_DataReadDouble   ( double *dst );

/* end of fb_data.h */

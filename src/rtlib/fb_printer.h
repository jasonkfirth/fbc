/*
 * Project: FreeBASIC runtime library
 * File: fb_printer.h
 * Purpose: Provide fb printer runtime support.
 * Responsibilities: Shared runtime types, ABI declarations and internal contracts.
 * This file intentionally does NOT contain: Platform implementations or compiler parsing.
 */
/* Keep the GNU flexible-tail spelling explicit in C++ builds. */
#if defined(__GNUC__)
#define FB_PRINTER_EXTENSION __extension__
#else
#define FB_PRINTER_EXTENSION
#endif

typedef struct {
	char * proto;
	int iPort;
#if defined(__SIZEOF_POINTER__) && (__SIZEOF_POINTER__ == 8)
	/* Preserve the existing pointer alignment and flexible-tail offset. */
	unsigned char name_alignment_padding[4];
#endif
	char * name;
	char * title;
	char * emu;
	FB_PRINTER_EXTENSION char raw[];
} DEV_LPT_PROTOCOL;

#undef FB_PRINTER_EXTENSION

typedef struct {
    void  *driver_opaque; /* this member must be first */
    char  *pszDevice;
    int    iPort;
#if defined(__SIZEOF_POINTER__) && (__SIZEOF_POINTER__ == 8)
    /* The native reference count keeps its original 64-bit offset. */
    unsigned char refcount_alignment_padding[4];
#endif
    size_t uiRefCount;
} DEV_LPT_INFO;

       int          fb_DevLptParseProtocol( DEV_LPT_PROTOCOL ** lpt_proto_out, const char * proto_raw, size_t proto_raw_len, int substprn );
       int          fb_DevLptTestProtocol( FB_FILE *handle, const char *filename, size_t filename_len );

       int          fb_DevLptWrite      ( FB_FILE *handle, const void* value, size_t valuelen );
       int          fb_DevLptWriteWstr  ( FB_FILE *handle, const FB_WCHAR* value, size_t valuelen );
       int          fb_DevLptClose      ( FB_FILE *handle );

       int          fb_DevPrinterSetWidth ( const char *pszDevice, int width,int default_width );
       int          fb_DevPrinterGetOffset( const char *pszDevice );

       int          fb_PrinterOpen      ( DEV_LPT_INFO *devInfo, int iPort, const char *pszDevice );
       int          fb_PrinterWrite     ( DEV_LPT_INFO *devInfo, const void *data, size_t length );
       int          fb_PrinterWriteWstr ( DEV_LPT_INFO *devInfo, const FB_WCHAR *data,size_t length );
       int          fb_PrinterClose     ( DEV_LPT_INFO *devInfo );

/* end of fb_printer.h */

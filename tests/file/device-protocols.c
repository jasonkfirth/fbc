typedef   signed char       int8;
typedef unsigned char      uint8;
typedef   signed short      int16;
typedef unsigned short     uint16;
typedef   signed int        int32;
typedef unsigned int       uint32;
typedef   signed long long  int64;
typedef unsigned long long uint64;
typedef struct { char *data; int32 len; int32 size; } FBSTRING;
typedef int8 boolean;
FBSTRING* fb_StrInit( void*, const int32, const void*, const int32, const int32 );
int32 fb_StrCompare( const void*, const int32, const void*, const int32 );
int32 fb_StrLen( const void*, const int32 );
static void fb_ctor__devicezprotocols( void ) __attribute__(( constructor ));
void free( void* );
int32 fb_DevComTestProtocolEx( void*, char*, uint32, uint32* );
struct $16DEV_LPT_PROTOCOL {
	char* PROTO;
	int32 IPORT;
	char* NAME;
	char* TITLE;
	char* EMU;
};
#define __FB_STATIC_ASSERT( expr ) extern int __$fb_structsizecheck[(expr) ? 1 : -1]
__FB_STATIC_ASSERT( sizeof( struct $16DEV_LPT_PROTOCOL ) == 20 );
int32 fb_DevLptParseProtocol( struct $16DEV_LPT_PROTOCOL**, char*, uint32, int32 );
struct $16DEV_TCP_PROTOCOL {
	char* HOST;
	uint32 PORT;
	uint32 TIMEOUT;
	uint32 BACKLOG;
	int32 IS_SERVER;
};
__FB_STATIC_ASSERT( sizeof( struct $16DEV_TCP_PROTOCOL ) == 20 );
int32 fb_DevTcpParseProtocol( struct $16DEV_TCP_PROTOCOL**, char*, uint32, int32 );

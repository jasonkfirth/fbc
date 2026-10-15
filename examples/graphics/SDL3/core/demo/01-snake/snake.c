typedef   signed char       int8;
typedef unsigned char      uint8;
typedef   signed short      int16;
typedef unsigned short     uint16;
typedef   signed int        int32;
typedef unsigned int       uint32;
typedef   signed long long  int64;
typedef unsigned long long uint64;
typedef struct { char *data; int64 len; int64 size; } FBSTRING;
typedef int8 boolean;
typedef int64 $9SNAKECELL;
struct $12SNAKECONTEXT {
	uint8 CELLS[162];
	int8 HEAD_XPOS;
	int8 HEAD_YPOS;
	int8 TAIL_XPOS;
	int8 TAIL_YPOS;
	int8 NEXT_DIR;
	int8 INHIBIT_TAIL_STEP;
	uint32 OCCUPIED_CELLS;
};
#define __FB_STATIC_ASSERT( expr ) extern int __$fb_structsizecheck[(expr) ? 1 : -1]
__FB_STATIC_ASSERT( sizeof( struct $12SNAKECONTEXT ) == 172 );
struct $9SDL_FRect {
	float X;
	float Y;
	float W;
	float H;
};
__FB_STATIC_ASSERT( sizeof( struct $9SDL_FRect ) == 16 );
typedef int64 $14SNAKEDIRECTION;
struct $8APPSTATE {
	void* WINDOW_;
	void* RENDERER;
	struct $12SNAKECONTEXT SNAKE_CTX;
	uint64 LAST_STEP;
};
__FB_STATIC_ASSERT( sizeof( struct $8APPSTATE ) == 200 );
struct $22EXTENDEDMETADATARECORD {
	char* KEY;
	char* VALUE;
};
__FB_STATIC_ASSERT( sizeof( struct $22EXTENDEDMETADATARECORD ) == 16 );
struct $15SDL_CommonEvent {
	uint32 TYPE;
	uint32 RESERVED;
	uint64 TIMESTAMP;
};
__FB_STATIC_ASSERT( sizeof( struct $15SDL_CommonEvent ) == 16 );
struct $16SDL_DisplayEvent {
	int32 TYPE;
	uint32 RESERVED;
	uint64 TIMESTAMP;
	uint32 DISPLAYID;
	int32 DATA1;
	int32 DATA2;
};
__FB_STATIC_ASSERT( sizeof( struct $16SDL_DisplayEvent ) == 32 );
struct $15SDL_WindowEvent {
	int32 TYPE;
	uint32 RESERVED;
	uint64 TIMESTAMP;
	uint32 WINDOWID;
	int32 DATA1;
	int32 DATA2;
};
__FB_STATIC_ASSERT( sizeof( struct $15SDL_WindowEvent ) == 32 );
struct $23SDL_KeyboardDeviceEvent {
	int32 TYPE;
	uint32 RESERVED;
	uint64 TIMESTAMP;
	uint32 WHICH;
};
__FB_STATIC_ASSERT( sizeof( struct $23SDL_KeyboardDeviceEvent ) == 24 );
struct $17SDL_KeyboardEvent {
	int32 TYPE;
	uint32 RESERVED;
	uint64 TIMESTAMP;
	uint32 WINDOWID;
	uint32 WHICH;
	int32 SCANCODE;
	uint32 KEY;
	uint16 MOD_;
	uint16 RAW;
	boolean DOWN;
	boolean REPEAT;
};
__FB_STATIC_ASSERT( sizeof( struct $17SDL_KeyboardEvent ) == 40 );
struct $20SDL_TextEditingEvent {
	int32 TYPE;
	uint32 RESERVED;
	uint64 TIMESTAMP;
	uint32 WINDOWID;
	char* TEXT;
	int32 START;
	int32 LENGTH;
};
__FB_STATIC_ASSERT( sizeof( struct $20SDL_TextEditingEvent ) == 40 );
struct $30SDL_TextEditingCandidatesEvent {
	int32 TYPE;
	uint32 RESERVED;
	uint64 TIMESTAMP;
	uint32 WINDOWID;
	char** CANDIDATES;
	int32 NUM_CANDIDATES;
	int32 SELECTED_CANDIDATE;
	boolean HORIZONTAL;
	uint8 PADDING1;
	uint8 PADDING2;
	uint8 PADDING3;
};
__FB_STATIC_ASSERT( sizeof( struct $30SDL_TextEditingCandidatesEvent ) == 48 );
struct $18SDL_TextInputEvent {
	int32 TYPE;
	uint32 RESERVED;
	uint64 TIMESTAMP;
	uint32 WINDOWID;
	char* TEXT;
};
__FB_STATIC_ASSERT( sizeof( struct $18SDL_TextInputEvent ) == 32 );
struct $20SDL_MouseDeviceEvent {
	int32 TYPE;
	uint32 RESERVED;
	uint64 TIMESTAMP;
	uint32 WHICH;
};
__FB_STATIC_ASSERT( sizeof( struct $20SDL_MouseDeviceEvent ) == 24 );
struct $20SDL_MouseMotionEvent {
	int32 TYPE;
	uint32 RESERVED;
	uint64 TIMESTAMP;
	uint32 WINDOWID;
	uint32 WHICH;
	uint32 STATE;
	float X;
	float Y;
	float XREL;
	float YREL;
};
__FB_STATIC_ASSERT( sizeof( struct $20SDL_MouseMotionEvent ) == 48 );
struct $20SDL_MouseButtonEvent {
	int32 TYPE;
	uint32 RESERVED;
	uint64 TIMESTAMP;
	uint32 WINDOWID;
	uint32 WHICH;
	uint8 BUTTON;
	boolean DOWN;
	uint8 CLICKS;
	uint8 PADDING;
	float X;
	float Y;
};
__FB_STATIC_ASSERT( sizeof( struct $20SDL_MouseButtonEvent ) == 40 );
struct $19SDL_MouseWheelEvent {
	int32 TYPE;
	uint32 RESERVED;
	uint64 TIMESTAMP;
	uint32 WINDOWID;
	uint32 WHICH;
	float X;
	float Y;
	int32 DIRECTION;
	float MOUSE_X;
	float MOUSE_Y;
	int32 INTEGER_X;
	int32 INTEGER_Y;
};
__FB_STATIC_ASSERT( sizeof( struct $19SDL_MouseWheelEvent ) == 56 );
struct $18SDL_JoyDeviceEvent {
	int32 TYPE;
	uint32 RESERVED;
	uint64 TIMESTAMP;
	uint32 WHICH;
};
__FB_STATIC_ASSERT( sizeof( struct $18SDL_JoyDeviceEvent ) == 24 );
struct $16SDL_JoyAxisEvent {
	int32 TYPE;
	uint32 RESERVED;
	uint64 TIMESTAMP;
	uint32 WHICH;
	uint8 AXIS;
	uint8 PADDING1;
	uint8 PADDING2;
	uint8 PADDING3;
	int16 VALUE;
	uint16 PADDING4;
};
__FB_STATIC_ASSERT( sizeof( struct $16SDL_JoyAxisEvent ) == 32 );
struct $16SDL_JoyBallEvent {
	int32 TYPE;
	uint32 RESERVED;
	uint64 TIMESTAMP;
	uint32 WHICH;
	uint8 BALL;
	uint8 PADDING1;
	uint8 PADDING2;
	uint8 PADDING3;
	int16 XREL;
	int16 YREL;
};
__FB_STATIC_ASSERT( sizeof( struct $16SDL_JoyBallEvent ) == 32 );
struct $15SDL_JoyHatEvent {
	int32 TYPE;
	uint32 RESERVED;
	uint64 TIMESTAMP;
	uint32 WHICH;
	uint8 HAT;
	uint8 VALUE;
	uint8 PADDING1;
	uint8 PADDING2;
};
__FB_STATIC_ASSERT( sizeof( struct $15SDL_JoyHatEvent ) == 24 );
struct $18SDL_JoyButtonEvent {
	int32 TYPE;
	uint32 RESERVED;
	uint64 TIMESTAMP;
	uint32 WHICH;
	uint8 BUTTON;
	boolean DOWN;
	uint8 PADDING1;
	uint8 PADDING2;
};
__FB_STATIC_ASSERT( sizeof( struct $18SDL_JoyButtonEvent ) == 24 );
struct $19SDL_JoyBatteryEvent {
	int32 TYPE;
	uint32 RESERVED;
	uint64 TIMESTAMP;
	uint32 WHICH;
	int32 STATE;
	int32 PERCENT;
};
__FB_STATIC_ASSERT( sizeof( struct $19SDL_JoyBatteryEvent ) == 32 );
struct $22SDL_GamepadDeviceEvent {
	int32 TYPE;
	uint32 RESERVED;
	uint64 TIMESTAMP;
	uint32 WHICH;
};
__FB_STATIC_ASSERT( sizeof( struct $22SDL_GamepadDeviceEvent ) == 24 );
struct $20SDL_GamepadAxisEvent {
	int32 TYPE;
	uint32 RESERVED;
	uint64 TIMESTAMP;
	uint32 WHICH;
	uint8 AXIS;
	uint8 PADDING1;
	uint8 PADDING2;
	uint8 PADDING3;
	int16 VALUE;
	uint16 PADDING4;
};
__FB_STATIC_ASSERT( sizeof( struct $20SDL_GamepadAxisEvent ) == 32 );
struct $22SDL_GamepadButtonEvent {
	int32 TYPE;
	uint32 RESERVED;
	uint64 TIMESTAMP;
	uint32 WHICH;
	uint8 BUTTON;
	boolean DOWN;
	uint8 PADDING1;
	uint8 PADDING2;
};
__FB_STATIC_ASSERT( sizeof( struct $22SDL_GamepadButtonEvent ) == 24 );
struct $24SDL_GamepadTouchpadEvent {
	int32 TYPE;
	uint32 RESERVED;
	uint64 TIMESTAMP;
	uint32 WHICH;
	int32 TOUCHPAD;
	int32 FINGER;
	float X;
	float Y;
	float PRESSURE;
};
__FB_STATIC_ASSERT( sizeof( struct $24SDL_GamepadTouchpadEvent ) == 40 );
struct $22SDL_GamepadSensorEvent {
	int32 TYPE;
	uint32 RESERVED;
	uint64 TIMESTAMP;
	uint32 WHICH;
	int32 SENSOR;
	float DATA[3];
	uint64 SENSOR_TIMESTAMP;
};
__FB_STATIC_ASSERT( sizeof( struct $22SDL_GamepadSensorEvent ) == 48 );
struct $20SDL_AudioDeviceEvent {
	int32 TYPE;
	uint32 RESERVED;
	uint64 TIMESTAMP;
	uint32 WHICH;
	boolean RECORDING;
	uint8 PADDING1;
	uint8 PADDING2;
	uint8 PADDING3;
};
__FB_STATIC_ASSERT( sizeof( struct $20SDL_AudioDeviceEvent ) == 24 );
struct $21SDL_CameraDeviceEvent {
	int32 TYPE;
	uint32 RESERVED;
	uint64 TIMESTAMP;
	uint32 WHICH;
};
__FB_STATIC_ASSERT( sizeof( struct $21SDL_CameraDeviceEvent ) == 24 );
struct $15SDL_SensorEvent {
	int32 TYPE;
	uint32 RESERVED;
	uint64 TIMESTAMP;
	uint32 WHICH;
	float DATA[6];
	uint64 SENSOR_TIMESTAMP;
};
__FB_STATIC_ASSERT( sizeof( struct $15SDL_SensorEvent ) == 56 );
struct $13SDL_QuitEvent {
	int32 TYPE;
	uint32 RESERVED;
	uint64 TIMESTAMP;
};
__FB_STATIC_ASSERT( sizeof( struct $13SDL_QuitEvent ) == 16 );
struct $13SDL_UserEvent {
	uint32 TYPE;
	uint32 RESERVED;
	uint64 TIMESTAMP;
	uint32 WINDOWID;
	int32 CODE;
	void* DATA1;
	void* DATA2;
};
__FB_STATIC_ASSERT( sizeof( struct $13SDL_UserEvent ) == 40 );
struct $20SDL_TouchFingerEvent {
	int32 TYPE;
	uint32 RESERVED;
	uint64 TIMESTAMP;
	uint64 TOUCHID;
	uint64 FINGERID;
	float X;
	float Y;
	float DX;
	float DY;
	float PRESSURE;
	uint32 WINDOWID;
};
__FB_STATIC_ASSERT( sizeof( struct $20SDL_TouchFingerEvent ) == 56 );
struct $20SDL_PinchFingerEvent {
	int32 TYPE;
	uint32 RESERVED;
	uint64 TIMESTAMP;
	float SCALE;
	uint32 WINDOWID;
};
__FB_STATIC_ASSERT( sizeof( struct $20SDL_PinchFingerEvent ) == 24 );
struct $21SDL_PenProximityEvent {
	int32 TYPE;
	uint32 RESERVED;
	uint64 TIMESTAMP;
	uint32 WINDOWID;
	uint32 WHICH;
	uint32 PEN_STATE;
	int32 DEVICE_TYPE;
};
__FB_STATIC_ASSERT( sizeof( struct $21SDL_PenProximityEvent ) == 32 );
struct $17SDL_PenTouchEvent {
	int32 TYPE;
	uint32 RESERVED;
	uint64 TIMESTAMP;
	uint32 WINDOWID;
	uint32 WHICH;
	uint32 PEN_STATE;
	float X;
	float Y;
	boolean ERASER;
	boolean DOWN;
	int32 DEVICE_TYPE;
};
__FB_STATIC_ASSERT( sizeof( struct $17SDL_PenTouchEvent ) == 48 );
struct $18SDL_PenMotionEvent {
	int32 TYPE;
	uint32 RESERVED;
	uint64 TIMESTAMP;
	uint32 WINDOWID;
	uint32 WHICH;
	uint32 PEN_STATE;
	float X;
	float Y;
	int32 DEVICE_TYPE;
};
__FB_STATIC_ASSERT( sizeof( struct $18SDL_PenMotionEvent ) == 40 );
struct $18SDL_PenButtonEvent {
	int32 TYPE;
	uint32 RESERVED;
	uint64 TIMESTAMP;
	uint32 WINDOWID;
	uint32 WHICH;
	uint32 PEN_STATE;
	float X;
	float Y;
	uint8 BUTTON;
	boolean DOWN;
	int32 DEVICE_TYPE;
};
__FB_STATIC_ASSERT( sizeof( struct $18SDL_PenButtonEvent ) == 48 );
struct $16SDL_PenAxisEvent {
	int32 TYPE;
	uint32 RESERVED;
	uint64 TIMESTAMP;
	uint32 WINDOWID;
	uint32 WHICH;
	uint32 PEN_STATE;
	float X;
	float Y;
	int32 AXIS;
	float VALUE;
	int32 DEVICE_TYPE;
};
__FB_STATIC_ASSERT( sizeof( struct $16SDL_PenAxisEvent ) == 48 );
struct $15SDL_RenderEvent {
	int32 TYPE;
	uint32 RESERVED;
	uint64 TIMESTAMP;
	uint32 WINDOWID;
};
__FB_STATIC_ASSERT( sizeof( struct $15SDL_RenderEvent ) == 24 );
struct $13SDL_DropEvent {
	int32 TYPE;
	uint32 RESERVED;
	uint64 TIMESTAMP;
	uint32 WINDOWID;
	float X;
	float Y;
	char* SOURCE;
	char* DATA;
};
__FB_STATIC_ASSERT( sizeof( struct $13SDL_DropEvent ) == 48 );
struct $18SDL_ClipboardEvent {
	int32 TYPE;
	uint32 RESERVED;
	uint64 TIMESTAMP;
	boolean OWNER;
	int32 NUM_MIME_TYPES;
	char** MIME_TYPES;
};
__FB_STATIC_ASSERT( sizeof( struct $18SDL_ClipboardEvent ) == 32 );
union $9SDL_Event {
	uint32 TYPE;
	struct $15SDL_CommonEvent COMMON;
	struct $16SDL_DisplayEvent DISPLAY;
	struct $15SDL_WindowEvent WINDOW;
	struct $23SDL_KeyboardDeviceEvent KDEVICE;
	struct $17SDL_KeyboardEvent KEY;
	struct $20SDL_TextEditingEvent EDIT;
	struct $30SDL_TextEditingCandidatesEvent EDIT_CANDIDATES;
	struct $18SDL_TextInputEvent TEXT;
	struct $20SDL_MouseDeviceEvent MDEVICE;
	struct $20SDL_MouseMotionEvent MOTION;
	struct $20SDL_MouseButtonEvent BUTTON;
	struct $19SDL_MouseWheelEvent WHEEL;
	struct $18SDL_JoyDeviceEvent JDEVICE;
	struct $16SDL_JoyAxisEvent JAXIS;
	struct $16SDL_JoyBallEvent JBALL;
	struct $15SDL_JoyHatEvent JHAT;
	struct $18SDL_JoyButtonEvent JBUTTON;
	struct $19SDL_JoyBatteryEvent JBATTERY;
	struct $22SDL_GamepadDeviceEvent GDEVICE;
	struct $20SDL_GamepadAxisEvent GAXIS;
	struct $22SDL_GamepadButtonEvent GBUTTON;
	struct $24SDL_GamepadTouchpadEvent GTOUCHPAD;
	struct $22SDL_GamepadSensorEvent GSENSOR;
	struct $20SDL_AudioDeviceEvent ADEVICE;
	struct $21SDL_CameraDeviceEvent CDEVICE;
	struct $15SDL_SensorEvent SENSOR;
	struct $13SDL_QuitEvent QUIT;
	struct $13SDL_UserEvent USER;
	struct $20SDL_TouchFingerEvent TFINGER;
	struct $20SDL_PinchFingerEvent PINCH;
	struct $21SDL_PenProximityEvent PPROXIMITY;
	struct $17SDL_PenTouchEvent PTOUCH;
	struct $18SDL_PenMotionEvent PMOTION;
	struct $18SDL_PenButtonEvent PBUTTON;
	struct $16SDL_PenAxisEvent PAXIS;
	struct $15SDL_RenderEvent RENDER;
	struct $13SDL_DropEvent DROP;
	struct $18SDL_ClipboardEvent CLIPBOARD;
	uint8 PADDING[128];
};
__FB_STATIC_ASSERT( sizeof( union $9SDL_Event ) == 128 );
typedef int32 (*tmp$210)( int32, char** );
typedef int32 (*tmp$211)( void**, int32, char** );
typedef int32 (*tmp$212)( void* );
typedef int32 (*tmp$213)( void*, union $9SDL_Event* );
typedef void (*tmp$214)( void*, int32 );
int32 fb_StrCompare( const void*, const int64, const void*, const int64 );
FBSTRING* fb_StrAllocTempDescZEx( const char*, const int64 );
int32 fb_VALINT( const FBSTRING* );
void fb_End( const int32 );
FBSTRING* fb_Command( const int32 );
FBSTRING* fb_GetEnviron( const FBSTRING* );
static void fb_ctor__snake( void ) __attribute__(( constructor ));
void* SDL_calloc( uint64, uint64 );
void SDL_free( void* );
void* SDL_memcpy( void*, const void*, uint64 );
void* SDL_memset( void*, int32, uint64 );
int32 SDL_rand( int32 );
float SDL_fabsf( float );
const char* SDL_GetError( void );
void SDL_DestroyWindow( void* );
void* SDL_OpenJoystick( uint32 );
uint32 SDL_GetJoystickID( void* );
void SDL_CloseJoystick( void* );
boolean SDL_Init( uint32 );
boolean SDL_SetAppMetadata( const char*, const char*, const char* );
boolean SDL_SetAppMetadataProperty( const char*, const char* );
void tmp$204( const char*, ... ) asm("SDL_Log");
boolean SDL_CreateWindowAndRenderer( const char*, int32, int32, uint64, void**, void** );
boolean SDL_SetRenderLogicalPresentation( void*, int32, int32, int32 );
boolean SDL_SetRenderDrawColor( void*, uint8, uint8, uint8, uint8 );
boolean SDL_RenderClear( void* );
boolean SDL_RenderFillRect( void*, const struct $9SDL_FRect* );
boolean SDL_RenderPresent( void* );
void SDL_DestroyRenderer( void* );
uint64 SDL_GetTicks( void );
int32 SDL_RunApp( int32, char**, tmp$210, void* );
int32 SDL_EnterAppMainCallbacks( int32, char**, tmp$211, tmp$212, tmp$213, tmp$214 );
static uint32 SDL_SWAP32( uint32 );
static boolean SDL_RECTSEQUALEPSILON( const struct $9SDL_FRect*, const struct $9SDL_FRect*, float );
$9SNAKECELL SNAKE_CELL_AT( const struct $12SNAKECONTEXT*, int8, int8 );
void SET_RECT_XY_( struct $9SDL_FRect*, int16, int16 );
void PUT_CELL_AT_( struct $12SNAKECONTEXT*, int8, int8, $9SNAKECELL );
int32 ARE_CELLS_FULL_( struct $12SNAKECONTEXT* );
void NEW_FOOD_POS_( struct $12SNAKECONTEXT* );
void SNAKE_INITIALIZE( struct $12SNAKECONTEXT* );
void SNAKE_REDIR( struct $12SNAKECONTEXT*, $14SNAKEDIRECTION );
void WRAP_AROUND_( char*, int8 );
void SNAKE_STEP( struct $12SNAKECONTEXT* );
int32 HANDLE_KEY_EVENT_( struct $12SNAKECONTEXT*, int32 );
int32 HANDLE_HAT_EVENT_( struct $12SNAKECONTEXT*, uint8 );
int32 SDL_APPITERATE( void* );
int32 SDL_APPINIT( void**, int32, char** );
int32 SDL_APPEVENT( void*, union $9SDL_Event* );
void SDL_APPQUIT( void*, int32 );
static int32 SDL3_EXAMPLEITERATE( void* );
static int32 SDL3_EXAMPLEMAIN( int32, char** );
static void* JOYSTICK$ = (void*)0ull;
static struct $22EXTENDEDMETADATARECORD EXTENDED_METADATA$[4] = { { (char*)"SDL.app.metadata.url", (char*)"https://examples.libsdl.org/SDL3/demo/01-snake/" }, { (char*)"SDL.app.metadata.creator", (char*)"SDL team" }, { (char*)"SDL.app.metadata.copyright", (char*)"Placed in the public domain" }, { (char*)"SDL.app.metadata.type", (char*)"game" } };

$9SNAKECELL SNAKE_CELL_AT( const struct $12SNAKECONTEXT* CTX$1, int8 X$1, int8 Y$1 )
{
	_unusedlabel: ; void *_llvmbug18658 = &&_unusedlabel;
	$9SNAKECELL fb$result$1;
	__builtin_memset( &fb$result$1, 0, 8ll );
	label$79:;
	{
		int64 TMP$188$2;
		int32 SHIFT_BITS$2;
		SHIFT_BITS$2 = (int32)(((uint64)(int64)X$1 + ((uint64)(int64)Y$1 * 24ull)) * 3ull);
		uint16 RANGE$2;
		__builtin_memset( &RANGE$2, 0, 2ll );
		TMP$188$2 = (int64)SHIFT_BITS$2;
		SDL_memcpy( (void*)&RANGE$2, (void*)((uint8*)CTX$1 + ((TMP$188$2 + ((TMP$188$2 >> (63ll & 63ll)) & 7ll)) >> (3ll & 63ll))), 2ull );
		fb$result$1 = ($9SNAKECELL)((uint64)((int64)RANGE$2 >> (((int64)SHIFT_BITS$2 % 8ll) & 63ll)) & 7ull);
		goto label$80;
	}
	label$80:;
	return fb$result$1;
}

void SET_RECT_XY_( struct $9SDL_FRect* R$1, int16 X$1, int16 Y$1 )
{
	_unusedlabel: ; void *_llvmbug18658 = &&_unusedlabel;
	label$81:;
	{
		*(float*)R$1 = (float)((int64)X$1 * 24ll);
		*(float*)((uint8*)R$1 + 4ll) = (float)((int64)Y$1 * 24ll);
	}
	label$82:;
}

void PUT_CELL_AT_( struct $12SNAKECONTEXT* CTX$1, int8 X$1, int8 Y$1, $9SNAKECELL CT$1 )
{
	_unusedlabel: ; void *_llvmbug18658 = &&_unusedlabel;
	label$83:;
	{
		int64 TMP$189$2;
		int32 SHIFT_BITS$2;
		SHIFT_BITS$2 = (int32)(((uint64)(int64)X$1 + ((uint64)(int64)Y$1 * 24ull)) * 3ull);
		int32 ADJUST$2;
		ADJUST$2 = (int32)((int64)SHIFT_BITS$2 % 8ll);
		const uint8* POS_$2;
		TMP$189$2 = (int64)SHIFT_BITS$2;
		POS_$2 = (uint8*)((uint8*)CTX$1 + ((TMP$189$2 + ((TMP$189$2 >> (63ll & 63ll)) & 7ll)) >> (3ll & 63ll)));
		uint16 RANGE$2;
		__builtin_memset( &RANGE$2, 0, 2ll );
		SDL_memcpy( (void*)&RANGE$2, (void*)POS_$2, 2ull );
		RANGE$2 = (uint16)((uint64)(int64)RANGE$2 & ~(7ull << ((int64)ADJUST$2 & 63ll)));
		RANGE$2 = (uint16)((uint64)(int64)RANGE$2 | (((uint64)CT$1 & 7ull) << ((int64)ADJUST$2 & 63ll)));
		SDL_memcpy( (void*)POS_$2, (void*)&RANGE$2, 2ull );
	}
	label$84:;
}

int32 ARE_CELLS_FULL_( struct $12SNAKECONTEXT* CTX$1 )
{
	_unusedlabel: ; void *_llvmbug18658 = &&_unusedlabel;
	int32 fb$result$1;
	__builtin_memset( &fb$result$1, 0, 4ll );
	label$85:;
	{
		fb$result$1 = (int32)(int64)-((uint64)(int64)*(uint32*)((uint8*)CTX$1 + 168ll) == 432ull);
		goto label$86;
	}
	label$86:;
	return fb$result$1;
}

void NEW_FOOD_POS_( struct $12SNAKECONTEXT* CTX$1 )
{
	_unusedlabel: ; void *_llvmbug18658 = &&_unusedlabel;
	label$87:;
	{
		label$89:;
		{
			goto label$93;
			{
				goto label$90;
				label$93:;
			}
			{
				int8 X$4;
				int32 vr$0 = SDL_rand( 24 );
				X$4 = (int8)vr$0;
				int8 Y$4;
				int32 vr$2 = SDL_rand( 18 );
				Y$4 = (int8)vr$2;
				$9SNAKECELL vr$4 = SNAKE_CELL_AT( (struct $12SNAKECONTEXT*)CTX$1, X$4, Y$4 );
				if( vr$4 != 0ll) goto label$95;
				{
					{
						PUT_CELL_AT_( CTX$1, X$4, Y$4, 5ll );
						goto label$90;
					}
				}
				label$95:;
				label$94:;
			}
		}
		label$91:;
		goto label$89;
		label$90:;
	}
	label$88:;
}

void SNAKE_INITIALIZE( struct $12SNAKECONTEXT* CTX$1 )
{
	_unusedlabel: ; void *_llvmbug18658 = &&_unusedlabel;
	label$96:;
	{
		int32 I$2;
		__builtin_memset( &I$2, 0, 4ll );
		SDL_memset( (void*)CTX$1, 0, 162ull );
		*(int8*)((uint8*)CTX$1 + 164ll) = (int8)12;
		*(int8*)((uint8*)CTX$1 + 162ll) = *(int8*)((uint8*)CTX$1 + 164ll);
		*(int8*)((uint8*)CTX$1 + 165ll) = (int8)9;
		*(int8*)((uint8*)CTX$1 + 163ll) = *(int8*)((uint8*)CTX$1 + 165ll);
		*(int8*)((uint8*)CTX$1 + 166ll) = (int8)0;
		*(uint32*)((uint8*)CTX$1 + 168ll) = 4u;
		*(int8*)((uint8*)CTX$1 + 167ll) = (int8)*(uint32*)((uint8*)CTX$1 + 168ll);
		*(uint32*)((uint8*)CTX$1 + 168ll) = (uint32)((int64)*(uint32*)((uint8*)CTX$1 + 168ll) + -1ll);
		PUT_CELL_AT_( CTX$1, *(int8*)((uint8*)CTX$1 + 164ll), *(int8*)((uint8*)CTX$1 + 165ll), 1ll );
		{
			I$2 = 0;
			label$98:;
			if( (int64)I$2 >= 4ll) goto label$99;
			{
				{
					NEW_FOOD_POS_( CTX$1 );
					*(uint32*)((uint8*)CTX$1 + 168ll) = (uint32)((int64)*(uint32*)((uint8*)CTX$1 + 168ll) + 1ll);
				}
				I$2 = (int32)((int64)I$2 + 1ll);
			}
			goto label$98;
			label$99:;
		}
	}
	label$97:;
}

void SNAKE_REDIR( struct $12SNAKECONTEXT* CTX$1, $14SNAKEDIRECTION DIRECTION_INDEX$1 )
{
	_unusedlabel: ; void *_llvmbug18658 = &&_unusedlabel;
	label$100:;
	{
		int64 TMP$190$2;
		int64 TMP$191$2;
		int64 TMP$192$2;
		int64 TMP$193$2;
		int64 TMP$194$2;
		int64 TMP$195$2;
		int64 TMP$196$2;
		$9SNAKECELL CT$2;
		$9SNAKECELL vr$2 = SNAKE_CELL_AT( (struct $12SNAKECONTEXT*)CTX$1, *(int8*)((uint8*)CTX$1 + 162ll), *(int8*)((uint8*)CTX$1 + 163ll) );
		CT$2 = vr$2;
		if( DIRECTION_INDEX$1 != 0ll) goto label$102;
		TMP$190$2 = (int64)-(CT$2 != 3ll);
		goto label$111;
		label$102:;
		TMP$190$2 = 0ll;
		label$111:;
		if( TMP$190$2 != 0ll) goto label$104;
		if( DIRECTION_INDEX$1 != 1ll) goto label$103;
		TMP$191$2 = (int64)-(CT$2 != 4ll);
		goto label$113;
		label$103:;
		TMP$191$2 = 0ll;
		label$113:;
		TMP$192$2 = (int64)-(TMP$191$2 != 0ll);
		goto label$112;
		label$104:;
		TMP$192$2 = -1ll;
		label$112:;
		if( TMP$192$2 != 0ll) goto label$106;
		if( DIRECTION_INDEX$1 != 2ll) goto label$105;
		TMP$193$2 = (int64)-(CT$2 != 1ll);
		goto label$115;
		label$105:;
		TMP$193$2 = 0ll;
		label$115:;
		TMP$194$2 = (int64)-(TMP$193$2 != 0ll);
		goto label$114;
		label$106:;
		TMP$194$2 = -1ll;
		label$114:;
		if( TMP$194$2 != 0ll) goto label$108;
		if( DIRECTION_INDEX$1 != 3ll) goto label$107;
		TMP$195$2 = (int64)-(CT$2 != 2ll);
		goto label$117;
		label$107:;
		TMP$195$2 = 0ll;
		label$117:;
		TMP$196$2 = (int64)-(TMP$195$2 != 0ll);
		goto label$116;
		label$108:;
		TMP$196$2 = -1ll;
		label$116:;
		if( TMP$196$2 == 0ll) goto label$110;
		{
			{
				*(int8*)((uint8*)CTX$1 + 166ll) = (int8)DIRECTION_INDEX$1;
			}
		}
		label$110:;
		label$109:;
	}
	label$101:;
}

void WRAP_AROUND_( char* VAL_$1, int8 MAX$1 )
{
	_unusedlabel: ; void *_llvmbug18658 = &&_unusedlabel;
	label$118:;
	{
		if( (int64)*(int8*)VAL_$1 >= 0ll) goto label$121;
		{
			{
				*(int8*)VAL_$1 = (int8)((int64)MAX$1 + -1ll);
			}
		}
		goto label$120;
		label$121:;
		{
			if( (int64)*(int8*)VAL_$1 <= ((int64)MAX$1 + -1ll)) goto label$123;
			{
				{
					*(int8*)VAL_$1 = (int8)0;
				}
			}
			label$123:;
			label$122:;
		}
		label$120:;
	}
	label$119:;
}

void SNAKE_STEP( struct $12SNAKECONTEXT* CTX$1 )
{
	_unusedlabel: ; void *_llvmbug18658 = &&_unusedlabel;
	label$124:;
	{
		int64 TMP$198$2;
		$9SNAKECELL DIR_AS_CELL$2;
		DIR_AS_CELL$2 = (int64)*(int8*)((uint8*)CTX$1 + 166ll) + 1ll;
		$9SNAKECELL CT$2;
		__builtin_memset( &CT$2, 0, 8ll );
		int8 PREV_XPOS$2;
		__builtin_memset( &PREV_XPOS$2, 0, 1ll );
		int8 PREV_YPOS$2;
		__builtin_memset( &PREV_YPOS$2, 0, 1ll );
		*(int8*)((uint8*)CTX$1 + 167ll) = (int8)((int64)*(int8*)((uint8*)CTX$1 + 167ll) + -1ll);
		if( (int64)*(int8*)((uint8*)CTX$1 + 167ll) != 0ll) goto label$127;
		{
			{
				*(int8*)((uint8*)CTX$1 + 167ll) = (int8)((int64)*(int8*)((uint8*)CTX$1 + 167ll) + 1ll);
				$9SNAKECELL vr$20 = SNAKE_CELL_AT( (struct $12SNAKECONTEXT*)CTX$1, *(int8*)((uint8*)CTX$1 + 164ll), *(int8*)((uint8*)CTX$1 + 165ll) );
				CT$2 = vr$20;
				PUT_CELL_AT_( CTX$1, *(int8*)((uint8*)CTX$1 + 164ll), *(int8*)((uint8*)CTX$1 + 165ll), 0ll );
				{
					if( CT$2 != 1ll) goto label$129;
					label$130:;
					{
						goto label$131;
					}
					goto label$128;
					label$129:;
					if( CT$2 != 2ll) goto label$132;
					label$133:;
					{
						goto label$134;
					}
					goto label$128;
					label$132:;
					if( CT$2 != 3ll) goto label$135;
					label$136:;
					{
						goto label$137;
					}
					goto label$128;
					label$135:;
					if( CT$2 != 4ll) goto label$138;
					label$139:;
					{
						goto label$140;
					}
					goto label$128;
					label$138:;
					{
						goto label$142;
					}
					label$141:;
					label$128:;
				}
				label$131:;
				{
					*(int8*)((uint8*)CTX$1 + 164ll) = (int8)((int64)*(int8*)((uint8*)CTX$1 + 164ll) + 1ll);
					goto label$143;
				}
				label$134:;
				{
					*(int8*)((uint8*)CTX$1 + 165ll) = (int8)((int64)*(int8*)((uint8*)CTX$1 + 165ll) + -1ll);
					goto label$143;
				}
				label$137:;
				{
					*(int8*)((uint8*)CTX$1 + 164ll) = (int8)((int64)*(int8*)((uint8*)CTX$1 + 164ll) + -1ll);
					goto label$143;
				}
				label$140:;
				{
					*(int8*)((uint8*)CTX$1 + 165ll) = (int8)((int64)*(int8*)((uint8*)CTX$1 + 165ll) + 1ll);
					goto label$143;
				}
				label$142:;
				{
					goto label$143;
				}
				label$143:;
				WRAP_AROUND_( (char*)((uint8*)CTX$1 + 164ll), (int8)24 );
				WRAP_AROUND_( (char*)((uint8*)CTX$1 + 165ll), (int8)18 );
			}
		}
		label$127:;
		label$126:;
		PREV_XPOS$2 = *(int8*)((uint8*)CTX$1 + 162ll);
		PREV_YPOS$2 = *(int8*)((uint8*)CTX$1 + 163ll);
		{
			int8 TMP$197$3;
			TMP$197$3 = *(int8*)((uint8*)CTX$1 + 166ll);
			if( (int64)TMP$197$3 != 0ll) goto label$145;
			label$146:;
			{
				goto label$147;
			}
			goto label$144;
			label$145:;
			if( (int64)TMP$197$3 != 1ll) goto label$148;
			label$149:;
			{
				goto label$150;
			}
			goto label$144;
			label$148:;
			if( (int64)TMP$197$3 != 2ll) goto label$151;
			label$152:;
			{
				goto label$153;
			}
			goto label$144;
			label$151:;
			if( (int64)TMP$197$3 != 3ll) goto label$154;
			label$155:;
			{
				goto label$156;
			}
			goto label$144;
			label$154:;
			{
				goto label$158;
			}
			label$157:;
			label$144:;
		}
		label$147:;
		{
			*(int8*)((uint8*)CTX$1 + 162ll) = (int8)((int64)*(int8*)((uint8*)CTX$1 + 162ll) + 1ll);
			goto label$159;
		}
		label$150:;
		{
			*(int8*)((uint8*)CTX$1 + 163ll) = (int8)((int64)*(int8*)((uint8*)CTX$1 + 163ll) + -1ll);
			goto label$159;
		}
		label$153:;
		{
			*(int8*)((uint8*)CTX$1 + 162ll) = (int8)((int64)*(int8*)((uint8*)CTX$1 + 162ll) + -1ll);
			goto label$159;
		}
		label$156:;
		{
			*(int8*)((uint8*)CTX$1 + 163ll) = (int8)((int64)*(int8*)((uint8*)CTX$1 + 163ll) + 1ll);
			goto label$159;
		}
		label$158:;
		{
			goto label$159;
		}
		label$159:;
		WRAP_AROUND_( (char*)((uint8*)CTX$1 + 162ll), (int8)24 );
		WRAP_AROUND_( (char*)((uint8*)CTX$1 + 163ll), (int8)18 );
		$9SNAKECELL vr$80 = SNAKE_CELL_AT( (struct $12SNAKECONTEXT*)CTX$1, *(int8*)((uint8*)CTX$1 + 162ll), *(int8*)((uint8*)CTX$1 + 163ll) );
		CT$2 = vr$80;
		if( CT$2 == 0ll) goto label$160;
		TMP$198$2 = (int64)-(CT$2 != 5ll);
		goto label$167;
		label$160:;
		TMP$198$2 = 0ll;
		label$167:;
		if( TMP$198$2 == 0ll) goto label$162;
		{
			{
				SNAKE_INITIALIZE( CTX$1 );
				goto label$125;
			}
		}
		label$162:;
		label$161:;
		PUT_CELL_AT_( CTX$1, PREV_XPOS$2, PREV_YPOS$2, DIR_AS_CELL$2 );
		PUT_CELL_AT_( CTX$1, *(int8*)((uint8*)CTX$1 + 162ll), *(int8*)((uint8*)CTX$1 + 163ll), DIR_AS_CELL$2 );
		if( CT$2 != 5ll) goto label$164;
		{
			{
				int32 vr$84 = ARE_CELLS_FULL_( CTX$1 );
				if( (int64)vr$84 == 0ll) goto label$166;
				{
					{
						SNAKE_INITIALIZE( CTX$1 );
						goto label$125;
					}
				}
				label$166:;
				label$165:;
				NEW_FOOD_POS_( CTX$1 );
				*(int8*)((uint8*)CTX$1 + 167ll) = (int8)((int64)*(int8*)((uint8*)CTX$1 + 167ll) + 1ll);
				*(uint32*)((uint8*)CTX$1 + 168ll) = (uint32)((int64)*(uint32*)((uint8*)CTX$1 + 168ll) + 1ll);
			}
		}
		label$164:;
		label$163:;
	}
	label$125:;
}

int32 HANDLE_KEY_EVENT_( struct $12SNAKECONTEXT* CTX$1, int32 KEY_CODE$1 )
{
	_unusedlabel: ; void *_llvmbug18658 = &&_unusedlabel;
	int32 fb$result$1;
	__builtin_memset( &fb$result$1, 0, 4ll );
	label$168:;
	{
		{
			if( (int64)KEY_CODE$1 == 41ll) goto label$172;
			label$173:;
			if( (int64)KEY_CODE$1 != 20ll) goto label$171;
			label$172:;
			{
				goto label$174;
			}
			goto label$170;
			label$171:;
			if( (int64)KEY_CODE$1 != 21ll) goto label$175;
			label$176:;
			{
				goto label$177;
			}
			goto label$170;
			label$175:;
			if( (int64)KEY_CODE$1 != 79ll) goto label$178;
			label$179:;
			{
				goto label$180;
			}
			goto label$170;
			label$178:;
			if( (int64)KEY_CODE$1 != 82ll) goto label$181;
			label$182:;
			{
				goto label$183;
			}
			goto label$170;
			label$181:;
			if( (int64)KEY_CODE$1 != 80ll) goto label$184;
			label$185:;
			{
				goto label$186;
			}
			goto label$170;
			label$184:;
			if( (int64)KEY_CODE$1 != 81ll) goto label$187;
			label$188:;
			{
				goto label$189;
			}
			goto label$170;
			label$187:;
			{
				goto label$191;
			}
			label$190:;
			label$170:;
		}
		label$174:;
		{
			fb$result$1 = 1;
			goto label$169;
		}
		label$177:;
		{
			SNAKE_INITIALIZE( CTX$1 );
			goto label$192;
		}
		label$180:;
		{
			SNAKE_REDIR( CTX$1, 0ll );
			goto label$192;
		}
		label$183:;
		{
			SNAKE_REDIR( CTX$1, 1ll );
			goto label$192;
		}
		label$186:;
		{
			SNAKE_REDIR( CTX$1, 2ll );
			goto label$192;
		}
		label$189:;
		{
			SNAKE_REDIR( CTX$1, 3ll );
			goto label$192;
		}
		label$191:;
		{
			goto label$192;
		}
		label$192:;
		fb$result$1 = 0;
		goto label$169;
	}
	label$169:;
	return fb$result$1;
}

int32 HANDLE_HAT_EVENT_( struct $12SNAKECONTEXT* CTX$1, uint8 HAT$1 )
{
	_unusedlabel: ; void *_llvmbug18658 = &&_unusedlabel;
	int32 fb$result$1;
	__builtin_memset( &fb$result$1, 0, 4ll );
	label$193:;
	{
		{
			if( (uint64)(int64)HAT$1 != 2ull) goto label$196;
			label$197:;
			{
				goto label$198;
			}
			goto label$195;
			label$196:;
			if( (uint64)(int64)HAT$1 != 1ull) goto label$199;
			label$200:;
			{
				goto label$201;
			}
			goto label$195;
			label$199:;
			if( (uint64)(int64)HAT$1 != 8ull) goto label$202;
			label$203:;
			{
				goto label$204;
			}
			goto label$195;
			label$202:;
			if( (uint64)(int64)HAT$1 != 4ull) goto label$205;
			label$206:;
			{
				goto label$207;
			}
			goto label$195;
			label$205:;
			{
				goto label$209;
			}
			label$208:;
			label$195:;
		}
		label$198:;
		{
			SNAKE_REDIR( CTX$1, 0ll );
			goto label$210;
		}
		label$201:;
		{
			SNAKE_REDIR( CTX$1, 1ll );
			goto label$210;
		}
		label$204:;
		{
			SNAKE_REDIR( CTX$1, 2ll );
			goto label$210;
		}
		label$207:;
		{
			SNAKE_REDIR( CTX$1, 3ll );
			goto label$210;
		}
		label$209:;
		{
			goto label$210;
		}
		label$210:;
		fb$result$1 = 0;
		goto label$194;
	}
	label$194:;
	return fb$result$1;
}

int32 SDL_APPITERATE( void* APPSTATE$1 )
{
	_unusedlabel: ; void *_llvmbug18658 = &&_unusedlabel;
	int32 fb$result$1;
	__builtin_memset( &fb$result$1, 0, 4ll );
	label$211:;
	{
		struct $8APPSTATE* AS_$2;
		AS_$2 = (struct $8APPSTATE*)APPSTATE$1;
		struct $12SNAKECONTEXT* CTX$2;
		CTX$2 = (struct $12SNAKECONTEXT*)((uint8*)AS_$2 + 16ll);
		uint64 NOW$2;
		uint64 vr$3 = SDL_GetTicks(  );
		NOW$2 = vr$3;
		struct $9SDL_FRect R$2;
		__builtin_memset( &R$2, 0, 16ll );
		uint32 I$2;
		__builtin_memset( &I$2, 0, 4ll );
		uint32 J$2;
		__builtin_memset( &J$2, 0, 4ll );
		int32 CT$2;
		__builtin_memset( &CT$2, 0, 4ll );
		label$213:;
		{
			if( (NOW$2 - *(uint64*)((uint8*)AS_$2 + 192ll)) >= 125ull) goto label$217;
			{
				goto label$214;
				label$217:;
			}
			{
				SNAKE_STEP( CTX$2 );
				*(uint64*)((uint8*)AS_$2 + 192ll) = *(uint64*)((uint8*)AS_$2 + 192ll) + 125ull;
			}
		}
		label$215:;
		goto label$213;
		label$214:;
		*(float*)((uint8*)&R$2 + 12ll) = 0x1.8p+4f;
		*(float*)((uint8*)&R$2 + 8ll) = *(float*)((uint8*)&R$2 + 12ll);
		SDL_SetRenderDrawColor( *(void**)((uint8*)AS_$2 + 8ll), (uint8)0u, (uint8)0u, (uint8)0u, (uint8)255u );
		SDL_RenderClear( *(void**)((uint8*)AS_$2 + 8ll) );
		{
			I$2 = 0u;
			label$218:;
			if( (uint64)(int64)I$2 >= 24ull) goto label$219;
			{
				{
					{
						J$2 = 0u;
						label$220:;
						if( (uint64)(int64)J$2 >= 18ull) goto label$221;
						{
							{
								$9SNAKECELL vr$19 = SNAKE_CELL_AT( (struct $12SNAKECONTEXT*)CTX$2, (int8)I$2, (int8)J$2 );
								CT$2 = (int32)vr$19;
								if( (int64)CT$2 != 0ll) goto label$223;
								{
									goto label$224;
								}
								label$223:;
								label$222:;
								SET_RECT_XY_( &R$2, (int16)I$2, (int16)J$2 );
								if( (int64)CT$2 != 5ll) goto label$226;
								{
									SDL_SetRenderDrawColor( *(void**)((uint8*)AS_$2 + 8ll), (uint8)80u, (uint8)80u, (uint8)255u, (uint8)255u );
								}
								goto label$225;
								label$226:;
								{
									SDL_SetRenderDrawColor( *(void**)((uint8*)AS_$2 + 8ll), (uint8)0u, (uint8)128u, (uint8)0u, (uint8)255u );
								}
								label$225:;
								SDL_RenderFillRect( *(void**)((uint8*)AS_$2 + 8ll), (struct $9SDL_FRect*)&R$2 );
							}
							label$224:;
							J$2 = (uint32)((int64)J$2 + 1ll);
						}
						goto label$220;
						label$221:;
					}
				}
				I$2 = (uint32)((int64)I$2 + 1ll);
			}
			goto label$218;
			label$219:;
		}
		SDL_SetRenderDrawColor( *(void**)((uint8*)AS_$2 + 8ll), (uint8)255u, (uint8)255u, (uint8)0u, (uint8)255u );
		SET_RECT_XY_( &R$2, (int16)*(int8*)((uint8*)CTX$2 + 162ll), (int16)*(int8*)((uint8*)CTX$2 + 163ll) );
		SDL_RenderFillRect( *(void**)((uint8*)AS_$2 + 8ll), (struct $9SDL_FRect*)&R$2 );
		SDL_RenderPresent( *(void**)((uint8*)AS_$2 + 8ll) );
		fb$result$1 = 0;
		goto label$212;
	}
	label$212:;
	return fb$result$1;
}

int32 SDL_APPINIT( void** APPSTATE$1, int32 ARGC$1, char** ARGV$1 )
{
	_unusedlabel: ; void *_llvmbug18658 = &&_unusedlabel;
	int32 fb$result$1;
	__builtin_memset( &fb$result$1, 0, 4ll );
	label$227:;
	{
		uint64 I$2;
		__builtin_memset( &I$2, 0, 8ll );
		boolean vr$2 = SDL_SetAppMetadata( (char*)"Example Snake game", (char*)"1.0", (char*)"com.example.Snake" );
		if( (int64)-vr$2 != 0ll) goto label$230;
		{
			{
				fb$result$1 = 2;
				goto label$228;
			}
		}
		label$230:;
		label$229:;
		{
			I$2 = 0ull;
			label$231:;
			if( I$2 >= 4ull) goto label$232;
			{
				{
					boolean vr$6 = SDL_SetAppMetadataProperty( *(char**)((uint64)(struct $22EXTENDEDMETADATARECORD*)EXTENDED_METADATA$ + ((int64)I$2 << (4ll & 63ll))), *(char**)(((uint64)(struct $22EXTENDEDMETADATARECORD*)EXTENDED_METADATA$ + ((int64)I$2 << (4ll & 63ll))) + 8ll) );
					if( (int64)-vr$6 != 0ll) goto label$234;
					{
						{
							fb$result$1 = 2;
							goto label$228;
						}
					}
					label$234:;
					label$233:;
				}
				I$2 = I$2 + 1ull;
			}
			goto label$231;
			label$232:;
		}
		boolean vr$9 = SDL_Init( 544u );
		if( (int64)-vr$9 != 0ll) goto label$236;
		{
			{
				const char* vr$11 = SDL_GetError(  );
				tmp$204( (char*)"Couldn't initialize SDL: %s", vr$11 );
				fb$result$1 = 2;
				goto label$228;
			}
		}
		label$236:;
		label$235:;
		struct $8APPSTATE* AS_$2;
		void* vr$12 = SDL_calloc( 1ull, 200ull );
		AS_$2 = (struct $8APPSTATE*)vr$12;
		if( AS_$2 != (struct $8APPSTATE*)0ull) goto label$238;
		{
			{
				fb$result$1 = 2;
				goto label$228;
			}
		}
		label$238:;
		label$237:;
		*APPSTATE$1 = (void*)AS_$2;
		boolean vr$18 = SDL_CreateWindowAndRenderer( (char*)"examples/demo/snake", 576, 432, 32ull, (void**)AS_$2, (void**)((uint8*)AS_$2 + 8ll) );
		if( (int64)-vr$18 != 0ll) goto label$240;
		{
			{
				fb$result$1 = 2;
				goto label$228;
			}
		}
		label$240:;
		label$239:;
		SDL_SetRenderLogicalPresentation( *(void**)((uint8*)AS_$2 + 8ll), 576, 432, 2 );
		SNAKE_INITIALIZE( (struct $12SNAKECONTEXT*)((uint8*)AS_$2 + 16ll) );
		uint64 vr$23 = SDL_GetTicks(  );
		*(uint64*)((uint8*)AS_$2 + 192ll) = vr$23;
		fb$result$1 = 0;
		goto label$228;
	}
	label$228:;
	return fb$result$1;
}

int32 SDL_APPEVENT( void* APPSTATE$1, union $9SDL_Event* EVENT$1 )
{
	_unusedlabel: ; void *_llvmbug18658 = &&_unusedlabel;
	int32 fb$result$1;
	__builtin_memset( &fb$result$1, 0, 4ll );
	label$241:;
	{
		struct $12SNAKECONTEXT* CTX$2;
		CTX$2 = (struct $12SNAKECONTEXT*)((uint8*)APPSTATE$1 + 16ll);
		{
			uint32 TMP$205$3;
			TMP$205$3 = *(uint32*)EVENT$1;
			if( (int64)TMP$205$3 != 256ll) goto label$244;
			label$245:;
			{
				goto label$246;
			}
			goto label$243;
			label$244:;
			if( (int64)TMP$205$3 != 1541ll) goto label$247;
			label$248:;
			{
				goto label$249;
			}
			goto label$243;
			label$247:;
			if( (int64)TMP$205$3 != 1542ll) goto label$250;
			label$251:;
			{
				goto label$252;
			}
			goto label$243;
			label$250:;
			if( (int64)TMP$205$3 != 1538ll) goto label$253;
			label$254:;
			{
				goto label$255;
			}
			goto label$243;
			label$253:;
			if( (int64)TMP$205$3 != 768ll) goto label$256;
			label$257:;
			{
				goto label$258;
			}
			goto label$243;
			label$256:;
			{
				goto label$260;
			}
			label$259:;
			label$243:;
		}
		label$246:;
		{
			fb$result$1 = 1;
			goto label$242;
		}
		label$249:;
		{
			if( JOYSTICK$ != (void*)0ull) goto label$262;
			{
				{
					void* vr$10 = SDL_OpenJoystick( *(uint32*)((uint8*)EVENT$1 + 16ll) );
					JOYSTICK$ = vr$10;
					if( JOYSTICK$ != (void*)0ull) goto label$264;
					{
						{
							const char* vr$11 = SDL_GetError(  );
							tmp$204( (char*)"Failed to open joystick ID %u: %s", *(uint32*)((uint8*)EVENT$1 + 16ll), vr$11 );
						}
					}
					label$264:;
					label$263:;
				}
			}
			label$262:;
			label$261:;
			goto label$265;
		}
		label$252:;
		{
			int64 TMP$207$3;
			if( JOYSTICK$ == (void*)0ull) goto label$266;
			uint32 vr$13 = SDL_GetJoystickID( JOYSTICK$ );
			TMP$207$3 = (int64)-((void*)(int64)-((int64)vr$13 == (int64)*(uint32*)((uint8*)EVENT$1 + 16ll)) != (void*)0ull);
			goto label$269;
			label$266:;
			TMP$207$3 = 0ll;
			label$269:;
			if( TMP$207$3 == 0ll) goto label$268;
			{
				{
					SDL_CloseJoystick( JOYSTICK$ );
					JOYSTICK$ = (void*)0ull;
				}
			}
			label$268:;
			label$267:;
			goto label$265;
		}
		label$255:;
		{
			int32 vr$20 = HANDLE_HAT_EVENT_( CTX$2, *(uint8*)((uint8*)EVENT$1 + 21ll) );
			fb$result$1 = vr$20;
			goto label$242;
		}
		label$258:;
		{
			int32 vr$22 = HANDLE_KEY_EVENT_( CTX$2, *(int32*)((uint8*)EVENT$1 + 24ll) );
			fb$result$1 = vr$22;
			goto label$242;
		}
		label$260:;
		{
			goto label$265;
		}
		label$265:;
		fb$result$1 = 0;
		goto label$242;
	}
	label$242:;
	return fb$result$1;
}

void SDL_APPQUIT( void* APPSTATE$1, int32 RESULT$1 )
{
	_unusedlabel: ; void *_llvmbug18658 = &&_unusedlabel;
	label$270:;
	{
		if( JOYSTICK$ == (void*)0ull) goto label$273;
		{
			{
				SDL_CloseJoystick( JOYSTICK$ );
			}
		}
		label$273:;
		label$272:;
		if( APPSTATE$1 == (void*)0ull) goto label$275;
		{
			{
				struct $8APPSTATE* AS_$4;
				AS_$4 = (struct $8APPSTATE*)APPSTATE$1;
				SDL_DestroyRenderer( *(void**)((uint8*)AS_$4 + 8ll) );
				SDL_DestroyWindow( *(void**)AS_$4 );
				SDL_free( (void*)AS_$4 );
			}
		}
		label$275:;
		label$274:;
	}
	label$271:;
}

__attribute__(( constructor )) static void fb_ctor__snake( void )
{
	_unusedlabel: ; void *_llvmbug18658 = &&_unusedlabel;
	label$0:;
	int64 __FB_ARGC__$0;
	__builtin_memset( &__FB_ARGC__$0, 0, 8ll );
	int64 __FB_ARGV__$0;
	__builtin_memset( &__FB_ARGV__$0, 0, 8ll );
	int32 vr$3 = SDL_RunApp( (int32)__FB_ARGC__$0, (char**)__FB_ARGV__$0, (tmp$210)&SDL3_EXAMPLEMAIN, (void*)0ull );
	fb_End( vr$3 );
	label$1:;
}

static uint32 SDL_SWAP32( uint32 VALUE$1 )
{
	_unusedlabel: ; void *_llvmbug18658 = &&_unusedlabel;
	uint32 fb$result$1;
	__builtin_memset( &fb$result$1, 0, 4ll );
	label$4:;
	fb$result$1 = (uint32)((((((uint64)(int64)VALUE$1 & 255ull) << (24ll & 63ll)) | (((uint64)(int64)VALUE$1 & 65280ull) << (8ll & 63ll))) | (((uint64)(int64)VALUE$1 & 16711680ull) >> (8ll & 63ll))) | (((uint64)(int64)VALUE$1 & 4278190080ull) >> (24ll & 63ll)));
	goto label$5;
	label$5:;
	return fb$result$1;
}

static boolean SDL_RECTSEQUALEPSILON( const struct $9SDL_FRect* A$1, const struct $9SDL_FRect* B$1, float EPSILON$1 )
{
	_unusedlabel: ; void *_llvmbug18658 = &&_unusedlabel;
	int64 TMP$171$1;
	int64 TMP$172$1;
	int64 TMP$173$1;
	int64 TMP$174$1;
	boolean fb$result$1;
	__builtin_memset( &fb$result$1, 0, 1ll );
	label$67:;
	if( A$1 == (struct $9SDL_FRect*)0ull) goto label$69;
	TMP$171$1 = (int64)-(B$1 == (struct $9SDL_FRect*)0ull);
	goto label$309;
	label$69:;
	TMP$171$1 = -1ll;
	label$309:;
	if( TMP$171$1 == 0ll) goto label$71;
	{
		fb$result$1 = (boolean)0ll;
		goto label$68;
		label$71:;
	}
	if( A$1 != B$1) goto label$73;
	{
		fb$result$1 = (boolean)1ll;
		goto label$68;
		label$73:;
	}
	float vr$5 = SDL_fabsf( *(float*)A$1 - *(float*)B$1 );
	if( !(vr$5 <= EPSILON$1)) goto label$74;
	float vr$9 = SDL_fabsf( *(float*)((uint8*)A$1 + 4ll) - *(float*)((uint8*)B$1 + 4ll) );
	TMP$172$1 = (int64)-(vr$9 <= EPSILON$1);
	goto label$310;
	label$74:;
	TMP$172$1 = 0ll;
	label$310:;
	if( TMP$172$1 == 0ll) goto label$75;
	float vr$14 = SDL_fabsf( *(float*)((uint8*)A$1 + 8ll) - *(float*)((uint8*)B$1 + 8ll) );
	TMP$173$1 = (int64)-(vr$14 <= EPSILON$1);
	goto label$311;
	label$75:;
	TMP$173$1 = 0ll;
	label$311:;
	if( TMP$173$1 == 0ll) goto label$76;
	float vr$19 = SDL_fabsf( *(float*)((uint8*)A$1 + 12ll) - *(float*)((uint8*)B$1 + 12ll) );
	TMP$174$1 = (int64)-(vr$19 <= EPSILON$1);
	goto label$312;
	label$76:;
	TMP$174$1 = 0ll;
	label$312:;
	fb$result$1 = (boolean)(TMP$174$1 != 0ll);
	goto label$68;
	label$68:;
	return fb$result$1;
}

static int32 SDL3_EXAMPLEITERATE( void* APPSTATE$1 )
{
	_unusedlabel: ; void *_llvmbug18658 = &&_unusedlabel;
	int32 fb$result$1;
	__builtin_memset( &fb$result$1, 0, 4ll );
	label$276:;
	static uint32 FRAMES$1;
	static int64 FRAME_LIMIT$1;
	static boolean INITIALIZED$1;
	if( (boolean)!INITIALIZED$1 == (boolean)0ll) goto label$279;
	{
		FBSTRING* vr$2 = fb_StrAllocTempDescZEx( (char*)"FB_SDL3_SMOKE_FRAMES", 20ll );
		FBSTRING* vr$3 = fb_GetEnviron( (FBSTRING*)vr$2 );
		int32 vr$4 = fb_VALINT( (FBSTRING*)vr$3 );
		FRAME_LIMIT$1 = (int64)vr$4;
		INITIALIZED$1 = (boolean)1ll;
	}
	label$279:;
	label$278:;
	int32 RESULT$1;
	int32 vr$6 = SDL_APPITERATE( APPSTATE$1 );
	RESULT$1 = vr$6;
	if( (int64)RESULT$1 == 0ll) goto label$281;
	{
		fb$result$1 = RESULT$1;
		goto label$277;
		label$281:;
	}
	FBSTRING* vr$8 = fb_Command( 1 );
	int32 vr$9 = fb_StrCompare( (void*)vr$8, -1ll, (void*)"--smoke", 8ll );
	if( (int64)vr$9 != 0ll) goto label$283;
	{
		FRAME_LIMIT$1 = 16ll;
		label$283:;
	}
	if( FRAME_LIMIT$1 <= 0ll) goto label$285;
	{
		FRAMES$1 = (uint32)((int64)FRAMES$1 + 1ll);
		if( (uint64)(int64)FRAMES$1 < (uint64)FRAME_LIMIT$1) goto label$287;
		{
			fb$result$1 = 1;
			goto label$277;
			label$287:;
		}
	}
	label$285:;
	label$284:;
	fb$result$1 = 0;
	goto label$277;
	label$277:;
	return fb$result$1;
}

static int32 SDL3_EXAMPLEMAIN( int32 ARGC$1, char** ARGV$1 )
{
	_unusedlabel: ; void *_llvmbug18658 = &&_unusedlabel;
	int32 fb$result$1;
	__builtin_memset( &fb$result$1, 0, 4ll );
	label$288:;
	int32 vr$1 = SDL_EnterAppMainCallbacks( ARGC$1, ARGV$1, (tmp$211)&SDL_APPINIT, (tmp$212)&SDL3_EXAMPLEITERATE, (tmp$213)&SDL_APPEVENT, (tmp$214)&SDL_APPQUIT );
	fb$result$1 = vr$1;
	goto label$289;
	label$289:;
	return fb$result$1;
}

static const char __attribute__((used, section(".fbctinf"))) __fbctinf[] = "-l\0SDL3\0-l\0kernel32\0-lang\0deprecated";

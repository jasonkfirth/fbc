/*
    FreeBASIC Windows CE OMA launch runner
    ---------------------------------------

    File: tests/wince/oma-runner.c

    Purpose:

        Launch one staged OMA game, drive it from its title screen into
        gameplay, and report whether it remains alive on Windows CE.

    Responsibilities:

        - read the game key and directory from the CERF shared folder
        - launch the game's executable without desktop-shell dependencies
        - activate the game window and deliver a title-specific input script
        - distinguish stable gameplay from an early process exit
        - publish the result before creating the completion marker

    This file intentionally does NOT contain:

        - OMA source or asset discovery
        - target compilation or package construction
        - emulator startup and shutdown
        - title or gameplay implementation
*/

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>

#ifndef OMA_STABILITY_MS
#define OMA_STABILITY_MS 8000
#endif

#define MAX_CONTROL_VALUE 96
#define MAX_PROGRAM_ARGUMENTS 96
#define MAX_PROGRAM_PATH 260
#define OBJECT_STORE_FLOOR_BYTES (16UL * 1024UL * 1024UL)
#define PROGRAM_MEMORY_IMAGE_FLOOR_BYTES (900UL * 1024UL)

#define ARRAY_COUNT(items) (sizeof(items) / sizeof((items)[0]))
#define INPUT_STEP_DIRECT_CHAR 1U
#define WINDOW_POLL_COUNT 100
#define WINDOW_POLL_MS 100

typedef struct InputStep {
	BYTE virtual_key;
	DWORD hold_ms;
	DWORD delay_ms;
	int mouse_x;
	int mouse_y;
	unsigned int flags;
	unsigned int repeat_count;
} InputStep;

typedef struct InputScript {
	const char *game_key;
	const char *name;
	const InputStep *steps;
	size_t step_count;
	DWORD startup_ms;
	DWORD settle_ms;
} InputScript;

/* CeGCC omits these coredll declarations even though Windows CE exports them. */
WINBASEAPI BOOL WINAPI GetSystemMemoryDivision( LPDWORD store_pages,
	                                             LPDWORD ram_pages,
	                                             LPDWORD page_size );
WINBASEAPI DWORD WINAPI SetSystemMemoryDivision( DWORD store_pages );

static const char control_path[] = "\\Storage Card\\oma-launch.txt";
static const char result_path[] = "\\Storage Card\\oma.result";
static const char completion_path[] = "\\Storage Card\\oma.done";
static const char trace_path[] = "\\Storage Card\\oma.trace";
static const wchar_t gfx_window_class[] = L"FreeBASICGfxWinCE";
static const wchar_t console_window_title[] = L"Console";
static const wchar_t application_error_title[] = L"Application Error";

static void write_checkpoint( const char *checkpoint )
{
	FILE *trace;

	trace = fopen( trace_path, "wb" );
	if( trace == NULL )
		return;
	fprintf( trace, "%s\r\n", checkpoint );
	fclose( trace );
}

static void rebalance_program_memory( void )
{
	DWORD page_size = 0;
	DWORD ram_pages = 0;
	DWORD store_pages = 0;
	DWORD target_store_pages;

	if( !GetSystemMemoryDivision( &store_pages, &ram_pages, &page_size ) ||
	    page_size == 0 )
		return;

	target_store_pages = OBJECT_STORE_FLOOR_BYTES / page_size;
	if( (OBJECT_STORE_FLOOR_BYTES % page_size) != 0 )
		target_store_pages += 1;

	/*
	    OMA assets live on the external Storage Card.  Keep a useful object
	    store, but return any excess pages to program memory before CE loads a
	    substantial statically-linked game image.
	*/
	if( target_store_pages > 0 && store_pages > target_store_pages )
		SetSystemMemoryDivision( target_store_pages );
}

static void prepare_program_memory( const wchar_t *program_path,
	                                const char *game_directory )
{
	WIN32_FILE_ATTRIBUTE_DATA attributes;

	memset( &attributes, 0, sizeof( attributes ) );
	if( !GetFileAttributesExW( program_path, GetFileExInfoStandard,
	                           &attributes ) )
		return;

	/*
	    Behold is a small image, but it replaces an indexed Screen mode with a
	    two-page 32-bit ScreenRes mode during startup.  Duel is also small, but
	    starts with a two-page 640x480 screen before allocating rotated sprites.
	    Star Phalanx likewise creates a two-page screen from a compact image.
	    Those graphics peaks need the same program-memory headroom as large
	    images.
	*/
	if( attributes.nFileSizeHigh != 0 ||
	    attributes.nFileSizeLow >= PROGRAM_MEMORY_IMAGE_FLOOR_BYTES ||
	    strcmp( game_directory, "OMA-Behold" ) == 0 ||
	    strcmp( game_directory, "OMA-Duel999" ) == 0 ||
	    strcmp( game_directory, "OMA-StarPhalanx" ) == 0 )
		rebalance_program_memory();
}

/*
    Gameplay scripts

    These scripts describe public keyboard controls already accepted by each
    title.  They live in the platform test harness so the original games do
    not need test-only branches or command-line options.  Navigation keys
    after the start action exercise a live game loop rather than stopping at
    the first non-title frame.
*/
static const InputStep behold_steps[] = {
	{ '1', 220, 1200, 0, 0 },
	{ '1', 220, 900, 0, 0 },
	{ VK_RIGHT, 500, 700, 0, 0 },
	{ VK_SPACE, 220, 1500, 0, 0 }
};

static const InputStep duel999_steps[] = {
	{ VK_UP, 900, 700, 0, 0 },
	{ VK_RIGHT, 700, 700, 0, 0 },
	{ VK_SPACE, 220, 1500, 0, 0 }
};

static const InputStep kinematics_steps[] = {
	{ 'R', 220, 1500, 0, 0 }
};

static const InputStep jrpg_steps[] = {
	{ VK_SPACE, 220, 4500, 0, 0 },
	{ VK_RIGHT, 500, 700, 0, 0 },
	{ VK_DOWN, 500, 1500, 0, 0 }
};

static const InputStep qfak_steps[] = {
	{ VK_SPACE, 50, 500, 0, 0, INPUT_STEP_DIRECT_CHAR, 120 },
	{ VK_RIGHT, 500, 700, 0, 0 },
	{ VK_DOWN, 500, 1500, 0, 0 }
};

static const InputStep rambo_steps[] = {
	{ VK_SPACE, 220, 1800, 0, 0 },
	{ VK_RIGHT, 700, 700, 0, 0 },
	{ VK_SPACE, 220, 1500, 0, 0 }
};

static const InputStep starphalanx_steps[] = {
	{ VK_SPACE, 700, 3500, 0, 0 },
	{ VK_RIGHT, 600, 700, 0, 0 },
	{ VK_SPACE, 700, 1500, 0, 0 }
};

static const InputStep openmarket_steps[] = {
	{ VK_RETURN, 220, 1800, 0, 0, INPUT_STEP_DIRECT_CHAR },
	{ VK_RETURN, 220, 1400, 0, 0, INPUT_STEP_DIRECT_CHAR },
	{ 'P', 180, 400, 0, 0, INPUT_STEP_DIRECT_CHAR },
	{ '1', 180, 800, 0, 0, INPUT_STEP_DIRECT_CHAR },
	{ VK_RETURN, 220, 1800, 0, 0, INPUT_STEP_DIRECT_CHAR },
	{ VK_RETURN, 220, 1800, 0, 0, INPUT_STEP_DIRECT_CHAR },
	{ VK_RETURN, 220, 1800, 0, 0, INPUT_STEP_DIRECT_CHAR },
	{ VK_RETURN, 220, 2500, 0, 0, INPUT_STEP_DIRECT_CHAR },
	{ VK_RIGHT, 500, 1500, 0, 0 }
};

static const InputStep openslicks_steps[] = {
	{ VK_RETURN, 220, 2200, 0, 0 },
	{ VK_UP, 1200, 1500, 0, 0 }
};

static const InputScript input_scripts[] = {
	{ "behold", "start-1-move-fire", behold_steps,
	  ARRAY_COUNT(behold_steps), 5000, 5000 },
	{ "duel999", "autostart-local-move-fire", duel999_steps,
	  ARRAY_COUNT(duel999_steps), 60000, 5000 },
	{ "kinematics", "reset-simulation", kinematics_steps,
	  ARRAY_COUNT(kinematics_steps), 3000, 3000 },
	{ "nietzsche", "start-and-walk", jrpg_steps,
	  ARRAY_COUNT(jrpg_steps), 5000, 5000 },
	{ "qfak", "start-and-walk", qfak_steps,
	  ARRAY_COUNT(qfak_steps), 60000, 5000 },
	{ "rambo", "start-move-fire", rambo_steps,
	  ARRAY_COUNT(rambo_steps), 5000, 5000 },
	{ "starphalanx", "new-game-move-fire", starphalanx_steps,
	  ARRAY_COUNT(starphalanx_steps), 60000, 5000 },
	{ "openmarket", "title-new-game-board", openmarket_steps,
	  ARRAY_COUNT(openmarket_steps), 5000, 5000 },
	{ "openslicks", "start-race-accelerate", openslicks_steps,
	  ARRAY_COUNT(openslicks_steps), 5000, 5000 }
};

static void trim_line( char *text )
{
	size_t length;

	length = strlen( text );
	while( length > 0 && (text[length - 1] == '\r' ||
	                     text[length - 1] == '\n') ) {
		text[length - 1] = '\0';
		length -= 1;
	}
}

static int is_safe_component( const char *text )
{
	const unsigned char *cursor;

	if( text == NULL || text[0] == '\0' )
		return 0;

	for( cursor = (const unsigned char *)text; *cursor != '\0'; ++cursor ) {
		if( !isalnum( *cursor ) && *cursor != '-' && *cursor != '_' )
			return 0;
	}
	return 1;
}

static int arguments_are_safe( const char *text )
{
	const unsigned char *cursor;

	for( cursor = (const unsigned char *)text; *cursor != '\0'; ++cursor ) {
		if( !isalnum( *cursor ) && *cursor != '-' && *cursor != '_' &&
		    *cursor != ' ' )
			return 0;
	}
	return 1;
}

static int widen_ascii( const char *source, wchar_t *destination,
	                    size_t destination_chars )
{
	size_t index;
	size_t length;

	if( source == NULL || destination == NULL || destination_chars == 0 )
		return 0;

	length = strlen( source );
	if( length >= destination_chars )
		return 0;

	for( index = 0; index <= length; ++index )
		destination[index] = (unsigned char)source[index];

	return 1;
}

static const InputScript *find_input_script( const char *game_key )
{
	size_t index;

	for( index = 0; index < ARRAY_COUNT(input_scripts); ++index ) {
		if( strcmp( input_scripts[index].game_key, game_key ) == 0 )
			return &input_scripts[index];
	}
	return NULL;
}

static HWND find_process_window( DWORD process_id )
{
	DWORD foreground_process_id = 0;
	HWND window;

	/*
	    Windows CE presents one foreground application rather than a desktop
	    stack.  Prefer that fast path, then use gfxlib's stable window class for
	    small windowed titles that do not own the foreground slot.  This avoids
	    EnumWindows, which can block behind a graphics window-manager lock.
	*/
	window = GetForegroundWindow();
	if( window != NULL && IsWindowVisible( window ) ) {
		GetWindowThreadProcessId( window, &foreground_process_id );
		if( foreground_process_id == process_id )
			return window;
	}

	window = FindWindowW( gfx_window_class, NULL );
	if( window != NULL && IsWindowVisible( window ) ) {
		GetWindowThreadProcessId( window, &foreground_process_id );
		if( foreground_process_id == process_id )
			return window;
	}

	/*
	    WinCE hosts console UI in a shell-owned window rather than in the
	    process that opened the console.  A clean emulator has no Console
	    window until a console title creates one, so this remains a narrow
	    fallback while allowing text-mode games to receive scripted input.
	*/
	window = FindWindowW( NULL, console_window_title );
	if( window != NULL && IsWindowVisible( window ) )
		return window;
	return NULL;
}

static HWND wait_for_process_window( DWORD process_id )
{
	unsigned int attempt;
	HWND window;

	for( attempt = 0; attempt < WINDOW_POLL_COUNT; ++attempt ) {
		window = find_process_window( process_id );
		if( window != NULL )
			return window;
		Sleep( WINDOW_POLL_MS );
	}
	return NULL;
}

static int process_is_alive( HANDLE process, DWORD *exit_code )
{
	if( FindWindowW( NULL, application_error_title ) != NULL ) {
		*exit_code = ERROR_PROCESS_ABORTED;
		return 0;
	}
	if( !GetExitCodeProcess( process, exit_code ) )
		return 0;
	return *exit_code == STILL_ACTIVE;
}

static WPARAM character_for_virtual_key( BYTE virtual_key )
{
	if( virtual_key >= '0' && virtual_key <= '9' )
		return virtual_key;
	if( virtual_key >= 'A' && virtual_key <= 'Z' )
		return virtual_key;
	if( virtual_key == VK_SPACE )
		return ' ';
	if( virtual_key == VK_RETURN )
		return '\r';
	return 0;
}

static unsigned int input_step_repeat_count( const InputStep *step )
{
	if( step->repeat_count == 0 )
		return 1;
	return step->repeat_count;
}

static unsigned int expected_input_count( const InputScript *script )
{
	unsigned int count = 0;
	unsigned int repeat_count;
	size_t index;

	for( index = 0; index < script->step_count; ++index ) {
		repeat_count = input_step_repeat_count( &script->steps[index] );
		if( repeat_count > (~0U - count) )
			return ~0U;
		count += repeat_count;
	}
	return count;
}

static unsigned int run_input_script( const InputScript *script,
	                                  HANDLE process, DWORD process_id,
	                                  int *window_seen,
	                                  DWORD *exit_code )
{
	char checkpoint[32];
	unsigned int delivered = 0;
	unsigned int repeat_count;
	unsigned int repetition;
	size_t index;
	HWND window;
	WPARAM character;

	Sleep( script->startup_ms );
	window = find_process_window( process_id );
	if( window != NULL )
		*window_seen = 1;

	for( index = 0; index < script->step_count; ++index ) {
		if( !process_is_alive( process, exit_code ) )
			break;
		repeat_count = input_step_repeat_count( &script->steps[index] );

		for( repetition = 0; repetition < repeat_count; ++repetition ) {
			if( !process_is_alive( process, exit_code ) )
				goto input_complete;

			/*
			    A WinCE console belongs to the shell rather than to the game process.
			    Repeatedly finding and foregrounding that window can synchronously wait
			    for a busy console game.  Direct character delivery only needs the
			    handle captured after startup, so retain it across those steps.
			*/
			if( (script->steps[index].flags & INPUT_STEP_DIRECT_CHAR) == 0 ||
			    window == NULL ) {
				window = find_process_window( process_id );
				if( window != NULL ) {
					*window_seen = 1;
					SetForegroundWindow( window );
				}
			}

			/*
			    keybd_event updates the system key state as well as the active
			    window's message queue.  This serves both legacy Inkey loops and
			    newer games that poll MultiKey/GetAsyncKeyState.
			*/
			if( script->steps[index].virtual_key == 0 ) {
				LPARAM position;

				if( window == NULL )
					goto input_complete;
				position = MAKELPARAM( script->steps[index].mouse_x,
				                           script->steps[index].mouse_y );
				PostMessageW( window, WM_MOUSEMOVE, 0, position );
				PostMessageW( window, WM_LBUTTONDOWN, MK_LBUTTON, position );
				Sleep( script->steps[index].hold_ms );
				PostMessageW( window, WM_LBUTTONUP, 0, position );
			} else {
				if( (script->steps[index].flags &
				     INPUT_STEP_DIRECT_CHAR) == 0 ) {
					keybd_event( script->steps[index].virtual_key, 0, 0, 0 );
				}
				character = character_for_virtual_key(
				    script->steps[index].virtual_key );
				if( window != NULL && character != 0 ) {
					/*
					    Some DOS-era titles consume Inkey characters directly rather
					    than polling key state.  WinCE's synthetic key path does not
					    guarantee translation to WM_CHAR, so provide that character to
					    the already-identified game window explicitly.
					*/
					PostMessageW( window, WM_CHAR, character, 0 );
				}
				Sleep( script->steps[index].hold_ms );
				if( (script->steps[index].flags &
				     INPUT_STEP_DIRECT_CHAR) == 0 ) {
					keybd_event( script->steps[index].virtual_key, 0,
					             KEYEVENTF_KEYUP, 0 );
				}
			}
			delivered += 1;
			sprintf( checkpoint, "input-%u", delivered );
			write_checkpoint( checkpoint );
			Sleep( script->steps[index].delay_ms );
		}
	}

input_complete:
	window = wait_for_process_window( process_id );
	if( window != NULL ) {
		*window_seen = 1;
		SetForegroundWindow( window );
	}
	Sleep( script->settle_ms );
	return delivered;
}

static DWORD launch_game( const char *game_directory,
	                      const char *program_arguments,
	                      const InputScript *script, const char **status,
	                      unsigned int *input_count, int *window_seen )
{
	char program_path[MAX_PROGRAM_PATH];
	wchar_t wide_program_arguments[MAX_PROGRAM_ARGUMENTS];
	wchar_t wide_program_path[MAX_PROGRAM_PATH];
	LPWSTR command_line = NULL;
	PROCESS_INFORMATION process_info;
	STARTUPINFOW startup_info;
	DWORD exit_code = ERROR_GEN_FAILURE;
	DWORD wait_result;
	int written;

	written = snprintf( program_path, sizeof( program_path ),
	                    "\\Storage Card\\oma\\%s\\game.exe",
	                    game_directory );
	if( written < 0 || (size_t)written >= sizeof( program_path ) ) {
		*status = "error";
		return ERROR_BUFFER_OVERFLOW;
	}

	if( !widen_ascii( program_path, wide_program_path,
	                  sizeof( wide_program_path ) /
	                  sizeof( wide_program_path[0] ) ) ) {
		*status = "error";
		return ERROR_BUFFER_OVERFLOW;
	}
	if( program_arguments[0] != '\0' ) {
		if( !widen_ascii( program_arguments, wide_program_arguments,
		                  ARRAY_COUNT(wide_program_arguments) ) ) {
			*status = "error";
			return ERROR_BUFFER_OVERFLOW;
		}
		command_line = wide_program_arguments;
	}

	memset( &startup_info, 0, sizeof( startup_info ) );
	memset( &process_info, 0, sizeof( process_info ) );
	startup_info.cb = sizeof( startup_info );

	prepare_program_memory( wide_program_path, game_directory );
	write_checkpoint( "create-process" );
	if( !CreateProcessW( wide_program_path, command_line, NULL, NULL, FALSE, 0,
	                     NULL, NULL, &startup_info, &process_info ) ) {
		*status = "error";
		return GetLastError();
	}
	write_checkpoint( "process-created" );

	*input_count = run_input_script( script, process_info.hProcess,
	                                 process_info.dwProcessId, window_seen,
	                                 &exit_code );
	write_checkpoint( "inputs-complete" );
	if( *input_count != expected_input_count( script ) ) {
		*status = "exited";
		CloseHandle( process_info.hThread );
		CloseHandle( process_info.hProcess );
		return exit_code;
	}

	wait_result = WaitForSingleObject( process_info.hProcess,
	                                   OMA_STABILITY_MS );
	write_checkpoint( "stability-complete" );
	if( wait_result == WAIT_TIMEOUT ) {
		if( FindWindowW( NULL, application_error_title ) != NULL ) {
			*status = "exited";
			exit_code = ERROR_PROCESS_ABORTED;
		} else if( !GetExitCodeProcess( process_info.hProcess, &exit_code ) ) {
			*status = "error";
			exit_code = GetLastError();
		} else if( exit_code == STILL_ACTIVE ) {
			*status = "alive";
		} else {
			*status = "exited";
		}
	} else if( wait_result == WAIT_OBJECT_0 ) {
		*status = "exited";
		if( !GetExitCodeProcess( process_info.hProcess, &exit_code ) ) {
			*status = "error";
			exit_code = GetLastError();
		}
	} else {
		*status = "error";
		exit_code = GetLastError();
	}

	CloseHandle( process_info.hThread );
	CloseHandle( process_info.hProcess );
	return exit_code;
}

int main( void )
{
	char game_directory[MAX_CONTROL_VALUE] = "invalid";
	char game_key[MAX_CONTROL_VALUE] = "invalid";
	char program_arguments[MAX_PROGRAM_ARGUMENTS] = "";
	const char *status = "error";
	const char *proof = "incomplete";
	const InputScript *script = NULL;
	FILE *completion;
	FILE *control;
	FILE *results;
	DWORD result_code = ERROR_INVALID_DATA;
	int window_seen = 0;
	unsigned int input_count = 0;

	write_checkpoint( "runner-start" );
	control = fopen( control_path, "rb" );
	if( control != NULL ) {
		if( fgets( game_key, sizeof( game_key ), control ) != NULL &&
		    fgets( game_directory, sizeof( game_directory ), control ) != NULL ) {
			trim_line( game_key );
			trim_line( game_directory );
				if( is_safe_component( game_key ) &&
				    is_safe_component( game_directory ) ) {
					if( fgets( program_arguments,
					           sizeof( program_arguments ), control ) != NULL )
						trim_line( program_arguments );
					script = find_input_script( game_key );
					if( script != NULL &&
					    arguments_are_safe( program_arguments ) ) {
						write_checkpoint( "control-ready" );
						result_code = launch_game( game_directory,
						                           program_arguments, script,
					                           &status, &input_count,
					                           &window_seen );
				}
			}
		}
		fclose( control );
	}

	results = fopen( result_path, "wb" );
	if( results == NULL )
		return 2;
	if( script != NULL && strcmp( status, "alive" ) == 0 &&
	    input_count == expected_input_count( script ) && window_seen ) {
		proof = "gameplay";
	}
	fprintf( results, "%s\t%s\t%lu\t%s\t%s\t%u\t%s\r\n",
	         game_key, status, (unsigned long)result_code, proof,
	         script != NULL ? script->name : "no-script", input_count,
	         window_seen ? "window" : "no-window" );
	fclose( results );

	completion = fopen( completion_path, "wb" );
	if( completion == NULL )
		return 3;
	fprintf( completion, "%s\r\n", game_key );
	fclose( completion );
	return 0;
}

/* end of tests/wince/oma-runner.c */

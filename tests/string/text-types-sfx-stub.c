/*
    FreeBASIC text argument tests
    File: text-types-sfx-stub.c
    Purpose: Observe UTF-8 arguments at the sound runtime's actual C ABI.
    Responsibilities: Validate every text command without opening audio devices.
    This file intentionally does NOT implement sound playback or file access.
*/

#include "fb_sfx_internal.h"
#include <assert.h>
#include <string.h>

static int calls;

static void hCheck( const char *text )
{
	assert( text != NULL );
	assert( strcmp(text, "\xC3\xA9\xE4\xB8\xAD\xF0\x9F\x98\x80") == 0 );
	++calls;
}

void fb_sfxPlay( const char *text ) { hCheck(text); }
void fb_sfxPlayChannel( int channel, const char *text ) { hCheck(text); }
void fb_sfxPlay2( const char *first, const char *second ) { hCheck(first); hCheck(second); }
void fb_sfxPlay3( const char *first, const char *second, const char *third ) { hCheck(first); hCheck(second); hCheck(third); }
void fb_sfxNote( const char *text, int octave, float duration ) { hCheck(text); }
void fb_sfxNoteChannel( int channel, const char *text, int octave, float duration ) { hCheck(text); }
int fb_sfxMusicLoad( const char *text ) { hCheck(text); return 0; }
int fb_sfxMusicPlayFile( const char *text ) { hCheck(text); return 0; }
int fb_sfxMusicLoopFile( const char *text ) { hCheck(text); return 0; }
void fb_sfxSfxLoad( int id, const char *text ) { hCheck(text); }
int fb_sfxMidiPlay( const char *text ) { hCheck(text); return 0; }
int fb_sfxCaptureSaveCmd( const char *text ) { hCheck(text); return 0; }
int fb_sfxOutputCaptureSave( const char *text ) { hCheck(text); return 0; }
int fb_text_test_calls( void ) { return calls; }

/* end of text-types-sfx-stub.c */

/*
 * FreeBASIC graphics: fb_x11_drop.h
 * Purpose: Share bounded Xdnd v3-v5 reception between gfxlib2 and gfxlib3.
 * Responsibilities: Copy negotiation, selection validation and timeout cleanup.
 * This file does not own the Display, open files or implement Wayland protocols.
 */
#ifndef FB_X11_DROP_H
#define FB_X11_DROP_H
#include "../../rtlib/fb_drop.h"
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <pthread.h>
#include <stdatomic.h>
#include <sched.h>

#define FB_X11_DROP_MIN_VERSION 3u
#define FB_X11_DROP_MAX_VERSION 5u

/* Xdnd ClientMessage data is five 32-bit protocol words, exposed by Xlib
 * through long slots even on a 64-bit client:
 *   Enter:    source, version in high byte/extended-types bit 0, three types.
 *   Position: source, reserved, packed root coordinates, timestamp, action.
 *   Drop:     source, reserved, timestamp, unused, unused.
 *   Status:   target, accept bit 0/continuous-position bit 1, rectangle, action.
 *   Finished: target, success bit 0, completed action, unused, unused.
 * We request continuous positions and return only the copy action. No DELETE
 * selection is requested, even when a file manager initially suggests move.
 */
typedef int (*FB_X11_DROP_ENABLED)(void *);
typedef int (*FB_X11_DROP_SUBMIT)(void *, int, const char *, size_t, int);
typedef struct FB_X11_DROP {
	Display *display;
	Window window, source;
	Atom aware, enter, position, leave, drop, status, finished;
	Atom selection, property, type_list, uri_list, utf8, plain, copy, move, link;
	Atom selected;
	unsigned int version;
	int waiting, accepted;
	double deadline;
	FB_X11_DROP_ENABLED enabled;
	FB_X11_DROP_SUBMIT submit;
	void *user;
} FB_X11_DROP;

/* Xlib's error handler is process-global. A source may disappear during a
 * drag. Trap only the serial of our own source-window request and forward all
 * unrelated errors to the previous handler. Synchronize before restoring it.
 * Native event handling and teardown run on the owning Display thread.
 */
static pthread_mutex_t fb_drop_xerror_mutex = PTHREAD_MUTEX_INITIALIZER;
typedef int (*FB_DROP_XERROR_HANDLER)(Display *, XErrorEvent *);
static _Atomic(Display *) fb_drop_xerror_display;
static _Atomic unsigned long fb_drop_xerror_serial;
static _Atomic int fb_drop_xerror_seen;
static _Atomic int fb_drop_xerror_ready = TRUE;
static _Atomic(FB_DROP_XERROR_HANDLER) fb_drop_xerror_previous;

static int fb_drop_xerror_handler(Display *display, XErrorEvent *error)
{
	FB_DROP_XERROR_HANDLER previous;
	/* Another Display thread can enter this global handler immediately after
	 * installation. Wait until its forwarding target has been published. No
	 * Xlib call occurs between handler installation and that publication.
	 */
	while (!atomic_load_explicit(&fb_drop_xerror_ready, memory_order_acquire))
		sched_yield();
	if (display == fb_drop_xerror_display && error->serial == fb_drop_xerror_serial) {
		fb_drop_xerror_seen = TRUE;
		return 0;
	}
	previous = atomic_load_explicit(&fb_drop_xerror_previous, memory_order_acquire);
	return previous != NULL ? previous(display, error) : 0;
}

static void fb_drop_xerror_begin(Display *display)
{
	pthread_mutex_lock(&fb_drop_xerror_mutex);
	fb_drop_xerror_display = display;
	fb_drop_xerror_serial = NextRequest(display);
	fb_drop_xerror_seen = FALSE;
	atomic_store_explicit(&fb_drop_xerror_ready, FALSE, memory_order_release);
	fb_drop_xerror_previous = XSetErrorHandler(fb_drop_xerror_handler);
	atomic_store_explicit(&fb_drop_xerror_ready, TRUE, memory_order_release);
}

static int fb_drop_xerror_end(Display *display)
{
	int result;
	XSync(display, False);
	result = !fb_drop_xerror_seen;
	XSetErrorHandler(atomic_load_explicit(&fb_drop_xerror_previous, memory_order_acquire));
	fb_drop_xerror_display = NULL;
	pthread_mutex_unlock(&fb_drop_xerror_mutex);
	return result;
}

static void fb_hX11DropMessage(FB_X11_DROP *state, Atom message,
	long first, long second, long action)
{
	XEvent event;
	if (state->source == None) return;
	memset(&event, 0, sizeof(event));
	event.xclient.type = ClientMessage;
	event.xclient.window = state->source;
	event.xclient.message_type = message;
	event.xclient.format = 32;
	event.xclient.data.l[0] = (long)state->window;
	event.xclient.data.l[1] = first;
	event.xclient.data.l[2] = second;
	event.xclient.data.l[4] = action;
	fb_drop_xerror_begin(state->display);
	XSendEvent(state->display, state->source, False, NoEventMask, &event);
	(void)fb_drop_xerror_end(state->display);
}

static void fb_hX11DropFinish(FB_X11_DROP *state, int success)
{
	/* XdndFinished v5 puts the action in data[2]; XdndStatus uses data[4].
	 * Earlier Finished versions reserve both success/action words as zero.
	 */
	fb_hX11DropMessage(state, state->finished,
		state->version >= 5u && success ? 1 : 0,
		state->version >= 5u && success ? (long)state->copy : 0, 0);
	XDeleteProperty(state->display, state->window, state->property);
	state->source = None;
	state->selected = None;
	state->waiting = FALSE;
	state->accepted = FALSE;
	state->version = 0;
}

static void fb_hX11DropInit(FB_X11_DROP *state, Display *display, Window window,
	FB_X11_DROP_ENABLED enabled, FB_X11_DROP_SUBMIT submit, void *user)
{
	unsigned long version = FB_X11_DROP_MAX_VERSION;
	memset(state, 0, sizeof(*state));
	state->display = display;
	state->window = window;
	state->enabled = enabled;
	state->submit = submit;
	state->user = user;
#define DROP_ATOM(field, text) state->field = XInternAtom(display, text, False)
	DROP_ATOM(aware, "XdndAware");
	DROP_ATOM(enter, "XdndEnter");
	DROP_ATOM(position, "XdndPosition");
	DROP_ATOM(leave, "XdndLeave");
	DROP_ATOM(drop, "XdndDrop");
	DROP_ATOM(status, "XdndStatus");
	DROP_ATOM(finished, "XdndFinished");
	DROP_ATOM(selection, "XdndSelection");
	DROP_ATOM(property, "_FREEBASIC_DROP");
	DROP_ATOM(type_list, "XdndTypeList");
	DROP_ATOM(uri_list, "text/uri-list");
	DROP_ATOM(utf8, "UTF8_STRING");
	DROP_ATOM(plain, "text/plain;charset=utf-8");
	DROP_ATOM(copy, "XdndActionCopy");
	DROP_ATOM(move, "XdndActionMove");
	DROP_ATOM(link, "XdndActionLink");
#undef DROP_ATOM
	XChangeProperty(display, window, state->aware, XA_ATOM, 32,
		PropModeReplace, (unsigned char *)&version, 1);
}

static void fb_hX11DropTick(FB_X11_DROP *state)
{
	double now;
	if (state->waiting && (!state->enabled(state->user) ||
	    !fb_hIdleNow(&now) || now >= state->deadline))
		fb_hX11DropFinish(state, FALSE);
}

static void fb_hX11DropChoose(FB_X11_DROP *state, const Atom *types, unsigned long count)
{
	unsigned long index;
	for (index = 0; index < count; ++index) {
		if (types[index] == state->uri_list) {
			state->selected = state->uri_list;
			return;
		}
		if (types[index] == state->utf8 || types[index] == state->plain)
			state->selected = types[index];
	}
}

static int fb_hX11DropEvent(FB_X11_DROP *state, const XEvent *event)
{
	if (event->xany.window != state->window) return FALSE;
	if (event->type == ClientMessage) {
		const XClientMessageEvent *message = &event->xclient;
		Atom type = message->message_type;
		if (type != state->enter && type != state->position &&
		    type != state->leave && type != state->drop) return FALSE;
		if (message->format != 32) return TRUE;
		if (type == state->enter) {
			unsigned long version = ((unsigned long)message->data.l[1] >> 24) & 255u;
			if (state->waiting) fb_hX11DropFinish(state, FALSE);
			state->source = (Window)message->data.l[0];
			state->selected = None;
			state->accepted = FALSE;
			state->version = 0;
			if (version < FB_X11_DROP_MIN_VERSION ||
			    version > FB_X11_DROP_MAX_VERSION || state->source == None)
				return TRUE;
			state->version = (unsigned int)version;
			if ((message->data.l[1] & 1) != 0) {
				Atom actual;
				int format, result, safe;
				unsigned long count, remaining;
				unsigned char *data = NULL;
				fb_drop_xerror_begin(state->display);
				result = XGetWindowProperty(state->display, state->source,
					state->type_list, 0, 64, False, XA_ATOM,
					&actual, &format, &count, &remaining, &data);
				safe = fb_drop_xerror_end(state->display);
				if (safe && result == Success && actual == XA_ATOM &&
				    format == 32 && remaining == 0 && data != NULL)
					fb_hX11DropChoose(state, (Atom *)data, count);
				if (data != NULL) XFree(data);
			} else {
				Atom types[3];
				types[0] = (Atom)message->data.l[2];
				types[1] = (Atom)message->data.l[3];
				types[2] = (Atom)message->data.l[4];
				fb_hX11DropChoose(state, types, 3);
			}
			return TRUE;
		}
		if ((Window)message->data.l[0] != state->source) return TRUE;
		if (type == state->position) {
			Atom requested = (Atom)message->data.l[4];
			/* Xdnd permits the target to fall back to copy. This is necessary
			 * for ordinary file-manager drags which suggest move or link. Our
			 * Status and Finished replies both advertise copy, so the source
			 * never receives permission to delete or move its original data.
			 * Ask/private/custom actions require application policy and are refused.
			 */
			int accept = state->selected != None && !state->waiting &&
				state->enabled(state->user) &&
				(requested == state->copy || requested == state->move ||
				 requested == state->link);
			state->accepted = accept;
			/* Ask for future positions instead of interpreting root coordinates. */
			fb_hX11DropMessage(state, state->status, accept ? 3 : 2,
				0, accept ? (long)state->copy : 0);
		} else if (type == state->leave) {
			if (state->waiting) fb_hX11DropFinish(state, FALSE);
			state->source = None;
			state->selected = None;
		} else if (type == state->drop) {
			double now;
			if (state->waiting || !state->accepted || state->selected == None ||
			    !state->enabled(state->user) || !fb_hIdleNow(&now)) {
				fb_hX11DropFinish(state, FALSE);
				return TRUE;
			}
			state->waiting = TRUE;
			state->deadline = now + 5.0; /* abandon an unresponsive source */
			XDeleteProperty(state->display, state->window, state->property);
			XConvertSelection(state->display, state->selection, state->selected,
				state->property, state->window, (Time)message->data.l[2]);
		}
		return TRUE;
	}
	if (event->type == SelectionNotify && state->waiting &&
	    event->xselection.selection == state->selection) {
		Atom actual = None;
		int format = 0, result = BadValue, success = FALSE;
		unsigned long count = 0, remaining = 0;
		unsigned char *data = NULL;
		char *payload = NULL;
		if (event->xselection.target == state->selected &&
		    event->xselection.property == state->property)
			result = XGetWindowProperty(state->display, state->window,
				state->property, 0, (long)(FB_DROP_MAX_BYTES / 4u), True,
				AnyPropertyType, &actual, &format, &count, &remaining, &data);
		/* INCR and oversized properties are refused. Waiting for an unbounded
		 * stream would let another application exhaust memory or hold a mode.
		 */
		if (result == Success && actual == state->selected && format == 8 &&
		    remaining == 0 && count < FB_DROP_MAX_BYTES && data != NULL &&
		    state->enabled(state->user)) {
			payload = malloc(FB_DROP_MAX_BYTES);
			if (payload != NULL) {
				size_t bytes = 0;
				int items = 1, kind = FB_DROP_TEXT;
				if (state->selected == state->uri_list) {
					kind = FB_DROP_FILES;
					if (fb_hDropUriList((const char *)data, (size_t)count,
						payload, FB_DROP_MAX_BYTES, &bytes, &items) != FB_DROP_OK)
						items = 0;
				} else {
					memcpy(payload, data, (size_t)count);
					payload[count] = 0;
					bytes = (size_t)count + 1u;
				}
				if (items != 0)
					success = state->submit(state->user, kind, payload,
						bytes, items) == FB_DROP_OK;
			}
		}
		free(payload);
		if (data != NULL) XFree(data);
		fb_hX11DropFinish(state, success);
		return TRUE;
	}
	return FALSE;
}
#endif
/* end of fb_x11_drop.h */

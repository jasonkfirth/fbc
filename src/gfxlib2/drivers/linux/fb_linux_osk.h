/*
 * FreeBASIC Linux graphics: fb_linux_osk.h
 * Purpose: Request and observe a session on-screen keyboard when one exports
 *     a supported desktop-session interface.
 * Responsibilities: Keep session IPC details separate from graphics windows.
 * This file intentionally does NOT contain: form-factor detection, text-input
 *     widgets, or Wayland surface management.
 */
#ifndef FB_LINUX_OSK_H
#define FB_LINUX_OSK_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <sys/wait.h>

#include "fb_linux_maliit.h"

/*
    Phosh and KWin publish session-bus visibility contracts. Hildon accepts a
    session signal but does not publish visibility, so its state stays unknown.
    Availability is checked against the running session service.
*/
static pthread_mutex_t fb_hLinuxOskMutex = PTHREAD_MUTEX_INITIALIZER;
static int fb_hLinuxOskAvailable = -1;
static int fb_hLinuxOskVisible = -2;
static time_t fb_hLinuxOskCachedAt;
static unsigned int fb_hLinuxOskGeneration;
static unsigned int fb_hLinuxOskQueryGeneration;
static int fb_hLinuxOskQueryRunning;

static int fb_hLinuxOskIsHildon(void)
{
	const char *input_method = getenv("QT_IM_MODULE");
	return input_method && (strcmp(input_method, "him") == 0);
}

static int fb_hLinuxOskQueryHildon(void)
{
	FILE *pipe;
	char result[128];
	int status;

	pipe = popen("gdbus call --timeout=1 --session --dest org.freedesktop.DBus "
		"--object-path /org/freedesktop/DBus "
		"--method org.freedesktop.DBus.GetNameOwner "
		"com.nokia.hildon_input_method 2>/dev/null", "r");
	if (!pipe)
		return 0;
	result[0] = '\0';
	if (!fgets(result, sizeof(result), pipe))
		result[0] = '\0';
	status = pclose(pipe);
	if ((status == -1) || !WIFEXITED(status) ||
	    (WEXITSTATUS(status) != 0) || !strchr(result, ':'))
		return 0;
	return 1;
}

/*
    A session-bus query can take longer than a frame on a phone. Only one
    detached worker may query desktop services at a time; readers continue
    using the previous result or the Maliit fallback while it runs.
*/
static void *fb_hLinuxOskQueryWorker(void *unused)
{
	FILE *pipe;
	char result[128];
	int status, available = 0, visible = -2;
	(void)unused;

	/* The gdbus command-line timeout is in seconds, unlike GDBus API timeouts. */
	pipe = popen("gdbus call --timeout=1 --session --dest sm.puri.OSK0 "
		"--object-path /sm/puri/OSK0 "
		"--method org.freedesktop.DBus.Properties.Get "
		"sm.puri.OSK0 Visible 2>/dev/null", "r");
	if (pipe) {
		result[0] = '\0';
		if (!fgets(result, sizeof(result), pipe))
			result[0] = '\0';
		status = pclose(pipe);
		if ((status != -1) && WIFEXITED(status) &&
		    (WEXITSTATUS(status) == 0)) {
			if (strstr(result, "<true>")) {
				available = 1;
				visible = -1;
			} else if (strstr(result, "<false>")) {
				available = 1;
				visible = 0;
			}
		}
	}
	if (!available && fb_hLinuxOskIsHildon() &&
	    fb_hLinuxOskQueryHildon()) {
		available = 1;
		visible = -2;
	}
	if (!available && fb_hLinuxKwinQueryKeyboard(&visible))
		available = 1;

	pthread_mutex_lock(&fb_hLinuxOskMutex);
	/* A show/hide request invalidates results started before that request. */
	if (fb_hLinuxOskQueryGeneration == fb_hLinuxOskGeneration) {
		fb_hLinuxOskAvailable = available;
		fb_hLinuxOskVisible = visible;
		fb_hLinuxOskCachedAt = time(NULL);
	}
	fb_hLinuxOskQueryRunning = 0;
	pthread_mutex_unlock(&fb_hLinuxOskMutex);
	return NULL;
}

static int fb_hLinuxOskQuery(int *visible)
{
	time_t now = time(NULL);
	int available, cached_visible;

	if (!visible)
		return 0;
	pthread_mutex_lock(&fb_hLinuxOskMutex);
	if (!((fb_hLinuxOskAvailable >= 0) && (now >= fb_hLinuxOskCachedAt) &&
	      (now - fb_hLinuxOskCachedAt < 1)) && !fb_hLinuxOskQueryRunning) {
		if (!getenv("DBUS_SESSION_BUS_ADDRESS")) {
			fb_hLinuxOskAvailable = 0;
			fb_hLinuxOskVisible = -2;
			fb_hLinuxOskCachedAt = now;
		} else {
			pthread_t thread;
			pthread_attr_t attributes;
			int started = 0;
			fb_hLinuxOskQueryRunning = 1;
			fb_hLinuxOskQueryGeneration = fb_hLinuxOskGeneration;
			if (pthread_attr_init(&attributes) == 0) {
				if ((pthread_attr_setdetachstate(&attributes,
					PTHREAD_CREATE_DETACHED) == 0) &&
				    (pthread_create(&thread, &attributes,
					fb_hLinuxOskQueryWorker, NULL) == 0))
					started = 1;
				(void)pthread_attr_destroy(&attributes);
			}
			if (!started) {
				fb_hLinuxOskQueryRunning = 0;
				/* Avoid retrying thread creation on every graphics frame. */
				if (fb_hLinuxOskAvailable < 0) {
					fb_hLinuxOskAvailable = 0;
					fb_hLinuxOskVisible = -2;
				}
				fb_hLinuxOskCachedAt = now;
			}
		}
	}
	available = fb_hLinuxOskAvailable;
	cached_visible = fb_hLinuxOskVisible;
	pthread_mutex_unlock(&fb_hLinuxOskMutex);
	if (available == 1) {
		*visible = cached_visible;
		return -1;
	}
	/* Neither desktop service replied; Maliit can still be active. */
	*visible = -2;
	return fb_hLinuxMaliitQueryKeyboard(visible);
}

/*
    A Maliit worker waits for its fallback so it can order a compensating hide
    after an in-flight show. A direct Phosh request retains the existing
    background behavior because it runs on the application thread.
*/
static void fb_hLinuxOskRequestPhosh(int visible, int background)
{
	const char *value = visible ? "true" : "false";
	const char *suffix = background ? " &" : "";
	char command[192];
	int command_length;
	int command_status;

	if (!getenv("DBUS_SESSION_BUS_ADDRESS"))
		return;
	command_length = snprintf(command, sizeof(command),
		"gdbus call --timeout=1 --session --dest sm.puri.OSK0 "
		"--object-path /sm/puri/OSK0 --method sm.puri.OSK0.SetVisible %s "
		">/dev/null 2>&1%s", value, suffix);
	if (command_length < 0 || (size_t)command_length >= sizeof(command))
		return;
	command_status = system(command);
	if (command_status == -1)
		return;
}

static int fb_hLinuxOskRequestHildon(int visible, int background)
{
	const char *value = visible ? "true" : "false";
	const char *suffix = background ? " &" : "";
	char command[192];
	int command_length;
	int command_status;

	if (!fb_hLinuxOskIsHildon() || !getenv("DBUS_SESSION_BUS_ADDRESS"))
		return 0;
	command_length = snprintf(command, sizeof(command),
		"gdbus emit --session --object-path /org/maemo/him "
		"--signal org.maemo.him.set_visible %s "
		">/dev/null 2>&1%s", value, suffix);
	if (command_length < 0 || (size_t)command_length >= sizeof(command))
		return 0;
	command_status = system(command);
	return command_status != -1;
}

static void fb_hLinuxOskRequestFallback(int visible)
{
	if (!fb_hLinuxOskRequestHildon(visible, 0))
		fb_hLinuxOskRequestPhosh(visible, 0);
}

static void fb_hLinuxOskRequest(int visible)
{
	/* Any request invalidates a query started before it. */
	pthread_mutex_lock(&fb_hLinuxOskMutex);
	fb_hLinuxOskGeneration++;
	fb_hLinuxOskAvailable = -1;
	fb_hLinuxOskCachedAt = 0;
	pthread_mutex_unlock(&fb_hLinuxOskMutex);
	if (!fb_hLinuxMaliitQueueKeyboard(visible, fb_hLinuxOskRequestFallback)) {
		if (!fb_hLinuxOskRequestHildon(visible, 1))
			fb_hLinuxOskRequestPhosh(visible, 1);
	}
}

#endif
/* end of fb_linux_osk.h */

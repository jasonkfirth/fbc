/*
 * FreeBASIC graphics tests: x11-drop-source.c
 * Purpose: Drive the native Xdnd protocol against a running graphics window.
 * Responsibilities: Selection ownership, Unicode data and hostile transfers.
 * This fixture does not call FreeBASIC's private event or payload queues.
 */
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <time.h>
#include <unistd.h>

static double seconds(void)
{
	struct timespec value;
	assert(clock_gettime(CLOCK_MONOTONIC, &value) == 0);
	return (double)value.tv_sec + (double)value.tv_nsec / 1000000000.0;
}

static void message(Display *display, Window target, Atom type, Window source,
	long flags, long third, long fourth, long fifth)
{
	XEvent event;
	memset(&event, 0, sizeof(event));
	event.xclient.type = ClientMessage;
	event.xclient.window = target;
	event.xclient.message_type = type;
	event.xclient.format = 32;
	event.xclient.data.l[0] = (long)source;
	event.xclient.data.l[1] = flags;
	event.xclient.data.l[2] = third;
	event.xclient.data.l[3] = fourth;
	event.xclient.data.l[4] = fifth;
	assert(XSendEvent(display, target, False, NoEventMask, &event));
	XFlush(display);
}

int main(int argc, char **argv)
{
	Display *display = XOpenDisplay(NULL);
	Window target, source;
	Atom selection, offered, status, finished, copy, drop;
	int protocol_version;
	const char *scenario;
	double deadline;
	assert(argc == 3 && display != NULL);
	target = (Window)strtoul(argv[1], NULL, 10);
	scenario = argv[2];
	protocol_version = 5;
	if (strcmp(scenario, "version3") == 0) protocol_version = 3;
	else if (strcmp(scenario, "version4") == 0) protocol_version = 4;
	else if (strcmp(scenario, "version") == 0) protocol_version = 6;
	source = XCreateSimpleWindow(display, DefaultRootWindow(display), 0, 0, 1, 1, 0, 0, 0);
	selection = XInternAtom(display, "XdndSelection", False);
	offered = XInternAtom(display, strcmp(scenario, "text") == 0 ? "UTF8_STRING" : "text/uri-list", False);
	status = XInternAtom(display, "XdndStatus", False);
	finished = XInternAtom(display, "XdndFinished", False);
	copy = XInternAtom(display, "XdndActionCopy", False);
	drop = XInternAtom(display, "XdndDrop", False);
	XSetSelectionOwner(display, selection, source, CurrentTime);
	if (strcmp(scenario, "extended") == 0) {
		unsigned long types[4] = {0};
		types[0] = XInternAtom(display, "application/unsupported", False);
		types[1] = offered;
		types[2] = XInternAtom(display, "UTF8_STRING", False);
		types[3] = XInternAtom(display, "text/plain;charset=utf-8", False);
		XChangeProperty(display, source, XInternAtom(display, "XdndTypeList", False),
			XA_ATOM, 32, PropModeReplace, (unsigned char *)types, 4);
	}
	message(display, target, XInternAtom(display, "XdndEnter", False), source,
		((long)protocol_version << 24) |
		(strcmp(scenario, "extended") == 0 ? 1 : 0), (long)offered, 0, 0);
	Atom requested = copy;
	if (strcmp(scenario, "move-copy") == 0)
		requested = XInternAtom(display, "XdndActionMove", False);
	if (strcmp(scenario, "unsupported-action") == 0)
		requested = XInternAtom(display, "FreeBASICUnsupportedAction", False);
	message(display, target, XInternAtom(display, "XdndPosition", False), source,
		0, 0, CurrentTime, (long)requested);
	deadline = seconds() + 8.0;
	while (seconds() < deadline) {
		XEvent event;
		if (XPending(display) == 0) {
			fd_set input;
			struct timeval timeout = {0, 100000};
			FD_ZERO(&input);
			FD_SET(ConnectionNumber(display), &input);
			(void)select(ConnectionNumber(display) + 1, &input, NULL, NULL, &timeout);
			continue;
		}
		XNextEvent(display, &event);
		if (event.type == ClientMessage && event.xclient.message_type == status) {
			if ((event.xclient.data.l[1] & 1) == 0) {
				puts("XDND_REFUSED");
				XDestroyWindow(display, source);
				XCloseDisplay(display);
				return strcmp(scenario, "unsupported-action") == 0 ||
					strcmp(scenario, "version") == 0 ? 0 : 1;
			}
			assert((Atom)event.xclient.data.l[4] == copy);
			message(display, target, drop, source, 0, CurrentTime, 0, 0);
			if (strcmp(scenario, "source-destroy") == 0) {
				XDestroyWindow(display, source);
				XSync(display, False);
				XCloseDisplay(display);
				puts("XDND_SOURCE_DESTROYED");
				return 0;
			}
		} else if (event.type == SelectionRequest) {
			XSelectionRequestEvent *request = &event.xselectionrequest;
			XEvent reply;
			Atom property = request->property != None ? request->property : request->target;
			const char *data = "file:///tmp/a%20b.txt\r\nfile:///tmp/snow-%E2%98%83-%F0%9F%8C%8D.txt\r\n";
			if (strcmp(scenario, "timeout") == 0) continue;
			if (strcmp(scenario, "remote") == 0) data = "file://remote/tmp/a.txt\n";
			if (strcmp(scenario, "invalid") == 0) data = "file:///tmp/%00.txt\n";
			if (strcmp(scenario, "text") == 0) data = "Post \xe2\x98\x83 \xf0\x9f\x8c\x8d";
			if (strcmp(scenario, "incr") == 0) {
				unsigned long bytes = 70000;
				XChangeProperty(display, request->requestor, property,
					XInternAtom(display, "INCR", False), 32, PropModeReplace,
					(unsigned char *)&bytes, 1);
			} else if (strcmp(scenario, "oversize") == 0) {
				char *large = malloc(70000);
				assert(large != NULL);
				memset(large, 'x', 70000);
				XChangeProperty(display, request->requestor, property, request->target,
					8, PropModeReplace, (unsigned char *)large, 70000);
				free(large);
			} else {
				XChangeProperty(display, request->requestor, property, request->target,
					8, PropModeReplace, (const unsigned char *)data, (int)strlen(data));
			}
			memset(&reply, 0, sizeof(reply));
			reply.xselection.type = SelectionNotify;
			reply.xselection.requestor = request->requestor;
			reply.xselection.selection = request->selection;
			reply.xselection.target = request->target;
			reply.xselection.property = property;
			reply.xselection.time = request->time;
			assert(XSendEvent(display, request->requestor, False, NoEventMask, &reply));
			XFlush(display);
		} else if (event.type == ClientMessage && event.xclient.message_type == finished) {
			int accepted = protocol_version < 5 ? 1 : (event.xclient.data.l[1] & 1) != 0;
			int expected = strcmp(scenario, "files") == 0 || strcmp(scenario, "text") == 0 ||
				strcmp(scenario, "extended") == 0 || strcmp(scenario, "move-copy") == 0 ||
				strcmp(scenario, "version3") == 0 || strcmp(scenario, "version4") == 0;
			if (protocol_version < 5)
				assert(event.xclient.data.l[1] == 0 && event.xclient.data.l[2] == 0);
			else
				assert(!accepted || (Atom)event.xclient.data.l[2] == copy);
			printf("XDND_FINISHED=%d\n", accepted);
			XDestroyWindow(display, source);
			XCloseDisplay(display);
			return accepted == expected ? 0 : 1;
		}
	}
	fputs("XDND_TIMEOUT\n", stderr);
	XCloseDisplay(display);
	return 1;
}
/* end of x11-drop-source.c */

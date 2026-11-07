/*
 * Project: FreeBASIC Unix notification tests
 * File: notify-luna-stdin.c
 * Purpose: Exercise the production Luna subprocess path without a desktop bus.
 * Responsibilities: Asynchronous replies, stdin isolation, low descriptors,
 * bounded output and descriptor cleanup.
 * This file does not invoke a shell or install a replacement luna-send.
 * Linux build from the repository root:
 * gcc -std=gnu11 -O2 -ffunction-sections -fdata-sections -Wl,--gc-sections
 *     tests/services/notify-luna-stdin.c -pthread -ldl -o notify-luna-stdin
 */
#define FB_NOTIFY_LUNA_SEND_PATH "/proc/self/exe"
#include "../../src/rtlib/drivers/unix/sys_hnotify.c"
#include <poll.h>

static int descriptor_count(void)
{
	int count = 0;
	for (int descriptor = 0; descriptor < 128; ++descriptor)
		if (fcntl(descriptor, F_GETFD) >= 0)
			++count;
	return count;
}

/* Model luna-send's stdin watch before a delayed service reply. A readable
 * descriptor would expose caller input; EOF/HUP aborts the real client. */
static int service_client(int argc, char **argv)
{
	struct pollfd input = { STDIN_FILENO, POLLIN, 0 };
	if (argc != 9 || strcmp(argv[5], "-a") != 0 ||
	    strcmp(argv[6], "templari-native-qa") != 0 ||
	    strcmp(argv[7], FB_NOTIFY_LUNA_CREATE_URI) != 0)
		return 10;
	if (poll(&input, 1, 40) != 0)
		return 11;
	if (strcmp(argv[8], "oversize") == 0) {
		for (unsigned index = 0; index < FB_NOTIFY_LUNA_RESPONSE_MAX + 1u; ++index)
			putchar('x');
	} else {
		if (strcmp(argv[8], "{\"message\":\"literal ; $()\"}") != 0)
			return 12;
		puts("{\"returnValue\":true,\"toastId\":\"qa-toast\"}");
	}
	return 0;
}

static int exercise(int mode)
{
	FB_NOTIFY_UNIX context;
	char response[FB_NOTIFY_LUNA_RESPONSE_MAX];
	int input_pipe[2] = {-1, -1};
	memset(&context, 0, sizeof(context));
	strcpy(context.app_id, "templari-native-qa");
	if (mode == 0) {
		int descriptor = open("/dev/null", O_RDONLY);
		if (descriptor < 0 || dup2(descriptor, STDIN_FILENO) < 0) return 20;
		if (descriptor != STDIN_FILENO) close(descriptor);
	} else if (mode == 1) {
		close(STDIN_FILENO);
	} else if (mode == 2) {
		if (pipe(input_pipe) != 0) return 21;
		for (int index = 0; index < 2; ++index)
			if (fcntl(input_pipe[index], F_SETFD, FD_CLOEXEC) < 0) return 22;
		if (write(input_pipe[1], "caller", 6) != 6 || dup2(input_pipe[0], STDIN_FILENO) < 0) return 23;
		if (input_pipe[0] != STDIN_FILENO) close(input_pipe[0]);
	} else {
		close(STDIN_FILENO); close(STDOUT_FILENO); close(STDERR_FILENO);
	}
	int before = descriptor_count();
	for (int attempt = 0; attempt < 4; ++attempt) {
		int result = run_luna_send(&context, FB_NOTIFY_LUNA_CREATE_URI,
			"{\"message\":\"literal ; $()\"}", response, sizeof(response));
		if (result != FB_SERVICE_OK || !json_field_is_true(response, "returnValue")) return 24;
		if (descriptor_count() != before) return 25;
	}
	if (run_luna_send(&context, FB_NOTIFY_LUNA_CREATE_URI,
	    "oversize", response, sizeof(response)) == FB_SERVICE_OK) return 26;
	if (descriptor_count() != before) return 27;
	if (mode == 2) {
		char original[6];
		if (read(STDIN_FILENO, original, sizeof(original)) != 6 || memcmp(original, "caller", 6) != 0) return 28;
		close(input_pipe[1]);
	}
	return 0;
}

int main(int argc, char **argv)
{
	if (argc > 1 && strcmp(argv[1], "-n") == 0)
		return service_client(argc, argv);
	for (int mode = 0; mode < 4; ++mode) {
		pid_t child = fork();
		int status = 0;
		if (child < 0) return 1;
		if (child == 0) _exit(exercise(mode));
		if (waitpid(child, &status, 0) != child || !WIFEXITED(status) || WEXITSTATUS(status) != 0) {
			fprintf(stderr, "notify-luna-stdin: mode=%d status=%d\n", mode, status);
			return 1;
		}
	}
	puts("NOTIFY_LUNA_STDIN_PASS modes=4 sends=20");
	return 0;
}
/* end of notify-luna-stdin.c */

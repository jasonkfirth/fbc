/*
 * Project: FreeBASIC Unix runtime tests
 * File: notify-argument-types.c
 * Purpose: Check borrowed notification arguments at the native spawn boundary.
 * Responsibilities: Pointer identity, argument order and ordinary failure cleanup.
 * Process creation is replaced by an argument observer.
 * Build from the repository root with -Isrc/rtlib -ffunction-sections
 * -fdata-sections -Wl,--gc-sections -Wl,--wrap=posix_spawn -pthread -ldl.
 */
#include "drivers/unix/sys_hnotify.c"

static unsigned long checks;
static unsigned calls;
static const char *expected_method;
static const char *expected_payload;
static char *expected_app_id;

int __wrap_posix_spawn(pid_t *child, const char *path,
	const posix_spawn_file_actions_t *actions, const posix_spawnattr_t *attributes,
	char *const arguments[], char *const environment[]);

static void check(int condition)
{
	checks++;
	if (!condition) {
		fprintf(stderr, "NATIVE_NOTIFY_ARGUMENTS_FAIL check=%lu\n", checks);
		exit(1);
	}
}

int __wrap_posix_spawn(pid_t *child, const char *path,
	const posix_spawn_file_actions_t *actions, const posix_spawnattr_t *attributes,
	char *const arguments[], char *const environment[])
{
	calls++;
	check(child != NULL);
	check(strcmp(path, FB_NOTIFY_LUNA_SEND_PATH) == 0);
	check(actions != NULL);
	check(attributes == NULL);
	check(environment == environ);
	check(strcmp(arguments[0], FB_NOTIFY_LUNA_SEND_PATH) == 0);
	check(strcmp(arguments[1], "-n") == 0);
	check(strcmp(arguments[2], "1") == 0);
	check(strcmp(arguments[3], "-w") == 0);
	check(strcmp(arguments[4], FB_NOTIFY_LUNA_WAIT_MSEC) == 0);
	check(strcmp(arguments[5], "-a") == 0);
	check(arguments[6] == expected_app_id);
	check(arguments[7] == expected_method);
	check(arguments[8] == expected_payload);
	check(arguments[9] == NULL);
	return EAGAIN;
}

static int descriptor_count(void)
{
	int count = 0;
	for (int descriptor = 0; descriptor < 128; ++descriptor) {
		if (fcntl(descriptor, F_GETFD) >= 0)
			++count;
	}
	return count;
}

int main(void)
{
	static const char *const methods[] = {
		FB_NOTIFY_LUNA_CREATE_URI, FB_NOTIFY_LUNA_CLOSE_URI
	};
	static const char *const payloads[] = {"alpha", "", "two words", "\xC3\xA9"};
	FB_NOTIFY_UNIX context;
	char response[32];
	memset(&context, 0, sizeof(context));
	strcpy(context.app_id, "native-notification-values");
	expected_app_id = context.app_id;
	for (size_t method = 0; method < sizeof(methods) / sizeof(methods[0]); ++method) {
		for (size_t payload = 0; payload < sizeof(payloads) / sizeof(payloads[0]); ++payload) {
			int before = descriptor_count();
			expected_method = methods[method];
			expected_payload = payloads[payload];
			strcpy(response, "unchanged");
			check(run_luna_send(&context, expected_method, expected_payload,
				response, sizeof(response)) == FB_SERVICE_UNAVAILABLE);
			check(strcmp(response, "unchanged") == 0);
			check(descriptor_count() == before);
		}
	}
	check(calls == 8);
	printf("NATIVE_NOTIFY_ARGUMENTS_PASS checks=%lu calls=%u\n", checks, calls);
	return 0;
}

/* end of notify-argument-types.c */

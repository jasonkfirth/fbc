/*
 * Project FreeBASIC Coreutils qualification
 * File coreutils-volatile-state.c
 * Purpose Check the complete savewd_errno function and its volatile state load.
 * Responsibilities Verify signed state/error values, fresh reads and byte preservation.
 * Working-directory and process operations have separate application tests.
 */
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

int audit_savewd_state(int, int, unsigned int *);

static unsigned long cases, checks;

static void require_at(int condition, int line)
{
    checks++;
    if (!condition) {
        fprintf(stderr, "VOLATILE_STATE_FAIL case=%lu line=%d\n", cases, line);
        exit(1);
    }
}

#define require(condition) require_at((condition), __LINE__)

int main(void)
{
    static const int states[] = {INT_MIN, -1, 0, 1, 2, 3, 4, 5, 6, INT_MAX};
    static const int errors[] = {INT_MIN, -1, 0, 1, 2, 13, INT_MAX};
    for (unsigned int state = 0; state < sizeof(states) / sizeof(states[0]); state++) {
        for (unsigned int error = 0; error < sizeof(errors) / sizeof(errors[0]); error++) {
            unsigned int unchanged = 0;
            int expected = states[state] < 4 ? 0 : errors[error];
            require(audit_savewd_state(states[state], errors[error], &unchanged) == expected);
            require(unchanged == 1);
            require(audit_savewd_state(4, errors[error], &unchanged) == errors[error]);
            require(unchanged == 1);
            require(audit_savewd_state(0, errors[error], &unchanged) == 0);
            require(unchanged == 1);
            cases++;
        }
    }
    printf("VOLATILE_STATE_PASS cases=%lu checks=%lu\n", cases, checks);
    return 0;
}
/* end of coreutils-volatile-state.c */

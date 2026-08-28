FreeBASIC OPEN COM pseudo-terminal integration tests
----------------------------------------------------

This test creates a kernel pseudo-terminal and opens its slave through the
FreeBASIC OPEN COM runtime. It verifies that a hostile prior configuration is
replaced by the requested serial settings, that binary bytes travel unchanged
in both directions, and that CLOSE restores the prior terminal state.

Run it against the compiler and runtime from this source tree:

    make FBC=../../bin/fbc tests

No physical serial hardware or elevated privileges are required.

end of readme.txt

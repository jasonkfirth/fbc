# Wii runtime waits and partial TCP reads

File: `docs/wii-runtime-io.md`

The Wii TCP receive pump owns the connected socket's blocking receive operation.
It publishes received bytes through a handle-local ring. `EOF` tests whether
bytes are available; `EOC` becomes true only after closure and buffered-data
drain. A binary `GET` may return fewer bytes than requested. Callers use its
byte-count result and retain their own protocol framing.

Current libogc exposes both POSIX/newlib descriptors and raw IOS socket numbers.
`socket`, `accept`, `send` and `recv` use the POSIX namespace. Their descriptors
must also go through `poll`, `select`, `ioctl` and `close`, rather than the raw
`net_*` equivalents. Mixing namespaces can select a different socket and leak
the original descriptor. The worker uses a bounded POSIX readiness poll before
receiving and is joined before socket shutdown/close. One worker is the sole
receiver, so another reader cannot consume readiness between poll and recv.

The Wii helper at the beginning of `fb_DevTcpRead` consumes currently available
bytes. When that first copy is nonempty, the read must return immediately.
Waiting a second time for the unused destination capacity can deadlock a
request/reply exchange: the peer is waiting for the application to process the
bytes already copied. In TLS this can delay handshake flights or capability
negotiation until an unrelated packet or peer timeout arrives.

`tests/wii/tcp.bas` requests 64 bytes from a peer that sends four bytes and waits
for an acknowledgement. `tests/wii/tcp-peer.py` supplies that loopback peer on
port 18078. Run the peer before the guest. Build the BASIC test for Wii with
multithreading enabled and use `build_scripts/wii-run-program.py` with
`--watch-file TCPCHECK.TXT`. The test requires the partial read to finish in less
than one second, acknowledges only valid data, and writes a nonce-bound exit
receipt. It uses no production service or account credentials.

`tests/wii/tcp_leak.bas` repeatedly opens and closes 32 idle loopback connections.
Its peer accepts each connection and waits for EOF before accepting the next.
This exceeds Dolphin's 24 raw IOS sockets and exposes descriptor leakage that a
single successful shutdown cannot detect.

Positive `SLEEP` delays use libogc's `nanosleep` service. Repeatedly calling
`LWP_YieldThread` does not make the caller inactive, and therefore cannot let
lower-priority runnable threads advance. Timed suspension also avoids spending
the entire delay polling the PowerPC timebase. Zero or negative runtime delays
retain the existing one-yield behavior. Milliseconds are split into valid
`timespec` fields, and interrupted sleeps continue with the reported remainder.

<!-- end of wii-runtime-io.md -->

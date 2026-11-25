/*
    Project: FreeBASIC browser corpus
    File: fb-corpus-fbvnc-websocket.js

    Purpose:
        Carry the fb-VNC viewer's RFB byte stream over a browser WebSocket.

    Responsibilities:
        Open one connection, queue binary input and copy bytes into Wasm.
        Bound both the receive queue and the browser's outgoing queue.
        The viewer owns RFB negotiation, decoding and presentation.
*/

addToLibrary({
    $fbVncSocket: {
        // Eight MiB holds a complete 1080p Raw framebuffer plus RFB headers.
        limit: 8 * 1024 * 1024,
        session: null,
        find(handle) {
            return handle === 1 ? fbVncSocket.session : null;
        },
    },

    fb_vnc_ws_open__deps: ["$fbVncSocket", "$UTF8ToString"],
    fb_vnc_ws_open: function(address) {
        if (fbVncSocket.session) return 0;
        let socket;
        let queue;
        try {
            const url = new URL(UTF8ToString(address));
            if ((url.protocol !== "wss:" && url.protocol !== "ws:") ||
                url.username || url.password || url.hash) return 0;
            queue = new Uint8Array(fbVncSocket.limit);
            socket = new WebSocket(url, "binary");
        } catch {
            return 0;
        }
        const session = { socket, queue, head: 0, size: 0, failed: false };
        fbVncSocket.session = session;
        socket.binaryType = "arraybuffer";
        socket.onmessage = event => {
            if (session.failed) return;
            const bytes = event.data;
            // RFB is a stream. A frame may hold any fraction of a message.
            if (!(bytes instanceof ArrayBuffer) || bytes.byteLength > queue.length - session.size) {
                session.failed = true;
                socket.close();
                return;
            }
            const data = new Uint8Array(bytes);
            const tail = (session.head + session.size) % queue.length;
            const first = Math.min(data.length, queue.length - tail);
            queue.set(data.subarray(0, first), tail);
            queue.set(data.subarray(first), 0);
            session.size += data.length;
        };
        return 1;
    },

    fb_vnc_ws_close__deps: ["$fbVncSocket"],
    fb_vnc_ws_close: function(handle) {
        const session = fbVncSocket.find(handle);
        if (!session) return;
        session.socket.onmessage = null;
        session.socket.close();
        fbVncSocket.session = null;
    },

    fb_vnc_ws_state__deps: ["$fbVncSocket"],
    fb_vnc_ws_state: function(handle) {
        const session = fbVncSocket.find(handle);
        // -1 signals a missing socket or a rejected receive frame.
        return session && !session.failed ? session.socket.readyState : -1;
    },

    fb_vnc_ws_available__deps: ["$fbVncSocket"],
    fb_vnc_ws_available: function(handle) {
        const session = fbVncSocket.find(handle);
        return session && !session.failed ? session.size : 0;
    },

    fb_vnc_ws_read__deps: ["$fbVncSocket"],
    fb_vnc_ws_read: function(handle, destination, length) {
        const session = fbVncSocket.find(handle);
        const address = destination >>> 0;
        if (!session || session.failed || length < 1 ||
            address === 0 || address > HEAPU8.length - length) return -1;
        const count = Math.min(length, session.size);
        const first = Math.min(count, session.queue.length - session.head);
        HEAPU8.set(session.queue.subarray(session.head, session.head + first), address);
        HEAPU8.set(session.queue.subarray(0, count - first), address + first);
        session.head = (session.head + count) % session.queue.length;
        session.size -= count;
        return count;
    },

    fb_vnc_ws_write__deps: ["$fbVncSocket"],
    fb_vnc_ws_write: function(handle, source, length) {
        const session = fbVncSocket.find(handle);
        const address = source >>> 0;
        if (!session || session.failed || session.socket.readyState !== WebSocket.OPEN ||
            length < 1 || address === 0 || address > HEAPU8.length - length ||
            length > fbVncSocket.limit - session.socket.bufferedAmount) return 0;
        try {
            session.socket.send(HEAPU8.subarray(address, address + length));
        } catch {
            return 0;
        }
        return 1;
    },
});

/* end of fb-corpus-fbvnc-websocket.js */

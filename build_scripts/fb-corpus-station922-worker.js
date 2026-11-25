/*
    Project: FreeBASIC browser corpus
    File: fb-corpus-station922-worker.js
    Purpose: Run one Station922 request against a private virtual document root.
    Responsibilities: Mount request, configuration and files, then return the response.
    Station922 owns HTTP parsing, site selection, MIME lookup and file serving.
*/
"use strict";

const sampleFiles = [
    ["index.html", "<!doctype html>\n<title>Station922 sample</title>\n<h1>Station922 served this file.</h1>\n"],
    ["guide/about.txt", "This is a nested document served by Station922.\n"],
];

function nativeIni(text) {
    // Station922's configuration reader expects a UTF-16 little-endian BOM.
    const bytes = new Uint8Array(2 + text.length * 2);
    const words = new DataView(bytes.buffer);
    words.setUint16(0, 0xfeff, true);
    for (let i = 0; i < text.length; i++) words.setUint16(2 + i * 2, text.charCodeAt(i), true);
    return bytes;
}

function splitResponse(bytes) {
    for (let i = 0; i < Math.min(bytes.length - 1, 32768); i++) {
        if (bytes[i] !== 10) continue;
        // The runtime uses the target's newline convention for PRINT #.
        const length = bytes[i + 1] === 10 ? 2 : bytes[i + 1] === 13 && bytes[i + 2] === 10 ? 3 : 0;
        if (!length) continue;
        const headers = new TextDecoder().decode(bytes.subarray(0, i)).trimEnd();
        if (!/^HTTP\/1\.1 \d{3} /.test(headers)) break;
        return { headers, body: bytes.slice(i + length) };
    }
    throw new Error("Incomplete Station922 response");
}

self.addEventListener("message", ({ data }) => {
    let finished = false;
    function reply(value, transfer) {
        if (finished) return;
        finished = true;
        self.postMessage(value, transfer);
    }

    self.Module = {
        preRun: [function () {
            const { FS } = Module;
            const config = "[preview.test]\r\nHost=preview.test\r\nPhysicalDir=/site\r\nMethods=GET,HEAD\r\n";
            FS.writeFile("/WebSites.ini", nativeIni(config));
            FS.mkdirTree("/site");
            for (const [relative, bytes] of data.files.length ? data.files : sampleFiles) {
                const parts = relative.split("/");
                if (parts.some(part => !part || part === "." || part === ".." || /[\\\x00-\x1f]/.test(part))) {
                    throw new Error("Invalid document path");
                }
                const file = "/site/" + relative;
                FS.mkdirTree(file.slice(0, file.lastIndexOf("/")));
                FS.writeFile(file, bytes);
            }
            FS.writeFile("/request.http", `${data.method} ${data.target} HTTP/1.1\r\nHost: ${data.host}\r\n\r\n`);
        }],
        onExit(code) {
            try {
                if (code !== 0) throw new Error("Station922 could not open its inputs");
                const { headers, body } = splitResponse(Module.FS.readFile("/response.http"));
                reply({ headers, body }, [body.buffer]);
            } catch {
                reply({ error: "Station922 did not return a complete response." });
            }
        },
        onAbort() { reply({ error: "Station922 stopped unexpectedly." }); },
    };
    try { importScripts("index.js"); }
    catch { reply({ error: "Could not load Station922." }); }
}, { once: true });

/* end of fb-corpus-station922-worker.js */

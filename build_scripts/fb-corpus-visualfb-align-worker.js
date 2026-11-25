/*
    Project: FreeBASIC browser corpus
    File: fb-corpus-visualfb-align-worker.js
    Purpose: Run one alignment request in the FreeBASIC worker.
    Responsibilities: Mount the input and return the formatter's result.
    The worker leaves source parsing and alignment to the compiled command.
*/

"use strict";

self.addEventListener("message", ({ data: request }) => {
    let message = "";
    let done = false;
    const finish = result => {
        if (done) return;
        done = true;
        self.postMessage(result);
    };
    self.Module = {
        arguments: [request.first, request.last, "/picked.bas"],
        preRun: [() => {
            __fb_rtlib.console.write = text => { message += text; };
            Module.FS.writeFile("/picked.bas", new TextEncoder().encode(request.source));
        }],
        onExit(status) {
            if (status !== 0) {
                finish({ error: message.trim() || "Alignment failed." });
                return;
            }
            try {
                const output = Module.FS.readFile("/formatted.bas", { encoding: "utf8" });
                finish({ output, message: message.trim() });
            } catch {
                finish({ error: "The formatter finished without a result." });
            }
        },
        onAbort: () => finish({ error: "The formatter stopped unexpectedly." }),
    };
    try {
        importScripts("index.js");
    } catch {
        finish({ error: "Could not load the formatter." });
    }
}, { once: true });

/* end of fb-corpus-visualfb-align-worker.js */

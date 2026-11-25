/*
    Project: FreeBASIC browser corpus
    File: fb-corpus-opensesh-worker.js
    Purpose: Convert one MIDI file using the compiled OpenSesh programs.
    Responsibilities: Mount input and return the report and binary output.
    BASIC owns the MIDI reader, quantization, and ProTracker file writer.
*/

"use strict";

self.addEventListener("message", ({ data: buffer }) => {
    let done = false;

    function finish(reply) {
        if (done) return;
        done = true;
        self.postMessage(reply, reply.mod ? [reply.mod] : []);
    }

    self.Module = {
        preRun: [function() {
            __fb_rtlib.console.write = () => {};
            Module.FS.writeFile("/input.mid", new Uint8Array(buffer), { canOwn: true });
        }],
        onExit(status) {
            try {
                const report = Module.FS.readFile("/conversion.txt", { encoding: "utf8" }).trim();
                if (status === 0) {
                    const mod = Module.FS.readFile("/converted.mod");
                    finish({ report, mod: mod.buffer });
                } else {
                    finish({ report, error: "Conversion failed." });
                }
            } catch {
                finish({ error: "OpenSesh could not read its conversion result." });
            }
        },
        onAbort: () => finish({ error: "OpenSesh stopped unexpectedly." }),
    };

    try {
        importScripts("index.js");
    } catch {
        finish({ error: "Could not load OpenSesh." });
    }
}, { once: true });

/* end of fb-corpus-opensesh-worker.js */

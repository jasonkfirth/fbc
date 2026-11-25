/*
    Project: FreeBASIC browser corpus
    File: fb-corpus-kiwi-worker.js
    Purpose: Run Kiwi's compiled URL sample for one address.
    Responsibilities: Pass the argument and return the BASIC program output.
    URL parsing and the page controls live outside this worker.
*/

"use strict";

self.addEventListener("message", ({ data: address }) => {
    let done = false;

    function finish(result) {
        if (done) return;
        done = true;
        self.postMessage(result);
    }

    self.Module = {
        arguments: [address],
        preRun: [function() {
            __fb_rtlib.console.write = () => {};
        }],
        onExit(status) {
            if (status !== 0) {
                finish({ error: status === 2 ? "Enter an absolute HTTP or HTTPS URL without credentials."
                    : "Kiwi could not inspect the address." });
                return;
            }
            try {
                const report = Module.FS.readFile("/kiwi-result.txt", { encoding: "utf8" });
                const parts = /^Host:([^\r\n]+)\r?\nPath:([^\r\n]*)\r?\nProtocol:(https?)$/.exec(report.trimEnd());
                if (!parts) throw new Error("Malformed report");
                const [, host, resource, protocol] = parts;
                finish({ host, resource, protocol });
            } catch {
                finish({ error: "Kiwi did not return URL parts." });
            }
        },
        onAbort: () => finish({ error: "Kiwi stopped unexpectedly." }),
    };

    try {
        importScripts("index.js");
    } catch {
        finish({ error: "Could not load Kiwi." });
    }
}, { once: true });

/* end of fb-corpus-kiwi-worker.js */

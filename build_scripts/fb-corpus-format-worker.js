/*
    Project: FreeBASIC browser corpus
    File: fb-corpus-format-worker.js
    Purpose: Launch one compiled fbrtLib Format request.
    Responsibilities: Pass the value and mask to BASIC and capture the result.
    The page owns input fields, timeouts and presentation.
*/

"use strict";

self.addEventListener("message", ({ data }) => {
    let output = "";
    let ended = false;
    function finish(error) {
        if (ended) return;
        ended = true;
        self.postMessage({ output: error ? "" : output, error });
    }

    self.Module = {
        arguments: [data.value, data.mask],
        preRun: [function() {
            __fb_rtlib.console.write = text => {
                output += text;
            };
        }],
        onExit: code => finish(code ? "The formatter returned an error." : ""),
        onAbort: () => finish("The formatter stopped unexpectedly."),
    };
    try {
        importScripts("index.js");
    } catch {
        finish("Could not load the formatter.");
    }
}, { once: true });

/* end of fb-corpus-format-worker.js */

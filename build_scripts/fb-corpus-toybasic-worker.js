/*
    Project: FreeBASIC browser corpus
    File: fb-corpus-toybasic-worker.js
    Purpose: Run one ToyBASIC expression through the compiled interpreter.
    Responsibilities: Pass the expression and relay BASIC's result or error.
    The page owns input, timing, and presentation.
*/

"use strict";

self.addEventListener("message", ({ data: expression }) => {
    let output = "";
    let finished = false;
    function finish(result, error) {
        if (finished) return;
        finished = true;
        self.postMessage({ result, error });
    }

    self.Module = {
        arguments: ["--expression", expression],
        preRun: [function() {
            __fb_rtlib.console.write = text => { output += text; };
        }],
        onExit(code) {
            if (code === 0) finish(output.trim(), "");
            else if (code === 2) finish("", output.trim());
            else finish("", "ToyBASIC returned an error.");
        },
        onAbort: () => finish("", "ToyBASIC stopped unexpectedly."),
    };
    try {
        importScripts("index.js");
    } catch {
        finish("", "Could not load ToyBASIC.");
    }
}, { once: true });

/* end of fb-corpus-toybasic-worker.js */

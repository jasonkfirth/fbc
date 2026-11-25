/*
    Project: FreeBASIC browser corpus
    File: fb-corpus-converter-worker.js
    Purpose: Run one FlightGear conversion and return the BASIC result.
    Responsibilities: Supply the form's values to INPUT and capture PRINT.
    Range controls and result presentation belong to the page.
*/

"use strict";

self.addEventListener("message", ({ data }) => {
    const { operation, values } = data;
    let input = 0;
    let output = "";
    let ended = false;
    let unexpectedPrompt = false;

    function finish(text) {
        if (ended) return;
        ended = true;
        self.postMessage({ text });
    }

    self.Module = {
        arguments: [operation],
        preRun: [function() {
            __fb_rtlib.console.write = text => {
                if (output.length < 4096) output += text;
            };
        }],
        fbReadLine() {
            // The last unanswered prompt comes before each call to INPUT.
            // Only the output after the final answer is the result.
            output = "";
            if (input < values.length) return String(values[input++]);
            unexpectedPrompt = true;
            finish("The calculator could not use those values.");
            return "";
        },
        onExit(status) {
            const text = output.trim();
            if (status === 0 && !unexpectedPrompt && input === values.length && text) finish(text);
            else finish("The calculator did not return a result.");
        },
        onAbort() {
            finish("The calculator stopped unexpectedly.");
        },
    };

    try {
        importScripts("index.js");
    } catch {
        finish("Could not load the calculator.");
    }
}, { once: true });

/* end of fb-corpus-converter-worker.js */

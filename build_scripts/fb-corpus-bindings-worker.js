/*
    Project: FreeBASIC browser corpus
    File: fb-corpus-bindings-worker.js
    Purpose: Run one fbfrog conversion off the browser's UI thread.
    Responsibilities: Stage a header, collect diagnostics, and return the .bi.
    The page owns input controls and download links.
*/

"use strict";

self.addEventListener("message", ({ data: header }) => {
    let log = "";
    let finished = false;
    const inputPath = "/input.h";
    const outputPath = "/bindings.bi";

    function append(text) {
        // Bound diagnostics independently of the input size and parser errors.
        log += String(text).slice(0, Math.max(0, 65536 - log.length));
    }

    function finish(status, error) {
        if (finished) return;
        finished = true;
        let result;
        if (status === 0) {
            try {
                result = self.Module.FS.readFile(outputPath, { encoding: "utf8" });
            } catch (exception) {
                error = "fbfrog did not write a bindings file.";
            }
        }
        self.postMessage({ result, log, error: error || (status !== 0 && "fbfrog could not convert this header.") });
    }

    self.Module = {
        arguments: [inputPath, "-emit", "*", outputPath],
        preRun: [function() {
            self.Module.FS.writeFile(inputPath, header);
            // BASIC's console writes go through fb_rtlib. A worker has no DOM
            // terminal, so forward those writes to the same diagnostic buffer.
            __fb_rtlib.console.write = append;
        }],
        print: text => append(text + "\n"),
        printErr: text => append(text + "\n"),
        onAbort: () => finish(1, "The fbfrog runtime stopped unexpectedly."),
        onExit: status => finish(status),
    };

    try {
        importScripts("index.js");
    } catch (error) {
        finish(1, "The browser could not load fbfrog.");
    }
}, { once: true });

/* end of fb-corpus-bindings-worker.js */

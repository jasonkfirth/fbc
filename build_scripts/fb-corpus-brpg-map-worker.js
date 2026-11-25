/*
    Project: FreeBASIC browser corpus
    File: fb-corpus-brpg-map-worker.js
    Purpose: Run one map conversion in an isolated FreeBASIC filesystem.
    Responsibilities: Mount the uploaded file and return the converted map.
    The FreeBASIC program owns validation and binary encoding.
*/

"use strict";

self.addEventListener("message", ({ data }) => {
    let output = "";
    let finished = false;
    const finish = result => {
        if (finished) return;
        finished = true;
        postMessage(result, result.map ? [result.map] : []);
    };
    self.Module = {
        arguments: ["/picked.map", "/converted.map"],
        preRun: [() => {
            __fb_rtlib.console.write = text => { output += text; };
            Module.FS.writeFile("/picked.map", new Uint8Array(data));
        }],
        onExit(status) {
            if (status !== 0) {
                finish({ error: output.trim() || "Map conversion failed." });
                return;
            }
            try {
                finish({ map: Module.FS.readFile("/converted.map").buffer });
            } catch {
                finish({ error: "The converter finished without an output map." });
            }
        },
        onAbort: () => finish({ error: "The converter stopped unexpectedly." }),
    };
    try {
        importScripts("index.js");
    } catch {
        finish({ error: "Could not load the converter." });
    }
}, { once: true });

/* end of fb-corpus-brpg-map-worker.js */

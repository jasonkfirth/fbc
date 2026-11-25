/*
    Project: FreeBASIC browser corpus
    File: fb-corpus-imageproc-worker.js
    Purpose: Run one fb-imageproc conversion off the page's UI thread.
    Responsibilities: Stage RGBA bytes for BASIC and return the result.
    Image decoding and downloads belong to the page.
*/

"use strict";

self.addEventListener("message", ({ data }) => {
    let finished = false;
    function finish(error) {
        if (finished) return;
        finished = true;
        if (error) {
            self.postMessage({ error });
            return;
        }
        try {
            const pixels = self.Module.FS.readFile("/result.rgba");
            if (pixels.length !== data.pixels.byteLength) throw new Error("Unexpected output size");
            self.postMessage({ pixels: pixels.buffer }, [pixels.buffer]);
        } catch {
            self.postMessage({ error: "fb-imageproc did not write a complete image." });
        }
    }

    self.Module = {
        arguments: ["--rgba", String(data.width), String(data.height), data.method],
        preRun: [function() {
            self.Module.FS.writeFile("/source.rgba", new Uint8Array(data.pixels));
        }],
        onExit: code => finish(code ? "fb-imageproc could not convert the image." : ""),
        onAbort: () => finish("The image converter stopped unexpectedly."),
    };
    try {
        importScripts("index.js");
    } catch {
        finish("Could not load fb-imageproc.");
    }
}, { once: true });

/* end of fb-corpus-imageproc-worker.js */

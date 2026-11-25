/*
    Project: FreeBASIC browser corpus
    File: fb-corpus-azdecrypt-worker.js
    Purpose: Run a single bounded AZdecrypt search and return its candidate.
    The compiled BASIC program owns ciphertext parsing, scoring and search.
*/
"use strict";

self.addEventListener("message", ({ data }) => {
    let complete = false;
    let diagnostic = "";
    function reply(result) {
        if (complete) return;
        complete = true;
        self.postMessage(result);
    }

    self.Module = {
        arguments: [String(data.restarts), String(data.seed)],
        preRun: [function () {
            Module.FS.writeFile("/cipher.txt", data.ciphertext);
            __fb_rtlib.console.write = text => { if (diagnostic.length < 1000) diagnostic += text; };
        }],
        onExit(code) {
            if (code !== 0) {
                const message = code === 3 ? "Use 40 to 4,000 letters and at least two distinct letters." :
                    code === 4 ? "The search did not find a candidate. Try a different seed or a longer passage." :
                    diagnostic.trim() || "Could not run AZdecrypt.";
                reply({ error: message });
                return;
            }
            try {
                const solution = Module.FS.readFile("/solution.txt", { encoding: "utf8" });
                const values = Module.FS.readFile("/score.txt", { encoding: "utf8" }).trim().split(/\s+/);
                const [score, iterations] = values.map(Number);
                if (!solution || !Number.isFinite(score) || !Number.isInteger(iterations) || iterations < 1) {
                    throw new Error("Missing solution details");
                }
                reply({ solution, score, iterations });
            } catch {
                reply({ error: "AZdecrypt did not return a complete candidate." });
            }
        },
        onAbort() { reply({ error: "AZdecrypt stopped unexpectedly." }); },
    };
    try { importScripts("index.js"); }
    catch { reply({ error: "Could not load AZdecrypt." }); }
}, { once: true });

/* end of fb-corpus-azdecrypt-worker.js */

/*
    Project: FreeBASIC browser corpus
    File: fb-corpus-fblint-worker.js
    Purpose: Run fblint on one submitted BASIC file.
    Responsibilities: Mount the input and return the linter's JSONL records.
    The browser page owns source entry and diagnostic presentation.
*/
"use strict";

self.addEventListener("message", ({ data }) => {
    const input = data.header ? "/source.bi" : "/source.bas";
    const report = "/findings.jsonl";
    const args = ["--target", data.target, "--profile", data.profile,
        data.honorSuppressions ? "--honor-suppressions" : "--no-suppressions",
        "--no-config", "--no-semantic", "--format", "jsonl", "--output-file", report, input];
    let consoleText = "";
    let finished = false;

    function respond(result) {
        if (finished) return;
        finished = true;
        self.postMessage(result);
    }

    self.Module = {
        arguments: args,
        preRun: [function () {
            Module.FS.writeFile(input, data.source);
            __fb_rtlib.console.write = text => {
                if (consoleText.length < 4096) consoleText += text;
            };
        }],
        onExit(code) {
            // Code 1 can be a complete report containing lint errors.
            if (code !== 0 && code !== 1) {
                respond({ error: consoleText.trim() || "The linter could not check this file." });
                return;
            }
            try {
                const results = Module.FS.readFile(report, { encoding: "utf8" });
                const findings = [];
                let summary;
                for (const line of results.split("\n")) {
                    if (!line.trim()) continue;
                    const record = JSON.parse(line);
                    if (record.type === "finding" && findings.length < 500) findings.push(record);
                    if (record.type === "summary") summary = record;
                }
                if (!summary) throw new Error("Missing linter summary");
                respond({ findings, summary });
            } catch {
                respond({ error: "The linter did not return a complete report." });
            }
        },
        onAbort() { respond({ error: "The linter stopped unexpectedly." }); },
    };

    try {
        importScripts("index.js");
    } catch {
        respond({ error: "Could not load the linter." });
    }
}, { once: true });

/* end of fb-corpus-fblint-worker.js */

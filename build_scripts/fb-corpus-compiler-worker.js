/*
    Project: FreeBASIC browser corpus
    File: fb-corpus-compiler-worker.js
    Purpose: Launch one upstream FreeBASIC compiler job.
    Responsibilities: Mount source and headers, pass compiler arguments,
    and return diagnostics and the stage output. The compiler handles
    parsing and code generation.
*/

"use strict";

self.addEventListener("message", ({ data: request }) => {
    const parse = request.mode === "lbasic-parse";
    const bitcode = request.mode === "lbasic-ir";
    const lbasic = parse || bitcode;
    const preprocess = request.mode === "preprocess";
    const outputPath = lbasic ? (parse ? "/main.parse" : "/main.bc") : preprocess ? "/main.pp.bas" : "/main.c";
    const maxOutput = 4 * 1024 * 1024;
    const maxDiagnostics = 128 * 1024;
    let diagnostics = "";
    let done = false;
    const finish = (result, transfers = []) => {
        if (done) return;
        done = true;
        self.postMessage({ diagnostics: diagnostics.trim(), ...result }, transfers);
    };
    self.Module = {
        // -r otherwise treats the source as a secondary module and emits a constructor.
        arguments: lbasic ? ["-t", "--no-core", "-e", parse ? "parse" : "ir", "-o", outputPath, "/main.bas"] :
            [preprocess ? "-pp" : "-r", "-m", "main", "-gen", "gcc", "-prefix", "/", "-i", "/headers", "/main.bas"],
        preRun: [() => {
            __fb_rtlib.console.write = text => {
                if (diagnostics.length < maxDiagnostics) diagnostics += text.slice(0, maxDiagnostics - diagnostics.length);
            };
            Module.FS.writeFile("/main.bas", new TextEncoder().encode(request.source));
            if (!lbasic) {
                Module.FS.mkdir("/headers");
                for (const { name, bytes } of request.headers) {
                    Module.FS.writeFile("/headers/" + name, new Uint8Array(bytes));
                }
            }
        }],
        onExit(status) {
            if (status !== 0) {
                finish({ error: parse ? "Parsing failed." : "Compilation failed." });
                return;
            }
            try {
                if (Module.FS.stat(outputPath).size > maxOutput) {
                    finish({ error: "Compiler output exceeds 4 MiB." });
                    return;
                }
                if (bitcode) {
                    // FS.readFile returns a copy that can cross the worker boundary.
                    const output = Module.FS.readFile(outputPath);
                    finish({ output }, [output.buffer]);
                } else {
                    finish({ output: Module.FS.readFile(outputPath, { encoding: "utf8" }) });
                }
            } catch {
                finish({ error: "The compiler exited without writing the requested file." });
            }
        },
        onAbort: () => finish({ error: "The compiler stopped unexpectedly." }),
    };
    try {
        importScripts("index.js");
    } catch {
        finish({ error: "Could not load the compiler." });
    }
}, { once: true });

/* end of fb-corpus-compiler-worker.js */

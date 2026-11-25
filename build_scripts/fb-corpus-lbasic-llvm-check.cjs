/*
    Project: FreeBASIC browser corpus
    File: fb-corpus-lbasic-llvm-check.cjs
    Purpose: Exercise the LLVM C interface and L-BASIC code generation.
    The Wasm programs write the bitcode. This check supplies inputs and
    copies the resulting files out of the browser for execution checks.
*/

"use strict";

const assert = require("node:assert/strict");
const { createHash } = require("node:crypto");
const { readFile, writeFile } = require("node:fs/promises");
const { createServer } = require("node:http");
const path = require("node:path");
const { chromium } = require("playwright-core");

const contentTypes = { "index.html": "text/html", "index.js": "text/javascript", "index.wasm": "application/wasm" };

async function upstreamCase(stage, file, title) {
    const text = (await readFile(path.join(stage, "tests", file), "utf8")).replace(/\r\n/g, "\n");
    const heading = `$title: ${title}\n`;
    const start = text.indexOf(heading);
    assert.ok(start >= 0, title);
    const expected = /\$expect: stdout(_exact)?\n/.exec(text.slice(start + heading.length));
    assert.ok(expected, title);
    const sourceEnd = start + heading.length + expected.index;
    const outputStart = sourceEnd + expected[0].length;
    const end = text.indexOf("$finish\n", outputStart);
    assert.ok(end > outputStart, title);
    return { source: text.slice(start + heading.length, sourceEnd),
        stdout: text.slice(outputStart, end), exact: Boolean(expected[1]) };
}

function withJob(shell, job) {
    const launch = /<script async type="text\/javascript" src="index\.js"><\/script>/;
    assert.match(shell, launch);
    return shell.replace(launch, tag => `<script>
let diagnostics = "";
const test = ${JSON.stringify(job).replace(/</g, "\\u003c")};
if (test.source) Module.arguments = ["-t", "--no-core", "-e", test.mode || "ir", "-o", "/output.bc", "/main.bas"];
Module.preRun.push(() => {
    __fb_rtlib.console.write = text => { diagnostics += text; };
    if (test.source) Module.FS.writeFile("/main.bas", new TextEncoder().encode(test.source));
});
Module.onExit = status => {
    const result = { status, diagnostics };
    const filename = test.source ? "/output.bc" : "/probe.bc";
    if (!test.source && status === 0) result.ir = Module.FS.readFile("/probe.ll", { encoding: "utf8" });
    if (Module.FS.analyzePath(filename).exists) result.bitcode = Array.from(Module.FS.readFile(filename));
    window.llvmProbe = result;
};
</script>` + tag);
}

async function main() {
    const [, , app, stage] = process.argv;
    assert.ok(app && process.argv.length <= 4, "Usage: node fb-corpus-lbasic-llvm-check.cjs <app directory> [staged L-BASIC]");
    const directory = path.resolve(app);
    const shell = await readFile(path.join(directory, "index.html"), "utf8");
    const jobs = stage ? {
        arithmetic: { source: "DIM amount AS LONG\namount = 6 * 7\nPRINT amount\n", stdout: "42" },
        counter: await upstreamCase(stage, "for.test", "ascending counter"),
        text: await upstreamCase(stage, "print.test", "semicolon joined"),
        // The pinned upstream LLVM emitter accepts scalar and array types.
        // Its parser emits record ASTs, but ll_type rejects them during IR generation.
        record: { ...await upstreamCase(stage, "udt.test", "simple udt"), error: "bad ll type: T" },
        invalid: { source: "DIM amount AS LONG\namount = (\n", error: "Unexpected" },
        unsupported: { source: "PRINT 42\n", mode: "obj", error: "Choose -e parse or -e ir" },
    } : { probe: {} };
    const server = createServer(async (request, response) => {
        const url = new URL(request.url, "http://127.0.0.1");
        const name = url.pathname.slice(1);
        if (!Object.hasOwn(contentTypes, name)) return response.writeHead(404).end();
        try {
            const test = url.searchParams.get("case");
            if (name === "index.html" && !Object.hasOwn(jobs, test)) return response.writeHead(404).end();
            const data = name === "index.html" ? withJob(shell, jobs[test]) : await readFile(path.join(directory, name));
            response.writeHead(200, { "Content-Type": contentTypes[name] }).end(data);
        } catch {
            response.writeHead(500).end();
        }
    });
    await new Promise((resolve, reject) => {
        server.once("error", reject);
        server.listen(0, "127.0.0.1", resolve);
    });
    let browser;
    const rows = [];
    try {
        browser = await chromium.launch({ executablePath: process.env.FBC_BROWSER });
        for (const [test, job] of Object.entries(jobs)) {
            const page = await browser.newPage();
            const errors = [];
            page.on("pageerror", error => errors.push(error.message));
            try {
                await page.goto(`http://127.0.0.1:${server.address().port}/index.html?case=${test}`);
                try {
                    await page.waitForFunction(() => window.llvmProbe, undefined, { timeout: 20000 });
                } catch (error) {
                    throw new Error(`${test}: ${errors.join("\n") || error.message}`, { cause: error });
                }
                const result = await page.evaluate(() => window.llvmProbe);
                assert.deepEqual(errors, []);
                assert.equal(result.status, job.error ? 1 : 0, `${test}: ${result.diagnostics}`);
                if (job.error) {
                    assert.ok(result.diagnostics.includes(job.error), result.diagnostics);
                    assert.equal(result.bitcode, undefined);
                    rows.push({ test, diagnostic: result.diagnostics.trim() });
                    console.log(`LBASIC_WASM_REJECTED ${test}`);
                    continue;
                }
                if (!stage) {
                    assert.match(result.diagnostics, /LBASIC_LLVM_PROBE_OK/);
                    assert.match(result.ir, /target triple = "wasm32-unknown-emscripten"/);
                    assert.match(result.ir, /define i32 @probe_answer\(\)[\s\S]*call i32 @probe_add\(i32 19, i32 23\)/);
                    await writeFile(path.join(directory, "probe.ll"), result.ir, "utf8");
                }
                const bitcode = Buffer.from(result.bitcode);
                assert.equal(bitcode.subarray(0, 4).toString("hex"), "4243c0de");
                const output = test + ".bc";
                await writeFile(path.join(directory, output), bitcode);
                const sha256 = createHash("sha256").update(bitcode).digest("hex");
                rows.push({ test, output, bytes: bitcode.length, sha256, stdout: job.stdout, exact: job.exact });
                console.log(`${stage ? "LBASIC_WASM_EMITTED" : "LLVM_WASM_BROWSER_PASS"} ${test} bitcode=${bitcode.length}`);
            } finally {
                await page.close();
            }
        }
        await writeFile(path.join(directory, "browser-bitcode-results.json"), JSON.stringify(rows, null, 2) + "\n");
    } finally {
        server.close();
        if (browser) await browser.close();
    }
}

main().catch(error => { console.error(error); process.exitCode = 1; });

/* end of fb-corpus-lbasic-llvm-check.cjs */

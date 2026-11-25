/*
    Project: FreeBASIC browser corpus
    File: fb-corpus-lbasic-check.cjs
    Purpose: Check L-BASIC's parser table generators in Chromium.
    Responsibilities: Mount the original rule files, compare the generated
    tables with reference data, and check diagnostics for a malformed rule.
    The compiled generators handle rule parsing and file output.
*/

"use strict";

const assert = require("node:assert/strict");
const { createHash } = require("node:crypto");
const { readFile, writeFile } = require("node:fs/promises");
const { createServer } = require("node:http");
const path = require("node:path");
const { chromium } = require("playwright-core");

const tools = [
    { slug: "lbasic-state-generator", rules: "ts.rules", outputs: ["ts_data.bi", "ts_data.bm"],
        invalid: "?invalid rule\n", diagnostic: /Syntax error/i },
    { slug: "lbasic-token-generator", rules: "tokens.list", outputs: ["token_data.bi", "token_registrations.bm"],
        invalid: "badtype name\n", diagnostic: /Unknown token type/i },
];
const contentTypes = { "index.html": "text/html", "index.js": "text/javascript", "index.wasm": "application/wasm" };

function withJob(shell, rules, input, outputs) {
    const config = JSON.stringify({ rules, input, outputs }).replace(/</g, "\\u003c");
    const boot = `<script>
const job = ${config};
let diagnostics = "";
Module.arguments = ["/" + job.rules, ...job.outputs.map(name => "/" + name)];
Module.preRun.push(function() {
    __fb_rtlib.console.write = text => { diagnostics += text; };
    Module.FS.writeFile(Module.arguments[0], new TextEncoder().encode(job.input));
});
Module.onExit = function(status) {
    const tables = status === 0 ? job.outputs.map(name => Module.FS.readFile("/" + name, { encoding: "utf8" })) : [];
    window.generatorResult = { status, diagnostics, tables };
};
</script>`;
    const launch = /<script async type="text\/javascript" src="index\.js"><\/script>/;
    assert.match(shell, launch);
    return shell.replace(launch, tag => boot + tag);
}

async function runJob(browser, base, tool, bad) {
    const page = await browser.newPage();
    const errors = [];
    page.on("pageerror", error => errors.push(error.message));
    try {
        await page.goto(base + tool.slug + "/index.html" + (bad ? "?bad" : ""));
        await page.waitForFunction(() => window.generatorResult, undefined, { timeout: 12000 });
        assert.deepEqual(errors, []);
        return await page.evaluate(() => window.generatorResult);
    } catch (error) {
        throw new Error(`${tool.slug}: ${errors.join("\n") || error.message}`, { cause: error });
    } finally {
        await page.close();
    }
}

async function main() {
    assert.equal(process.argv.length, 5, "Usage: node fb-corpus-lbasic-check.cjs <build> <staged corpus> <reference tables>");
    const [, , build, corpus, reference] = process.argv;
    const cases = [];
    for (const tool of tools) {
        const rules = await readFile(path.join(corpus, "L-BASIC", "compiler", "parser", tool.rules), "utf8");
        const shell = await readFile(path.join(build, "apps", tool.slug, "index.html"), "utf8");
        const wasm = await readFile(path.join(build, "apps", tool.slug, "index.wasm"));
        cases.push({ ...tool, valid: withJob(shell, tool.rules, rules, tool.outputs),
            bad: withJob(shell, tool.rules, tool.invalid, tool.outputs),
            wasmSha256: createHash("sha256").update(wasm).digest("hex"),
            rulesSha256: createHash("sha256").update(rules).digest("hex") });
    }
    const server = createServer(async (request, response) => {
        const { pathname, search } = new URL(request.url, "http://localhost");
        const [, slug, name] = pathname.split("/");
        const job = cases.find(tool => tool.slug === slug);
        if (!job || !Object.hasOwn(contentTypes, name)) return response.writeHead(404).end();
        try {
            const bytes = name === "index.html" ? (search === "?bad" ? job.bad : job.valid) :
                await readFile(path.join(build, "apps", slug, name));
            response.writeHead(200, { "Content-Type": contentTypes[name] }).end(bytes);
        } catch {
            response.writeHead(500).end();
        }
    });
    await new Promise((resolve, reject) => {
        server.once("error", reject);
        server.listen(0, "127.0.0.1", resolve);
    });
    const base = `http://127.0.0.1:${server.address().port}/`;
    let browser;
    try {
        browser = await chromium.launch({ executablePath: process.env.FBC_BROWSER });
        const rows = [];
        for (const tool of cases) {
            const valid = await runJob(browser, base, tool, false);
            assert.equal(valid.status, 0, valid.diagnostics);
            const outputs = {};
            for (const [index, name] of tool.outputs.entries()) {
                // Numeric formatting differs in CONST declarations, the state
                // array bound, and padding at the end of PRINT # lines.
                const trimPrintPadding = text => text.replace(/[ \t]+$/gm, "").replace(
                    /^((?:CONST [A-Z0-9_]+[ \t]*=|DIM SHARED t_states~%\(127,))[ \t]+(?=[+-]?\d)/gmi, "$1");
                const original = (await readFile(path.join(reference, name), "utf8")).replace(/\r\n/g, "\n");
                const generated = valid.tables[index].replace(/\r\n/g, "\n");
                const actual = path.join(build, "logs", "browser-" + name);
                await writeFile(actual, generated, "utf8");
                // Record the file first so an output mismatch can be inspected.
                assert.ok(trimPrintPadding(generated) === trimPrintPadding(original),
                    `${actual} differs from the reference output`);
                outputs[name] = createHash("sha256").update(generated).digest("hex");
            }
            const bad = await runJob(browser, base, tool, true);
            assert.equal(bad.status, 1, bad.diagnostics);
            assert.match(bad.diagnostics, tool.diagnostic);
            rows.push({ slug: tool.slug, status: "passed", referenceDirectory: path.resolve(reference),
                wasmSha256: tool.wasmSha256, rulesSha256: tool.rulesSha256, outputs, invalidRuleRejected: true });
            console.log(`${tool.slug} passed`);
        }
        await writeFile(path.join(build, "generator-browser-results.json"), JSON.stringify(rows, null, 2) + "\n");
    } finally {
        server.close();
        if (browser) await browser.close();
    }
}

main().catch(error => { console.error(error); process.exitCode = 1; });

/* end of fb-corpus-lbasic-check.cjs */

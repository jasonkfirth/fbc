/*
    Project: FreeBASIC browser corpus
    File: fb-corpus-lbasic-frontend-check.cjs
    Purpose: Run L-BASIC's parse-and-dump stage in Chromium.
    Responsibilities: Mount the test cases and check the compiler's AST
    dumps and diagnostics. The Wasm program owns parsing and dump output.
*/

"use strict";

const assert = require("node:assert/strict");
const { readFile, writeFile } = require("node:fs/promises");
const { createServer } = require("node:http");
const path = require("node:path");
const { chromium } = require("playwright-core");

const types = { "index.html": "text/html", "index.js": "text/javascript", "index.wasm": "application/wasm" };

async function upstreamCase(stage, file, title) {
    const text = (await readFile(path.join(stage, "L-BASIC", "tests", file), "utf8")).replace(/\r\n/g, "\n");
    const start = "$title: " + title + "\n";
    const offset = text.indexOf(start);
    assert.ok(offset >= 0, title);
    const end = text.indexOf("\n$expect:", offset);
    assert.ok(end > offset, title);
    return text.slice(offset + start.length, end + 1);
}

function addJob(shell, source) {
    const launch = /<script async type="text\/javascript" src="index\.js"><\/script>/;
    assert.match(shell, launch);
    return shell.replace(launch, tag => `<script>
let diagnostic = "";
Module.arguments = ["-t", "--no-core", "-e", "parse", "-o", "/sample.parse", "/sample.bas"];
Module.preRun.push(() => {
    __fb_rtlib.console.write = text => { diagnostic += text; };
    Module.FS.writeFile("/sample.bas", new TextEncoder().encode(${JSON.stringify(source).replace(/</g, "\\u003c")}));
});
Module.onExit = status => {
    const output = status === 0 ? Module.FS.readFile("/sample.parse", {encoding:"utf8"}) : "";
    window.compilerResult = { status, diagnostic, output };
};
</script>` + tag);
}

async function main() {
    assert.equal(process.argv.length, 4, "Usage: node fb-corpus-lbasic-frontend-check.cjs <build> <staged corpus>");
    const [, , build, stage] = process.argv;
    const [artifact] = JSON.parse(await readFile(path.join(build, "build-results.json"), "utf8"));
    assert.equal(artifact.slug, "lbasic-frontend");
    assert.equal(artifact.status, "linked");
    const app = path.join(build, "apps", "lbasic-frontend");
    const shell = await readFile(path.join(app, "index.html"), "utf8");
    const cases = new Map([
        ["arithmetic", { source: "DIM amount AS LONG\namount = 6 * 7\nPRINT amount\n", status: 0,
            pattern: /Program:[\s\S]*AMOUNT[\s\S]*call\(\*[\s\S]*6, 7/i }],
        ["for", { source: await upstreamCase(stage, "for.test", "ascending counter"), status: 0,
            pattern: /Program:[\s\S]*FOR[\s\S]*NEXT/i }],
        ["udt", { source: await upstreamCase(stage, "udt.test", "simple udt"), status: 0,
            pattern: /Program:[\s\S]*udt\(.*T\.B\) = "hello"/i }],
        ["invalid", { source: "DIM amount AS LONG\namount = (\n", status: 1, pattern: /Parser: Line2: Unexpected/i }],
    ]);
    const server = createServer(async (request, response) => {
        const { pathname, searchParams } = new URL(request.url, "http://127.0.0.1");
        const leaf = pathname.slice(1);
        if (!Object.hasOwn(types, leaf)) return response.writeHead(404).end();
        const job = cases.get(searchParams.get("case"));
        try {
            const bytes = leaf === "index.html" && job ? addJob(shell, job.source) : await readFile(path.join(app, leaf));
            response.writeHead(200, { "Content-Type": types[leaf] }).end(bytes);
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
        for (const [name, job] of cases) {
            const page = await browser.newPage();
            const errors = [];
            page.on("pageerror", error => errors.push(error.message));
            try {
                await page.goto(`http://127.0.0.1:${server.address().port}/index.html?case=${name}`);
                await page.waitForFunction(() => window.compilerResult, undefined, { timeout: 15000 });
                const result = await page.evaluate(() => window.compilerResult);
                rows.push({ case: name, ...result, errors });
                assert.equal(result.status, job.status, result.diagnostic);
                assert.deepEqual(errors, []);
                assert.match(job.status === 0 ? result.output : result.diagnostic, job.pattern);
                console.log(`${name} passed`);
            } catch (error) {
                throw new Error(`${name}: ${errors.join("\n") || error.message}`, { cause: error });
            } finally {
                await page.close();
            }
        }
        await writeFile(path.join(build, "browser-results.json"), JSON.stringify([
            { slug: artifact.slug, artifacts: artifact.artifacts, status: "passed", cases: [...cases.keys()] },
        ], null, 2) + "\n");
    } finally {
        try {
            await writeFile(path.join(build, "frontend-browser-results.json"), JSON.stringify(rows, null, 2) + "\n");
        } finally {
            server.close();
            if (browser) await browser.close();
        }
    }
}

main().catch(error => { console.error(error); process.exitCode = 1; });

/* end of fb-corpus-lbasic-frontend-check.cjs */

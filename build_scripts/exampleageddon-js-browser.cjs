/*
    Project: FreeBASIC Exampleageddon
    File: exampleageddon-js-browser.cjs
    Purpose: Run a single generated Emscripten page in Chromium.
    Responsibilities: Serve local artifacts and report the program exit status.
    This file intentionally does not compile or classify examples.
*/

"use strict";

const { readFile } = require("node:fs/promises");
const { createServer } = require("node:http");
const path = require("node:path");
const { chromium } = require("playwright-core");

const htmlPath = path.resolve(process.argv[2]);
const htmlLeaf = path.basename(htmlPath);
const extensions = { ".html": "text/html", ".js": "text/javascript", ".wasm": "application/wasm" };
const hook = `<script>
    Module.arguments = ${JSON.stringify(process.argv.slice(3)).replace(/</g, "\\u003c")};
    Module.onExit = function(status) { window.exampleExit = status; };
    Module.preRun.push(function() {
        var terminalWrite = __fb_rtlib.console.write;
        __fb_rtlib.console.write = function(text) {
            console.log(text);
            return terminalWrite.call(this, text);
        };
    });
</script>`;

async function serve(request, response) {
    const leaf = decodeURIComponent(request.url.slice(1));
    if (leaf === "favicon.ico") {
        response.writeHead(204).end();
        return;
    }
    const extension = path.extname(leaf);
    // Only serve this program's generated files. Emscripten fetches its
    // WebAssembly and any preloaded asset bundle relative to the HTML.
    const stem = path.basename(htmlPath, ".html");
    if (leaf !== path.basename(leaf) || ![htmlLeaf, stem + ".js", stem + ".wasm", stem + ".data"].includes(leaf)) {
        response.writeHead(404).end();
        return;
    }
    let body = await readFile(path.join(path.dirname(htmlPath), leaf));
    if (leaf === htmlLeaf) {
        const html = body.toString("utf8");
        const script = /<script async type="text\/javascript" src="[^"]+"><\/script>/;
        if (!script.test(html)) throw new Error("FreeBASIC HTML shell launch tag is missing");
        body = html.replace(script, tag => hook + tag);
    }
    response.writeHead(200, { "Content-Type": extensions[extension] || "application/octet-stream" }).end(body);
}

async function main() {
    const server = createServer((request, response) => {
        serve(request, response).catch(error => {
            console.error(error);
            response.writeHead(500).end();
        });
    });
    await new Promise((resolve, reject) => {
        server.once("error", reject);
        server.listen(0, "127.0.0.1", resolve);
    });
    let browser;
    try {
        browser = await chromium.launch({
            executablePath: process.env.FBC_BROWSER,
            args: ["--use-angle=swiftshader", "--enable-unsafe-swiftshader",
                "--autoplay-policy=no-user-gesture-required"],
        });
        const page = await browser.newPage();
        const errors = [];
        page.on("console", event => console.log(event.text()));
        page.on("pageerror", error => errors.push(error.message));
        await page.goto("http://127.0.0.1:" + server.address().port + "/" + encodeURIComponent(htmlLeaf));
        try {
            await page.waitForFunction(() => Number.isInteger(window.exampleExit), { }, { timeout: 12000 });
        } catch (error) {
            if (errors.length) throw new Error(errors.join("\n"), { cause: error });
            console.error(await page.evaluate(() => ({
                status: document.getElementById("status")?.textContent,
                canvas: [Module.canvas.width, Module.canvas.height],
                arguments: Module.arguments,
            })));
            throw error;
        }
        const status = await page.evaluate(() => window.exampleExit);
        if (errors.length) throw new Error(errors.join("\n"));
        if (status !== 0) throw new Error("Browser program exited with status " + status);
        console.log("EXAMPLEAGEDDON_BROWSER_EXIT=0");
    } finally {
        if (browser) await browser.close();
        server.close();
    }
}

main().catch(error => { console.error(error); process.exitCode = 1; });

/* end of exampleageddon-js-browser.cjs */

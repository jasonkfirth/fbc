/*
    Project: FreeBASIC browser corpus
    File: fb-corpus-fbvnc-check.cjs

    Purpose:
        Verify the original fb-VNC viewer against a loopback RFB desktop.

    Responsibilities:
        Serve the built player, exercise binary WebSocket framing, and
        verify decoded framebuffer pixels and returned keyboard and pointer
        messages. The test server runs only while this check is active.
*/

"use strict";

const assert = require("node:assert/strict");
const { createHash } = require("node:crypto");
const { readFile, writeFile } = require("node:fs/promises");
const { createServer } = require("node:http");
const path = require("node:path");
const { chromium } = require("playwright-core");

// Reuse the WebSocket server bundled with the same Playwright installation.
const playwrightRoot = path.dirname(require.resolve("playwright-core/package.json"));
const { wsServer: WebSocketServer } = require(path.join(playwrightRoot, "lib/utilsBundle.js"));
const VERSION = Buffer.from("RFB 003.008\n");
const WIDTH = 320;
const HEIGHT = 200;
const DIALOG = 0x37414b;
const INITIAL = 0x285396;
const CHANGED = 0x3f8e64;
const FORMATS = { "/index.html": "text/html", "/index.js": "text/javascript", "/index.wasm": "application/wasm" };

function serverInit() {
    const name = Buffer.from("fb-VNC loopback desktop");
    const message = Buffer.alloc(24 + name.length);
    message.writeUInt16BE(WIDTH, 0);
    message.writeUInt16BE(HEIGHT, 2);
    // RFB 32-bit little-endian true color, with eight bits per channel.
    Buffer.from([32, 24, 0, 1, 0, 255, 0, 255, 0, 255, 16, 8, 0]).copy(message, 4);
    message.writeUInt32BE(name.length, 20);
    name.copy(message, 24);
    return message;
}

function framebuffer(color) {
    // One Raw rectangle at the desktop origin. RFB fields are big-endian,
    // while the negotiated pixel format sends blue, green, red, then padding.
    const message = Buffer.alloc(16 + WIDTH * HEIGHT * 4);
    message.writeUInt16BE(1, 2);
    message.writeUInt16BE(WIDTH, 8);
    message.writeUInt16BE(HEIGHT, 10);
    for (let offset = 16; offset < message.length; offset += 4) message.writeUInt32LE(color, offset);
    return message;
}

function serveDesktop(socket, evidence, errors) {
    let pending = Buffer.alloc(0);
    let phase = 0;
    const sendSplit = message => {
        // Separate WebSocket messages can split RFB fields and pixel data.
        socket.send(message.subarray(0, 3));
        socket.send(message.subarray(3));
    };
    sendSplit(VERSION);
    socket.on("message", (data, binary) => {
        try {
            assert.equal(binary, true);
            pending = Buffer.concat([pending, data]);
            while (pending.length) {
                if (phase === 0) {
                    if (pending.length < VERSION.length) return;
                    assert.deepEqual(pending.subarray(0, VERSION.length), VERSION);
                    pending = pending.subarray(VERSION.length);
                    evidence.version = VERSION.toString().trim();
                    socket.send(Buffer.from([1, 1]));
                    phase = 1;
                } else if (phase === 1) {
                    assert.equal(pending[0], 1);
                    pending = pending.subarray(1);
                    socket.send(Buffer.alloc(4));
                    phase = 2;
                } else if (phase === 2) {
                    assert.equal(pending[0], 1);
                    pending = pending.subarray(1);
                    sendSplit(serverInit());
                    phase = 3;
                } else {
                    const type = pending[0];
                    const lengths = { 0: 20, 3: 10, 4: 8, 5: 6 };
                    if (type === 2 && pending.length < 4) return;
                    const size = type === 2 ? 4 + pending.readUInt16BE(2) * 4 : lengths[type];
                    assert.ok(size, "unexpected RFB client message " + type);
                    if (pending.length < size) return;
                    const message = pending.subarray(0, size);
                    pending = pending.subarray(size);
                    if (type === 0) {
                        assert.deepEqual([...message.subarray(4, 8)], [32, 24, 0, 1]);
                        evidence.pixelFormat = true;
                    } else if (type === 2) {
                        evidence.rawEncoding = Array.from({ length: (size - 4) / 4 },
                            (_, index) => message.readInt32BE(4 + index * 4)).includes(0);
                        assert.ok(evidence.rawEncoding, "viewer did not request Raw encoding");
                    } else if (type === 3 && !evidence.firstFrame) {
                        assert.equal(message[1], 0);
                        evidence.firstFrame = true;
                        sendSplit(framebuffer(INITIAL));
                    } else if (type === 4 && message[1] === 1) {
                        evidence.key = message.readUInt32BE(4);
                    } else if (type === 5 && (message[1] & 1)) {
                        evidence.pointer = [message.readUInt16BE(2), message.readUInt16BE(4)];
                    }
                    if (evidence.key === 97 && evidence.pointer && !evidence.changedFrame) {
                        evidence.changedFrame = true;
                        sendSplit(framebuffer(CHANGED));
                    }
                }
            }
        } catch (error) {
            errors.push(error.message);
            socket.close();
        }
    });
    socket.on("error", error => errors.push(error.message));
}

async function waitForColor(page, canvas, color) {
    const deadline = Date.now() + 15000;
    do {
        const screenshot = await canvas.screenshot({ timeout: 4500 });
        const targetPixels = await page.evaluate(async ({ image, color }) => {
            const png = new Image();
            png.src = "data:image/png;base64," + image;
            await png.decode();
            const buffer = document.createElement("canvas");
            buffer.width = png.width;
            buffer.height = png.height;
            const drawing = buffer.getContext("2d");
            drawing.drawImage(png, 0, 0);
            const pixels = drawing.getImageData(0, 0, png.width, png.height).data;
            let count = 0;
            for (let index = 0; index < pixels.length; index += 4) {
                if (pixels[index] * 65536 + pixels[index + 1] * 256 + pixels[index + 2] === color) ++count;
            }
            return count;
        }, { image: screenshot.toString("base64"), color });
        if (targetPixels >= 1000) return { targetPixels, screenshot };
        await page.waitForTimeout(250);
    } while (Date.now() < deadline);
    throw new Error("The viewer did not display the framebuffer color " + color.toString(16));
}

async function main() {
    const root = path.resolve(process.argv[2]);
    const report = path.resolve(process.argv[3]);
    const slug = "fb-vnc-browser";
    const builds = JSON.parse(await readFile(path.resolve(root, "../../build-results.json"), "utf8"));
    const build = builds.find(row => row.slug === slug);
    assert.equal(build?.status, "linked");
    const evidence = { slug, artifacts: {}, status: "failed", errors: [], connections: [] };
    const payloads = new Map();
    for (const route of Object.keys(FORMATS)) {
        const name = route.slice(1);
        const body = await readFile(path.join(root, name));
        const hash = createHash("sha256").update(body).digest("hex");
        assert.equal(hash, build.artifacts[name], name);
        evidence.artifacts[name] = hash;
        payloads.set(route, body);
    }
    const server = createServer((request, response) => {
        const route = request.url;
        if (!Object.hasOwn(FORMATS, route)) return response.writeHead(404).end();
        try {
            let body = payloads.get(route);
            if (route === "/index.html") {
                const launch = /<script async type="text\/javascript" src="[^"]+"><\/script>/;
                assert.match(body.toString(), launch);
                const args = JSON.stringify(["--raw", "--hide-toolbar"]);
                body = body.toString().replace(launch, tag => `<script>Module.arguments = ${args};</script>` + tag);
            }
            response.writeHead(200, { "Content-Type": FORMATS[route] }).end(body);
        } catch (error) {
            evidence.errors.push(error.message);
            response.writeHead(500).end();
        }
    });
    const sockets = new WebSocketServer({ server, path: "/desk", maxPayload: 1024 * 1024 });
    let activeSocket;
    sockets.on("connection", socket => {
        const connection = {};
        activeSocket = socket;
        evidence.connections.push(connection);
        serveDesktop(socket, connection, evidence.errors);
    });
    await new Promise(resolve => server.listen(0, "127.0.0.1", resolve));
    let browser;
    try {
        browser = await chromium.launch({ executablePath: process.env.FBC_BROWSER });
        const page = await browser.newPage({ viewport: { width: 1240, height: 1040 } });
        page.on("pageerror", error => evidence.errors.push(error.message));
        await page.goto(`http://127.0.0.1:${server.address().port}/index.html`);
        const canvas = page.locator("#canvas");
        await waitForColor(page, canvas, DIALOG);
        await page.keyboard.type(`ws://127.0.0.1:${server.address().port}/desk`, { delay: 35 });
        await page.keyboard.press("Enter", { delay: 120 });
        evidence.initialPixels = (await waitForColor(page, canvas, INITIAL)).targetPixels;
        const box = await canvas.boundingBox();
        assert.ok(box && box.width >= WIDTH && box.height >= HEIGHT);
        await page.mouse.move(box.x + box.width / 2, box.y + box.height / 2);
        await page.mouse.down();
        await page.waitForTimeout(120);
        await page.mouse.up();
        await page.keyboard.press("a", { delay: 120 });
        const changed = await waitForColor(page, canvas, CHANGED);
        evidence.changedPixels = changed.targetPixels;
        const [x, y] = evidence.connections[0].pointer;
        assert.ok(x > 0 && x < WIDTH);
        assert.ok(y > 0 && y < HEIGHT);
        activeSocket.close(1000);
        await waitForColor(page, canvas, DIALOG);
        await page.keyboard.press("Enter", { delay: 120 });
        evidence.reconnectPixels = (await waitForColor(page, canvas, INITIAL)).targetPixels;
        assert.equal(evidence.connections.length, 2);
        assert.equal(evidence.connections[1].firstFrame, true);
        assert.deepEqual(evidence.errors, []);
        await writeFile(report.replace(/\.json$/, ".png"), changed.screenshot);
        evidence.status = "passed";
    } catch (error) {
        evidence.errors.push(error.message);
    } finally {
        await browser?.close();
        for (const socket of sockets.clients) socket.terminate();
        sockets.close();
        server.close();
    }
    await writeFile(report, JSON.stringify([evidence], null, 2) + "\n");
    console.log(JSON.stringify(evidence, null, 2));
    if (evidence.status !== "passed") process.exitCode = 1;
}

main().catch(error => { console.error(error); process.exitCode = 1; });

/* end of fb-corpus-fbvnc-check.cjs */

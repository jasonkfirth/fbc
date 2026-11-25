/*
    Project: FreeBASIC browser corpus
    File: fb-corpus-js-check.cjs
    Purpose: Exercise compiled corpus programs in Chromium.
    Responsibilities: Serve verified build artifacts, drive program input,
    and record results tied to the build's hashes.
    This file intentionally contains no compiler or publishing logic.
*/

"use strict";

const { createHash } = require("node:crypto");
const { mkdir, readFile, writeFile } = require("node:fs/promises");
const { createServer } = require("node:http");
const path = require("node:path");
const { chromium } = require("playwright-core");
const sampleMidi = require("./fb-corpus-opensesh-sample.js");

const formats = { ".html": "text/html", ".js": "text/javascript", ".wasm": "application/wasm",
    ".data": "application/octet-stream" };
const checks = {
    fgconvert: { converter: true, args: ["1"], answers: ["1000"], output: [/3280\.84 feet/] },
    fblint: { lintTool: true, args: ["--target", "js", "--profile", "style", "--no-semantic", "/banner.bas"],
        output: [/FBL031.*Section banner/, /SUMMARY files=\s*1 errors=\s*0 warnings=\s*1/] },
    "station922-static": { station922: true, outputFile: "/response.http",
        output: [/HTTP\/1\.1 200 OK/, /Content-Type: text\/html/, /Station922 browser check/] },
    "brpg-map": { brpgMap: true, output: [/Converted\s+24 x\s+16 map\./] },
    "visualfb-align": { visualfbAlign: true, output: [/Aligned 2 lines\./] },
    "freebasic-compiler": { compilerTool: true, args: ["-version"], output: [/FreeBASIC Compiler - Version/i] },
    "freebasic-ng-compiler": { compilerTool: true, args: ["-version"], output: [/FreeBASIC Compiler - Version/i] },
    "lbasic-frontend": { compilerTool: true },
    "azdecrypt-substitution": { azdecrypt: true, outputFile: "/solution.txt", args: ["3", "31"],
        output: [/The old observatory stood at the end of a narrow road/] },
    araxia: { canvas: true, araxiaGame: true, startupDelay: 2200 },
    "ohrrpgce-game": { canvas: true, standaloneShell: true, viewport: { width: 960, height: 720 } },
    "pyromax-dax": { canvas: true, pyromaxGame: true },
    "pyromax-workshop": { canvas: true, pyromaxGame: true, pyromaxWorkshop: true },
    openmidisesh: { canvas: true, midiEditor: true, viewport: { width: 1340, height: 920 } },
    chessbrain: { canvas: true, chessMove: true, startupDelay: 1800 },
    imageviewer: { canvas: true, args: ["/samples/sample.bmp"], sampleImage: "rotate", startupDelay: 1500 },
    slideshow: { canvas: true, args: ["/samples/sample.bmp"], sampleImage: "slideshow", startupDelay: 900 },
    nebunoid: { canvas: true, nebunoidGame: true, startupDelay: 1200, viewport: { width: 1152, height: 1024 } },
    "tilengine-learning": { canvas: true, animationDelay: 650 },
    "tilengine-mode7": { canvas: true, mode7: true },
    guess: { answers: Array(8).fill("0"), output: [/Too Small/, /Game Over/] },
    rps: { answers: Array(30).fill("r"), output: [/Player: Rock/i, /Computer:/, /(?:WINS GAME|GAME TIED)/] },
    tictactoe: { answers: "123456789".split(""), output: [/Tic-Tac-Toe/, /Wins!|DRAW!/] },
    asylum: { answers: ["g", "c", "x"], output: [/Dirt Path:/, /Foyer:/, /Hallway:/] },
    fbjson: { output: [/"Name"\s*:\s*"fbJson"/i, /"array"\s*:\s*\[\s*1,\s*2,/, /,\s*10\s*\]/] },
    "fbjson-tests": { output: [/(?:\[OK\][\s\S]*?){23}/] },
    toybasic: { toybasicTool: true, args: ["/code.toy"], key: "Enter", output: [/Result Main:\s*11\b/, /END PARSING/] },
    asteroids: { canvas: true, key: "Space", awaitColor: 0xffffff },
    img2ascii: { asciiBitmap: true, args: ["/samples/sample.bmp"], sampleImage: "img2ascii" },
    "kiwi-url": { kiwiTool: true, args: ["https://birdwatch.example/yard/sightings?season=fall"],
        output: [/^Host:birdwatch\.example\s*Path:\/yard\/sightings\?season=fall\s*Protocol:https\s*$/] },
    "opensesh-mod": { midiTool: true, outputFile: "/conversion.txt",
        output: [/OPENSESH_MOD_EXPORTED/, /Tracks:\s*1/, /Notes written:\s*4/] },
    lijstjes: { canvas: true, key: "F1" },
    "lin-regressor": { canvas: true, linearRegression: true, viewport: { width: 900, height: 1220 } },
    catfish: { canvas: true, paddle: 0x828282 },
    "raylib-window": { canvas: true, drawingColor: 0xc8c8c8 },
    "raylib-camera": { canvas: true, key: "w", hold: 650 },
    rekenblad: { canvas: true, keys: ["ArrowDown", "F2", "=", "6", "Shift+Digit8", "7", "Enter"] },
    schrijf: { canvas: true, keys: ["b", "r", "o", "w", "s", "e", "r"] },
    tiko: { canvas: true, editSample: true, startupDelay: 2500 },
    ugomoku: { canvas: true, gomokuMove: true },
    strawberrynes: { canvas: true, nesController: true, startupDelay: 1400 },
    "fbsound-eq": { canvas: true, eqPlayback: true },
    "fbrt-format": { formatTool: true, output: [] },
    "opengl-triangle": { canvas: true, drawingColor: 0xff8033 },
    "griffon-legend": { canvas: true, griffonGame: true, startupDelay: 6800 },
    phoenix: { canvas: true, phoenixGame: true, startupDelay: 900, viewport: { width: 1360, height: 760 } },
    "imageproc-grayscale": { canvas: true, grayscale: true, imageprocTool: true },
    "basicverse-island": { canvas: true, key: "w", hold: 1000, directCanvas: true },
    "ext-list": { output: [/\(-5\).*\(-1\).*\(1\).*\(5\)/, /\(-5\)\(-3\)\(-1\)\(1\)\(3\)\(5\)/] },
    fbfrog: { bindings: true, args: ["/sample.h", "-emit", "*", "/sample.bi"], outputFile: "/sample.bi",
        output: [/#define\s+A\b/i, /\btype\s+UDT\b/i, /\bi\s+as\s+long\b/i, /\btype\s+myint\s+as\s+long\b/i] },
    darkcastle: { canvas: true, darkCastleGame: true, viewport: { width: 1100, height: 800 } },
    "darkcastle-editor": { canvas: true, darkCastleEditor: true, startupDelay: 1000 },
    prospector: { canvas: true, standaloneShell: true, prospectorGame: true, viewport: { width: 1040, height: 820 } },
    clumsier: { terminalEditor: true, viewport: { width: 960, height: 800 } },
    hackit: { terminalEditor: "hackit", viewport: { width: 1080, height: 900 } },
    cmdsqlite: { databaseFixture: "examples/database/sqlite3_test.db",
        args: ["sample.db", "select id, title from features where id <= 2 order by id", "csv"],
        output: [/^id,title\n1,BASIC Compatibility\n2,Clean Syntax\s*$/] },
    "oregon-trail": {
        answers: ["NO", "1", "250", "100", "100", "100", "100"],
        oregonHunt: true,
        stopAtNextPrompt: true,
        output: [/YOU NOW HAVE\s+50\s+DOLLARS LEFT/, /TOTAL MILEAGE IS\s+0/, /HUNT, OR \(2\) CONTINUE/,
            /FOOD\s+100\s*\nBULLETS\s+5000\s*\nCLOTHING\s+100\s*\nMISC\. SUPP\.\s+100\s*\nCASH\s+50/],
    },
};

function sampleBitmap(solidGreen = false, width = 80, height = 40) {
    // 24-bit BMP scanlines are padded to a four-byte boundary.
    const stride = Math.ceil(width * 3 / 4) * 4;
    // A 14-byte file header precedes the 40-byte BITMAPINFOHEADER.
    const pixelOffset = 54;
    const file = Buffer.alloc(pixelOffset + stride * height);
    file.write("BM");
    file.writeUInt32LE(file.length, 2);
    file.writeUInt32LE(pixelOffset, 10);
    file.writeUInt32LE(40, 14);
    file.writeUInt32LE(width, 18);
    file.writeUInt32LE(height, 22);
    file.writeUInt16LE(1, 26);
    file.writeUInt16LE(24, 28);
    for (let y = 0; y < height; ++y) {
        for (let x = 0; x < width; ++x) {
            const offset = pixelOffset + y * stride + x * 3;
            const channel = solidGreen ? 1 : (x < width / 2 ? 0 : 2);
            file[offset + channel] = 255;
        }
    }
    return file.toString("base64");
}

const hackitFixture = { file: "browser-check.bas", source: 'print "HACKIT browser check"\n' };

const azdecryptPlain = "The old observatory stood at the end of a narrow road. Every evening the keeper would climb " +
    "the stairs and open the tall windows before the sun went down. In the summer he could hear children playing " +
    "in the village below, but in winter the only sound was the wind coming across the fields. On a clear night " +
    "he would bring out his notebook and write down what he had seen. He drew the shape of the moon and marked " +
    "the brightest stars, then made a little sketch to show which way the clouds were moving. The notebook was " +
    "full of small drawings and careful observations. Years later the children found it in a wooden box behind " +
    "a shelf in the library. They carried it outside and looked up at the same sky.";

function encipherFixture(text, key = "QWERTYUIOPASDFGHJKLZXCVBNM") {
    return text.replace(/[A-Z]/gi, letter => {
        const replacement = key[letter.toUpperCase().charCodeAt(0) - 65];
        return letter === letter.toUpperCase() ? replacement : replacement.toLowerCase();
    });
}

function addHooks(html, { args, outputFile, sampleImage, database, midiTool, observeLevel, hackit, station922, azdecrypt } = {}) {
    const launch = /<script async type="text\/javascript" src="[^"]+"><\/script>/;
    if (!launch.test(html)) throw new Error("FreeBASIC HTML launch tag is missing");
    // The generated shell creates Module before loading the Wasm program.
    // preRun sees the terminal as soon as the compiled JS runtime loads.
    function captureTerminal() {
        const write = __fb_rtlib.console.write;
        __fb_rtlib.console.write = function(text) {
            console.log("FB:" + text);
            return write.call(this, text);
        };
    }
    function captureLevelLoads() {
        window.corpusLevels = [];
        const open = FS.open;
        FS.open = function(file, ...args) {
            const stream = open.call(this, file, ...args);
            if (typeof file === "string" && /\/maps\/level\d+\.txt$/.test(file)) {
                window.corpusLevels.push(file);
            }
            return stream;
        };
    }
    const argumentsTag = args ? "Module.arguments = " + JSON.stringify(args).replace(/</g, "\\u003c") + ";" : "";
    const readOutput = outputFile ? "if (status === 0) console.log('FB:' + Module.FS.readFile(" +
        JSON.stringify(outputFile).replace(/</g, "\\u003c") + ", { encoding: 'utf8' })); " : "";
    const sampleFile = (name, data) => "Module.FS.writeFile('/samples/" + name + "', Uint8Array.from(atob('" +
        data + "'), letter => letter.charCodeAt(0)));";
    const slideshowSetup = sampleImage === "slideshow" ? sampleFile("second.bmp", sampleBitmap(true)) +
        "Module.FS.mkdir('/conf'); Module.FS.writeFile('/conf/conf.ini', 'playtype = sequential\\ninterval = 1800\\n'); " : "";
    const mountSample = sampleImage ? "\nModule.preRun.push(() => { Module.FS.mkdir('/samples'); " +
        sampleFile("sample.bmp", sampleImage === "img2ascii" ? sampleBitmap(false, 83, 41) : sampleBitmap()) +
        slideshowSetup + " });" : "";
    const mountDatabase = database ? "\nModule.preRun.push(() => Module.FS.writeFile('/sample.db', " +
        "Uint8Array.from(atob('" + database.toString("base64") + "'), char => char.charCodeAt(0))));" : "";
    const mountMidi = midiTool ? "\nModule.preRun.push(() => Module.FS.writeFile('/input.mid', " +
        "Uint8Array.from(atob('" + Buffer.from(sampleMidi()).toString("base64") + "'), char => char.charCodeAt(0))));" : "";
    const levelHook = observeLevel ? "\nModule.preRun.push(" + captureLevelLoads.toString() + ");" : "";
    const hackitHook = hackit ? "\nModule.arguments = [" + JSON.stringify("/" + hackitFixture.file) +
        "]; Module.preRun.push(() => Module.FS.writeFile(Module.arguments[0], " +
        JSON.stringify(hackitFixture.source) + "));" : "";
    const stationConfig = Buffer.from("\uFEFF[preview.test]\r\nPhysicalDir=/site\r\n", "utf16le").toString("base64");
    const stationHook = station922 ? "\nModule.preRun.push(() => {" +
        "Module.FS.writeFile('/WebSites.ini', Uint8Array.from(atob('" + stationConfig + "'), c => c.charCodeAt(0)));" +
        "Module.FS.writeFile('/request.http', 'GET / HTTP/1.1\\r\\nHost: preview.test\\r\\n\\r\\n');" +
        "Module.FS.mkdir('/site'); Module.FS.writeFile('/site/index.html', '<h1>Station922 browser check</h1>'); });" : "";
    const cipherHook = azdecrypt ? "\nModule.preRun.push(() => Module.FS.writeFile('/cipher.txt', " +
        JSON.stringify(encipherFixture(azdecryptPlain)) + "));" : "";
    const hook = "<script>" + argumentsTag + mountSample + mountDatabase + mountMidi + levelHook +
        hackitHook + stationHook + cipherHook +
        "\nModule.onExit = status => { " + readOutput + "window.corpusExit = status; };" +
        "\nModule.preRun.push(" + captureTerminal.toString() + ");</script>";
    return html.replace(launch, tag => hook + tag);
}

async function serve(request, response, output, builds) {
    const url = new URL(request.url, "http://127.0.0.1");
    const parts = url.pathname.split("/");
    const [, slug, file] = parts;
    const type = formats[path.extname(file || "")];
    if (parts.length !== 3 || !builds.has(slug) || !type || !/^[\w.-]+$/.test(file) ||
        !Object.hasOwn(builds.get(slug).artifacts, file)) {
        response.writeHead(404).end();
        return;
    }
    let data = await readFile(path.join(output, "apps", slug, file));
    const hash = createHash("sha256").update(data).digest("hex");
    if (hash !== builds.get(slug).artifacts[file]) throw new Error("Stale artifact: " + request.url);
    const check = checks[slug];
    if (file === "index.html" && !check.standaloneShell) {
        const database = check.databaseFixture ? await readFile(path.resolve(__dirname, "..", check.databaseFixture)) : null;
        const args = check.asciiBitmap && url.searchParams.get("mode") === "monochrome" ? [...check.args, "b"] : check.args;
        data = addHooks(data.toString("utf8"), {
            args: args || [],
            outputFile: check.outputFile,
            midiTool: check.midiTool,
            sampleImage: check.sampleImage,
            database,
            observeLevel: check.pyromaxGame,
            hackit: check.terminalEditor === "hackit",
            station922: check.station922,
            azdecrypt: check.azdecrypt,
        });
    }
    response.writeHead(200, { "Content-Type": type }).end(data);
}

async function canvasFrame(page, target, samples = [], compareGrayscale = false, direct = false, squares = []) {
    // The island renders on a 2D canvas. Read its backing image directly
    // so screenshot tooling does not compete with its software renderer.
    const png = direct
        ? Buffer.from(await page.locator("#canvas").evaluate(canvas => canvas.toDataURL().split(",")[1]), "base64")
        : await page.locator("#canvas").screenshot({ timeout: 4000 });
    const pixels = await page.evaluate(async ({ base64, target, samples, compareGrayscale, squares }) => {
        const picture = new Image();
        picture.src = "data:image/png;base64," + base64;
        await picture.decode();
        const canvas = document.createElement("canvas");
        canvas.width = picture.width;
        canvas.height = picture.height;
        const context = canvas.getContext("2d");
        context.drawImage(picture, 0, 0);
        const { data } = context.getImageData(0, 0, canvas.width, canvas.height);
        const colors = new Set();
        const trackedColors = Array.isArray(target) ? new Set(target) : null;
        let targetPixels = 0;
        let targetX = 0;
        let targetY = 0;
        for (let i = 0; i < data.length; i += 4) {
            const color = data[i] * 65536 + data[i + 1] * 256 + data[i + 2];
            colors.add(color);
            if (color === target || trackedColors?.has(color)) {
                targetPixels++;
                const pixel = i / 4;
                targetX += pixel % canvas.width;
                targetY += Math.floor(pixel / canvas.width);
            }
        }
        const sampleColors = samples.map(([x, y]) => {
            const offset = 4 * (Math.floor(y * canvas.height) * canvas.width + Math.floor(x * canvas.width));
            return data[offset] * 65536 + data[offset + 1] * 256 + data[offset + 2];
        });
        const frame = { colors: colors.size, targetPixels, sampleColors };
        if (targetPixels) frame.targetCenter = [targetX / targetPixels, targetY / targetPixels];
        if (squares.length) {
            frame.occupied = squares.map(([x, y]) => {
                const px = Math.floor(x * canvas.width);
                const py = Math.floor(y * canvas.height);
                const size = Math.floor(canvas.width * 0.03);
                const data = context.getImageData(px - size / 2, py - size / 2, size, size).data;
                // Only sample the center of each square to exclude its edges.
                return data.some((value, index) => value !== data[index % 4]);
            });
        }
        if (compareGrayscale) {
            // The sample draws a 320 by 240 original at the upper left of
            // a 1280 by 600 canvas, followed by its luma conversion.
            const width = Math.floor(canvas.width / 4);
            const height = Math.floor(canvas.height * 0.4);
            const region = x => {
                const pixels = context.getImageData(x, 0, width, height).data;
                let chromatic = 0;
                const tones = new Set();
                for (let i = 0; i < pixels.length; i += 4) {
                    if (pixels[i] !== pixels[i + 1] || pixels[i] !== pixels[i + 2]) ++chromatic;
                    tones.add(pixels[i]);
                }
                return { chromatic, tones: tones.size };
            };
            frame.original = region(0);
            frame.luma = region(width);
        }
        return frame;
    }, { base64: png.toString("base64"), target, samples, compareGrayscale, squares });
    return { png, ...pixels, hash: createHash("sha256").update(png).digest("hex") };
}

async function checkChessMove(page, canvas, row) {
    const bounds = await canvas.boundingBox();
    // The original 800 x 600 game draws its board at (40, 78).
    // Each square is 56 pixels wide, and the black pieces start at the top.
    const square = (file, rank) => [(40 + (file + 0.5) * 56) / 800, (78 + (8.5 - rank) * 56) / 600];
    const from = square(4, 2);
    const to = square(4, 4);
    const home = [7, 8].flatMap(rank => Array.from({ length: 8 }, (_, file) => square(file, rank)));
    const squares = [from, to, ...home];
    // Read the software canvas directly while the engine is repainting.
    const frame = () => canvasFrame(page, null, [], false, true, squares);
    const moveMouse = ([x, y]) => page.mouse.move(bounds.x + bounds.width * x, bounds.y + bounds.height * y);
    const before = await frame();
    if (!before.occupied[0] || before.occupied[1] || before.occupied.slice(2).some(occupied => !occupied)) {
        throw new Error("Starting chess pieces were missing from their squares");
    }
    await moveMouse(from);
    await page.mouse.down();
    await page.waitForTimeout(100);
    await moveMouse(to);
    await page.waitForTimeout(100);
    await page.mouse.up();
    const deadline = Date.now() + 6000;
    do {
        const after = await frame();
        if (!after.occupied[0] && after.occupied[1] && after.occupied.slice(2).some(occupied => !occupied)) {
            row.canvas = { move: "e2e4", engineMoved: true };
            return after;
        }
        await page.waitForTimeout(180);
    } while (Date.now() < deadline);
    throw new Error("The white pawn move or the engine's reply did not appear on the board");
}

async function checkNebunoidGame(page, canvas, row) {
    const bounds = await canvas.boundingBox();
    async function choose(y) {
        await page.mouse.move(bounds.x + bounds.width * 0.45, bounds.y + bounds.height * y / 768);
        await page.mouse.down();
        await page.waitForTimeout(110);
        await page.mouse.up();
        await page.waitForTimeout(350);
    }
    const menu = await canvasFrame(page);
    await choose(110); // Play local game.
    await choose(328); // Introductory Training, first official campaign.
    await page.waitForTimeout(800);
    const game = await canvasFrame(page);
    if (game.colors < 10 || game.hash === menu.hash) throw new Error("The training campaign did not render");
    await page.keyboard.press("1", { delay: 140 });
    await page.waitForTimeout(300);
    // The paddle spans the row at y=727 on the original 768-pixel court.
    const paddleSamples = [[0.30, 0.947], [0.70, 0.947]];
    async function movePaddle(x) {
        await page.mouse.move(bounds.x + bounds.width * x, bounds.y + bounds.height * 0.75);
        await page.waitForTimeout(350);
        return canvasFrame(page, null, paddleSamples);
    }
    const left = await movePaddle(0.30);
    const right = await movePaddle(0.70);
    const isPaddle = color => (color >> 16) > 170 && ((color >> 8) & 255) > 170 && (color & 255) > 170;
    if (!isPaddle(left.sampleColors[0]) || isPaddle(left.sampleColors[1]) ||
        isPaddle(right.sampleColors[0]) || !isPaddle(right.sampleColors[1])) {
        throw new Error("The training paddle did not follow the pointer across the court");
    }
    await page.keyboard.press("Space", { delay: 140 });
    await page.waitForTimeout(500);
    const after = await canvasFrame(page);
    if (after.colors < 10 || after.hash === right.hash) throw new Error("The training campaign did not respond to Space");
    row.canvas = { openedCampaign: true, paddleFollowedPointer: true, changedAfterSpace: true };
    return after;
}

async function checkImage(page, mode, row) {
    const samples = [[0.4, 0.5], [0.6, 0.5], [0.5, 0.3], [0.5, 0.7]];
    const primary = color => {
        const channels = [color >> 16, (color >> 8) & 255, color & 255];
        return channels.filter(channel => channel > 60).length === 1 ? channels.findIndex(channel => channel > 90) : -1;
    };
    const takeFrame = () => canvasFrame(page, null, samples);
    const colors = frame => frame.sampleColors.map(primary);
    // Canvas allocation can precede the first decoded image, and the
    // slideshow can already be fading when a slow screenshot arrives.
    const initialDeadline = Date.now() + 5500;
    const initialSamples = new Set();
    let left, right;
    do {
        const first = await takeFrame();
        [left, right] = colors(first);
        if (left === 2 && right === 0) break;
        initialSamples.add(first.sampleColors.slice(0, 2).map(color => color.toString(16)).join("/"));
        await page.waitForTimeout(100);
    } while (Date.now() < initialDeadline);
    if (left !== 2 || right !== 0) {
        throw new Error("The viewer did not display the red and blue test bitmap (sampled colors: " +
            [...initialSamples].join(", ") + ")");
    }
    if (mode === "rotate") {
        await page.keyboard.press("r", { delay: 150 });
        await page.waitForTimeout(200);
        const after = await takeFrame();
        const [, , top, bottom] = colors(after);
        if (top !== right || bottom !== left) throw new Error("The viewer did not rotate the bitmap");
        row.canvas = { rotatedBitmap: true };
        return after;
    }
    const deadline = Date.now() + 8500;
    const seen = new Set();
    while (Date.now() < deadline) {
        await page.waitForTimeout(300);
        const after = await takeFrame();
        seen.add(after.sampleColors.slice(0, 2).map(color => color.toString(16).padStart(6, "0")).join("/"));
        if (colors(after).slice(0, 2).every(color => color === 1)) {
            row.canvas = { loadedSecondSlide: true };
            return after;
        }
    }
    throw new Error("The slideshow did not advance to the second bitmap (sampled colors: " + [...seen].join(", ") + ")");
}

async function checkOhrGame(page, canvas, row) {
    await canvas.click();
    await page.waitForTimeout(700);
    await page.keyboard.press("Enter", { delay: 120 });
    for (const key of ["a", "ArrowUp", "b", "ArrowLeft", "c", "Space"]) {
        await page.keyboard.press(key, { delay: 160 });
        await page.waitForTimeout(160);
    }
    await page.waitForTimeout(5400);
    const bounds = await canvas.boundingBox();
    // The test RPG's instruction is in the last two lines. Compare just that
    // band so sprites moving elsewhere cannot satisfy the input check.
    const band = { x: bounds.x, y: bounds.y + Math.floor(bounds.height * 0.9),
        width: bounds.width, height: Math.floor(bounds.height * 0.1) };
    const before = await page.screenshot({ clip: band });
    await page.keyboard.press("q", { delay: 160 });
    await page.waitForTimeout(260);
    const after = await page.screenshot({ clip: band });
    if (before.equals(after)) throw new Error("The test RPG did not respond to Q");
    row.canvas = { loadedGame: "Interactive Testing Game", newInstructionAfterQ: true };
    return canvasFrame(page);
}

async function checkAraxiaGame(page, row) {
    // Leave the loading screen and select New Game on the title menu.
    await page.keyboard.press("Enter", { delay: 120 });
    await page.waitForTimeout(220);
    await page.keyboard.press("Enter", { delay: 120 });
    await page.waitForTimeout(350);
    // The bundled intro has ten pages, with Enter advancing each page.
    for (let pageNumber = 1; pageNumber <= 10; ++pageNumber) {
        await page.keyboard.press("Enter", { delay: 120 });
        await page.waitForTimeout(100);
    }
    await page.waitForTimeout(450);
    const room = await canvasFrame(page);
    const bounds = await page.locator("#canvas").boundingBox();
    // The player marker moves on the minimap. Animations in the room do
    // not change this region when the player stays on the same tile.
    const clip = { x: bounds.x + Math.floor(bounds.width * 0.82), y: bounds.y,
        width: Math.floor(bounds.width * 0.18), height: Math.floor(bounds.height * 0.17) };
    const beforeMarker = await page.screenshot({ clip });
    await page.keyboard.press("ArrowLeft", { delay: 680 });
    const after = await canvasFrame(page);
    const afterMarker = await page.screenshot({ clip });
    if (room.colors < 35 || after.colors < 35 || beforeMarker.equals(afterMarker)) {
        throw new Error("Araxia did not render the starting room and respond to movement");
    }
    row.canvas = { playedIntro: true, movedInStartingRoom: true };
    return after;
}

async function checkLinearRegression(page, canvas, row) {
    const key = async value => {
        await page.keyboard.press(value, { delay: 65 });
        await page.waitForTimeout(200);
    };
    const line = async text => {
        await page.keyboard.type(text, { delay: 75 });
        await key("Enter");
        row.input.push(text);
    };
    await canvas.click({ position: { x: 12, y: 12 } });
    await key("Enter");
    await key("2");
    // Three collinear points give an exact slope of 2 and intercept of 1.
    for (const text of ["3", "0", "1", "1", "3", "2", "5"]) await line(text);
    await key("Space");
    await line("<def>");
    await key("Enter");
    await key("5");
    const plot = await canvasFrame(page, null, [], false, true);
    await key("s");
    const result = await page.evaluate(() => Module.FS.readFile("Output_points.dat", { encoding: "utf8" }));
    if (!/^Y = 2 \* X \+ 1$/m.test(result) || plot.colors < 5) {
        throw new Error("The plotted regression or saved equation was incorrect");
    }
    row.canvas = { colors: plot.colors, plotted: true };
    return plot;
}

async function checkTikoEditor(page, canvas, row) {
    const button = page.getByRole("button", { name: "Download saved example.bas" });
    async function downloadSample() {
        const [download] = await Promise.all([page.waitForEvent("download", { timeout: 7000 }), button.click()]);
        if (download.suggestedFilename() !== "example.bas") throw new Error("Incorrect suggested filename");
        return readFile(await download.path());
    }

    const original = await downloadSample();
    if (!original.toString().includes("sub LogInit")) throw new Error("Tiko did not load the bundled BASIC file");
    const marker = "' Browser save check";
    await canvas.click({ position: { x: 460, y: 140 } });
    await page.keyboard.press("Control+End", { delay: 100 });
    await page.keyboard.press("Enter", { delay: 70 });
    await page.keyboard.type(marker, { delay: 40 });
    // Tiko draws its own menu inside the canvas. Use File > Save so the
    // browser's Ctrl+S shortcut cannot intercept the save command.
    await canvas.click({ position: { x: 20, y: 14 }, delay: 100 });
    await page.waitForTimeout(250);
    await canvas.click({ position: { x: 60, y: 244 }, delay: 100 });
    await page.waitForTimeout(250);
    const saved = await downloadSample();
    if (!saved.toString().includes(marker) || saved.equals(original)) {
        throw new Error(`Tiko did not save the edit (${original.length} bytes before, ${saved.length} after)`);
    }

    await page.reload();
    const restored = await downloadSample();
    if (!restored.equals(saved)) throw new Error("Tiko's saved file did not survive reload");
    await page.keyboard.press("Control+End", { delay: 100 });
    const frame = await canvasFrame(page);
    if (frame.colors < 20) throw new Error("Tiko did not render the editor after reload");
    row.workspace = { savedBytes: saved.length, reloadMatched: true, download: "example.bas" };
    return frame;
}

async function checkUgomoku(page, row) {
    // The UBL 8-bit palette uses bright green for the computer's pieces.
    const computerColor = 0x55ff55;
    await page.keyboard.press("Space", { delay: 100 });
    await page.waitForTimeout(350);
    const before = await canvasFrame(page, computerColor);
    await page.keyboard.press("Enter", { delay: 100 });
    await page.waitForTimeout(600);
    const after = await canvasFrame(page, computerColor);
    if (after.targetPixels <= before.targetPixels) throw new Error("Ugomoku did not draw the computer's reply");
    row.canvas = { colors: after.colors, computerMove: true };
    return after;
}

async function checkNesController(page, row) {
    // The test cartridge writes palette indices $21 and $2A, which the
    // emulator expands to these RGB values over the NES picture area.
    const blue = 0x4c9aec;
    const green = 0x4cd020;
    const coloredFrame = async color => {
        const deadline = Date.now() + 6500;
        do {
            const frame = await canvasFrame(page, color, [], false, true);
            if (frame.targetPixels >= 40000) return frame;
            await page.waitForTimeout(200);
        } while (Date.now() < deadline);
        throw new Error(`NES picture did not reach palette color ${color.toString(16)}`);
    };
    const before = await coloredFrame(blue);
    await page.keyboard.down("ArrowRight");
    const pressed = await coloredFrame(green);
    await page.keyboard.up("ArrowRight");
    const after = await coloredFrame(blue);
    row.canvas = { bluePixels: before.targetPixels, greenPixels: pressed.targetPixels,
        restoredPixels: after.targetPixels, controllerResponded: true };
    return pressed;
}

function observeBrowserAudio() {
    window.corpusAudio = [];
    function observe(buffer) {
        if (!buffer || window.corpusAudio.length >= 64) return;
        const samples = buffer.getChannelData(0);
        let power = 0;
        for (const sample of samples) power += sample * sample;
        const rms = Math.sqrt(power / samples.length);
        if (rms > 0.001) window.corpusAudio.push(rms);
    }

    // SDL's Emscripten audio driver writes into a ScriptProcessor buffer.
    const property = Object.getOwnPropertyDescriptor(ScriptProcessorNode.prototype, "onaudioprocess");
    const open = AudioContext.prototype.createScriptProcessor;
    AudioContext.prototype.createScriptProcessor = function(...args) {
        const node = open.apply(this, args);
        Object.defineProperty(node, "onaudioprocess", {
            get: property.get,
            set(callback) {
                if (typeof callback !== "function") {
                    property.set.call(this, callback);
                    return;
                }
                property.set.call(this, event => {
                    callback.call(this, event);
                    observe(event.outputBuffer);
                });
            },
        });
        return node;
    };
}

async function checkEqualizer(page, row) {
    async function play(key) {
        await page.evaluate(() => { window.corpusAudio = []; });
        await page.keyboard.press(key, { delay: 100 });
        await page.waitForFunction(() => window.corpusAudio.length >= 4, null, { timeout: 6500 });
        // Wait through the one-second clip before selecting another mode.
        await page.waitForTimeout(1100);
        return page.evaluate(() => Math.max(...window.corpusAudio));
    }
    const original = await play("1");
    const boost = await play("2");
    const boostedFrame = await canvasFrame(page, 0x44d39b);
    const cut = await play("3");
    if (boost < original * 1.8 || cut > original * 0.9 || boostedFrame.targetPixels < 300) {
        throw new Error(`EQ waveform or playback mismatch (original ${original}, boost ${boost}, cut ${cut})`);
    }
    row.audio = { originalRms: original, boostRms: boost, cutRms: cut };
    return boostedFrame;
}

async function checkGriffonGame(page, row) {
    const frame = () => canvasFrame(page, null, [], false, true);
    async function trace(name) {
        if (!process.env.FBC_GRIFFON_TRACE) return;
        await writeFile(path.join(process.env.FBC_GRIFFON_TRACE, "griffon-" + name + ".png"), (await frame()).png);
    }
    async function press(key) {
        await page.keyboard.press(key, { delay: 100 });
        // The slot menu debounces input for 125 to 150 milliseconds.
        await page.waitForTimeout(225);
    }
    async function waitForMap() {
        // The opening town draws a blue fountain beside a dark stone tile.
        // The title, story, and slot menu do not contain this pair of pixels.
        await page.waitForFunction(() => {
            const context = document.getElementById("canvas")?.getContext("2d");
            if (!context) return false;
            const blue = context.getImageData(160, 115, 1, 1).data;
            const stone = context.getImageData(142, 126, 1, 1).data;
            return blue[2] > 125 && blue[2] - blue[0] > 65 && stone[2] < 65;
        }, null, { timeout: 14000 });
    }
    const readSlot = slot => page.evaluate(index =>
        Module.FS.readFile("/saves/player" + index + ".sav", { encoding: "utf8" }), slot);
    async function saveAt(slot) {
        await page.waitForTimeout(250);
        await press("Escape");
        await trace(slot + "-pause");
        await press("Enter");
        await trace(slot + "-slots");
        await press("ArrowRight");
        await trace(slot + "-save-selected");
        await press("Enter");
        await trace(slot + "-slot-selected");
        if (slot === 1) await press("ArrowDown");
        await press("Enter");
        await page.waitForFunction(index =>
            Module.FS.stat("/saves/player" + index + ".sav").size > 1, slot, { timeout: 8000 });
        return readSlot(slot);
    }
    function position(save) {
        // The original save stores the player level, time, and a marker
        // before the two coordinates, each on its own line.
        const [level, , marker, x, y] = save.trim().split(/\r?\n/);
        if (level.trim() !== "1" || marker.trim() !== "a" ||
            !Number.isFinite(Number(x)) || !Number.isFinite(Number(y))) {
            throw new Error("Griffon did not write a valid starting save");
        }
        return [Number(x), Number(y)];
    }

    await press("Enter");
    await press("Enter");
    await page.waitForTimeout(800);
    await press("Escape");
    await waitForMap();
    await page.keyboard.press("ArrowLeft", { delay: 600 });
    const saved = await saveAt(0);
    const firstPosition = position(saved);
    row.griffon = { savedPosition: firstPosition };
    // New Game starts at x=236, y=92. The left key must move the actor,
    // independently of changes to clouds or other animated scenery.
    if (firstPosition[0] >= 228 || Math.abs(firstPosition[1] - 92) > 1) {
        throw new Error("The save does not record player movement");
    }

    await page.reload({ waitUntil: "domcontentloaded" });
    await page.waitForFunction(() => typeof Module?.FS?.stat === "function" &&
        Module.FS.analyzePath("/saves/player0.sav").exists, null, { timeout: 14000 });
    if (await readSlot(0) !== saved) throw new Error("The saved slot was lost on reload");
    // Allow the opening logos to finish before choosing Load at the title.
    await page.waitForTimeout(6800);
    await press("Enter");
    await press("ArrowRight");
    await press("ArrowRight");
    await press("Enter");
    await press("Enter");
    await waitForMap();
    const after = await frame();
    const restoredPosition = position(await saveAt(1));
    if (!restoredPosition.every((value, axis) => Math.abs(value - firstPosition[axis]) <= 1)) {
        throw new Error("Loading the slot did not restore the saved position");
    }
    row.griffon = { savedPosition: firstPosition, restoredPosition, colors: after.colors };
    return after;
}

async function checkPhoenixGame(page, canvas, row) {
    // Both intro panels draw a blue emblem near the middle. The player
    // spawns in the first 13 of 75 tile columns after the two panels.
    const playerBlue = 0x0080ff;
    const spawnLimit = await canvas.evaluate(element => element.width * 13 / 75);
    const trace = process.env.FBC_PHOENIX_TRACE;
    const snapshot = async label => {
        const frame = await canvasFrame(page, playerBlue, [], false, true);
        if (trace) await writeFile(path.join(trace, "phoenix-" + label + ".png"), frame.png);
        return frame;
    };
    const titleDeadline = Date.now() + 12000;
    let frame = await snapshot("title");
    while (frame.targetPixels < 20 && Date.now() < titleDeadline) {
        await page.waitForTimeout(150);
        frame = await snapshot("title");
    }
    if (frame.targetPixels < 20) throw new Error("Phoenix did not display its intro");
    await canvas.click();
    let presses = 0;
    // On a slow renderer a new room can take several frames before the next
    // prompt responds. Wait for an actual dungeon spawn before moving.
    while (presses < 6 && (frame.targetPixels < 20 || frame.targetCenter[0] >= spawnLimit)) {
        await page.keyboard.press("Space", { delay: 150 });
        await page.waitForTimeout(550);
        frame = await snapshot("start-" + ++presses);
    }
    const before = frame;
    if (before.targetPixels < 20 || before.targetCenter[0] >= spawnLimit) {
        throw new Error("Phoenix did not draw the player in the dungeon");
    }
    row.phoenix = { start: before.targetCenter, startPixels: before.targetPixels, presses, attempts: [] };
    for (const key of ["ArrowRight", "ArrowDown", "ArrowLeft", "ArrowUp"]) {
        await page.keyboard.press(key, { delay: 550 });
        await page.waitForTimeout(120);
        const moved = await snapshot(key);
        row.phoenix.attempts.push({ key, center: moved.targetCenter, pixels: moved.targetPixels });
        if (moved.targetCenter && moved.targetCenter.some((position, axis) =>
            Math.abs(position - before.targetCenter[axis]) >= 8)) {
            const audio = await page.evaluate(() => window.corpusAudio.length);
            if (!audio) throw new Error("Phoenix did not produce an audible Web Audio buffer");
            row.phoenix = { start: before.targetCenter, afterKey: moved.targetCenter, key, presses, audioBuffers: audio };
            return moved;
        }
    }
    throw new Error("None of the movement keys moved the player sprite");
}

async function checkPyromaxGame(page, canvas, row, workshop) {
    await canvas.click();
    // Enter skips the logos and story. The Play command opens the first
    // map. Stop sending Enter there since it pauses the live game.
    const firstLevel = "/maps/level01.txt";
    const opened = level => page.evaluate(name =>
        window.corpusLevels?.some(file => file.endsWith(name)), level);
    const menuDeadline = Date.now() + 55000;
    while (!await opened(firstLevel) && Date.now() < menuDeadline) {
        await page.keyboard.press("Enter", { delay: 160 });
        await page.waitForTimeout(700);
    }
    if (!await opened(firstLevel)) throw new Error("Play did not open Pyromax's first map");
    row.pyromax = { level: firstLevel };

    // Each atlas has a player color absent from its HUD and scenery.
    const playerColors = pyromaxPlayerColors(workshop ? 0xff4a70 : 0xf80868);
    const snapshot = () => canvasFrame(page, playerColors, [], false, true);
    const levelDeadline = Date.now() + 24000;
    let before = await snapshot();
    while (before.targetPixels < 16 && Date.now() < levelDeadline) {
        await page.waitForTimeout(500);
        before = await snapshot();
    }
    if (before.targetPixels < 16) throw new Error("The first map loaded without drawing the player");
    row.pyromax.playerBefore = before.targetCenter;
    // Allow the opening countdown to finish. Space dismisses the first
    // dialog if the player reaches its trigger while moving.
    for (let attempt = 0; attempt < 10; ++attempt) {
        const after = await movePyromaxRight(page, snapshot, before.targetCenter[0] + 20);
        if (after) {
            row.pyromax.playerAfterRight = after.targetCenter;
            if (workshop) return checkPyromaxCourses(page, row.pyromax, after, snapshot, opened);
            return after;
        }
        if (attempt === 4) await page.keyboard.press("Space", { delay: 150 });
        await page.waitForTimeout(350);
    }
    throw new Error("Right arrow did not move the player in the first map");
}

function pyromaxPlayerColors(color) {
    const colors = [color];
    const redChannel = color >> 16;
    const greenChannel = (color >> 8) & 255;
    const blueChannel = color & 255;
    // Player.Draw multiplies RGB by (1 - c, c, c) while the player is
    // invincible. Retain both adjacent byte values for framebuffer rounding.
    for (let red = 0; red <= redChannel; ++red) {
        const tint = 1 - red / redChannel;
        const green = greenChannel * tint;
        const blue = blueChannel * tint;
        for (const g of [Math.floor(green), Math.ceil(green)]) {
            for (const b of [Math.floor(blue), Math.ceil(blue)]) {
                colors.push((red << 16) | (g << 8) | b);
            }
        }
    }
    return colors;
}

async function movePyromaxRight(page, snapshot, destination) {
    // The tracking color varies across animation frames. Read several frames
    // while the key is down, including frames before the player stops.
    await page.keyboard.down("ArrowRight");
    try {
        for (let frameIndex = 0; frameIndex < 4; ++frameIndex) {
            await page.waitForTimeout(200);
            const frame = await snapshot();
            if (frame.targetPixels >= 16 && frame.targetCenter[0] >= destination) return frame;
        }
    } finally {
        await page.keyboard.up("ArrowRight");
    }
    return null;
}

async function checkPyromaxCourses(page, result, standing, snapshot, opened) {
    // Sample several frames while the original jump and projectile states run.
    await page.keyboard.down("Space");
    try {
        for (let attempt = 0; attempt < 12; ++attempt) {
            const frame = await snapshot();
            if (frame.targetCenter && frame.targetCenter[1] <= standing.targetCenter[1] - 12) {
                result.playerAfterJump = frame.targetCenter;
                break;
            }
            await page.waitForTimeout(65);
        }
    } finally {
        await page.keyboard.up("Space");
    }
    if (!result.playerAfterJump) throw new Error("Space did not make the player jump");

    const shot = () => canvasFrame(page, 0xffe65c, [], false, true);
    if ((await shot()).targetPixels) throw new Error("The shot color appeared before firing");
    await page.keyboard.down("z");
    try {
        for (let attempt = 0; attempt < 12; ++attempt) {
            const frame = await shot();
            if (frame.targetPixels >= 12) {
                result.shotPixels = frame.targetPixels;
                break;
            }
            await page.waitForTimeout(65);
        }
    } finally {
        await page.keyboard.up("z");
    }
    if (!result.shotPixels) throw new Error("Z did not draw a projectile");

    // The gate switches to the score screen and clears the player sprite.
    let missing = 0;
    const gateDeadline = Date.now() + 16000;
    await page.keyboard.down("ArrowRight");
    try {
        while (missing < 3 && Date.now() < gateDeadline) {
            const frame = await snapshot();
            missing = frame.targetPixels < 16 ? missing + 1 : 0;
            await page.waitForTimeout(170);
        }
    } finally {
        await page.keyboard.up("ArrowRight");
    }
    if (missing < 3) throw new Error("Walking to the gate did not open the score screen");

    const secondLevel = "/maps/level02.txt";
    const nextDeadline = Date.now() + 20000;
    while (!await opened(secondLevel) && Date.now() < nextDeadline) {
        await page.keyboard.press("Enter", { delay: 160 });
        await page.waitForTimeout(450);
    }
    if (!await opened(secondLevel)) throw new Error("The gate did not load the second course");
    result.nextLevel = secondLevel;

    const drawDeadline = Date.now() + 16000;
    let arrival;
    do {
        const frame = await snapshot();
        if (frame.targetPixels >= 16 && frame.colors >= 12) {
            arrival = frame.targetCenter;
            break;
        }
        await page.waitForTimeout(300);
    } while (Date.now() < drawDeadline);
    if (!arrival) throw new Error("The second course loaded without drawing the player");
    result.playerOnNextLevel = arrival;

    // Wait out the entrance animation and confirm the next course accepts play.
    const playDeadline = Date.now() + 12000;
    do {
        const frame = await movePyromaxRight(page, snapshot, arrival[0] + 50);
        if (frame) {
            result.playerAfterNextRight = frame.targetCenter;
            return frame;
        }
    } while (Date.now() < playDeadline);
    throw new Error("The player could not move in the second course");
}

async function checkDarkCastleGame(page, canvas, row) {
    const actorFrame = () => canvas.evaluate(element => {
        const { data } = element.getContext("2d").getImageData(0, 0, element.width, element.height);
        let count = 0;
        let x = 0;
        let y = 0;
        // The game room's amber actor is the only sprite in this color range.
        for (let index = 0; index < data.length; index += 4) {
            const red = data[index];
            const green = data[index + 1];
            const blue = data[index + 2];
            if (red < 110 || red < green + 30 || green < blue + 22 || blue > 90) continue;
            count++;
            x += index / 4 % element.width;
            y += Math.floor(index / 4 / element.width);
        }
        return { count, center: count ? [x / count, y / count] : null };
    });
    await canvas.focus();
    let start = await actorFrame();
    for (let attempt = 0; !start.center && attempt < 9; attempt++) {
        await page.keyboard.press("Enter", { delay: 100 });
        await page.waitForTimeout(650);
        start = await actorFrame();
    }
    if (!start.center || start.count < 40) throw new Error("The Dark Castle menu did not start the game room");

    await page.keyboard.press("ArrowRight", { delay: 1750 });
    await page.waitForTimeout(250);
    const right = await actorFrame();
    if (!right.center || right.center[0] - start.center[0] < 8 || right.count > start.count * 1.5) {
        throw new Error("The Dark Castle actor did not move cleanly to the right");
    }
    await page.keyboard.press("ArrowUp", { delay: 1750 });
    await page.waitForTimeout(250);
    const up = await actorFrame();
    if (!up.center || right.center[1] - up.center[1] < 8 || up.count > start.count * 1.5) {
        throw new Error("The Dark Castle actor did not move cleanly upward");
    }
    row.darkCastleGame = { start, right, up };
    return canvasFrame(page, null, [], false, true);
}

async function checkDarkCastleEditor(page, canvas, row, output) {
    const mapPath = "/maps/newfile.map";
    const saveRoom = async () => {
        await page.keyboard.press("F2");
        await page.waitForTimeout(180);
        await page.keyboard.press("F4");
        await page.waitForTimeout(180);
        await page.keyboard.press("Enter");
    };
    const readRoom = () => page.evaluate(file => Module.FS.readFile(file, { encoding: "utf8" }), mapPath);
    // The native writer stores an empty object name as a space-padded field.
    const originFloor = map => Number(/^0,0,0,(-?\d+),0," *",0\r?$/m.exec(map)?.[1]);
    const waitForRoom = async () => {
        const deadline = Date.now() + 9000;
        do {
            const frame = await canvasFrame(page, null, [], false, true);
            if (frame.colors >= 12) return frame;
            await page.waitForTimeout(150);
        } while (Date.now() < deadline);
        throw new Error("The original Dark Castle room did not render");
    };

    const original = await waitForRoom();
    await canvas.click();
    await saveRoom();
    await page.waitForFunction(file => globalThis.Module?.FS?.analyzePath(file).exists, mapPath, { timeout: 7000 });
    const before = await readRoom();
    await writeFile(path.join(output, "logs", "darkcastle-initial.map"), before);
    if (!before.includes("Stone workshop") || originFloor(before) !== 2) {
        throw new Error("Saved map does not match the workshop seed");
    }

    await page.keyboard.press("Escape");
    await page.keyboard.press("a");
    await page.waitForTimeout(250);
    const edited = await canvasFrame(page, null, [], false, true);
    if (edited.hash === original.hash) throw new Error("Raising a floor did not change the displayed map");
    await saveRoom();
    await page.waitForFunction(({ file, prior }) => Module.FS.readFile(file, { encoding: "utf8" }) !== prior,
        { file: mapPath, prior: before }, { timeout: 7000 });
    const after = await readRoom();
    await writeFile(path.join(output, "logs", "darkcastle-edited.map"), after);
    if (originFloor(after) !== 3) throw new Error("The saved origin floor did not rise by one cell");

    const mapMenu = page.locator("#darkcastle-maps summary");
    if (await mapMenu.count()) {
        await mapMenu.click();
        const button = page.getByRole("button", { name: "Download map" });
        const [download] = await Promise.all([page.waitForEvent("download"), button.click()]);
        if (download.suggestedFilename() !== "newfile.map" || await readFile(await download.path(), "utf8") !== after) {
            throw new Error("The browser download differs from the saved native map file");
        }
    }

    // Restoration belongs to the page's IndexedDB workspace. Opening the
    // restored file goes through the editor's original F3 file browser.
    await page.waitForTimeout(500);
    await page.reload({ waitUntil: "domcontentloaded" });
    await page.waitForFunction(file => globalThis.Module?.FS?.analyzePath(file).exists, mapPath, { timeout: 9000 });
    if (await readRoom() !== after) throw new Error("The edited map did not survive a reload");
    await waitForRoom();
    await canvas.click();
    await page.keyboard.press("F2");
    await page.waitForTimeout(180);
    await page.keyboard.press("F3");
    await page.waitForTimeout(500);
    const picker = await canvasFrame(page, null, [], false, true);
    await writeFile(path.join(output, "logs", "darkcastle-picker.png"), picker.png);
    await page.keyboard.press("Enter");
    await page.waitForTimeout(250);
    await page.keyboard.press("Escape");
    await page.keyboard.press("a", { delay: 80 });
    await saveRoom();
    await page.waitForFunction(({ file, prior }) => Module.FS.readFile(file, { encoding: "utf8" }) !== prior,
        { file: mapPath, prior: after }, { timeout: 7000 });
    const reopened = await readRoom();
    await writeFile(path.join(output, "logs", "darkcastle-reopened.map"), reopened);
    if (!reopened.includes("Stone workshop") || originFloor(reopened) !== 4) {
        throw new Error("Reopened map has unexpected origin floor " + originFloor(reopened));
    }
    row.darkcastle = { originalFloor: 2, editedFloor: 3, floorAfterReopen: 4,
        bytes: Buffer.byteLength(after), downloadMatchesMap: Boolean(await mapMenu.count()) };
    return edited;
}

async function checkProspectorGame(page, canvas, row, output) {
    page.on("request", request => {
        if (new URL(request.url()).pathname.endsWith("/roguelikes/beacon")) {
            row.errors.push("Prospector attempted to report a run to a leaderboard");
        }
    });
    const waitReady = () => page.waitForFunction(() => {
        const screen = document.getElementById("canvas");
        return typeof window.Module?._rv_layout === "function" &&
            screen.getContext("2d").getImageData(0, 0, 1, 1).data[3] === 255;
    }, null, { timeout: 18000 });
    const layout = () => page.evaluate(() => {
        const first = Module._rv_layout() >> 2;
        return Array.from(Module.HEAP32.subarray(first, first + 9));
    });
    const position = values => values.slice(6, 8);
    const mapSignature = () => page.evaluate(() => {
        const screen = document.getElementById("canvas");
        const first = Module._rv_layout() >> 2;
        const [mode, width, height] = Module.HEAP32.subarray(first, first + 3);
        if (mode !== 1 || width <= 0 || height <= 0 || width > screen.width || height > screen.height) return null;
        const pixels = screen.getContext("2d").getImageData(0, 0, width, height).data;
        let hash = 2166136261; // FNV-1a over the space map, excluding the status and messages.
        for (let offset = 0; offset < pixels.length; offset += 4) {
            const color = pixels[offset] * 65536 + pixels[offset + 1] * 256 + pixels[offset + 2];
            hash = Math.imul(hash ^ color, 16777619);
        }
        return hash >>> 0;
    });
    const press = key => page.keyboard.press(key, { delay: 110 });
    const saves = () => page.evaluate(() => Module.FS.readdir("/prospector/savegames")
        .filter(name => name.endsWith(".sav") && name !== "empty.sav").map(name => ({ name,
            bytes: Module.FS.stat("/prospector/savegames/" + name).size })));

    await waitReady();
    await canvas.focus();
    await writeFile(path.join(output, "logs", "prospector-title.png"),
        (await canvasFrame(page, null, [], false, true)).png);
    for (let step = 0; step < 11 && (await layout())[0] !== 1; step++) {
        await press("Enter");
        await page.waitForTimeout(650);
    }
    const before = await layout();
    if (before[0] !== 1) throw new Error("Prospector did not reach the space map");
    await page.waitForTimeout(250);
    const originalMap = await mapSignature();
    await press("ArrowRight");
    // When the ship is centered, movement scrolls the map underneath it.
    // Compare the map itself so a moving viewport counts as navigation.
    const moveLimit = Date.now() + 5500;
    let movedMap;
    do {
        movedMap = await mapSignature();
        if (movedMap !== null && movedMap !== originalMap) break;
        await page.waitForTimeout(160);
    } while (Date.now() < moveLimit);
    if (movedMap === null || movedMap === originalMap) throw new Error("The space map did not respond to movement");
    const moved = await layout();
    const frame = await canvasFrame(page, null, [], false, true);
    if (frame.colors < 6) throw new Error("The space map is missing from the canvas");

    const previous = await saves();
    await press("Shift+KeyS");
    await page.waitForFunction(() => Module.HEAP32[(Module._rv_layout() >> 2) + 8] === 1,
        null, { timeout: 6000 });
    await press("y");
    const deadline = Date.now() + 14000;
    let saved;
    do {
        const files = await saves().catch(() => []);
        saved = files.find(file => file.bytes > 128 && !previous.some(old => old.name === file.name));
        if (saved) break;
        await page.waitForTimeout(300);
    } while (Date.now() < deadline);
    if (!saved) throw new Error("The native save command did not write a game file");

    // The game may ask the player to dismiss its closing message before the
    // page returns to the title. The browser must restore the actual file.
    await page.waitForTimeout(1400);
    await page.reload({ waitUntil: "domcontentloaded" });
    await waitReady();
    if (!(await saves()).some(file => file.name === saved.name && file.bytes === saved.bytes)) {
        throw new Error("The saved game did not survive a reload");
    }
    row.prospector = { from: position(before), to: position(moved), mapChanged: true, saved };
    await canvas.focus();
    await press("ArrowDown");
    await page.waitForTimeout(250);
    await press("Enter");
    await page.waitForTimeout(650);
    // The original menus share a cursor. Load game starts on row two,
    // which is Exit in the picker when there is one saved run.
    await press("ArrowUp");
    await page.waitForTimeout(180);
    await press("Enter");
    await page.waitForFunction(() => Module.HEAP32[Module._rv_layout() >> 2] === 1, null, { timeout: 9000 });
    const restored = await layout();
    await page.waitForTimeout(250);
    if (restored[6] !== moved[6] || restored[7] !== moved[7] || await mapSignature() !== movedMap) {
        throw new Error("Loading the save did not restore the ship's position");
    }
    row.prospector.restored = position(restored);
    return frame;
}

async function checkCompilerTool(page, row, outputRoot) {
    const status = page.locator("#compiler-status");
    const editor = page.locator("#compiler-source");
    const output = page.locator("#compiler-output");
    const download = page.locator("#compiler-download");
    const run = () => page.getByRole("button", { name: "Compile", exact: true }).click();
    const waitForStatus = async text => {
        try {
            await status.filter({ hasText: text }).waitFor({ timeout: 22000 });
        } catch (error) {
            const diagnostic = await page.locator("#compiler-diagnostics").textContent();
            throw new Error(`Expected ${text} Received ${await status.textContent()} ${diagnostic}`, { cause: error });
        }
    };
    const source = '#include once "workshop.bi"\n' +
        "Function Twice(ByVal v As Long) As Long\n    Return v + v\nEnd Function\nPrint Twice(WORKSHOP_VALUE)\n";
    await page.locator("#compiler-file").setInputFiles({
        name: "workshop.bas", mimeType: "text/plain", buffer: Buffer.from(source),
    });
    await page.waitForFunction(value => document.querySelector("#compiler-source").value === value, source);
    await page.locator("#compiler-includes").setInputFiles({
        name: "workshop.bi", mimeType: "text/plain", buffer: Buffer.from("#define WORKSHOP_VALUE 42\n"),
    });
    await page.locator("#compiler-mode").selectOption("preprocess");
    await run();
    await waitForStatus("Generated main.pp.bas.");
    const preprocessed = await output.inputValue();
    if (!/print\s+Twice\s*\(\s*42\s*\)/i.test(preprocessed) || /WORKSHOP_VALUE/.test(preprocessed)) {
        throw new Error("The compiler did not expand the macro from the uploaded include");
    }
    await page.locator("#compiler-mode").selectOption("c");
    await run();
    await waitForStatus("Generated main.c.");
    const generated = await output.inputValue();
    if (!/\bmain\s*\(/.test(generated) || !/\bTWICE\s*\(\s*42\s*\)/i.test(generated) || !/fb_PrintInt/.test(generated)) {
        throw new Error("The compiler did not emit C with a program entry point and the expected function call");
    }
    const pending = page.waitForEvent("download");
    await download.click();
    const saved = await pending;
    const bytes = await readFile(await saved.path());
    if (saved.suggestedFilename() !== "main.c" || bytes.toString("utf8").replace(/\r\n/g, "\n") !== generated) {
        throw new Error("The downloaded C differs from the compiler output");
    }
    await editor.fill("Function MissingBody(\n");
    await run();
    await waitForStatus("Compilation failed.");
    const diagnostic = await page.locator("#compiler-diagnostics").textContent();
    if (!/error\s+\d+/i.test(diagnostic) || await download.isVisible()) {
        throw new Error("Malformed BASIC did not produce a compiler diagnostic");
    }
    await writeFile(path.join(outputRoot, "logs", row.slug + "-generated.c"), bytes);
    row.compiler = { preprocessedBytes: Buffer.byteLength(preprocessed), generatedBytes: bytes.length,
        localIncludeExpanded: true, invalidSourceRejected: true };
}

async function checkLbasicParser(page, row, output) {
    const source = 'DIM amount AS LONG\namount = 6 * 7\nPRINT amount\n';
    const editor = page.locator("#compiler-source");
    const status = page.locator("#compiler-status");
    const download = page.locator("#compiler-download");
    const run = () => page.getByRole("button", { name: "Parse", exact: true }).click();
    await page.locator("#compiler-file").setInputFiles({
        name: "arithmetic.bas", mimeType: "text/plain", buffer: Buffer.from(source),
    });
    await page.waitForFunction(value => document.querySelector("#compiler-source").value === value, source);
    await run();
    await status.filter({ hasText: "Generated main.parse." }).waitFor({ timeout: 10000 });
    const dump = await page.locator("#compiler-output").inputValue();
    if (!/Program:[\s\S]*AMOUNT[\s\S]*call\(\*[\s\S]*6, 7/i.test(dump)) {
        throw new Error("L-BASIC did not dump the arithmetic program");
    }
    const pending = page.waitForEvent("download");
    await download.click();
    const saved = await pending;
    const bytes = await readFile(await saved.path());
    if (saved.suggestedFilename() !== "main.parse" || bytes.toString("utf8").replace(/\r\n/g, "\n") !== dump) {
        throw new Error("The downloaded parse dump differs from the visible output");
    }
    await editor.fill("DIM amount AS LONG\namount = (\n");
    await run();
    await status.filter({ hasText: "Parsing failed." }).waitFor({ timeout: 10000 });
    const diagnostics = await page.locator("#compiler-diagnostics").textContent();
    if (!/Parser: Line2: Unexpected/i.test(diagnostics) || await download.isVisible()) {
        throw new Error("Malformed BASIC did not produce the expected L-BASIC diagnostic");
    }
    row.lbasicParser = { dumpBytes: bytes.length, invalidSourceRejected: true };
    const mode = page.locator('#compiler-mode option[value="lbasic-ir"]');
    if (!await mode.count()) return;

    await page.locator("#compiler-mode").selectOption("lbasic-ir");
    const emit = () => page.getByRole("button", { name: "Generate bitcode", exact: true }).click();
    await editor.fill(source);
    await emit();
    await status.filter({ hasText: "Generated main.bc." }).waitFor({ timeout: 15000 });
    const bitcodeEvent = page.waitForEvent("download");
    await download.click();
    const bitcode = await bitcodeEvent;
    const generated = await readFile(await bitcode.path());
    if (bitcode.suggestedFilename() !== "main.bc" || generated.subarray(0, 4).toString("hex") !== "4243c0de") {
        throw new Error("L-BASIC did not download LLVM bitcode");
    }
    await bitcode.saveAs(path.join(output, "logs", "lbasic-play-arithmetic.bc"));
    await editor.fill("DIM amount AS LONG\namount = (\n");
    await emit();
    await status.filter({ hasText: "Compilation failed." }).waitFor({ timeout: 15000 });
    const error = await page.locator("#compiler-diagnostics").textContent();
    if (!/Parser: Line2: Unexpected/i.test(error) || await download.isVisible()) {
        throw new Error("The bitcode mode offered an output after malformed BASIC");
    }
    row.lbasicLlvm = { bytes: generated.length, sha256: createHash("sha256").update(generated).digest("hex"),
        invalidSourceRejected: true };
}

async function checkVisualfbAlignment(page, row) {
    const status = page.locator("#visualfb-status");
    const source = page.locator("#visualfb-source");
    const result = page.locator("#visualfb-result");
    const download = page.locator("#visualfb-download");
    const file = page.locator("#visualfb-file");
    const waitForStatus = text => status.filter({ hasText: text }).waitFor({ timeout: 12000 });
    await waitForStatus("Aligned 2 lines.");
    const sample = await result.inputValue();
    if (sample !== "Const Small       = 7\nConst LargestName = 11\nConst Ratio       = Small + 1\n") {
        throw new Error("The sample assignments were not aligned");
    }

    const original = [
        "Dim outside = 0", "Const a = 1", "Const trèsLong = 2 ' leave = note",
        'Print "do = not change"', "Print 2 : Rem leave = this", "Const xy = 3", "Dim external = 2", "",
    ];
    const expected = [...original];
    expected[1] = "Const a        = 1";
    expected[5] = "Const xy       = 3";
    const utf8File = lines => Buffer.concat([Buffer.from([0xef, 0xbb, 0xbf]), Buffer.from(lines.join("\r\n"))]);
    await file.setInputFiles({ name: "selection.bas", mimeType: "text/plain", buffer: utf8File(original) });
    await page.waitForFunction(() => document.querySelector("#visualfb-source").value.includes("trèsLong"));
    await source.evaluate(editor => {
        const lines = editor.value.split("\n");
        editor.setSelectionRange(lines[0].length + 1, lines.slice(0, 6).join("\n").length + 1);
    });
    await page.locator("#visualfb-align").click();
    await waitForStatus("Aligned 2 lines.");
    if ((await result.inputValue()) !== expected.join("\n")) throw new Error("The selected source was changed incorrectly");
    const pending = page.waitForEvent("download");
    await download.click();
    const saved = await pending;
    const bytes = await readFile(await saved.path());
    if (!bytes.equals(utf8File(expected)) || saved.suggestedFilename() !== "selection-aligned.bas") {
        throw new Error("The download changed the selected file's UTF-8 signature or line endings");
    }

    let originalFile;
    if (process.env.FBC_VISUALFB_PROJECT) {
        originalFile = "Tools/LNGCreator/LNGCreator.bas";
        const buffer = await readFile(path.join(process.env.FBC_VISUALFB_PROJECT, originalFile));
        await file.setInputFiles({ name: "LNGCreator.bas", mimeType: "text/plain", buffer });
        await page.waitForFunction(() => document.querySelector("#visualfb-source").value.includes('With txtPathProj'));
        await source.evaluate(editor => {
            const start = editor.value.indexOf('\t\t\t.Name = "txtPathProj"');
            const stop = editor.value.indexOf("\t\tEnd With", start);
            if (start < 0 || stop <= start) throw new Error("Could not find the original textbox property block");
            editor.setSelectionRange(start, stop);
        });
        await page.locator("#visualfb-align").click();
        await waitForStatus("Aligned 6 lines.");
        const edited = await result.inputValue();
        if (!edited.includes('.Name         = "txtPathProj"') ||
            !edited.includes(".Anchor.Left  = AnchorStyle.asAnchor") ||
            !edited.includes(".TabIndex     = 4")) {
            throw new Error("The original VisualFBEditor property block did not align");
        }
    }

    await source.fill("Const x = 1\n".repeat(180) + "Const " + "y".repeat(8192) + " = 2\n");
    await page.locator("#visualfb-align").click();
    await waitForStatus("The aligned result exceeds 1 MiB.");
    if (await download.isVisible()) throw new Error("An oversized result left a download available");
    await page.locator("#visualfb-sample").click();
    await waitForStatus("Aligned 2 lines.");
    row.visualfbAlign = { selectedLines: [2, 6], savedBytes: bytes.length, keptUtf8BomAndCrlf: true,
        limitedOutput: true, originalFile };
}

async function checkBrpgMap(page, row) {
    const status = page.locator("#brpg-status");
    const file = page.locator("#brpg-file");
    const download = page.locator("#brpg-download");
    const waitForStatus = text => status.filter({ hasText: text }).waitFor({ timeout: 12000 });
    const downloaded = async () => {
        await download.waitFor({ state: "visible", timeout: 12000 });
        const pending = page.waitForEvent("download");
        await download.click();
        return readFile(await (await pending).path());
    };
    await waitForStatus("24 × 16");
    const preview = page.locator("#brpg-preview");
    await preview.click({ position: { x: 5, y: 5 } });
    await page.locator("#brpg-cell").filter({ hasText: "Ground -1" }).waitFor();

    const compact = Buffer.alloc(28);
    const words = [3, 2, 0x8002, 0, 3, 0x8010, 0, 4, 32767, 0, 0xffff, 0xffff, 1, 5];
    words.forEach((word, index) => compact.writeUInt16LE(word, index * 2));
    await file.setInputFiles({ name: "small.map", mimeType: "application/octet-stream", buffer: compact });
    await waitForStatus("3 × 2 · 3 blocked cells · 4 overlays");
    const wide = await downloaded();
    const expected = Buffer.alloc(56);
    const values = [3, 2, -2, 0, 3, -16, 0, 4, 32767, 0, -32767, -32767, 1, 5];
    values.forEach((value, index) => expected.writeInt32LE(value, index * 4));
    if (!wide.equals(expected)) throw new Error("The map download does not match the signed 32-bit format");

    const originalRoot = process.env.FBC_BRPG_PROJECT;
    const originalMaps = [];
    if (originalRoot) {
        const project = path.join(originalRoot, "BRpg");
        const simulator = path.join(project, "Sim", "BBasic", "异域传说", "Map");
        for (const leaf of ["test.map", "三里镇.map"]) {
            const buffer = await readFile(path.join(project, "Res", "Map", leaf));
            const original = await readFile(path.join(simulator, leaf));
            await file.setInputFiles({ name: leaf, mimeType: "application/octet-stream", buffer });
            const output = await downloaded();
            if (!original.subarray(0, -4).equals(output) || original.readUInt32LE(original.length - 4) !== 0) {
                throw new Error("The map differs from the simulator's tile payload " + leaf);
            }
            originalMaps.push(leaf);
        }
    }

    await file.setInputFiles({ name: "truncated.map", mimeType: "application/octet-stream",
        buffer: compact.subarray(0, -2) });
    await waitForStatus("Invalid compact map dimensions or length.");
    if (await download.isVisible()) throw new Error("A malformed map left a download available");
    await page.locator("#brpg-sample").click();
    await waitForStatus("24 × 16");
    row.brpgMap = { downloadedBytes: wide.length, signedTiles: values.slice(2), originalMaps, rejectedTruncation: true };
}

async function checkMode7(page, canvas, row) {
    // These two bands exclude the horizon boundary. Camera translation
    // affects the ground and rotation also scrolls the sky.
    const band = (top, bottom) => {
        const points = [];
        for (let y = top; y < bottom; y += 0.02) {
            for (let x = 0.02; x < 0.99; x += 0.02) points.push([x, y]);
        }
        return points;
    };
    const sky = band(0.05, 0.26);
    const ground = band(0.37, 0.97);
    const read = () => canvasFrame(page, null, [...sky, ...ground]);
    const differences = (first, second) => {
        const moved = { sky: 0, ground: 0 };
        first.sampleColors.forEach((color, index) => {
            if (color !== second.sampleColors[index]) {
                if (index < sky.length) ++moved.sky;
                else ++moved.ground;
            }
        });
        return moved;
    };
    await canvas.click();
    await page.waitForTimeout(350);
    const start = await read();
    const skyColors = new Set(start.sampleColors.slice(0, sky.length)).size;
    const groundColors = new Set(start.sampleColors.slice(sky.length)).size;
    if (skyColors < 5 || groundColors < 5) throw new Error("Tilengine did not render the sky and ground");

    await page.keyboard.press("ArrowUp", { delay: 850 });
    const forward = await read();
    const forwardChange = differences(start, forward);
    if (forwardChange.ground < 60 || forwardChange.sky !== 0) {
        throw new Error("Forward input did not shift just the ground");
    }
    await page.keyboard.press("ArrowRight", { delay: 650 });
    const turned = await read();
    const turnChange = differences(forward, turned);
    if (turnChange.ground < 60 || turnChange.sky < 25) {
        throw new Error("Steering did not shift both the ground and sky");
    }
    await page.keyboard.press("z", { delay: 240 });
    const resetChange = differences(start, await read());
    row.mode7 = { skyColors, groundColors, forwardChange, turnChange, resetChange };
    if (resetChange.ground || resetChange.sky) throw new Error("Reset did not restore the original view");
    return turned;
}

async function checkCanvas(page, check, row, output) {
    const canvas = page.locator("#canvas");
    await page.waitForFunction(() => document.querySelector("#canvas")?.width >= 320,
        null, { timeout: check.standaloneShell ? 15000 : 7000 });
    if (check.prospectorGame) return checkProspectorGame(page, canvas, row, output);
    if (check.standaloneShell) return checkOhrGame(page, canvas, row);
    // The first frame may arrive one browser repaint after the canvas resize.
    await page.waitForTimeout(check.startupDelay ?? 180);
    if (check.araxiaGame) return checkAraxiaGame(page, row);
    if (check.griffonGame) return checkGriffonGame(page, row);
    if (check.phoenixGame) return checkPhoenixGame(page, canvas, row);
    if (check.mode7) return checkMode7(page, canvas, row);
    if (check.pyromaxGame) return checkPyromaxGame(page, canvas, row, check.pyromaxWorkshop);
    if (check.darkCastleGame) return checkDarkCastleGame(page, canvas, row);
    if (check.darkCastleEditor) return checkDarkCastleEditor(page, canvas, row, output);
    if (check.chessMove) return checkChessMove(page, canvas, row);
    if (check.nebunoidGame) return checkNebunoidGame(page, canvas, row);
    if (check.linearRegression) return checkLinearRegression(page, canvas, row);
    if (check.editSample) return checkTikoEditor(page, canvas, row);
    if (check.gomokuMove) return checkUgomoku(page, row);
    if (check.nesController) return checkNesController(page, row);
    if (check.eqPlayback) return checkEqualizer(page, row);
    if (check.sampleImage) return checkImage(page, check.sampleImage, row);
    if (check.paddle) {
        const bounds = await canvas.boundingBox();
        const samples = [[0.25, 0.008], [0.75, 0.008]];
        await page.mouse.move(bounds.x + bounds.width * 0.25, bounds.y + bounds.height * 0.4);
        await page.waitForTimeout(160);
        const left = await canvasFrame(page, null, samples);
        await page.mouse.move(bounds.x + bounds.width * 0.75, bounds.y + bounds.height * 0.4);
        await page.waitForTimeout(160);
        const right = await canvasFrame(page, null, samples);
        if (left.sampleColors[0] !== check.paddle || left.sampleColors[1] === check.paddle ||
            right.sampleColors[0] === check.paddle || right.sampleColors[1] !== check.paddle) {
            throw new Error("The mouse did not move the top paddle between the two positions");
        }
        row.canvas = { colors: right.colors, paddleFollowedPointer: true };
        return right;
    }
    if (check.drawingColor) {
        const frame = await canvasFrame(page, check.drawingColor);
        if (frame.colors < 2 || frame.targetPixels < 25) throw new Error("Expected drawing was missing from the canvas");
        row.canvas = { colors: frame.colors, drawingPixels: frame.targetPixels };
        return frame;
    }
    if (check.grayscale) {
        const frame = await canvasFrame(page, null, [], true);
        if (frame.original.chromatic < 100 || frame.luma.chromatic !== 0 || frame.luma.tones < 10) {
            throw new Error("Expected a color source and a grayscale image with multiple tones");
        }
        row.canvas = { original: frame.original, luma: frame.luma };
        return frame;
    }
    const before = await canvasFrame(page, check.awaitColor, [], false, check.directCanvas);
    if (check.animationDelay) {
        await page.waitForTimeout(check.animationDelay);
    } else if (check.keys) {
        for (const key of check.keys) await page.keyboard.press(key, { delay: 40 });
        await page.waitForTimeout(150);
    } else {
        await page.keyboard.down(check.key);
        if (check.hold) await page.waitForTimeout(check.hold);
    }
    let after = await canvasFrame(page, check.awaitColor, [], false, check.directCanvas);
    for (let frame = 0; check.awaitColor && after.targetPixels <= before.targetPixels && frame < 8; ++frame) {
        after = await canvasFrame(page, check.awaitColor);
    }
    if (check.key) await page.keyboard.up(check.key);
    row.canvas = { before: before.colors, after: after.colors, changed: before.hash !== after.hash };
    if (after.colors < 2 || !row.canvas.changed) throw new Error("Input produced no visible canvas activity");
    if (check.awaitColor && after.targetPixels <= before.targetPixels) {
        throw new Error("The key press did not produce a visible bullet");
    }
    return after;
}

async function checkCatalogConsole(page, check, row) {
    const transcript = page.locator("#console-output");
    const status = page.locator("#program-status");
    const field = page.getByRole("textbox", { name: "Your answer" });
    const send = page.getByRole("button", { name: "Send" });
    // A multibyte input longer than the runtime's 512-byte read buffer
    // must be rejected by the game as one turn, then allow normal play.
    const answers = row.slug === "asylum" ? ["\u{1f98a}".repeat(150), ...check.answers] : check.answers;
    let turn = 0;
    // Wait for the visible field or normal completion. The page can retain
    // the same DOM nodes between turns, so waiting for a new element would
    // miss a fast program's next request.
    await transcript.waitFor({ state: "visible", timeout: 7000 });
    while (answers && turn <= answers.length) {
        await page.waitForFunction(() => {
            const form = document.getElementById("console-form");
            return (!form.hidden && !form.querySelector('button[type="submit"]').disabled) ||
                document.getElementById("program-status").textContent === "Program finished.";
        }, null, { timeout: 7000 });
        if (await status.textContent() === "Program finished.") break;
        const currentText = await transcript.textContent();
        if (!currentText.trim() || !await field.isVisible()) throw new Error("The question or input is missing");
        if (turn === answers.length) {
            if (!check.stopAtNextPrompt) throw new Error("Program requested additional input");
            row.reachedPrompt = true;
            break;
        }
        const answer = answers[turn++];
        row.input.push(answer);
        await field.fill(answer, { timeout: 4000 });
        if (turn === 1) await send.tap({ timeout: 4000 });
        else if (turn % 2) await field.press("Enter", { timeout: 4000 });
        else await send.click({ timeout: 4000 });
        // A disabled button may be re-enabled before a DOM poll runs. The
        // echoed answer confirms that the browser actually dispatched submit.
        await page.waitForFunction(length => document.getElementById("console-output").textContent.length > length,
            currentText.length, { timeout: 4000 });
    }
    if (check.key) {
        await page.waitForFunction(() => document.getElementById("console-output").textContent.includes("END PARSING"),
            null, { timeout: 7000 });
        await page.getByRole("button", { name: "Finish" }).tap();
    }
    if (!check.stopAtNextPrompt) {
        await page.waitForFunction(() => document.getElementById("program-status").textContent === "Program finished.",
            null, { timeout: 7000 });
    }
    for (const expected of check.output) {
        if (!expected.test(await transcript.textContent())) throw new Error("Missing visible output " + expected);
    }
    if (row.slug === "asylum" && !(await transcript.textContent()).includes("Try Again:")) {
        throw new Error("The game did not receive the long line as a single answer");
    }
    row.output = (await transcript.textContent()).slice(0, 12000);
    if (check.stopAtNextPrompt) await page.setViewportSize({ width: 390, height: 410 });
    row.viewport = await page.evaluate(() => new Promise(resolve => requestAnimationFrame(() => requestAnimationFrame(() => {
        const output = document.getElementById("console-output");
        resolve({ width: innerWidth, content: document.documentElement.scrollWidth,
            height: innerHeight, inputBottom: document.getElementById("console-answer").getBoundingClientRect().bottom,
            outputScroll: output.scrollTop, outputTail: output.scrollHeight - output.scrollTop - output.clientHeight });
    }))));
    if (row.viewport.content > row.viewport.width) throw new Error("The program page overflows its viewport");
    if (row.viewport.inputBottom > row.viewport.height) throw new Error("The input fell below the short viewport");
    if (check.stopAtNextPrompt && row.viewport.outputTail > 24) throw new Error("Resizing hid the latest prompt");
    if (!check.answers && row.viewport.outputScroll !== 0) throw new Error("Read-only output did not start at the top");
    if (check.oregonHunt) {
        await field.fill("1");
        await field.press("Enter");
        const prompt = await page.waitForFunction(() => {
            const text = document.getElementById("console-output").textContent;
            const send = document.querySelector("#console-form button[type=submit]");
            return !send.disabled && /TYPE (BANG|BLAM|POW|WHAM)\s*\?/i.exec(text)?.[1];
        }, null, { timeout: 5000 });
        const word = await prompt.jsonValue();
        // Send characters individually so the timer sees ordinary typing.
        await field.pressSequentially(word, { delay: 60 });
        await send.tap();
        await page.waitForFunction(() => document.getElementById("console-output").textContent
            .includes("RIGHT BETWEEN THE EYES"), null, { timeout: 5000 });
        row.hunt = { word, shotSucceeded: true };
        row.output = (await transcript.textContent()).slice(0, 12000);
    }
}

async function checkTerminalEditor(page, row, catalog) {
    const terminal = page.locator("#termDiv");
    async function waitForText(text) {
        try {
            await page.waitForFunction(value => document.getElementById("termDiv")
                ?.innerText.replace(/\u00a0/g, " ").includes(value), text, { timeout: 8000 });
        } catch {
            row.output = ((await terminal.textContent()) || "").replace(/\u00a0/g, " ").slice(0, 4000);
            throw new Error("Clumsier did not display " + JSON.stringify(text));
        }
    }
    await waitForText("Hello, World!");
    if (!catalog) {
        await page.keyboard.press("End", { delay: 120 });
        await page.keyboard.press("KeyX", { delay: 120 });
        await waitForText('World!"x');
        row.output = (await terminal.innerText()).replace(/\u00a0/g, " ");
        return;
    }
    // WIDTH() must preserve the native 80 by 25 viewport as the editor
    // queries it. The previous runtime cleared the screen and lost this menu.
    await waitForText("<F1> New | <F2> Open | <F3> Save");
    await waitForText("<F9> Help");

    const status = page.locator("#editor-status");
    const upload = page.getByLabel("Import .bas");
    const name = "browser_test.bas";
    await upload.setInputFiles({ name, mimeType: "text/plain", buffer: Buffer.from("x".repeat(80)) });
    await status.filter({ hasText: "up to 79 characters" }).waitFor();
    const comment = "' Keep %CS and %n as literal source.";
    const file = { name, mimeType: "text/plain", buffer: Buffer.from("print 40\r\n" + comment + "\r\n") };
    await upload.setInputFiles(file);
    await status.filter({ hasText: "Imported browser_test.bas" }).waitFor();
    await upload.setInputFiles(file);
    await status.filter({ hasText: "already exists" }).waitFor();

    async function openSavedFile() {
        await page.getByRole("button", { name: "Open", exact: true }).click();
        await waitForText("Insert file name");
        await page.getByRole("textbox", { name: "Editor prompt" }).fill("browser_test");
        await page.getByRole("button", { name: "Send", exact: true }).click();
    }

    await openSavedFile();
    await waitForText("print 40");
    await waitForText(comment);
    await page.keyboard.press("End", { delay: 120 });
    await page.keyboard.press("Shift+Equal", { delay: 120 });
    await page.keyboard.press("Digit2", { delay: 120 });
    await waitForText("print 40+2");
    await page.getByRole("button", { name: "Save", exact: true }).click();
    await waitForText("file saved!");
    await waitForText("print 40+2");
    await page.getByLabel("Saved files").selectOption(name);
    const [saved] = await Promise.all([
        page.waitForEvent("download"),
        page.getByRole("button", { name: "Download .bas" }).click(),
    ]);
    const source = (await readFile(await saved.path(), "utf8")).replace(/\r\n/g, "\n").trimEnd();
    if (saved.suggestedFilename() !== name || source !== "print 40+2\n" + comment) {
        throw new Error("Clumsier did not download the saved edit");
    }

    // Reload the same browser context and reopen through Clumsier's own
    // directory listing to confirm the edit was saved in IndexedDB.
    await page.reload();
    await waitForText("Hello, World!");
    await openSavedFile();
    await waitForText("print 40+2");
    await waitForText(comment);
    row.editor = { file: name, uploadValidated: true, exported: true, restoredAfterReload: true };
    row.output = (await terminal.innerText()).replace(/\u00a0/g, " ");
    await page.setViewportSize({ width: 390, height: 844 });
    if (await page.evaluate(() => document.documentElement.scrollWidth > innerWidth)) {
        throw new Error("The Clumsier page overflows a narrow viewport");
    }
    await page.setViewportSize({ width: 960, height: 800 });
}

async function checkBindings(page, row) {
    const source = page.getByRole("textbox", { name: "C header", exact: true });
    const output = page.getByRole("textbox", { name: "FreeBASIC bindings" });
    const convert = page.getByRole("button", { name: "Convert header" });
    const download = page.getByRole("link", { name: "Download .bi" });
    const convertBounds = await convert.boundingBox();
    const outputBounds = await output.boundingBox();
    if (!convertBounds || !outputBounds || convertBounds.y >= outputBounds.y) {
        throw new Error("The narrow layout makes visitors scroll past the result to convert");
    }

    await convert.click();
    await page.getByText("Bindings ready.", { exact: true }).waitFor({ timeout: 22000 });
    const sample = await output.inputValue();
    if (!/\btype\s+point\b/i.test(sample) || !/\bdeclare function point_distance\b/i.test(sample)) {
        throw new Error("The sample header did not produce its type and function declarations");
    }

    await source.fill("/* comment left open\n");
    await convert.click();
    await page.waitForFunction(() => !document.getElementById("binding-convert").disabled, null, { timeout: 22000 });
    const invalidStatus = await page.locator("#binding-status").textContent();
    if (invalidStatus !== "fbfrog could not convert this header.") {
        throw new Error("Malformed header: " + invalidStatus + " " + await output.inputValue());
    }
    if (!/comment left open/i.test(await page.locator("#binding-diagnostics").textContent())) {
        throw new Error("The malformed header did not display a lexer diagnostic");
    }
    if (await output.inputValue() || await download.isVisible()) throw new Error("An invalid header retained an old result");

    await page.getByLabel("Open header").setInputFiles({ name: "custom.h", mimeType: "text/plain",
        buffer: Buffer.from("typedef unsigned int WidgetId;\nint open_widget(WidgetId id);\n") });
    await page.getByText("Loaded custom.h.", { exact: true }).waitFor();
    await convert.click();
    await page.getByText("Bindings ready.", { exact: true }).waitFor({ timeout: 22000 });
    const result = await output.inputValue();
    if (!/\btype\s+WidgetId\s+as\s+ulong\b/i.test(result) ||
        !/\bdeclare function open_widget\b/i.test(result) || /point_distance/i.test(result)) {
        throw new Error("The custom header was not converted in a fresh worker");
    }

    const [saved] = await Promise.all([page.waitForEvent("download"), download.click()]);
    if (saved.suggestedFilename() !== "bindings.bi" || await readFile(await saved.path(), "utf8") !== result) {
        throw new Error("The download differs from the displayed bindings");
    }
    const overflow = await page.evaluate(() => document.documentElement.scrollWidth > innerWidth);
    if (overflow) throw new Error("Binding generator overflows a narrow viewport");
    row.bindings = { downloaded: saved.suggestedFilename(), recoveredFromError: true, bytes: Buffer.byteLength(result) };
    row.output = result;
}

async function checkConverter(page, row) {
    const select = page.getByLabel("Conversion");
    const fields = page.locator("#converter-fields input");
    const submit = page.getByRole("button", { name: "Calculate" });
    const result = page.locator("#converter-result");
    const operations = [
        ["1", ["1000"], /3280\.84 feet/],
        ["2", ["10000"], /3048\.00 meters/],
        ["3", ["100"], /62\.14 miles/],
        ["4", ["100"], /160\.93 kilometers/],
        ["5", ["12", "30", "0"], /12\.50000.*decimal/],
        ["6", ["12.5"], /12.*30'.*0\.00000/],
        ["6", ["0"], /0\.00000.*decimal is\s+0/],
        ["7", ["3", "10000", "1000"], /32\.5 miles \(\s*52\.3 km\)/],
    ];
    async function calculate(operation, values) {
        await select.selectOption(operation);
        for (const [index, value] of values.entries()) await fields.nth(index).fill(value);
        await submit.click();
        await page.waitForFunction(() => !document.getElementById("converter-calculate").disabled);
        return result.textContent();
    }
    const bad = await calculate("7", ["3", "1000", "2000"]);
    if (!bad.includes("Destination elevation must be")) {
        throw new Error("The calculator accepted a destination above the starting altitude");
    }
    row.calculations = [];
    for (const [operation, values, expected] of operations) {
        const text = await calculate(operation, values);
        if (!expected.test(text)) throw new Error("Unexpected result for conversion " + operation + ": " + text);
        row.calculations.push({ operation, values, text });
    }
    if (await page.evaluate(() => document.documentElement.scrollWidth > innerWidth)) {
        throw new Error("The calculator overflows a narrow viewport");
    }
    row.output = row.calculations.map(item => item.text).join("\n");
}

async function checkFormatTool(page, row) {
    const value = page.getByLabel("Value", { exact: true });
    const mask = page.getByLabel("Format mask");
    const submit = page.getByRole("button", { name: "Format", exact: true });
    const result = page.getByLabel("Formatted output");
    async function format(number, pattern) {
        await value.fill(number);
        await mask.fill(pattern);
        await submit.click();
        await page.waitForFunction(() => !document.getElementById("format-submit").disabled);
        return result.textContent();
    }
    const grouped = await format("1234.5", "#,##0.00");
    if (grouped !== "1,234.50") throw new Error("Unexpected grouped result: " + grouped);
    const negative = await format("-12.25", "0.000");
    if (negative !== "-12.250") throw new Error("Unexpected negative result: " + negative);
    const precision = 5000;
    const precise = await format("0.125", "0." + "0".repeat(precision));
    if (precise !== "0.125" + "0".repeat(precision - 3)) {
        throw new Error("The long mask did not produce all 5,000 fractional places");
    }
    if (await page.evaluate(() => document.documentElement.scrollWidth > innerWidth)) {
        throw new Error("Long formatted text overflows the page");
    }
    row.format = { grouped, negative, fractionalPlaces: precision };
}

async function checkImageTool(page, row) {
    const input = page.getByLabel("Open image");
    const method = page.getByLabel("Method");
    const status = page.getByRole("status");
    const download = page.getByRole("link", { name: "Download PNG" });
    const ready = (width, height) =>
        status.filter({ hasText: "Converted " + width + " × " + height + " pixels." }).waitFor();
    await ready(320, 240);

    async function fixture(width, height) {
        const data = await page.evaluate(({ width, height }) => {
            const canvas = document.createElement("canvas");
            canvas.width = width;
            canvas.height = height;
            const context = canvas.getContext("2d");
            context.fillStyle = "red";
            context.fillRect(0, 0, width, height);
            context.fillStyle = "rgb(0,255,0)";
            context.fillRect(1, 0, 1, 1);
            context.clearRect(2, 1, 1, 1);
            context.fillStyle = "rgba(0,0,255,.5)";
            context.fillRect(2, 1, 1, 1);
            return canvas.toDataURL("image/png").split(",")[1];
        }, { width, height });
        return { name: "test.png", mimeType: "image/png", buffer: Buffer.from(data, "base64") };
    }
    function observedPixels() {
        return page.locator("#imageproc-result").evaluate(canvas => {
            const context = canvas.getContext("2d");
            return [[0, 0], [1, 0], [2, 1]].map(([x, y]) => Array.from(context.getImageData(x, y, 1, 1).data));
        });
    }
    await input.setInputFiles(await fixture(3, 2));
    await ready(3, 2);
    const methods = [
        ["luma", [54, 181, 18]], ["lightness", [128, 128, 128]],
        ["average", [85, 85, 85]], ["value", [255, 255, 255]],
    ];
    for (const [name, expected] of methods) {
        if ((await method.inputValue()) !== name) {
            await method.selectOption(name);
            await ready(3, 2);
        }
        const actual = await observedPixels();
        if (actual.some((pixel, i) => pixel.slice(0, 3).some(value =>
            Math.abs(value - expected[i]) > (i === 2 ? 1 : 0)) || pixel[3] !== (i === 2 ? 128 : 255))) {
            throw new Error(name + " produced unexpected pixels " + JSON.stringify(actual));
        }
    }
    const [saved] = await Promise.all([page.waitForEvent("download"), download.click()]);
    const bytes = await readFile(await saved.path());
    if (saved.suggestedFilename() !== "value-grayscale.png" ||
        !bytes.subarray(0, 8).equals(Buffer.from("89504e470d0a1a0a", "hex")) ||
        bytes.readUInt32BE(16) !== 3 || bytes.readUInt32BE(20) !== 2) {
        throw new Error("The downloaded PNG did not match the source dimensions");
    }
    await input.setInputFiles(await fixture(1601, 1));
    await ready(1600, 1);
    await input.setInputFiles({ name: "broken.png", mimeType: "image/png", buffer: Buffer.from("not an image") });
    await status.filter({ hasText: "Could not open that image." }).waitFor();
    await page.getByRole("button", { name: "Use sample" }).click();
    await ready(320, 240);
    if (await page.evaluate(() => document.documentElement.scrollWidth > innerWidth)) {
        throw new Error("The image tool overflows a narrow viewport");
    }
    row.imageproc = { methods: methods.map(([name]) => name), alphaPreserved: true, download: saved.suggestedFilename(),
        resizedWidth: 1600, recoveredFromInvalidFile: true };
}

async function checkAsciiBitmap(page, program, row) {
    async function readSavedImage() {
        const name = "/samples/sample_ASC.BMP";
        // The SDL display can keep the Emscripten runtime alive after BASIC
        // saves the image. Wait for the output file or an explicit error.
        await page.waitForFunction(file => Number.isInteger(window.corpusExit) ||
            window.Module?.FS?.analyzePath(file).exists, name, { timeout: 8000 });
        return page.evaluate(file => {
            if (window.corpusExit) throw new Error("The image conversion failed");
            const bytes = Module.FS.readFile(file);
            const header = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength);
            const start = header.getUint32(10, true);
            const width = header.getInt32(18, true);
            const height = header.getInt32(22, true);
            const bits = header.getUint16(28, true);
            if (header.getUint16(0, true) !== 0x4D42 || width !== 83 || height !== 41 || ![24, 32].includes(bits)) {
                throw new Error("Expected an 83 by 41 RGB bitmap");
            }
            const stride = Math.ceil(width * (bits / 8) / 4) * 4;
            if (start < 54 || start + stride * height > bytes.length) throw new Error("Truncated bitmap");
            let colored = 0, black = 0, white = 0;
            for (let y = 0; y < height; ++y) {
                for (let x = 0; x < width; ++x) {
                    const offset = start + stride * y + x * (bits / 8);
                    const [b, g, r] = bytes.subarray(offset, offset + 3);
                    if (r !== g || g !== b) ++colored;
                    if (r === 0 && g === 0 && b === 0) ++black;
                    if (r === 255 && g === 255 && b === 255) ++white;
                }
            }
            return { width, height, colored, black, white };
        }, name);
    }

    const color = await readSavedImage();
    if (!color.colored || !color.black) throw new Error("Color conversion did not draw colored glyphs on black");
    await page.goto(program + "index.html?mode=monochrome", { waitUntil: "domcontentloaded" });
    const monochrome = await readSavedImage();
    if (monochrome.colored || !monochrome.black) throw new Error("Monochrome conversion retained color");
    row.bitmaps = { color, monochrome };
}

async function checkToyBASIC(page, row) {
    const field = page.getByRole("textbox", { name: "Arithmetic expression" });
    const submit = page.getByRole("button", { name: "Evaluate" });
    const cases = [
        ["5+2*3", "11"], ["(5+2)*3", "21"], ["18 / 3 / 2", "3"],
        ["-2 * (3 + .5) + 1", "-6"], ["4 / (2-2)", "", "Division by zero."],
        ["(2+3", "", "Expected a closing parenthesis."],
        ["1 2", "", "Unexpected input after the expression."],
        ["1.2.3", "", "Invalid number."], ["PRINT 1", "", "Use numbers, parentheses and +, -, * or /."],
        ["(".repeat(127) + "1" + ")".repeat(127), "1"],
        ["-".repeat(255) + "1", "-1"], ["1+".repeat(127) + "1", "128"],
        ["10 - 3 - 2", "5"],
    ];
    for (const [expression, expected, error = ""] of cases) {
        await field.fill(expression);
        await submit.click();
        await page.waitForFunction(() => !document.getElementById("toy-submit").disabled, null, { timeout: 16000 });
        const result = await page.locator("#toy-result").innerText();
        const status = await page.locator("#toy-status").innerText();
        if (result !== expected || status !== error) {
            throw new Error(`Unexpected ToyBASIC response to ${expression}: ${result || status}`);
        }
    }
    if (await page.evaluate(() => document.documentElement.scrollWidth > innerWidth)) {
        throw new Error("The ToyBASIC page overflows the narrow viewport");
    }
    row.toybasic = { expressions: cases.length, recoveredAfterErrors: true };
}

async function checkKiwi(page, row) {
    const address = page.getByLabel("HTTP or HTTPS address");
    const submit = page.getByRole("button", { name: "Inspect URL" });
    const cases = [
        ["https://birdwatch.example:8443/yard/birdwatch.example?season=fall#counts", "https", "birdwatch.example:8443",
            "/yard/birdwatch.example?season=fall#counts"],
        ["HTTP://[2001:db8::1]:8080?type=A%20B", "http", "[2001:db8::1]:8080", "?type=A%20B"],
        ["https://bare.example", "https", "bare.example", "(empty)"],
        ["mailto:visitor@example.test"],
        ["https://user@private.example/"],
        ["/relative/path"],
        ["https://fragment.example#top", "https", "fragment.example", "#top"],
    ];
    const remote = [];
    const origin = new URL(page.url()).origin;
    page.context().on("request", request => {
        if (new URL(request.url()).origin !== origin) remote.push(request.url());
    });
    for (const [url, protocol, host, resource] of cases) {
        await address.fill(url);
        await submit.click();
        await page.waitForFunction(() => !document.querySelector("#kiwi-form button").disabled);
        if (!protocol) {
            const status = await page.locator("#kiwi-status").textContent();
            if (!status.includes("absolute HTTP or HTTPS URL") || await page.locator("#kiwi-result").isVisible()) {
                throw new Error("Kiwi returned an unexpected response for " + url + ": " + status);
            }
            continue;
        }
        const actual = await page.locator("#kiwi-result dd").allTextContents();
        if (!await page.locator("#kiwi-result").isVisible() || actual.join("\n") !== [protocol, host, resource].join("\n")) {
            const status = await page.locator("#kiwi-status").textContent();
            throw new Error("Kiwi returned unexpected parts for " + url + ": " + JSON.stringify(actual) + " " + status);
        }
    }
    if (remote.length) throw new Error("The Kiwi inspector made an external request");
    row.kiwi = { addresses: cases.length, recoveredAfterErrors: true, externalRequests: 0 };
}

async function checkOpenSesh(page, row, output) {
    const input = page.getByLabel("MIDI file");
    const convert = page.getByRole("button", { name: "Convert file" });
    const download = page.getByRole("link", { name: "Download MOD" });
    const report = page.locator("#opensesh-result");

    async function verifyDownload(filename, noteCount) {
        await download.waitFor();
        const [file] = await Promise.all([page.waitForEvent("download"), download.click()]);
        if (file.suggestedFilename() !== filename) throw new Error("Unexpected MOD filename");
        const bytes = await readFile(await file.path());
        // A four-channel MOD has a 1,084-byte header, 1,024 bytes per pattern,
        // and 31 sample length fields measured in big-endian words.
        if (bytes.length < 2108 || bytes.toString("ascii", 1080, 1084) !== "M.K.") {
            throw new Error("The downloaded file is missing the four-channel ProTracker header");
        }
        const songLength = bytes[950];
        if (!songLength || songLength > 128) throw new Error("The MOD song length is out of range");
        const orders = bytes.subarray(952, 952 + songLength);
        const patternCount = 1 + Math.max(...orders);
        const patternEnd = 1084 + patternCount * 1024;
        let sampleBytes = 0;
        for (let i = 0; i < 31; i++) sampleBytes += bytes.readUInt16BE(42 + i * 30) * 2;
        if (!orders.length || sampleBytes <= 0 || patternEnd + sampleBytes !== bytes.length) {
            throw new Error("The MOD pattern and sample lengths do not match the file size");
        }
        let periods = 0;
        for (let offset = 1084; offset < patternEnd; offset += 4) {
            if ((bytes[offset] & 15) || bytes[offset + 1]) periods++;
        }
        if (periods !== noteCount || !new RegExp("Notes written: " + noteCount + "\\b").test(await report.textContent())) {
            throw new Error("The MOD note count differs from the conversion report");
        }
        await writeFile(path.join(output, "logs", filename), bytes);
        return { noteCount, patternCount, bytes: bytes.length };
    }

    async function upload(buffer, name) {
        await input.setInputFiles({ name, mimeType: "audio/midi", buffer: Buffer.from(buffer) });
        await convert.click();
    }

    await page.getByRole("button", { name: "Convert sample" }).click();
    const sample = await verifyDownload("OpenSesh-Sample.mod", 4);
    await upload(Buffer.alloc(32), "bad.mid");
    await report.waitFor();
    if (await download.isVisible() || !/does not begin with an MThd chunk/i.test(await report.textContent())) {
        throw new Error("OpenSesh did not reject malformed MIDI input");
    }
    await upload(sampleMidi([]), "no-notes.mid");
    await report.waitFor();
    if (await download.isVisible() || !/Notes: 0[\s\S]*no notes to export/i.test(await report.textContent())) {
        throw new Error("OpenSesh did not report the empty MIDI track");
    }
    await upload(sampleMidi([64, 69]), "two-notes.midi");
    const custom = await verifyDownload("two-notes.mod", 2);
    row.opensesh = { sample, custom, rejectsMalformedMidi: true, reportsEmptyTrack: true };
}

async function checkOpenMidiEditor(page, row, output) {
    const canvas = page.locator("#canvas");
    const files = page.getByLabel("Workspace file");
    const refresh = page.getByRole("button", { name: "Refresh files" });
    const download = page.getByRole("button", { name: "Download", exact: true });
    const click = (x, y) => canvas.click({ position: { x, y } });
    const exportFile = async name => {
        await files.selectOption(name);
        const [saved] = await Promise.all([page.waitForEvent("download"), download.click()]);
        if (saved.suggestedFilename() !== name) throw new Error("Incorrect download name");
        return readFile(await saved.path());
    };

    await page.waitForFunction(() => document.querySelector("#canvas")?.width === 1340);
    await page.waitForFunction(() => !document.querySelector("#midi-refresh").disabled);
    // The canvas uses one backing pixel per CSS pixel at the test viewport.
    await click(48, 153); // Note tool
    await click(320, 151); // Add a note to the first measure
    await page.evaluate(() => { window.corpusAudio = []; });
    await click(340, 42); // Play
    await page.waitForFunction(() => window.corpusAudio.length > 0);
    const peakRms = await page.evaluate(() => Math.max(...window.corpusAudio));
    await click(190, 42); // Stop
    await writeFile(path.join(output, "logs", "openmidisesh-score.png"), await canvas.screenshot());

    await click(130, 42); // Save
    await page.waitForTimeout(450);
    await click(1044, 747); // Confirm the editor's MIDI filename
    await page.waitForFunction(() => Module.FS.analyzePath("/fb-corpus-openmidisesh/session-edited.mid").exists);
    await refresh.click();
    const score = await exportFile("session-edited.mid");
    if (score.toString("ascii", 0, 4) !== "MThd" || score.length <= 26) {
        throw new Error("The editor did not export the edited MIDI");
    }
    await page.getByLabel("Import file").setInputFiles({ name: "imported.mid", mimeType: "audio/midi", buffer: score });
    await page.getByRole("status").filter({ hasText: "Imported imported.mid" }).waitFor();
    await page.reload({ waitUntil: "domcontentloaded" });
    if (!(await exportFile("imported.mid")).equals(score)) throw new Error("Workspace contents changed after reload");
    row.midiEditor = { bytes: score.length, peakRms, fileRoundtrip: true };
}

async function checkHackit(page, row, catalog) {
    const { file, source } = hackitFixture;
    if (catalog) {
        const upload = page.locator("#hackit-upload");
        await upload.setInputFiles({ name: file, mimeType: "text/plain", buffer: Buffer.from(source) });
        await page.getByRole("button", { name: "Open in editor" }).click();
    }
    const terminal = page.locator("#termDiv");
    const original = source.trimEnd();
    await page.waitForFunction(text => document.getElementById("termDiv")?.innerText.replace(/\xa0/g, " ").includes(text),
        original, { timeout: 12000 });
    await terminal.click();
    await page.keyboard.press("End", { delay: 120 });
    const addition = "'Edited in Chromium";
    await page.keyboard.type(addition, { delay: 75 });
    const expected = original + addition + "\n";
    await page.waitForFunction(text => document.getElementById("termDiv").innerText.replace(/\xa0/g, " ").includes(text),
        expected.trimEnd(), { timeout: 6000 });
    // The native editor polls the key state while Asyncify yields to Chromium.
    await page.keyboard.press("Control+s", { delay: 200 });
    await page.waitForFunction(({ file, expected }) =>
        Module.FS.readFile("/" + file, { encoding: "utf8" }).replace(/\r\n/g, "\n") === expected,
        { file, expected }, { timeout: 6000 });
    if (catalog) {
        const [saved] = await Promise.all([page.waitForEvent("download", { timeout: 7000 }),
            page.getByRole("button", { name: "Download saved file" }).click()]);
        if (saved.suggestedFilename() !== file ||
            (await readFile(await saved.path(), "utf8")).replace(/\r\n/g, "\n") !== expected) {
            throw new Error("Downloaded HACKIT document differs from the edited text");
        }
        await page.setViewportSize({ width: 390, height: 844 });
        if (await page.evaluate(() => document.documentElement.scrollWidth > innerWidth)) {
            throw new Error("The HACKIT page overflows the narrow viewport");
        }
        await page.setViewportSize({ width: 1080, height: 900 });
    }
    row.hackit = { opened: file, saved: true, downloaded: !!catalog };
}

async function checkFblint(page, row) {
    const source = page.getByRole("textbox", { name: "BASIC source" });
    const summary = page.getByRole("status");
    const findings = page.getByRole("table", { name: "Diagnostics" });
    const rule = findings.getByRole("cell", { name: "FBL001", exact: true });
    const submit = page.getByRole("button", { name: "Check source" });
    const suppressions = page.getByRole("checkbox", { name: "Honor suppressions" });
    await page.getByLabel("Profile").selectOption("style");
    const header = "' Project: Browser check\n' File: source.bas\n' Purpose: Check one source line.\n";
    const footer = "' end of source.bas\n";
    const statement = 'Print "Hello"   \n';
    const input = header + statement + footer;
    const run = async () => {
        await submit.click();
        await page.waitForFunction(() => !document.getElementById("lint-submit").disabled, null, { timeout: 18000 });
        const text = await summary.textContent();
        if (!/^0 errors, \d+ warnings, \d+ notes/.test(text)) throw new Error("Unexpected linter report: " + text);
        return text;
    };

    await page.getByLabel("Open a BASIC file").setInputFiles({ name: "browser-check.bas", mimeType: "text/plain",
        buffer: Buffer.from(input) });
    await page.waitForFunction(text => document.getElementById("lint-source").value === text, input);
    const unclean = await run();
    const location = await rule.locator("..").getByRole("cell").first().textContent({ timeout: 4000 });
    if (!/^4:\d+$/.test(location)) throw new Error("fblint reported the wrong source line: " + location);

    await source.fill(header + "' FB-LINTER: DISABLE-NEXT-LINE FBL001\n" + statement + footer);
    const suppressed = await run();
    if (await rule.count()) throw new Error("fblint ignored the source suppression");
    await suppressions.uncheck();
    const unmasked = await run();
    if (await rule.count() !== 1) throw new Error("Disabling suppressions did not restore the finding");

    await source.fill(header + statement.trimEnd() + "\n" + footer);
    const clean = await run();
    if (await rule.count()) throw new Error("The corrected source still reports trailing whitespace");
    row.lint = { rule: "FBL001", location, unclean, suppressed, unmasked, clean, sourceUpload: true };
}

async function checkStation922(page, row, output) {
    const method = page.getByLabel("Method");
    const target = page.getByLabel("Path");
    const submit = page.getByRole("button", { name: "Send request" });
    const headers = page.locator("#station-headers");
    const body = page.locator("#station-body");
    const download = page.getByRole("button", { name: "Download body" });
    const request = async (verb, path, status) => {
        await method.selectOption(verb);
        await target.fill(path);
        await submit.click();
        await page.waitForFunction(() => !document.getElementById("station-submit").disabled, null, { timeout: 12000 });
        const header = await headers.textContent();
        if (!header.startsWith(`HTTP/1.1 ${status} `)) {
            throw new Error(`Unexpected response for ${verb} ${path}: ` + (header || await page.getByRole("status").textContent()));
        }
        return { header, body: await body.textContent() };
    };

    const sample = await request("GET", "/", 200);
    if (!/Content-Type: text\/html/.test(sample.header) || !sample.body.includes("Station922 served this file.")) {
        throw new Error("Station922 did not serve its sample index document");
    }
    const head = await request("HEAD", "/", 200);
    if (head.header !== sample.header || head.body || !await download.isDisabled()) {
        throw new Error("HEAD did not return the same headers with an empty body");
    }
    await request("GET", "/missing.txt", 404);

    const fixture = path.join(output, "logs", "station922-fixture");
    await mkdir(path.join(fixture, "guide"), { recursive: true });
    const expected = "Uploaded notes served by Station922.\n";
    await writeFile(path.join(fixture, "guide", "notes.txt"), expected);
    await page.getByLabel("Site folder (optional)").setInputFiles(fixture);
    const uploaded = await request("GET", "/guide/notes.txt", 200);
    if (!/Content-Type: text\/plain/.test(uploaded.header) || uploaded.body !== expected) {
        throw new Error("Station922 did not serve the uploaded document with its original MIME type and bytes");
    }
    const [save] = await Promise.all([page.waitForEvent("download"), download.click()]);
    if (await readFile(await save.path(), "utf8") !== expected) throw new Error("The download differs from the HTTP body");
    row.station922 = { get: 200, head: 200, missing: 404, uploaded: 200, bytes: Buffer.byteLength(expected) };
}

async function checkAzdecrypt(page, row) {
    // The launch link can return control while the hosted page is still
    // downloading its deferred submit handler.
    await page.waitForURL("**/play.html", { waitUntil: "domcontentloaded" });
    const ciphertext = page.getByLabel("Ciphertext");
    const run = page.getByRole("button", { name: "Solve cipher" });
    const cancel = page.getByRole("button", { name: "Cancel search" });
    const status = page.getByRole("status");
    const candidate = page.locator("#azdecrypt-candidate");
    const waitForResult = async expected => {
        await run.click();
        await page.waitForFunction(() => !document.getElementById("azdecrypt-submit").disabled &&
            document.getElementById("azdecrypt-status").textContent.length > 0,
            null, { timeout: 35000 });
        const actual = await candidate.textContent();
        const details = await status.textContent();
        if (actual !== expected || !/^Score [\d.]+ after [\d,]+ iterations\.$/.test(details)) {
            throw new Error(`AZdecrypt did not recover the expected plaintext (${details}): ${actual.slice(0, 96)}`);
        }
        return details;
    };

    if (await ciphertext.inputValue() !== encipherFixture(azdecryptPlain)) throw new Error("The sample cipher differs from its test fixture");
    const sample = await waitForResult(azdecryptPlain);
    await ciphertext.fill("ABCD");
    await run.click();
    if (!/40 to 4,000 letters/.test(await status.textContent()) || await candidate.textContent()) {
        throw new Error("The solver did not reject a short cipher");
    }

    const alternate = azdecryptPlain.replace(/\.$/, "").split(". ").reverse().join(". ") + ".";
    const encoded = encipherFixture(alternate, "MNBVCXZLKJHGFDSAPOIUYTREWQ");
    await ciphertext.fill((encoded + "\n").repeat(3));
    await page.getByLabel("Restarts").selectOption("4");
    await run.click();
    await cancel.click();
    if (await status.textContent() !== "Search canceled.") throw new Error("The solver did not cancel the worker");

    await page.getByLabel("Restarts").selectOption("3");
    await ciphertext.fill(encoded);
    const pasted = await waitForResult(alternate);
    row.azdecrypt = { sample, pasted, shortCipherRejected: true, canceled: true };
}

async function runCheck(browser, base, output, build, catalog, consoleLayout) {
    const { slug, artifacts } = build;
    const check = checks[slug];
    const row = { slug, artifacts, status: "failed", input: [], errors: [], output: "" };
    const useBindingPage = catalog && check.bindings;
    const useConverterPage = catalog && check.converter;
    const useFormatPage = catalog && check.formatTool;
    const useImagePage = catalog && check.imageprocTool;
    const useToyPage = catalog && check.toybasicTool;
    const useKiwiPage = catalog && check.kiwiTool;
    const useMidiPage = catalog && check.midiTool;
    const useLintPage = !catalog && check.lintTool && Object.hasOwn(artifacts, "lint.html");
    const useStationPage = catalog && check.station922;
    const useAzdecryptPage = catalog && check.azdecrypt;
    const useBrpgPage = catalog && check.brpgMap;
    const useVisualfbPage = catalog && check.visualfbAlign;
    const useCompilerPage = catalog && check.compilerTool;
    const useCustomTool = useBindingPage || useConverterPage || useFormatPage || useImagePage || useToyPage ||
        useKiwiPage || useMidiPage || useStationPage || useAzdecryptPage || useBrpgPage || useVisualfbPage ||
        useCompilerPage;
    const useConsole = consoleLayout && !check.canvas && !check.terminalEditor && !useCustomTool;
    const viewport = useConsole || useCustomTool ? { width: 390, height: 844 } : check.viewport;
    const page = await browser.newPage(useConsole ? { viewport, isMobile: true, hasTouch: true } : { viewport });
    if (check.eqPlayback || check.phoenixGame || check.midiEditor) await page.addInitScript(observeBrowserAudio);
    if (check.phoenixGame || check.pyromaxGame) {
        // Keep WebGL's last presented frame readable for the sprite check.
        await page.addInitScript(() => {
            const open = HTMLCanvasElement.prototype.getContext;
            HTMLCanvasElement.prototype.getContext = function(type, options) {
                if (this.id === "canvas" && /^(?:experimental-)?webgl2?$/.test(type)) {
                    return open.call(this, type, { ...options, preserveDrawingBuffer: true });
                }
                return open.call(this, type, options);
            };
        });
    }
    const terminal = [];
    page.on("console", event => {
        const message = event.text();
        if (message.startsWith("FB:") && terminal.length < 2000) terminal.push(message.slice(3));
        if (check.standaloneShell && message.includes("Name: Interactive Testing Game")) terminal.push(message);
    });
    page.on("pageerror", error => row.errors.push(error.stack || error.message));
    page.on("dialog", async dialog => {
        if (useConsole || useCustomTool || (catalog && check.terminalEditor)) {
            row.errors.push("The text player opened a browser dialog");
            await dialog.dismiss();
            return;
        }
        const answer = check.answers?.[row.input.length];
        if (answer === undefined) {
            if (check.stopAtNextPrompt) {
                row.reachedPrompt = true;
                return;
            }
            row.errors.push("Program prompted for unexpected input");
            await page.close();
            return;
        }
        row.input.push(answer);
        await dialog.accept(answer).catch(error => row.errors.push(error.message));
    });
    try {
        const program = base + "/" + slug + "/";
        if (catalog) {
            // Dark Castle checks its files through Module.FS and reloads the player.
            // Exercise the shipped HTML on both visits without response rewriting.
            if (!useConsole && !useCustomTool && !check.standaloneShell && !check.terminalEditor &&
                    !check.darkCastleEditor && !check.darkCastleGame && !check.midiEditor) {
                await page.route(program + "play.html", async route => {
                    const response = await route.fetch();
                    await route.fulfill({ response, body: addHooks(await response.text(), {
                        observeLevel: check.pyromaxGame,
                    }) });
                });
            }
            await page.goto(program + "play.html", { timeout: 10000, waitUntil: "domcontentloaded" });
        } else {
            const shell = useLintPage ? "lint.html" : "index.html";
            await page.goto(program + shell, { timeout: 10000, waitUntil: "domcontentloaded" });
        }
        if (useLintPage) {
            await checkFblint(page, row);
            await page.screenshot({ path: path.join(output, "logs", slug + "-lint.png") });
        } else if (useStationPage) {
            await checkStation922(page, row, output);
            await page.screenshot({ path: path.join(output, "logs", slug + "-site-preview.png") });
        } else if (useBrpgPage) {
            await checkBrpgMap(page, row);
            await page.screenshot({ path: path.join(output, "logs", slug + "-map.png") });
        } else if (useVisualfbPage) {
            await checkVisualfbAlignment(page, row);
            await page.screenshot({ path: path.join(output, "logs", slug + "-alignment.png") });
        } else if (useCompilerPage) {
            if (slug === "lbasic-frontend") await checkLbasicParser(page, row, output);
            else await checkCompilerTool(page, row, output);
            await page.screenshot({ path: path.join(output, "logs", slug + "-compiler.png") });
        } else if (useAzdecryptPage) {
            await checkAzdecrypt(page, row);
            await page.screenshot({ path: path.join(output, "logs", slug + "-solver.png") });
        } else if (check.asciiBitmap && !catalog) {
            await checkAsciiBitmap(page, program, row);
        } else if (useBindingPage) {
            await checkBindings(page, row);
            await page.screenshot({ path: path.join(output, "logs", slug + "-bindings.png") });
        } else if (useConverterPage) {
            await checkConverter(page, row);
            await page.screenshot({ path: path.join(output, "logs", slug + "-converter.png") });
        } else if (useFormatPage) {
            await checkFormatTool(page, row);
            await page.screenshot({ path: path.join(output, "logs", slug + "-format.png") });
        } else if (useImagePage) {
            await checkImageTool(page, row);
            await page.screenshot({ path: path.join(output, "logs", slug + "-upload.png") });
        } else if (useToyPage) {
            await checkToyBASIC(page, row);
            await page.screenshot({ path: path.join(output, "logs", slug + "-playground.png") });
        } else if (useKiwiPage) {
            await checkKiwi(page, row);
            await page.screenshot({ path: path.join(output, "logs", slug + "-inspector.png") });
        } else if (useMidiPage) {
            await checkOpenSesh(page, row, output);
            await page.screenshot({ path: path.join(output, "logs", slug + "-converter.png") });
        } else if (catalog && check.midiEditor) {
            await checkOpenMidiEditor(page, row, output);
        } else if (useConsole) {
            await checkCatalogConsole(page, check, row);
            await page.screenshot({ path: path.join(output, "logs", slug + "-console.png") });
        } else if (check.terminalEditor) {
            if (check.terminalEditor === "hackit") await checkHackit(page, row, catalog);
            else await checkTerminalEditor(page, row, catalog);
            await page.screenshot({ path: path.join(output, "logs", slug + "-console.png") });
        } else if (check.canvas) {
            const after = await checkCanvas(page, check, row, output);
            if (check.standaloneShell && !check.prospectorGame && terminal.length === 0) {
                throw new Error("OHRRPGCE did not load the test RPG");
            }
            await writeFile(path.join(output, "logs", slug + (catalog ? "-live" : "") + ".png"), after.png);
            if (catalog && await page.locator("body.game-player").count()) {
                const measure = async () => {
                    const box = await page.locator("#canvas").boundingBox();
                    const { width, height } = page.viewportSize();
                    if (!box || box.x !== 0 || box.y !== 0 || box.width !== width || box.height !== height) {
                        throw new Error(`The canvas does not cover the ${width}x${height} browser viewport`);
                    }
                    return { width, height };
                };
                const desktop = await measure();
                await page.setViewportSize({ width: 390, height: 844 });
                row.gameViewport = { desktop, narrow: await measure() };
            } else if (catalog && check.phoenixGame) {
                await page.setViewportSize({ width: 390, height: 844 });
                row.phoenix.narrowLayout = await page.evaluate(() => ({
                    pageWidth: document.documentElement.scrollWidth,
                    canvasWidth: document.getElementById("canvas").getBoundingClientRect().width,
                }));
                if (row.phoenix.narrowLayout.pageWidth > 390 || row.phoenix.narrowLayout.canvasWidth > 390) {
                    throw new Error("The Phoenix page overflows a narrow viewport");
                }
            }
        } else {
            if (check.key) {
                const deadline = Date.now() + 7000;
                while (!terminal.join("").includes("END PARSING") && Date.now() < deadline) {
                    await new Promise(resolve => setTimeout(resolve, 50));
                }
                await page.keyboard.press(check.key);
            }
            if (check.stopAtNextPrompt) {
                const deadline = Date.now() + 10000;
                while (!row.reachedPrompt && !row.errors.length && Date.now() < deadline) {
                    await new Promise(resolve => setTimeout(resolve, 50));
                }
                if (!row.reachedPrompt) throw new Error("Did not reach the expected prompt");
            } else {
                await page.waitForFunction(() => Number.isInteger(window.corpusExit), null, { timeout: 12000 });
                row.exit = await page.evaluate(() => window.corpusExit);
                if (row.exit !== 0) throw new Error("Unexpected program exit " + row.exit);
            }
            for (const expected of check.output) {
                if (!expected.test(terminal.join(""))) throw new Error("Missing output " + expected);
            }
        }
        if (!row.errors.length) row.status = "passed";
    } catch (error) {
        row.errors.push(error.message);
        if ((check.editSample || check.terminalEditor || check.griffonGame || check.pyromaxGame ||
            check.mode7 || check.brpgMap || check.visualfbAlign || check.compilerTool ||
            check.darkCastleEditor || check.prospectorGame) && !page.isClosed()) {
            await page.screenshot({ path: path.join(output, "logs", slug + "-failed.png"), timeout: 3000 })
                .catch(() => {});
        }
    } finally {
        if (!useConsole && !useCustomTool && !check.terminalEditor) row.output = terminal.join("").slice(0, 12000);
        await page.close();
    }
    return row;
}

async function main() {
    const output = path.resolve(process.argv[2]);
    const only = process.argv.slice(3);
    const catalog = process.env.FBC_CORPUS_SITE?.replace(/\/$/, "");
    const consoleLayout = !!catalog && process.env.FBC_CORPUS_CONSOLE === "1";
    if (catalog && !only.length) throw new Error("Specify the programs included in the deployed catalog");
    const receipts = JSON.parse(await readFile(path.join(output, "build-results.json"), "utf8"));
    const builds = new Map(receipts.filter(row => row.status === "linked" && Object.hasOwn(checks, row.slug) &&
        (!only.length || only.includes(row.slug))).map(row => [row.slug, row]));
    if (!builds.size || (only.length && only.some(slug => !builds.has(slug)))) {
        throw new Error("Each selected program needs a browser check and a successful build");
    }
    await mkdir(path.join(output, "logs"), { recursive: true });
    const server = createServer((request, response) => {
        serve(request, response, output, builds).catch(error => {
            console.error(error.message);
            response.writeHead(500).end();
        });
    });
    await new Promise((resolve, reject) => {
        server.once("error", reject);
        server.listen(0, "127.0.0.1", resolve);
    });
    let browser;
    const rows = [];
    try {
        browser = await chromium.launch({
            executablePath: process.env.FBC_BROWSER,
            args: ["--use-angle=swiftshader", "--enable-unsafe-swiftshader"],
        });
        const base = catalog || "http://127.0.0.1:" + server.address().port;
        for (const build of builds.values()) {
            const row = await runCheck(browser, base, output, build, catalog, consoleLayout);
            console.log(build.slug, row.status, row.errors.map(error => error.split("\n")[0]).join("; "));
            rows.push(row);
        }
        // Save completed checks before waiting for the browser subprocess
        // to exit. On Windows, shutdown can stall after the last page closes.
        const resultName = !catalog ? "browser-results.json" :
            new URL(catalog).hostname === "127.0.0.1" ? "preview-browser-results.json" : "live-browser-results.json";
        const results = path.join(output, resultName);
        const previous = only.length ? JSON.parse(await readFile(results, "utf8").catch(error => {
            if (error.code !== "ENOENT") throw error;
            return "[]";
        })) : [];
        rows.push(...previous.filter(row => !builds.has(row.slug)));
        await writeFile(results, JSON.stringify(rows, null, 2) + "\n");
        if (rows.some(row => builds.has(row.slug) && row.status !== "passed")) process.exitCode = 1;
    } finally {
        server.close();
        if (browser) await browser.close();
    }
}

main().catch(error => { console.error(error); process.exitCode = 1; });

/* end of fb-corpus-js-check.cjs */

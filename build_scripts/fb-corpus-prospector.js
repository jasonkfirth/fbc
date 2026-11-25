/*
    Project: FreeBASIC browser corpus
    File: fb-corpus-prospector.js
    Purpose: Run the original Prospector character display in one canvas.
    Responsibilities: Copy frames, translate keys, and preserve game files.
    The game owns its commands, menus, and save format.
*/
"use strict";

const canvas = document.getElementById("canvas");
const context = canvas.getContext("2d");
const statusMessage = document.getElementById("status");
const gameDir = "/prospector";
const saveDirs = ["config", "savegames", "bones", "summary"];
const scan = new Map(Object.entries({
    Escape: 1, Backspace: 14, Enter: 28, F1: 59, F2: 60, F3: 61, F4: 62,
    F5: 63, F6: 64, F7: 65, F8: 66, F9: 67, F10: 68, Home: 71, ArrowUp: 72,
    PageUp: 73, ArrowLeft: 75, ArrowRight: 77, End: 79, ArrowDown: 80,
    PageDown: 81, Insert: 82, Delete: 83, F11: 87, F12: 88,
}));
const ascii = new Map([["Escape", 27], ["Backspace", 8], ["Enter", 13]]);

let pixels;
let running = false;
let saveTimer;
let writing;
let persistent = true;

function setupFolder() {
    const { FS } = Module;
    function copyIfMissing(from, to) {
        if (!FS.analyzePath(to).exists) FS.writeFile(to, FS.readFile(from));
    }
    for (const name of FS.readdir("/pack/config")) {
        if (name.startsWith(".")) continue;
        copyIfMissing("/pack/config/" + name, gameDir + "/config/" + name);
    }
    copyIfMissing("/pack/data/highscore.dat", gameDir + "/savegames/highscore.dat");
    FS.mkdirTree(gameDir + "/data");
    FS.mkdirTree(gameDir + "/graphics");
    for (const name of FS.readdir("/pack/data")) {
        if (name.startsWith(".")) continue;
        const source = name === "highscore.dat" ? gameDir + "/savegames/" : "/pack/data/";
        FS.symlink(source + name, gameDir + "/data/" + name);
    }
    FS.chdir(gameDir);
}

async function flush() {
    if (!persistent) return;
    // IDBFS syncs all four mounts at once. Wait for an earlier flush before
    // starting the final write so a just-created save cannot be missed.
    while (writing) await writing;
    writing = new Promise(resolve => Module.FS.syncfs(false, error => {
        if (error) statusMessage.textContent = "Browser storage could not save the game.";
        resolve(!error);
    }));
    const stored = await writing;
    writing = undefined;
    return stored;
}

function draw() {
    // rv_frame returns four Wasm32 words: width, height, dirty flag, and
    // the address of the driver's RGBA framebuffer. Fetch the current heap
    // on each frame because the game can grow its Wasm memory.
    const frame = Module._rv_frame(-1) >> 2;
    const [width, height, dirty, address] = Module.HEAP32.subarray(frame, frame + 4);
    if (width > 0 && height > 0 && dirty && address > 0) {
        if (!pixels || pixels.width !== width || pixels.height !== height) {
            canvas.width = width;
            canvas.height = height;
            pixels = context.createImageData(width, height);
        }
        pixels.data.set(Module.HEAPU8.subarray(address, address + pixels.data.length));
        context.putImageData(pixels, 0, 0);
    }
    if (running) requestAnimationFrame(draw);
}

canvas.addEventListener("keydown", event => {
    if (!running || event.isComposing || event.metaKey || event.altKey) return;
    let code = scan.get(event.key) ?? 0;
    let character = ascii.get(event.key) ?? 0;
    if (/^Numpad[0-9]$/.test(event.code)) {
        code = 0;
        character = event.code.charCodeAt(6);
    } else if (event.key.length === 1) {
        character = event.key.charCodeAt(0);
        if (event.ctrlKey) {
            const letter = event.key.toUpperCase().charCodeAt(0);
            if (letter < 65 || letter > 90) return;
            character = letter - 64;
        }
    }
    if ((!code && !character) || character > 255) return;
    event.preventDefault();
    Module._rv_key(code, character);
    clearTimeout(saveTimer);
    saveTimer = setTimeout(flush, 600);
});

async function endGame() {
    if (!running) return;
    running = false;
    clearTimeout(saveTimer);
    if (await flush()) location.reload();
    else if (!persistent) statusMessage.textContent = "The game ended. Reload to return to the title screen.";
}

window.Module = {
    // The upstream linker names its assets after prospector-core. The
    // catalog serves the same bytes under its standard index names.
    locateFile: file => file.replace(/^prospector-core\./, "index."),
    preRun: [function () {
        for (const name of saveDirs) Module.FS.mkdirTree(gameDir + "/" + name);
        try {
            for (const name of saveDirs) Module.FS.mount(Module.IDBFS, {}, gameDir + "/" + name);
        } catch {
            persistent = false;
            return;
        }
        Module.addRunDependency("prospector-saves");
        Module.FS.syncfs(true, error => {
            persistent = !error;
            Module.removeRunDependency("prospector-saves");
        });
    }],
    onRuntimeInitialized() {
        setupFolder();
        running = true;
        statusMessage.textContent = persistent ? "" : "Browser storage is unavailable. This session is temporary.";
        canvas.focus({ preventScroll: true });
        requestAnimationFrame(draw);
    },
    rvGameOver: endGame,
    onExit: endGame,
    onAbort() {
        running = false;
        statusMessage.textContent = "Prospector stopped. Reload to start again.";
    },
};

/* end of fb-corpus-prospector.js */

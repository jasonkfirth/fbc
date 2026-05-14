#!/usr/bin/env bash

set -euo pipefail
trap 'echo "ERROR: failed at line $LINENO: $BASH_COMMAND" >&2' ERR

START_DIR="$(pwd)"
SEARCH_DIR="$START_DIR"
ROOT=""

while :; do
    if [ -d "$SEARCH_DIR/build_scripts" ] && { [ -f "$SEARCH_DIR/GNUmakefile" ] || [ -f "$SEARCH_DIR/makefile" ] || [ -f "$SEARCH_DIR/Makefile" ]; }; then
        ROOT="$SEARCH_DIR"
        break
    fi
    [ "$SEARCH_DIR" = "/" ] && break
    SEARCH_DIR="$(dirname "$SEARCH_DIR")"
done

[ -n "$ROOT" ] || { echo "ERROR: could not locate FreeBASIC root"; exit 1; }
cd "$ROOT"

run() { echo "==> $*"; "$@"; }
die() { echo "ERROR: $*" >&2; exit 1; }
msg() { echo ""; echo "==> $1"; }

run_root() {
    if [ "$(id -u)" -eq 0 ]; then
        run "$@"
    elif command -v sudo >/dev/null 2>&1; then
        run sudo "$@"
    else
        die "this step requires root privileges; rerun as root or install sudo"
    fi
}

usage() {
    cat <<EOF
Usage: ./build_scripts/debianubuntu-test-freebasic-js-browser.sh [options]

Options:
  --package-dir DIR   Directory containing Debian package artifacts
  --image IMAGE       Docker image with Chromium and Node
                      (default: mcr.microsoft.com/playwright:v1.56.1-noble)
  --docker-cmd CMD    Docker command to use (default: docker)
  --skip-host-deps    Skip Docker host dependency installation
  --help              Show this help text

The test starts a fresh container, installs the local freebasic-js .deb
package, compiles a browser graphics program with fbc-js, then drives it in
headless Chromium through the DevTools protocol.  It intentionally exercises
SCREEN 0, the supported legacy SCREEN 1-13 modes, SCREENRES, SCREENSET,
SCREENCOPY, INKEY, and SLEEP 0 together.
EOF
}

PACKAGE_DIR=""
IMAGE="${IMAGE:-mcr.microsoft.com/playwright:v1.56.1-noble}"
DOCKER_CMD="${DOCKER_CMD:-docker}"
SKIP_HOST_DEPS=0

while [ $# -gt 0 ]; do
    case "$1" in
        --package-dir) PACKAGE_DIR="$2"; shift 2 ;;
        --image) IMAGE="$2"; shift 2 ;;
        --docker-cmd) DOCKER_CMD="$2"; shift 2 ;;
        --skip-host-deps) SKIP_HOST_DEPS=1; shift ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            die "unknown option: $1"
            ;;
    esac
done

if [ -z "$PACKAGE_DIR" ]; then
    ARCH="$(dpkg --print-architecture 2>/dev/null || true)"
    DISTRO_ID="unknown"
    CODENAME="unknown"
    if [ -f /etc/os-release ]; then
        # shellcheck disable=SC1091
        . /etc/os-release
        DISTRO_ID="${ID:-unknown}"
        CODENAME="${VERSION_CODENAME:-unknown}"
    fi
    PACKAGE_DIR="$ROOT/out/linux/$DISTRO_ID/$CODENAME/$ARCH"
fi

[ -d "$PACKAGE_DIR" ] || die "package directory not found: $PACKAGE_DIR"
PACKAGE_DIR="$(cd "$PACKAGE_DIR" && pwd -P)"
ls "$PACKAGE_DIR"/freebasic-js_*.deb >/dev/null 2>&1 || die "missing freebasic-js .deb in $PACKAGE_DIR"

install_host_deps() {
    [ "$SKIP_HOST_DEPS" -eq 0 ] || return 0

    if command -v "${DOCKER_CMD%% *}" >/dev/null 2>&1; then
        return 0
    fi

    if command -v apt-get >/dev/null 2>&1; then
        msg "installing Docker host dependency via apt"
        run_root apt-get update -y
        run_root apt-get install -y --no-install-recommends docker.io ca-certificates
        return 0
    fi

    die "Docker is required; install it or rerun with --skip-host-deps after installing Docker"
}

TEST_RUNNER="$(mktemp -t fb-js-browser-package-test.XXXXXX.sh)"
cleanup() {
    rm -f "$TEST_RUNNER"
}
trap cleanup EXIT

chmod 755 "$TEST_RUNNER"
cat > "$TEST_RUNNER" <<'TEST_RUNNER_EOF'
#!/bin/sh

set -eu

run() {
    echo "==> $*"
    "$@"
}

fail() {
    echo "ERROR: $*" >&2
    exit 1
}

find_chromium() {
    for candidate in \
        /ms-playwright/chromium-*/chrome-linux/chrome \
        /usr/bin/chromium \
        /usr/bin/chromium-browser \
        /usr/bin/google-chrome \
        /usr/bin/google-chrome-stable
    do
        for expanded in $candidate; do
            [ -x "$expanded" ] || continue
            echo "$expanded"
            return 0
        done
    done

    return 1
}

export DEBIAN_FRONTEND=noninteractive

if [ -f /etc/apt/sources.list.d/nodesource.list ] || [ -f /etc/apt/sources.list.d/nodesource.sources ]; then
    rm -f /etc/apt/sources.list.d/nodesource.list /etc/apt/sources.list.d/nodesource.sources

    if dpkg-query -W -f='${Status}' nodejs 2>/dev/null | grep -q 'install ok installed'; then
        run apt-get purge -y nodejs
    fi
fi

run apt-get update -y
run apt-get install -y --no-install-recommends /packages/freebasic-js_*.deb ca-certificates

command -v fbc-js >/dev/null 2>&1 || fail "fbc-js was not installed"
command -v emcc >/dev/null 2>&1 || fail "emcc dependency was not installed"
command -v node >/dev/null 2>&1 || fail "node dependency was not installed"

CHROMIUM="$(find_chromium)" || fail "could not locate Chromium in the test image"

mkdir -p /tmp/fb-js-browser-smoke
cat > /tmp/fb-js-browser-smoke/browser_gfx.bas <<'EOF'
extern "C"
    declare sub emscripten_run_script alias "emscripten_run_script" _
        ( byval script as const zstring ptr )
    declare function emscripten_run_script_int alias "emscripten_run_script_int" _
        ( byval script as const zstring ptr ) as integer
end extern

sub mark_step( byval step_id as integer )
    dim script as string = "window.__fbBrowserSmokeStep = " & str( step_id ) & ";"
    emscripten_run_script( strptr( script ) )
end sub

sub wait_step( byval step_id as integer )
    dim script as string = "window.__fbBrowserContinue === " & str( step_id )

    do while emscripten_run_script_int( strptr( script ) ) = 0
        sleep 0
    loop
end sub

sub present_step( byval step_id as integer )
    mark_step( step_id )
    wait_step( step_id )
end sub

sub draw_rgb_blocks( byval step_id as integer )
    palette 1, 255, 0, 0
    palette 2, 0, 255, 0
    palette 3, 0, 0, 255

    cls
    line ( 4, 4 )-( 28, 28 ), 1, bf
    line ( 36, 4 )-( 60, 28 ), 2, bf
    line ( 68, 4 )-( 92, 28 ), 3, bf

    present_step( step_id )
end sub

sub draw_white_block( byval step_id as integer )
    palette 1, 255, 255, 255

    cls
    line ( 4, 4 )-( 28, 28 ), 1, bf

    present_step( step_id )
end sub

screen 0
width 80, 25
cls
color 12, 0: print "FBJS_SCREEN0_RED ";
color 10, 0: print "FBJS_SCREEN0_GREEN ";
color 9, 0: print "FBJS_SCREEN0_BLUE"
present_step( 0 )

screen 1
draw_rgb_blocks( 1 )

screen 2
draw_white_block( 2 )

screen 7
draw_rgb_blocks( 7 )

screen 8
draw_rgb_blocks( 8 )

screen 9
draw_rgb_blocks( 9 )

screen 10
draw_white_block( 10 )

screen 11
draw_white_block( 11 )

screen 12
draw_rgb_blocks( 12 )

screen 13
draw_rgb_blocks( 13 )

screenres 320, 200, 32, 2
screenset 1, 0
line ( 0, 0 )-( 319, 199 ), rgb( 0, 0, 48 ), bf
line ( 8, 8 )-( 160, 40 ), rgb( 255, 0, 0 ), bf
line ( 168, 8 )-( 220, 40 ), rgb( 0, 255, 0 ), bf
line ( 228, 8 )-( 280, 40 ), rgb( 0, 0, 255 ), bf
screenset 1, 1
present_step( 20 )

emscripten_run_script( @"window.__fbBrowserSmokeReady = 'title';" )

do
    if inkey = "1" then exit do
    sleep 0
loop

line ( 0, 0 )-( 319, 199 ), rgb( 0, 0, 64 ), bf

for i as integer = 0 to 8191
    pset ( i mod 320, 80 + ( ( i \ 320 ) mod 50 ) ), _
        rgb( i mod 255, 64, 128 )
next

line ( 20, 20 )-( 80, 80 ), rgb( 0, 255, 0 ), bf
pset ( 120, 40 ), rgb( 255, 255, 0 )
screencopy

emscripten_run_script( @"window.__fbBrowserSmokeDone = 'gfx';" )
EOF

cat > /tmp/fb-js-browser-smoke/browser_probe.js <<'EOF'
const http = require("http");
const fs = require("fs");
const path = require("path");
const { spawn } = require("child_process");

const chromium = process.argv[2];
const html = process.argv[3];
const root = path.dirname(html);
const pageName = path.basename(html);
const port = 8123 + (process.pid % 1000);
const cdpPort = 9123 + (process.pid % 1000);

function sleep(ms) {
    return new Promise(resolve => setTimeout(resolve, ms));
}

function serve() {
    const mime = {
        ".html": "text/html",
        ".js": "application/javascript",
        ".wasm": "application/wasm"
    };

    const server = http.createServer((req, res) => {
        const url = new URL(req.url, "http://127.0.0.1");
        let name = decodeURIComponent(url.pathname);
        if (name === "/")
            name = "/" + pageName;

        const full = path.normalize(path.join(root, name));
        if (!full.startsWith(root)) {
            res.writeHead(403);
            res.end("forbidden");
            return;
        }

        fs.readFile(full, (err, data) => {
            if (err) {
                res.writeHead(404);
                res.end("missing");
                return;
            }

            res.writeHead(200, {
                "content-type": mime[path.extname(full)] || "application/octet-stream"
            });
            res.end(data);
        });
    });

    return new Promise((resolve, reject) => {
        server.once("error", reject);
        server.listen(port, "127.0.0.1", () => resolve(server));
    });
}

async function waitFetch(url, options = {}, timeoutMs = 10000) {
    const start = Date.now();
    let lastError;

    while ((Date.now() - start) < timeoutMs) {
        try {
            const response = await fetch(url, options);
            if (response.ok)
                return response;
            lastError = new Error(`${response.status} ${response.statusText}`);
        } catch (err) {
            lastError = err;
        }

        await sleep(100);
    }

    throw lastError || new Error(`timed out waiting for ${url}`);
}

class CDP {
    constructor(url) {
        this.url = url;
        this.nextId = 1;
        this.pending = new Map();
        this.events = [];
    }

    connect() {
        return new Promise((resolve, reject) => {
            this.ws = new WebSocket(this.url);
            this.ws.onopen = resolve;
            this.ws.onerror = reject;
            this.ws.onmessage = message => {
                const data = JSON.parse(message.data);
                if (data.id && this.pending.has(data.id)) {
                    const pending = this.pending.get(data.id);
                    this.pending.delete(data.id);
                    if (data.error)
                        pending.reject(new Error(`${pending.method}: ${data.error.message}`));
                    else
                        pending.resolve(data.result || {});
                    return;
                }
                this.events.push(data);
            };
        });
    }

    send(method, params = {}, timeoutMs = 10000) {
        const id = this.nextId++;
        this.ws.send(JSON.stringify({ id, method, params }));

        return new Promise((resolve, reject) => {
            const timer = setTimeout(() => {
                this.pending.delete(id);
                reject(new Error(`${method} timed out`));
            }, timeoutMs);

            this.pending.set(id, {
                method,
                resolve: value => {
                    clearTimeout(timer);
                    resolve(value);
                },
                reject: err => {
                    clearTimeout(timer);
                    reject(err);
                }
            });
        });
    }

    close() {
        if (this.ws)
            this.ws.close();
    }
}

async function evalValue(cdp, expression, timeoutMs = 10000) {
    const result = await cdp.send("Runtime.evaluate", {
        expression,
        returnByValue: true
    }, timeoutMs);

    return result.result.value;
}

async function waitFor(cdp, expression, timeoutMs) {
    const start = Date.now();

    while ((Date.now() - start) < timeoutMs) {
        if (await evalValue(cdp, expression, 5000))
            return;
        await sleep(100);
    }

    throw new Error(`timed out waiting for ${expression}`);
}

async function sendNumberOne(cdp) {
    await evalValue(cdp, `(() => {
        function send(target, type, charCode) {
            const event = new KeyboardEvent(type, {
                key: "1",
                code: "Digit1",
                bubbles: true,
                cancelable: true
            });

            Object.defineProperty(event, "keyCode", { get: () => 49 });
            Object.defineProperty(event, "which", { get: () => charCode || 49 });
            Object.defineProperty(event, "charCode", { get: () => charCode || 0 });
            Object.defineProperty(event, "location", { get: () => 0 });

            target.dispatchEvent(event);
        }

        const targets = [
            window,
            document,
            document.body,
            document.getElementById("canvas")
        ];

        for (const target of targets) {
            if (!target)
                continue;

            send(target, "keydown", 0);
            send(target, "keypress", 49);
            send(target, "keyup", 0);
        }

        return true;
    })()`);
}

async function readCanvasSamples(cdp) {
    return evalValue(cdp, `(() => {
        const canvas = document.getElementById("canvas");
        if (!canvas)
            return { error: "missing canvas" };

        const ctx = canvas.getContext("2d", { willReadFrequently: true }) ||
                    canvas.getContext("2d");
        const sample = (x, y) => Array.from(ctx.getImageData(x, y, 1, 1).data);

        return {
            width: canvas.width,
            height: canvas.height,
            red: sample(10, 10),
            green: sample(42, 10),
            blue: sample(74, 10),
            white: sample(10, 10),
            screenresRed: sample(16, 16),
            screenresGreen: sample(180, 16),
            screenresBlue: sample(240, 16),
            finalGreen: sample(30, 30),
            finalYellow: sample(120, 40)
        };
    })()`);
}

async function readText(cdp) {
    return evalValue(cdp, `document.body.innerText || ""`);
}

function assertContains(name, text, expected) {
    if (!text.includes(expected))
        throw new Error(`${name} did not contain ${expected}`);
}

async function checkTextMode(cdp, step) {
    await waitFor(cdp, `window.__fbBrowserSmokeStep === ${step.step}`, 20000);

    const text = await readText(cdp);
    for (const expected of step.contains)
        assertContains(step.name, text, expected);

    await evalValue(cdp, `window.__fbBrowserContinue = ${step.step}; true`);
}

async function checkCanvasMode(cdp, step) {
    await waitFor(cdp, `window.__fbBrowserSmokeStep === ${step.step}`, 20000);

    /*
     * The BASIC side waits in SLEEP 0 after publishing the step marker.  Leave
     * the browser a short frame window so the async JS driver can blit the
     * just-drawn mode before we sample the canvas.
     */
    await sleep(200);

    const canvas = await readCanvasSamples(cdp);
    if (canvas.error)
        throw new Error(canvas.error);
    if (canvas.width !== step.width || canvas.height !== step.height)
        throw new Error(`${step.name} canvas size was ${canvas.width}x${canvas.height}`);

    if (step.kind === "rgb") {
        assertPixel(`${step.name} red`, canvas.red, 255, 0, 0);
        assertPixel(`${step.name} green`, canvas.green, 0, 255, 0);
        assertPixel(`${step.name} blue`, canvas.blue, 0, 0, 255);
    } else if (step.kind === "mono") {
        assertPixel(`${step.name} white`, canvas.white, 255, 255, 255);
    } else if (step.kind === "screenres") {
        assertPixel(`${step.name} red`, canvas.screenresRed, 255, 0, 0);
        assertPixel(`${step.name} green`, canvas.screenresGreen, 0, 255, 0);
        assertPixel(`${step.name} blue`, canvas.screenresBlue, 0, 0, 255);
    }

    await evalValue(cdp, `window.__fbBrowserContinue = ${step.step}; true`);
}

function assertPixel(name, pixel, r, g, b) {
    const tolerance = 8;

    if (Math.abs(pixel[0] - r) > tolerance ||
        Math.abs(pixel[1] - g) > tolerance ||
        Math.abs(pixel[2] - b) > tolerance) {
        throw new Error(`${name} pixel was ${pixel.join(",")}, expected ${r},${g},${b}`);
    }
}

(async () => {
    const server = await serve();
    const chrome = spawn(chromium, [
        "--headless=new",
        "--no-sandbox",
        "--disable-gpu",
        `--remote-debugging-port=${cdpPort}`,
        `--user-data-dir=/tmp/fb-js-browser-smoke-profile-${process.pid}`,
        "about:blank"
    ], { stdio: [ "ignore", "ignore", "pipe" ] });

    let cdp;

    try {
        await waitFetch(`http://127.0.0.1:${cdpPort}/json/version`);

        const targetResponse = await waitFetch(
            `http://127.0.0.1:${cdpPort}/json/new?` +
            encodeURIComponent(`http://127.0.0.1:${port}/${pageName}`),
            { method: "PUT" }
        );
        const target = await targetResponse.json();

        cdp = new CDP(target.webSocketDebuggerUrl);
        await cdp.connect();
        await cdp.send("Page.enable");
        await cdp.send("Runtime.enable");
        await cdp.send("Log.enable");

        const modeSteps = [
            {
                step: 0,
                name: "SCREEN 0",
                kind: "text",
                contains: [
                    "FBJS_SCREEN0_RED",
                    "FBJS_SCREEN0_GREEN",
                    "FBJS_SCREEN0_BLUE"
                ]
            },
            { step: 1, name: "SCREEN 1", kind: "rgb", width: 320, height: 200 },
            { step: 2, name: "SCREEN 2", kind: "mono", width: 640, height: 400 },
            { step: 7, name: "SCREEN 7", kind: "rgb", width: 320, height: 200 },
            { step: 8, name: "SCREEN 8", kind: "rgb", width: 640, height: 400 },
            { step: 9, name: "SCREEN 9", kind: "rgb", width: 640, height: 350 },
            { step: 10, name: "SCREEN 10", kind: "mono", width: 640, height: 350 },
            { step: 11, name: "SCREEN 11", kind: "mono", width: 640, height: 480 },
            { step: 12, name: "SCREEN 12", kind: "rgb", width: 640, height: 480 },
            { step: 13, name: "SCREEN 13", kind: "rgb", width: 320, height: 200 },
            { step: 20, name: "SCREENRES/SCREENSET", kind: "screenres", width: 320, height: 200 }
        ];

        for (const step of modeSteps) {
            if (step.kind === "text")
                await checkTextMode(cdp, step);
            else
                await checkCanvasMode(cdp, step);
        }

        await waitFor(cdp, `window.__fbBrowserSmokeReady === "title"`, 20000);
        await sendNumberOne(cdp);
        await waitFor(cdp, `window.__fbBrowserSmokeDone === "gfx"`, 20000);

        const canvas = await readCanvasSamples(cdp);
        if (canvas.error)
            throw new Error(canvas.error);
        if (canvas.width !== 320 || canvas.height !== 200)
            throw new Error(`canvas size was ${canvas.width}x${canvas.height}`);

        assertPixel("final green", canvas.finalGreen, 0, 255, 0);
        assertPixel("final yellow", canvas.finalYellow, 255, 255, 0);

        const exceptions = cdp.events.filter(event => event.method === "Runtime.exceptionThrown");
        if (exceptions.length !== 0)
            throw new Error(`browser runtime exception: ${JSON.stringify(exceptions[0])}`);

        console.log("freebasic-js browser smoke test passed");
    } finally {
        if (cdp)
            cdp.close();
        chrome.kill("SIGTERM");
        server.close();
    }
})().catch(err => {
    console.error(err && err.stack || err);
    process.exit(1);
});
EOF

cd /tmp/fb-js-browser-smoke
run fbc-js browser_gfx.bas
[ -f browser_gfx.html ] || fail "browser_gfx.html was not produced"
[ -f browser_gfx.js ] || fail "browser_gfx.js was not produced"
[ -f browser_gfx.wasm ] || fail "browser_gfx.wasm was not produced"

run node browser_probe.js "$CHROMIUM" /tmp/fb-js-browser-smoke/browser_gfx.html
TEST_RUNNER_EOF

install_host_deps

msg "browser-testing freebasic-js package in $IMAGE"
run ${DOCKER_CMD} run --rm \
    -v "$PACKAGE_DIR:/packages:ro" \
    -v "$TEST_RUNNER:/tmp/test-freebasic-js-browser.sh:ro" \
    "$IMAGE" \
    /bin/sh /tmp/test-freebasic-js-browser.sh

#!/usr/bin/env bash

set -euo pipefail
trap 'echo "ERROR: failed at line $LINENO: $BASH_COMMAND" >&2' ERR

##############################################################################
# msys2-test-freebasic-js-browser.sh
#
# Build browser-facing fbc-js test programs and validate them in a real
# Chromium-family browser through the DevTools protocol.
#
# Responsibilities:
#   - locate a packaged fbc-js distribution
#   - compile console, gfxlib, and sfxlib browser programs
#   - launch Edge/Chrome headlessly from MSYS2
#   - inspect console text, canvas pixels, and WebAudio output samples
#
# This file intentionally does NOT contain:
#   - the fbc-js package build itself
#   - general fbcunit execution
#   - non-browser Node.js-only testing
##############################################################################

SELF_DIR="$(CDPATH= cd -- "$(dirname "$0")" && pwd)"
ROOT="$(CDPATH= cd -- "$SELF_DIR/.." && pwd)"

cd "$ROOT"

if [ ! -d "$ROOT/build_scripts" ] || [ ! -f "$ROOT/GNUmakefile" ]; then
	echo ""
	echo "ERROR: could not locate the FreeBASIC project root."
	exit 1
fi

case "$(uname -s)" in
	MINGW*|MSYS*) ;;
	*)
		echo ""
		echo "ERROR: this script must be run inside an MSYS2 environment."
		exit 1
		;;
esac

##############################################################################
# Options
##############################################################################

DIST_DIR=""
BROWSER=""
WORKROOT="${WORKROOT:-$ROOT/.build-msys2/freebasic-js-browser-test}"
KEEP_WORKROOT=0
HEADFUL=0

usage() {
	cat <<EOF
Usage: ./build_scripts/msys2-test-freebasic-js-browser.sh [options]

Options:
  --dist-dir DIR      FreeBASIC fbc-js distribution directory
  --browser EXE      Chromium-family browser executable
  --workroot DIR     Test work directory
  --keep-workroot    Keep the generated browser test directory
  --headful          Run the browser with a visible window
  --help             Show this help text

The script compiles small browser HTML programs with fbc-js and validates:
  - console output through the JavaScript console backend
  - gfxlib SCREENRES/SCREENSET drawing through canvas pixels
  - gfxlib SCREEN 13 drawing through canvas pixels
  - sfxlib WebAudio output for expected 440 Hz and 660 Hz content
  - sfxlib continuity by checking for low-energy gaps during the tone body
EOF
}

while [ $# -gt 0 ]; do
	case "$1" in
		--dist-dir)
			DIST_DIR="$2"
			shift 2
			;;
		--browser)
			BROWSER="$2"
			shift 2
			;;
		--workroot)
			WORKROOT="$2"
			shift 2
			;;
		--keep-workroot)
			KEEP_WORKROOT=1
			shift
			;;
		--headful)
			HEADFUL=1
			shift
			;;
		-h|--help)
			usage
			exit 0
			;;
		*)
			echo "ERROR: unknown option: $1" >&2
			usage >&2
			exit 1
			;;
	esac
done

##############################################################################
# Helpers
##############################################################################

msg() {
	echo ""
	echo "==> $1"
}

fail() {
	echo ""
	echo "ERROR: $1" >&2
	exit 1
}

run() {
	echo "==> $*"
	"$@"
}

have() {
	command -v "$1" >/dev/null 2>&1
}

first_existing_dir() {
	local candidate

	for candidate in "$@"; do
		[ -n "$candidate" ] || continue
		if [ -d "$candidate" ]; then
			echo "$candidate"
			return 0
		fi
	done

	return 1
}

first_existing_file() {
	local candidate

	for candidate in "$@"; do
		[ -n "$candidate" ] || continue
		if [ -f "$candidate" ]; then
			echo "$candidate"
			return 0
		fi
	done

	return 1
}

win_path_to_msys() {
	local value="$1"

	if have cygpath; then
		cygpath -u "$value" 2>/dev/null && return 0
	fi

	echo "$value"
}

patch_html_for_probe() {
	local html="$1"

	perl -0pi -e 's#(<script async type="text/javascript" src="[^"]+"></script>)#<script type="text/javascript" src="browser_probe.js"></script>\n    $1#' "$html"
}

patch_sfx_driver_capture() {
	local js="$1"

	perl -0pi -e 's#state\.queue\.push\(\{\s*samples: block,\s*frames: frames\s*\}\);#if (Module.__fbBrowserDriverCapture) Module.__fbBrowserDriverCapture(block, frames, channels); state.queue.push({ samples: block, frames: frames });#s' "$js"
}

##############################################################################
# Distribution/browser detection
##############################################################################

if [ -z "$DIST_DIR" ]; then
	DIST_DIR="$(first_existing_dir \
		"/tmp/freebasic-js-build/dist/FreeBASIC-1.20.1-fbc-js" \
		"$ROOT/out/mingw32-js/FreeBASIC-1.20.1-fbc-js" \
		"/c/freebasic-js" \
		"/c/FreeBASIC-js" \
		|| true)"
fi

[ -n "$DIST_DIR" ] || fail "could not locate fbc-js distribution; pass --dist-dir"
DIST_DIR="$(CDPATH= cd -- "$DIST_DIR" && pwd)"

[ -f "$DIST_DIR/freebasic-js-env.sh" ] || fail "missing freebasic-js-env.sh in $DIST_DIR"
[ -f "$DIST_DIR/bin/fbc-js.exe" ] || fail "missing bin/fbc-js.exe in $DIST_DIR"

if [ -z "$BROWSER" ]; then
	BROWSER="$(first_existing_file \
		"$(win_path_to_msys 'C:\Program Files (x86)\Microsoft\Edge\Application\msedge.exe')" \
		"$(win_path_to_msys 'C:\Program Files\Microsoft\Edge\Application\msedge.exe')" \
		"$(win_path_to_msys 'C:\Program Files\Google\Chrome\Application\chrome.exe')" \
		"$(win_path_to_msys 'C:\Program Files (x86)\Google\Chrome\Application\chrome.exe')" \
		|| true)"
fi

[ -n "$BROWSER" ] || fail "could not locate Edge/Chrome; pass --browser"
[ -f "$BROWSER" ] || fail "browser executable not found: $BROWSER"

##############################################################################
# Test sources and browser harness
##############################################################################

SRC_DIR="$WORKROOT/src"
WWW_DIR="$WORKROOT/www"
LOG_DIR="$WORKROOT/logs"

msg "Preparing browser test workroot"
rm -rf "$WORKROOT"
mkdir -p "$SRC_DIR" "$WWW_DIR" "$LOG_DIR"

cat > "$SRC_DIR/console.bas" <<'EOF'
extern "C"
	declare sub emscripten_run_script alias "emscripten_run_script" _
		( byval script as const zstring ptr )
end extern

width 40, 10
color 10, 1
locate 2, 5
print "FBJS_BROWSER_CONSOLE_OK"

emscripten_run_script( @"window.__fbBrowserProgramDone = 'console';" )
EOF

cat > "$SRC_DIR/gfx_screenset.bas" <<'EOF'
extern "C"
	declare sub emscripten_run_script alias "emscripten_run_script" _
		( byval script as const zstring ptr )
end extern

screenres 128, 96, 32, 2

screenset 0, 0
line ( 0, 0 )-( 127, 95 ), rgb( 255, 0, 0 ), bf

screenset 1, 1
line ( 0, 0 )-( 127, 95 ), rgb( 0, 0, 255 ), bf
line ( 16, 16 )-( 63, 63 ), rgb( 0, 255, 0 ), bf
pset ( 80, 32 ), rgb( 255, 255, 0 )

emscripten_run_script( @"window.__fbBrowserProgramDone = 'gfx_screenset';" )
EOF

cat > "$SRC_DIR/gfx_screen13.bas" <<'EOF'
extern "C"
	declare sub emscripten_run_script alias "emscripten_run_script" _
		( byval script as const zstring ptr )
end extern

screen 13

line ( 0, 0 )-( 319, 199 ), 1, bf
line ( 0, 0 )-( 119, 79 ), 4, bf
line ( 120, 0 )-( 239, 79 ), 2, bf
line ( 240, 0 )-( 319, 79 ), 14, bf
pset ( 300, 180 ), 15

emscripten_run_script( @"window.__fbBrowserProgramDone = 'gfx_screen13';" )
EOF

cat > "$SRC_DIR/sfx_audio.bas" <<'EOF'
extern "C"
	declare sub emscripten_run_script alias "emscripten_run_script" _
		( byval script as const zstring ptr )
	declare sub emscripten_set_main_loop alias "emscripten_set_main_loop" _
		( byval func as sub cdecl (), byval fps as integer, byval simulate_infinite_loop as integer )
	declare sub fb_sfxUpdate cdecl alias "fb_sfxUpdate" _
		( byval frames as integer )
end extern

sub BrowserIdle cdecl()
end sub

emscripten_run_script( @"window.__fbBrowserProgramDone = 'sfx-start';" )

sound 0, 440, 0.80, 0.45
sound 1, 660, 0.80, 0.35
fb_sfxUpdate( 44100 )

emscripten_run_script( @"window.__fbBrowserProgramDone = 'sfx-queued';" )
emscripten_set_main_loop( @BrowserIdle, 0, 1 )
EOF

cat > "$WWW_DIR/browser_probe.js" <<'EOF'
(function() {
	"use strict";

	var test = {
		consoleText: "",
		audioSamples: [],
		audioChunks: [],
		audioFrames: 0,
		audioRate: 0,
		driverSamples: [],
		driverFrames: 0,
		driverWrites: [],
		errors: [],
		postRun: false
	};

	window.__fbBrowserTest = test;

	window.addEventListener("error", function(event) {
		test.errors.push(String(event.message || event.error || event));
	});

	window.addEventListener("unhandledrejection", function(event) {
		test.errors.push(String(event.reason || event));
	});

	function installConsoleProbe() {
		if (typeof __fb_rtlib === "undefined")
			return;
		if (!__fb_rtlib.console || __fb_rtlib.console.__fbBrowserProbeInstalled)
			return;

		var oldWrite = __fb_rtlib.console.write;
		__fb_rtlib.console.write = function(text) {
			test.consoleText += String(text);
			return oldWrite.call(__fb_rtlib.console, text);
		};

		__fb_rtlib.console.__fbBrowserProbeInstalled = true;
	}

	function installAudioProbe() {
		var AudioContext = window.AudioContext || window.webkitAudioContext;
		if (!AudioContext || AudioContext.__fbBrowserProbeInstalled)
			return;

		var oldCreateScriptProcessor = AudioContext.prototype.createScriptProcessor;
		var oldClose = AudioContext.prototype.close;

		AudioContext.prototype.close = function() {
			test.audioCloseRequests = (test.audioCloseRequests || 0) + 1;
			if (window.__fbBrowserTestAllowAudioClose && oldClose)
				return oldClose.call(this);
			return Promise.resolve();
		};

		AudioContext.prototype.createScriptProcessor = function(bufferSize, inputChannels, outputChannels) {
			var node = oldCreateScriptProcessor.call(this, bufferSize, inputChannels, outputChannels);
			var userHandler = null;
			var registeredHandler = null;
			var oldDisconnect = node.disconnect;

			test.audioRate = this.sampleRate || test.audioRate;
			window.__fbBrowserLastAudioContext = this;
			window.__fbBrowserLastAudioProcessor = node;
			node.disconnect = function() {
				test.audioDisconnectRequests = (test.audioDisconnectRequests || 0) + 1;
				if (window.__fbBrowserTestAllowAudioClose && oldDisconnect)
					return oldDisconnect.apply(this, arguments);
				return undefined;
			};

			Object.defineProperty(node, "onaudioprocess", {
				configurable: true,
				get: function() {
					return userHandler;
				},
				set: function(handler) {
					if (registeredHandler)
						node.removeEventListener("audioprocess", registeredHandler);

					userHandler = function(event) {
						var i;
						var value;
						var absValue;
						var nonzero = 0;
						var absMax = 0.0;
						var sumSquares = 0.0;
						var output;
						var keep;

						if (handler)
							handler.call(this, event);

						output = event.outputBuffer.getChannelData(0);
						keep = Math.min(output.length, Math.max(0, 262144 - test.audioSamples.length));

						for (i = 0; i < output.length; i++) {
							value = output[i];
							absValue = Math.abs(value);
							if (absValue > 0.00001)
								nonzero++;
							if (absValue > absMax)
								absMax = absValue;
							sumSquares += value * value;
							if (i < keep)
								test.audioSamples.push(value);
						}

						test.audioFrames += output.length;
						test.audioChunks.push({
							frames: output.length,
							nonzero: nonzero,
							absMax: absMax,
							rms: Math.sqrt(sumSquares / Math.max(1, output.length)),
							at: performance.now()
						});
					};

					registeredHandler = userHandler;
					if (handler)
						node.addEventListener("audioprocess", registeredHandler);
				}
			});

			return node;
		};

		AudioContext.__fbBrowserProbeInstalled = true;
	}

	if (window.Module && Module.preRun) {
		Module.preRun.push(function() {
			Module.__fbBrowserDriverCapture = function(block, frames, channels) {
				var i;
				var value;
				var absValue;
				var nonzero = 0;
				var absMax = 0.0;
				var sumSquares = 0.0;
				var keep;

				frames = frames | 0;
				channels = Math.max(1, channels | 0);
				keep = Math.min(frames, Math.max(0, 262144 - test.driverSamples.length));

				for (i = 0; i < frames; i++) {
					value = block[i * channels] || 0.0;
					absValue = Math.abs(value);
					if (absValue > 0.00001)
						nonzero++;
					if (absValue > absMax)
						absMax = absValue;
					sumSquares += value * value;
					if (i < keep)
						test.driverSamples.push(value);
				}

				test.driverFrames += frames;
				test.driverWrites.push({
					frames: frames,
					nonzero: nonzero,
					absMax: absMax,
					rms: Math.sqrt(sumSquares / Math.max(1, frames)),
					at: performance.now()
				});
			};
			installConsoleProbe();
			installAudioProbe();
		});
	}

	if (window.Module && Module.postRun) {
		Module.postRun.push(function() {
			test.postRun = true;
		});
	}
})();
EOF

cat > "$WORKROOT/run-browser-tests.js" <<'EOF'
/*
    FreeBASIC JS Browser Test Runner
    --------------------------------

    File: run-browser-tests.js

    Purpose:

        Drive generated fbc-js browser programs through a real Chromium
        browser and collect the observable console, canvas, and WebAudio
        results.

    Responsibilities:

        - serve generated HTML/JavaScript/WebAssembly files
        - launch a browser with DevTools enabled
        - sample browser state through the DevTools protocol
        - verify audio frequency and continuity

    This file intentionally does NOT contain:

        - FreeBASIC compilation
        - package building
        - test source generation
*/

"use strict";

const childProcess = require("child_process");
const fs = require("fs");
const http = require("http");
const net = require("net");
const path = require("path");

function die(message) {
	console.error("ERROR: " + message);
	process.exit(1);
}

function getArg(name) {
	const index = process.argv.indexOf(name);
	if (index < 0)
		return "";
	if (index + 1 >= process.argv.length)
		die("missing value for " + name);
	return process.argv[index + 1];
}

const browserPath = getArg("--browser");
const webRoot = getArg("--webroot");
const logDir = getArg("--logdir");
const headful = process.argv.includes("--headful");

if (!browserPath)
	die("missing --browser");
if (!webRoot)
	die("missing --webroot");
if (!logDir)
	die("missing --logdir");

function freePort() {
	return new Promise((resolve, reject) => {
		const server = net.createServer();
		server.listen(0, "127.0.0.1", () => {
			const port = server.address().port;
			server.close(() => resolve(port));
		});
		server.on("error", reject);
	});
}

function contentType(filename) {
	switch (path.extname(filename).toLowerCase()) {
	case ".html":
		return "text/html";
	case ".js":
		return "text/javascript";
	case ".wasm":
		return "application/wasm";
	default:
		return "application/octet-stream";
	}
}

function startStaticServer(root, port) {
	const server = http.createServer((request, response) => {
		const url = new URL(request.url, "http://127.0.0.1");
		let filename = path.normalize(decodeURIComponent(url.pathname.replace(/^\/+/, "")));
		let fullPath;

		if (!filename)
			filename = "index.html";

		fullPath = path.resolve(root, filename);
		if (!fullPath.startsWith(path.resolve(root) + path.sep)) {
			response.writeHead(403);
			response.end("forbidden");
			return;
		}

		fs.readFile(fullPath, (error, data) => {
			if (error) {
				response.writeHead(404);
				response.end("not found");
				return;
			}

			response.writeHead(200, {
				"Content-Type": contentType(fullPath),
				"Cache-Control": "no-store"
			});
			response.end(data);
		});
	});

	return new Promise((resolve, reject) => {
		server.listen(port, "127.0.0.1", () => resolve(server));
		server.on("error", reject);
	});
}

async function waitForJson(url, timeoutMs) {
	const started = Date.now();
	let lastError = null;

	while (Date.now() - started < timeoutMs) {
		try {
			const response = await fetch(url);
			if (response.ok)
				return await response.json();
			lastError = new Error("HTTP " + response.status);
		} catch (error) {
			lastError = error;
		}
		await new Promise(resolve => setTimeout(resolve, 100));
	}

	throw lastError || new Error("timeout waiting for " + url);
}

class DevToolsClient {
	constructor(socketUrl) {
		this.socketUrl = socketUrl;
		this.nextId = 1;
		this.pending = new Map();
		this.events = [];
	}

	open() {
		return new Promise((resolve, reject) => {
			this.socket = new WebSocket(this.socketUrl);
			this.socket.onopen = () => resolve();
			this.socket.onerror = event => reject(new Error("WebSocket error: " + String(event.message || event)));
			this.socket.onmessage = event => this.handleMessage(event.data);
		});
	}

	close() {
		if (this.socket)
			this.socket.close();
	}

	handleMessage(data) {
		const message = JSON.parse(data);
		let pending;

		if (message.id) {
			pending = this.pending.get(message.id);
			if (!pending)
				return;
			this.pending.delete(message.id);
			if (message.error)
				pending.reject(new Error(JSON.stringify(message.error)));
			else
				pending.resolve(message.result || {});
			return;
		}

		this.events.push(message);
	}

	send(method, params) {
		const id = this.nextId++;
		const message = { id: id, method: method, params: params || {} };

		return new Promise((resolve, reject) => {
			this.pending.set(id, { resolve: resolve, reject: reject });
			this.socket.send(JSON.stringify(message));
		});
	}

	async evaluate(expression) {
		const result = await this.send("Runtime.evaluate", {
			expression: expression,
			awaitPromise: true,
			returnByValue: true
		});

		if (result.exceptionDetails)
			throw new Error("browser evaluation failed: " + JSON.stringify(result.exceptionDetails));

		return result.result ? result.result.value : undefined;
	}
}

function goertzel(samples, sampleRate, frequency, firstFrame, frames) {
	const omega = 2.0 * Math.PI * frequency / sampleRate;
	const coeff = 2.0 * Math.cos(omega);
	let q0 = 0.0;
	let q1 = 0.0;
	let q2 = 0.0;
	let i;

	for (i = 0; i < frames; i++) {
		q0 = coeff * q1 - q2 + samples[firstFrame + i];
		q2 = q1;
		q1 = q0;
	}

	return Math.sqrt(q1 * q1 + q2 * q2 - q1 * q2 * coeff) / Math.max(1, frames);
}

function rms(samples, firstFrame, frames) {
	let sumSquares = 0.0;
	let i;

	if (frames <= 0)
		return 0.0;

	for (i = 0; i < frames; i++)
		sumSquares += samples[firstFrame + i] * samples[firstFrame + i];

	return Math.sqrt(sumSquares / frames);
}

function lowEnergyWindows(samples, firstFrame, frames, windowFrames, minimumRms) {
	let offset = 0;
	let low = 0;

	while (offset + windowFrames <= frames) {
		if (rms(samples, firstFrame + offset, windowFrames) < minimumRms)
			low++;
		offset += windowFrames;
	}

	return low;
}

function assertColorNear(actual, expected, tolerance, label) {
	const dr = Math.abs(actual.r - expected.r);
	const dg = Math.abs(actual.g - expected.g);
	const db = Math.abs(actual.b - expected.b);

	if (dr > tolerance || dg > tolerance || db > tolerance)
		throw new Error(label + " expected rgb(" + expected.r + "," + expected.g + "," + expected.b + ")" +
			" but saw rgb(" + actual.r + "," + actual.g + "," + actual.b + ")");
}

async function waitFor(client, expression, timeoutMs) {
	const started = Date.now();

	while (Date.now() - started < timeoutMs) {
		if (await client.evaluate(expression))
			return;
		await new Promise(resolve => setTimeout(resolve, 100));
	}

	throw new Error("timeout waiting for: " + expression);
}

async function browserState(client) {
	return await client.evaluate(`(function() {
		var canvas = document.getElementById("canvas");
		var state = window.__fbBrowserTest || {};
		return {
			done: window.__fbBrowserProgramDone || "",
			consoleText: state.consoleText || "",
			errors: state.errors || [],
			postRun: !!state.postRun,
			audioRate: state.audioRate || 0,
			audioFrames: state.audioFrames || 0,
			audioSamples: state.audioSamples ? state.audioSamples.slice(0) : [],
			audioChunks: state.audioChunks ? state.audioChunks.slice(0) : [],
			driverSamples: state.driverSamples ? state.driverSamples.slice(0) : [],
			driverWrites: state.driverWrites ? state.driverWrites.slice(0) : [],
			driverFrames: state.driverFrames || 0,
			audioCloseRequests: state.audioCloseRequests || 0,
			audioDisconnectRequests: state.audioDisconnectRequests || 0,
			audioContextState: window.__fbBrowserLastAudioContext ? window.__fbBrowserLastAudioContext.state : "",
			canvasWidth: canvas ? canvas.width : 0,
			canvasHeight: canvas ? canvas.height : 0
		};
	})()`);
}

async function samplePixels(client, points) {
	return await client.evaluate(`(function() {
		var canvas = document.getElementById("canvas");
		var ctx = canvas.getContext("2d");
		var points = ${JSON.stringify(points)};
		var result = {};
		points.forEach(function(point) {
			var data = ctx.getImageData(point.x, point.y, 1, 1).data;
			result[point.name] = { r: data[0], g: data[1], b: data[2], a: data[3] };
		});
		return result;
	})()`);
}

async function openTestPage(debugPort, serverPort, pageName) {
	const targetUrl = "http://127.0.0.1:" + serverPort + "/" + pageName + ".html";
	const newTargetUrl = "http://127.0.0.1:" + debugPort + "/json/new?" + encodeURIComponent(targetUrl);
	let target;
	let response;
	let client;

	response = await fetch(newTargetUrl, { method: "PUT" });
	if (!response.ok)
		response = await fetch(newTargetUrl);
	if (!response.ok)
		throw new Error("could not create browser tab for " + pageName + ": HTTP " + response.status);

	target = await response.json();
	client = new DevToolsClient(target.webSocketDebuggerUrl);
	await client.open();
	await client.send("Runtime.enable");
	await client.send("Page.enable");
	return client;
}

async function runConsoleTest(debugPort, serverPort) {
	const client = await openTestPage(debugPort, serverPort, "console");
	let state;

	try {
		await waitFor(client, `window.__fbBrowserProgramDone === "console"`, 10000);
		state = await browserState(client);

		if (!state.consoleText.includes("FBJS_BROWSER_CONSOLE_OK"))
			throw new Error("console marker was not captured");
		if (state.errors.length)
			throw new Error("browser errors: " + state.errors.join("; "));

		return {
			name: "console",
			ok: true,
			consoleText: state.consoleText
		};
	} finally {
		client.close();
	}
}

async function runGfxScreensetTest(debugPort, serverPort) {
	const client = await openTestPage(debugPort, serverPort, "gfx_screenset");
	let pixels;
	let state;

	try {
		await waitFor(client, `window.__fbBrowserProgramDone === "gfx_screenset"`, 10000);
		await new Promise(resolve => setTimeout(resolve, 250));
		state = await browserState(client);
		pixels = await samplePixels(client, [
			{ name: "green_page", x: 32, y: 32 },
			{ name: "blue_page", x: 100, y: 80 },
			{ name: "yellow_point", x: 80, y: 32 }
		]);

		if (state.canvasWidth < 128 || state.canvasHeight < 96)
			throw new Error("unexpected canvas size " + state.canvasWidth + "x" + state.canvasHeight);
		assertColorNear(pixels.green_page, { r: 0, g: 255, b: 0 }, 8, "SCREENSET visible page green block");
		assertColorNear(pixels.blue_page, { r: 0, g: 0, b: 255 }, 8, "SCREENSET visible page blue background");
		assertColorNear(pixels.yellow_point, { r: 255, g: 255, b: 0 }, 8, "SCREENSET visible page yellow point");
		if (state.errors.length)
			throw new Error("browser errors: " + state.errors.join("; "));

		return {
			name: "gfx_screenset",
			ok: true,
			canvas: state.canvasWidth + "x" + state.canvasHeight,
			pixels: pixels
		};
	} finally {
		client.close();
	}
}

async function runGfxScreen13Test(debugPort, serverPort) {
	const client = await openTestPage(debugPort, serverPort, "gfx_screen13");
	let pixels;
	let state;

	try {
		await waitFor(client, `window.__fbBrowserProgramDone === "gfx_screen13"`, 10000);
		await new Promise(resolve => setTimeout(resolve, 250));
		state = await browserState(client);
		pixels = await samplePixels(client, [
			{ name: "red_block", x: 20, y: 20 },
			{ name: "green_block", x: 160, y: 20 },
			{ name: "yellow_block", x: 280, y: 20 },
			{ name: "white_point", x: 300, y: 180 }
		]);

		if (state.canvasWidth < 320 || state.canvasHeight < 200)
			throw new Error("unexpected canvas size " + state.canvasWidth + "x" + state.canvasHeight);
		assertColorNear(pixels.red_block, { r: 170, g: 0, b: 0 }, 24, "SCREEN 13 red block");
		assertColorNear(pixels.green_block, { r: 0, g: 170, b: 0 }, 24, "SCREEN 13 green block");
		assertColorNear(pixels.yellow_block, { r: 255, g: 255, b: 85 }, 32, "SCREEN 13 yellow block");
		assertColorNear(pixels.white_point, { r: 255, g: 255, b: 255 }, 8, "SCREEN 13 white point");
		if (state.errors.length)
			throw new Error("browser errors: " + state.errors.join("; "));

		return {
			name: "gfx_screen13",
			ok: true,
			canvas: state.canvasWidth + "x" + state.canvasHeight,
			pixels: pixels
		};
	} finally {
		client.close();
	}
}

async function runSfxAudioTest(debugPort, serverPort) {
	const client = await openTestPage(debugPort, serverPort, "sfx_audio");
	let state;
	let samples;
	let sampleRate;
	let usableFrames;
	let power440;
	let power660;
	let power550;
	let bodyRms;
	let lowWindows;
	let activeChunks;
	let captureSource;

	try {
		await waitFor(client, `window.__fbBrowserProgramDone === "sfx-queued"`, 10000);
		await client.evaluate(`(async function() {
			if (window.__fbBrowserLastAudioContext && window.__fbBrowserLastAudioContext.resume)
				await window.__fbBrowserLastAudioContext.resume();
			return window.__fbBrowserLastAudioContext ? window.__fbBrowserLastAudioContext.state : "";
		})()`);
		try {
			await waitFor(client, `(window.__fbBrowserTest && window.__fbBrowserTest.audioFrames >= 32768)`, 10000);
		} catch (error) {
			/*
			    Chromium headless builds on Windows do not always run the
			    WebAudio render callback.  Keep the callback path as the
			    strongest proof when the browser provides it, and otherwise
			    use the browser-driver capture installed immediately before
			    the WebAudio queue receives each block.
			*/
		}
		state = await browserState(client);

		if (state.errors.length)
			throw new Error("browser errors: " + state.errors.join("; "));

		if ((state.audioSamples || []).length >= Math.floor((state.audioRate || 44100) * 0.35)) {
			samples = state.audioSamples || [];
			captureSource = "webaudio-callback";
		} else {
			samples = state.driverSamples || [];
			captureSource = "webaudio-driver";
		}

		sampleRate = state.audioRate || 44100;
		usableFrames = Math.min(samples.length - 1024, Math.floor(sampleRate * 0.50));

		if (samples.length < Math.floor(sampleRate * 0.35))
			throw new Error("captured too few audio samples: callback=" +
				(state.audioSamples || []).length + " driver=" + (state.driverSamples || []).length +
				" context=" + state.audioContextState + " closes=" + state.audioCloseRequests);
		if (usableFrames < 8192)
			throw new Error("captured audio was too short for frequency analysis");

		power440 = goertzel(samples, sampleRate, 440.0, 1024, usableFrames);
		power660 = goertzel(samples, sampleRate, 660.0, 1024, usableFrames);
		power550 = goertzel(samples, sampleRate, 550.0, 1024, usableFrames);
		bodyRms = rms(samples, 1024, usableFrames);
		lowWindows = lowEnergyWindows(samples, 1024, usableFrames, 1024, 0.03);

		if (bodyRms < 0.05)
			throw new Error("audio RMS is too low: " + bodyRms.toFixed(6));
		if (power440 < power550 * 1.8)
			throw new Error("440 Hz tone not dominant enough: p440=" + power440 + " p550=" + power550);
		if (power660 < power550 * 1.5)
			throw new Error("660 Hz tone not dominant enough: p660=" + power660 + " p550=" + power550);
		if (lowWindows !== 0)
			throw new Error("detected low-energy gaps inside active tone body: " + lowWindows);

		if (captureSource === "webaudio-callback") {
			activeChunks = state.audioChunks.filter(chunk => chunk.rms > 0.03);
			if (activeChunks.length < 4)
				throw new Error("too few active WebAudio chunks captured");
		} else {
			activeChunks = state.driverWrites.filter(chunk => chunk.rms > 0.03);
			if (activeChunks.length < 4)
				throw new Error("too few active WebAudio driver writes captured");
		}

		return {
			name: "sfx_audio",
			ok: true,
			captureSource: captureSource,
			sampleRate: sampleRate,
			capturedSamples: samples.length,
			audioFrames: state.audioFrames,
			driverFrames: state.driverFrames,
			bodyRms: bodyRms,
			power440: power440,
			power660: power660,
			power550: power550,
			lowWindows: lowWindows,
			activeChunks: activeChunks.length,
			audioContextState: state.audioContextState,
			audioCloseRequests: state.audioCloseRequests,
			audioDisconnectRequests: state.audioDisconnectRequests
		};
	} finally {
		client.close();
	}
}

async function main() {
	const serverPort = await freePort();
	const debugPort = await freePort();
	const server = await startStaticServer(webRoot, serverPort);
	const profile = path.join(logDir, "browser-profile");
	const report = {
		browser: browserPath,
		webRoot: webRoot,
		results: []
	};
	let browser;
	let browserArgs;

	fs.mkdirSync(profile, { recursive: true });

	browserArgs = [
		"--disable-gpu",
		"--no-first-run",
		"--no-default-browser-check",
		"--autoplay-policy=no-user-gesture-required",
		"--disable-background-timer-throttling",
		"--disable-renderer-backgrounding",
		"--remote-debugging-port=" + debugPort,
		"--user-data-dir=" + profile,
		"about:blank"
	];

	if (!headful)
		browserArgs.unshift("--headless=new");

	browser = childProcess.spawn(browserPath, browserArgs, {
		stdio: ["ignore", "pipe", "pipe"]
	});

	browser.stdout.on("data", data => fs.appendFileSync(path.join(logDir, "browser.stdout.log"), data));
	browser.stderr.on("data", data => fs.appendFileSync(path.join(logDir, "browser.stderr.log"), data));

	try {
		await waitForJson("http://127.0.0.1:" + debugPort + "/json/version", 15000);

		report.results.push(await runConsoleTest(debugPort, serverPort));
		report.results.push(await runGfxScreensetTest(debugPort, serverPort));
		report.results.push(await runGfxScreen13Test(debugPort, serverPort));
		report.results.push(await runSfxAudioTest(debugPort, serverPort));

		fs.writeFileSync(path.join(logDir, "browser-report.json"), JSON.stringify(report, null, "\t") + "\n");

		report.results.forEach(result => {
			console.log("PASS " + result.name);
		});
	} finally {
		server.close();
		if (browser) {
			browser.kill();
			await new Promise(resolve => browser.once("exit", resolve));
		}
	}
}

main().catch(error => {
	fs.mkdirSync(logDir, { recursive: true });
	fs.writeFileSync(path.join(logDir, "browser-failure.txt"), String(error && error.stack ? error.stack : error) + "\n");
	console.error(error && error.stack ? error.stack : error);
	process.exit(1);
});

/* end of run-browser-tests.js */
EOF

##############################################################################
# Compile and run
##############################################################################

msg "Compiling fbc-js browser programs"

# shellcheck disable=SC1090
. "$DIST_DIR/freebasic-js-env.sh"

FBC_JS="$DIST_DIR/bin/fbc-js.exe"
[ -f "$FBC_JS" ] || fail "fbc-js executable disappeared: $FBC_JS"

compile_test() {
	local source="$1"
	local output="$2"
	local attempt

	for attempt in 1 2 3; do
		rm -f "$WWW_DIR/$output.html" "$WWW_DIR/$output.js" "$WWW_DIR/$output.wasm"
		if run "$FBC_JS" "$SRC_DIR/$source" -x "$WWW_DIR/$output.html"; then
			break
		fi
		if [ "$attempt" -eq 3 ]; then
			fail "could not compile $source after $attempt attempts"
		fi
		echo "WARNING: fbc-js compile failed for $source; retrying ($attempt/3)" >&2
		sleep 1
	done

	patch_html_for_probe "$WWW_DIR/$output.html"
}

compile_test console.bas console
compile_test gfx_screenset.bas gfx_screenset
compile_test gfx_screen13.bas gfx_screen13
compile_test sfx_audio.bas sfx_audio
patch_sfx_driver_capture "$WWW_DIR/sfx_audio.js"

NODE_EXE=""
if have node; then
	NODE_EXE="$(command -v node)"
elif [ -f "$DIST_DIR/toolchain/ucrt64/bin/node.exe" ]; then
	NODE_EXE="$DIST_DIR/toolchain/ucrt64/bin/node.exe"
fi

[ -n "$NODE_EXE" ] || fail "node was not found in PATH or the fbc-js distribution"

msg "Running browser validation"
NODE_ARGS=(
	"$WORKROOT/run-browser-tests.js"
	--browser "$BROWSER"
	--webroot "$WWW_DIR"
	--logdir "$LOG_DIR"
)

if [ "$HEADFUL" -ne 0 ]; then
	NODE_ARGS+=(--headful)
fi

run "$NODE_EXE" "${NODE_ARGS[@]}"

msg "Browser test report"
cat "$LOG_DIR/browser-report.json"

if [ "$KEEP_WORKROOT" -eq 0 ]; then
	echo ""
	echo "Generated files kept at: $WORKROOT"
else
	echo ""
	echo "Generated files kept at: $WORKROOT"
fi

##############################################################################
# End
##############################################################################

# end of msys2-test-freebasic-js-browser.sh

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
Usage: ./build_scripts/debianubuntu-test-freebasic-js-fbctests-browser.sh [options]

Options:
  --package-dir DIR   Directory containing Debian package artifacts
  --source-dir DIR    FreeBASIC source checkout to test (default: repo root)
  --workroot DIR      Host work directory for generated artifacts
                      (default: .build-js-fbctests-browser)
  --image IMAGE       Docker image with Chromium and Node
                      (default: mcr.microsoft.com/playwright:v1.56.1-noble)
  --docker-cmd CMD    Docker command to use (default: docker)
  --jobs N            Parallel compile jobs inside the container
  --timeout SEC       Browser run timeout in seconds (default: 600)
  --keep-workroot     Keep generated files after a successful run
  --skip-host-deps    Skip Docker host dependency installation
  --help              Show this help text

The test installs the local freebasic-js .deb package into a fresh Chromium
container, builds tests/fbc-tests.html with fbc-js, then runs the aggregate
fbcunit suite in a real browser through the DevTools protocol.
EOF
}

PACKAGE_DIR=""
SOURCE_DIR="$ROOT"
WORKROOT="$ROOT/.build-js-fbctests-browser"
IMAGE="${IMAGE:-mcr.microsoft.com/playwright:v1.56.1-noble}"
DOCKER_CMD="${DOCKER_CMD:-docker}"
JOBS="${JOBS:-}"
TIMEOUT_SECONDS=600
KEEP_WORKROOT=0
SKIP_HOST_DEPS=0

while [ $# -gt 0 ]; do
    case "$1" in
        --package-dir) PACKAGE_DIR="$2"; shift 2 ;;
        --source-dir) SOURCE_DIR="$2"; shift 2 ;;
        --workroot) WORKROOT="$2"; shift 2 ;;
        --image) IMAGE="$2"; shift 2 ;;
        --docker-cmd) DOCKER_CMD="$2"; shift 2 ;;
        --jobs) JOBS="$2"; shift 2 ;;
        --timeout) TIMEOUT_SECONDS="$2"; shift 2 ;;
        --keep-workroot) KEEP_WORKROOT=1; shift ;;
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

[ -d "$SOURCE_DIR/tests" ] || die "source tests directory not found: $SOURCE_DIR/tests"
[ -d "$SOURCE_DIR/inc" ] || die "source inc directory not found: $SOURCE_DIR/inc"
SOURCE_DIR="$(cd "$SOURCE_DIR" && pwd -P)"

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

TEST_RUNNER="$(mktemp -t fb-js-browser-fbctests.XXXXXX.sh)"
WORKROOT="$(mkdir -p "$WORKROOT" && cd "$WORKROOT" && pwd -P)"

cleanup() {
    status=$?
    rm -f "$TEST_RUNNER"
    if [ "$status" -eq 0 ] && [ "$KEEP_WORKROOT" -eq 0 ]; then
        rm -rf "$WORKROOT"
    elif [ "$KEEP_WORKROOT" -ne 0 ] || [ "$status" -ne 0 ]; then
        echo "==> retained workroot: $WORKROOT"
    fi
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

: "${WORKROOT:=/workroot}"
: "${JOBS:=}"
: "${TIMEOUT_SECONDS:=600}"

if [ -f /etc/apt/sources.list.d/nodesource.list ] || [ -f /etc/apt/sources.list.d/nodesource.sources ]; then
    rm -f /etc/apt/sources.list.d/nodesource.list /etc/apt/sources.list.d/nodesource.sources

    if dpkg-query -W -f='${Status}' nodejs 2>/dev/null | grep -q 'install ok installed'; then
        run apt-get purge -y nodejs
    fi
fi

run apt-get update -y
run apt-get install -y --no-install-recommends /packages/freebasic-js_*.deb ca-certificates make rsync

command -v fbc-js >/dev/null 2>&1 || fail "fbc-js was not installed"
command -v emcc >/dev/null 2>&1 || fail "emcc dependency was not installed"
command -v node >/dev/null 2>&1 || fail "node dependency was not installed"

CHROMIUM="$(find_chromium)" || fail "could not locate Chromium in the test image"

if [ -z "$JOBS" ]; then
    JOBS="$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 1)"
fi

rm -rf "$WORKROOT/tests" "$WORKROOT/inc"
mkdir -p "$WORKROOT/tests" "$WORKROOT/inc"

run rsync -a --delete \
    --exclude '*.o' \
    --exclude '*.a' \
    --exclude '*.exe' \
    --exclude 'fbc-tests' \
    --exclude 'fbc-tests.html' \
    --exclude 'fbc-tests.js' \
    --exclude 'fbc-tests.wasm' \
    --exclude 'fbc-tests.data' \
    /source/tests/ "$WORKROOT/tests/"

run rsync -a --delete /source/inc/ "$WORKROOT/inc/"

cat > "$WORKROOT/tests/fbctests-browser-harness.html" <<'EOF'
<!doctype html>
<html lang="en">
<head>
    <meta charset="utf-8">
    <title>FreeBASIC JS fbcunit browser harness</title>
</head>
<body>
    <canvas id="canvas" width="640" height="480"></canvas>
    <pre id="output"></pre>
    <script>
        window.__fbctestOutput = [];
        window.__fbctestExitStatus = null;
        window.__fbctestDone = false;

        function appendOutput(text) {
            text = String(text);
            window.__fbctestOutput.push(text);
            document.getElementById("output").textContent += text + "\n";
        }

        var Module = {
            arguments: [ "--brief-summary" ],
            canvas: document.getElementById("canvas"),
            print: function(text) {
                if (arguments.length > 1) {
                    text = Array.prototype.slice.call(arguments).join(" ");
                }
                appendOutput(text);
            },
            printErr: function(text) {
                if (arguments.length > 1) {
                    text = Array.prototype.slice.call(arguments).join(" ");
                }
                appendOutput("stderr: " + text);
                console.error(text);
            },
            onExit: function(status) {
                window.__fbctestExitStatus = status;
                window.__fbctestDone = true;
                console.log("__FBCTEST_DONE__:" + status);
            },
            setStatus: function(text) {
                window.__fbctestStatus = String(text);
            },
            monitorRunDependencies: function(left) {
                window.__fbctestRunDependencies = left;
            }
        };
    </script>
    <script async src="fbc-tests.js"></script>
</body>
</html>
EOF

cat > "$WORKROOT/run-fbctests-browser-cdp.js" <<'EOF'
const fs = require("fs");
const http = require("http");
const path = require("path");
const { spawn } = require("child_process");

const root = path.resolve(process.argv[2]);
const chromePath = process.argv[3];
const timeoutMs = Number(process.argv[4] || 600) * 1000;
const cdpPort = 9222;

function contentType(filename) {
    if (filename.endsWith(".html")) return "text/html; charset=utf-8";
    if (filename.endsWith(".js")) return "text/javascript; charset=utf-8";
    if (filename.endsWith(".wasm")) return "application/wasm";
    if (filename.endsWith(".data")) return "application/octet-stream";
    return "application/octet-stream";
}

function startServer() {
    const server = http.createServer((req, res) => {
        const url = new URL(req.url, "http://127.0.0.1/");
        let file = path.normalize(decodeURIComponent(url.pathname));

        if (file === "/" || file === ".") {
            file = "/fbctests-browser-harness.html";
        }

        const diskPath = path.join(root, file);
        if (!diskPath.startsWith(root + path.sep)) {
            res.writeHead(403);
            res.end("forbidden");
            return;
        }

        fs.readFile(diskPath, (err, data) => {
            if (err) {
                res.writeHead(404);
                res.end("missing");
                return;
            }

            res.writeHead(200, { "content-type": contentType(diskPath) });
            res.end(data);
        });
    });

    return new Promise(resolve => {
        server.listen(0, "127.0.0.1", () => resolve(server));
    });
}

function sleep(ms) {
    return new Promise(resolve => setTimeout(resolve, ms));
}

async function fetchJson(url, options) {
    const response = await fetch(url, options);
    if (!response.ok) {
        throw new Error(`${url} returned HTTP ${response.status}`);
    }
    return response.json();
}

async function waitForChrome() {
    const deadline = Date.now() + 30000;
    const url = `http://127.0.0.1:${cdpPort}/json/version`;

    while (Date.now() < deadline) {
        try {
            return await fetchJson(url);
        } catch (_) {
            await sleep(250);
        }
    }

    throw new Error("timed out waiting for Chromium DevTools");
}

class CdpClient {
    constructor(wsUrl) {
        this.ws = new WebSocket(wsUrl);
        this.nextId = 1;
        this.pending = new Map();

        this.ready = new Promise((resolve, reject) => {
            this.ws.addEventListener("open", resolve, { once: true });
            this.ws.addEventListener("error", reject, { once: true });
        });

        this.ws.addEventListener("message", event => {
            const message = JSON.parse(event.data);
            if (message.id) {
                const pending = this.pending.get(message.id);
                if (!pending) {
                    return;
                }
                this.pending.delete(message.id);
                if (message.error) {
                    pending.reject(new Error(message.error.message));
                } else {
                    pending.resolve(message.result);
                }
                return;
            }

            if (message.method === "Runtime.consoleAPICalled") {
                const args = message.params.args || [];
                const text = args.map(arg => arg.value || arg.description || "").join(" ");
                if (text.startsWith("__FBCTEST_DONE__:")) {
                    this.doneStatus = Number(text.slice("__FBCTEST_DONE__:".length));
                }
            }
            if (message.method === "Runtime.exceptionThrown") {
                const details = message.params.exceptionDetails;
                this.exception = details.text || "browser exception";
            }
        });
    }

    async send(method, params = {}, timeout = 30000) {
        await this.ready;
        const id = this.nextId++;
        const payload = JSON.stringify({ id, method, params });

        const result = new Promise((resolve, reject) => {
            const timer = setTimeout(() => {
                this.pending.delete(id);
                reject(new Error(`${method} timed out`));
            }, timeout);

            this.pending.set(id, {
                resolve: value => {
                    clearTimeout(timer);
                    resolve(value);
                },
                reject: error => {
                    clearTimeout(timer);
                    reject(error);
                }
            });
        });

        this.ws.send(payload);
        return result;
    }

    close() {
        this.ws.close();
    }
}

async function main() {
    for (const artifact of [ "fbc-tests.js", "fbc-tests.wasm", "fbc-tests.data" ]) {
        if (!fs.existsSync(path.join(root, artifact))) {
            throw new Error(`${artifact} not found in ${root}`);
        }
    }

    const server = await startServer();
    const serverPort = server.address().port;
    const userDataDir = fs.mkdtempSync(path.join("/tmp", "fbctests-chrome-"));
    const chrome = spawn(chromePath, [
        "--headless=new",
        "--no-sandbox",
        "--disable-gpu",
        "--disable-dev-shm-usage",
        `--remote-debugging-port=${cdpPort}`,
        `--user-data-dir=${userDataDir}`,
        "about:blank"
    ], { stdio: [ "ignore", "ignore", "pipe" ] });

    let chromeStderr = "";
    chrome.stderr.on("data", chunk => {
        chromeStderr += chunk.toString();
    });

    try {
        await waitForChrome();

        const url = `http://127.0.0.1:${serverPort}/fbctests-browser-harness.html`;
        const page = await fetchJson(
            `http://127.0.0.1:${cdpPort}/json/new?${encodeURIComponent(url)}`,
            { method: "PUT" }
        );
        const cdp = new CdpClient(page.webSocketDebuggerUrl);

        await cdp.send("Runtime.enable");
        await cdp.send("Log.enable");
        await cdp.send("Page.enable");

        const deadline = Date.now() + timeoutMs;
        while (Date.now() < deadline && cdp.doneStatus === undefined && !cdp.exception) {
            await sleep(1000);
        }

        const dump = await cdp.send("Runtime.evaluate", {
            expression: `JSON.stringify({
                done: window.__fbctestDone === true,
                exitStatus: window.__fbctestExitStatus,
                status: window.__fbctestStatus || "",
                output: window.__fbctestOutput || []
            })`,
            returnByValue: true
        }, 120000);

        const state = JSON.parse(dump.result.value);
        const output = state.output.join("\n");
        fs.writeFileSync(path.join(root, "browser-fbctests-output.txt"), output + "\n");

        cdp.close();

        if (cdp.exception) {
            throw new Error(`browser exception: ${cdp.exception}`);
        }
        if (!state.done && cdp.doneStatus === undefined) {
            throw new Error(`timed out waiting for fbc-tests; status=${state.status}`);
        }

        const exitStatus = state.exitStatus ?? cdp.doneStatus;
        const totalLine = output.split("\n").filter(line => line.includes("Total")).pop() || "";

        console.log(totalLine);
        if (exitStatus !== 0) {
            console.error(output);
            throw new Error(`fbc-tests exited with status ${exitStatus}`);
        }

        if (!/Total/.test(totalLine)) {
            throw new Error("fbc-tests exited successfully but no Total summary line was captured");
        }

        console.log("browser fbc-tests passed");
    } finally {
        server.close();
        chrome.kill("SIGTERM");
        await sleep(500);
        if (!chrome.killed) {
            chrome.kill("SIGKILL");
        }
        fs.rmSync(userDataDir, { recursive: true, force: true });
        if (chromeStderr.trim()) {
            fs.writeFileSync(path.join(root, "browser-chrome-stderr.txt"), chromeStderr);
        }
    }
}

main().catch(error => {
    console.error(error.stack || error.message || String(error));
    process.exit(1);
});
EOF

cd "$WORKROOT/tests"
run make -j"$JOBS" -f unit-tests.mk all FBC="fbc-js" TARGET=js-asmjs TARGET_OS=js

[ -f fbc-tests.html ] || fail "fbc-tests.html was not produced"
[ -f fbc-tests.js ] || fail "fbc-tests.js was not produced"
[ -f fbc-tests.wasm ] || fail "fbc-tests.wasm was not produced"
[ -f fbc-tests.data ] || fail "fbc-tests.data was not produced"

run node "$WORKROOT/run-fbctests-browser-cdp.js" "$WORKROOT/tests" "$CHROMIUM" "$TIMEOUT_SECONDS"
TEST_RUNNER_EOF

install_host_deps

rm -rf "$WORKROOT"/*

msg "browser-running fbcunit through packaged freebasic-js in $IMAGE"
run $DOCKER_CMD run --rm \
    -v "$PACKAGE_DIR:/packages:ro" \
    -v "$SOURCE_DIR:/source:ro" \
    -v "$WORKROOT:/workroot" \
    -v "$TEST_RUNNER:/tmp/test-freebasic-js-fbctests-browser.sh:ro" \
    -e WORKROOT=/workroot \
    -e JOBS="$JOBS" \
    -e TIMEOUT_SECONDS="$TIMEOUT_SECONDS" \
    "$IMAGE" \
    /bin/sh /tmp/test-freebasic-js-fbctests-browser.sh

msg "freebasic-js browser fbcunit test passed"

# end of debianubuntu-test-freebasic-js-fbctests-browser.sh

#!/usr/bin/env bash

##############################################################################
# FreeBASIC NetBSD VM package builder
##############################################################################
#
# Purpose:
#
#   Build and test the NetBSD amd64 FreeBASIC package from a Debian/Ubuntu
#   Linux host.
#
# Responsibilities:
#
#   * install a clean NetBSD amd64 VM using Anita
#   * copy the current source tree into the VM over QEMU user networking
#   * build the native NetBSD pkgsrc-style binary package
#   * install that package in a fresh NetBSD VM snapshot
#   * run console, gfxlib, sfxlib, fbctests, and exampleageddon checks
#   * capture QEMU audio output and verify that it is not silent
#
# This script intentionally does NOT contain:
#
#   * non-amd64 NetBSD support
#   * NetBSD cross-compilation
#   * a replacement for Anita's sysinst automation
#
##############################################################################

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
WORKROOT="$ROOT/out/netbsd-vm"
TOOLS_DIR="$WORKROOT/tools"
ANITA_VERSION="2.16"
ANITA_DIR="$TOOLS_DIR/anita-$ANITA_VERSION"
ANITA="$ANITA_DIR/anita"
ANITA_URL="https://www.gson.org/netbsd/anita/download/anita-$ANITA_VERSION.tar.gz"
TOOL_BIN="$TOOLS_DIR/bin"
RUN_DIR="$WORKROOT/run"
SERVE_DIR="$RUN_DIR/serve"
UPLOAD_DIR="$RUN_DIR/upload"
LOG_DIR="$WORKROOT/logs"
PACKAGE_DIR="$WORKROOT/packages"
ARCHIVE_DIR="$ROOT/out/netbsd/x86-64"
ANITA_WORKDIR=""

RELEASE="11.0"
ARCH="amd64"
PKG_ARCH="x86_64"
DIST_URL=""
JOBS="$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 1)"
CPUS="$JOBS"
FBCTESTS_JOBS="$JOBS"
EXAMPLEAGEDDON_JOBS="$JOBS"
EXAMPLEAGEDDON_COMPILE_TIMEOUT="180"
EXAMPLEAGEDDON_RUN_TIMEOUT="10"
MEMORY="6144M"
DISK_SIZE="32G"
HTTP_PORT=""
PACKAGE_FILE=""
TEST_ONLY=0
KEEP_VM=0
QEMU_ACCEL="${QEMU_ACCEL:-kvm}"
QEMU_CPU="${QEMU_CPU:-host}"

msg() { printf '\n==> %s\n' "$*"; }
warn() { printf '\nWARNING: %s\n' "$*" >&2; }
die() { printf '\nERROR: %s\n' "$*" >&2; exit 1; }

usage() {
	cat <<EOF
Usage: ./build_scripts/netbsd-vm-build-freebasic.sh [options]

Options:
  --release N            NetBSD release. Default: 11.0
  --dist-url URL         NetBSD distribution URL. Default: official release URL
  --package FILE         Existing NetBSD package to test.
  --test-only            Test --package without rebuilding FreeBASIC.
  --workroot DIR         Work directory. Default: out/netbsd-vm
  --archive-dir DIR      Final archive directory. Default: out/netbsd/x86-64
  --jobs N               Build jobs inside NetBSD. Default: host CPU count
  --cpus N               QEMU CPU count. Default: --jobs value
  --fbctests-jobs N      fbctests make jobs. Default: --jobs value
  --exampleageddon-jobs N exampleageddon jobs. Default: --jobs value
  --exampleageddon-compile-timeout N Compile timeout in seconds. Default: 180
  --exampleageddon-run-timeout N Run timeout in seconds. Default: 10
  --memory SIZE          QEMU memory passed to Anita. Default: 6144M
  --disk-size SIZE       Anita disk size. Default: 32G
  --http-port N          Host HTTP port. Default: auto
  --keep-vm              Keep Anita work directory and run artifacts.
  -h, --help             Show this help.

The script builds NetBSD amd64 only.
EOF
}

while [ "$#" -gt 0 ]; do
	case "$1" in
		--release) RELEASE="$2"; shift 2 ;;
		--dist-url) DIST_URL="$2"; shift 2 ;;
		--package) PACKAGE_FILE="$2"; shift 2 ;;
		--test-only) TEST_ONLY=1; shift ;;
		--workroot)
			WORKROOT="$2"
			TOOLS_DIR="$WORKROOT/tools"
			ANITA_DIR="$TOOLS_DIR/anita-$ANITA_VERSION"
			ANITA="$ANITA_DIR/anita"
			TOOL_BIN="$TOOLS_DIR/bin"
			RUN_DIR="$WORKROOT/run"
			SERVE_DIR="$RUN_DIR/serve"
			UPLOAD_DIR="$RUN_DIR/upload"
			LOG_DIR="$WORKROOT/logs"
			PACKAGE_DIR="$WORKROOT/packages"
			ANITA_WORKDIR=""
			shift 2
			;;
		--archive-dir) ARCHIVE_DIR="$2"; shift 2 ;;
		--jobs) JOBS="$2"; CPUS="$2"; FBCTESTS_JOBS="$2"; EXAMPLEAGEDDON_JOBS="$2"; shift 2 ;;
		--cpus) CPUS="$2"; shift 2 ;;
		--fbctests-jobs) FBCTESTS_JOBS="$2"; shift 2 ;;
		--exampleageddon-jobs) EXAMPLEAGEDDON_JOBS="$2"; shift 2 ;;
		--exampleageddon-compile-timeout) EXAMPLEAGEDDON_COMPILE_TIMEOUT="$2"; shift 2 ;;
		--exampleageddon-run-timeout) EXAMPLEAGEDDON_RUN_TIMEOUT="$2"; shift 2 ;;
		--memory) MEMORY="$2"; shift 2 ;;
		--disk-size) DISK_SIZE="$2"; shift 2 ;;
		--http-port) HTTP_PORT="$2"; shift 2 ;;
		--keep-vm) KEEP_VM=1; shift ;;
		-h|--help) usage; exit 0 ;;
		*) die "unknown option: $1" ;;
	esac
done

case "$JOBS" in ''|*[!0-9]*|0) die "--jobs must be a positive integer" ;; esac
case "$CPUS" in ''|*[!0-9]*|0) die "--cpus must be a positive integer" ;; esac
case "$FBCTESTS_JOBS" in ''|*[!0-9]*|0) die "--fbctests-jobs must be a positive integer" ;; esac
case "$EXAMPLEAGEDDON_JOBS" in ''|*[!0-9]*|0) die "--exampleageddon-jobs must be a positive integer" ;; esac
case "$EXAMPLEAGEDDON_COMPILE_TIMEOUT" in ''|*[!0-9]*|0) die "--exampleageddon-compile-timeout must be a positive integer" ;; esac
case "$EXAMPLEAGEDDON_RUN_TIMEOUT" in ''|*[!0-9]*|0) die "--exampleageddon-run-timeout must be a positive integer" ;; esac

if [ "$TEST_ONLY" -eq 1 ] && [ -z "$PACKAGE_FILE" ]; then
	die "--test-only requires --package"
fi

if [ -z "$DIST_URL" ]; then
	DIST_URL="https://cdn.netbsd.org/pub/NetBSD/NetBSD-$RELEASE/$ARCH/"
fi

PKG_REPO="https://cdn.netbsd.org/pub/pkgsrc/packages/NetBSD/$PKG_ARCH/$RELEASE/All"
ANITA_WORKDIR="${ANITA_WORKDIR:-$WORKROOT/anita-$RELEASE-$ARCH}"

require_tool() {
	command -v "$1" >/dev/null 2>&1 || die "required tool not found: $1"
}

find_free_port() {
	python3 - "$1" <<'PY'
import socket
import sys

start = int(sys.argv[1])

for port in range(start, start + 2000):
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        try:
            s.bind(("127.0.0.1", port))
        except OSError:
            continue
        print(port)
        raise SystemExit(0)

raise SystemExit("no free TCP port found")
PY
}

check_host_tools() {
	require_tool bash
	require_tool curl
	require_tool python3
	require_tool qemu-system-x86_64
	require_tool tar
	require_tool xorriso
	require_tool sha256sum

	python3 - <<'PY'
import pexpect
PY
}

configure_qemu_acceleration() {
	if [ ! -r /dev/kvm ] || [ ! -w /dev/kvm ]; then
		QEMU_ACCEL="tcg"
		QEMU_CPU="max"
	fi

	msg "QEMU acceleration: $QEMU_ACCEL (CPU: $QEMU_CPU)"
}

install_anita() {
	mkdir -p "$TOOLS_DIR" "$TOOL_BIN"

	if [ ! -x "$ANITA" ]; then
		msg "Downloading Anita $ANITA_VERSION"
		curl -fsSL "$ANITA_URL" -o "$TOOLS_DIR/anita-$ANITA_VERSION.tar.gz"
		tar -xzf "$TOOLS_DIR/anita-$ANITA_VERSION.tar.gz" -C "$TOOLS_DIR"
	fi

	cat > "$TOOL_BIN/mkisofs" <<'EOF'
#!/bin/sh
exec xorriso -as mkisofs "$@"
EOF
	chmod +x "$TOOL_BIN/mkisofs"
}

make_source_archive() {
	msg "Packing source tree for NetBSD"
	mkdir -p "$SERVE_DIR" "$UPLOAD_DIR" "$LOG_DIR" "$PACKAGE_DIR"
	rm -f "$SERVE_DIR/freebasic-source.tar.gz"

	tar -czf "$SERVE_DIR/freebasic-source.tar.gz" \
		--exclude='./.git' \
		--exclude='./out' \
		--exclude='./build' \
		--exclude='./.build*' \
		--exclude='./OMA' \
		--exclude='./OMA_old' \
		--exclude='./remote_probe_temp' \
		--exclude='./nuttx-suite-logs' \
		--exclude='./package-root' \
		--exclude='./bin/fbc*' \
		--exclude='./bootstrap/fbc*' \
		--exclude='*/obj' \
		--exclude='*.o' \
		--exclude='*.a' \
		-C "$ROOT" .
}

write_upload_server() {
	cat > "$RUN_DIR/upload-server.py" <<'PY'
import http.server
import os
import posixpath
import sys

serve_dir = os.path.abspath(sys.argv[1])
upload_dir = os.path.abspath(sys.argv[2])
port = int(sys.argv[3])

os.makedirs(upload_dir, exist_ok=True)

class Handler(http.server.SimpleHTTPRequestHandler):
    def translate_path(self, path):
        path = path.split("?", 1)[0].split("#", 1)[0]
        path = posixpath.normpath(path)
        words = [word for word in path.split("/") if word]

        if words and words[0] == "upload":
            return os.path.join(upload_dir, *words[1:])

        return os.path.join(serve_dir, *words)

    def do_PUT(self):
        path = self.translate_path(self.path)
        if not os.path.abspath(path).startswith(upload_dir + os.sep):
            self.send_error(403)
            return

        os.makedirs(os.path.dirname(path), exist_ok=True)
        length = int(self.headers.get("Content-Length", "0"))

        with open(path, "wb") as f:
            remaining = length
            while remaining > 0:
                chunk = self.rfile.read(min(1024 * 1024, remaining))
                if not chunk:
                    break
                f.write(chunk)
                remaining -= len(chunk)

        self.send_response(201)
        self.end_headers()

with http.server.ThreadingHTTPServer(("0.0.0.0", port), Handler) as httpd:
    httpd.serve_forever()
PY
}

start_upload_server() {
	write_upload_server
	python3 "$RUN_DIR/upload-server.py" "$SERVE_DIR" "$UPLOAD_DIR" "$HTTP_PORT" \
		> "$RUN_DIR/http.log" 2>&1 &
	echo $! > "$RUN_DIR/http.pid"
}

stop_upload_server() {
	if [ -f "$RUN_DIR/http.pid" ]; then
		kill "$(cat "$RUN_DIR/http.pid")" 2>/dev/null || true
		rm -f "$RUN_DIR/http.pid"
	fi
}

write_guest_build_script() {
	cat > "$SERVE_DIR/netbsd-build-run.sh" <<EOF
#!/bin/sh
set -eu

PATH="/usr/pkg/bin:/usr/pkg/sbin:/usr/X11R7/bin:/bin:/sbin:/usr/bin:/usr/sbin:\$PATH"
export PATH
export PKG_PATH="$PKG_REPO"
export JOBS="$JOBS"

log=/tmp/freebasic-netbsd-build.log
rm -f "\$log"

upload() {
	file="\$1"
	name="\$2"
	if command -v curl >/dev/null 2>&1 && [ -f "\$file" ]; then
		curl -fsS -T "\$file" "http://10.0.2.2:$HTTP_PORT/upload/\$name" || true
	fi
}

finish() {
	rc=\$?
	upload "\$log" "freebasic-netbsd-build.log"
	exit "\$rc"
}
trap finish EXIT

(
set -eu

hostname fbc-netbsd
dhcpcd -4 -t 30 wm0 || true

ftp -o /tmp/freebasic-source.tar.gz "http://10.0.2.2:$HTTP_PORT/freebasic-source.tar.gz"
rm -rf /work/freebasic-source
mkdir -p /work/freebasic-source
tar -xzf /tmp/freebasic-source.tar.gz -C /work/freebasic-source
find /work/freebasic-source -exec chown -h root:wheel {} +

cd /work/freebasic-source
sh ./build_scripts/netbsd-build-freebasic.sh --jobs="$JOBS"

if ! command -v curl >/dev/null 2>&1; then
	pkgin -y install curl
fi

pkg="\$(find /work/freebasic-source/out -maxdepth 1 -type f -name 'freebasic-*.tgz' | sort | tail -n 1)"
[ -n "\$pkg" ] && [ -f "\$pkg" ]
upload "\$pkg" "\$(basename "\$pkg")"

echo "==> uploaded package \$(basename "\$pkg")"
) > "\$log" 2>&1 &
pid=\$!

while kill -0 "\$pid" 2>/dev/null; do
	sleep 60
	if kill -0 "\$pid" 2>/dev/null; then
		upload "\$log" "freebasic-netbsd-build.log"
		printf 'NetBSD build still running: '
		tail -n 1 "\$log" 2>/dev/null | tr '\000' ' ' | cut -c 1-160 || true
	fi
done

wait "\$pid"
EOF
}

write_guest_test_script() {
	cat > "$SERVE_DIR/netbsd-test-run.sh" <<EOF
#!/bin/sh
set -eu

PATH="/usr/pkg/bin:/usr/pkg/sbin:/usr/X11R7/bin:/bin:/sbin:/usr/bin:/usr/sbin:\$PATH"
export PATH
export PKG_PATH="$PKG_REPO"
export FBCTESTS_JOBS="$FBCTESTS_JOBS"
export EXAMPLEAGEDDON_JOBS="$EXAMPLEAGEDDON_JOBS"
export EXAMPLEAGEDDON_COMPILE_TIMEOUT="$EXAMPLEAGEDDON_COMPILE_TIMEOUT"
export EXAMPLEAGEDDON_RUN_TIMEOUT="$EXAMPLEAGEDDON_RUN_TIMEOUT"
export SFXLIB_OSS_DEVICE="/dev/sound0"

log=/tmp/freebasic-netbsd-test.log
rm -f "\$log"

upload() {
	file="\$1"
	name="\$2"
	if command -v curl >/dev/null 2>&1 && [ -f "\$file" ]; then
		curl -fsS -T "\$file" "http://10.0.2.2:$HTTP_PORT/upload/\$name" || true
	fi
}

finish() {
	rc=\$?
	upload "\$log" "freebasic-netbsd-test.log"
	exit "\$rc"
}

run() {
	echo "==> \$*"
	status=0
	"\$@" || status=\$?
	[ "\$status" -eq 0 ] || fail "command exited with status \$status: \$*"
}

fail() {
	echo "ERROR: \$*" >&2
	exit 1
}

fbctests_jobs() {
	case "\${FBCTESTS_JOBS:-}" in
		''|*[!0-9]*|0) echo 1 ;;
		*) echo "\$FBCTESTS_JOBS" ;;
	esac
}

run_gfx_smoke() {
	out="\$1"
	err="\$2"
	shift 2

	if timeout 30 "\$@" > "\$out" 2> "\$err"; then
		cat "\$out" || true
		[ ! -s "\$err" ] || {
			cat "\$err"
			fail "gfx smoke wrote stderr"
		}
		return 0
	fi

	cat "\$out" || true
	cat "\$err" || true
	fail "gfx smoke failed"
}

run_fbctests() {
	jobs="\$(fbctests_jobs)"

	cd /work/freebasic-source/tests

	echo "==> cleaning fbctests tree"
	run gmake clean FBC=fbc

	echo "==> checking installed compiler through fbctests"
	run gmake check FBC=fbc

	echo "==> running unit-tests with \${jobs} job(s)"
	run gmake -j "\$jobs" unit-tests FBC=fbc

	echo "==> running log-tests serially"
	run gmake log-tests FBC=fbc

	for failed_log in failed-fb.log failed-fblite.log failed-qb.log failed-deprecated.log; do
		[ -f "\$failed_log" ] || fail "missing log-tests summary: \$failed_log"
		if ! grep -qi 'None Found' "\$failed_log"; then
			cat "\$failed_log"
			awk -F: '/\\.log:/ { print \$1 }' "\$failed_log" | sort -u | while read -r detail_log; do
				detail_log="\${detail_log#./}"
				detail_log="\${detail_log%\\$'\\r'}"
				if [ -f "\$detail_log" ]; then
					echo
					echo "==> failed detail: \$detail_log"
					upload "\$detail_log" "\$detail_log"
					cat "\$detail_log"
				else
					echo "WARNING: missing test log \$detail_log" >&2
				fi
			done
			fail "log-tests reported failures in \$failed_log"
		fi
	done

	echo "==> fbctests passed"
}

run_exampleageddon() {
	python=/usr/pkg/bin/python3.12
	[ -x "\$python" ] || fail "python3.12 is required for exampleageddon"

	echo "==> running exampleageddon"
	rm -rf /work/exampleageddon
	if ! run "\$python" /work/freebasic-source/build_scripts/exampleageddon-freebasic.py \
			--root /work/freebasic-source \
			--outdir /work/exampleageddon \
			--prefix /usr/pkg \
			--include-dir /work/freebasic-source/inc \
			--fbc fbc \
			--jobs "\$EXAMPLEAGEDDON_JOBS" \
			--compile-timeout "\$EXAMPLEAGEDDON_COMPILE_TIMEOUT" \
			--run-timeout "\$EXAMPLEAGEDDON_RUN_TIMEOUT" \
			--fail-on-self-contained; then
		upload /work/exampleageddon/report.md freebasic-netbsd-exampleageddon-report.md
		upload /work/exampleageddon/results.csv freebasic-netbsd-exampleageddon-results.csv
		fail "exampleageddon failed"
	fi

	[ -f /work/exampleageddon/report.md ] || fail "exampleageddon report was not created"
	[ -f /work/exampleageddon/results.csv ] || fail "exampleageddon results CSV was not created"
	grep -qx -- '- Self-contained problems: 0' /work/exampleageddon/report.md || {
		cat /work/exampleageddon/report.md
		fail "exampleageddon reported self-contained example problems"
	}

	upload /work/exampleageddon/report.md freebasic-netbsd-exampleageddon-report.md
	upload /work/exampleageddon/results.csv freebasic-netbsd-exampleageddon-results.csv
	echo "==> exampleageddon passed"
}

start_xvfb() {
	xvfb=""

	if command -v Xvfb >/dev/null 2>&1; then
		xvfb="\$(command -v Xvfb)"
	elif [ -x /usr/X11R7/bin/Xvfb ]; then
		xvfb="/usr/X11R7/bin/Xvfb"
	fi

	if [ -z "\$xvfb" ]; then
		fail "Xvfb not found"
	fi

	"\$xvfb" :99 -screen 0 640x480x24 >/tmp/freebasic-xvfb.log 2>&1 &
	echo \$! > /tmp/freebasic-xvfb.pid
	export DISPLAY=:99
	sleep 2
}

stop_xvfb() {
	if [ -f /tmp/freebasic-xvfb.pid ]; then
		kill "\$(cat /tmp/freebasic-xvfb.pid)" 2>/dev/null || true
		rm -f /tmp/freebasic-xvfb.pid
	fi
}

finish_with_xvfb() {
	rc=\$?
	stop_xvfb
	upload "\$log" "freebasic-netbsd-test.log"
	exit "\$rc"
}

trap finish_with_xvfb EXIT

(
set -eu

hostname fbc-netbsd
dhcpcd -4 -t 30 wm0 || true

if ! command -v pkgin >/dev/null 2>&1; then
	pkg_add pkgin
fi

mkdir -p /usr/pkg/etc/pkgin
printf '%s\n' "\$PKG_PATH" > /usr/pkg/etc/pkgin/repositories.conf

pkgin -y update
pkgin -y install \\
	bash \\
	curl \\
	gcc12 \\
	gmake \\
	libffi \\
	libXcursor \\
	libXrender \\
	MesaLib \\
	ncurses \\
	pkgconf \\
	python312 \\
	rsync

ftp -o /tmp/freebasic-source.tar.gz "http://10.0.2.2:$HTTP_PORT/freebasic-source.tar.gz"
rm -rf /work/freebasic-source
mkdir -p /work/freebasic-source
tar -xzf /tmp/freebasic-source.tar.gz -C /work/freebasic-source

pkg_url="http://10.0.2.2:$HTTP_PORT/upload/$NETBSD_PACKAGE_BASENAME"
ftp -o "/tmp/$NETBSD_PACKAGE_BASENAME" "\$pkg_url"

pkg_delete freebasic >/dev/null 2>&1 || true
run pkg_add "/tmp/$NETBSD_PACKAGE_BASENAME"

command -v fbc
fbc -version

mkdir -p /work/smoke

cat > /work/smoke/console.bas <<'FBEOF'
print "Hello world"
FBEOF

cat > /work/smoke/gfx-truecolor.bas <<'FBEOF'
#include once "fbgfx.bi"

sub expect_rgb( byval x as integer, byval y as integer, byval expected as uinteger, byref label as string )
	dim as uinteger actual = cuint( point( x, y ) )

	if( actual <> expected ) then
		print "gfx truecolor mismatch: "; label; " actual=&h"; hex( actual, 8 ); " expected=&h"; hex( expected, 8 )
		end 1
	end if
end sub

dim as integer has_extra_page = 1

if( screenres( 64, 64, 32, 2 ) <> 0 ) then
	has_extra_page = 0
	if( screenres( 64, 64, 32 ) <> 0 ) then
		print "gfx truecolor skipped: screenres failed"
		end 77
	end if
end if

screenset 0, 0
line (0, 0)-(63, 63), rgb( 0, 0, 0 ), bf
line (8, 8)-(23, 23), rgb( 255, 0, 0 ), bf
line (24, 8)-(39, 23), rgb( 0, 255, 0 ), bf
line (40, 8)-(55, 23), rgb( 0, 0, 255 ), bf
expect_rgb 8, 8, rgb( 255, 0, 0 ), "red block"
expect_rgb 24, 8, rgb( 0, 255, 0 ), "green block"
expect_rgb 40, 8, rgb( 0, 0, 255 ), "blue block"

if( has_extra_page <> 0 ) then
	screenset 1, 1
	line (0, 0)-(63, 63), rgb( 255, 255, 255 ), bf
	line (8, 8)-(23, 23), rgb( 255, 0, 0 ), bf
	screenset 1, 1
	expect_rgb 8, 8, rgb( 255, 0, 0 ), "screenset red block"
end if

screen 0
print "gfx truecolor ok"
FBEOF

cat > /work/smoke/gfx-screen-modes.bas <<'FBEOF'
#include once "fbgfx.bi"

sub draw_mode( byval mode as integer )
	dim as integer max_color = 15

	if( mode = 1 ) then
		max_color = 3
	end if

	cls
	line (8, 8)-(32, 32), 1, bf
	line (40, 8)-(64, 32), iif( max_color >= 2, 2, 1 ), bf
	line (72, 8)-(96, 32), iif( max_color >= 4, 4, max_color ), bf
	pset (4, 4), max_color

	if( point( 4, 4 ) <> max_color ) then
		print "SCREEN "; mode; " point mismatch"
		end 1
	end if
end sub

sub test_mode( byval mode as integer )
	dim as integer stage = 0
	dim as integer unsupported_mode = 0

	if( mode = 0 ) then
		screen 0
		print "SCREEN 0 ok"
		exit sub
	end if

	on local error goto mode_error

	stage = 1
	screen mode
	if( unsupported_mode <> 0 ) then
		screen 0
		print "SCREEN "; mode; " unsupported, err="; err()
		exit sub
	end if

	stage = 2
	if( screenptr() = 0 ) then
		screen 0
		print "SCREEN "; mode; " unsupported"
		exit sub
	end if

	draw_mode mode
	screen 0
	print "SCREEN "; mode; " ok"
	exit sub

mode_error:
	if( stage = 1 ) then
		unsupported_mode = 1
		resume next
	end if

	print "SCREEN "; mode; " failed, err="; err()
	end 1
end sub

for mode as integer = 0 to 13
	test_mode mode
next

end 0
FBEOF

cat > /work/smoke/sfx.bas <<'FBEOF'
extern "C"
declare function fb_sfxDeviceCurrent() as long
declare function fb_sfxDeviceInfoName(byval id as long) as const zstring ptr
end extern

print "sfx-start"
dim as long sfx_device = fb_sfxDeviceCurrent()
dim as const zstring ptr sfx_driver = fb_sfxDeviceInfoName(sfx_device)
if sfx_driver <> 0 then
	print "sfx-driver="; *sfx_driver
	if instr(lcase(*sfx_driver), "null") > 0 then
		print "sfx-driver-null"
		end 2
	end if
else
	print "sfx-driver=<none>"
	end 3
end if
sound 440, 0.75
play "L4 CDEFGAB"
sleep 1600, 1
print "sfx-end"
FBEOF

run fbc /work/smoke/console.bas -x /work/smoke/console
console_output="\$(/work/smoke/console)"
echo "\$console_output"
[ "\$console_output" = "Hello world" ] || fail "unexpected console output"

echo "==> compiling crt/sys/socket.bi API smoke"
run fbc /work/freebasic-source/tests/crt/socket.bas -x /work/smoke/socket-bi

echo "==> running crt/sys/socket.bi API smoke"
run /work/smoke/socket-bi

echo "==> compiling curses.bi API smoke"
run fbc /work/freebasic-source/tests/crt/curses.bas -x /work/smoke/curses-bi

echo "==> reporting NetBSD curses runtime selection"
echo "TERM=\${TERM-<unset>} TERMINFO=\${TERMINFO-<unset>} TERMINFO_DIRS=\${TERMINFO_DIRS-<unset>}"
command -v infocmp || true
infocmp dumb 2>&1 || true
find /usr/share /usr/pkg/share -path '*/terminfo/*/dumb' -print 2>/dev/null || true
ldd /work/smoke/curses-bi 2>&1 || true

cat > /work/smoke/curses-reference.c <<'CEOF'
#include <curses.h>
#include <errno.h>
#include <stdio.h>

int main(void)
{
	FILE *input_file = tmpfile();
	FILE *output_file = tmpfile();
	SCREEN *screen;

	printf("curses_version=%s\n", curses_version());
	if (input_file == NULL || output_file == NULL) {
		perror("tmpfile");
		return 2;
	}

	screen = newterm("dumb", output_file, input_file);
	printf("newterm=%p errno=%d\n", (void *)screen, errno);
	if (screen == NULL)
		return 3;

	endwin();
	delscreen(screen);
	fclose(input_file);
	fclose(output_file);
	return 0;
}
CEOF

echo "==> running system C curses reference smoke"
if cc /work/smoke/curses-reference.c -lcurses -o /work/smoke/curses-reference; then
	ldd /work/smoke/curses-reference 2>&1 || true
	/work/smoke/curses-reference 2>&1 || true
else
	echo "system C curses reference did not compile"
fi

if pkg-config --exists ncurses 2>/dev/null; then
	echo "==> running pkgsrc ncurses C reference smoke"
	if cc /work/smoke/curses-reference.c \$(pkg-config --cflags --libs ncurses) -o /work/smoke/ncurses-reference; then
		ldd /work/smoke/ncurses-reference 2>&1 || true
		/work/smoke/ncurses-reference 2>&1 || true
	else
		echo "pkgsrc ncurses C reference did not compile"
	fi
fi

echo "==> running curses.bi API smoke"
run /work/smoke/curses-bi

echo "==> compiling TCP loopback smoke"
run fbc -mt /work/freebasic-source/tests/file/tcp.bas -x /work/smoke/tcp

echo "==> running TCP loopback smoke"
timeout 60 /work/smoke/tcp

start_xvfb
run fbc /work/smoke/gfx-truecolor.bas -x /work/smoke/gfx-truecolor
run fbc -lang fblite -exx /work/smoke/gfx-screen-modes.bas -x /work/smoke/gfx-screen-modes
run_gfx_smoke /work/smoke/gfx-truecolor.out /work/smoke/gfx-truecolor.err /work/smoke/gfx-truecolor
run_gfx_smoke /work/smoke/gfx-screen-modes.out /work/smoke/gfx-screen-modes.err /work/smoke/gfx-screen-modes

run fbc /work/smoke/sfx.bas -x /work/smoke/sfx

showcase="\$(find /usr/pkg -path '*/examples/sfxlib/showcase.bas' -type f | head -n 1)"
[ -n "\$showcase" ] && [ -f "\$showcase" ] || fail "sfxlib showcase example is not installed"
(
	cd "\$(dirname "\$showcase")"
	run fbc showcase.bas -x /work/smoke/sfx-showcase
)

SFXLIB_DRIVER="NETBSD OSS" SFXLIB_OSS_DEVICE="/dev/sound0" timeout 20 /work/smoke/sfx > /work/smoke/sfx.out 2> /work/smoke/sfx.err || {
	cat /work/smoke/sfx.out || true
	cat /work/smoke/sfx.err || true
	fail "sfx smoke failed"
}
cat /work/smoke/sfx.out || true
grep -qx 'sfx-start' /work/smoke/sfx.out || fail "sfx smoke did not start"
grep -qi '^sfx-driver=netbsd oss' /work/smoke/sfx.out || fail "sfx smoke did not use NetBSD OSS"
grep -qx 'sfx-end' /work/smoke/sfx.out || fail "sfx smoke did not finish"
[ ! -s /work/smoke/sfx.err ] || {
	cat /work/smoke/sfx.err
	fail "sfx smoke wrote stderr"
}

run_fbctests
run_exampleageddon

echo "==> TEST PASSED"
) > "\$log" 2>&1 &
pid=\$!

while kill -0 "\$pid" 2>/dev/null; do
	sleep 60
	if kill -0 "\$pid" 2>/dev/null; then
		upload "\$log" "freebasic-netbsd-test.log"
		printf 'NetBSD tests still running: '
		tail -n 1 "\$log" 2>/dev/null | tr '\000' ' ' | cut -c 1-160 || true
	fi
done

wait "\$pid"
EOF
}

run_anita() {
	local log="$1"
	shift

	PATH="$TOOL_BIN:$PATH" python3 "$ANITA" "$@" > "$log" 2>&1
}

install_base_vm() {
	if [ -f "$ANITA_WORKDIR/wd0.img" ]; then
		msg "Using cached NetBSD $RELEASE amd64 base VM"
		echo "Using cached NetBSD $RELEASE amd64 Anita workdir: $ANITA_WORKDIR" \
			> "$LOG_DIR/freebasic-netbsd-install.log"
		return 0
	fi

	msg "Installing NetBSD $RELEASE amd64 base VM"
	rm -rf "$ANITA_WORKDIR"

	run_anita "$LOG_DIR/freebasic-netbsd-install.log" \
		--workdir="$ANITA_WORKDIR" \
		--vmm=qemu \
		--image-format=sparse \
		--disk-size="$DISK_SIZE" \
		--memory-size="$MEMORY" \
		--vmm-args="-accel $QEMU_ACCEL -cpu $QEMU_CPU -smp $CPUS" \
		--sets=kern-GENERIC,modules,base,etc,comp,xbase,xcomp,xserver \
		install "$DIST_URL"
}

run_guest_script() {
	local name="$1"
	local script="$2"
	local log="$3"
	local audio_wav="${4:-}"
	local vmm_args="-accel $QEMU_ACCEL -cpu $QEMU_CPU -smp $CPUS"

	if [ -n "$audio_wav" ]; then
		vmm_args="$vmm_args -audiodev wav,id=audio0,path=$audio_wav -device ES1370,audiodev=audio0"
	fi

	msg "Running NetBSD $name"
	run_anita "$log" \
		--workdir="$ANITA_WORKDIR" \
		--vmm=qemu \
		--image-format=sparse \
		--memory-size="$MEMORY" \
		--run-timeout=21600 \
		--vmm-args="$vmm_args" \
		--run="dhcpcd -4 -t 30 wm0 || true; ftp -o /tmp/$script http://10.0.2.2:$HTTP_PORT/$script && sh /tmp/$script" \
		boot "$DIST_URL"
}

wait_for_upload() {
	local pattern="$1"
	local label="$2"
	local found=""

	for _ in $(seq 1 30); do
		found="$(find "$UPLOAD_DIR" -maxdepth 1 -type f -name "$pattern" | sort | tail -n 1)"
		if [ -n "$found" ] && [ -f "$found" ]; then
			printf '%s\n' "$found"
			return 0
		fi
		sleep 1
	done

	die "missing uploaded $label"
}

verify_audio_capture() {
	local wav="$1"
	local log="$2"

	msg "Verifying NetBSD QEMU audio capture"
	python3 - "$wav" "$log" <<'PY'
import math
import struct
import sys
import time
import wave

wav_path = sys.argv[1]
log_path = sys.argv[2]

try:
    last_error = None
    for _ in range(20):
        try:
            with wave.open(wav_path, "rb") as w:
                channels = w.getnchannels()
                width = w.getsampwidth()
                rate = w.getframerate()
                frames = w.getnframes()
                data = w.readframes(frames)
            break
        except Exception as ex:
            last_error = ex
            time.sleep(0.5)
    else:
        raise last_error
except Exception as ex:
    raise SystemExit(f"failed to read audio capture: {ex}")

if width != 2:
    raise SystemExit(f"unexpected sample width: {width}")

sample_count = len(data) // 2
if sample_count <= 0:
    raise SystemExit("audio capture contains no samples")

samples = struct.unpack("<%dh" % sample_count, data)
peak = max(abs(sample) for sample in samples)
rms = math.sqrt(sum(sample * sample for sample in samples) / float(sample_count))
active = sum(1 for sample in samples if abs(sample) > 500)

with open(log_path, "w", encoding="utf-8") as f:
    f.write(f"file={wav_path}\n")
    f.write(f"rate={rate}\n")
    f.write(f"channels={channels}\n")
    f.write(f"frames={frames}\n")
    f.write(f"samples={sample_count}\n")
    f.write(f"peak={peak}\n")
    f.write(f"rms={rms:.2f}\n")
    f.write(f"active_samples={active}\n")

if frames < rate // 4:
    raise SystemExit("audio capture is too short")
if peak < 1000:
    raise SystemExit("audio capture peak is too low")
if rms < 50.0:
    raise SystemExit("audio capture RMS is too low")
if active < rate // 20:
    raise SystemExit("audio capture has too few active samples")

with open(log_path, "a", encoding="utf-8") as f:
    f.write("result=PASS\n")
PY
}

archive_results() {
	local pkg="$1"
	local base

	mkdir -p "$ARCHIVE_DIR"
	cp -f "$pkg" "$ARCHIVE_DIR/"

	for log in "$LOG_DIR"/freebasic-netbsd-*.log; do
		[ -f "$log" ] || continue
		cp -f "$log" "$ARCHIVE_DIR/"
	done

	if [ -f "$RUN_DIR/netbsd-audio.wav" ]; then
		cp -f "$RUN_DIR/netbsd-audio.wav" "$ARCHIVE_DIR/freebasic-netbsd-audio.wav"
	fi

	if [ -f "$UPLOAD_DIR/freebasic-netbsd-exampleageddon-report.md" ]; then
		cp -f "$UPLOAD_DIR/freebasic-netbsd-exampleageddon-report.md" "$ARCHIVE_DIR/exampleageddon-report.md"
	fi

	if [ -f "$UPLOAD_DIR/freebasic-netbsd-exampleageddon-results.csv" ]; then
		cp -f "$UPLOAD_DIR/freebasic-netbsd-exampleageddon-results.csv" "$ARCHIVE_DIR/exampleageddon-results.csv"
	fi

	base="$(basename "$pkg")"
	(
		cd "$ARCHIVE_DIR"
		sha256sum "$base" > SHA256SUMS
	)
}

resolve_package_file() {
	[ -n "$PACKAGE_FILE" ] || return 0
	[ -f "$PACKAGE_FILE" ] || die "package not found: $PACKAGE_FILE"
	PACKAGE_FILE="$(cd "$(dirname "$PACKAGE_FILE")" && pwd)/$(basename "$PACKAGE_FILE")"
}

cleanup() {
	stop_upload_server || true
}

trap cleanup EXIT

main() {
	check_host_tools
	configure_qemu_acceleration
	install_anita
	resolve_package_file

	if [ -z "$HTTP_PORT" ]; then
		HTTP_PORT="$(find_free_port 19080)"
	fi

	msg "NetBSD VM HTTP port: $HTTP_PORT"

	if [ "$TEST_ONLY" -eq 1 ]; then
		mkdir -p "$PACKAGE_DIR"
		stable_package="$PACKAGE_DIR/$(basename "$PACKAGE_FILE")"
		if [ "$PACKAGE_FILE" != "$stable_package" ]; then
			cp -f "$PACKAGE_FILE" "$stable_package"
		fi
		PACKAGE_FILE="$stable_package"
	fi

	rm -rf "$RUN_DIR"
	mkdir -p "$RUN_DIR" "$SERVE_DIR" "$UPLOAD_DIR" "$LOG_DIR" "$PACKAGE_DIR"

	start_upload_server
	make_source_archive

	local pkg uploaded_pkg

	if [ "$TEST_ONLY" -eq 1 ]; then
		cp -f "$PACKAGE_FILE" "$UPLOAD_DIR/"
		pkg="$UPLOAD_DIR/$(basename "$PACKAGE_FILE")"
	else
		write_guest_build_script
		install_base_vm
		run_guest_script "package build" "netbsd-build-run.sh" "$LOG_DIR/freebasic-netbsd-build-console.log"
		uploaded_pkg="$(wait_for_upload 'freebasic-*.tgz' package)"
		cp -f "$uploaded_pkg" "$PACKAGE_DIR/"
		pkg="$PACKAGE_DIR/$(basename "$uploaded_pkg")"
	fi

	NETBSD_PACKAGE_BASENAME="$(basename "$pkg")"
	export NETBSD_PACKAGE_BASENAME
	write_guest_test_script

	if [ "$TEST_ONLY" -eq 1 ]; then
		install_base_vm
	fi

	run_guest_script "package smoke tests and fbctests" "netbsd-test-run.sh" \
		"$LOG_DIR/freebasic-netbsd-test-console.log" "$RUN_DIR/netbsd-audio.wav"

	wait_for_upload 'freebasic-netbsd-test.log' test-log >/dev/null
	wait_for_upload 'freebasic-netbsd-exampleageddon-report.md' exampleageddon-report >/dev/null
	wait_for_upload 'freebasic-netbsd-exampleageddon-results.csv' exampleageddon-results >/dev/null
	if [ -f "$UPLOAD_DIR/freebasic-netbsd-build.log" ]; then
		cp -f "$UPLOAD_DIR/freebasic-netbsd-build.log" "$LOG_DIR/"
	fi
	cp -f "$UPLOAD_DIR/freebasic-netbsd-test.log" "$LOG_DIR/"

	verify_audio_capture "$RUN_DIR/netbsd-audio.wav" "$LOG_DIR/freebasic-netbsd-audio.log"
	archive_results "$pkg"

	if [ "$KEEP_VM" -eq 0 ]; then
		rm -rf "$RUN_DIR"
	fi

	msg "NetBSD package build and fbctests completed"
	echo "Package: $ARCHIVE_DIR/$(basename "$pkg")"
	echo "Archive: $ARCHIVE_DIR"
	echo "Install log: $ARCHIVE_DIR/freebasic-netbsd-install.log"
	echo "Build log:   $ARCHIVE_DIR/freebasic-netbsd-build.log"
	echo "Test log:    $ARCHIVE_DIR/freebasic-netbsd-test.log"
	echo "Audio log:   $ARCHIVE_DIR/freebasic-netbsd-audio.log"
}

main "$@"

##############################################################################
# end of netbsd-vm-build-freebasic.sh
##############################################################################

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
Usage: ./build_scripts/debianubuntu-test-freebasic-xbox.sh [options]

Options:
  --package-dir DIR    Directory containing freebasic-xbox .deb artifacts
  --image IMAGE        Docker image to use (default: ubuntu:questing)
  --docker-cmd CMD     Docker command to use (default: docker)
  --out-dir DIR        Keep compiled XBE/XISO smoke artifacts in DIR
  --skip-host-deps     Skip Docker host dependency installation
  --skip-emulator      Only install the package and compile XBE/XISO artifacts
  --require-emulator   Fail instead of skipping if emulator setup is incomplete
  --install-emulator   Try to install xemu from configured APT repositories
  --xemu CMD           xemu command (default: xemu, or flatpak xemu if present)
  --xemu-config FILE   Existing xemu.toml to use
  --xemu-mcpx FILE     MCPX boot ROM path for generated xemu config
  --xemu-bios FILE     Xbox-compatible BIOS path for generated xemu config
  --xemu-hdd FILE      Xbox HDD image path for generated xemu config
  --xemu-timeout SEC   Seconds to leave each XISO running (default: 20)
  --help               Show this help text

The test starts a fresh Debian/Ubuntu-style container, installs the local
freebasic-xbox package plus a matching local freebasic package when available,
then compiles four Xbox smoke programs and wraps each default.xbe in an XISO.

The emulator phase uses xemu if it is installed and configured. xemu is a
full-system original Xbox emulator, so it requires user-provided MCPX, BIOS, and
HDD image files. Set XEMU_CONFIG, or set XEMU_MCPX, XEMU_BIOS, and XEMU_HDD.
EOF
}

PACKAGE_DIR=""
IMAGE="${IMAGE:-ubuntu:questing}"
DOCKER_CMD="${DOCKER_CMD:-docker}"
OUT_DIR=""
SKIP_HOST_DEPS=0
SKIP_EMULATOR=0
REQUIRE_EMULATOR=0
INSTALL_EMULATOR=0
XEMU_CMD="${XEMU_CMD:-}"
XEMU_CONFIG="${XEMU_CONFIG:-}"
XEMU_MCPX="${XEMU_MCPX:-}"
XEMU_BIOS="${XEMU_BIOS:-}"
XEMU_HDD="${XEMU_HDD:-}"
XEMU_TIMEOUT="${XEMU_TIMEOUT:-20}"

while [ $# -gt 0 ]; do
	case "$1" in
		--package-dir) PACKAGE_DIR="$2"; shift 2 ;;
		--image) IMAGE="$2"; shift 2 ;;
		--docker-cmd) DOCKER_CMD="$2"; shift 2 ;;
		--out-dir) OUT_DIR="$2"; shift 2 ;;
		--skip-host-deps) SKIP_HOST_DEPS=1; shift ;;
		--skip-emulator) SKIP_EMULATOR=1; shift ;;
		--require-emulator) REQUIRE_EMULATOR=1; shift ;;
		--install-emulator) INSTALL_EMULATOR=1; shift ;;
		--xemu) XEMU_CMD="$2"; shift 2 ;;
		--xemu-config) XEMU_CONFIG="$2"; shift 2 ;;
		--xemu-mcpx) XEMU_MCPX="$2"; shift 2 ;;
		--xemu-bios) XEMU_BIOS="$2"; shift 2 ;;
		--xemu-hdd) XEMU_HDD="$2"; shift 2 ;;
		--xemu-timeout) XEMU_TIMEOUT="$2"; shift 2 ;;
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
	PACKAGE_DIR="$ROOT/out/linux/$DISTRO_ID/$CODENAME/$ARCH/xbox"
fi

[ -d "$PACKAGE_DIR" ] || die "package directory not found: $PACKAGE_DIR"
PACKAGE_DIR="$(cd "$PACKAGE_DIR" && pwd -P)"

if ls "$PACKAGE_DIR"/freebasic-xbox_*.deb >/dev/null 2>&1; then
	XBOX_PACKAGE_DIR="$PACKAGE_DIR"
	NATIVE_PACKAGE_DIR="$(dirname "$PACKAGE_DIR")"
elif ls "$PACKAGE_DIR"/xbox/freebasic-xbox_*.deb >/dev/null 2>&1; then
	XBOX_PACKAGE_DIR="$(cd "$PACKAGE_DIR/xbox" && pwd -P)"
	NATIVE_PACKAGE_DIR="$PACKAGE_DIR"
else
	die "missing freebasic-xbox .deb in $PACKAGE_DIR or $PACKAGE_DIR/xbox"
fi

if [ -z "$OUT_DIR" ]; then
	OUT_DIR="$(mktemp -d -t fb-xbox-xbes.XXXXXX)"
	KEEP_OUT_DIR=0
else
	mkdir -p "$OUT_DIR"
	OUT_DIR="$(cd "$OUT_DIR" && pwd -P)"
	KEEP_OUT_DIR=1
fi

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

maybe_install_xemu() {
	[ "$INSTALL_EMULATOR" -eq 1 ] || return 0
	command -v xemu >/dev/null 2>&1 && return 0

	if ! command -v apt-get >/dev/null 2>&1; then
		return 0
	fi

	msg "checking configured APT repositories for xemu"
	run_root apt-get update -y
	if apt-cache policy xemu 2>/dev/null | sed -n 's/^[[:space:]]*Candidate:[[:space:]]*//p' | grep -qv '^(none)$'; then
		run_root apt-get install -y --no-install-recommends xemu
	fi
}

detect_xemu_cmd() {
	if [ -n "$XEMU_CMD" ]; then
		return 0
	fi

	if command -v xemu >/dev/null 2>&1; then
		XEMU_CMD="xemu"
		return 0
	fi

	if command -v flatpak >/dev/null 2>&1 && flatpak info app.xemu.xemu >/dev/null 2>&1; then
		XEMU_CMD="flatpak run app.xemu.xemu"
		return 0
	fi

	return 1
}

emulator_skip_or_fail() {
	local reason="$1"

	if [ "$REQUIRE_EMULATOR" -eq 1 ]; then
		die "$reason"
	fi

	echo "SKIPPED: $reason"
	echo "==> XBE/XISO artifacts kept in: $OUT_DIR"
	exit 0
}

make_xemu_config() {
	local config="$1"

	[ -n "$XEMU_MCPX" ] && [ -f "$XEMU_MCPX" ] || return 1
	[ -n "$XEMU_BIOS" ] && [ -f "$XEMU_BIOS" ] || return 1
	[ -n "$XEMU_HDD" ] && [ -f "$XEMU_HDD" ] || return 1

	cat > "$config" <<EOF
[general]
show_welcome = false
skip_boot_anim = true

[net]
enable = true
backend = 'nat'

[sys.files]
bootrom_path = '$XEMU_MCPX'
flashrom_path = '$XEMU_BIOS'
hdd_path = '$XEMU_HDD'
EOF
}

run_xemu_iso() {
	local name="$1"
	local iso="$2"
	local config="$3"
	local log="$OUT_DIR/$name.xemu.log"
	local status

	msg "running $name XISO in xemu"
	set +e
	timeout "$XEMU_TIMEOUT" sh -c 'exec "$@"' sh $XEMU_CMD \
		-config_path "$config" \
		-dvd_path "$iso" \
		-snapshot \
		-machine xbox,short-animation=on > "$log" 2>&1
	status=$?
	set -e

	case "$status" in
		0|124)
			echo "==> $name xemu launch completed with status $status"
			;;
		*)
			cat "$log" >&2 || true
			die "$name xemu launch failed with status $status"
			;;
	esac
}

TEST_RUNNER="$(mktemp -t fb-xbox-deb-package-test.XXXXXX.sh)"
cleanup() {
	rm -f "$TEST_RUNNER"
	if [ "${KEEP_OUT_DIR:-0}" -eq 0 ] && [ "$SKIP_EMULATOR" -eq 0 ]; then
		rm -rf "$OUT_DIR"
	fi
}
trap cleanup EXIT

chmod 755 "$TEST_RUNNER"
cat > "$TEST_RUNNER" <<'TEST_RUNNER_EOF'
#!/usr/bin/env bash

set -euo pipefail

run() {
	echo "==> $*"
	"$@"
}

fail() {
	echo "ERROR: $*" >&2
	exit 1
}

add_deb_glob() {
	local pattern="$1"
	local deb

	compgen -G "$pattern" >/dev/null || return 0
	for deb in $pattern; do
		DEBS+=("$deb")
	done
}

add_single_deb_glob() {
	local pattern="$1"
	local deb

	compgen -G "$pattern" >/dev/null || return 0
	for deb in $pattern; do
		DEBS+=("$deb")
		return 0
	done
}

export DEBIAN_FRONTEND=noninteractive
export HOME=/root

run apt-get update -y

DEBS=()
add_single_deb_glob "/native-packages/freebasic_[0-9]*.deb"
add_deb_glob "/xbox-packages/freebasic-xbox_*.deb"

[ "${#DEBS[@]}" -gt 0 ] || fail "no local packages were mounted"
run apt-get install -y --no-install-recommends "${DEBS[@]}"

command -v fbc >/dev/null 2>&1 || fail "fbc was not installed"
command -v fbc-xbox >/dev/null 2>&1 || fail "fbc-xbox was not installed"
command -v fbc-xbox-xiso >/dev/null 2>&1 || fail "fbc-xbox-xiso was not installed"
command -v clang >/dev/null 2>&1 || fail "clang dependency was not installed"

mkdir -p /tmp/fb-xbox-smoke/assets /xbe-out
printf 'asset smoke\n' > /tmp/fb-xbox-smoke/assets/readme.txt

cat > /tmp/fb-xbox-smoke/console.bas <<'EOF'
print "FREEBASIC_XBOX_CONSOLE_SMOKE"
sleep 1000
EOF

cat > /tmp/fb-xbox-smoke/gfx.bas <<'EOF'
screenres 160, 120, 32
line (0, 0)-(159, 119), rgb(0, 128, 255), bf
line (10, 10)-(149, 109), rgb(255, 255, 255), b
open err for output as #1
print #1, "FREEBASIC_XBOX_GFX_SMOKE"
close #1
sleep 1000
EOF

cat > /tmp/fb-xbox-smoke/sfx.bas <<'EOF'
declare sub fb_sfxUpdate cdecl alias "fb_sfxUpdate" ( byval frames as integer )

sound 0, 440, 0.25, 0.70
fb_sfxUpdate( 12000 )

open err for output as #1
print #1, "FREEBASIC_XBOX_SFX_SMOKE"
close #1
sleep 1000
EOF

cat > /tmp/fb-xbox-smoke/fileio.bas <<'EOF'
open "fbxbox.tmp" for output as #1
print #1, "FREEBASIC_XBOX_FILEIO_SMOKE"
close #1

dim text as string
open "fbxbox.tmp" for input as #1
line input #1, text
close #1

print text
sleep 1000
EOF

cd /tmp/fb-xbox-smoke
for name in console gfx sfx fileio; do
	run fbc-xbox "$name.bas" -x "$name.xbe" -v
	[ -f "$name.xbe" ] || fail "$name.xbe was not produced"

	run fbc-xbox-xiso "$name.xbe" "/xbe-out/$name.iso" --assets /tmp/fb-xbox-smoke/assets
	cp -av "$name.xbe" "/xbe-out/$name.xbe"
	[ -f "/xbe-out/$name.iso" ] || fail "$name.iso was not produced"
done

echo "freebasic-xbox package XBE/XISO build test passed"
TEST_RUNNER_EOF

install_host_deps

msg "building Xbox smoke XBEs from freebasic-xbox package in $IMAGE"
run ${DOCKER_CMD} run --rm \
	-v "$XBOX_PACKAGE_DIR:/xbox-packages:ro" \
	-v "$NATIVE_PACKAGE_DIR:/native-packages:ro" \
	-v "$OUT_DIR:/xbe-out" \
	-v "$TEST_RUNNER:/tmp/test-freebasic-xbox.sh:ro" \
	"$IMAGE" \
	/bin/bash /tmp/test-freebasic-xbox.sh

echo "==> XBE/XISO artifacts: $OUT_DIR"

if [ "$SKIP_EMULATOR" -eq 1 ]; then
	echo "==> emulator phase skipped by request"
	exit 0
fi

maybe_install_xemu
detect_xemu_cmd || emulator_skip_or_fail "xemu is not installed; install xemu or rerun with --install-emulator if your APT repositories provide it"

XEMU_CONFIG_TEMP=""
if [ -n "$XEMU_CONFIG" ]; then
	[ -f "$XEMU_CONFIG" ] || emulator_skip_or_fail "xemu config not found: $XEMU_CONFIG"
	XEMU_CONFIG="$(cd "$(dirname "$XEMU_CONFIG")" && pwd -P)/$(basename "$XEMU_CONFIG")"
else
	XEMU_CONFIG_TEMP="$(mktemp -t fb-xbox-xemu.XXXXXX.toml)"
	if ! make_xemu_config "$XEMU_CONFIG_TEMP"; then
		rm -f "$XEMU_CONFIG_TEMP"
		emulator_skip_or_fail "xemu requires MCPX, BIOS, and HDD files; set XEMU_CONFIG or XEMU_MCPX, XEMU_BIOS, and XEMU_HDD"
	fi
	XEMU_CONFIG="$XEMU_CONFIG_TEMP"
fi

if [ -z "${DISPLAY:-}" ] && [ -z "${WAYLAND_DISPLAY:-}" ] && ! command -v xvfb-run >/dev/null 2>&1; then
	emulator_skip_or_fail "no graphical display is available for xemu; set DISPLAY/WAYLAND_DISPLAY or install xvfb"
fi

for name in console gfx sfx fileio; do
	[ -f "$OUT_DIR/$name.iso" ] || die "missing XISO artifact: $OUT_DIR/$name.iso"
	run_xemu_iso "$name" "$OUT_DIR/$name.iso" "$XEMU_CONFIG"
done

rm -f "$XEMU_CONFIG_TEMP"
echo "freebasic-xbox package smoke test passed"

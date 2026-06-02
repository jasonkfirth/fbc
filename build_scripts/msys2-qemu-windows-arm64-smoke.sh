#!/usr/bin/env bash

##############################################################################
# FreeBASIC Windows ARM64 QEMU smoke test
##############################################################################
#
# Purpose:
#
#   Run the packaged FreeBASIC Windows ARM64 compiler inside a Windows-on-Arm
#   guest booted by qemu-system-aarch64.
#
# Responsibilities:
#
#   * launch a prepared Windows ARM64 QEMU disk when requested
#   * copy a packaged FreeBASIC distribution into the guest over SSH
#   * compile and run a tiny ARM64 Windows program in the guest
#   * provide a one-time installer launch mode for creating the VM disk
#
# This script intentionally does NOT contain:
#
#   * Windows licensing or activation handling
#   * GUI automation of the Windows installer
#   * a full fbctests run inside the guest
#
##############################################################################

set -euo pipefail
trap 'echo "ERROR: failed at line $LINENO: $BASH_COMMAND" >&2' ERR

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
WORKROOT="${QEMU_ARM64_WORKROOT:-$ROOT/out/msys2-qemu-arm64}"
RUN_DIR="$WORKROOT/run"
LOG_DIR="$WORKROOT/logs"

DISTROOT="${QEMU_ARM64_DIST:-}"
VM_DISK="${QEMU_ARM64_DISK:-}"
INSTALL_ISO="${QEMU_ARM64_INSTALL_ISO:-}"
EFI_CODE="${QEMU_AARCH64_EFI:-${QEMU_EFI:-}}"
EFI_VARS="${QEMU_AARCH64_EFI_VARS:-}"
EFI_VARS_TEMPLATE="${QEMU_AARCH64_EFI_VARS_TEMPLATE:-}"
QEMU_BIN="${QEMU_SYSTEM_AARCH64:-}"
QEMU_IMG="${QEMU_IMG:-}"
VIRTIO_ISO="${QEMU_ARM64_VIRTIO_ISO:-}"

SSH_USER="${QEMU_ARM64_SSH_USER:-}"
SSH_HOST="${QEMU_ARM64_SSH_HOST:-127.0.0.1}"
SSH_PORT="${QEMU_ARM64_SSH_PORT:-2222}"
SSH_KEY="${QEMU_ARM64_SSH_KEY:-}"
SSH_TIMEOUT="${QEMU_ARM64_SSH_TIMEOUT:-1800}"
REMOTE_ROOT="${QEMU_ARM64_REMOTE_ROOT:-freebasic-qemu-arm64-smoke}"

CPUS="${QEMU_ARM64_CPUS:-4}"
MEMORY="${QEMU_ARM64_MEMORY:-4096}"
CPU_MODEL="${QEMU_ARM64_CPU:-max,pauth-impdef=on}"
ACCEL="${QEMU_ARM64_ACCEL:-tcg,thread=multi}"
DISK_SIZE="${QEMU_ARM64_DISK_SIZE:-80G}"
DISK_FORMAT="${QEMU_ARM64_DISK_FORMAT:-}"
DISK_DEVICE="${QEMU_ARM64_DISK_DEVICE:-virtio-blk-pci}"
NET_DEVICE="${QEMU_ARM64_NET_DEVICE:-usb-net}"
DISPLAY_MODE="${QEMU_ARM64_DISPLAY:-none}"
INSTALL_DISPLAY_MODE="${QEMU_ARM64_INSTALL_DISPLAY:-default}"
EXTRA_ARGS="${QEMU_ARM64_EXTRA_ARGS:-}"
KEEP_VM="${QEMU_ARM64_KEEP_VM:-0}"
START_VM=1
INSTALL_MODE=0
VM_STARTED=0

msg() { printf '\n==> %s\n' "$*"; }
warn() { printf '\nWARNING: %s\n' "$*" >&2; }
die() { printf '\nERROR: %s\n' "$*" >&2; exit 1; }

usage() {
	cat <<EOF
Usage: ./build_scripts/msys2-qemu-windows-arm64-smoke.sh [options]

Smoke-test mode:
  --dist DIR             FreeBASIC package tree. Default: latest out/mingw32 package
  --disk FILE            Bootable Windows ARM64 qcow2/raw disk
  --ssh-user USER        Windows SSH user. Required for smoke-test mode
  --ssh-host HOST        SSH host. Default: ${SSH_HOST}
  --ssh-port N           SSH forwarded port. Default: ${SSH_PORT}
  --ssh-key FILE         SSH private key
  --ssh-timeout N        Seconds to wait for SSH. Default: ${SSH_TIMEOUT}
  --remote-root NAME     Guest work directory under the SSH user's home
  --no-start-vm          Use an already-running VM instead of launching QEMU
  --keep-vm              Leave the QEMU process running after the smoke test

Installer mode:
  --install-iso FILE     Boot a Windows ARM64 installer ISO instead of testing
  --disk FILE            Disk to create or reuse. Default: <workroot>/windows-arm64.qcow2
  --disk-size SIZE       New disk size for installer mode. Default: ${DISK_SIZE}
  --install-display MODE QEMU display backend for installer mode. Default: default

QEMU options:
  --qemu FILE            qemu-system-aarch64 executable
  --qemu-img FILE        qemu-img executable
  --efi FILE             AArch64 UEFI firmware image
  --efi-vars FILE        Writable AArch64 UEFI vars image
  --efi-vars-template FILE
                         Template copied to <workroot>/run/QEMU_VARS.fd
  --virtio-iso FILE      Optional virtio-win driver ISO for installer mode
  --cpus N               Guest CPU count. Default: ${CPUS}
  --memory MB            Guest memory. Default: ${MEMORY}
  --cpu MODEL            QEMU CPU model. Default: ${CPU_MODEL}
  --accel NAME           QEMU accelerator. Default: ${ACCEL}
  --disk-format FORMAT   Disk format. Default: inferred from filename
  --disk-device NAME     virtio-blk-pci, usb-storage, or hda. Default: ${DISK_DEVICE}
  --net-device NAME      usb-net, virtio-net-pci, e1000, rtl8139, or none. Default: ${NET_DEVICE}
  --display MODE         QEMU display backend for smoke mode. Default: ${DISPLAY_MODE}
  --workroot DIR         Work directory. Default: ${WORKROOT}
  -h, --help             Show this help text

One-time VM setup:
  1. Install QEMU in MSYS2:
       pacman -S --needed mingw-w64-x86_64-qemu openssh
  2. Boot the Windows ARM64 installer:
       ./build_scripts/msys2-qemu-windows-arm64-smoke.sh \\
         --install-iso Win11_Arm64.iso \\
         --disk out/msys2-qemu-arm64/windows-arm64.qcow2
     If the installer cannot see the disk or network adapter, attach the
     Fedora virtio-win driver ISO with --virtio-iso and load the driver there.
  3. Inside Windows, enable OpenSSH Server and allow port 22 in the firewall.
  4. Run smoke-test mode with --ssh-user and the same --disk.
EOF
}

while [ "$#" -gt 0 ]; do
	case "$1" in
		--dist) DISTROOT="$2"; shift 2 ;;
		--disk) VM_DISK="$2"; shift 2 ;;
		--install-iso) INSTALL_ISO="$2"; INSTALL_MODE=1; shift 2 ;;
		--efi|--bios) EFI_CODE="$2"; shift 2 ;;
		--efi-vars) EFI_VARS="$2"; shift 2 ;;
		--efi-vars-template) EFI_VARS_TEMPLATE="$2"; shift 2 ;;
		--qemu) QEMU_BIN="$2"; shift 2 ;;
		--qemu-img) QEMU_IMG="$2"; shift 2 ;;
		--virtio-iso) VIRTIO_ISO="$2"; shift 2 ;;
		--ssh-user) SSH_USER="$2"; shift 2 ;;
		--ssh-host) SSH_HOST="$2"; shift 2 ;;
		--ssh-port) SSH_PORT="$2"; shift 2 ;;
		--ssh-key) SSH_KEY="$2"; shift 2 ;;
		--ssh-timeout) SSH_TIMEOUT="$2"; shift 2 ;;
		--remote-root) REMOTE_ROOT="$2"; shift 2 ;;
		--no-start-vm) START_VM=0; shift ;;
		--keep-vm) KEEP_VM=1; shift ;;
		--cpus) CPUS="$2"; shift 2 ;;
		--memory) MEMORY="$2"; shift 2 ;;
		--cpu) CPU_MODEL="$2"; shift 2 ;;
		--accel) ACCEL="$2"; shift 2 ;;
		--disk-size) DISK_SIZE="$2"; shift 2 ;;
		--disk-format) DISK_FORMAT="$2"; shift 2 ;;
		--disk-device) DISK_DEVICE="$2"; shift 2 ;;
		--net-device) NET_DEVICE="$2"; shift 2 ;;
		--display) DISPLAY_MODE="$2"; shift 2 ;;
		--install-display) INSTALL_DISPLAY_MODE="$2"; shift 2 ;;
		--workroot)
			WORKROOT="$2"
			RUN_DIR="$WORKROOT/run"
			LOG_DIR="$WORKROOT/logs"
			shift 2
			;;
		-h|--help) usage; exit 0 ;;
		*) die "unknown option: $1" ;;
	esac
done

run() {
	echo "==> $*"
	"$@"
}

have() {
	command -v "$1" >/dev/null 2>&1
}

abs_path() {
	local path="$1"
	local dir
	local base

	dir="$(cd "$(dirname "$path")" && pwd)"
	base="$(basename "$path")"
	printf '%s/%s\n' "$dir" "$base"
}

qemu_path() {
	if have cygpath; then
		cygpath -am "$1"
	else
		printf '%s\n' "$1"
	fi
}

find_tool() {
	local preferred="$1"
	shift
	local candidate
	local prefix

	if [ -n "$preferred" ]; then
		if [ -x "$preferred" ]; then
			abs_path "$preferred"
			return 0
		fi
		if command -v "$preferred" >/dev/null 2>&1; then
			command -v "$preferred"
			return 0
		fi
		return 1
	fi

	for candidate in "$@"; do
		if command -v "$candidate" >/dev/null 2>&1; then
			command -v "$candidate"
			return 0
		fi
	done

	for prefix in /mingw64 /ucrt64 /clang64 /clangarm64 /usr; do
		for candidate in "$@"; do
			if [ -x "$prefix/bin/$candidate" ]; then
				printf '%s/bin/%s\n' "$prefix" "$candidate"
				return 0
			fi
		done
	done

	return 1
}

find_firmware() {
	local candidate

	for candidate in \
		"$EFI_CODE" \
		/mingw64/share/qemu/edk2-aarch64-code.fd \
		/ucrt64/share/qemu/edk2-aarch64-code.fd \
		/clang64/share/qemu/edk2-aarch64-code.fd \
		/clangarm64/share/qemu/edk2-aarch64-code.fd \
		/mingw64/share/qemu/QEMU_EFI.fd \
		/ucrt64/share/qemu/QEMU_EFI.fd \
		/clang64/share/qemu/QEMU_EFI.fd \
		/clangarm64/share/qemu/QEMU_EFI.fd \
		/usr/share/qemu/edk2-aarch64-code.fd \
		/usr/share/qemu-efi-aarch64/QEMU_EFI.fd
	do
		[ -n "$candidate" ] || continue
		if [ -f "$candidate" ]; then
			abs_path "$candidate"
			return 0
		fi
	done

	return 1
}

find_firmware_vars_template() {
	local candidate

	for candidate in \
		"$EFI_VARS_TEMPLATE" \
		/mingw64/share/qemu/edk2-aarch64-vars.fd \
		/ucrt64/share/qemu/edk2-aarch64-vars.fd \
		/clang64/share/qemu/edk2-aarch64-vars.fd \
		/clangarm64/share/qemu/edk2-aarch64-vars.fd \
		/mingw64/share/qemu/edk2-arm-vars.fd \
		/ucrt64/share/qemu/edk2-arm-vars.fd \
		/clang64/share/qemu/edk2-arm-vars.fd \
		/clangarm64/share/qemu/edk2-arm-vars.fd \
		/mingw64/share/qemu/QEMU_VARS.fd \
		/ucrt64/share/qemu/QEMU_VARS.fd \
		/clang64/share/qemu/QEMU_VARS.fd \
		/clangarm64/share/qemu/QEMU_VARS.fd \
		/usr/share/qemu/edk2-aarch64-vars.fd \
		/usr/share/qemu-efi-aarch64/QEMU_VARS.fd
	do
		[ -n "$candidate" ] || continue
		if [ -f "$candidate" ]; then
			abs_path "$candidate"
			return 0
		fi
	done

	return 1
}

prepare_efi_vars() {
	local template

	if [ -n "$EFI_VARS" ]; then
		[ -f "$EFI_VARS" ] || die "UEFI vars file not found: $EFI_VARS"
		EFI_VARS="$(abs_path "$EFI_VARS")"
		return 0
	fi

	template="$(find_firmware_vars_template || true)"
	[ -n "$template" ] || return 1

	mkdir -p "$RUN_DIR"
	EFI_VARS="$RUN_DIR/QEMU_VARS.fd"
	if [ ! -f "$EFI_VARS" ]; then
		cp -f "$template" "$EFI_VARS"
	fi
	EFI_VARS="$(abs_path "$EFI_VARS")"
	return 0
}

find_latest_distroot() {
	local candidate

	candidate="$(
		{
			find "$ROOT/out/mingw32" "$ROOT/.build-msys2/dist" \
				-maxdepth 1 -type d -name 'FreeBASIC-*-winlibs-*' 2>/dev/null || true
		} |
			sort |
			tail -n 1
	)"

	[ -n "$candidate" ] || return 1
	abs_path "$candidate"
}

infer_disk_format() {
	if [ -n "$DISK_FORMAT" ]; then
		printf '%s\n' "$DISK_FORMAT"
		return 0
	fi

	case "$VM_DISK" in
		*.raw|*.img) printf 'raw\n' ;;
		*) printf 'qcow2\n' ;;
	esac
}

validate_simple_remote_root() {
	case "$REMOTE_ROOT" in
		""|*\"*|*\'*|*\\*|*/*|*:*|*" "*|*[\&\|\<\>\(\)\;\%]*)
			die "--remote-root must be a simple relative directory name"
			;;
	esac
}

require_number() {
	local name="$1"
	local value="$2"

	case "$value" in
		""|*[!0-9]*|0) die "$name must be a positive integer" ;;
	esac
}

require_common_tools() {
	if [ "$START_VM" -ne 0 ] || [ "$INSTALL_MODE" -ne 0 ]; then
		QEMU_BIN="$(find_tool "$QEMU_BIN" qemu-system-aarch64.exe qemu-system-aarch64)" ||
			die "qemu-system-aarch64 not found; install mingw-w64-x86_64-qemu or pass --qemu"
		EFI_CODE="$(find_firmware)" ||
			die "AArch64 UEFI firmware not found; pass --efi or install the MSYS2 QEMU package"
	fi

	if [ "$INSTALL_MODE" -ne 0 ]; then
		QEMU_IMG="$(find_tool "$QEMU_IMG" qemu-img.exe qemu-img)" ||
			die "qemu-img not found; install mingw-w64-x86_64-qemu or pass --qemu-img"
	else
		have ssh || die "ssh not found; install the MSYS2 openssh package"
		have scp || die "scp not found; install the MSYS2 openssh package"
	fi
}

append_display_args() {
	local mode="$1"

	if [ "$mode" != "default" ]; then
		QEMU_ARGS+=(-display "$mode")
	fi
}

append_disk_args() {
	local disk="$1"
	local format="$2"

	case "$DISK_DEVICE" in
		virtio-blk-pci)
			QEMU_ARGS+=(
				-drive "if=none,file=$disk,format=$format,id=system"
				-device "virtio-blk-pci,drive=system,bootindex=0"
			)
			;;
		usb-storage)
			QEMU_ARGS+=(
				-drive "if=none,file=$disk,format=$format,id=system"
				-device "usb-storage,drive=system,bootindex=0"
			)
			;;
		hda)
			QEMU_ARGS+=(-hda "$disk")
			;;
		*)
			die "unsupported --disk-device: $DISK_DEVICE"
			;;
	esac
}

append_network_args() {
	case "$NET_DEVICE" in
		none)
			return 0
			;;
		usb-net|virtio-net-pci|e1000|rtl8139)
			QEMU_ARGS+=(
				-netdev "user,id=net0,hostfwd=tcp:127.0.0.1:${SSH_PORT}-:22"
				-device "$NET_DEVICE,netdev=net0"
			)
			;;
		*)
			die "unsupported --net-device: $NET_DEVICE"
			;;
	esac
}

build_qemu_args() {
	local display="$1"
	local disk
	local firmware
	local vars
	local format
	local extra_words=()

	disk="$(qemu_path "$VM_DISK")"
	firmware="$(qemu_path "$EFI_CODE")"
	format="$(infer_disk_format)"

	QEMU_ARGS=(
		"$QEMU_BIN"
		-machine virt
		-cpu "$CPU_MODEL"
		-smp "$CPUS"
		-m "$MEMORY"
		-accel "$ACCEL"
		-device ramfb
		-device nec-usb-xhci
		-device usb-kbd
		-device usb-tablet
	)

	if prepare_efi_vars; then
		vars="$(qemu_path "$EFI_VARS")"
		QEMU_ARGS+=(
			-drive "if=pflash,format=raw,readonly=on,file=$firmware"
			-drive "if=pflash,format=raw,file=$vars"
		)
	else
		QEMU_ARGS+=(-bios "$firmware")
	fi

	append_display_args "$display"
	append_disk_args "$disk" "$format"
	append_network_args

	if [ -n "$INSTALL_ISO" ]; then
		QEMU_ARGS+=(
			-cdrom "$(qemu_path "$INSTALL_ISO")"
			-boot d
		)
	fi

	if [ -n "$VIRTIO_ISO" ]; then
		[ -f "$VIRTIO_ISO" ] || die "VirtIO driver ISO not found: $VIRTIO_ISO"
		QEMU_ARGS+=(
			-drive "if=none,id=virtio_drivers,format=raw,media=cdrom,readonly=on,file=$(qemu_path "$VIRTIO_ISO")"
			-device "usb-storage,drive=virtio_drivers"
		)
	fi

	if [ -n "$EXTRA_ARGS" ]; then
		# Extra arguments are intentionally split like a normal shell command.
		# This keeps the common case readable while leaving complex setups to
		# wrapper scripts or QEMU_ARM64_EXTRA_ARGS.
		extra_words=($EXTRA_ARGS)
		QEMU_ARGS+=("${extra_words[@]}")
	fi
}

prepare_install_disk() {
	if [ -z "$VM_DISK" ]; then
		VM_DISK="$WORKROOT/windows-arm64.qcow2"
	fi

	mkdir -p "$(dirname "$VM_DISK")"
	if [ ! -f "$VM_DISK" ]; then
		msg "Creating Windows ARM64 QEMU disk"
		run "$QEMU_IMG" create -f qcow2 "$VM_DISK" "$DISK_SIZE"
	fi

	VM_DISK="$(abs_path "$VM_DISK")"
}

run_installer_vm() {
	[ -f "$INSTALL_ISO" ] || die "installer ISO not found: $INSTALL_ISO"
	INSTALL_ISO="$(abs_path "$INSTALL_ISO")"
	prepare_install_disk
	build_qemu_args "$INSTALL_DISPLAY_MODE"

	msg "Starting Windows ARM64 installer under QEMU"
	warn "After Windows is installed, enable OpenSSH Server and allow TCP port 22 in the guest firewall."
	run "${QEMU_ARGS[@]}"
}

cleanup_vm() {
	local pid

	if [ "$VM_STARTED" -eq 0 ] || [ "$KEEP_VM" -ne 0 ]; then
		return 0
	fi

	if [ -f "$RUN_DIR/qemu.pid" ]; then
		pid="$(cat "$RUN_DIR/qemu.pid")"
		kill "$pid" >/dev/null 2>&1 || true
		rm -f "$RUN_DIR/qemu.pid"
	fi
}

start_vm() {
	local log="$LOG_DIR/qemu.log"
	local pid

	[ -n "$VM_DISK" ] || die "--disk or QEMU_ARM64_DISK is required when launching QEMU"
	[ -f "$VM_DISK" ] || die "VM disk not found: $VM_DISK"
	VM_DISK="$(abs_path "$VM_DISK")"
	build_qemu_args "$DISPLAY_MODE"

	msg "Starting Windows ARM64 VM on SSH port $SSH_PORT"
	mkdir -p "$RUN_DIR" "$LOG_DIR"
	"${QEMU_ARGS[@]}" > "$log" 2>&1 &
	pid="$!"
	echo "$pid" > "$RUN_DIR/qemu.pid"
	VM_STARTED=1

	sleep 2
	if ! kill -0 "$pid" >/dev/null 2>&1; then
		die "QEMU exited early; see $log"
	fi
}

make_ssh_args() {
	SSH_ARGS=(
		-o BatchMode=yes
		-o StrictHostKeyChecking=no
		-o UserKnownHostsFile="$RUN_DIR/known_hosts"
		-o LogLevel=ERROR
		-p "$SSH_PORT"
	)
	SCP_ARGS=(
		-o BatchMode=yes
		-o StrictHostKeyChecking=no
		-o UserKnownHostsFile="$RUN_DIR/known_hosts"
		-o LogLevel=ERROR
		-P "$SSH_PORT"
	)

	if [ -n "$SSH_KEY" ]; then
		SSH_ARGS+=(-i "$SSH_KEY")
		SCP_ARGS+=(-i "$SSH_KEY")
	fi
}

remote_ssh() {
	ssh "${SSH_ARGS[@]}" "$SSH_USER@$SSH_HOST" "$@"
}

remote_scp() {
	scp "${SCP_ARGS[@]}" "$@"
}

wait_for_ssh() {
	local elapsed=0

	msg "Waiting for Windows SSH on $SSH_HOST:$SSH_PORT"
	while [ "$elapsed" -lt "$SSH_TIMEOUT" ]; do
		if remote_ssh "cmd.exe /d /c ver" >/dev/null 2>&1; then
			return 0
		fi
		sleep 5
		elapsed=$((elapsed + 5))
	done

	die "timed out waiting for Windows SSH"
}

write_guest_runner() {
	cat > "$RUN_DIR/run-smoke.cmd" <<'EOF'
@echo off
setlocal

set "ROOT=%~dp0"
set "PKG=%ROOT%package"

if not exist "%PKG%\fbcarm64.exe" (
    echo fbcarm64.exe was not copied into the guest package directory
    exit /b 10
)

cd /d "%PKG%" || exit /b 11
set "PATH=%CD%;%CD%\bin\win32-aarch64;%CD%\bin;%PATH%"

for /f "usebackq delims=" %%A in (`fbcarm64.exe -print target -target win32-aarch64`) do set "FBTARGET=%%A"
if not "%FBTARGET%"=="win32-aarch64" (
    echo unexpected target: %FBTARGET%
    exit /b 12
)

> hello.bas echo print "FreeBASIC package test OK"
fbcarm64.exe hello.bas -x helloarm64.exe
if errorlevel 1 exit /b 13

for /f "delims=" %%A in ('helloarm64.exe') do set "HELLO_OUT=%%A"
if not "%HELLO_OUT%"=="FreeBASIC package test OK" (
    echo unexpected hello output: %HELLO_OUT%
    exit /b 14
)

echo FreeBASIC Windows ARM64 QEMU smoke test OK
exit /b 0
EOF
}

prepare_remote_tree() {
	local ps_command

	ps_command="\$root = Join-Path \$HOME '$REMOTE_ROOT'; Remove-Item -LiteralPath \$root -Recurse -Force -ErrorAction SilentlyContinue; New-Item -ItemType Directory -Force -Path \$root | Out-Null"
	remote_ssh "powershell.exe -NoProfile -ExecutionPolicy Bypass -Command \"$ps_command\""
}

copy_package_to_guest() {
	msg "Copying FreeBASIC package into the Windows ARM64 guest"
	write_guest_runner
	prepare_remote_tree
	run remote_scp "$RUN_DIR/run-smoke.cmd" "$SSH_USER@$SSH_HOST:$REMOTE_ROOT/run-smoke.cmd"
	run remote_scp -r "$DISTROOT" "$SSH_USER@$SSH_HOST:$REMOTE_ROOT/package"
}

run_guest_smoke() {
	local ps_command

	msg "Running Windows ARM64 compiler smoke test"
	ps_command="\$root = Join-Path \$HOME '$REMOTE_ROOT'; Set-Location -LiteralPath \$root; & '.\\run-smoke.cmd'; exit \$LASTEXITCODE"
	run remote_ssh "powershell.exe -NoProfile -ExecutionPolicy Bypass -Command \"$ps_command\""
}

run_smoke_test() {
	[ -n "$SSH_USER" ] || die "--ssh-user or QEMU_ARM64_SSH_USER is required for smoke-test mode"
	validate_simple_remote_root

	if [ -z "$DISTROOT" ]; then
		DISTROOT="$(find_latest_distroot)" ||
			die "could not find a FreeBASIC package tree; pass --dist"
	fi
	[ -d "$DISTROOT" ] || die "package tree not found: $DISTROOT"
	DISTROOT="$(abs_path "$DISTROOT")"
	[ -f "$DISTROOT/fbcarm64.exe" ] || die "package tree does not contain fbcarm64.exe: $DISTROOT"

	mkdir -p "$RUN_DIR" "$LOG_DIR"
	make_ssh_args
	trap cleanup_vm EXIT

	if [ "$START_VM" -ne 0 ]; then
		start_vm
	fi

	wait_for_ssh
	copy_package_to_guest
	run_guest_smoke
}

##############################################################################
# Main
##############################################################################

require_number "--cpus" "$CPUS"
require_number "--memory" "$MEMORY"
require_number "--ssh-port" "$SSH_PORT"
require_number "--ssh-timeout" "$SSH_TIMEOUT"
require_common_tools

if [ "$INSTALL_MODE" -ne 0 ]; then
	run_installer_vm
else
	run_smoke_test
fi

msg "Done"

##############################################################################
# End of msys2-qemu-windows-arm64-smoke.sh
##############################################################################

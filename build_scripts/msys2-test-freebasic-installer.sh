#!/usr/bin/env bash
#
#   Project: FreeBASIC MSYS2 package tests
#   --------------------------------------
#
#   File: msys2-test-freebasic-installer.sh
#
#   Purpose:
#
#       Smoke-test one generated Windows installer by installing it into an
#       isolated temporary directory and validating the installed package.
#
#   Responsibilities:
#
#       * run the NSIS installer silently with an explicit test install root
#       * verify expected installed files and launchers
#       * run local compiler smoke tests where the host can execute them
#       * run the uninstaller and restore global PATH/profile state
#
#   This file intentionally does NOT contain:
#
#       * package build logic
#       * emulator-based target runtime tests
#       * release upload or signing logic
#

set -euo pipefail
trap 'echo "ERROR: failed at line $LINENO: $BASH_COMMAND" >&2' ERR

SELF_DIR="$(CDPATH= cd -- "$(dirname "$0")" && pwd)"
ROOT="$(CDPATH= cd -- "$SELF_DIR/.." && pwd)"

cd "$ROOT"

if [ ! -d "$ROOT/build_scripts" ] || [ ! -f "$ROOT/GNUmakefile" ]; then
	echo "ERROR: could not locate the FreeBASIC project root." >&2
	exit 1
fi

case "$(uname -s)" in
	MINGW*|MSYS*) ;;
	*)
		echo "ERROR: this script must be run inside an MSYS2 environment." >&2
		exit 1
		;;
esac

INSTALLER=""
KIND=""

# Keep the default close to the drive root.  The bundled toolchains contain
# deep directory trees, and Windows archive APIs can still trip over long
# install paths even on current Windows builds.
WORKROOT="${WORKROOT:-${INSTALLER_SMOKE_WORKROOT:-/c/fbc-installer-smoke}}"
KEEP_WORK=0

usage()
{
	cat <<EOF
Usage: ./build_scripts/msys2-test-freebasic-installer.sh --installer PATH --kind KIND [options]

Options:
  --installer PATH  NSIS installer to test
  --kind KIND       windows-gcc, windows-arm64, android, js, wii, or xbox
  --workroot PATH   temporary smoke-test root
  --keep-work       keep smoke-test files after success
  -h, --help        show this help text

The test installs into a temporary directory, validates the installed copy,
then runs the generated uninstaller.  It snapshots and restores the global
Windows PATH registry value and MSYS2 profile fragments that the installers
may touch.
EOF
}

fail()
{
	echo "ERROR: $*" >&2
	exit 1
}

msg()
{
	echo ""
	echo "==> $*"
}

to_msys_path()
{
	cygpath -au "$1"
}

to_win_path()
{
	cygpath -aw "$1"
}

safe_name()
{
	printf '%s\n' "$1" | sed -e 's/[^A-Za-z0-9_.-]/_/g'
}

while [ $# -gt 0 ]; do
	case "$1" in
		--installer)
			[ $# -ge 2 ] || fail "--installer requires a path"
			INSTALLER="$(to_msys_path "$2")"
			shift 2
			;;
		--kind)
			[ $# -ge 2 ] || fail "--kind requires a value"
			KIND="$2"
			shift 2
			;;
		--workroot)
			[ $# -ge 2 ] || fail "--workroot requires a path"
			WORKROOT="$(to_msys_path "$2")"
			shift 2
			;;
		--keep-work)
			KEEP_WORK=1
			shift
			;;
		-h|--help)
			usage
			exit 0
			;;
		*)
			fail "unknown option: $1"
			;;
	esac
done

[ -n "$INSTALLER" ] || fail "--installer is required"
[ -f "$INSTALLER" ] || fail "installer not found: $INSTALLER"

case "$KIND" in
	windows-gcc|windows-arm64|android|js|wii|xbox) ;;
	"") fail "--kind is required" ;;
	*) fail "unsupported installer kind: $KIND" ;;
esac

TESTROOT="$WORKROOT/$(safe_name "$(basename "$INSTALLER" .exe)")"
INSTALLDIR="$TESTROOT/install"
TESTDIR="$TESTROOT/work"
PS1="$TESTROOT/installer-smoke.ps1"

case "$INSTALLDIR" in
	*" "*) fail "installer smoke workroot cannot contain spaces: $INSTALLDIR" ;;
esac

rm -rf "$TESTROOT"
mkdir -p "$TESTDIR"

cat > "$PS1" <<'EOF'
param(
	[string] $Installer,
	[string] $InstallDir,
	[string] $WorkDir,
	[string] $Kind
)

Set-StrictMode -Version 2.0
$ErrorActionPreference = "Stop"

function Fail {
	param([string] $Message)
	throw $Message
}

function Msg {
	param([string] $Message)
	Write-Host ""
	Write-Host "==> $Message"
}

function Assert-Path {
	param(
		[string] $Path,
		[string] $Description
	)

	if (-not (Test-Path -LiteralPath $Path)) {
		Fail "missing ${Description}: ${Path}"
	}
}

function Invoke-Native {
	param(
		[string] $FilePath,
		[string[]] $Arguments = @()
	)

	Write-Host "==> $FilePath $($Arguments -join ' ')"
	& $FilePath @Arguments
	$status = $LASTEXITCODE
	if ($null -ne $status -and $status -ne 0) {
		Fail "command failed with exit code ${status}: ${FilePath}"
	}
}

function Invoke-SilentInstaller {
	param(
		[string] $FilePath,
		[string[]] $Arguments = @()
	)

	Write-Host "==> $FilePath $($Arguments -join ' ')"
	$process = Start-Process -FilePath $FilePath -ArgumentList $Arguments -Wait -PassThru
	if ($null -ne $process.ExitCode -and $process.ExitCode -ne 0) {
		Fail "installer command failed with exit code $($process.ExitCode): ${FilePath}"
	}
}

function Write-BasicProgram {
	param(
		[string] $Path,
		[string] $Message
	)

	Set-Content -LiteralPath $Path -Encoding ASCII -Value "print ""${Message}"""
}

function Invoke-ProgramAndExpect {
	param(
		[string] $FilePath,
		[string] $Expected
	)

	$output = & $FilePath
	$status = $LASTEXITCODE
	if ($null -ne $status -and $status -ne 0) {
		Fail "program failed with exit code ${status}: ${FilePath}"
	}
	if (($output -join "`n").Trim() -ne $Expected) {
		Fail "program output did not match for ${FilePath}"
	}
}

function Snapshot-File {
	param([string] $Path)

	if (Test-Path -LiteralPath $Path) {
		return [Convert]::ToBase64String([IO.File]::ReadAllBytes($Path))
	}

	return $null
}

function Restore-File {
	param(
		[string] $Path,
		[AllowNull()] [string] $Snapshot
	)

	if ($null -eq $Snapshot) {
		if (Test-Path -LiteralPath $Path) {
			Remove-Item -LiteralPath $Path -Force
		}
		return
	}

	$dir = Split-Path -Parent $Path
	if (-not (Test-Path -LiteralPath $dir)) {
		New-Item -ItemType Directory -Force -Path $dir | Out-Null
	}

	[IO.File]::WriteAllBytes($Path, [Convert]::FromBase64String($Snapshot))
}

function Test-WindowsGcc {
	$fbc64 = Join-Path $InstallDir "fbc64.exe"
	$fbc32 = Join-Path $InstallDir "fbc32.exe"
	$src = Join-Path $WorkDir "hello.bas"
	$out64 = Join-Path $WorkDir "hello64.exe"
	$out32 = Join-Path $WorkDir "hello32.exe"

	Assert-Path $fbc64 "win64 compiler"
	Assert-Path $fbc32 "win32 compiler"
	Assert-Path (Join-Path $InstallDir "bin\win64") "win64 tool directory"
	Assert-Path (Join-Path $InstallDir "bin\win32") "win32 tool directory"

	Write-BasicProgram $src "FreeBASIC installer smoke OK"
	Invoke-Native $fbc64 @($src, "-x", $out64)
	Invoke-ProgramAndExpect $out64 "FreeBASIC installer smoke OK"
	Invoke-Native $fbc32 @($src, "-x", $out32)
	Invoke-ProgramAndExpect $out32 "FreeBASIC installer smoke OK"
}

function Test-WindowsArm64 {
	$fbc = Join-Path $InstallDir "fbcarm64.exe"
	Assert-Path $fbc "Windows ARM64 compiler"
	Assert-Path (Join-Path $InstallDir "bin\win32-aarch64") "Windows ARM64 tool directory"

	if ($env:PROCESSOR_ARCHITECTURE -eq "ARM64") {
		$src = Join-Path $WorkDir "hello.bas"
		$out = Join-Path $WorkDir "helloarm64.exe"
		Write-BasicProgram $src "FreeBASIC ARM64 installer smoke OK"
		Invoke-Native $fbc @($src, "-x", $out)
		Invoke-ProgramAndExpect $out "FreeBASIC ARM64 installer smoke OK"
	} else {
		Write-Host "Windows ARM64 compiler execution skipped on $env:PROCESSOR_ARCHITECTURE host."
	}
}

function Test-Android {
	Assert-Path (Join-Path $InstallDir "fbc-android.cmd") "Android launcher"
	Assert-Path (Join-Path $InstallDir "fbc-android.exe") "Android driver"
	Assert-Path (Join-Path $InstallDir "setup-android-sdk.cmd") "Android SDK setup launcher"
	Assert-Path (Join-Path $InstallDir "setup-android-sdk.ps1") "Android SDK setup script"
	Assert-Path (Join-Path $InstallDir "lib\freebasic-android\bin\fbc-android-compiler.exe") "Android FreeBASIC compiler"
	Write-Host "Android compile smoke skipped because the installer intentionally omits the SDK/NDK cache."
}

function Test-JavaScript {
	$src = Join-Path $WorkDir "hello.bas"
	$out = Join-Path $WorkDir "hello.js"
	$result = Join-Path $WorkDir "hello.out"
	$app = Join-Path $WorkDir "app"
	$assets = Join-Path $WorkDir "assets"
	$node = Join-Path $InstallDir "toolchain\ucrt64\bin\node.exe"

	Assert-Path (Join-Path $InstallDir "fbc-js.cmd") "JavaScript launcher"
	Assert-Path (Join-Path $InstallDir "fbc-js-app.cmd") "JavaScript app launcher"
	Assert-Path $node "bundled node.exe"

	New-Item -ItemType Directory -Force -Path $assets | Out-Null
	Set-Content -LiteralPath (Join-Path $assets "readme.txt") -Encoding ASCII -Value "asset smoke"
	Write-BasicProgram $src "FreeBASIC JS installer smoke OK"

	$env:EMCC_TEMP_DIR = Join-Path $WorkDir "emcc-temp"
	$env:EM_CACHE = Join-Path $WorkDir "em-cache"
	$env:BINARYEN_CORES = "1"
	$env:EMCC_BATCH_BUILD = "0"

	Invoke-Native (Join-Path $InstallDir "fbc-js.cmd") @($src, "-x", $out)
	& $node $out > $result
	$status = $LASTEXITCODE
	if ($null -ne $status -and $status -ne 0) {
		Fail "generated JavaScript failed with exit code ${status}"
	}
	if ((Get-Content -LiteralPath $result -Raw).Trim() -ne "FreeBASIC JS installer smoke OK") {
		Fail "generated JavaScript output was wrong"
	}

	Invoke-Native (Join-Path $InstallDir "fbc-js-app.cmd") @("--out", $app, "--assets", $assets, $src)
	Assert-Path (Join-Path $app "index.html") "generated JavaScript app"
}

function Test-Wii {
	$src = Join-Path $WorkDir "hello.bas"
	$out = Join-Path $WorkDir "hello.dol"
	$assets = Join-Path $WorkDir "assets"
	$bundle = Join-Path $WorkDir "bundle"

	Assert-Path (Join-Path $InstallDir "fbc-wii.cmd") "Wii launcher"
	Write-BasicProgram $src "FreeBASIC Wii installer smoke OK"
	Invoke-Native (Join-Path $InstallDir "fbc-wii.cmd") @($src, "-x", $out)
	Assert-Path $out "Wii DOL output"

	New-Item -ItemType Directory -Force -Path $assets | Out-Null
	Set-Content -LiteralPath (Join-Path $assets "readme.txt") -Encoding ASCII -Value "asset smoke"
	Invoke-Native (Join-Path $InstallDir "fbc-wii.cmd") @("--bundle", $bundle, "--assets", $assets, $src)
	Assert-Path (Join-Path $bundle "boot.dol") "Wii bundle boot.dol"
	Assert-Path (Join-Path $bundle "readme.txt") "Wii bundle asset"
}

function Test-Xbox {
	$src = Join-Path $WorkDir "hello.bas"
	$xbe = Join-Path $WorkDir "hello.xbe"
	$iso = Join-Path $WorkDir "hello.iso"
	$assets = Join-Path $WorkDir "assets"

	Assert-Path (Join-Path $InstallDir "fbc-xbox.cmd") "Xbox launcher"
	Assert-Path (Join-Path $InstallDir "fbc-xbox-xiso.cmd") "Xbox XISO launcher"

	New-Item -ItemType Directory -Force -Path $assets | Out-Null
	Set-Content -LiteralPath (Join-Path $assets "readme.txt") -Encoding ASCII -Value "asset smoke"
	Write-BasicProgram $src "FreeBASIC Xbox installer smoke OK"
	Invoke-Native (Join-Path $InstallDir "fbc-xbox.cmd") @($src, "-x", $xbe)
	Assert-Path $xbe "Xbox XBE output"
	Invoke-Native (Join-Path $InstallDir "fbc-xbox-xiso.cmd") @($xbe, $iso, "--assets", $assets)
	Assert-Path $iso "Xbox XISO output"
}

$identity = [Security.Principal.WindowsIdentity]::GetCurrent()
$principal = New-Object Security.Principal.WindowsPrincipal($identity)
if (-not $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
	Fail "installer smoke tests must run from an elevated MSYS2 shell"
}

$envKeyName = "SYSTEM\CurrentControlSet\Control\Session Manager\Environment"
$envKey = [Microsoft.Win32.Registry]::LocalMachine.OpenSubKey($envKeyName, $true)
if ($null -eq $envKey) {
	Fail "could not open machine environment registry key"
}

$oldPath = $envKey.GetValue("Path", $null, [Microsoft.Win32.RegistryValueOptions]::DoNotExpandEnvironmentNames)
$oldPathKind = $null
if ($null -ne $oldPath) {
	$oldPathKind = $envKey.GetValueKind("Path")
}
$profilePaths = @(
	"C:\msys64\etc\profile.d\freebasic.sh",
	"C:\msys64\etc\profile.d\freebasic-js.sh",
	"C:\msys32\etc\profile.d\freebasic.sh",
	"C:\msys32\etc\profile.d\freebasic-js.sh"
)
$profileSnapshots = @{}
foreach ($path in $profilePaths) {
	$profileSnapshots[$path] = Snapshot-File $path
}

$installed = $false

try {
	Msg "Installing $Kind package smoke root"
	if (Test-Path -LiteralPath $InstallDir) {
		Remove-Item -LiteralPath $InstallDir -Recurse -Force
	}
	New-Item -ItemType Directory -Force -Path $WorkDir | Out-Null

	Invoke-SilentInstaller $Installer @("/S", "/D=$InstallDir")
	$installed = $true

	Assert-Path (Join-Path $InstallDir "uninstall.exe") "uninstaller"

	Msg "Validating installed $Kind package"
	switch ($Kind) {
		"windows-gcc" { Test-WindowsGcc }
		"windows-arm64" { Test-WindowsArm64 }
		"android" { Test-Android }
		"js" { Test-JavaScript }
		"wii" { Test-Wii }
		"xbox" { Test-Xbox }
		default { Fail "unsupported installer kind: $Kind" }
	}
} finally {
	if ($installed) {
		Msg "Uninstalling $Kind package smoke root"
		$uninstaller = Join-Path $InstallDir "uninstall.exe"
		if (Test-Path -LiteralPath $uninstaller) {
			try {
				Invoke-SilentInstaller $uninstaller @("/S")
			} catch {
				Write-Warning $_
			}
		}
	}

	if (Test-Path -LiteralPath $InstallDir) {
		Remove-Item -LiteralPath $InstallDir -Recurse -Force
	}

	if ($null -ne $oldPath) {
		$envKey.SetValue("Path", $oldPath, $oldPathKind)
	} else {
		try {
			$envKey.DeleteValue("Path", $false)
		} catch {
			Write-Warning $_
		}
	}

	foreach ($path in $profilePaths) {
		Restore-File $path $profileSnapshots[$path]
	}

	$envKey.Close()
}

Write-Host ""
Write-Host "Installer smoke test passed: $Installer"
EOF

msg "Running installer smoke test for $(basename "$INSTALLER")"
powershell.exe -NoProfile -ExecutionPolicy Bypass \
	-File "$(to_win_path "$PS1")" \
	-Installer "$(to_win_path "$INSTALLER")" \
	-InstallDir "$(to_win_path "$INSTALLDIR")" \
	-WorkDir "$(to_win_path "$TESTDIR")" \
	-Kind "$KIND"

if [ "$KEEP_WORK" -eq 0 ]; then
	rm -rf "$TESTROOT"
else
	msg "Keeping installer smoke workroot: $TESTROOT"
fi

# end of msys2-test-freebasic-installer.sh

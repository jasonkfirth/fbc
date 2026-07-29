#
# Project: FreeBASIC gfxlib3 tests
# --------------------------------
#
# File: profile-oma-games.ps1
#
# Purpose:
#
#     Build and profile the active OMA game collection against both gfxlib2
#     and gfxlib3 on Windows.
#
# Responsibilities:
#
#     - keep the current OMA game inventory in one reproducible test matrix
#     - build matching gfxlib2 and gfxlib3 executables from unchanged sources
#     - select the matching win32 or win64 runtime for the supplied compiler
#     - enter actual gameplay instead of measuring title or menu screens
#     - collect bounded process and per-thread CPU samples
#     - preserve gfxlib3 renderer profiles for shared-library optimization
#
# This file intentionally does NOT contain:
#
#     - game-specific source changes
#     - fixed performance pass or fail thresholds
#     - Android packaging or device deployment
#

[CmdletBinding()]
param(
    [string]$Compiler = "",
    [string]$OutputDirectory = "",
    [string[]]$Games = @(),
    [ValidateSet("gfx2", "gfx3")]
    [string[]]$Runtimes = @("gfx2", "gfx3"),
    [ValidateSet("", "OPENGL", "VULKAN")]
    [string]$Gfx3Backend = "",
    [ValidateRange(2, 120)]
    [int]$MeasureSeconds = 8,
    [ValidateRange(1, 20)]
    [int]$Samples = 1,
    [ValidateSet("error", "warning", "info", "trace")]
    [string]$Gfx3LogLevel = "info",
    [string[]]$AdditionalLibraryDirectories = @(),
    [switch]$CaptureScreenshots,
    [switch]$SkipBuild
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

Add-Type @'
using System;
using System.Runtime.InteropServices;
using System.Text;

public static class OmaProfileInput
{
    private delegate bool EnumerateWindow(IntPtr window, IntPtr parameter);

    [StructLayout(LayoutKind.Sequential)]
    public struct WindowRectangle
    {
        public int Left;
        public int Top;
        public int Right;
        public int Bottom;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct WindowPoint
    {
        public int X;
        public int Y;
    }

    [DllImport("user32.dll")]
    public static extern bool PostMessage(
        IntPtr window,
        uint message,
        IntPtr parameter,
        IntPtr extra);

    [DllImport("user32.dll")]
    public static extern bool SetForegroundWindow(IntPtr window);

    [DllImport("user32.dll")]
    public static extern uint MapVirtualKey(
        uint code,
        uint mapType);

    [DllImport("user32.dll")]
    public static extern bool GetWindowRect(
        IntPtr window,
        out WindowRectangle rectangle);

    [DllImport("user32.dll")]
    public static extern bool ClientToScreen(
        IntPtr window,
        ref WindowPoint point);

    [DllImport("user32.dll")]
    public static extern bool SetCursorPos(
        int x,
        int y);

    [DllImport("user32.dll")]
    public static extern void mouse_event(
        uint flags,
        uint x,
        uint y,
        uint data,
        UIntPtr extraInfo);

    [DllImport("user32.dll")]
    private static extern bool SetWindowPos(
        IntPtr window,
        IntPtr insertAfter,
        int x,
        int y,
        int width,
        int height,
        uint flags);

    [DllImport("user32.dll")]
    private static extern bool EnumWindows(
        EnumerateWindow callback,
        IntPtr parameter);

    [DllImport("user32.dll")]
    private static extern uint GetWindowThreadProcessId(
        IntPtr window,
        out uint processId);

    [DllImport("user32.dll")]
    private static extern bool IsWindowVisible(IntPtr window);

    [DllImport("user32.dll", CharSet = CharSet.Unicode)]
    private static extern int GetClassName(
        IntPtr window,
        StringBuilder className,
        int maximumCount);

    public static IntPtr FindGraphicsWindow(int processId)
    {
        IntPtr result = IntPtr.Zero;

        /*
            A console FreeBASIC program can own both ConsoleWindowClass and
            the actual graphics window. Process.MainWindowHandle is whichever
            Windows happens to enumerate first, so it is not a stable way to
            target renderer input or capture.
        */
        EnumWindows(delegate(IntPtr window, IntPtr parameter)
        {
            uint owner;

            GetWindowThreadProcessId(window, out owner);

            if (owner != (uint)processId || !IsWindowVisible(window))
                return true;

            StringBuilder className = new StringBuilder(256);
            GetClassName(window, className, className.Capacity);

            string name = className.ToString();

            if (name.StartsWith(
                    "FreeBASIC", StringComparison.OrdinalIgnoreCase) ||
                name.StartsWith(
                    "fbgfxclass_", StringComparison.OrdinalIgnoreCase))
            {
                result = window;
                return false;
            }

            return true;
        }, IntPtr.Zero);

        return result;
    }

    public static bool SetWindowTopmost(IntPtr window, bool topmost)
    {
        /*
            SetForegroundWindow can be denied when the profiling shell does
            not own the current foreground-input queue. Temporarily placing
            the game at the top keeps CopyFromScreen from recording an
            unrelated editor window in that case.
        */
        IntPtr insertAfter = new IntPtr(topmost ? -1 : -2);

        const uint NoSize = 0x0001;
        const uint NoMove = 0x0002;
        const uint NoActivate = 0x0010;

        return SetWindowPos(
            window,
            insertAfter,
            0,
            0,
            0,
            0,
            NoSize | NoMove | NoActivate);
    }
}
'@

if ($CaptureScreenshots) {
    Add-Type -AssemblyName System.Drawing
}

# -------------------------------------------------------------------------
# Paths and game inventory
# -------------------------------------------------------------------------

$repositoryRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path

if ([string]::IsNullOrWhiteSpace($Compiler)) {
    $repositoryCompiler = Join-Path $repositoryRoot "bin\fbc.exe"
    $installedCompiler = "C:\FreeBASIC\fbc.exe"

    if (Test-Path -LiteralPath $repositoryCompiler) {
        $Compiler = $repositoryCompiler
    }
    elseif (Test-Path -LiteralPath $installedCompiler) {
        $Compiler = $installedCompiler
    }
    else {
        throw "No FreeBASIC compiler was found. Pass -Compiler explicitly."
    }
}

if ([string]::IsNullOrWhiteSpace($OutputDirectory)) {
    $OutputDirectory = Join-Path $repositoryRoot `
        ".codex-tmp-gfx3-build\oma-all-current"
}

$compilerPath = (Resolve-Path $Compiler).Path
$includeRoot = (Resolve-Path (Join-Path $repositoryRoot "inc")).Path
$compilerTarget = (& $compilerPath -print target).Trim()

if ($LASTEXITCODE -ne 0) {
    throw "Could not determine the target of compiler: $compilerPath"
}

if ($compilerTarget -ne "win32" -and $compilerTarget -ne "win64") {
    throw "OMA Windows profiling requires a win32 or win64 compiler."
}

$libraryRoot = (Resolve-Path (
    Join-Path $repositoryRoot ("lib\freebasic\" + $compilerTarget))).Path

New-Item -ItemType Directory -Force -Path $OutputDirectory | Out-Null
$outputRoot = (Resolve-Path $OutputDirectory).Path

$matrix = @(
    [pscustomobject]@{
        Name = "arkanoid"
        DisplayName = "Arkanoid"
        Source = "OMA\ArkanoidTest\ArkanoidTest.bas"
        WorkingDirectory = "OMA\ArkanoidTest"
        Arguments = @()
        Environment = @{}
    },
    [pscustomobject]@{
        Name = "behold"
        DisplayName = "Behold"
        Source = "OMA\Behold\Behold.bas"
        WorkingDirectory = "OMA\Behold"
        Arguments = @()
        Environment = @{}
    },
    [pscustomobject]@{
        Name = "demoderby"
        DisplayName = "Demolition Derby"
        Source = "OMA\DemolitionDerby\main.bas"
        WorkingDirectory = "OMA\DemolitionDerby"
        Arguments = @()
        Environment = @{}
    },
    [pscustomobject]@{
        Name = "duel999"
        DisplayName = "Duel 999"
        Source = "OMA\duel999\SD_Main.bas"
        WorkingDirectory = "OMA\duel999"
        Arguments = @()
        Environment = @{
            SD_AUTOSTART = "1"
            SD_HOSTNAME = "127.0.0.1"
            SD_PLAYERNAME = "Benchmark"
        }
    },
    [pscustomobject]@{
        Name = "kinematics"
        DisplayName = "Kinematics"
        Source = "OMA\kinematics\kinematic_man_two_bodies_self_collision_friction.bas"
        WorkingDirectory = "OMA\kinematics"
        Arguments = @()
        Environment = @{}
    },
    [pscustomobject]@{
        Name = "nietzsche"
        DisplayName = "Nietzsche Special Edition"
        Source = "OMA\NietzscheSE-MSDOS-1.1\Nietzsche\src\win32\win11.bas"
        WorkingDirectory = "OMA\NietzscheSE-MSDOS-1.1\Nietzsche"
        Arguments = @()
        Environment = @{}
    },
    [pscustomobject]@{
        Name = "qfak"
        DisplayName = "Quest for a King"
        Source = "OMA\QuestForAKing-Win32-1.5\src\win11.bas"
        WorkingDirectory = "OMA\QuestForAKing-Win32-1.5"
        Arguments = @()
        Environment = @{}
    },
    [pscustomobject]@{
        Name = "rambo"
        DisplayName = "Rambo vs Kitty Cat"
        Source = "OMA\RamboVsKittyCat-Win32-0.1\killquest.bas"
        WorkingDirectory = "OMA\RamboVsKittyCat-Win32-0.1"
        Arguments = @()
        Environment = @{}
    },
    [pscustomobject]@{
        Name = "starphalanx"
        DisplayName = "Star Phalanx"
        Source = "OMA\StarPhalanx-win32-0.5\entryv2.bas"
        WorkingDirectory = "OMA\StarPhalanx-win32-0.5"
        Arguments = @()
        Environment = @{}
    },
    [pscustomobject]@{
        Name = "openmarket"
        DisplayName = "Open Market"
        Source = "OMA\Tamper\tamper\src\openmarket_bootstrap.bas"
        WorkingDirectory = "OMA\Tamper\tamper"
        Arguments = @()
        Environment = @{}
    },
    [pscustomobject]@{
        Name = "openhostility"
        DisplayName = "OpenHostility"
        Source = "OMA\Scorched Earth\src\scorch_gfx.bas"
        AdditionalSources = @(
            "OMA\Scorched Earth\src\scorch_platform.bas",
            "OMA\Scorched Earth\src\scorch_engine.bas",
            "OMA\Scorched Earth\src\scorch_io.bas",
            "OMA\Scorched Earth\src\scorch_mtn.bas"
        )
        IncludeDirectories = @(
            "OMA\Scorched Earth\src",
            "OMA\Scorched Earth\src\omaGUI"
        )
        CompilerArguments = @("-exx", "-w", "all")
        WorkingDirectory = "OMA\Scorched Earth"
        Arguments = @((Join-Path $repositoryRoot "OMA\Scorched Earth"))
        Environment = @{}
    },
    [pscustomobject]@{
        Name = "turbotrek"
        DisplayName = "TurboTrek"
        Source = "OMA\TurboTrek\src\turbotrek\main.bas"
        AdditionalSources = @(
            "OMA\TurboTrek\src\turbotrek\clone_scenario_file.bas",
            "OMA\TurboTrek\src\turbotrek\clone_scenario_import.bas",
            "OMA\TurboTrek\src\turbotrek\clone_scenario_runtime.bas",
            "OMA\TurboTrek\src\turbotrek\clone_scenario.bas",
            "OMA\TurboTrek\src\turbotrek\clone_session_file.bas",
            "OMA\TurboTrek\src\turbotrek\damage.bas",
            "OMA\TurboTrek\src\turbotrek\deflectors.bas",
            "OMA\TurboTrek\src\turbotrek\device_condition.bas",
            "OMA\TurboTrek\src\turbotrek\disruptor.bas",
            "OMA\TurboTrek\src\turbotrek\drone_projectile.bas",
            "OMA\TurboTrek\src\turbotrek\residual_damage.bas",
            "OMA\TurboTrek\src\turbotrek\residual_damage_resolution.bas",
            "OMA\TurboTrek\src\turbotrek\original_random.bas",
            "OMA\TurboTrek\src\turbotrek\photon.bas",
            "OMA\TurboTrek\src\turbotrek\plasma_projectile.bas",
            "OMA\TurboTrek\src\turbotrek\pulser_redirect.bas",
            "OMA\TurboTrek\src\turbotrek\critical_damage.bas",
            "OMA\TurboTrek\src\turbotrek\energy.bas",
            "OMA\TurboTrek\src\turbotrek\fire_control.bas",
            "OMA\TurboTrek\src\turbotrek\game_state.bas",
            "OMA\TurboTrek\src\turbotrek\terrain_damage.bas",
            "OMA\TurboTrek\src\turbotrek\movement.bas",
            "OMA\TurboTrek\src\turbotrek\operations.bas",
            "OMA\TurboTrek\src\turbotrek\operations_display.bas",
            "OMA\TurboTrek\src\turbotrek\repair.bas",
            "OMA\TurboTrek\src\turbotrek\robot_ai.bas",
            "OMA\TurboTrek\src\turbotrek\scenario_catalog.bas",
            "OMA\TurboTrek\src\turbotrek\scenario_metadata.bas",
            "OMA\TurboTrek\src\turbotrek\scenario_objects.bas",
            "OMA\TurboTrek\src\turbotrek\scenario_world.bas",
            "OMA\TurboTrek\src\turbotrek\self_destruct.bas",
            "OMA\TurboTrek\src\turbotrek\sensors.bas",
            "OMA\TurboTrek\src\turbotrek\ship_definition.bas",
            "OMA\TurboTrek\src\turbotrek\simulation.bas",
            "OMA\TurboTrek\src\turbotrek\weapon_targeting.bas",
            "OMA\TurboTrek\src\turbotrek\world.bas"
        )
        IncludeDirectories = @(
            "OMA\TurboTrek\src\omaGUI",
            "OMA\TurboTrek\src\turbotrek"
        )
        CompilerArguments = @()
        WorkingDirectory = "OMA\TurboTrek"
        Arguments = @()
        Environment = @{}
    },
    [pscustomobject]@{
        Name = "vtrek"
        DisplayName = "vtrek"
        Source = "OMA\vtrek\src\vtrek.bas"
        AdditionalSources = @()
        IncludeDirectories = @()
        CompilerArguments = @()
        WorkingDirectory = "OMA\vtrek"
        Arguments = @()
        Environment = @{}
    },
    [pscustomobject]@{
        Name = "openwallstreet"
        DisplayName = "OpenWallStreet"
        Source = "OMA\WSR5_3\src\raider.bas"
        AdditionalSources = @()
        IncludeDirectories = @(
            "OMA\WSR5_3\src",
            "OMA\WSR5_3\src\omaGUI"
        )
        CompilerArguments = @("-lang", "fblite")
        WorkingDirectory = "OMA\WSR5_3"
        Arguments = @()
        Environment = @{}
    }
)

if ($Games.Count -ne 0) {
    $requestedGames = @{}

    foreach ($gameName in $Games) {
        $requestedGames[$gameName.ToLowerInvariant()] = $true
    }

    $matrix = @($matrix | Where-Object {
        $requestedGames.ContainsKey($_.Name.ToLowerInvariant())
    })

    if ($matrix.Count -ne $requestedGames.Count) {
        $knownNames = ($matrix.Name -join ", ")
        throw "One or more requested games are unknown. Selected: $knownNames"
    }
}

# -------------------------------------------------------------------------
# Build support
# -------------------------------------------------------------------------

function Build-OmaExecutable {
    param(
        [Parameter(Mandatory = $true)]
        [pscustomobject]$Game,
        [Parameter(Mandatory = $true)]
        [ValidateSet("gfx2", "gfx3")]
        [string]$Runtime
    )

    $outputPath = Join-Path $outputRoot (
        $Game.Name + "-" + $Runtime + ".exe")
    $arguments = @("-mt")
    $sourceNames = @($Game.Source)

    if ($Game.PSObject.Properties.Name -contains "CompilerArguments") {
        $arguments += @($Game.CompilerArguments)
    }

    $arguments += @("-i", $includeRoot)

    if ($Game.PSObject.Properties.Name -contains "IncludeDirectories") {
        foreach ($directoryName in $Game.IncludeDirectories) {
            $directoryPath = Join-Path $repositoryRoot $directoryName

            if (-not (Test-Path -LiteralPath $directoryPath -PathType Container)) {
                throw "Game include directory does not exist: $directoryPath"
            }

            $arguments += @("-i", $directoryPath)
        }
    }

    $arguments += @("-p", $libraryRoot)

    <#
        A compiler built inside the source tree locates the current FreeBASIC
        runtime beside itself, but a separately installed MinGW toolchain may
        still own the platform startup objects and import libraries. Keep that
        relationship explicit so 32-bit cross-checks do not depend on PATH
        search order.
    #>
    foreach ($directory in $AdditionalLibraryDirectories) {
        $arguments += @("-p", $directory)
    }

    if ($Runtime -eq "gfx3") {
        $arguments += @("-d", "__FB_GFXLIB3__")
    }

    if ($Game.PSObject.Properties.Name -contains "AdditionalSources") {
        $sourceNames += @($Game.AdditionalSources)
    }

    foreach ($sourceName in $sourceNames) {
        $sourcePath = Join-Path $repositoryRoot $sourceName

        if (-not (Test-Path -LiteralPath $sourcePath -PathType Leaf)) {
            throw "Game source file does not exist: $sourcePath"
        }

        $arguments += $sourcePath
    }

    $arguments += @("-x", $outputPath)

    Write-Host ("Building {0} ({1})" -f $Game.DisplayName, $Runtime)
    & $compilerPath @arguments

    if ($LASTEXITCODE -ne 0) {
        throw "Compilation failed: $($Game.DisplayName) ($Runtime)"
    }

    if (-not (Test-Path -LiteralPath $outputPath)) {
        throw "Compiler did not create the expected executable: $outputPath"
    }
}

if (-not $SkipBuild) {
    foreach ($game in $matrix) {
        foreach ($runtime in $Runtimes) {
            Build-OmaExecutable -Game $game -Runtime $runtime
        }
    }
}

# -------------------------------------------------------------------------
# Window input and gameplay entry
# -------------------------------------------------------------------------

function Get-OmaGraphicsWindow {
    param(
        [Parameter(Mandatory = $true)]
        [System.Diagnostics.Process]$Process
    )

    if ($Process.HasExited) {
        return [IntPtr]::Zero
    }

    return [OmaProfileInput]::FindGraphicsWindow($Process.Id)
}

function Wait-OmaWindow {
    param(
        [Parameter(Mandatory = $true)]
        [System.Diagnostics.Process]$Process,
        [int]$TimeoutSeconds = 15
    )

    $deadline = [DateTime]::UtcNow.AddSeconds($TimeoutSeconds)
    $window = Get-OmaGraphicsWindow -Process $Process

    while (-not $Process.HasExited -and $window -eq [IntPtr]::Zero -and
           [DateTime]::UtcNow -lt $deadline) {
        Start-Sleep -Milliseconds 50
        $Process.Refresh()
        $window = Get-OmaGraphicsWindow -Process $Process
    }

    if ($Process.HasExited) {
        throw "Game exited before creating its graphics window."
    }

    if ($window -eq [IntPtr]::Zero) {
        throw "Game did not create a graphics window within $TimeoutSeconds seconds."
    }
}

function Activate-OmaWindow {
    param(
        [Parameter(Mandatory = $true)]
        [System.Diagnostics.Process]$Process
    )

    Wait-OmaWindow -Process $Process
    $Process.Refresh()
    $window = Get-OmaGraphicsWindow -Process $Process

    if ($window -eq [IntPtr]::Zero) {
        throw "The game graphics window is no longer available."
    }

    [void][OmaProfileInput]::SetForegroundWindow($window)

    # WM_ACTIVATE with WA_ACTIVE makes the synthetic key state deterministic.
    [void][OmaProfileInput]::PostMessage(
        $window,
        0x0006,
        [IntPtr]1,
        [IntPtr]0)
}

function Send-OmaKey {
    param(
        [Parameter(Mandatory = $true)]
        [System.Diagnostics.Process]$Process,
        [Parameter(Mandatory = $true)]
        [int]$VirtualKey,
        [ValidateRange(20, 5000)]
        [int]$HoldMilliseconds = 120
    )

    Wait-OmaWindow -Process $Process
    $Process.Refresh()
    $window = Get-OmaGraphicsWindow -Process $Process

    if ($Process.HasExited -or
        $window -eq [IntPtr]::Zero) {
        throw "Cannot send input because the game window has closed."
    }

    # WM_KEYDOWN and WM_KEYUP are consumed by both gfxlib2 and gfxlib3.
    $scanCode = [OmaProfileInput]::MapVirtualKey(
        [uint32]$VirtualKey, 0)
    $keyDownState = [int64]1 -bor ([int64]$scanCode -shl 16)
    $keyUpState = $keyDownState -bor [int64]0xC0000000

    [void][OmaProfileInput]::PostMessage(
        $window,
        0x0100,
        [IntPtr]$VirtualKey,
        [IntPtr]$keyDownState)

    Start-Sleep -Milliseconds $HoldMilliseconds
    [void][OmaProfileInput]::PostMessage(
        $window,
        0x0101,
        [IntPtr]$VirtualKey,
        [IntPtr]$keyUpState)
}

function Send-OmaText {
    param(
        [Parameter(Mandatory = $true)]
        [System.Diagnostics.Process]$Process,
        [Parameter(Mandatory = $true)]
        [string]$Text
    )

    foreach ($character in $Text.ToUpperInvariant().ToCharArray()) {
        $virtualKey = [int][char]$character

        if (($virtualKey -ge [int][char]"A" -and
             $virtualKey -le [int][char]"Z") -or
            ($virtualKey -ge [int][char]"0" -and
             $virtualKey -le [int][char]"9")) {
            Send-OmaKey -Process $Process -VirtualKey $virtualKey `
                -HoldMilliseconds 60
        }
        else {
            throw "Unsupported automated text character: $character"
        }
    }
}

function Send-OmaMouseClick {
    param(
        [Parameter(Mandatory = $true)]
        [System.Diagnostics.Process]$Process,
        [ValidateRange(0, 32767)]
        [int]$X,
        [ValidateRange(0, 32767)]
        [int]$Y,
        [ValidateRange(20, 5000)]
        [int]$HoldMilliseconds = 100
    )

    Wait-OmaWindow -Process $Process
    $Process.Refresh()
    $window = Get-OmaGraphicsWindow -Process $Process

    if ($Process.HasExited -or $window -eq [IntPtr]::Zero) {
        throw "Cannot send mouse input because the game window has closed."
    }

    <#
        omaGUI polls the system mouse state instead of consuming only queued
        window messages. Convert the logical client point to desktop space and
        send a real bounded left click so both gfxlib2 and gfxlib3 observe the
        same cursor and button state.
    #>
    $point = New-Object OmaProfileInput+WindowPoint
    $point.X = $X
    $point.Y = $Y

    if (-not [OmaProfileInput]::ClientToScreen($window, [ref]$point)) {
        throw "The game mouse coordinate could not be converted to the desktop."
    }

    [void][OmaProfileInput]::SetForegroundWindow($window)

    if (-not [OmaProfileInput]::SetCursorPos($point.X, $point.Y)) {
        throw "The mouse cursor could not be positioned over the game."
    }

    # MOUSEEVENTF_LEFTDOWN and MOUSEEVENTF_LEFTUP.
    [OmaProfileInput]::mouse_event(
        0x0002, 0, 0, 0, [UIntPtr]::Zero)
    Start-Sleep -Milliseconds $HoldMilliseconds
    [OmaProfileInput]::mouse_event(
        0x0004, 0, 0, 0, [UIntPtr]::Zero)
}

function Enter-OmaGameplay {
    param(
        [Parameter(Mandatory = $true)]
        [pscustomobject]$Game,
        [Parameter(Mandatory = $true)]
        [System.Diagnostics.Process]$Process
    )

    Wait-OmaWindow -Process $Process
    Activate-OmaWindow -Process $Process

    switch ($Game.Name) {
        "arkanoid" {
            Start-Sleep -Milliseconds 800
            Send-OmaKey -Process $Process -VirtualKey 32 `
                -HoldMilliseconds 300
        }
        "behold" {
            #
            # Behold opens SCREEN 13 while it builds a 256 by 255 palette
            # table, then replaces that window with its 32-bit game screen.
            #
            Start-Sleep -Milliseconds 4000
            Send-OmaKey -Process $Process -VirtualKey 0x31 `
                -HoldMilliseconds 300
        }
        "demoderby" {
            Start-Sleep -Milliseconds 1200
            Send-OmaKey -Process $Process -VirtualKey 13 `
                -HoldMilliseconds 300
        }
        "duel999" {
            # Asset rotation and the loopback server handshake happen here.
            Start-Sleep -Milliseconds 3500
        }
        "kinematics" {
            Start-Sleep -Milliseconds 1200
        }
        "nietzsche" {
            #
            # The bundled 2004 save slots are shorter than the current native
            # save layout. Enter the live load browser, but do not feed one of
            # those legacy files to the current loader. QFAK below profiles the
            # same engine family's normal map and sprite renderer.
            #
            Start-Sleep -Milliseconds 3500
            Send-OmaKey -Process $Process -VirtualKey 0x28 `
                -HoldMilliseconds 160
            Send-OmaKey -Process $Process -VirtualKey 32 `
                -HoldMilliseconds 160
            Start-Sleep -Milliseconds 1200
        }
        "qfak" {
            #
            # Entry zero displays two fades and holds the OMA logo for six
            # seconds before opening the start menu. Space selects its default
            # Start item. Entry one then presents a sequence of story panels;
            # advance all of them so the measurement covers the scrolling map,
            # sprites, particles, text, and page-flip path used during play.
            #
            Start-Sleep -Milliseconds 9000
            Send-OmaKey -Process $Process -VirtualKey 32 `
                -HoldMilliseconds 160
            Start-Sleep -Milliseconds 500

            # The first five panels precede map and character initialization.
            for ($panel = 0; $panel -lt 8; $panel++) {
                Send-OmaKey -Process $Process -VirtualKey 32 `
                    -HoldMilliseconds 80
                Start-Sleep -Milliseconds 450
            }

            # Inputs sent during the scripted fade are deliberately discarded.
            Start-Sleep -Milliseconds 10000

            for ($panel = 0; $panel -lt 30; $panel++) {
                Send-OmaKey -Process $Process -VirtualKey 32 `
                    -HoldMilliseconds 80
                Start-Sleep -Milliseconds 350
            }
        }
        "rambo" {
            Start-Sleep -Milliseconds 2200
            Send-OmaKey -Process $Process -VirtualKey 32 `
                -HoldMilliseconds 800
        }
        "starphalanx" {
            Start-Sleep -Milliseconds 2500
            Send-OmaKey -Process $Process -VirtualKey 32 `
                -HoldMilliseconds 500
        }
        "openmarket" {
            Start-Sleep -Milliseconds 1000
            Send-OmaKey -Process $Process -VirtualKey 32

            # The title and menu intentionally drain the key that entered them.
            Start-Sleep -Milliseconds 1000
            Send-OmaKey -Process $Process -VirtualKey 13
            Start-Sleep -Milliseconds 800

            Send-OmaText -Process $Process -Text "BENCH"
            Send-OmaKey -Process $Process -VirtualKey 13
            Start-Sleep -Milliseconds 400

            # Empty-name Enter opens the confirmation window after one player.
            Send-OmaKey -Process $Process -VirtualKey 13
            Start-Sleep -Milliseconds 400
            Send-OmaKey -Process $Process -VirtualKey 13
            Start-Sleep -Milliseconds 1500
        }
        "openhostility" {
            # Enter accepts the default setup and starts the first tank round.
            Start-Sleep -Milliseconds 1200
            Send-OmaKey -Process $Process -VirtualKey 13 `
                -HoldMilliseconds 180
            Start-Sleep -Milliseconds 1000
            Send-OmaKey -Process $Process -VirtualKey 32 `
                -HoldMilliseconds 120
        }
        "turbotrek" {
            #
            # Training enters the real energy-allocation and operations path
            # without depending on a particular external scenario file.
            #
            Start-Sleep -Milliseconds 1200
            Send-OmaMouseClick -Process $Process -X 680 -Y 546
            Start-Sleep -Milliseconds 500

            # Advance each training ship through its default allocation.
            for ($ship = 0; $ship -lt 8; $ship++) {
                Send-OmaMouseClick -Process $Process -X 646 -Y 328 `
                    -HoldMilliseconds 60
                Start-Sleep -Milliseconds 180
            }
        }
        "vtrek" {
            # Enter starts a live mission from the recovered setup screen.
            Start-Sleep -Milliseconds 900
            Send-OmaKey -Process $Process -VirtualKey 13 `
                -HoldMilliseconds 180
        }
        "openwallstreet" {
            #
            # Activate the default New Game button, then accept every default
            # setup value. The final two setup prompts name the two default
            # human players, and the tenth confirmation accepts "Play this
            # turn" so the measurement reaches the live market desk. Keyboard
            # activation avoids coupling this profile to the remembered
            # desktop scale while exercising the same omaGUI action as a
            # pointer release.
            #
            Start-Sleep -Milliseconds 1000
            Send-OmaKey -Process $Process -VirtualKey 13 `
                -HoldMilliseconds 180
            Start-Sleep -Milliseconds 500

            for ($setupStep = 0; $setupStep -lt 10; $setupStep++) {
                Send-OmaKey -Process $Process -VirtualKey 13 `
                    -HoldMilliseconds 120
                #
                # Vulkan shader startup and the first font-cache population
                # can outlive a short key repeat interval on an integrated
                # GPU. Wait for the next modal window instead of allowing the
                # following confirmation to be discarded by the old one.
                #
                Start-Sleep -Milliseconds 2000
            }

            Start-Sleep -Milliseconds 2000
        }
        default {
            throw "No gameplay automation is defined for $($Game.Name)."
        }
    }

    if ($Process.HasExited) {
        throw "$($Game.DisplayName) exited while entering gameplay."
    }

    # Exclude asset loading, shader compilation, and menu transitions.
    Start-Sleep -Milliseconds 1000
}

function Save-OmaWindowScreenshot {
    param(
        [Parameter(Mandatory = $true)]
        [System.Diagnostics.Process]$Process,
        [Parameter(Mandatory = $true)]
        [string]$Path
    )

    $Process.Refresh()
    $window = Get-OmaGraphicsWindow -Process $Process
    $rectangle = New-Object OmaProfileInput+WindowRectangle

    if ($window -eq [IntPtr]::Zero -or
        -not [OmaProfileInput]::GetWindowRect(
            $window, [ref]$rectangle)) {
        throw "The game window rectangle could not be captured."
    }

    $width = $rectangle.Right - $rectangle.Left
    $height = $rectangle.Bottom - $rectangle.Top

    if ($width -le 0 -or $height -le 0 -or
        $width -gt 16384 -or $height -gt 16384) {
        throw "The game reported an invalid window size: ${width}x${height}."
    }

    $bitmap = New-Object System.Drawing.Bitmap $width, $height
    $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
    $madeTopmost = [OmaProfileInput]::SetWindowTopmost($window, $true)

    try {
        if (-not $madeTopmost) {
            throw "The game window could not be raised for capture."
        }

        # DWM applies the new stacking order asynchronously.
        Start-Sleep -Milliseconds 200
        $graphics.CopyFromScreen(
            $rectangle.Left,
            $rectangle.Top,
            0,
            0,
            $bitmap.Size,
            [System.Drawing.CopyPixelOperation]::SourceCopy)
        $bitmap.Save($Path, [System.Drawing.Imaging.ImageFormat]::Png)
    }
    finally {
        if ($madeTopmost) {
            [void][OmaProfileInput]::SetWindowTopmost($window, $false)
        }

        $graphics.Dispose()
        $bitmap.Dispose()
    }
}

function Invoke-OmaMeasurementInput {
    param(
        [Parameter(Mandatory = $true)]
        [pscustomobject]$Game,
        [Parameter(Mandatory = $true)]
        [System.Diagnostics.Process]$Process,
        [Parameter(Mandatory = $true)]
        [int]$ElapsedMilliseconds,
        [Parameter(Mandatory = $true)]
        [ref]$NextInputMilliseconds
    )

    if ($ElapsedMilliseconds -lt $NextInputMilliseconds.Value) {
        return
    }

    switch ($Game.Name) {
        "openmarket" {
            #
            # Open Market redraws by design only when state changes. Repeated
            # accept actions advance rounds and dismiss intermission windows.
            #
            Send-OmaKey -Process $Process -VirtualKey 13 `
                -HoldMilliseconds 80
            $NextInputMilliseconds.Value += 700
        }
        "openhostility" {
            # Fire again whenever the previous projectile sequence permits it.
            Send-OmaKey -Process $Process -VirtualKey 32 `
                -HoldMilliseconds 80
            $NextInputMilliseconds.Value += 1000
        }
        "turbotrek" {
            # Move East exercises the operations map and simulation redraw.
            Send-OmaMouseClick -Process $Process -X 725 -Y 446 `
                -HoldMilliseconds 60
            $NextInputMilliseconds.Value += 700
        }
        "vtrek" {
            # Long-range scan is a safe repeatable live-mission redraw.
            Send-OmaKey -Process $Process -VirtualKey 0x4C `
                -HoldMilliseconds 60
            $NextInputMilliseconds.Value += 700
        }
        "openwallstreet" {
            # Cycle menu groups while preserving the active game state.
            Send-OmaKey -Process $Process -VirtualKey 0x27 `
                -HoldMilliseconds 60
            $NextInputMilliseconds.Value += 700
        }
        default {
            $NextInputMilliseconds.Value = [int]::MaxValue
        }
    }
}

# -------------------------------------------------------------------------
# CPU sampling
# -------------------------------------------------------------------------

function Get-OmaThreadSnapshot {
    param(
        [Parameter(Mandatory = $true)]
        [System.Diagnostics.Process]$Process
    )

    $snapshot = @{}

    foreach ($thread in $Process.Threads) {
        try {
            $snapshot[$thread.Id] = [pscustomobject]@{
                CpuSeconds = $thread.TotalProcessorTime.TotalSeconds
                StartMilliseconds =
                    ($thread.StartTime - $Process.StartTime).TotalMilliseconds
            }
        }
        catch {
            # Driver helpers can leave between enumeration and property access.
        }
    }

    return $snapshot
}

function Measure-OmaProcess {
    param(
        [Parameter(Mandatory = $true)]
        [pscustomobject]$Game,
        [Parameter(Mandatory = $true)]
        [System.Diagnostics.Process]$Process,
        [Parameter(Mandatory = $true)]
        [int]$DurationSeconds
    )

    $Process.Refresh()
    $cpuAtStart = $Process.CPU
    $threadsAtStart = Get-OmaThreadSnapshot -Process $Process
    $stopwatch = [System.Diagnostics.Stopwatch]::StartNew()
    $nextInputMilliseconds = 0

    while ($stopwatch.Elapsed.TotalSeconds -lt $DurationSeconds) {
        if ($Process.HasExited) {
            throw "$($Game.DisplayName) exited during its measurement."
        }

        Invoke-OmaMeasurementInput -Game $Game -Process $Process `
            -ElapsedMilliseconds ([int]$stopwatch.ElapsedMilliseconds) `
            -NextInputMilliseconds ([ref]$nextInputMilliseconds)
        Start-Sleep -Milliseconds 40
    }

    $stopwatch.Stop()
    $Process.Refresh()
    $cpuSeconds = $Process.CPU - $cpuAtStart
    $threadResults = @()

    foreach ($thread in $Process.Threads) {
        try {
            if ($threadsAtStart.ContainsKey($thread.Id)) {
                $start = $threadsAtStart[$thread.Id]
                $threadResults += [pscustomobject]@{
                    Id = $thread.Id
                    StartMilliseconds = $start.StartMilliseconds
                    CpuSeconds =
                        $thread.TotalProcessorTime.TotalSeconds -
                        $start.CpuSeconds
                }
            }
        }
        catch {
            # The process CPU total remains valid when a helper exits.
        }
    }

    $mainThread = @($threadResults |
        Sort-Object StartMilliseconds, Id |
        Select-Object -First 1)
    $mainThreadCpu = 0.0

    if ($mainThread.Count -ne 0) {
        $mainThreadCpu = $mainThread[0].CpuSeconds
    }

    return [pscustomobject]@{
        WallSeconds = $stopwatch.Elapsed.TotalSeconds
        CpuSeconds = $cpuSeconds
        CpuPercent = 100.0 * $cpuSeconds / $stopwatch.Elapsed.TotalSeconds
        MainThreadCpuSeconds = $mainThreadCpu
        MainThreadCpuPercent =
            100.0 * $mainThreadCpu / $stopwatch.Elapsed.TotalSeconds
        ThreadCpu = @($threadResults | Sort-Object CpuSeconds -Descending)
    }
}

# -------------------------------------------------------------------------
# Profile execution and cleanup
# -------------------------------------------------------------------------

function Set-OmaChildEnvironment {
    param(
        [Parameter(Mandatory = $true)]
        [pscustomobject]$Game,
        [Parameter(Mandatory = $true)]
        [ValidateSet("gfx2", "gfx3")]
        [string]$Runtime
    )

    Remove-Item Env:FBGFX,Env:FBGFX3_PROFILE,Env:FBGFX3_LOG `
        -ErrorAction SilentlyContinue
    Remove-Item Env:SD_AUTOSTART,Env:SD_HOSTNAME,Env:SD_PLAYERNAME `
        -ErrorAction SilentlyContinue

    foreach ($name in $Game.Environment.Keys) {
        [Environment]::SetEnvironmentVariable(
            $name, $Game.Environment[$name], "Process")
    }

    if ($Runtime -eq "gfx3") {
        $env:FBGFX3_PROFILE = "1"
        $env:FBGFX3_LOG = $Gfx3LogLevel

        if (-not [string]::IsNullOrWhiteSpace($Gfx3Backend)) {
            $env:FBGFX = $Gfx3Backend
        }
    }
}

function Stop-OmaProcess {
    param(
        [Parameter(Mandatory = $true)]
        [System.Diagnostics.Process]$Process
    )

    if ($Process.HasExited) {
        return
    }

    $Process.Refresh()
    $window = Get-OmaGraphicsWindow -Process $Process

    if ($window -ne [IntPtr]::Zero) {
        # WM_CLOSE follows the public FreeBASIC window-close event path.
        [void][OmaProfileInput]::PostMessage(
            $window,
            0x0010,
            [IntPtr]0,
            [IntPtr]0)
        [void]$Process.WaitForExit(3000)
    }

    if (-not $Process.HasExited) {
        Stop-Process -Id $Process.Id -Force
        $Process.WaitForExit()
    }
}

function Invoke-OmaProfile {
    param(
        [Parameter(Mandatory = $true)]
        [pscustomobject]$Game,
        [Parameter(Mandatory = $true)]
        [ValidateSet("gfx2", "gfx3")]
        [string]$Runtime,
        [Parameter(Mandatory = $true)]
        [int]$Sample
    )

    $executable = Join-Path $outputRoot (
        $Game.Name + "-" + $Runtime + ".exe")

    if (-not (Test-Path -LiteralPath $executable)) {
        throw "Missing executable: $executable"
    }

    $stem = "{0}-{1}-sample{2}" -f $Game.Name, $Runtime, $Sample
    $standardOutput = Join-Path $outputRoot ($stem + ".stdout.log")
    $standardError = Join-Path $outputRoot ($stem + ".stderr.log")
    $screenshot = Join-Path $outputRoot ($stem + ".png")
    Remove-Item -LiteralPath $standardOutput,$standardError `
        -Force -ErrorAction SilentlyContinue
    Remove-Item -LiteralPath $screenshot -Force -ErrorAction SilentlyContinue

    Set-OmaChildEnvironment -Game $Game -Runtime $Runtime
    $process = $null
    $configurationPath = $null
    $configurationBytes = $null

    if ($Game.Name -eq "duel999") {
        #
        # Duel writes its selected host and player back to defaults.cfg during
        # normal startup. Preserve the user's configuration around automation.
        #
        $configurationPath = Join-Path $repositoryRoot `
            "OMA\duel999\defaults.cfg"

        if (Test-Path -LiteralPath $configurationPath) {
            $configurationBytes = [IO.File]::ReadAllBytes($configurationPath)
        }
    }

    try {
        $startArguments = @{
            FilePath = $executable
            WorkingDirectory =
                (Resolve-Path (
                    Join-Path $repositoryRoot $Game.WorkingDirectory)).Path
            RedirectStandardOutput = $standardOutput
            RedirectStandardError = $standardError
            PassThru = $true
        }

        if ($Game.Arguments.Count -ne 0) {
            $startArguments.ArgumentList = $Game.Arguments
        }

        Write-Host ("Profiling {0} ({1}, sample {2})" -f
            $Game.DisplayName, $Runtime, $Sample)
        $process = Start-Process @startArguments

        Enter-OmaGameplay -Game $Game -Process $process
        $measurement = Measure-OmaProcess -Game $Game -Process $process `
            -DurationSeconds $MeasureSeconds

        if ($CaptureScreenshots) {
            Activate-OmaWindow -Process $process
            # Let the compositor finish the activation repaint before capture.
            Start-Sleep -Milliseconds 350
            Save-OmaWindowScreenshot -Process $process -Path $screenshot
        }

        return [pscustomobject]@{
            Game = $Game.Name
            DisplayName = $Game.DisplayName
            Runtime = $Runtime
            Backend = if ($Runtime -eq "gfx3") {
                if ([string]::IsNullOrWhiteSpace($Gfx3Backend)) {
                    "automatic"
                }
                else {
                    $Gfx3Backend.ToLowerInvariant()
                }
            }
            else {
                "gfxlib2-default"
            }
            Sample = $Sample
            WallSeconds = $measurement.WallSeconds
            CpuSeconds = $measurement.CpuSeconds
            CpuPercent = $measurement.CpuPercent
            MainThreadCpuSeconds = $measurement.MainThreadCpuSeconds
            MainThreadCpuPercent = $measurement.MainThreadCpuPercent
            Responding = $process.Responding
            WindowTitle = $process.MainWindowTitle
            Screenshot = if ($CaptureScreenshots) { $screenshot } else { "" }
            StandardOutput = $standardOutput
            StandardError = $standardError
            ThreadCpu = $measurement.ThreadCpu
        }
    }
    finally {
        if ($null -ne $process) {
            Stop-OmaProcess -Process $process
        }

        if (($null -ne $configurationPath) -and
            ($null -ne $configurationBytes)) {
            [IO.File]::WriteAllBytes(
                $configurationPath, $configurationBytes)
        }
    }
}

$savedEnvironment = @{}
$environmentNames = @(
    "FBGFX",
    "FBGFX3_PROFILE",
    "FBGFX3_LOG",
    "SD_AUTOSTART",
    "SD_HOSTNAME",
    "SD_PLAYERNAME"
)

foreach ($name in $environmentNames) {
    $savedEnvironment[$name] =
        [Environment]::GetEnvironmentVariable($name, "Process")
}

$results = @()
$progressJsonPath = Join-Path $outputRoot "oma-profile-progress.json"
$progressCsvPath = Join-Path $outputRoot "oma-profile-progress.csv"

function Save-OmaProfileProgress {
    param(
        [Parameter(Mandatory = $true)]
        [array]$ProfileResults
    )

    #
    # The complete OMA matrix can take several minutes because some games have
    # scripted introductions. Keep an atomic checkpoint after every run so an
    # external command timeout does not discard measurements already obtained.
    #
    $temporaryJsonPath = $progressJsonPath + ".tmp"
    $temporaryCsvPath = $progressCsvPath + ".tmp"

    $ProfileResults |
        ConvertTo-Json -Depth 6 |
        Set-Content -LiteralPath $temporaryJsonPath
    $ProfileResults |
        Select-Object Game,DisplayName,Runtime,Backend,Sample,WallSeconds,
            CpuSeconds,CpuPercent,MainThreadCpuSeconds,MainThreadCpuPercent,
            Responding,StandardOutput,StandardError |
        Export-Csv -LiteralPath $temporaryCsvPath -NoTypeInformation

    Move-Item -LiteralPath $temporaryJsonPath -Destination $progressJsonPath `
        -Force
    Move-Item -LiteralPath $temporaryCsvPath -Destination $progressCsvPath `
        -Force
}

try {
    for ($sample = 1; $sample -le $Samples; $sample++) {
        foreach ($game in $matrix) {
            #
            # Alternate runtime order so disk cache and GPU clock state do not
            # consistently benefit the same library in multi-sample runs.
            #
            $preferredRuntimeOrder = if (($sample % 2) -eq 1) {
                @("gfx2", "gfx3")
            }
            else {
                @("gfx3", "gfx2")
            }
            $runtimeOrder = @($preferredRuntimeOrder |
                Where-Object { $Runtimes -contains $_ })

            foreach ($runtime in $runtimeOrder) {
                $result = Invoke-OmaProfile -Game $game -Runtime $runtime `
                    -Sample $sample
                $results += $result
                Save-OmaProfileProgress -ProfileResults $results

                Write-Host ("  CPU {0:N1}%  main thread {1:N1}%" -f
                    $result.CpuPercent, $result.MainThreadCpuPercent)
            }
        }
    }
}
finally {
    foreach ($name in $environmentNames) {
        [Environment]::SetEnvironmentVariable(
            $name, $savedEnvironment[$name], "Process")
    }
}

$timestamp = Get-Date -Format "yyyyMMdd-HHmmss"
$jsonPath = Join-Path $outputRoot ("oma-profile-" + $timestamp + ".json")
$csvPath = Join-Path $outputRoot ("oma-profile-" + $timestamp + ".csv")

$results | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $jsonPath
$results |
    Select-Object Game,DisplayName,Runtime,Backend,Sample,WallSeconds,
        CpuSeconds,CpuPercent,MainThreadCpuSeconds,MainThreadCpuPercent,
        Responding,StandardOutput,StandardError |
    Export-Csv -LiteralPath $csvPath -NoTypeInformation

Write-Output ""
Write-Output "OMA_PROFILE_RESULTS=$jsonPath"
Write-Output "OMA_PROFILE_SUMMARY=$csvPath"
Write-Output ""
$results |
    Select-Object DisplayName,Runtime,Sample,
        @{ Name = "CPU %"; Expression = { "{0:N1}" -f $_.CpuPercent } },
        @{ Name = "Main %"; Expression = {
            "{0:N1}" -f $_.MainThreadCpuPercent
        } } |
    Format-Table -AutoSize

# end of profile-oma-games.ps1

# Project: FreeBASIC gfxlib3 tests
# --------------------------------
#
# File: profile-openslicks.ps1
#
# Purpose:
#
#     Build and profile Open Slicks Racing against gfxlib2 and gfxlib3 on
#     Windows.
#
# Responsibilities:
#
#     - build the game and deterministic test runner for either Windows target
#     - run the deterministic suite through the selected graphics backend
#     - enter a live race and hold player-one acceleration during measurement
#     - collect bounded process and main-thread CPU samples
#     - preserve renderer logs and optional live-window screenshots
#
# This file intentionally does NOT contain:
#
#     - Android packaging
#     - changes to OpenSlicks game source
#     - fixed performance pass or fail thresholds
#

[CmdletBinding()]
param(
    [string]$OpenSlicksRoot = "E:\openSlicks",
    [string]$Compiler = "",
    [ValidateSet("", "win32", "win64")]
    [string]$CompilerTarget = "",
    [string]$OutputDirectory = "",
    [ValidateSet("gfx2", "gfx3")]
    [string[]]$Runtimes = @("gfx2", "gfx3"),
    [ValidateSet("", "OPENGL", "VULKAN")]
    [string]$Gfx3Backend = "",
    [ValidateRange(-1, 255)]
    [int]$VulkanDeviceIndex = -1,
    [ValidateRange(2, 120)]
    [int]$MeasureSeconds = 8,
    [ValidateRange(1, 20)]
    [int]$Samples = 1,
    [string[]]$AdditionalLibraryDirectories = @(),
    [switch]$CaptureScreenshots,
    [switch]$SkipBuild,
    [switch]$SkipTests
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

Add-Type @'
using System;
using System.Runtime.InteropServices;
using System.Text;

public static class OpenSlicksProfileNative
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

    [DllImport("user32.dll")]
    public static extern bool PostMessage(
        IntPtr window,
        uint message,
        IntPtr parameter,
        IntPtr extra);

    [DllImport("user32.dll")]
    public static extern bool SetForegroundWindow(IntPtr window);

    [DllImport("user32.dll")]
    public static extern uint MapVirtualKey(uint code, uint mapType);

    [DllImport("user32.dll")]
    public static extern bool GetWindowRect(
        IntPtr window,
        out WindowRectangle rectangle);

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
            A console build can own both a console and the FreeBASIC graphics
            window. Enumerating by process and class avoids sending the race
            controls to whichever window Windows happened to list first.
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
}
'@

if ($CaptureScreenshots) {
    Add-Type -AssemblyName System.Drawing
}

# -------------------------------------------------------------------------
# Paths and build inventory
# -------------------------------------------------------------------------

$freeBasicRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
$openSlicksPath = (Resolve-Path $OpenSlicksRoot).Path
$sourceRoot = (Resolve-Path (Join-Path $openSlicksPath "src")).Path

if ([string]::IsNullOrWhiteSpace($Compiler)) {
    $Compiler = Join-Path $freeBasicRoot "bin\fbc.exe"
}

if ([string]::IsNullOrWhiteSpace($OutputDirectory)) {
    $OutputDirectory = Join-Path $freeBasicRoot `
        ".codex-tmp-gfx3-build\openslicks-current"
}

$compilerPath = (Resolve-Path $Compiler).Path
$includeRoot = (Resolve-Path (Join-Path $freeBasicRoot "inc")).Path
$targetArguments = @()

if (-not [string]::IsNullOrWhiteSpace($CompilerTarget)) {
    $targetArguments = @("-target", $CompilerTarget)
}

$compilerTarget = (& $compilerPath @targetArguments -print target).Trim()

if ($LASTEXITCODE -ne 0) {
    throw "Could not determine the compiler target: $compilerPath"
}

if ($compilerTarget -ne "win32" -and $compilerTarget -ne "win64") {
    throw "OpenSlicks Windows profiling requires a win32 or win64 compiler."
}

$libraryRoot = (Resolve-Path (
    Join-Path $freeBasicRoot ("lib\freebasic\" + $compilerTarget))).Path

New-Item -ItemType Directory -Force -Path $OutputDirectory | Out-Null
$outputRoot = (Resolve-Path $OutputDirectory).Path

$commonSources = @(
    "slicks_app.bas",
    "slicks_font.bas",
    "slicks_track.bas",
    "slicks_input.bas",
    "oma_native_input.bas",
    "slicks_vehicle.bas",
    "slicks_ai_track.bas",
    "slicks_ai_driver.bas",
    "slicks_game.bas",
    "slicks_render.bas",
    "slicks_menu.bas"
)

$gameSources = @("slicks.bas") + $commonSources
$testSources = @("tests\test_runner.bas") + @(
    "slicks_app.bas",
    "slicks_font.bas",
    "slicks_track.bas",
    "slicks_input.bas",
    "oma_native_input.bas",
    "slicks_ai_track.bas",
    "slicks_ai_driver.bas",
    "slicks_game.bas",
    "slicks_render.bas",
    "slicks_vehicle.bas",
    "slicks_menu.bas"
)

function Get-OpenSlicksExecutable {
    param(
        [Parameter(Mandatory = $true)]
        [ValidateSet("game", "tests")]
        [string]$Kind,
        [Parameter(Mandatory = $true)]
        [ValidateSet("gfx2", "gfx3")]
        [string]$Runtime
    )

    return Join-Path $outputRoot (
        "openslicks-{0}-{1}-{2}.exe" -f $Kind, $Runtime, $compilerTarget)
}

function Build-OpenSlicksExecutable {
    param(
        [Parameter(Mandatory = $true)]
        [ValidateSet("game", "tests")]
        [string]$Kind,
        [Parameter(Mandatory = $true)]
        [ValidateSet("gfx2", "gfx3")]
        [string]$Runtime
    )

    $arguments = @(
        "-mt",
        "-exx",
        "-w", "all",
        "-i", $sourceRoot,
        "-i", $includeRoot,
        "-p", $libraryRoot
    )
    $arguments = $targetArguments + $arguments

    foreach ($directory in $AdditionalLibraryDirectories) {
        $arguments += @("-p", (Resolve-Path $directory).Path)
    }

    if ($Kind -eq "tests") {
        $arguments += @("-d", "SLICKS_TESTING_AUTORUN")

        if ($compilerTarget -eq "win32") {
            <#
                The test runner intentionally keeps several complete track,
                route, cache, and race fixtures alive together. Its checked
                32-bit build exceeds the one-megabyte PE stack default before
                the first assertion, while the game itself does not. Reserve
                32 MiB for the test process without changing production code.
            #>
            $arguments += @("-Wl", "--stack=33554432")
        }

        $sourceNames = $testSources
    }
    else {
        $sourceNames = $gameSources
    }

    if ($Runtime -eq "gfx3") {
        $arguments += @("-d", "__FB_GFXLIB3__")
    }

    foreach ($sourceName in $sourceNames) {
        $sourcePath = Join-Path $sourceRoot $sourceName

        if (-not (Test-Path -LiteralPath $sourcePath -PathType Leaf)) {
            throw "OpenSlicks source file does not exist: $sourcePath"
        }

        $arguments += $sourcePath
    }

    $outputPath = Get-OpenSlicksExecutable -Kind $Kind -Runtime $Runtime
    $arguments += @("-x", $outputPath)

    Write-Host (
        "Building OpenSlicks {0} ({1}, {2})" -f
        $Kind, $Runtime, $compilerTarget)
    & $compilerPath @arguments

    if ($LASTEXITCODE -ne 0) {
        throw "OpenSlicks compilation failed: $Kind, $Runtime"
    }
}

if (-not $SkipBuild) {
    foreach ($runtime in $Runtimes) {
        Build-OpenSlicksExecutable -Kind "game" -Runtime $runtime

        if (-not $SkipTests) {
            Build-OpenSlicksExecutable -Kind "tests" -Runtime $runtime
        }
    }
}

# -------------------------------------------------------------------------
# Child environment and deterministic tests
# -------------------------------------------------------------------------

$environmentNames = @(
    "FBGFX",
    "FBGFX3_LOG",
    "FBGFX3_PROFILE",
    "FBGFX3_VULKAN_DEVICE_INDEX"
)
$savedEnvironment = @{}

foreach ($name in $environmentNames) {
    $savedEnvironment[$name] =
        [Environment]::GetEnvironmentVariable($name, "Process")
}

function Set-OpenSlicksEnvironment {
    param(
        [Parameter(Mandatory = $true)]
        [ValidateSet("gfx2", "gfx3")]
        [string]$Runtime,
        [Parameter(Mandatory = $true)]
        [bool]$Profiling
    )

    foreach ($name in $environmentNames) {
        Remove-Item ("Env:" + $name) -ErrorAction SilentlyContinue
    }

    if ($Runtime -ne "gfx3") {
        return
    }

    $env:FBGFX3_LOG = "info"

    if ($Profiling) {
        $env:FBGFX3_PROFILE = "1"
    }

    if (-not [string]::IsNullOrWhiteSpace($Gfx3Backend)) {
        $env:FBGFX = $Gfx3Backend
    }

    if ($Gfx3Backend -eq "VULKAN" -and $VulkanDeviceIndex -ge 0) {
        $env:FBGFX3_VULKAN_DEVICE_INDEX = [string]$VulkanDeviceIndex
    }
}

function Invoke-OpenSlicksTests {
    param(
        [Parameter(Mandatory = $true)]
        [ValidateSet("gfx2", "gfx3")]
        [string]$Runtime
    )

    $executable = Get-OpenSlicksExecutable -Kind "tests" -Runtime $Runtime
    $standardOutput = Join-Path $outputRoot (
        "openslicks-tests-{0}-{1}.stdout.log" -f $Runtime, $compilerTarget)
    $standardError = Join-Path $outputRoot (
        "openslicks-tests-{0}-{1}.stderr.log" -f $Runtime, $compilerTarget)

    Set-OpenSlicksEnvironment -Runtime $Runtime -Profiling $false

    $process = Start-Process -FilePath $executable `
        -WorkingDirectory $openSlicksPath `
        -RedirectStandardOutput $standardOutput `
        -RedirectStandardError $standardError `
        -PassThru

    <#
        Windows PowerShell can leave ExitCode unset for a Start-Process object
        unless its native handle was materialized while the child was alive.
        Reading Handle before WaitForExit makes a successful test result
        distinguishable from an unavailable exit code.
    #>
    [void]$process.Handle
    $process.WaitForExit()

    if ($process.ExitCode -ne 0) {
        throw "OpenSlicks deterministic tests failed: $Runtime"
    }

    $testText = Get-Content -Raw -LiteralPath $standardOutput

    if ($testText -notmatch "Failures\s*:\s*0") {
        throw "OpenSlicks test output did not report zero failures: $Runtime"
    }

    Write-Host ("OpenSlicks tests passed ({0})" -f $Runtime)
}

# -------------------------------------------------------------------------
# Window control and measurement
# -------------------------------------------------------------------------

function Get-OpenSlicksWindow {
    param(
        [Parameter(Mandatory = $true)]
        [System.Diagnostics.Process]$Process
    )

    if ($Process.HasExited) {
        return [IntPtr]::Zero
    }

    return [OpenSlicksProfileNative]::FindGraphicsWindow($Process.Id)
}

function Wait-OpenSlicksWindow {
    param(
        [Parameter(Mandatory = $true)]
        [System.Diagnostics.Process]$Process,
        [int]$TimeoutSeconds = 30
    )

    $deadline = [DateTime]::UtcNow.AddSeconds($TimeoutSeconds)
    $window = Get-OpenSlicksWindow -Process $Process

    while (-not $Process.HasExited -and $window -eq [IntPtr]::Zero -and
           [DateTime]::UtcNow -lt $deadline) {
        Start-Sleep -Milliseconds 50
        $Process.Refresh()
        $window = Get-OpenSlicksWindow -Process $Process
    }

    if ($Process.HasExited) {
        throw "OpenSlicks exited before creating its graphics window."
    }

    if ($window -eq [IntPtr]::Zero) {
        throw "OpenSlicks did not create a graphics window."
    }

    return $window
}

function Send-OpenSlicksKey {
    param(
        [Parameter(Mandatory = $true)]
        [System.Diagnostics.Process]$Process,
        [Parameter(Mandatory = $true)]
        [int]$VirtualKey,
        [Parameter(Mandatory = $true)]
        [bool]$Pressed
    )

    $window = Wait-OpenSlicksWindow -Process $Process
    [void][OpenSlicksProfileNative]::SetForegroundWindow($window)

    $scanCode = [OpenSlicksProfileNative]::MapVirtualKey(
        [uint32]$VirtualKey, 0)
    $keyState = [int64]1 -bor ([int64]$scanCode -shl 16)

    if ($Pressed) {
        $message = 0x0100
    }
    else {
        $message = 0x0101
        $keyState = $keyState -bor [int64]0xC0000000
    }

    [void][OpenSlicksProfileNative]::PostMessage(
        $window,
        $message,
        [IntPtr]$VirtualKey,
        [IntPtr]$keyState)
}

function Press-OpenSlicksKey {
    param(
        [Parameter(Mandatory = $true)]
        [System.Diagnostics.Process]$Process,
        [Parameter(Mandatory = $true)]
        [int]$VirtualKey,
        [int]$HoldMilliseconds = 150
    )

    Send-OpenSlicksKey -Process $Process -VirtualKey $VirtualKey `
        -Pressed $true
    Start-Sleep -Milliseconds $HoldMilliseconds
    Send-OpenSlicksKey -Process $Process -VirtualKey $VirtualKey `
        -Pressed $false
}

function Get-OpenSlicksThreadSnapshot {
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
            # Driver helper threads can exit during enumeration.
        }
    }

    return $snapshot
}

function Measure-OpenSlicksProcess {
    param(
        [Parameter(Mandatory = $true)]
        [System.Diagnostics.Process]$Process
    )

    $Process.Refresh()
    $cpuAtStart = $Process.CPU
    $threadsAtStart = Get-OpenSlicksThreadSnapshot -Process $Process
    $stopwatch = [System.Diagnostics.Stopwatch]::StartNew()

    # Player one accelerates throughout the live-race sample.
    Send-OpenSlicksKey -Process $Process -VirtualKey 0x26 -Pressed $true

    try {
        while ($stopwatch.Elapsed.TotalSeconds -lt $MeasureSeconds) {
            if ($Process.HasExited) {
                throw (
                    "OpenSlicks exited during measurement with code {0}." -f
                    $Process.ExitCode)
            }

            Start-Sleep -Milliseconds 40
        }
    }
    finally {
        if (-not $Process.HasExited) {
            Send-OpenSlicksKey -Process $Process -VirtualKey 0x26 `
                -Pressed $false
        }
    }

    $stopwatch.Stop()
    $Process.Refresh()
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
            # The process total remains valid when a helper exits.
        }
    }

    $mainThread = @($threadResults |
        Sort-Object StartMilliseconds, Id |
        Select-Object -First 1)
    $mainThreadCpu = 0.0

    if ($mainThread.Count -ne 0) {
        $mainThreadCpu = $mainThread[0].CpuSeconds
    }

    $cpuSeconds = $Process.CPU - $cpuAtStart

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

function Save-OpenSlicksScreenshot {
    param(
        [Parameter(Mandatory = $true)]
        [System.Diagnostics.Process]$Process,
        [Parameter(Mandatory = $true)]
        [string]$Path
    )

    $window = Wait-OpenSlicksWindow -Process $Process
    $rectangle = New-Object OpenSlicksProfileNative+WindowRectangle

    if (-not [OpenSlicksProfileNative]::GetWindowRect(
            $window, [ref]$rectangle)) {
        throw "The OpenSlicks window rectangle could not be read."
    }

    $width = $rectangle.Right - $rectangle.Left
    $height = $rectangle.Bottom - $rectangle.Top

    if ($width -le 0 -or $height -le 0 -or
        $width -gt 16384 -or $height -gt 16384) {
        throw "OpenSlicks reported an invalid window size."
    }

    $bitmap = New-Object System.Drawing.Bitmap $width, $height
    $graphics = [System.Drawing.Graphics]::FromImage($bitmap)

    try {
        [void][OpenSlicksProfileNative]::SetForegroundWindow($window)
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
        $graphics.Dispose()
        $bitmap.Dispose()
    }
}

function Stop-OpenSlicksProcess {
    param(
        [Parameter(Mandatory = $true)]
        [System.Diagnostics.Process]$Process
    )

    if ($Process.HasExited) {
        return
    }

    $window = Get-OpenSlicksWindow -Process $Process

    if ($window -ne [IntPtr]::Zero) {
        [void][OpenSlicksProfileNative]::PostMessage(
            $window, 0x0010, [IntPtr]0, [IntPtr]0)
        [void]$Process.WaitForExit(3000)
    }

    if (-not $Process.HasExited) {
        Stop-Process -Id $Process.Id -Force
        $Process.WaitForExit()
    }
}

function Invoke-OpenSlicksProfile {
    param(
        [Parameter(Mandatory = $true)]
        [ValidateSet("gfx2", "gfx3")]
        [string]$Runtime,
        [Parameter(Mandatory = $true)]
        [int]$Sample
    )

    $executable = Get-OpenSlicksExecutable -Kind "game" -Runtime $Runtime
    $stem = "openslicks-{0}-{1}-sample{2}" -f
        $Runtime, $compilerTarget, $Sample
    $standardOutput = Join-Path $outputRoot ($stem + ".stdout.log")
    $standardError = Join-Path $outputRoot ($stem + ".stderr.log")
    $screenshot = Join-Path $outputRoot ($stem + ".png")

    Set-OpenSlicksEnvironment -Runtime $Runtime -Profiling $true

    $process = Start-Process -FilePath $executable `
        -WorkingDirectory $openSlicksPath `
        -RedirectStandardOutput $standardOutput `
        -RedirectStandardError $standardError `
        -PassThru
    [void]$process.Handle

    try {
        [void](Wait-OpenSlicksWindow -Process $process)
        Start-Sleep -Milliseconds 1000

        # Enter activates the default Start Race action.
        Press-OpenSlicksKey -Process $process -VirtualKey 13
        Start-Sleep -Milliseconds 2000

        $measurement = Measure-OpenSlicksProcess -Process $process

        if ($CaptureScreenshots) {
            Save-OpenSlicksScreenshot -Process $process -Path $screenshot
        }

        return [pscustomobject]@{
            Program = "OpenSlicks"
            Runtime = $Runtime
            Target = $compilerTarget
            Backend = if ($Runtime -eq "gfx3") {
                if ([string]::IsNullOrWhiteSpace($Gfx3Backend)) {
                    "automatic"
                }
                elseif ($Gfx3Backend -eq "VULKAN" -and
                        $VulkanDeviceIndex -ge 0) {
                    "vulkan-device-" + $VulkanDeviceIndex
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
            Screenshot = if ($CaptureScreenshots) { $screenshot } else { "" }
            StandardOutput = $standardOutput
            StandardError = $standardError
            ThreadCpu = $measurement.ThreadCpu
        }
    }
    finally {
        Stop-OpenSlicksProcess -Process $process
    }
}

# -------------------------------------------------------------------------
# Execution and result persistence
# -------------------------------------------------------------------------

$results = @()
$progressPath = Join-Path $outputRoot "openslicks-profile-progress.json"

try {
    if (-not $SkipTests) {
        foreach ($runtime in $Runtimes) {
            Invoke-OpenSlicksTests -Runtime $runtime
        }
    }

    for ($sample = 1; $sample -le $Samples; $sample++) {
        $preferredOrder = if (($sample % 2) -eq 1) {
            @("gfx2", "gfx3")
        }
        else {
            @("gfx3", "gfx2")
        }

        foreach ($runtime in @($preferredOrder |
            Where-Object { $Runtimes -contains $_ })) {
            Write-Host (
                "Profiling OpenSlicks ({0}, sample {1})" -f
                $runtime, $sample)
            $result = Invoke-OpenSlicksProfile `
                -Runtime $runtime -Sample $sample
            $results += $result
            $results | ConvertTo-Json -Depth 6 |
                Set-Content -LiteralPath $progressPath

            Write-Host (
                "  CPU {0:N1}%  main thread {1:N1}%" -f
                $result.CpuPercent, $result.MainThreadCpuPercent)
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
$jsonPath = Join-Path $outputRoot (
    "openslicks-profile-" + $timestamp + ".json")
$csvPath = Join-Path $outputRoot (
    "openslicks-profile-" + $timestamp + ".csv")

$results | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $jsonPath
$results |
    Select-Object Program,Runtime,Target,Backend,Sample,WallSeconds,
        CpuSeconds,CpuPercent,MainThreadCpuSeconds,MainThreadCpuPercent,
        Responding,StandardOutput,StandardError |
    Export-Csv -LiteralPath $csvPath -NoTypeInformation

Write-Output "OPENSLICKS_PROFILE_RESULTS=$jsonPath"
Write-Output "OPENSLICKS_PROFILE_SUMMARY=$csvPath"

# end of profile-openslicks.ps1

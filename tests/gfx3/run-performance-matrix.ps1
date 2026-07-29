#
# Project: FreeBASIC gfxlib3 tests
# --------------------------------
#
# File: run-performance-matrix.ps1
#
# Purpose:
#
#     Build and run the reproducible desktop performance comparison between
#     gfxlib2 and each desktop gfxlib3 GPU backend.
#
# Responsibilities:
#
#     - compile each benchmark against its intended graphics archive
#     - preserve the benchmark's machine-readable result lines
#     - fail immediately when compilation or a benchmark process fails
#     - keep generated executables outside the source tree by default
#
# This file intentionally does NOT contain:
#
#     - vendor-specific pass/fail timing thresholds
#     - Android packaging or installation policy
#     - changes to benchmark workloads at run time
#

[CmdletBinding()]
param(
    [string]$Compiler = "",
    [string]$OutputDirectory = "",
    [ValidateRange(1, 99)]
    [int]$Runs = 3
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$testRoot = $PSScriptRoot
if ([string]::IsNullOrWhiteSpace($Compiler)) {
    $Compiler = Join-Path $testRoot "..\..\bin\fbc.exe"
}
if ([string]::IsNullOrWhiteSpace($OutputDirectory)) {
    $OutputDirectory = Join-Path $env:TEMP "gfxlib3-performance"
}
$includeRoot = (Resolve-Path (Join-Path $testRoot "..\..\inc")).Path
$compilerPath = (Resolve-Path $Compiler).Path

New-Item -ItemType Directory -Force -Path $OutputDirectory | Out-Null

function Build-Benchmark {
    param(
        [string]$Source,
        [string]$OutputName,
        [string[]]$Options
    )

    $sourcePath = Join-Path $testRoot $Source
    $outputPath = Join-Path $OutputDirectory $OutputName
    $arguments = @("-i", $includeRoot) + $Options + @($sourcePath, "-x", $outputPath)

    & $compilerPath @arguments
    if ($LASTEXITCODE -ne 0) {
        throw "Compilation failed: $Source ($OutputName)"
    }
    return $outputPath
}

function Run-Benchmark {
    param(
        [string]$Name,
        [string]$Executable,
        [int]$RunCount
    )

    $timings = @{}

    Write-Output "BENCHMARK=$Name"
    for ($run = 1; $run -le $RunCount; $run++) {
        #
        # A visible renderer is affected by compositing, GPU clocks, and the
        # previous process. Preserve each completed-work sample instead of
        # treating one convenient run as a performance conclusion.
        #
        Write-Output "BENCHMARK_SAMPLE=$run"
        $sampleOutput = @(& $Executable)
        if ($LASTEXITCODE -ne 0) {
            throw "Benchmark failed: $Name sample $run (exit $LASTEXITCODE)"
        }
        foreach ($line in $sampleOutput) {
            $text = [string]$line

            Write-Output $text
            if ($text -match '^([A-Za-z0-9_]+_seconds)=\s*([-+0-9.Ee]+)\s*$') {
                $value = 0.0

                if ([double]::TryParse($Matches[2],
                    [Globalization.NumberStyles]::Float,
                    [Globalization.CultureInfo]::InvariantCulture, [ref]$value)) {
                    if (-not $timings.ContainsKey($Matches[1])) {
                        $timings[$Matches[1]] = [System.Collections.Generic.List[double]]::new()
                    }
                    $timings[$Matches[1]].Add($value)
                }
            }
        }
    }
    foreach ($timingName in ($timings.Keys | Sort-Object)) {
        $values = @($timings[$timingName] | Sort-Object)
        $middle = [int]($values.Count / 2)

        if (($values.Count % 2) -eq 0) {
            $median = ($values[$middle - 1] + $values[$middle]) / 2.0
        } else {
            $median = $values[$middle]
        }
        Write-Output ("BENCHMARK_MEDIAN_{0}={1:R}" -f $timingName, $median)
    }
}

Write-Output "PERFORMANCE_RUNS=$Runs"

$matrix = @(
    @{ Source = "mode-open-benchmark.bas"; Name = "mode-open-gfxlib2"; Output = "mode-open-gfxlib2.exe"; Options = @() },
    @{ Source = "mode-open-benchmark.bas"; Name = "mode-open-gfx3-opengl"; Output = "mode-open-gfx3-opengl.exe"; Options = @("-gfx3", "-d", "GFX3_OPENGL_TEST") },
    @{ Source = "mode-open-benchmark.bas"; Name = "mode-open-gfx3-vulkan"; Output = "mode-open-gfx3-vulkan.exe"; Options = @("-gfx3", "-d", "GFX3_VULKAN_TEST") },
    @{ Source = "console-benchmark.bas"; Name = "console-gfxlib2"; Output = "console-gfxlib2.exe"; Options = @() },
    @{ Source = "console-benchmark.bas"; Name = "console-gfx3-opengl"; Output = "console-gfx3-opengl.exe"; Options = @("-gfx3", "-d", "GFX3_OPENGL_TEST") },
    @{ Source = "console-benchmark.bas"; Name = "console-gfx3-vulkan"; Output = "console-gfx3-vulkan.exe"; Options = @("-gfx3", "-d", "GFX3_VULKAN_TEST") },
    @{ Source = "primitive-benchmark.bas"; Name = "primitive-gfxlib2"; Output = "primitive-gfxlib2.exe"; Options = @() },
    @{ Source = "primitive-benchmark.bas"; Name = "primitive-gfx3-opengl"; Output = "primitive-gfx3-opengl.exe"; Options = @("-gfx3", "-d", "GFX3_OPENGL_TEST") },
    @{ Source = "primitive-benchmark.bas"; Name = "primitive-gfx3-vulkan"; Output = "primitive-gfx3-vulkan.exe"; Options = @("-gfx3", "-d", "GFX3_VULKAN_TEST") },
    @{ Source = "pset-benchmark.bas"; Name = "pset-gfxlib2"; Output = "pset-gfxlib2.exe"; Options = @() },
    @{ Source = "pset-benchmark.bas"; Name = "pset-gfx3-opengl"; Output = "pset-gfx3-opengl.exe"; Options = @("-gfx3", "-d", "GFX3_OPENGL_TEST") },
    @{ Source = "pset-benchmark.bas"; Name = "pset-gfx3-vulkan"; Output = "pset-gfx3-vulkan.exe"; Options = @("-gfx3", "-d", "GFX3_VULKAN_TEST") },
    @{ Source = "paint-benchmark.bas"; Name = "paint-gfxlib2"; Output = "paint-gfxlib2.exe"; Options = @() },
    @{ Source = "paint-benchmark.bas"; Name = "paint-gfx3-opengl"; Output = "paint-gfx3-opengl.exe"; Options = @("-gfx3", "-d", "GFX3_OPENGL_TEST") },
    @{ Source = "paint-benchmark.bas"; Name = "paint-gfx3-vulkan"; Output = "paint-gfx3-vulkan.exe"; Options = @("-gfx3", "-d", "GFX3_VULKAN_TEST") },
    @{ Source = "arc-benchmark.bas"; Name = "arc-gfxlib2"; Output = "arc-gfxlib2.exe"; Options = @() },
    @{ Source = "arc-benchmark.bas"; Name = "arc-gfx3-opengl"; Output = "arc-gfx3-opengl.exe"; Options = @("-gfx3", "-d", "GFX3_OPENGL_TEST") },
    @{ Source = "arc-benchmark.bas"; Name = "arc-gfx3-vulkan"; Output = "arc-gfx3-vulkan.exe"; Options = @("-gfx3", "-d", "GFX3_VULKAN_TEST") },
    @{ Source = "transfer-benchmark.bas"; Name = "transfer-gfxlib2"; Output = "transfer-gfxlib2.exe"; Options = @() },
    @{ Source = "transfer-benchmark.bas"; Name = "transfer-gfx3-opengl"; Output = "transfer-gfx3-opengl.exe"; Options = @("-gfx3", "-d", "GFX3_OPENGL_TEST") },
    @{ Source = "transfer-benchmark.bas"; Name = "transfer-gfx3-vulkan"; Output = "transfer-gfx3-vulkan.exe"; Options = @("-gfx3", "-d", "GFX3_VULKAN_TEST") },
    @{ Source = "transfer-path-benchmark.bas"; Name = "transfer-path-gfxlib2"; Output = "transfer-path-gfxlib2.exe"; Options = @() },
    @{ Source = "transfer-path-benchmark.bas"; Name = "transfer-path-gfx3-opengl"; Output = "transfer-path-gfx3-opengl.exe"; Options = @("-gfx3", "-d", "GFX3_OPENGL_TEST") },
    @{ Source = "transfer-path-benchmark.bas"; Name = "transfer-path-gfx3-vulkan"; Output = "transfer-path-gfx3-vulkan.exe"; Options = @("-gfx3", "-d", "GFX3_VULKAN_TEST") },
    @{ Source = "draw-benchmark.bas"; Name = "draw-gfxlib2"; Output = "draw-gfxlib2.exe"; Options = @() },
    @{ Source = "draw-benchmark.bas"; Name = "draw-gfx3-opengl"; Output = "draw-gfx3-opengl.exe"; Options = @("-gfx3", "-d", "GFX3_OPENGL_TEST") },
    @{ Source = "draw-benchmark.bas"; Name = "draw-gfx3-vulkan"; Output = "draw-gfx3-vulkan.exe"; Options = @("-gfx3", "-d", "GFX3_VULKAN_TEST") },
    @{ Source = "screen-state-benchmark.bas"; Name = "screen-state-gfxlib2"; Output = "screen-state-gfxlib2.exe"; Options = @() },
    @{ Source = "screen-state-benchmark.bas"; Name = "screen-state-gfx3-opengl"; Output = "screen-state-gfx3-opengl.exe"; Options = @("-gfx3", "-d", "GFX3_OPENGL_TEST") },
    @{ Source = "screen-state-benchmark.bas"; Name = "screen-state-gfx3-vulkan"; Output = "screen-state-gfx3-vulkan.exe"; Options = @("-gfx3", "-d", "GFX3_VULKAN_TEST") },
    @{ Source = "palette-family-benchmark.bas"; Name = "palette-family-gfxlib2"; Output = "palette-family-gfxlib2.exe"; Options = @() },
    @{ Source = "palette-family-benchmark.bas"; Name = "palette-family-gfx3-opengl"; Output = "palette-family-gfx3-opengl.exe"; Options = @("-gfx3", "-d", "GFX3_OPENGL_TEST") },
    @{ Source = "palette-family-benchmark.bas"; Name = "palette-family-gfx3-vulkan"; Output = "palette-family-gfx3-vulkan.exe"; Options = @("-gfx3", "-d", "GFX3_VULKAN_TEST") },
    @{ Source = "control-query-benchmark.bas"; Name = "control-query-gfxlib2"; Output = "control-query-gfxlib2.exe"; Options = @() },
    @{ Source = "control-query-benchmark.bas"; Name = "control-query-gfx3-opengl"; Output = "control-query-gfx3-opengl.exe"; Options = @("-gfx3", "-d", "GFX3_OPENGL_TEST") },
    @{ Source = "control-query-benchmark.bas"; Name = "control-query-gfx3-vulkan"; Output = "control-query-gfx3-vulkan.exe"; Options = @("-gfx3", "-d", "GFX3_VULKAN_TEST") },
    @{ Source = "coordinate-state-benchmark.bas"; Name = "coordinate-state-gfxlib2"; Output = "coordinate-state-gfxlib2.exe"; Options = @() },
    @{ Source = "coordinate-state-benchmark.bas"; Name = "coordinate-state-gfx3-opengl"; Output = "coordinate-state-gfx3-opengl.exe"; Options = @("-gfx3", "-d", "GFX3_OPENGL_TEST") },
    @{ Source = "coordinate-state-benchmark.bas"; Name = "coordinate-state-gfx3-vulkan"; Output = "coordinate-state-gfx3-vulkan.exe"; Options = @("-gfx3", "-d", "GFX3_VULKAN_TEST") },
    @{ Source = "gpu-surface-benchmark.bas"; Name = "gpu-surface-gfx3-opengl"; Output = "gpu-surface-gfx3-opengl.exe"; Options = @("-gfx3", "-d", "GFX3_OPENGL_TEST") },
    @{ Source = "gpu-surface-benchmark.bas"; Name = "gpu-surface-gfx3-vulkan"; Output = "gpu-surface-gfx3-vulkan.exe"; Options = @("-gfx3", "-d", "GFX3_VULKAN_TEST") },
    @{ Source = "gpu-sprite-benchmark.bas"; Name = "gpu-sprite-gfx3-opengl"; Output = "gpu-sprite-gfx3-opengl.exe"; Options = @("-gfx3", "-d", "GFX3_OPENGL_TEST") },
    @{ Source = "gpu-sprite-benchmark.bas"; Name = "gpu-sprite-gfx3-vulkan"; Output = "gpu-sprite-gfx3-vulkan.exe"; Options = @("-gfx3", "-d", "GFX3_VULKAN_TEST") },
    @{ Source = "gpu-transform-benchmark.bas"; Name = "gpu-transform-gfx3-opengl"; Output = "gpu-transform-gfx3-opengl.exe"; Options = @("-gfx3", "-d", "GFX3_OPENGL_TEST") },
    @{ Source = "gpu-transform-benchmark.bas"; Name = "gpu-transform-gfx3-vulkan"; Output = "gpu-transform-gfx3-vulkan.exe"; Options = @("-gfx3", "-d", "GFX3_VULKAN_TEST") },
    @{ Source = "image-allocation-benchmark.bas"; Name = "image-allocation-gfxlib2"; Output = "image-allocation-gfxlib2.exe"; Options = @() },
    @{ Source = "image-allocation-benchmark.bas"; Name = "image-allocation-gfx3"; Output = "image-allocation-gfx3.exe"; Options = @("-gfx3") },
    @{ Source = "image-cache-benchmark.bas"; Name = "image-cache-gfxlib2"; Output = "image-cache-gfxlib2.exe"; Options = @() },
    @{ Source = "image-cache-benchmark.bas"; Name = "image-cache-gfx3-opengl"; Output = "image-cache-gfx3-opengl.exe"; Options = @("-gfx3", "-d", "GFX3_OPENGL_TEST") },
    @{ Source = "image-cache-benchmark.bas"; Name = "image-cache-gfx3-vulkan"; Output = "image-cache-gfx3-vulkan.exe"; Options = @("-gfx3", "-d", "GFX3_VULKAN_TEST") },
    @{ Source = "large-image-cache-smoke.bas"; Name = "large-image-cache-gfxlib2"; Output = "large-image-cache-gfxlib2.exe"; Options = @() },
    @{ Source = "large-image-cache-smoke.bas"; Name = "large-image-cache-gfx3-opengl"; Output = "large-image-cache-gfx3-opengl.exe"; Options = @("-gfx3", "-d", "GFX3_OPENGL_TEST") },
    @{ Source = "large-image-cache-smoke.bas"; Name = "large-image-cache-gfx3-vulkan"; Output = "large-image-cache-gfx3-vulkan.exe"; Options = @("-gfx3", "-d", "GFX3_VULKAN_TEST") },
    @{ Source = "file-row-benchmark.bas"; Name = "file-row-gfxlib2"; Output = "file-row-gfxlib2.exe"; Options = @() },
    @{ Source = "file-row-benchmark.bas"; Name = "file-row-gfx3-opengl"; Output = "file-row-gfx3-opengl.exe"; Options = @("-gfx3", "-d", "GFX3_OPENGL_TEST") },
    @{ Source = "file-row-benchmark.bas"; Name = "file-row-gfx3-vulkan"; Output = "file-row-gfx3-vulkan.exe"; Options = @("-gfx3", "-d", "GFX3_VULKAN_TEST") },
    @{ Source = "oma-sprite-benchmark.bas"; Name = "oma-sprite-gfxlib2"; Output = "oma-sprite-gfxlib2.exe"; Options = @() },
    @{ Source = "oma-sprite-benchmark.bas"; Name = "oma-sprite-gfx3-opengl"; Output = "oma-sprite-gfx3-opengl.exe"; Options = @("-gfx3", "-d", "GFX3_OPENGL_TEST") },
    @{ Source = "oma-sprite-benchmark.bas"; Name = "oma-sprite-gfx3-vulkan"; Output = "oma-sprite-gfx3-vulkan.exe"; Options = @("-gfx3", "-d", "GFX3_VULKAN_TEST") },
    @{ Source = "put-clipping-benchmark.bas"; Name = "put-clipping-gfxlib2"; Output = "put-clipping-gfxlib2.exe"; Options = @() },
    @{ Source = "put-clipping-benchmark.bas"; Name = "put-clipping-gfx3-opengl"; Output = "put-clipping-gfx3-opengl.exe"; Options = @("-gfx3", "-d", "GFX3_OPENGL_TEST") },
    @{ Source = "put-clipping-benchmark.bas"; Name = "put-clipping-gfx3-vulkan"; Output = "put-clipping-gfx3-vulkan.exe"; Options = @("-gfx3", "-d", "GFX3_VULKAN_TEST") },
    @{ Source = "sprite-offload-benchmark.bas"; Name = "sprite-offload-gfxlib2"; Output = "sprite-offload-gfxlib2.exe"; Options = @() },
    @{ Source = "sprite-offload-benchmark.bas"; Name = "sprite-offload-gfx3-opengl"; Output = "sprite-offload-gfx3-opengl.exe"; Options = @("-gfx3", "-d", "GFX3_OPENGL_TEST") },
    @{ Source = "sprite-offload-benchmark.bas"; Name = "sprite-offload-gfx3-vulkan"; Output = "sprite-offload-gfx3-vulkan.exe"; Options = @("-gfx3", "-d", "GFX3_VULKAN_TEST") }
)

foreach ($entry in $matrix) {
    $executable = Build-Benchmark -Source $entry.Source -OutputName $entry.Output -Options $entry.Options
    Run-Benchmark -Name $entry.Name -Executable $executable -RunCount $Runs
}

# end of run-performance-matrix.ps1

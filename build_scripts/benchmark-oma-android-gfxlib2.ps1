<#
    Project: FreeBASIC OMA Android performance tests
    ------------------------------------------------

    File: benchmark-oma-android-gfxlib2.ps1

    Purpose:

        Measure the runtime behavior of every installed OMA gfxlib2 game on
        one Android device.

    Responsibilities:

        * launch every package from the successful gfxlib2 build summary
        * enter gameplay when a safe keyboard start action is known
        * collect native SurfaceFlinger presentation timestamps
        * sample process CPU, resident memory, and device CPU temperature
        * retain a screenshot and isolated error log for every measurement
        * restore the device display timeout when the run finishes

    This file intentionally does NOT contain:

        * APK compilation or installation
        * renderer selection
        * subjective gameplay scoring
        * destructive package or application-data operations
#>

[CmdletBinding()]
param(
    [string]$Adb = 'C:\Nextcloud\Android adb\platform-tools\adb.exe',
    [string]$BuildDirectory = '',
    [string]$Serial = '',
    [int]$InitializationSeconds = 5,
    [int]$SampleSeconds = 10,
    [int]$CpuSamples = 5,
    [switch]$Resume
)

$ErrorActionPreference = 'Stop'

$repositoryRoot = Split-Path -Parent $PSScriptRoot

if ([string]::IsNullOrWhiteSpace($BuildDirectory)) {
    $BuildDirectory = Join-Path $repositoryRoot '.build-gfx2-android\games'
}

$BuildDirectory = [System.IO.Path]::GetFullPath($BuildDirectory)
$summaryPath = Join-Path $BuildDirectory 'oma-gfxlib2-android-build.json'
$resultDirectory = Join-Path $BuildDirectory 'performance-tests'

# ---------------------------------------------------------------------------
# Validation and ADB helpers
# ---------------------------------------------------------------------------

function Require-File {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Path
    )

    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        throw "Required file was not found: $Path"
    }
}

function Invoke-Adb {
    param(
        [Parameter(Mandatory = $true)]
        [string[]]$Arguments
    )

    <#
        ADB writes some successful operations to standard error on Windows.
        Native exit status is therefore the authoritative result.
    #>
    $previousErrorAction = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'

    try {
        $output = @(& $Adb -s $Serial @Arguments 2>&1)
        $exitCode = $LASTEXITCODE
    }
    finally {
        $ErrorActionPreference = $previousErrorAction
    }

    return [pscustomobject]@{
        ExitCode = $exitCode
        Output = $output
    }
}

function Require-AdbSuccess {
    param(
        [Parameter(Mandatory = $true)]
        [object]$Result,

        [Parameter(Mandatory = $true)]
        [string]$Operation
    )

    if ($Result.ExitCode -ne 0) {
        throw (
            "$Operation failed with exit code $($Result.ExitCode): " +
            ($Result.Output -join [Environment]::NewLine))
    }
}

function Get-AndroidCpuTemperature {
    $result = Invoke-Adb -Arguments @(
        'shell',
        'cat',
        '/sys/class/thermal/thermal_zone5/temp'
    )

    if ($result.ExitCode -ne 0) {
        return $null
    }

    $rawValue = (($result.Output -join '').Trim())

    if ($rawValue -notmatch '^-?\d+$') {
        return $null
    }

    return [Math]::Round(([double]$rawValue / 1000.0), 1)
}

function Get-Percentile {
    param(
        [Parameter(Mandatory = $true)]
        [AllowEmptyCollection()]
        [double[]]$Values,

        [Parameter(Mandatory = $true)]
        [double]$Percentile
    )

    if ($Values.Count -eq 0) {
        return 0.0
    }

    $ordered = @($Values | Sort-Object)
    $index = [int][Math]::Ceiling(
        $Percentile * [double]($ordered.Count - 1))

    if ($index -lt 0) {
        $index = 0
    }

    if ($index -ge $ordered.Count) {
        $index = $ordered.Count - 1
    }

    return [double]$ordered[$index]
}

function Measure-PresentationTiming {
    param(
        [Parameter(Mandatory = $true)]
        [object[]]$LatencyOutput
    )

    $timestamps = [System.Collections.Generic.List[double]]::new()

    foreach ($line in @($LatencyOutput | Select-Object -Skip 1)) {
        $lineText = $line.ToString()

        if ($lineText -notmatch '^\s*(\d+)\s+(\d+)\s+(\d+)\s*$') {
            continue
        }

        $actualPresentTime = [double]$Matches[2]

        if ($actualPresentTime -le 0 -or
            $actualPresentTime -ge 9.0E+18) {
            continue
        }

        $timestamps.Add($actualPresentTime)
    }

    $intervalsMs = [System.Collections.Generic.List[double]]::new()

    for ($index = 1; $index -lt $timestamps.Count; $index++) {
        $interval = (
            $timestamps[$index] - $timestamps[$index - 1]) / 1000000.0

        if ($interval -gt 0 -and $interval -lt 10000.0) {
            $intervalsMs.Add($interval)
        }
    }

    $framesPerSecond = 0.0

    if ($timestamps.Count -gt 1) {
        $durationSeconds = (
            $timestamps[$timestamps.Count - 1] - $timestamps[0]) /
            1000000000.0

        if ($durationSeconds -gt 0) {
            $framesPerSecond = (
                [double]($timestamps.Count - 1) / $durationSeconds)
        }
    }

    $medianMs = Get-Percentile -Values $intervalsMs.ToArray() `
        -Percentile 0.50
    $p95Ms = Get-Percentile -Values $intervalsMs.ToArray() `
        -Percentile 0.95
    $worstMs = Get-Percentile -Values $intervalsMs.ToArray() `
        -Percentile 1.00
    $stallThresholdMs = [Math]::Max(100.0, $medianMs * 2.5)
    $stallCount = @(
        $intervalsMs | Where-Object { $_ -gt $stallThresholdMs }
    ).Count

    return [pscustomobject]@{
        FrameCount = $timestamps.Count
        FramesPerSecond = [Math]::Round($framesPerSecond, 2)
        MedianFrameMs = [Math]::Round($medianMs, 2)
        P95FrameMs = [Math]::Round($p95Ms, 2)
        WorstFrameMs = [Math]::Round($worstMs, 2)
        StallCount = $stallCount
    }
}

function Measure-ProcessCpu {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Package
    )

    $topCommand = (
        "top -n $CpuSamples -d 1 -m 100 | grep " + $Package)
    $topResult = Invoke-Adb -Arguments @('shell', $topCommand)
    $samples = [System.Collections.Generic.List[double]]::new()

    foreach ($line in $topResult.Output) {
        $lineText = $line.ToString()

        if ($lineText -match (
            '^\s*\d+\s+\S+\s+-?\d+\s+-?\d+\s+' +
            '([0-9]+(?:\.[0-9]+)?)%\s')) {
            $samples.Add([double]$Matches[1])
        }
    }

    if ($samples.Count -eq 0) {
        return [pscustomobject]@{
            SampleCount = 0
            AveragePercent = 0.0
            MedianPercent = 0.0
            PeakPercent = 0.0
        }
    }

    return [pscustomobject]@{
        SampleCount = $samples.Count
        AveragePercent = [Math]::Round(
            (($samples | Measure-Object -Average).Average), 1)
        MedianPercent = [Math]::Round(
            (Get-Percentile -Values $samples.ToArray() -Percentile 0.50), 1)
        PeakPercent = [Math]::Round(
            (($samples | Measure-Object -Maximum).Maximum), 1)
    }
}

function Get-ResidentMemoryKb {
    param(
        [Parameter(Mandatory = $true)]
        [string]$ProcessId
    )

    if ([string]::IsNullOrWhiteSpace($ProcessId)) {
        return 0
    }

    $status = Invoke-Adb -Arguments @(
        'shell',
        'cat',
        "/proc/$ProcessId/status"
    )
    $rssLine = $status.Output |
        Select-String -Pattern '^\s*VmRSS:\s+(\d+)\s+kB' |
        Select-Object -First 1

    if ($null -eq $rssLine) {
        return 0
    }

    if ($rssLine.Line -notmatch '^\s*VmRSS:\s+(\d+)\s+kB') {
        return 0
    }

    return [int]$Matches[1]
}

function Write-Results {
    param(
        [Parameter(Mandatory = $true)]
        [object[]]$Results,

        [Parameter(Mandatory = $true)]
        [string]$BaseName
    )

    $jsonPath = Join-Path $resultDirectory ($BaseName + '.json')
    $csvPath = Join-Path $resultDirectory ($BaseName + '.csv')

    $Results |
        ConvertTo-Json -Depth 5 |
        Set-Content -LiteralPath $jsonPath -Encoding UTF8
    $Results |
        Export-Csv -LiteralPath $csvPath -NoTypeInformation -Encoding UTF8
}

Require-File -Path $Adb
Require-File -Path $summaryPath

if ($InitializationSeconds -lt 3 -or $InitializationSeconds -gt 30) {
    throw 'InitializationSeconds must be between 3 and 30.'
}

if ($SampleSeconds -lt 5 -or $SampleSeconds -gt 60) {
    throw 'SampleSeconds must be between 5 and 60.'
}

if ($CpuSamples -lt 2 -or $CpuSamples -gt $SampleSeconds) {
    throw 'CpuSamples must be between 2 and SampleSeconds.'
}

$buildSummary = Get-Content -LiteralPath $summaryPath -Raw |
    ConvertFrom-Json
$buildRecordCount = @($buildSummary).Count

if ($buildRecordCount -ne 14) {
    throw "Expected fourteen build records, found $buildRecordCount."
}

foreach ($record in $buildSummary) {
    if (-not $record.Success) {
        throw "Build summary contains a failed package: $($record.Name)"
    }
}

if ([string]::IsNullOrWhiteSpace($Serial)) {
    $devices = @(
        & $Adb devices |
            Select-Object -Skip 1 |
            ForEach-Object {
                if ($_ -match '^(\S+)\s+device$') {
                    $Matches[1]
                }
            }
    )

    if ($devices.Count -ne 1) {
        throw (
            'Serial was not supplied and exactly one ready Android device ' +
            "was not found. Ready device count: $($devices.Count)")
    }

    $Serial = $devices[0]
}

$deviceState = @(& $Adb -s $Serial get-state 2>&1)

if ($LASTEXITCODE -ne 0 -or ($deviceState -join '').Trim() -ne 'device') {
    throw "Android device is not ready: $Serial"
}

New-Item -ItemType Directory -Path $resultDirectory -Force | Out-Null

# ---------------------------------------------------------------------------
# Per-game benchmark actions
# ---------------------------------------------------------------------------

<#
    These keys only advance screens with an established, harmless start
    action. Menu-heavy games are measured at their live first screen and are
    exercised separately with touch input during review.
#>
$startKeyCodes = @{
    'arkanoid-test' = @(62)
    'behold' = @(62)
    'demolitionderby' = @(66)
    'kinematics-self' = @(62)
    'nietzsche' = @(66)
    'qfak' = @(66)
    'rambo' = @(62)
    'starphalanx' = @(62)
    'vtrek' = @(66)
}

$results = [System.Collections.Generic.List[object]]::new()
$progressJsonPath = Join-Path $resultDirectory `
    'oma-performance-progress.json'

if ($Resume -and
    (Test-Path -LiteralPath $progressJsonPath -PathType Leaf)) {
    $savedResults = Get-Content -LiteralPath $progressJsonPath -Raw |
        ConvertFrom-Json

    foreach ($savedResult in @($savedResults)) {
        $results.Add($savedResult)
    }

    Write-Host "RESUMING AFTER $($results.Count) SAVED RESULTS"
}

$activityClass = 'org.freebasic.android.FreeBasicNativeActivity'
$originalScreenTimeout = (
    (Invoke-Adb -Arguments @(
        'shell',
        'settings',
        'get',
        'system',
        'screen_off_timeout'
    )).Output -join ''
).Trim()

$fatalPattern = (
    'FATAL EXCEPTION|ANR in|Fatal signal|linker.*cannot locate|' +
    'exited due to signal|Process .* has died')

try {
    Require-AdbSuccess -Operation 'Extending screen timeout' -Result (
        Invoke-Adb -Arguments @(
            'shell',
            'settings',
            'put',
            'system',
            'screen_off_timeout',
            '600000'
        ))

    $null = Invoke-Adb -Arguments @('shell', 'input', 'keyevent', '224')
    $null = Invoke-Adb -Arguments @('shell', 'wm', 'dismiss-keyguard')
    $null = Invoke-Adb -Arguments @('shell', 'input', 'keyevent', '82')

    foreach ($record in $buildSummary) {
        if ($null -ne (
            $results |
                Where-Object { $_.Name -eq $record.Name } |
                Select-Object -First 1)) {
            Write-Host "SKIP SAVED $($record.Name)"
            continue
        }

        $component = $record.Package + '/' + $activityClass
        $surfaceName = $component
        $screenshotPath = Join-Path $resultDirectory (
            $record.Name + '.png')
        $logcatPath = Join-Path $resultDirectory (
            $record.Name + '-errors.txt')

        Write-Host "BENCHMARK $($record.Name) [$($record.Package)]"

        $null = Invoke-Adb -Arguments @('shell', 'input', 'keyevent', '3')
        Start-Sleep -Milliseconds 500
        $null = Invoke-Adb -Arguments @(
            'shell',
            'am',
            'force-stop',
            $record.Package
        )
        $null = Invoke-Adb -Arguments @('logcat', '-c')

        $temperatureBefore = Get-AndroidCpuTemperature
        $launch = Invoke-Adb -Arguments @(
            'shell',
            'am',
            'start',
            '-n',
            $component
        )
        $launchOk = (
            $launch.ExitCode -eq 0 -and
            ($launch.Output -match 'Starting: Intent|Warning: Activity not started'))

        Start-Sleep -Seconds $InitializationSeconds

        if ($startKeyCodes.ContainsKey($record.Name)) {
            foreach ($keyCode in $startKeyCodes[$record.Name]) {
                $null = Invoke-Adb -Arguments @(
                    'shell',
                    'input',
                    'keyevent',
                    [string]$keyCode
                )
                Start-Sleep -Milliseconds 750
            }
        }

        $null = Invoke-Adb -Arguments @(
            'shell',
            'dumpsys',
            'SurfaceFlinger',
            '--latency-clear'
        )

        $cpu = Measure-ProcessCpu -Package $record.Package
        $remainingSeconds = $SampleSeconds - $CpuSamples

        if ($remainingSeconds -gt 0) {
            Start-Sleep -Seconds $remainingSeconds
        }

        $latency = Invoke-Adb -Arguments @(
            'shell',
            'dumpsys',
            'SurfaceFlinger',
            '--latency',
            $surfaceName
        )
        $timing = Measure-PresentationTiming -LatencyOutput $latency.Output

        $pidResult = Invoke-Adb -Arguments @(
            'shell',
            'pidof',
            $record.Package
        )
        $gameProcessId = (($pidResult.Output -join ' ').Trim())
        $residentMemoryKb = Get-ResidentMemoryKb `
            -ProcessId $gameProcessId

        $activity = Invoke-Adb -Arguments @(
            'shell',
            'dumpsys',
            'activity',
            'activities'
        )
        $foreground = $null -ne (
            $activity.Output |
                Select-String -Pattern (
                    'mResumedActivity.*' +
                    [regex]::Escape($record.Package)) |
                Select-Object -First 1)

        $remoteScreenshot = (
            '/sdcard/oma-performance-' + $record.Name + '.png')
        $capture = Invoke-Adb -Arguments @(
            'shell',
            'screencap',
            '-p',
            $remoteScreenshot
        )
        $pull = Invoke-Adb -Arguments @(
            'pull',
            $remoteScreenshot,
            $screenshotPath
        )
        $null = Invoke-Adb -Arguments @(
            'shell',
            'rm',
            '-f',
            $remoteScreenshot
        )
        $screenshotOk = (
            $capture.ExitCode -eq 0 -and
            $pull.ExitCode -eq 0 -and
            (Test-Path -LiteralPath $screenshotPath -PathType Leaf) -and
            (Get-Item -LiteralPath $screenshotPath).Length -gt 1024)

        $errors = Invoke-Adb -Arguments @(
            'logcat',
            '-d',
            '-v',
            'threadtime',
            'AndroidRuntime:E',
            'ActivityManager:W',
            '*:S'
        )
        $fatalLines = @(
            $errors.Output |
                Select-String -Pattern $fatalPattern -CaseSensitive:$false
        )
        $errors.Output |
            Set-Content -LiteralPath $logcatPath -Encoding UTF8

        $temperatureAfter = Get-AndroidCpuTemperature
        $processAlive = -not [string]::IsNullOrWhiteSpace($gameProcessId)
        $completed = (
            $launchOk -and
            $processAlive -and
            $foreground -and
            $screenshotOk -and
            $fatalLines.Count -eq 0)

        $result = [pscustomobject]@{
            Name = $record.Name
            Package = $record.Package
            LaunchOk = $launchOk
            ProcessAlive = $processAlive
            Foreground = $foreground
            ScreenshotCaptured = $screenshotOk
            FatalLineCount = $fatalLines.Count
            Completed = $completed
            StartKeySent = $startKeyCodes.ContainsKey($record.Name)
            ProcessId = $gameProcessId
            FrameCount = $timing.FrameCount
            FramesPerSecond = $timing.FramesPerSecond
            MedianFrameMs = $timing.MedianFrameMs
            P95FrameMs = $timing.P95FrameMs
            WorstFrameMs = $timing.WorstFrameMs
            StallCount = $timing.StallCount
            CpuSampleCount = $cpu.SampleCount
            CpuAveragePercent = $cpu.AveragePercent
            CpuMedianPercent = $cpu.MedianPercent
            CpuPeakPercent = $cpu.PeakPercent
            ResidentMemoryMb = [Math]::Round(
                ([double]$residentMemoryKb / 1024.0), 1)
            CpuTemperatureBeforeC = $temperatureBefore
            CpuTemperatureAfterC = $temperatureAfter
            Screenshot = $screenshotPath
            ErrorLog = $logcatPath
        }

        $results.Add($result)
        Write-Results -Results $results.ToArray() `
            -BaseName 'oma-performance-progress'

        Write-Host (
            "  FPS=$($timing.FramesPerSecond) " +
            "p95=$($timing.P95FrameMs)ms " +
            "CPU=$($cpu.MedianPercent)% " +
            "RSS=$($result.ResidentMemoryMb)MB " +
            "temp=$temperatureAfter C " +
            "complete=$completed")

        $null = Invoke-Adb -Arguments @(
            'shell',
            'am',
            'force-stop',
            $record.Package
        )
    }
}
finally {
    if ($originalScreenTimeout -match '^\d+$') {
        $null = Invoke-Adb -Arguments @(
            'shell',
            'settings',
            'put',
            'system',
            'screen_off_timeout',
            $originalScreenTimeout
        )
    }

    $null = Invoke-Adb -Arguments @('shell', 'input', 'keyevent', '3')
}

# ---------------------------------------------------------------------------
# Final report
# ---------------------------------------------------------------------------

Write-Results -Results $results.ToArray() `
    -BaseName 'oma-performance-results'

$completedCount = @($results | Where-Object { $_.Completed }).Count

Write-Host "COMPLETED $completedCount of $($results.Count)"
Write-Host (
    'RESULTS ' +
    (Join-Path $resultDirectory 'oma-performance-results.json'))

if ($results.Count -ne $buildRecordCount) {
    throw (
        "Only $($results.Count) of $buildRecordCount games were measured.")
}

if ($completedCount -ne $results.Count) {
    throw 'One or more performance measurements did not complete cleanly.'
}

<# end of benchmark-oma-android-gfxlib2.ps1 #>

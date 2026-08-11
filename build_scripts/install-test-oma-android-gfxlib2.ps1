<#
    Project: FreeBASIC OMA Android device tests
    -------------------------------------------

    File: install-test-oma-android-gfxlib2.ps1

    Purpose:

        Install and smoke-test every APK produced by the OMA Android gfxlib2
        build matrix on one connected Android device.

    Responsibilities:

        * pin all ADB operations to one verified device serial
        * install every APK recorded in the successful build summary
        * launch each NativeActivity and allow bounded initialization time
        * confirm the process and foreground activity remain alive
        * retain a screenshot and isolated logcat record for every game
        * reject Java, linker, native-signal, ANR, and nonzero runtime exits
        * leave every successfully installed package on the device

    This file intentionally does NOT contain:

        * Android SDK installation
        * gameplay input automation
        * package uninstallation or application-data deletion
        * renderer performance benchmarking
#>

[CmdletBinding()]
param(
    [string]$Adb = 'C:\Nextcloud\Android adb\platform-tools\adb.exe',
    [string]$BuildDirectory = '',
    [string]$Serial = '',
    [int]$InitializationSeconds = 12,
    [switch]$SkipInstall
)

$ErrorActionPreference = 'Stop'

$repositoryRoot = Split-Path -Parent $PSScriptRoot

if ([string]::IsNullOrWhiteSpace($BuildDirectory)) {
    $BuildDirectory = Join-Path $repositoryRoot '.build-gfx2-android\games'
}

$BuildDirectory = [System.IO.Path]::GetFullPath($BuildDirectory)
$summaryPath = Join-Path $BuildDirectory 'oma-gfxlib2-android-build.json'
$testDirectory = Join-Path $BuildDirectory 'device-tests'

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

function Test-PathUnderRoot {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Path,

        [Parameter(Mandatory = $true)]
        [string]$Root
    )

    $separators = [char[]]@('\', '/')
    $fullPath = [System.IO.Path]::GetFullPath($Path).TrimEnd($separators)
    $fullRoot = [System.IO.Path]::GetFullPath($Root).TrimEnd($separators)
    $rootPrefix = $fullRoot + [System.IO.Path]::DirectorySeparatorChar

    return $fullPath.StartsWith(
        $rootPrefix,
        [System.StringComparison]::OrdinalIgnoreCase)
}

function Invoke-Adb {
    param(
        [Parameter(Mandatory = $true)]
        [string[]]$Arguments
    )

    <#
        Windows PowerShell 5 represents native standard-error lines as error
        records. ADB writes successful pull progress to standard error, so the
        script must judge native commands by their exit status rather than by
        PowerShell's ErrorActionPreference.
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

function Write-ProgressSummary {
    param(
        [Parameter(Mandatory = $true)]
        [object[]]$Results
    )

    $jsonPath = Join-Path $testDirectory 'oma-device-test-progress.json'
    $csvPath = Join-Path $testDirectory 'oma-device-test-progress.csv'

    $Results | ConvertTo-Json -Depth 4 |
        Set-Content -LiteralPath $jsonPath -Encoding UTF8
    $Results |
        Export-Csv -LiteralPath $csvPath -NoTypeInformation -Encoding UTF8
}

Require-File -Path $Adb
Require-File -Path $summaryPath

if ($InitializationSeconds -lt 3 -or $InitializationSeconds -gt 120) {
    throw 'InitializationSeconds must be between 3 and 120.'
}

$buildSummary = Get-Content -LiteralPath $summaryPath -Raw |
    ConvertFrom-Json

if (@($buildSummary).Count -ne 14) {
    throw "Expected fourteen build records, found $(@($buildSummary).Count)."
}

foreach ($record in $buildSummary) {
    if (-not $record.Success) {
        throw "Build summary contains a failed package: $($record.Name)"
    }

    Require-File -Path $record.Apk

    $actualHash = (Get-FileHash -LiteralPath $record.Apk `
        -Algorithm SHA256).Hash

    if ($actualHash -ne $record.Sha256) {
        throw "APK hash changed after build verification: $($record.Name)"
    }
}

$deviceLines = @(& $Adb devices) |
    Select-Object -Skip 1 |
    ForEach-Object { $_.Trim() } |
    Where-Object { $_ -match '\sdevice$' }

if ([string]::IsNullOrWhiteSpace($Serial)) {
    if ($deviceLines.Count -ne 1) {
        throw "Expected one connected Android device, found $($deviceLines.Count)."
    }

    $Serial = ($deviceLines[0] -split '\s+')[0]
}

$selectedDevice = @(& $Adb devices) |
    Select-String -Pattern (
        '^' + [regex]::Escape($Serial) + '\s+device$')

if ($selectedDevice.Count -ne 1) {
    throw "Android device is not connected and authorized: $Serial"
}

if (-not (Test-PathUnderRoot -Path $testDirectory -Root $BuildDirectory)) {
    throw "Refusing to replace a test directory outside the build root: $testDirectory"
}

if (Test-Path -LiteralPath $testDirectory) {
    Remove-Item -LiteralPath $testDirectory -Recurse -Force
}

New-Item -ItemType Directory -Path $testDirectory -Force | Out-Null

# ---------------------------------------------------------------------------
# Package installation
# ---------------------------------------------------------------------------

$installResults = [System.Collections.Generic.List[object]]::new()

foreach ($record in $buildSummary) {
    Write-Host "INSTALL $($record.Name) [$($record.Package)]"

    if ($SkipInstall) {
        $install = Invoke-Adb -Arguments @(
            'shell',
            'pm',
            'path',
            $record.Package
        )
    }
    else {
        $install = Invoke-Adb -Arguments @(
            'install',
            '-r',
            $record.Apk
        )
    }
    $install.Output | ForEach-Object {
        Write-Host "  $_"
    }

    if ($SkipInstall) {
        $installed = ($install.ExitCode -eq 0) -and
            ($install.Output -match '^package:')
    }
    else {
        $installed = ($install.ExitCode -eq 0) -and
            ($install.Output -contains 'Success')
    }

    $installLog = Join-Path $testDirectory (
        $record.Name + '-install.log')
    $install.Output |
        Set-Content -LiteralPath $installLog -Encoding UTF8

    $installResults.Add([pscustomobject]@{
        Name = $record.Name
        Package = $record.Package
        Installed = $installed
        ExitCode = $install.ExitCode
        Log = $installLog
    })

    if ($installed) {
        Write-Host "PASS INSTALL $($record.Name)"
    }
    else {
        Write-Host "FAIL INSTALL $($record.Name)"
    }
}

$installResults |
    Export-Csv -LiteralPath (
        Join-Path $testDirectory 'oma-device-install-results.csv') `
        -NoTypeInformation -Encoding UTF8

# ---------------------------------------------------------------------------
# Per-game launch and runtime smoke tests
# ---------------------------------------------------------------------------

$testResults = [System.Collections.Generic.List[object]]::new()
$fatalPattern = (
    'FATAL EXCEPTION|' +
    'Fatal signal|' +
    'ANR in |' +
    'UnsatisfiedLinkError|' +
    'dlopen failed|' +
    'No implementation found|' +
    'FREEBASIC_ANDROID_EXIT:[^0]|' +
    'Abort message:'
)

foreach ($record in $buildSummary) {
    $installRecord = $installResults |
        Where-Object { $_.Name -eq $record.Name } |
        Select-Object -First 1

    if (-not $installRecord.Installed) {
        $testResults.Add([pscustomobject]@{
            Name = $record.Name
            Package = $record.Package
            Installed = $false
            LaunchExitCode = -1
            LaunchStatusOk = $false
            ProcessAlive = $false
            Foreground = $false
            ScreenshotCaptured = $false
            FatalLineCount = -1
            Passed = $false
            Pid = ''
            Screenshot = ''
            Logcat = ''
        })
        Write-ProgressSummary -Results $testResults
        continue
    }

    Write-Host "TEST $($record.Name) [$($record.Package)]"

    $null = Invoke-Adb -Arguments @('logcat', '-c')
    $null = Invoke-Adb -Arguments @(
        'shell',
        'input',
        'keyevent',
        '224'
    )
    $null = Invoke-Adb -Arguments @(
        'shell',
        'wm',
        'dismiss-keyguard'
    )

    $component = (
        $record.Package +
        '/org.freebasic.android.FreeBasicNativeActivity')
    $launch = Invoke-Adb -Arguments @(
        'shell',
        'am',
        'start',
        '-S',
        '-n',
        $component
    )
    $launch.Output | ForEach-Object {
        Write-Host "  $_"
    }

    $launchLog = Join-Path $testDirectory (
        $record.Name + '-launch.log')
    $launch.Output |
        Set-Content -LiteralPath $launchLog -Encoding UTF8

    Start-Sleep -Seconds $InitializationSeconds

    $pidResult = Invoke-Adb -Arguments @(
        'shell',
        'pidof',
        $record.Package
    )
    $gameProcessId = (($pidResult.Output -join ' ').Trim())

    if ([string]::IsNullOrWhiteSpace($gameProcessId)) {
        $processList = Invoke-Adb -Arguments @('shell', 'ps')
        $processLine = $processList.Output |
            Select-String -SimpleMatch $record.Package |
            Select-Object -First 1

        if ($null -ne $processLine) {
            $processFields = $processLine.ToString().Trim() -split '\s+'
            if ($processFields.Count -gt 1) {
                $gameProcessId = $processFields[1]
            }
        }
    }

    $activity = Invoke-Adb -Arguments @(
        'shell',
        'dumpsys',
        'activity',
        'activities'
    )
    $foregroundLine = $activity.Output |
        Select-String -Pattern (
            'mResumedActivity.*' + [regex]::Escape($record.Package)) |
        Select-Object -First 1

    $remoteScreenshot = (
        '/sdcard/oma-test-' + $record.Name + '.png')
    $screenshotPath = Join-Path $testDirectory (
        $record.Name + '.png')
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

    $screenshotCaptured = ($capture.ExitCode -eq 0) -and
        ($pull.ExitCode -eq 0) -and
        (Test-Path -LiteralPath $screenshotPath -PathType Leaf) -and
        ((Get-Item -LiteralPath $screenshotPath).Length -gt 1024)

    $logcat = Invoke-Adb -Arguments @(
        'logcat',
        '-d',
        '-v',
        'threadtime'
    )
    $logcatPath = Join-Path $testDirectory (
        $record.Name + '-logcat.txt')
    $logcat.Output |
        Set-Content -LiteralPath $logcatPath -Encoding UTF8

    $fatalLines = @(
        $logcat.Output |
            Select-String -Pattern $fatalPattern -CaseSensitive:$false
    )
    $launchStatusOk = ($launch.ExitCode -eq 0) -and
        ($launch.Output -match '^(Starting: Intent|Warning: Activity not started)') -and
        -not ($launch.Output -match '^(Error:|Exception)')
    $processAlive = -not [string]::IsNullOrWhiteSpace($gameProcessId)
    $foreground = $null -ne $foregroundLine
    $passed = $launchStatusOk -and
        $processAlive -and
        $foreground -and
        $screenshotCaptured -and
        ($fatalLines.Count -eq 0)

    $testResults.Add([pscustomobject]@{
        Name = $record.Name
        Package = $record.Package
        Installed = $true
        LaunchExitCode = $launch.ExitCode
        LaunchStatusOk = $launchStatusOk
        ProcessAlive = $processAlive
        Foreground = $foreground
        ScreenshotCaptured = $screenshotCaptured
        FatalLineCount = $fatalLines.Count
        Passed = $passed
        Pid = $gameProcessId
        Screenshot = $screenshotPath
        Logcat = $logcatPath
    })

    Write-ProgressSummary -Results $testResults

    if ($passed) {
        Write-Host "PASS TEST $($record.Name): PID $gameProcessId"
    }
    else {
        Write-Host (
            "FAIL TEST $($record.Name): " +
            "launch=$launchStatusOk process=$processAlive " +
            "foreground=$foreground screenshot=$screenshotCaptured " +
            "fatalLines=$($fatalLines.Count)")
    }

    $null = Invoke-Adb -Arguments @(
        'shell',
        'am',
        'force-stop',
        $record.Package
    )
}

$null = Invoke-Adb -Arguments @(
    'shell',
    'input',
    'keyevent',
    '3'
)

# ---------------------------------------------------------------------------
# Final installed-package and result verification
# ---------------------------------------------------------------------------

$packageQuery = Invoke-Adb -Arguments @(
    'shell',
    'pm',
    'list',
    'packages'
)

if ($packageQuery.ExitCode -ne 0) {
    throw 'Could not query the final installed package set.'
}

$missingPackages = @()

foreach ($record in $buildSummary) {
    if ($packageQuery.Output -notcontains ('package:' + $record.Package)) {
        $missingPackages += $record.Package
    }
}

$finalJson = Join-Path $testDirectory 'oma-device-test-results.json'
$finalCsv = Join-Path $testDirectory 'oma-device-test-results.csv'
$finalPackages = Join-Path $testDirectory 'oma-installed-packages.txt'

$testResults | ConvertTo-Json -Depth 4 |
    Set-Content -LiteralPath $finalJson -Encoding UTF8
$testResults |
    Export-Csv -LiteralPath $finalCsv -NoTypeInformation -Encoding UTF8
$packageQuery.Output |
    Where-Object {
        $packageName = $_
        $buildSummary.Package -contains $packageName.Substring(8)
    } |
    Sort-Object |
    Set-Content -LiteralPath $finalPackages -Encoding ASCII

$installPassCount = @(
    $installResults |
        Where-Object { $_.Installed }
).Count
$testPassCount = @(
    $testResults |
        Where-Object { $_.Passed }
).Count

Write-Host "INSTALLED $installPassCount of 14"
Write-Host "TESTED $testPassCount of 14"
Write-Host "MISSING PACKAGES $($missingPackages.Count)"
Write-Host "RESULTS $finalJson"

if ($installPassCount -ne 14 -or
    $testPassCount -ne 14 -or
    $missingPackages.Count -ne 0) {
    throw 'One or more OMA Android install or runtime tests failed.'
}

Write-Host 'All fourteen OMA Android packages installed and passed.'

# end of install-test-oma-android-gfxlib2.ps1

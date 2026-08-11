<#
    Project: FreeBASIC OMA Android builds
    -------------------------------------

    File: build-oma-android-gfxlib2.ps1

    Purpose:

        Build every current OMA game as an Android APK using gfxlib2.

    Responsibilities:

        * keep the current fourteen-game Android build matrix in one place
        * select a validated Android ABI runtime from the packaged toolchain
        * stage only the assets required by OpenHostility and OpenWallStreet
        * preserve the established package names, labels, and game data
        * write a build log, SHA-256 manifest, and machine-readable summary

    This file intentionally does NOT contain:

        * Android SDK or NDK installation
        * device installation or launch automation
        * gfxlib3 selection or renderer benchmarking
#>

[CmdletBinding()]
param(
    [string]$ToolchainRoot = 'C:\freebasic-android',
    [string]$OutputDirectory = '',
    [ValidateSet('android-arm', 'android-aarch64', 'android-x86_64')]
    [string]$Target = 'android-arm',
    [int]$Api = 21,
    [int]$MinimumApi = 21,
    [int]$TargetApi = 35,
    [switch]$KeepGoing
)

$ErrorActionPreference = 'Stop'

$repositoryRoot = Split-Path -Parent $PSScriptRoot

if ([string]::IsNullOrWhiteSpace($OutputDirectory)) {
    $OutputDirectory = Join-Path $repositoryRoot '.build-gfx2-android\games'
}

$OutputDirectory = [System.IO.Path]::GetFullPath($OutputDirectory)

# ---------------------------------------------------------------------------
# Path and process helpers
# ---------------------------------------------------------------------------

function ConvertTo-CygwinPath {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Path
    )

    $fullPath = [System.IO.Path]::GetFullPath($Path)

    if ($fullPath -notmatch '^([A-Za-z]):\\(.*)$') {
        throw "Cannot convert a non-drive path for the Android build shell: $Path"
    }

    $drive = $Matches[1].ToLowerInvariant()
    $tail = $Matches[2].Replace('\', '/')

    return "/cygdrive/$drive/$tail"
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

function Reset-StagingDirectory {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Path
    )

    if (-not (Test-PathUnderRoot -Path $Path -Root $OutputDirectory)) {
        throw "Refusing to replace a staging directory outside the output root: $Path"
    }

    if (Test-Path -LiteralPath $Path) {
        Remove-Item -LiteralPath $Path -Recurse -Force
    }

    New-Item -ItemType Directory -Path $Path -Force | Out-Null
}

function Require-File {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Path
    )

    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        throw "Required build file was not found: $Path"
    }
}

function Require-Directory {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Path
    )

    if (-not (Test-Path -LiteralPath $Path -PathType Container)) {
        throw "Required build directory was not found: $Path"
    }
}

# ---------------------------------------------------------------------------
# Android toolchain selection
# ---------------------------------------------------------------------------

$bash = Join-Path $ToolchainRoot 'toolchain\msys2\usr\bin\bash.exe'
$sdkRoot = Join-Path $ToolchainRoot 'toolchain\android-sdk'
$javaRoot = Join-Path $ToolchainRoot 'toolchain\java'
$wrapper = Join-Path $repositoryRoot 'src\tools\android\fbc-android'

Require-File -Path $bash
Require-File -Path $wrapper
Require-Directory -Path $sdkRoot
Require-Directory -Path $javaRoot

$packageRootCandidates = @(
    $ToolchainRoot,
    (Join-Path $ToolchainRoot 'toolchain\stage\fbc-android'),
    (Join-Path $ToolchainRoot 'toolchain\dist\FreeBASIC-1.20.2-fbc-android')
)

$packageRoot = $packageRootCandidates |
    Where-Object {
        Test-Path -LiteralPath (
            Join-Path $_ "lib\freebasic-android\$Target")
    } |
    Select-Object -First 1

if ([string]::IsNullOrWhiteSpace($packageRoot)) {
    throw "No packaged $Target runtime was found below $ToolchainRoot."
}

$libraryRoot = Join-Path $packageRoot 'lib\freebasic-android'
$compiler = Join-Path $libraryRoot 'bin\fbc-android-compiler.exe'
$includeRoot = Join-Path $packageRoot 'include\freebasic-android'
$shareRoot = Join-Path $packageRoot 'share\freebasic-android'
$ndkRoot = Get-ChildItem -LiteralPath (Join-Path $sdkRoot 'ndk') -Directory |
    Sort-Object Name -Descending |
    Select-Object -First 1 -ExpandProperty FullName

Require-File -Path $compiler
Require-Directory -Path (Join-Path $libraryRoot $Target)
Require-Directory -Path $includeRoot
Require-Directory -Path $shareRoot
Require-Directory -Path $ndkRoot

$env:PATH = (
    (Join-Path $ToolchainRoot 'toolchain\msys2\usr\bin') + ';' +
    (Join-Path $javaRoot 'bin') + ';' +
    (Join-Path $sdkRoot 'platform-tools') + ';' +
    $env:PATH
)
$env:ANDROID_HOME = ConvertTo-CygwinPath -Path $sdkRoot
$env:ANDROID_SDK_ROOT = $env:ANDROID_HOME
$env:ANDROID_NDK_HOME = ConvertTo-CygwinPath -Path $ndkRoot
$env:JAVA_HOME = ConvertTo-CygwinPath -Path $javaRoot
$env:FBANDROID_PREFIX = ConvertTo-CygwinPath -Path $packageRoot
$env:FBANDROID_LIBROOT = ConvertTo-CygwinPath -Path $libraryRoot
$env:FBANDROID_COMPILER = ConvertTo-CygwinPath -Path $compiler
$env:FBANDROID_INCDIR = ConvertTo-CygwinPath -Path $includeRoot
$env:FBANDROID_SHARE = ConvertTo-CygwinPath -Path $shareRoot

# ---------------------------------------------------------------------------
# Curated asset staging
# ---------------------------------------------------------------------------

New-Item -ItemType Directory -Path $OutputDirectory -Force | Out-Null

$stagingRoot = Join-Path $OutputDirectory 'staging'
$openHostilityAssets = Join-Path $stagingRoot 'openhostility-assets'
$openWallStreetAssets = Join-Path $stagingRoot 'openwallstreet-assets'

Reset-StagingDirectory -Path $openHostilityAssets
Reset-StagingDirectory -Path $openWallStreetAssets

$openHostilityRoot = Join-Path $repositoryRoot 'OMA\Scorched Earth'
$openHostilityPatterns = @(
    '*.MTN',
    '*.ppm',
    'OPENHOSTILITY.CFG',
    'OPENHOSTILITY_ATTACK.CFG',
    'OPENHOSTILITY_DEATH.CFG',
    'OPENHOSTILITY.MKT',
    'OPENHOSTILITY.ICO'
)

foreach ($pattern in $openHostilityPatterns) {
    Get-ChildItem -LiteralPath $openHostilityRoot -File -Filter $pattern |
        ForEach-Object {
            Copy-Item -LiteralPath $_.FullName `
                -Destination $openHostilityAssets -Force
        }
}

$openHostilityAssetCount = @(
    Get-ChildItem -LiteralPath $openHostilityAssets -File
).Count

if ($openHostilityAssetCount -eq 0) {
    throw 'No generated OpenHostility assets were staged.'
}

$openWallStreetRoot = Join-Path $repositoryRoot 'OMA\WSR5_3'
$openWallStreetOriginal = Join-Path $openWallStreetRoot 'original_data'
$openWallStreetOriginalStage = Join-Path $openWallStreetAssets 'original_data'
$openWallStreetAssetsList = @(
    'BADNEWS.DAT',
    'MISCNEWS.DAT',
    'SCENTXT1.DAT',
    'SCENTXT2.DAT',
    'SCENTXT3.DAT',
    'GAMEHIGH.DAT',
    'NEWGAME1.DAT',
    'NEWGAME2.DAT',
    'FILE_ID.DIZ',
    'VENDOR.DOC',
    'VENDOR.TXT',
    'REGISTER.DOC',
    'USERINFO.DOC'
)

New-Item -ItemType Directory -Path $openWallStreetOriginalStage -Force |
    Out-Null

foreach ($assetName in $openWallStreetAssetsList) {
    $sourceAsset = Join-Path $openWallStreetOriginal $assetName
    Require-File -Path $sourceAsset
    Copy-Item -LiteralPath $sourceAsset `
        -Destination $openWallStreetOriginalStage -Force
}

$openWallStreetExtracted = Join-Path $openWallStreetOriginal `
    'extracted\strings\strings_SUBSID1.EXE.txt'
$openWallStreetExtractedStage = Join-Path $openWallStreetOriginalStage `
    'extracted\strings'

Require-File -Path $openWallStreetExtracted
New-Item -ItemType Directory -Path $openWallStreetExtractedStage -Force |
    Out-Null
Copy-Item -LiteralPath $openWallStreetExtracted `
    -Destination $openWallStreetExtractedStage -Force

# ---------------------------------------------------------------------------
# Current OMA Android build matrix
# ---------------------------------------------------------------------------

$turboTrekRoot = Join-Path $repositoryRoot 'OMA\TurboTrek'
$turboTrekSource = Join-Path $turboTrekRoot 'src\turbotrek'
$turboTrekModules = @(
    'clone_scenario_file.bas',
    'clone_scenario_import.bas',
    'clone_scenario_runtime.bas',
    'clone_scenario.bas',
    'clone_session_file.bas',
    'damage.bas',
    'deflectors.bas',
    'device_condition.bas',
    'disruptor.bas',
    'drone_projectile.bas',
    'residual_damage.bas',
    'residual_damage_resolution.bas',
    'original_random.bas',
    'photon.bas',
    'plasma_projectile.bas',
    'pulser_redirect.bas',
    'critical_damage.bas',
    'energy.bas',
    'fire_control.bas',
    'game_state.bas',
    'terrain_damage.bas',
    'movement.bas',
    'operations.bas',
    'operations_display.bas',
    'repair.bas',
    'robot_ai.bas',
    'scenario_catalog.bas',
    'scenario_metadata.bas',
    'scenario_objects.bas',
    'scenario_world.bas',
    'self_destruct.bas',
    'sensors.bas',
    'ship_definition.bas',
    'simulation.bas',
    'weapon_targeting.bas',
    'world.bas'
) | ForEach-Object {
    Join-Path $turboTrekSource $_
}

$matrix = @(
    [pscustomobject]@{
        Name = 'arkanoid-test'
        Label = 'Arkanoid Test'
        Package = 'org.freebasic.oma.arkanoidtest'
        WorkingDirectory = Join-Path $repositoryRoot 'OMA\ArkanoidTest'
        Sources = @(
            Join-Path $repositoryRoot 'OMA\ArkanoidTest\ArkanoidTest.bas'
        )
        Includes = @()
        Assets = ''
        Arguments = @()
    },
    [pscustomobject]@{
        Name = 'behold'
        Label = 'Behold'
        Package = 'org.freebasic.oma.behold'
        WorkingDirectory = Join-Path $repositoryRoot 'OMA\Behold'
        Sources = @(
            Join-Path $repositoryRoot 'OMA\Behold\Behold.bas'
        )
        Includes = @()
        Assets = ''
        Arguments = @()
    },
    [pscustomobject]@{
        Name = 'demolitionderby'
        Label = 'Demolition Derby'
        Package = 'org.freebasic.oma.demolitionderby'
        WorkingDirectory = Join-Path $repositoryRoot 'OMA\DemolitionDerby'
        Sources = @(
            Join-Path $repositoryRoot 'OMA\DemolitionDerby\main.bas'
        )
        Includes = @()
        Assets = ''
        Arguments = @()
    },
    [pscustomobject]@{
        Name = 'duel999'
        Label = 'Duel 999'
        Package = 'org.freebasic.oma.duel999'
        WorkingDirectory = Join-Path $repositoryRoot 'OMA\duel999'
        Sources = @(
            Join-Path $repositoryRoot 'OMA\duel999\SD_Main.bas'
        )
        Includes = @()
        Assets = Join-Path $repositoryRoot `
            'OMA\android-output\stage-duel999'
        Arguments = @()
    },
    [pscustomobject]@{
        Name = 'kinematics-self'
        Label = 'Kinematics Self'
        Package = 'org.freebasic.oma.kinematics.self'
        WorkingDirectory = Join-Path $repositoryRoot 'OMA\kinematics'
        Sources = @(
            Join-Path $repositoryRoot `
                'OMA\kinematics\kinematic_man_two_bodies_self_collision_friction.bas'
        )
        Includes = @()
        Assets = ''
        Arguments = @()
    },
    [pscustomobject]@{
        Name = 'nietzsche'
        Label = 'Nietzsche SE'
        Package = 'org.freebasic.oma.nietzsche'
        WorkingDirectory = Join-Path $repositoryRoot `
            'OMA\NietzscheSE-MSDOS-1.1\Nietzsche'
        Sources = @(
            Join-Path $repositoryRoot `
                'OMA\NietzscheSE-MSDOS-1.1\Nietzsche\src\win32\win11.bas'
        )
        Includes = @()
        Assets = Join-Path $repositoryRoot `
            'OMA\android-output\stage-nietzsche'
        Arguments = @()
    },
    [pscustomobject]@{
        Name = 'qfak'
        Label = 'Quest for a King'
        Package = 'org.freebasic.oma.qfak'
        WorkingDirectory = Join-Path $repositoryRoot `
            'OMA\QuestForAKing-Win32-1.5'
        Sources = @(
            Join-Path $repositoryRoot `
                'OMA\QuestForAKing-Win32-1.5\src\win11.bas'
        )
        Includes = @()
        Assets = Join-Path $repositoryRoot `
            'OMA\android-output\stage-qfak'
        Arguments = @()
    },
    [pscustomobject]@{
        Name = 'rambo'
        Label = 'Rambo vs Kitty Cat'
        Package = 'org.freebasic.oma.rambo'
        WorkingDirectory = Join-Path $repositoryRoot `
            'OMA\RamboVsKittyCat-Win32-0.1'
        Sources = @(
            Join-Path $repositoryRoot `
                'OMA\RamboVsKittyCat-Win32-0.1\killquest.bas'
        )
        Includes = @()
        Assets = Join-Path $repositoryRoot `
            'OMA\android-output\stage-rambo'
        Arguments = @()
    },
    [pscustomobject]@{
        Name = 'starphalanx'
        Label = 'Star Phalanx'
        Package = 'org.freebasic.oma.starphalanx'
        WorkingDirectory = Join-Path $repositoryRoot `
            'OMA\StarPhalanx-win32-0.5'
        Sources = @(
            Join-Path $repositoryRoot `
                'OMA\StarPhalanx-win32-0.5\entryv2.bas'
        )
        Includes = @()
        Assets = Join-Path $repositoryRoot `
            'OMA\android-output\stage-starphalanx'
        Arguments = @()
    },
    [pscustomobject]@{
        Name = 'openmarket'
        Label = 'Open Market'
        Package = 'org.freebasic.oma.openmarket'
        WorkingDirectory = Join-Path $repositoryRoot 'OMA\Tamper\tamper'
        Sources = @(
            Join-Path $repositoryRoot `
                'OMA\Tamper\tamper\src\openmarket_bootstrap.bas'
        )
        Includes = @()
        Assets = Join-Path $repositoryRoot `
            'OMA\android-output\stage-openmarket'
        Arguments = @()
    },
    [pscustomobject]@{
        Name = 'openhostility'
        Label = 'OpenHostility'
        Package = 'org.openhostility.game'
        WorkingDirectory = $openHostilityRoot
        Sources = @(
            (Join-Path $openHostilityRoot 'src\scorch_gfx.bas'),
            (Join-Path $openHostilityRoot 'src\scorch_platform.bas'),
            (Join-Path $openHostilityRoot 'src\scorch_engine.bas'),
            (Join-Path $openHostilityRoot 'src\scorch_io.bas'),
            (Join-Path $openHostilityRoot 'src\scorch_mtn.bas')
        )
        Includes = @(
            (Join-Path $openHostilityRoot 'src'),
            (Join-Path $openHostilityRoot 'src\omaGUI')
        )
        Assets = $openHostilityAssets
        Arguments = @(
            '-exx',
            '-w', 'all',
            '-d', 'OPENHOSTILITY_TARGET_ANDROID',
            '-d', 'OPENHOSTILITY_TOUCH_UI',
            '--landscape',
            '--hideKeyboardButton'
        )
    },
    [pscustomobject]@{
        Name = 'turbotrek'
        Label = 'TurboTrek'
        Package = 'org.freebasic.oma.turbotrek'
        WorkingDirectory = $turboTrekRoot
        Sources = @(
            (Join-Path $turboTrekSource 'main.bas')
        ) + $turboTrekModules
        Includes = @(
            (Join-Path $turboTrekRoot 'src\omaGUI'),
            $turboTrekSource
        )
        Assets = ''
        Arguments = @(
            '--landscape',
            '--hideKeyboardButton'
        )
    },
    [pscustomobject]@{
        Name = 'vtrek'
        Label = 'vtrek'
        Package = 'org.freebasic.oma.vtrek'
        WorkingDirectory = Join-Path $repositoryRoot 'OMA\vtrek'
        Sources = @(
            Join-Path $repositoryRoot 'OMA\vtrek\src\vtrek.bas'
        )
        Includes = @()
        Assets = ''
        Arguments = @()
    },
    [pscustomobject]@{
        Name = 'openwallstreet'
        Label = 'OpenWallStreet'
        Package = 'net.fbxl.openwallstreet'
        WorkingDirectory = $openWallStreetRoot
        Sources = @(
            Join-Path $openWallStreetRoot 'src\raider.bas'
        )
        Includes = @(
            (Join-Path $openWallStreetRoot 'src'),
            (Join-Path $openWallStreetRoot 'src\omaGUI')
        )
        Assets = $openWallStreetAssets
        Arguments = @(
            '--landscape',
            '--hideKeyboardButton'
        )
    }
)

if ($matrix.Count -ne 14) {
    throw "Expected fourteen OMA games, found $($matrix.Count)."
}

# ---------------------------------------------------------------------------
# Compilation
# ---------------------------------------------------------------------------

$results = [System.Collections.Generic.List[object]]::new()
$failureCount = 0
$wrapperPath = ConvertTo-CygwinPath -Path $wrapper

foreach ($game in $matrix) {
    Write-Host "BUILD $($game.Name) [$Target, gfxlib2]"

    Require-Directory -Path $game.WorkingDirectory

    foreach ($source in $game.Sources) {
        Require-File -Path $source
    }

    foreach ($include in $game.Includes) {
        Require-Directory -Path $include
    }

    if (-not [string]::IsNullOrWhiteSpace($game.Assets)) {
        Require-Directory -Path $game.Assets
    }

    $apkPath = Join-Path $OutputDirectory ($game.Name + '.apk')
    $logPath = Join-Path $OutputDirectory ($game.Name + '.log')
    $arguments = @(
        $wrapperPath,
        '--target', $Target,
        '--api', [string]$Api,
        '--min-api', [string]$MinimumApi,
        '--target-api', [string]$TargetApi,
        '--package', $game.Package,
        '--label', $game.Label,
        '-x', (ConvertTo-CygwinPath -Path $apkPath),
        '-d', 'GFX2_REFERENCE'
    )

    if (-not [string]::IsNullOrWhiteSpace($game.Assets)) {
        $arguments += @(
            '--assets',
            (ConvertTo-CygwinPath -Path $game.Assets)
        )
    }

    foreach ($include in $game.Includes) {
        $arguments += @('-i', (ConvertTo-CygwinPath -Path $include))
    }

    $arguments += $game.Arguments

    foreach ($source in $game.Sources) {
        $arguments += ConvertTo-CygwinPath -Path $source
    }

    $started = Get-Date
    $buildOutput = @()
    $exitCode = -1

    Push-Location $game.WorkingDirectory
    try {
        $buildOutput = @(& $bash @arguments 2>&1)
        $exitCode = $LASTEXITCODE
    }
    finally {
        Pop-Location
    }

    $buildOutput | Set-Content -LiteralPath $logPath -Encoding UTF8
    $buildOutput | ForEach-Object {
        Write-Host "  $_"
    }

    $success = ($exitCode -eq 0) -and
        (Test-Path -LiteralPath $apkPath -PathType Leaf)
    $hash = ''
    $length = 0

    if ($success) {
        $apk = Get-Item -LiteralPath $apkPath
        $hash = (Get-FileHash -LiteralPath $apkPath -Algorithm SHA256).Hash
        $length = $apk.Length
        Write-Host "PASS $($game.Name): $length bytes, SHA-256 $hash"
    }
    else {
        $failureCount++
        Write-Host "FAIL $($game.Name): compiler exit $exitCode"
    }

    $results.Add([pscustomobject]@{
        Name = $game.Name
        Label = $game.Label
        Package = $game.Package
        Target = $Target
        Runtime = 'gfxlib2'
        Success = $success
        ExitCode = $exitCode
        Bytes = $length
        Sha256 = $hash
        Apk = $apkPath
        Log = $logPath
        Seconds = [Math]::Round(((Get-Date) - $started).TotalSeconds, 3)
    })

    if (-not $success -and -not $KeepGoing) {
        break
    }
}

# ---------------------------------------------------------------------------
# Build summary
# ---------------------------------------------------------------------------

$summaryJson = Join-Path $OutputDirectory 'oma-gfxlib2-android-build.json'
$summaryCsv = Join-Path $OutputDirectory 'oma-gfxlib2-android-build.csv'
$hashManifest = Join-Path $OutputDirectory 'SHA256SUMS.txt'

$results | ConvertTo-Json -Depth 4 |
    Set-Content -LiteralPath $summaryJson -Encoding UTF8
$results | Export-Csv -LiteralPath $summaryCsv -NoTypeInformation -Encoding UTF8
$results |
    Where-Object { $_.Success } |
    ForEach-Object {
        '{0} *{1}.apk' -f $_.Sha256, $_.Name
    } |
    Set-Content -LiteralPath $hashManifest -Encoding ASCII

Write-Host "SUMMARY $summaryJson"
Write-Host "BUILT $(@($results | Where-Object { $_.Success }).Count) of 14"

if ($failureCount -ne 0 -or $results.Count -ne 14) {
    throw "OMA Android gfxlib2 build failed for $failureCount game(s)."
}

Write-Host 'All fourteen OMA Android gfxlib2 packages built successfully.'

# end of build-oma-android-gfxlib2.ps1

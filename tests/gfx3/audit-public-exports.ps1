<#
    Project: FreeBASIC gfxlib3 tests
    --------------------------------

    File: audit-public-exports.ps1

    Purpose:

        Compare the built gfxlib3 static archive against the public graphics
        ABI declared by gfxlib2's fb_gfx.h on Windows.

    Responsibilities:

        - extract public fb_Gfx* and fb_hPut* declarations from fb_gfx.h

        - extract the runtime graphics hooks that gfxlib2 installs by pointer
        - require every public fbgfx3.bi extension symbol from gfxlib3
        - require both Win64 graphics archives to define every declaration
        - report missing symbols with the archive and tool paths used

    This file intentionally does NOT contain:

        - source-level implementation comparisons
        - private gfxlib2 helper requirements
        - ABI checks for a target that has not built both archives
#>

[CmdletBinding()]
param(
    [string]$Nm,
    [string]$Architecture = 'win64',
    [string]$Gfx2Archive,
    [string]$Gfx3Archive
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Get-ArchiveSymbols {
    param(
        [Parameter(Mandatory = $true)]
        [string]$ArchivePath,
        [Parameter(Mandatory = $true)]
        [string]$NmPath
    )

    $symbols = & $NmPath -g --defined-only $ArchivePath
    if ($LASTEXITCODE -ne 0) {
        throw "nm failed for $ArchivePath"
    }

    return @(
        $symbols |
            ForEach-Object {
                if ($_ -match '\b([A-Za-z_][A-Za-z0-9_]+)$') {
                    $Matches[1]
                }
            } |
            Sort-Object -Unique
    )
}

$repositoryRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$headerPath = Join-Path $repositoryRoot 'src\gfxlib2\fb_gfx.h'
$extensionHeaderPath = Join-Path $repositoryRoot 'inc\fbgfx3.bi'
$libraryDirectory = Join-Path $repositoryRoot "lib\freebasic\$Architecture"
if ([string]::IsNullOrWhiteSpace($Gfx2Archive)) {
    $Gfx2Archive = Join-Path $libraryDirectory 'libfbgfx.a'
}
if ([string]::IsNullOrWhiteSpace($Gfx3Archive)) {
    $Gfx3Archive = Join-Path $libraryDirectory 'libfbgfx3.a'
}
$gfx2Archive = (Resolve-Path -LiteralPath $Gfx2Archive).Path
$gfx3Archive = (Resolve-Path -LiteralPath $Gfx3Archive).Path

foreach ($requiredPath in @($headerPath, $extensionHeaderPath, $gfx2Archive, $gfx3Archive)) {
    if (-not (Test-Path -LiteralPath $requiredPath -PathType Leaf)) {
        throw "Required file was not found: $requiredPath"
    }
}

if ([string]::IsNullOrWhiteSpace($Nm)) {
    $nmCommand = Get-Command nm.exe, nm -ErrorAction SilentlyContinue |
        Select-Object -First 1
    if ($null -eq $nmCommand) {
        throw 'nm.exe or nm was not found on PATH. Supply -Nm with a GNU or LLVM nm path.'
    }
    $Nm = $nmCommand.Source
}
if (-not (Test-Path -LiteralPath $Nm -PathType Leaf)) {
    throw "nm tool was not found: $Nm"
}

$headerText = Get-Content -LiteralPath $headerPath -Raw
$publicSymbols = @(
    [regex]::Matches($headerText,
        'FBCALL[\s\S]{0,256}?\b(fb_(?:Gfx|hPut)[A-Za-z0-9_]+)\s*\(') |
        ForEach-Object { $_.Groups[1].Value } |
        Sort-Object -Unique
)
if ($publicSymbols.Count -lt 59) {
    throw "Only $($publicSymbols.Count) public graphics symbols were parsed from $headerPath"
}

# These declarations form the runtime hook table installed by gfxlib2 after a
# graphics mode opens. They are intentionally not FBCALL entries, yet a
# gfxlib3 archive must export them because rtlib holds their function pointers.
$hookStart = $headerText.IndexOf('int fb_GfxGetkey(void);')
$hookEnd = $headerText.IndexOf('FBCALL void fb_GfxImageConvertRow')
if (($hookStart -lt 0) -or ($hookEnd -le $hookStart)) {
    throw "The runtime graphics-hook block could not be found in $headerPath"
}
$hookText = $headerText.Substring($hookStart, $hookEnd - $hookStart)
$hookSymbols = @(
    [regex]::Matches($hookText, '\b(fb_Gfx[A-Za-z0-9_]+)\s*\(') |
        ForEach-Object { $_.Groups[1].Value } |
        Sort-Object -Unique
)
if ($hookSymbols.Count -lt 29) {
    throw "Only $($hookSymbols.Count) runtime graphics hooks were parsed from $headerPath"
}
$expectedSymbols = @($publicSymbols + $hookSymbols | Sort-Object -Unique)
$extensionHeaderText = Get-Content -LiteralPath $extensionHeaderPath -Raw
$extensionSymbols = @(
    [regex]::Matches($extensionHeaderText,
        'alias\s+"(fb_Gfx3[A-Za-z0-9_]+)"') |
        ForEach-Object { $_.Groups[1].Value } |
        Sort-Object -Unique
)
if ($extensionSymbols.Count -lt 16) {
    throw "Only $($extensionSymbols.Count) public gfxlib3 extension symbols were parsed from $extensionHeaderPath"
}

$gfx2Symbols = Get-ArchiveSymbols -ArchivePath $gfx2Archive -NmPath $Nm
$gfx3Symbols = Get-ArchiveSymbols -ArchivePath $gfx3Archive -NmPath $Nm
$missingFromGfx2 = @($expectedSymbols | Where-Object { $_ -notin $gfx2Symbols })
$missingFromGfx3 = @($expectedSymbols | Where-Object { $_ -notin $gfx3Symbols })
$missingExtensionFromGfx3 = @($extensionSymbols | Where-Object { $_ -notin $gfx3Symbols })

Write-Output "Public graphics declarations: $($publicSymbols.Count)"
Write-Output "Runtime graphics hooks: $($hookSymbols.Count)"
Write-Output "Required graphics archive symbols: $($expectedSymbols.Count)"
Write-Output "Public gfxlib3 extension symbols: $($extensionSymbols.Count)"
Write-Output "gfxlib2 archive: $gfx2Archive"
Write-Output "gfxlib3 archive: $gfx3Archive"
Write-Output "nm: $Nm"

if ($missingFromGfx2.Count -ne 0) {
    Write-Error "gfxlib2 is missing declared public graphics symbols: $($missingFromGfx2 -join ', ')"
}
if ($missingFromGfx3.Count -ne 0) {
    Write-Error "gfxlib3 is missing declared public graphics symbols: $($missingFromGfx3 -join ', ')"
}
if ($missingExtensionFromGfx3.Count -ne 0) {
    Write-Error "gfxlib3 is missing declared extension symbols: $($missingExtensionFromGfx3 -join ', ')"
}
if (($missingFromGfx2.Count -ne 0) -or ($missingFromGfx3.Count -ne 0) -or
    ($missingExtensionFromGfx3.Count -ne 0)) {
    exit 1
}

Write-Output 'GFX3_PUBLIC_EXPORT_AUDIT_PASS'
exit 0

# end of audit-public-exports.ps1

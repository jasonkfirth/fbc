<#
    Project: FreeBASIC gfxlib3 tests
    --------------------------------

    File: check-gles-transform-shaders.ps1

    Purpose:

        Validate the embedded OpenGL ES transform shaders without requiring a
        connected Android device.

    Responsibilities:

        - extract named C string literals from the GLES backend source
        - validate and link the single-transform shader pair
        - validate and link the instanced transform-batch shader pair

    This file intentionally does NOT contain:

        - an EGL context or GPU execution test
        - shader source duplicated from the backend
        - pixel-result assertions
#>

[CmdletBinding()]
param(
    [string]$Validator
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Get-EmbeddedShader {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Source,
        [Parameter(Mandatory = $true)]
        [string]$Name
    )

    $escapedName = [regex]::Escape($Name)
    $pattern = 'static\s+const\s+char\s+' + $escapedName +
        '\[\]\s*=\s*(?<body>(?:"(?:\\.|[^"\\])*"\s*)+);'
    $definition = [regex]::Match($Source, $pattern,
        [System.Text.RegularExpressions.RegexOptions]::Singleline)
    if (-not $definition.Success) {
        throw "Embedded shader was not found: $Name"
    }

    $builder = [System.Text.StringBuilder]::new()
    $pieces = [regex]::Matches($definition.Groups['body'].Value,
        '"(?<text>(?:\\.|[^"\\])*)"')
    foreach ($piece in $pieces) {
        [void]$builder.Append([regex]::Unescape(
            $piece.Groups['text'].Value))
    }
    return $builder.ToString()
}

function Test-ShaderPair {
    param(
        [Parameter(Mandatory = $true)]
        [string]$PairName,
        [Parameter(Mandatory = $true)]
        [string]$VertexSource,
        [Parameter(Mandatory = $true)]
        [string]$FragmentSource,
        [Parameter(Mandatory = $true)]
        [string]$Directory,
        [Parameter(Mandatory = $true)]
        [string]$ValidatorPath
    )

    $vertexPath = Join-Path $Directory "$PairName.vert"
    $fragmentPath = Join-Path $Directory "$PairName.frag"
    [System.IO.File]::WriteAllText($vertexPath, $VertexSource,
        [System.Text.UTF8Encoding]::new($false))
    [System.IO.File]::WriteAllText($fragmentPath, $FragmentSource,
        [System.Text.UTF8Encoding]::new($false))
    & $ValidatorPath -l $vertexPath $fragmentPath
    if ($LASTEXITCODE -ne 0) {
        throw "OpenGL ES shader validation failed: $PairName"
    }
}

$repositoryRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$backendPath = Join-Path $repositoryRoot 'src\gfxlib3\android\gfx3_backend_gles.c'
$backendSource = Get-Content -LiteralPath $backendPath -Raw
if ([string]::IsNullOrWhiteSpace($Validator)) {
    $command = Get-Command glslangValidator.exe, glslangValidator -ErrorAction SilentlyContinue |
        Select-Object -First 1
    if ($null -eq $command) {
        throw 'glslangValidator was not found. Supply -Validator explicitly.'
    }
    $Validator = $command.Source
}
$validatorPath = (Resolve-Path -LiteralPath $Validator).Path
$temporaryRoot = [System.IO.Path]::GetTempPath()
$temporaryDirectory = Join-Path $temporaryRoot ("gfx3-gles-transform-{0}" -f
    [guid]::NewGuid().ToString('N'))
[void][System.IO.Directory]::CreateDirectory($temporaryDirectory)

try {
    $commonVertex = Get-EmbeddedShader -Source $backendSource -Name 'gles_vertex_shader'
    $singleFragment = Get-EmbeddedShader -Source $backendSource -Name 'gles_transform_blit_fragment_shader'
    $singleArguments = @{
        PairName = 'single-transform'
        VertexSource = $commonVertex
        FragmentSource = $singleFragment
        Directory = $temporaryDirectory
        ValidatorPath = $validatorPath
    }
    Test-ShaderPair @singleArguments

    $batchVertex = Get-EmbeddedShader -Source $backendSource -Name 'gles_transform_blit_batch_vertex_shader'
    $batchFragment = Get-EmbeddedShader -Source $backendSource -Name 'gles_transform_blit_batch_fragment_shader'
    $batchArguments = @{
        PairName = 'batch-transform'
        VertexSource = $batchVertex
        FragmentSource = $batchFragment
        Directory = $temporaryDirectory
        ValidatorPath = $validatorPath
    }
    Test-ShaderPair @batchArguments
} finally {
    $resolvedTemporary = [System.IO.Path]::GetFullPath($temporaryDirectory)
    $resolvedRoot = [System.IO.Path]::GetFullPath($temporaryRoot)
    if ($resolvedTemporary.StartsWith($resolvedRoot,
        [System.StringComparison]::OrdinalIgnoreCase)) {
        Remove-Item -LiteralPath $resolvedTemporary -Recurse -Force -ErrorAction SilentlyContinue
    }
}

Write-Output 'GFX3_GLES_TRANSFORM_SHADERS_PASS'
exit 0

# end of check-gles-transform-shaders.ps1

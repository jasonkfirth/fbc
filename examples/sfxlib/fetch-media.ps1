$ErrorActionPreference = "Stop"

$dest = Join-Path $PSScriptRoot "media"
$headers = @{
    "User-Agent" = "FreeBASIC-sfxlib-example-refresh/1.0"
}

$files = @(
    @{ Title = "File:Buzzer.wav"; Target = "buzzer.wav" },
    @{ Title = "File:Good Morning to All.ogg"; Target = "good-morning-to-all.ogg" },
    @{ Title = "File:Harmonized scale.mid"; Target = "harmonized-scale.mid" },
    @{ Title = "File:Scotcampbell - clown laugh (cc0) (freesound).mp3"; Target = "clown-laugh.mp3" }
)

New-Item -ItemType Directory -Force -Path $dest | Out-Null

foreach( $file in $files )
{
    $title = [uri]::EscapeDataString( $file.Title )
    $api = "https://commons.wikimedia.org/w/api.php?action=query&titles=$title&prop=imageinfo&iiprop=url&format=json"

    $json = Invoke-WebRequest -Headers $headers -Uri $api -UseBasicParsing
    $data = $json.Content | ConvertFrom-Json
    $page = $data.query.pages.PSObject.Properties | Select-Object -First 1
    $url = $page.Value.imageinfo[0].url

    if( -not $url )
    {
        throw "No download URL returned for $($file.Title)"
    }

    Invoke-WebRequest -Headers $headers -Uri $url -OutFile ( Join-Path $dest $file.Target )
    Start-Sleep -Seconds 2
}

Get-ChildItem -LiteralPath $dest | Select-Object Name, Length

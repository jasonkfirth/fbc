<#
    Project: FreeBASIC NuttX/RP2350-PiZero program upload
    ------------------------------------------------------

    File: nuttx-rp2350-upload-ymodem.ps1

    Purpose:

        Copy one built NuttX program module to the RP2350-PiZero SD card
        through the USB CDC console.

    Responsibilities:

        - start the NuttX YMODEM receiver on the board
        - send one local file using YMODEM/CRC packets
        - place the received file under /mnt/sd0/bin by default
        - optionally mark the received program executable

    This file intentionally does NOT contain:

        - a FreeBASIC compiler invocation
        - UF2 flashing logic
        - terminal emulator behavior
        - support for batch/multi-file transfers
#>

[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$File,

    [string]$Port = "COM5",
    [int]$Baud = 115200,
    [string]$RemoteDir = "/mnt/sd0/bin",
    [string]$RemoteName = "",
    [string]$RemoteMode = "",
    [int]$TimeoutSec = 30,
    [int]$InterCommandDelayMs = 350,
    [int]$CommandCharDelayMs = 30
)

$ErrorActionPreference = "Stop"

if (-not (Test-Path -LiteralPath $File)) {
    throw "input file does not exist: $File"
}

$sourceItem = Get-Item -LiteralPath $File
if ($sourceItem.Length -lt 0) {
    throw "invalid input file length: $File"
}

if ($RemoteName -eq "") {
    $RemoteName = $sourceItem.Name
}

if ($RemoteName.Contains("/") -or $RemoteName.Contains("\")) {
    throw "remote name must be a plain file name, not a path: $RemoteName"
}

if ($RemoteName.Length -gt 100) {
    throw "remote name is too long for this YMODEM sender: $RemoteName"
}

if ($RemoteMode -ne "" -and $RemoteMode -notmatch "^[0-7]{3}$") {
    throw "remote mode must be a three-digit octal mode: $RemoteMode"
}

$SOH = [byte]0x01
$STX = [byte]0x02
$EOT = [byte]0x04
$ACK = [byte]0x06
$CRC = [byte]0x43
$Packet128 = 128
$Packet1K = 1024

function Get-Crc16Ccitt {
    param([byte[]]$Data)

    [int]$crc = 0

    foreach ($byte in $Data) {
        $crc = $crc -bxor ([int]$byte -shl 8)

        for ($bit = 0; $bit -lt 8; $bit++) {
            if (($crc -band 0x8000) -ne 0) {
                $crc = (($crc -shl 1) -bxor 0x1021) -band 0xffff
            } else {
                $crc = ($crc -shl 1) -band 0xffff
            }
        }
    }

    return $crc
}

function New-PaddedBlock {
    param(
        [byte[]]$Bytes,
        [int]$Size
    )

    if ($Bytes.Length -gt $Size) {
        throw "block is larger than packet size"
    }

    $block = New-Object byte[] $Size
    if ($Bytes.Length -gt 0) {
        [Array]::Copy($Bytes, 0, $block, 0, $Bytes.Length)
    }

    return $block
}

function Write-SerialBytes {
    param(
        [System.IO.Ports.SerialPort]$Serial,
        [byte[]]$Bytes
    )

    $Serial.Write($Bytes, 0, $Bytes.Length)
}

function Write-SerialText {
    param(
        [System.IO.Ports.SerialPort]$Serial,
        [string]$Text,
        [int]$CharDelayMs = 0
    )

    if ($CharDelayMs -le 0) {
        $bytes = [System.Text.Encoding]::ASCII.GetBytes($Text)
        Write-SerialBytes -Serial $Serial -Bytes $bytes
        return
    }

    foreach ($ch in $Text.ToCharArray()) {
        $bytes = [System.Text.Encoding]::ASCII.GetBytes([string]$ch)
        Write-SerialBytes -Serial $Serial -Bytes $bytes
        Start-Sleep -Milliseconds $CharDelayMs
    }
}

function Wait-SerialByte {
    param(
        [System.IO.Ports.SerialPort]$Serial,
        [byte]$Expected,
        [int]$TimeoutSeconds
    )

    $deadline = (Get-Date).AddSeconds($TimeoutSeconds)

    while ((Get-Date) -lt $deadline) {
        try {
            $value = $Serial.ReadByte()
            if ($value -eq [int]$Expected) {
                return
            }
        } catch [TimeoutException] {
        }
    }

    throw ("timed out waiting for byte 0x{0:x2}" -f $Expected)
}

function Send-YModemPacket {
    param(
        [System.IO.Ports.SerialPort]$Serial,
        [byte]$Header,
        [byte]$Sequence,
        [byte[]]$Data
    )

    $crcValue = Get-Crc16Ccitt -Data $Data
    $packet = New-Object byte[] (3 + $Data.Length + 2)

    $packet[0] = $Header
    $packet[1] = $Sequence
    $packet[2] = [byte]($Sequence -bxor 0xff)
    [Array]::Copy($Data, 0, $packet, 3, $Data.Length)
    $packet[$packet.Length - 2] = [byte](($crcValue -shr 8) -band 0xff)
    $packet[$packet.Length - 1] = [byte]($crcValue -band 0xff)

    Write-SerialBytes -Serial $Serial -Bytes $packet
}

$fileBytes = [System.IO.File]::ReadAllBytes($sourceItem.FullName)
$serial = [System.IO.Ports.SerialPort]::new(
    $Port,
    $Baud,
    [System.IO.Ports.Parity]::None,
    8,
    [System.IO.Ports.StopBits]::One
)

$serial.ReadTimeout = 200
$serial.WriteTimeout = 5000
$serial.DtrEnable = $true
$serial.RtsEnable = $true

try {
    $serial.Open()

    $serial.DtrEnable = $false
    $serial.RtsEnable = $false
    Start-Sleep -Milliseconds 300
    $serial.DtrEnable = $true
    $serial.RtsEnable = $true
    Start-Sleep -Milliseconds 700

    $serial.DiscardInBuffer()
    $serial.DiscardOutBuffer()

    Write-SerialText -Serial $serial -Text "`r" -CharDelayMs $CommandCharDelayMs
    Start-Sleep -Milliseconds $InterCommandDelayMs
    $serial.DiscardInBuffer()

    Write-SerialText -Serial $serial -Text "mkdir $RemoteDir`r" -CharDelayMs $CommandCharDelayMs
    Start-Sleep -Milliseconds $InterCommandDelayMs
    $serial.DiscardInBuffer()

    Write-Host "Starting receiver on ${Port}: rb -f $RemoteDir"
    Write-SerialText -Serial $serial -Text "rb -f $RemoteDir`r" -CharDelayMs $CommandCharDelayMs

    Wait-SerialByte -Serial $serial -Expected $CRC -TimeoutSeconds $TimeoutSec

    $headerText = "$RemoteName`0$($fileBytes.Length)`0"
    $headerBlock = New-PaddedBlock `
        -Bytes ([System.Text.Encoding]::ASCII.GetBytes($headerText)) `
        -Size $Packet128

    Send-YModemPacket -Serial $serial -Header $SOH -Sequence 0 -Data $headerBlock
    Wait-SerialByte -Serial $serial -Expected $ACK -TimeoutSeconds $TimeoutSec
    Wait-SerialByte -Serial $serial -Expected $CRC -TimeoutSeconds $TimeoutSec

    [byte]$sequence = 1
    $offset = 0

    while ($offset -lt $fileBytes.Length) {
        $remaining = $fileBytes.Length - $offset

        if ($remaining -le $Packet128) {
            $packetSize = $Packet128
            $packetHeader = $SOH
        } else {
            $packetSize = $Packet1K
            $packetHeader = $STX
        }

        $chunkLength = [Math]::Min($remaining, $packetSize)
        $chunk = New-Object byte[] $chunkLength
        [Array]::Copy($fileBytes, $offset, $chunk, 0, $chunkLength)
        $block = New-PaddedBlock -Bytes $chunk -Size $packetSize

        Send-YModemPacket `
            -Serial $serial `
            -Header $packetHeader `
            -Sequence $sequence `
            -Data $block

        Wait-SerialByte -Serial $serial -Expected $ACK -TimeoutSeconds $TimeoutSec

        $offset += $chunkLength
        $sequence = [byte](($sequence + 1) -band 0xff)

        $percent = [Math]::Floor(($offset * 100.0) / [Math]::Max(1, $fileBytes.Length))
        Write-Progress `
            -Activity "Uploading $RemoteName" `
            -Status "$offset / $($fileBytes.Length) bytes" `
            -PercentComplete $percent
    }

    Write-Progress -Activity "Uploading $RemoteName" -Completed

    Write-SerialBytes -Serial $serial -Bytes ([byte[]]@($EOT))
    Wait-SerialByte -Serial $serial -Expected $ACK -TimeoutSeconds $TimeoutSec
    Wait-SerialByte -Serial $serial -Expected $CRC -TimeoutSeconds $TimeoutSec

    $emptyBlock = New-Object byte[] $Packet128
    Send-YModemPacket -Serial $serial -Header $SOH -Sequence 0 -Data $emptyBlock
    Wait-SerialByte -Serial $serial -Expected $ACK -TimeoutSeconds $TimeoutSec

    Write-SerialText -Serial $serial -Text "`r" -CharDelayMs $CommandCharDelayMs
    Start-Sleep -Milliseconds $InterCommandDelayMs

    if ($RemoteMode -ne "") {
        Write-SerialText `
            -Serial $serial `
            -Text "chmod $RemoteMode $RemoteDir/$RemoteName`r" `
            -CharDelayMs $CommandCharDelayMs
        Start-Sleep -Milliseconds $InterCommandDelayMs
    }

    Write-Host "Uploaded $($sourceItem.FullName) to $RemoteDir/$RemoteName"
} finally {
    if ($serial.IsOpen) {
        $serial.Close()
    }
}

# end of nuttx-rp2350-upload-ymodem.ps1

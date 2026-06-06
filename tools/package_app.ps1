param(
    [Parameter(Mandatory = $true)]
    [string]$InputPath,

    [Parameter(Mandatory = $true)]
    [string]$OutputPath,

    [string]$Version = '2.0'
)

$ErrorActionPreference = 'Stop'

$appStart = [uint32]0x08011000
$appEnd = [uint32]0x08031000
$sramStart = [uint32]0x20000000
$sramEnd = [uint32]0x20030000
$magic = [byte[]](0x5A, 0xA5, 0xC3, 0x3C)

$raw = [System.IO.File]::ReadAllBytes($InputPath)
if ($raw.Length -lt 8) {
    throw "App image is too small: $InputPath"
}

$stack = [BitConverter]::ToUInt32($raw, 0)
$entry = [BitConverter]::ToUInt32($raw, 4)
$entryAddress = $entry -band 0xFFFFFFFE

if (($stack -lt $sramStart) -or ($stack -ge $sramEnd)) {
    throw ('Invalid App stack address: 0x{0:X8}' -f $stack)
}

if (($entryAddress -lt $appStart) -or ($entryAddress -ge $appEnd)) {
    throw ('Invalid App reset address: 0x{0:X8}' -f $entry)
}

$package = New-Object byte[] ($magic.Length + $raw.Length)
[Array]::Copy($magic, 0, $package, 0, $magic.Length)
[Array]::Copy($raw, 0, $package, $magic.Length, $raw.Length)

$outputDirectory = Split-Path -Parent $OutputPath
if ($outputDirectory) {
    [System.IO.Directory]::CreateDirectory($outputDirectory) | Out-Null
}

[System.IO.File]::WriteAllBytes($OutputPath, $package)
Write-Host ('Packaged App V{0}: {1} bytes -> {2}' -f $Version, $package.Length, $OutputPath)

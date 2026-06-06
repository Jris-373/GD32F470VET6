param(
    [switch]$AllowRawApp,

    [Parameter(Mandatory = $true)]
    [string]$Path,

    [Parameter(ValueFromRemainingArguments = $true)]
    [string[]]$AdditionalPath
)

$ErrorActionPreference = 'Stop'

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$bootConfigPath = Join-Path $repoRoot 'Function\boot\boot_config.h'

function Get-HexDefine {
    param(
        [Parameter(Mandatory = $true)]
        [string]$ConfigText,

        [Parameter(Mandatory = $true)]
        [string]$Name
    )

    $match = [regex]::Match($ConfigText, "(?m)^\s*#define\s+$Name\s+(0x[0-9A-Fa-f]+)U?\b")
    if (-not $match.Success) {
        throw "Cannot find hex define: $Name"
    }
    return [uint32]$match.Groups[1].Value
}

function Format-Hex32 {
    param([uint32]$Value)
    return ('0x{0:X8}' -f $Value)
}

function Read-UInt32LE {
    param(
        [byte[]]$Bytes,
        [int]$Offset
    )
    return [BitConverter]::ToUInt32($Bytes, $Offset)
}

$configText = Get-Content -LiteralPath $bootConfigPath -Raw
$appStart = Get-HexDefine -ConfigText $configText -Name 'BOOT_APP_START_ADDR'
$appSizeLimit = Get-HexDefine -ConfigText $configText -Name 'BOOT_APP_SIZE'
$appEnd = [uint32]($appStart + $appSizeLimit)
$sramStart = Get-HexDefine -ConfigText $configText -Name 'BOOT_SRAM_START_ADDR'
$sramEnd = Get-HexDefine -ConfigText $configText -Name 'BOOT_SRAM_END_ADDR'
$packageMagic = Get-HexDefine -ConfigText $configText -Name 'BOOT_FW_PACKAGE_MAGIC'
$magicBytes = [byte[]](
    (($packageMagic -shr 24) -band 0xFF),
    (($packageMagic -shr 16) -band 0xFF),
    (($packageMagic -shr 8) -band 0xFF),
    ($packageMagic -band 0xFF)
)

$results = New-Object System.Collections.Generic.List[object]
$allOk = $true
$allPaths = @($Path)
if ($AdditionalPath) {
    $allPaths += $AdditionalPath
}

foreach ($item in $allPaths) {
    $resolved = Resolve-Path -LiteralPath $item
    $bytes = [System.IO.File]::ReadAllBytes($resolved.Path)
    $problems = New-Object System.Collections.Generic.List[string]

    $hasMagic = $false
    if ($bytes.Length -ge 4) {
        $hasMagic = (($bytes[0] -eq $magicBytes[0]) -and
            ($bytes[1] -eq $magicBytes[1]) -and
            ($bytes[2] -eq $magicBytes[2]) -and
            ($bytes[3] -eq $magicBytes[3]))
    }

    $vectorOffset = -1
    if ($hasMagic) {
        $vectorOffset = 4
    } elseif ($AllowRawApp) {
        $vectorOffset = 0
        $problems.Add('missing package magic; accepted only because -AllowRawApp was used')
    } else {
        $problems.Add(('missing package magic {0}; Bootloader USART receive will reject this file' -f (Format-Hex32 $packageMagic)))
    }

    $appSize = 0
    $stack = [uint32]0
    $entry = [uint32]0
    $entryAligned = [uint32]0

    if ($vectorOffset -ge 0) {
        $appSize = $bytes.Length - $vectorOffset
        if ($appSize -lt 8) {
            $problems.Add('App payload is smaller than the vector table prefix')
        } else {
            $stack = Read-UInt32LE -Bytes $bytes -Offset $vectorOffset
            $entry = Read-UInt32LE -Bytes $bytes -Offset ($vectorOffset + 4)
            $entryAligned = $entry -band 0xFFFFFFFE
        }

        if (($appSize -le 0) -or ($appSize -gt $appSizeLimit)) {
            $problems.Add(('App payload size {0} exceeds Bootloader limit {1}' -f $appSize, $appSizeLimit))
        }

        if (($stack -lt $sramStart) -or ($stack -gt $sramEnd)) {
            $problems.Add(('invalid stack pointer {0}; expected {1}..{2}' -f (Format-Hex32 $stack), (Format-Hex32 $sramStart), (Format-Hex32 $sramEnd)))
        }

        if (($entry -band 1) -eq 0) {
            $problems.Add(('invalid reset entry {0}; Cortex-M entry must be Thumb/odd' -f (Format-Hex32 $entry)))
        }

        if (($entryAligned -lt $appStart) -or ($entryAligned -ge $appEnd)) {
            $problems.Add(('invalid reset entry {0}; expected aligned address in {1}..{2}' -f (Format-Hex32 $entry), (Format-Hex32 $appStart), (Format-Hex32 ([uint32]($appEnd - 1)))))
        }
    }

    $hardFailures = @($problems | Where-Object { $_ -notlike 'missing package magic; accepted only because*' })
    $ok = ($hardFailures.Count -eq 0)
    if (-not $ok) {
        $allOk = $false
    }

    $results.Add([pscustomobject]@{
        File = $resolved.Path
        SizeBytes = $bytes.Length
        HasMagic = $hasMagic
        AppSizeBytes = $appSize
        Stack = Format-Hex32 $stack
        Entry = Format-Hex32 $entry
        Result = if ($ok) { 'PASS' } else { 'FAIL' }
        Problems = if ($problems.Count -eq 0) { '-' } else { ($problems -join '; ') }
    })
}

$results | Format-Table -AutoSize -Wrap

if (-not $allOk) {
    exit 1
}

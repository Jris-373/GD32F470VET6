param(
    [string]$OutputRoot = "export\contest_projects",

    [ValidateSet("Link", "ManifestOnly")]
    [string]$Mode = "Link",

    [switch]$Force
)

$ErrorActionPreference = 'Stop'

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$outputRootPath = Join-Path $repoRoot $OutputRoot
$targets = @('App', 'Bootloader')
$sourceDirs = @(
    'Driver',
    'Function',
    'Protocol',
    'common',
    'drivers',
    'src',
    'startup',
    'tools',
    'doc'
)
$smallRootFiles = @(
    '.clang-format',
    'AGENTS.md',
    'CONTEST_STRUCTURE.md'
)
$smallEideFiles = @(
    'debug.st.option.bytes.ini',
    'env.ini',
    'files.options.yml'
)
$smallVscodeFiles = @(
    'tasks.json'
)

function Get-SingleTargetEideYaml {
    param(
        [Parameter(Mandatory = $true)]
        [string]$YamlText,

        [Parameter(Mandatory = $true)]
        [string]$TargetName
    )

    $targetsIndex = $YamlText.IndexOf("targets:")
    if ($targetsIndex -lt 0) {
        throw "Cannot find targets: section in .eide/eide.yml"
    }

    $prefix = $YamlText.Substring(0, $targetsIndex)
    $targetsText = $YamlText.Substring($targetsIndex + "targets:".Length)
    $escapedTarget = [regex]::Escape($TargetName)
    $match = [regex]::Match($targetsText, "(?ms)^  $escapedTarget`:\r?\n.*?(?=^  [A-Za-z0-9_-]+`:\r?\n|\z)")
    if (-not $match.Success) {
        throw "Cannot find target '$TargetName' in .eide/eide.yml"
    }

    $projectName = "CMIS_$TargetName"
    $prefix = [regex]::Replace($prefix, '(?m)^name:\s*.*$', "name: $projectName")
    return ($prefix + "targets:`r`n" + $match.Value.TrimEnd() + "`r`n")
}

function Copy-SmallFileIfExists {
    param(
        [Parameter(Mandatory = $true)]
        [string]$RelativePath,

        [Parameter(Mandatory = $true)]
        [string]$ProjectDir
    )

    $sourcePath = Join-Path $repoRoot $RelativePath
    if (-not (Test-Path -LiteralPath $sourcePath -PathType Leaf)) {
        return
    }

    $destPath = Join-Path $ProjectDir $RelativePath
    $destParent = Split-Path -Parent $destPath
    if ($destParent) {
        New-Item -ItemType Directory -Force -Path $destParent | Out-Null
    }
    Copy-Item -LiteralPath $sourcePath -Destination $destPath -Force
}

function New-DirectoryJunction {
    param(
        [Parameter(Mandatory = $true)]
        [string]$RelativePath,

        [Parameter(Mandatory = $true)]
        [string]$ProjectDir
    )

    $sourcePath = Join-Path $repoRoot $RelativePath
    if (-not (Test-Path -LiteralPath $sourcePath -PathType Container)) {
        return
    }

    $linkPath = Join-Path $ProjectDir $RelativePath
    if (Test-Path -LiteralPath $linkPath) {
        return
    }

    $linkParent = Split-Path -Parent $linkPath
    if ($linkParent) {
        New-Item -ItemType Directory -Force -Path $linkParent | Out-Null
    }
    New-Item -ItemType Junction -Path $linkPath -Target $sourcePath | Out-Null
}

function Write-WorkspaceFile {
    param(
        [Parameter(Mandatory = $true)]
        [string]$ProjectDir
    )

    $workspacePath = Join-Path $ProjectDir 'firmware.code-workspace'
    @'
{
    "folders": [
        {
            "path": "."
        }
    ],
    "settings": {
        "terminal.integrated.shellIntegration.enabled": false,
        "clangd.arguments": [
            "--header-insertion=never"
        ],
        "files.autoGuessEncoding": true,
        "C_Cpp.default.configurationProvider": "cl.eide",
        "C_Cpp.errorSquiggles": "disabled"
    },
    "extensions": {
        "recommendations": [
            "cl.eide",
            "marus25.cortex-debug",
            "redhat.vscode-yaml"
        ]
    }
}
'@ | Set-Content -LiteralPath $workspacePath -Encoding UTF8
}

function Write-ProjectReadme {
    param(
        [Parameter(Mandatory = $true)]
        [string]$ProjectDir,

        [Parameter(Mandatory = $true)]
        [string]$TargetName
    )

    $readmePath = Join-Path $ProjectDir 'README_EXPORT.md'
    @"
# CMIS $TargetName EIDE Export

This is a thin contest export generated from the dual-target CMIS EIDE project.

- Active EIDE target: `$TargetName`
- Source folders are NTFS junctions pointing back to the repository root.
- Large source/vendor directories are not copied by this export.
- Build from VS Code with the `cl.eide` extension after opening `firmware.code-workspace`.

For a final self-contained archive, do not blindly zip this folder if your zip tool
follows junctions. Use `source_manifest.txt` to materialize only the listed files
after confirming the final submission rule.
"@ | Set-Content -LiteralPath $readmePath -Encoding UTF8
}

function Write-Manifest {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Path,

        [Parameter(Mandatory = $true)]
        [string]$TargetName
    )

    $lines = New-Object System.Collections.Generic.List[string]
    $lines.Add("target=$TargetName")
    $lines.Add("repo_root=$repoRoot")
    $lines.Add("mode=$Mode")
    $lines.Add("")
    $lines.Add("[linked_source_dirs]")
    foreach ($dir in $sourceDirs) {
        if (Test-Path -LiteralPath (Join-Path $repoRoot $dir) -PathType Container) {
            $lines.Add($dir)
        }
    }
    $lines.Add("")
    $lines.Add("[small_files]")
    foreach ($file in $smallRootFiles) {
        if (Test-Path -LiteralPath (Join-Path $repoRoot $file) -PathType Leaf) {
            $lines.Add($file)
        }
    }
    foreach ($file in $smallEideFiles) {
        if (Test-Path -LiteralPath (Join-Path $repoRoot ".eide\$file") -PathType Leaf) {
            $lines.Add(".eide/$file")
        }
    }
    foreach ($file in $smallVscodeFiles) {
        if (Test-Path -LiteralPath (Join-Path $repoRoot ".vscode\$file") -PathType Leaf) {
            $lines.Add(".vscode/$file")
        }
    }

    $manifestDir = Split-Path -Parent $Path
    if ($manifestDir) {
        New-Item -ItemType Directory -Force -Path $manifestDir | Out-Null
    }
    $lines | Set-Content -LiteralPath $Path -Encoding UTF8
}

if ((Test-Path -LiteralPath $outputRootPath) -and (-not $Force)) {
    throw "Output path already exists: $outputRootPath. Use -Force to replace it."
}

if ((Test-Path -LiteralPath $outputRootPath) -and $Force) {
    Write-Host "Reusing existing output path: $outputRootPath"
}

New-Item -ItemType Directory -Force -Path $outputRootPath | Out-Null

$eideYamlPath = Join-Path $repoRoot '.eide\eide.yml'
$eideYaml = Get-Content -LiteralPath $eideYamlPath -Raw

foreach ($target in $targets) {
    if ($Mode -eq 'ManifestOnly') {
        Write-Manifest -Path (Join-Path $outputRootPath "$target`_Project.source_manifest.txt") -TargetName $target
        continue
    }

    $projectDir = Join-Path $outputRootPath "$target`_Project"
    New-Item -ItemType Directory -Force -Path $projectDir | Out-Null
    New-Item -ItemType Directory -Force -Path (Join-Path $projectDir '.eide') | Out-Null
    New-Item -ItemType Directory -Force -Path (Join-Path $projectDir '.vscode') | Out-Null

    Get-SingleTargetEideYaml -YamlText $eideYaml -TargetName $target |
        Set-Content -LiteralPath (Join-Path $projectDir '.eide\eide.yml') -Encoding UTF8

    foreach ($file in $smallRootFiles) {
        Copy-SmallFileIfExists -RelativePath $file -ProjectDir $projectDir
    }
    foreach ($file in $smallEideFiles) {
        Copy-SmallFileIfExists -RelativePath ".eide\$file" -ProjectDir $projectDir
    }
    foreach ($file in $smallVscodeFiles) {
        Copy-SmallFileIfExists -RelativePath ".vscode\$file" -ProjectDir $projectDir
    }
    foreach ($dir in $sourceDirs) {
        New-DirectoryJunction -RelativePath $dir -ProjectDir $projectDir
    }

    Write-WorkspaceFile -ProjectDir $projectDir
    Write-ProjectReadme -ProjectDir $projectDir -TargetName $target
    Write-Manifest -Path (Join-Path $projectDir 'source_manifest.txt') -TargetName $target
}

Write-Host "Export generated: $outputRootPath"
if ($Mode -eq 'Link') {
    Write-Host "Generated folders: App_Project, Bootloader_Project"
    Write-Host "Warning: source folders are junctions. Do not zip with junction-following mode."
}

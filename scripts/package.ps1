param(
    [string]$Version = "0.1.0"
)

$ErrorActionPreference = "Stop"
Add-Type -AssemblyName System.IO.Compression.FileSystem
$projectRoot = Split-Path -Parent $PSScriptRoot
$stageRoot = Join-Path $projectRoot ".dist-stage"
$distRoot = Join-Path $projectRoot "dist"

if (Test-Path -LiteralPath $stageRoot) {
    Remove-Item -LiteralPath $stageRoot -Recurse -Force
}
New-Item -ItemType Directory -Path $stageRoot, $distRoot -Force | Out-Null

$platforms = @(
    @{
        Name = "windows"
        Module = Join-Path $projectRoot "bin/windows/vexid_amxx.dll"
    },
    @{
        Name = "linux"
        Module = Join-Path $projectRoot "bin/linux/vexid_amxx_i386.so"
    }
)

foreach ($platform in $platforms) {
    if (-not (Test-Path -LiteralPath $platform.Module)) {
        throw "Missing module binary: $($platform.Module)"
    }

    $packageName = "VEXID-v$Version-$($platform.Name)"
    $packageRoot = Join-Path $stageRoot $packageName
    $moduleDir = Join-Path $packageRoot "addons/amxmodx/modules"
    $pluginDir = Join-Path $packageRoot "addons/amxmodx/plugins"
    $scriptingDir = Join-Path $packageRoot "addons/amxmodx/scripting"
    $includeDir = Join-Path $scriptingDir "include"
    $docsDir = Join-Path $packageRoot "docs"

    New-Item -ItemType Directory -Path $moduleDir, $pluginDir, $includeDir, $docsDir -Force | Out-Null
    Copy-Item -LiteralPath $platform.Module -Destination $moduleDir
    Copy-Item -LiteralPath (Join-Path $projectRoot "bin/vexid_test.amxx") -Destination $pluginDir
    Copy-Item -LiteralPath (Join-Path $projectRoot "tests/vexid_test.sma") -Destination $scriptingDir
    Copy-Item -LiteralPath (Join-Path $projectRoot "include/vexid.inc") -Destination $includeDir
    Copy-Item -LiteralPath (Join-Path $projectRoot "README.md") -Destination $packageRoot
    Copy-Item -LiteralPath (Join-Path $projectRoot "LICENSE.md") -Destination $packageRoot
    Copy-Item -LiteralPath (Join-Path $projectRoot "docs/INSTALL_TR.md") -Destination $docsDir

    $checksumTarget = Get-ChildItem -LiteralPath $moduleDir | Select-Object -First 1
    $checksum = Get-FileHash -Algorithm SHA256 -LiteralPath $checksumTarget.FullName
    "$($checksum.Hash.ToLowerInvariant())  $($checksumTarget.Name)" |
        Set-Content -LiteralPath (Join-Path $packageRoot "SHA256SUMS.txt") -Encoding ascii

    $archive = Join-Path $distRoot "$packageName.zip"
    if (Test-Path -LiteralPath $archive) {
        Remove-Item -LiteralPath $archive -Force
    }
    [System.IO.Compression.ZipFile]::CreateFromDirectory(
        $packageRoot,
        $archive,
        [System.IO.Compression.CompressionLevel]::Optimal,
        $false
    )
}

Remove-Item -LiteralPath $stageRoot -Recurse -Force
$archives = Get-ChildItem -LiteralPath $distRoot -Filter "VEXID-v$Version-*.zip" | Sort-Object Name
$archiveChecksums = foreach ($archive in $archives) {
    $hash = (Get-FileHash -Algorithm SHA256 -LiteralPath $archive.FullName).Hash.ToLowerInvariant()
    "$hash  $($archive.Name)"
}
$archiveChecksums | Set-Content -LiteralPath (Join-Path $distRoot "SHA256SUMS.txt") -Encoding ascii

$archives |
    Select-Object Name, Length, @{Name="SHA256";Expression={(Get-FileHash -Algorithm SHA256 -LiteralPath $_.FullName).Hash.ToLowerInvariant()}}

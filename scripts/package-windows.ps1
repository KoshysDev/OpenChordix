[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$ExecutablePath,

    [Parameter(Mandatory = $true)]
    [string]$OutputDirectory,

    [Parameter(Mandatory = $true)]
    [string[]]$SearchDirectories,

    [string]$ZipPath
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

. (Join-Path $PSScriptRoot "WindowsRuntimeDependencies.ps1")

$sourceExecutable = (Resolve-Path -LiteralPath $ExecutablePath).Path
$outputPath = [System.IO.Path]::GetFullPath($OutputDirectory)
$sourceDirectory = [System.IO.Path]::GetDirectoryName($sourceExecutable)
if ($outputPath.TrimEnd('\') -eq $sourceDirectory.TrimEnd('\')) {
    throw "OutputDirectory must differ from the directory containing the installed executable."
}

$resolvedSearchDirectories = @()
foreach ($directory in $SearchDirectories) {
    if (Test-Path -LiteralPath $directory -PathType Container) {
        $resolvedSearchDirectories += (Resolve-Path -LiteralPath $directory).Path
    }
}
if ($resolvedSearchDirectories.Count -eq 0) {
    throw "No runtime dependency search directories exist."
}

if (Test-Path -LiteralPath $outputPath) {
    Remove-Item -LiteralPath $outputPath -Recurse -Force
}
New-Item -Path $outputPath -ItemType Directory -Force | Out-Null

$stagedExecutable = Join-Path $outputPath "OpenChordix.exe"
Copy-Item -LiteralPath $sourceExecutable -Destination $stagedExecutable -Force

$tool = Find-OpenChordixDependencyTool
Write-Host "Collecting Windows runtime dependencies with $($tool.Kind)."

$queue = [System.Collections.Generic.Queue[string]]::new()
$queue.Enqueue($stagedExecutable)
$inspected = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::OrdinalIgnoreCase)
$copied = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::OrdinalIgnoreCase)

while ($queue.Count -gt 0) {
    $binaryPath = $queue.Dequeue()
    if (-not $inspected.Add($binaryPath)) {
        continue
    }

    foreach ($dllName in Get-OpenChordixImportedDlls -BinaryPath $binaryPath -Tool $tool) {
        if (Test-OpenChordixSystemDll -DllName $dllName) {
            continue
        }

        $stagedDll = Join-Path $outputPath $dllName
        if (-not (Test-Path -LiteralPath $stagedDll -PathType Leaf)) {
            $sourceDll = $null
            foreach ($searchDirectory in $resolvedSearchDirectories) {
                $candidate = Join-Path $searchDirectory $dllName
                if (Test-Path -LiteralPath $candidate -PathType Leaf) {
                    $sourceDll = $candidate
                    break
                }
            }

            if ($null -eq $sourceDll) {
                throw "Required runtime DLL '$dllName' imported by '$binaryPath' was not found in the packaging search directories or Windows system directory."
            }

            Copy-Item -LiteralPath $sourceDll -Destination $stagedDll -Force
            [void]$copied.Add($dllName)
        }

        $queue.Enqueue($stagedDll)
    }
}

if ($ZipPath) {
    $resolvedZipPath = [System.IO.Path]::GetFullPath($ZipPath)
    if (Test-Path -LiteralPath $resolvedZipPath) {
        Remove-Item -LiteralPath $resolvedZipPath -Force
    }
    Compress-Archive -Path (Join-Path $outputPath "*") -DestinationPath $resolvedZipPath
    Write-Host "Created Windows package: $resolvedZipPath"
}

Write-Host "Staged Windows package contents:"
Get-ChildItem -LiteralPath $outputPath -File | Sort-Object Name | ForEach-Object { Write-Host "  $($_.Name)" }

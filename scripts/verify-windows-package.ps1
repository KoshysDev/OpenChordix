[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$PackageDirectory,

    [switch]$RunVersionCheck
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

. (Join-Path $PSScriptRoot "WindowsRuntimeDependencies.ps1")

$packagePath = (Resolve-Path -LiteralPath $PackageDirectory).Path
$executable = Join-Path $packagePath "OpenChordix.exe"
if (-not (Test-Path -LiteralPath $executable -PathType Leaf)) {
    throw "OpenChordix.exe is missing from the Windows package directory."
}

foreach ($excludedName in @("audio.conf", "songs.db", "tunings.json")) {
    if (Test-Path -LiteralPath (Join-Path $packagePath $excludedName)) {
        throw "Generated runtime data file '$excludedName' must not be included in the Windows package."
    }
}

$unwantedArtifacts = @(Get-ChildItem -LiteralPath $packagePath -Recurse -File | Where-Object {
    $_.Name -match '\.(a|lib|pdb)$' -or $_.Name -match '\.dll\.a$'
})
if ($unwantedArtifacts.Count -gt 0) {
    throw "The Windows package contains non-runtime build artifacts: $($unwantedArtifacts.Name -join ', ')"
}

$tool = Find-OpenChordixDependencyTool
Write-Host "Validating Windows runtime dependencies with $($tool.Kind)."

$queue = [System.Collections.Generic.Queue[string]]::new()
$queue.Enqueue($executable)
$inspected = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::OrdinalIgnoreCase)
$missing = [System.Collections.Generic.List[string]]::new()

while ($queue.Count -gt 0) {
    $binaryPath = $queue.Dequeue()
    if (-not $inspected.Add($binaryPath)) {
        continue
    }

    foreach ($dllName in Get-OpenChordixImportedDlls -BinaryPath $binaryPath -Tool $tool) {
        if (Test-OpenChordixSystemDll -DllName $dllName) {
            continue
        }

        $stagedDll = Join-Path $packagePath $dllName
        if (-not (Test-Path -LiteralPath $stagedDll -PathType Leaf)) {
            $missing.Add("$dllName (required by $([System.IO.Path]::GetFileName($binaryPath)))")
            continue
        }

        $queue.Enqueue($stagedDll)
    }
}

if ($missing.Count -gt 0) {
    throw "Windows package is missing non-system runtime DLLs: $($missing -join ', ')"
}

if ($RunVersionCheck) {
    $process = Start-Process -FilePath $executable -ArgumentList "--version" -WorkingDirectory $packagePath -NoNewWindow -Wait -PassThru
    if ($process.ExitCode -ne 0) {
        throw "OpenChordix.exe --version failed from the staged Windows package with exit code $($process.ExitCode)."
    }
}

Write-Host "Windows package validation passed. All recursively imported non-system DLLs are staged."

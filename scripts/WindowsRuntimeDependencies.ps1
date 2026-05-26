Set-StrictMode -Version Latest

function Find-OpenChordixDependencyTool {
    foreach ($candidate in @("objdump", "llvm-objdump", "dumpbin")) {
        $command = Get-Command $candidate -ErrorAction SilentlyContinue | Select-Object -First 1
        if ($null -ne $command) {
            return [PSCustomObject]@{
                Path = $command.Source
                Kind = $candidate
            }
        }
    }

    throw "No supported PE dependency inspection tool was found (objdump, llvm-objdump, or dumpbin)."
}

function Get-OpenChordixImportedDlls {
    param(
        [Parameter(Mandatory = $true)]
        [string]$BinaryPath,

        [Parameter(Mandatory = $true)]
        [PSCustomObject]$Tool
    )

    if ($Tool.Kind -eq "dumpbin") {
        $output = & $Tool.Path /dependents $BinaryPath 2>&1
        if ($LASTEXITCODE -ne 0) {
            throw "dumpbin failed while inspecting $BinaryPath"
        }

        return @($output | ForEach-Object {
            if ($_ -match '^\s*([A-Za-z0-9_.+-]+\.dll)\s*$') {
                $Matches[1]
            }
        })
    }

    $output = & $Tool.Path -p $BinaryPath 2>&1
    if ($LASTEXITCODE -ne 0) {
        throw "$($Tool.Kind) failed while inspecting $BinaryPath"
    }

    return @($output | ForEach-Object {
        if ($_ -match 'DLL Name:\s*([^\s]+\.dll)') {
            $Matches[1]
        }
    })
}

function Test-OpenChordixSystemDll {
    param(
        [Parameter(Mandatory = $true)]
        [string]$DllName
    )

    if ($DllName -match '^(api-ms-win-|ext-ms-win-)') {
        return $true
    }

    $systemPath = Join-Path ([Environment]::SystemDirectory) $DllName
    return Test-Path -LiteralPath $systemPath -PathType Leaf
}

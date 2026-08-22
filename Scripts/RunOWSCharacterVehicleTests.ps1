[CmdletBinding()]
param(
    [string]$EngineRoot = $env:OWS_UNREAL_ENGINE_ROOT
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$ProjectRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$ProjectPath = Join-Path $ProjectRoot 'OWS.uproject'
$ExitCode = 1
$TempRoot = Join-Path ([System.IO.Path]::GetTempPath()) ('ows-character-vehicle-' + [guid]::NewGuid().ToString('N'))
$StdoutPath = Join-Path $TempRoot 'stdout.log'
$UnrealLogPath = Join-Path $TempRoot 'unreal.log'
$ReportPath = Join-Path $TempRoot 'Report'

function Resolve-EngineRoot {
    param([string]$RequestedRoot)

    $Candidates = New-Object System.Collections.Generic.List[string]
    if (-not [string]::IsNullOrWhiteSpace($RequestedRoot)) {
        $Candidates.Add($RequestedRoot)
    }
    else {
        $Association = (Get-Content -Raw -LiteralPath $ProjectPath | ConvertFrom-Json).EngineAssociation
        if (-not [string]::IsNullOrWhiteSpace($Association)) {
            $Candidates.Add((Join-Path $env:ProgramFiles ('Epic Games\UE_' + $Association)))
            $Candidates.Add(('C:\UE_' + $Association))
        }
    }

    foreach ($Candidate in $Candidates) {
        if ([string]::IsNullOrWhiteSpace($Candidate)) {
            continue
        }
        $ResolvedCandidate = [System.IO.Path]::GetFullPath($Candidate)
        $EditorCommand = Join-Path $ResolvedCandidate 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
        if (Test-Path -LiteralPath $EditorCommand -PathType Leaf) {
            return $ResolvedCandidate
        }
    }

    throw 'Unreal Engine was not found. Pass -EngineRoot <path> or set OWS_UNREAL_ENGINE_ROOT to the Unreal Engine 5.8 installation directory.'
}

function Write-FailureExcerpt {
    param([object[]]$Tests)

    foreach ($Test in $Tests) {
        if ($Test.state -eq 'Success') {
            continue
        }
        [Console]::Error.WriteLine("[OWS character/vehicle] $($Test.fullTestPath): $($Test.state)")
        $Messages = @(
            $Test.entries.event |
                Where-Object { $_.type -eq 'Error' } |
                Select-Object -ExpandProperty message -Unique |
                Select-Object -First 20
        )
        foreach ($Message in $Messages) {
            [Console]::Error.WriteLine('  ' + $Message)
        }
    }
}

try {
    [void][System.IO.Directory]::CreateDirectory($TempRoot)

    if (-not (Test-Path -LiteralPath $ProjectPath -PathType Leaf)) {
        throw "Project file not found: $ProjectPath"
    }

    $ResolvedEngineRoot = Resolve-EngineRoot $EngineRoot
    $EditorCommand = Join-Path $ResolvedEngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
    [Console]::WriteLine("[OWS character/vehicle] Unreal: $EditorCommand")
    [Console]::WriteLine('[OWS character/vehicle] Running canonical-map functional tests...')

    $UnrealArguments = @(
        $ProjectPath,
        '-ExecCmds=Automation RunTests OWS.CharacterVehicle',
        '-TestExit=Automation Test Queue Empty',
        ('-ReportExportPath=' + $ReportPath),
        '-unattended',
        '-nop4',
        '-nosplash',
        '-nosound',
        '-RenderOffscreen',
        '-stdout',
        '-FullStdOutLogOutput',
        ('-abslog=' + $UnrealLogPath)
    )

    & $EditorCommand @UnrealArguments *> $StdoutPath
    $UnrealExitCode = $LASTEXITCODE
    $ReportFile = Join-Path $ReportPath 'index.json'
    if (-not (Test-Path -LiteralPath $ReportFile -PathType Leaf)) {
        throw "Unreal exited with code $UnrealExitCode without producing an automation report. Build OWSEditor, then rerun the test."
    }

    $Report = Get-Content -Raw -LiteralPath $ReportFile | ConvertFrom-Json
    $Tests = @($Report.tests | Where-Object { $_.fullTestPath -like 'OWS.CharacterVehicle.*' })
    $ExpectedTests = @(
        'OWS.CharacterVehicle.MovingBailoutRecovery',
        'OWS.CharacterVehicle.StoppedExitRepeatedCycles'
    )
    foreach ($ExpectedTest in $ExpectedTests) {
        if ($Tests.fullTestPath -notcontains $ExpectedTest) {
            throw "Automation report is missing required test: $ExpectedTest"
        }
    }

    $FailedTests = @($Tests | Where-Object { $_.state -ne 'Success' })
    if ($UnrealExitCode -ne 0 -or $FailedTests.Count -gt 0) {
        Write-FailureExcerpt $Tests
        throw "Character/vehicle automation failed: process exit code $UnrealExitCode; failed tests $($FailedTests.Count)."
    }

    [Console]::WriteLine('[OWS character/vehicle] PASS: repeated stopped cycles and moving bailout/recovery passed.')
    $ExitCode = 0
}
catch {
    [Console]::Error.WriteLine('[OWS character/vehicle] FAIL: ' + $_.Exception.Message)
}
finally {
    if (Test-Path -LiteralPath $TempRoot -PathType Container) {
        Get-ChildItem -LiteralPath $TempRoot -Force -Recurse -ErrorAction SilentlyContinue | ForEach-Object {
            $_.Attributes = 'Normal'
        }
        [System.IO.Directory]::Delete($TempRoot, $true)
    }
}

exit $ExitCode

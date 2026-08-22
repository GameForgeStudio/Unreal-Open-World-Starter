[CmdletBinding()]
param(
    [string]$EngineRoot = $env:OWS_UNREAL_ENGINE_ROOT
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$ProjectRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$ProjectPath = Join-Path $ProjectRoot 'OWS.uproject'
$ValidationScript = Join-Path $ProjectRoot 'Scripts\SmokeTest\OWSCleanStartSmoke.py'
$ExitCode = 1
$TempRoot = Join-Path ([System.IO.Path]::GetTempPath()) ('ows-clean-start-' + [guid]::NewGuid().ToString('N'))
$StdoutPath = Join-Path $TempRoot 'stdout.log'
$UnrealLogPath = Join-Path $TempRoot 'unreal.log'

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
    if (-not (Test-Path -LiteralPath $StdoutPath -PathType Leaf)) {
        return
    }

    $InterestingLines = Select-String -LiteralPath $StdoutPath -Pattern @(
        'OWS_SMOKE\|',
        'Error:',
        'Fatal error',
        'Failed to find object',
        "Couldn't find file for package",
        "Can't find file",
        'Missing or built with a different engine version'
    ) | Select-Object -Last 40

    if ($InterestingLines) {
        [Console]::Error.WriteLine('[OWS smoke] Relevant Unreal output:')
        foreach ($Line in $InterestingLines) {
            [Console]::Error.WriteLine($Line.Line)
        }
    }
}

try {
    [void][System.IO.Directory]::CreateDirectory($TempRoot)

    if (-not (Test-Path -LiteralPath $ProjectPath -PathType Leaf)) {
        throw "Project file not found: $ProjectPath"
    }
    if (-not (Test-Path -LiteralPath $ValidationScript -PathType Leaf)) {
        throw "Unreal validation script not found: $ValidationScript"
    }
    if (-not (Get-Command git -ErrorAction SilentlyContinue)) {
        throw 'Git was not found on PATH.'
    }

    [Console]::WriteLine('[OWS smoke] Checking Git LFS objects...')
    $LfsOutput = @(& git -C $ProjectRoot lfs fsck 2>&1)
    if ($LASTEXITCODE -ne 0) {
        throw "Git LFS integrity check failed:`n$($LfsOutput -join [Environment]::NewLine)"
    }

    $ResolvedEngineRoot = Resolve-EngineRoot $EngineRoot
    $EditorCommand = Join-Path $ResolvedEngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
    [Console]::WriteLine("[OWS smoke] Unreal: $EditorCommand")
    [Console]::WriteLine('[OWS smoke] Loading the canonical OWS experience...')

    $UnrealArguments = @(
        $ProjectPath,
        '-run=pythonscript',
        ('-script=' + $ValidationScript),
        '-unattended',
        '-nop4',
        '-nosplash',
        '-nosound',
        '-nullrhi',
        '-stdout',
        '-FullStdOutLogOutput',
        '-UTF8Output',
        ('-abslog=' + $UnrealLogPath)
    )

    & $EditorCommand @UnrealArguments *> $StdoutPath
    $UnrealExitCode = $LASTEXITCODE
    $Output = Get-Content -Raw -LiteralPath $StdoutPath

    if ($UnrealExitCode -ne 0) {
        if ($Output -match 'OWS_SMOKE\|' -or $Output -match 'Python script executed with errors') {
            throw "Unreal-side clean-start assertions failed with process exit code $UnrealExitCode."
        }
        throw "Unreal failed before the OWS assertions ran (process exit code $UnrealExitCode). Build OWSEditor for Unreal Engine 5.8.1, then rerun the test."
    }

    $FailurePatterns = @(
        '(?i)Failed to find object',
        "(?i)Couldn['’]t find file for package",
        "(?i)Can['’]t find file",
        '(?i)LoadErrors:\s+Error:',
        '(?i)Map check complete:\s+[1-9][0-9]* Error',
        '(?i)/Game/OWSPrototype/',
        '(?i)OWSTestLab'
    )
    foreach ($Pattern in $FailurePatterns) {
        if ($Output -match $Pattern) {
            throw "Unreal output matched required-asset or canonical-map failure pattern: $Pattern"
        }
    }

    if ($Output -notmatch 'OWS_SMOKE\|PASS\|clean-start validation passed') {
        throw 'Unreal exited without the required OWS smoke-test pass marker.'
    }

    [Console]::WriteLine('[OWS smoke] PASS: Git LFS, module startup, required assets, canonical maps, Map Check, and three-character setup passed.')
    $ExitCode = 0
}
catch {
    [Console]::Error.WriteLine('[OWS smoke] FAIL: ' + $_.Exception.Message)
    Write-FailureExcerpt
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

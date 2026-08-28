[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$OutputPath,

    [switch]$StaticOnly,

    [string]$EngineRoot = $env:OWS_UNREAL_ENGINE_ROOT
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$ProjectRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$ModulePath = Join-Path $ProjectRoot 'Scripts\PlatformBaseline\OWSPlatformBaseline.psm1'
$KnownGapsPath = Join-Path $ProjectRoot 'Scripts\PlatformBaseline\known-gaps.json'
$AssetExporterRelativePath = 'Scripts\PlatformBaseline\ExportOWSAssetBaseline.py'
$ExitCode = 1
$Report = $null
$FatalMessage = $null
$CommandRecords = New-Object System.Collections.Generic.List[object]
$TempRoot = $null
$WorktreePath = $null
$LogsPath = $null
$StagePath = $null
$ArchivePath = $null
$AssetRegistryDumpPath = $null
$ContainerManifestPath = $null
$AssetBaselinePath = $null
$AssetSizeCsvPath = $null
$SelectorReportPath = $null
$ResolvedEngineRoot = $null
$EngineChangelist = $null
$CompatibleChangelist = $null
$WorktreeAdded = $false
$WorktreeRemoved = $true
$CleanupPassed = $true
$CleanupIssue = $null
$EnvironmentBackup = @{}
$EnvironmentIsolated = $false
$InitialRelevantProcesses = @()
$RunStartedUtc = [DateTime]::UtcNow
$SanitizedTaskFiles = @{}
$SelfTest = $null
$PackageResult = $null
$PackageOracle = $null

Import-Module -Force $ModulePath
$ResolvedOutputPath = Assert-OWSExternalOutputPath -RepositoryRoot $ProjectRoot -OutputPath $OutputPath
$OriginalPreState = Get-OWSRepositoryState -RepositoryRoot $ProjectRoot

function Resolve-OWSEngineRoot {
    param([string]$RequestedRoot, [string]$RepositoryRoot)

    $Candidates = New-Object System.Collections.Generic.List[string]
    if (-not [string]::IsNullOrWhiteSpace($RequestedRoot)) {
        $Candidates.Add($RequestedRoot)
    }
    else {
        $ProjectData = Read-OWSJsonFile -Path (Join-Path $RepositoryRoot 'OWS.uproject')
        $Association = [string]$ProjectData.EngineAssociation
        if (-not [string]::IsNullOrWhiteSpace($Association)) {
            $Candidates.Add((Join-Path $env:ProgramFiles ('Epic Games\UE_' + $Association)))
            $Candidates.Add(('C:\UE_' + $Association))
        }
    }

    foreach ($Candidate in $Candidates) {
        if ([string]::IsNullOrWhiteSpace($Candidate)) { continue }
        $Resolved = [System.IO.Path]::GetFullPath($Candidate)
        if (Test-Path -LiteralPath (Join-Path $Resolved 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe') -PathType Leaf) {
            return $Resolved
        }
    }
    throw 'Unreal Engine was not found. Pass -EngineRoot or set OWS_UNREAL_ENGINE_ROOT.'
}

function Get-OWSEngineVersion {
    param([string]$ResolvedEngineRoot)

    $BuildVersionPath = Join-Path $ResolvedEngineRoot 'Engine\Build\Build.version'
    if (-not (Test-Path -LiteralPath $BuildVersionPath -PathType Leaf)) { return $null }
    $Version = Get-Content -Raw -LiteralPath $BuildVersionPath | ConvertFrom-Json
    return "$($Version.MajorVersion).$($Version.MinorVersion).$($Version.PatchVersion)"
}

function ConvertTo-OWSPortableArgument {
    param([AllowEmptyString()][string]$Value)

    $Portable = ConvertTo-OWSSanitizedText -Text $Value
    $Mappings = @(
        [pscustomobject]@{ Path = $script:WorktreePath; Token = '$WORKTREE' },
        [pscustomobject]@{ Path = $script:TempRoot; Token = '$SCRATCH' },
        [pscustomobject]@{ Path = $script:ResolvedEngineRoot; Token = '$ENGINE' },
        [pscustomobject]@{ Path = $script:ProjectRoot; Token = '$REPOSITORY' }
    ) | Where-Object { -not [string]::IsNullOrWhiteSpace([string]$_.Path) } | Sort-Object { ([string]$_.Path).Length } -Descending
    foreach ($Mapping in $Mappings) {
        $Portable = $Portable.Replace([string]$Mapping.Path, [string]$Mapping.Token)
        $Portable = $Portable.Replace(([string]$Mapping.Path).Replace('\', '/'), [string]$Mapping.Token)
    }
    return $Portable
}

function Invoke-OWSRecordedCommand {
    param(
        [Parameter(Mandatory)][string]$Id,
        [Parameter(Mandatory)][string]$FilePath,
        [Parameter(Mandatory)][string[]]$Arguments,
        [Parameter(Mandatory)][string]$LogRoot
    )

    $LogPath = Join-Path $LogRoot ($Id + '.log')
    $Stopwatch = [System.Diagnostics.Stopwatch]::StartNew()
    $ProcessExitCode = -1
    $Sanitized = ''
    $Encoding = New-Object System.Text.UTF8Encoding($false)
    $Writer = New-Object System.IO.StreamWriter($LogPath, $false, $Encoding)
    $PrivateKeyRedactionActive = $false
    try {
        & $FilePath @Arguments 2>&1 | ForEach-Object {
            $Line = [string]$_
            if ($Line -match '-----BEGIN [^-]*PRIVATE KEY-----') {
                $PrivateKeyRedactionActive = $true
                $Writer.WriteLine('[REDACTED PRIVATE KEY BLOCK]')
            }
            elseif ($PrivateKeyRedactionActive) {
                if ($Line -match '-----END [^-]*PRIVATE KEY-----') { $PrivateKeyRedactionActive = $false }
            }
            else {
                $Writer.WriteLine((ConvertTo-OWSSanitizedText -Text $Line))
            }
        }
        $ProcessExitCode = $LASTEXITCODE
    }
    catch {
        $Sanitized = $_.Exception.GetType().FullName + ': command invocation failed'
        $Writer.WriteLine((ConvertTo-OWSSanitizedText -Text $Sanitized))
        $ProcessExitCode = -1
    }
    finally {
        $Writer.Dispose()
        $Stopwatch.Stop()
    }
    try {
        Protect-OWSGeneratedTaskFiles
    }
    catch {
        $ProcessExitCode = -1
        $FailureText = ConvertTo-OWSSanitizedText -Text ('Generated task-file sanitization failed: ' + $_.Exception.Message)
        [System.IO.File]::AppendAllText($LogPath, $FailureText + [Environment]::NewLine, $Encoding)
    }
    if (Test-Path -LiteralPath $LogPath -PathType Leaf) {
        $Sanitized = Get-Content -Raw -LiteralPath $LogPath
    }
    return [ordered]@{
        id = $Id
        executable = ConvertTo-OWSPortableArgument -Value $FilePath
        argv = @($Arguments | ForEach-Object { ConvertTo-OWSPortableArgument -Value ([string]$_) })
        exit_code = $ProcessExitCode
        result = if ($ProcessExitCode -eq 0) { 'PASS' } else { 'FAIL' }
        duration_ms = [int64]$Stopwatch.ElapsedMilliseconds
        log_sha256 = Get-OWSSha256Text -Text $Sanitized
        evidence_ref = "command://$Id/log_sha256"
    }
}

function New-OWSBlockedCommand {
    param([string]$Id, [string]$FilePath, [string[]]$Arguments = @())
    return [ordered]@{
        id = $Id
        executable = ConvertTo-OWSPortableArgument -Value $FilePath
        argv = @($Arguments | ForEach-Object { ConvertTo-OWSPortableArgument -Value ([string]$_) })
        exit_code = $null
        result = 'BLOCKED'
        duration_ms = 0
        log_sha256 = $null
        evidence_ref = "command://$Id/not-run"
    }
}

function ConvertTo-OWSCommandResult {
    param($Command, [string]$PassDetail, [string]$FailureDetail)
    if ($Command.result -eq 'PASS') {
        return [ordered]@{ status = 'PASS'; detail = $PassDetail; evidence_refs = @("command://$($Command.id)") }
    }
    if ($Command.result -eq 'BLOCKED') {
        return [ordered]@{ status = 'BLOCKED'; detail = $FailureDetail; evidence_refs = @("command://$($Command.id)") }
    }
    return [ordered]@{ status = 'FAIL'; detail = $FailureDetail; evidence_refs = @("command://$($Command.id)") }
}

function Test-OWSSelectorReport {
    param([string]$ReportDirectory)

    $IndexPath = Join-Path $ReportDirectory 'index.json'
    if (-not (Test-Path -LiteralPath $IndexPath -PathType Leaf)) { return $false }
    $Data = Get-Content -Raw -LiteralPath $IndexPath | ConvertFrom-Json
    $Tests = @($Data.tests | Where-Object { $_.fullTestPath -like 'OWS.Selector.*' })
    $Expected = @(
        'OWS.Selector.DefaultStack',
        'OWS.Selector.SharedCharacterAttachment',
        'OWS.Selector.RuntimeReadout',
        'OWS.Selector.VehicleActivationRouting'
    )
    if ($Tests.Count -ne $Expected.Count) { return $false }
    foreach ($Name in $Expected) {
        $Match = @($Tests | Where-Object { $_.fullTestPath -eq $Name })
        if ($Match.Count -ne 1 -or $Match[0].state -ne 'Success') { return $false }
    }
    return $true
}

function Assert-OWSContainedPath {
    param(
        [Parameter(Mandatory)][string]$Parent,
        [Parameter(Mandatory)][string]$Child
    )

    $ParentFull = [System.IO.Path]::GetFullPath($Parent).TrimEnd('\', '/')
    $ChildFull = [System.IO.Path]::GetFullPath($Child)
    $Prefix = $ParentFull + [System.IO.Path]::DirectorySeparatorChar
    if (-not $ChildFull.StartsWith($Prefix, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Task-owned path escaped the controlled run root."
    }
    return $ChildFull
}

function Assert-OWSRunRootForDeletion {
    param([Parameter(Mandatory)][string]$RunRoot, [Parameter(Mandatory)][string]$ExternalOutputPath)

    $RunFull = [System.IO.Path]::GetFullPath($RunRoot).TrimEnd('\', '/')
    $ExpectedParent = [System.IO.Path]::GetFullPath([System.IO.Path]::GetDirectoryName($ExternalOutputPath)).TrimEnd('\', '/')
    $ActualParent = [System.IO.Path]::GetDirectoryName($RunFull).TrimEnd('\', '/')
    $Leaf = [System.IO.Path]::GetFileName($RunFull)
    if (-not $ActualParent.Equals($ExpectedParent, [System.StringComparison]::OrdinalIgnoreCase) -or
        $Leaf -notmatch '^\.ows-platform-baseline-[0-9a-f]{32}$') {
        throw 'Controlled run-root validation failed; recursive cleanup was refused.'
    }
    return $RunFull
}

function Protect-OWSGeneratedTaskFiles {
    param([switch]$IncludePackageArtifacts)

    if ([string]::IsNullOrWhiteSpace($script:TempRoot) -or -not (Test-Path -LiteralPath $script:TempRoot -PathType Container)) { return }
    $Encoding = New-Object System.Text.UTF8Encoding($false)
    $Roots = New-Object System.Collections.Generic.List[string]
    foreach ($Candidate in @(
        $script:LogsPath,
        (Join-Path $script:TempRoot 'uat-saved'),
        (Join-Path $script:TempRoot 'uat-logs'),
        (Join-Path $script:TempRoot 'process-temp'),
        $script:AssetRegistryDumpPath,
        $script:ContainerManifestPath,
        $script:SelectorReportPath
    )) {
        if (-not [string]::IsNullOrWhiteSpace($Candidate) -and (Test-Path -LiteralPath $Candidate -PathType Container)) { $Roots.Add($Candidate) }
    }
    if ($IncludePackageArtifacts) {
        foreach ($Candidate in @($script:StagePath, $script:ArchivePath)) {
            if (-not [string]::IsNullOrWhiteSpace($Candidate) -and (Test-Path -LiteralPath $Candidate -PathType Container)) { $Roots.Add($Candidate) }
        }
    }
    if (-not [string]::IsNullOrWhiteSpace($script:WorktreePath)) {
        foreach ($Relative in @('Saved', 'Intermediate')) {
            $Candidate = Join-Path $script:WorktreePath $Relative
            if (Test-Path -LiteralPath $Candidate -PathType Container) { $Roots.Add($Candidate) }
        }
    }
    foreach ($Root in @($Roots | Sort-Object -Unique)) {
        [void](Assert-OWSContainedPath -Parent $script:TempRoot -Child $Root)
        foreach ($File in Get-ChildItem -LiteralPath $Root -Recurse -File -ErrorAction Stop) {
            $IsTextLike = $File.Extension -match '^\.(?:log|txt|json|csv|xml|response|rsp|ini|config|yml|yaml|htm|html)$' -or $File.FullName -match '[\\/]ResponseFiles[\\/]'
            if (-not $IsTextLike) { continue }
            $Signature = "$($File.Length):$($File.LastWriteTimeUtc.Ticks)"
            if ($script:SanitizedTaskFiles.ContainsKey($File.FullName) -and $script:SanitizedTaskFiles[$File.FullName] -eq $Signature) { continue }
            $Text = Get-Content -Raw -LiteralPath $File.FullName -ErrorAction Stop
            $Sanitized = ConvertTo-OWSSanitizedText -Text $Text
            if ($Sanitized -cne $Text) { [System.IO.File]::WriteAllText($File.FullName, $Sanitized, $Encoding) }
            $Updated = Get-Item -LiteralPath $File.FullName
            $script:SanitizedTaskFiles[$File.FullName] = "$($Updated.Length):$($Updated.LastWriteTimeUtc.Ticks)"
        }
    }
    foreach ($SingleFile in @($script:AssetBaselinePath, $script:AssetSizeCsvPath)) {
        if ([string]::IsNullOrWhiteSpace($SingleFile) -or -not (Test-Path -LiteralPath $SingleFile -PathType Leaf)) { continue }
        [void](Assert-OWSContainedPath -Parent $script:TempRoot -Child $SingleFile)
        $File = Get-Item -LiteralPath $SingleFile
        $Signature = "$($File.Length):$($File.LastWriteTimeUtc.Ticks)"
        if ($script:SanitizedTaskFiles.ContainsKey($File.FullName) -and $script:SanitizedTaskFiles[$File.FullName] -eq $Signature) { continue }
        $Text = Get-Content -Raw -LiteralPath $File.FullName -ErrorAction Stop
        $Sanitized = ConvertTo-OWSSanitizedText -Text $Text
        if ($Sanitized -cne $Text) { [System.IO.File]::WriteAllText($File.FullName, $Sanitized, $Encoding) }
        $Updated = Get-Item -LiteralPath $File.FullName
        $script:SanitizedTaskFiles[$File.FullName] = "$($Updated.Length):$($Updated.LastWriteTimeUtc.Ticks)"
    }
}

function Get-OWSRelevantProcessSnapshot {
    $Rows = Get-CimInstance Win32_Process -ErrorAction Stop
    return @($Rows | Where-Object {
        $Name = [string]$_.Name
        $CommandLine = [string]$_.CommandLine
        $Name -match '^(?:UnrealEditor(?:-Cmd)?|ShaderCompileWorker|CrashReportClient(?:Editor)?|zenserver|UnrealTraceServer|mspdbsrv)\.exe$' -or
        ($Name -eq 'dotnet.exe' -and $CommandLine -match '(?:UnrealBuildTool|AutomationTool)') -or
        $Name -match '^(?:UbaAgent|UbaHost|UbaCli)\.exe$'
    } | ForEach-Object {
        [ordered]@{
            pid = [int]$_.ProcessId
            parent_pid = [int]$_.ParentProcessId
            name = [string]$_.Name
            executable_path = [string]$_.ExecutablePath
            command_line = [string]$_.CommandLine
            creation_date = [string]$_.CreationDate
        }
    } | Sort-Object pid)
}

function Set-OWSIsolatedProcessEnvironment {
    param([Parameter(Mandatory)][string]$RunRoot)

    $Paths = [ordered]@{
        'UE-LocalDataCachePath' = Assert-OWSContainedPath -Parent $RunRoot -Child (Join-Path $RunRoot 'ddc')
        'uebp_EngineSavedFolder' = Assert-OWSContainedPath -Parent $RunRoot -Child (Join-Path $RunRoot 'uat-saved')
        'uebp_LogFolder' = Assert-OWSContainedPath -Parent $RunRoot -Child (Join-Path $RunRoot 'uat-logs')
        'uebp_FinalLogFolder' = Assert-OWSContainedPath -Parent $RunRoot -Child (Join-Path $RunRoot 'uat-logs')
        'TEMP' = Assert-OWSContainedPath -Parent $RunRoot -Child (Join-Path $RunRoot 'process-temp')
        'TMP' = Assert-OWSContainedPath -Parent $RunRoot -Child (Join-Path $RunRoot 'process-temp')
    }
    foreach ($Path in @($Paths.Values | Sort-Object -Unique)) {
        [void][System.IO.Directory]::CreateDirectory($Path)
    }
    foreach ($Name in @($Paths.Keys)) {
        $script:EnvironmentBackup[$Name] = [Environment]::GetEnvironmentVariable($Name, 'Process')
        [Environment]::SetEnvironmentVariable($Name, [string]$Paths[$Name], 'Process')
    }
    foreach ($Name in @('UE-SharedDataCachePath', 'UE-ZenSharedDataCacheHost', 'UE-CloudDataCacheHost')) {
        $script:EnvironmentBackup[$Name] = [Environment]::GetEnvironmentVariable($Name, 'Process')
        [Environment]::SetEnvironmentVariable($Name, 'None', 'Process')
    }
    $script:EnvironmentIsolated = $true
}

function Restore-OWSProcessEnvironment {
    if (-not $script:EnvironmentIsolated) { return }
    foreach ($Name in @($script:EnvironmentBackup.Keys)) {
        [Environment]::SetEnvironmentVariable($Name, $script:EnvironmentBackup[$Name], 'Process')
    }
    $script:EnvironmentIsolated = $false
}

function Test-OWSAssetExportEvidence {
    param([Parameter(Mandatory)]$Command, [Parameter(Mandatory)][string]$Path, [Parameter(Mandatory)][string]$LogRoot)

    if ($Command.result -ne 'PASS' -or -not (Test-Path -LiteralPath $Path -PathType Leaf)) { return $false }
    $LogPath = Join-Path $LogRoot ($Command.id + '.log')
    if (-not (Test-Path -LiteralPath $LogPath -PathType Leaf) -or
        (Get-Content -Raw -LiteralPath $LogPath) -notmatch 'OWS_BASELINE\|PASS\|asset registry records=') { return $false }
    try {
        $Data = Get-Content -Raw -LiteralPath $Path | ConvertFrom-Json
        $ExpectedDependencyKinds = @('hard_package', 'manage_hard', 'manage_soft', 'searchable_name', 'soft_package')
        $ExpectedAssetRoles = @('blueprint_parent', 'data_layer', 'external_actor', 'external_object', 'hlod', 'map', 'world_partition')
        $ExpectedCapabilities = @($ExpectedDependencyKinds + $ExpectedAssetRoles | Sort-Object)
        $Capabilities = $Data.classification_capabilities
        if ($null -eq $Capabilities) { return $false }
        $CapabilityNames = @($Capabilities.PSObject.Properties.Name | Sort-Object)
        if (($CapabilityNames -join "`n") -cne ($ExpectedCapabilities -join "`n")) { return $false }
        foreach ($Name in $ExpectedCapabilities) {
            if (-not [bool]$Capabilities.$Name) { return $false }
        }
        if ([string]$Data.schema_version -ne '1.0.0' -or [int]$Data.asset_count -le 0 -or [int]$Data.asset_count -ne @($Data.assets).Count) {
            return $false
        }
        foreach ($Asset in @($Data.assets)) {
            if ([string]::IsNullOrWhiteSpace([string]$Asset.package_name)) { return $false }
            $DependencyNames = @($Asset.dependencies.PSObject.Properties.Name | Sort-Object)
            $RoleNames = @($Asset.classifications.PSObject.Properties.Name | Sort-Object)
            if (($DependencyNames -join "`n") -cne ($ExpectedDependencyKinds -join "`n") -or
                ($RoleNames -join "`n") -cne ($ExpectedAssetRoles -join "`n")) { return $false }
            foreach ($Name in @($ExpectedDependencyKinds + $ExpectedAssetRoles)) {
                $Container = if ($Name -in $ExpectedDependencyKinds) { $Asset.dependencies } else { $Asset.classifications }
                $Value = $Container.$Name
                if ($null -eq $Value -or $Value -is [string] -or $Value -isnot [System.Collections.IEnumerable]) { return $false }
            }
            $TagNames = @($Asset.registry_tags.PSObject.Properties.Name | Sort-Object)
            if (($TagNames -join "`n") -cne (@('generated_class', 'level_is_partitioned', 'native_parent_class', 'parent_class') -join "`n")) { return $false }
            $PartitionTag = ([string]$Asset.registry_tags.level_is_partitioned).ToLowerInvariant()
            if ($PartitionTag -in @('1', 'true') -and @($Asset.classifications.world_partition).Count -eq 0) { return $false }
        }
        return $true
    }
    catch { return $false }
}

function Test-OWSCsvHasData {
    param([Parameter(Mandatory)][string]$Path)
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf) -or (Get-Item -LiteralPath $Path).Length -le 0) { return $false }
    try { return @(Import-Csv -LiteralPath $Path).Count -gt 0 }
    catch { return $false }
}

function Test-OWSPackageEvidence {
    param(
        [Parameter(Mandatory)][string]$StageRoot,
        [Parameter(Mandatory)][string]$ArchiveRoot,
        [Parameter(Mandatory)][string]$MetadataRoot
    )

    $Missing = New-Object System.Collections.Generic.List[string]
    foreach ($Path in @($StageRoot, $ArchiveRoot)) {
        if (-not (Test-Path -LiteralPath $Path -PathType Container)) { $Missing.Add($Path) }
    }
    if ((Test-Path -LiteralPath $StageRoot -PathType Container) -and @(Get-ChildItem -LiteralPath $StageRoot -Recurse -File).Count -eq 0) { $Missing.Add((Join-Path $StageRoot '*')) }
    if ((Test-Path -LiteralPath $ArchiveRoot -PathType Container) -and @(Get-ChildItem -LiteralPath $ArchiveRoot -Recurse -File).Count -eq 0) { $Missing.Add((Join-Path $ArchiveRoot '*')) }
    foreach ($Name in @('DevelopmentAssetRegistry.bin', 'DevelopmentAssetRegistryStaged.bin', 'CookMetadata.ucookmeta', 'packagestore.manifest', 'plugin_sizes.csv', 'plugin_size_jsons.json')) {
        $Path = Join-Path $MetadataRoot $Name
        if (-not (Test-Path -LiteralPath $Path -PathType Leaf) -or (Get-Item -LiteralPath $Path).Length -le 0) { $Missing.Add($Path) }
    }
    foreach ($Extension in @('*.utoc', '*.ucas', '*.pak')) {
        if (-not (Test-Path -LiteralPath $ArchiveRoot -PathType Container) -or
            @(Get-ChildItem -LiteralPath $ArchiveRoot -Recurse -File -Filter $Extension).Count -eq 0) {
            $Missing.Add((Join-Path $ArchiveRoot $Extension))
        }
    }
    if (-not (Test-Path -LiteralPath (Join-Path $ArchiveRoot 'OWS.exe') -PathType Leaf)) { $Missing.Add((Join-Path $ArchiveRoot 'OWS.exe')) }
    return [pscustomobject]@{ valid = ($Missing.Count -eq 0); missing = @($Missing | ForEach-Object { $_ }) }
}

function Invoke-OWSPackageOutputOracle {
    param(
        [Parameter(Mandatory)][string]$StageRoot,
        [Parameter(Mandatory)][string]$ArchiveRoot,
        [Parameter(Mandatory)][string]$MetadataRoot
    )

    $Watch = [System.Diagnostics.Stopwatch]::StartNew()
    $Evidence = $null
    $Failure = $null
    try {
        $Evidence = Test-OWSPackageEvidence -StageRoot $StageRoot -ArchiveRoot $ArchiveRoot -MetadataRoot $MetadataRoot
    }
    catch {
        $Failure = ConvertTo-OWSSanitizedText -Text $_.Exception.Message
    }
    finally {
        $Watch.Stop()
    }

    $PortableFailure = if ($null -eq $Failure) { $null } else { ConvertTo-OWSPortableArgument -Value $Failure }
    $PortableMissing = if ($null -eq $Evidence) { @() } else { @($Evidence.missing | ForEach-Object { ConvertTo-OWSPortableArgument -Value ([string]$_) }) }
    $Passed = $null -eq $Failure -and [bool]$Evidence.valid
    $Detail = if ($null -ne $Failure) {
        'Package output oracle could not inspect every required output: ' + $PortableFailure
    }
    elseif ($Passed) {
        'Required stage, archive, container, executable, and cook-metadata outputs exist and are non-empty.'
    }
    else {
        "Package output oracle is missing $($PortableMissing.Count) required item(s)."
    }
    $SafeLog = $Detail + "`ninspection_failure=" + [string]$PortableFailure + "`n" + ($PortableMissing -join "`n")
    $Command = [ordered]@{
        id = 'package_output_oracle'
        executable = 'PowerShell'
        argv = @(
            'Test-OWSPackageEvidence',
            '-StageRoot', (ConvertTo-OWSPortableArgument -Value $StageRoot),
            '-ArchiveRoot', (ConvertTo-OWSPortableArgument -Value $ArchiveRoot),
            '-MetadataRoot', (ConvertTo-OWSPortableArgument -Value $MetadataRoot)
        )
        exit_code = if ($Passed) { 0 } elseif ($null -ne $Failure) { -1 } else { 1 }
        result = if ($Passed) { 'PASS' } else { 'FAIL' }
        duration_ms = [int64]$Watch.ElapsedMilliseconds
        log_sha256 = Get-OWSSha256Text -Text $SafeLog
        evidence_ref = 'command://package_output_oracle/log_sha256'
    }
    $Result = [ordered]@{
        status = if ($Passed) { 'PASS' } else { 'FAIL' }
        detail = $Detail
        missing = $PortableMissing
        inspection_failure = $PortableFailure
        evidence_refs = @('command://package_output_oracle', '#/inventories/stage', '#/inventories/archive', '#/inventories/cook_metadata')
    }
    return [pscustomobject]@{ command = $Command; result = $Result }
}

function Stop-OWSTaskOwnedProcesses {
    param(
        [object[]]$Before,
        [string]$RunRoot,
        [string]$WorktreeRoot
    )

    $BeforeIds = @($Before | ForEach-Object { [int]$_.pid })
    $Current = @(Get-OWSRelevantProcessSnapshot)
    $NewProcesses = @($Current | Where-Object { [int]$_.pid -notin $BeforeIds })
    $OwnedIds = New-Object 'System.Collections.Generic.HashSet[int]'
    $Unclear = New-Object System.Collections.Generic.List[string]

    foreach ($Process in $NewProcesses) {
        $Evidence = ([string]$Process.executable_path + "`n" + [string]$Process.command_line)
        $Owned = $Evidence.IndexOf($RunRoot, [System.StringComparison]::OrdinalIgnoreCase) -ge 0
        if (-not $Owned -and -not [string]::IsNullOrWhiteSpace($WorktreeRoot)) {
            $Owned = $Evidence.IndexOf($WorktreeRoot, [System.StringComparison]::OrdinalIgnoreCase) -ge 0
        }
        if ($Owned) { [void]$OwnedIds.Add([int]$Process.pid) }
    }

    $Changed = $true
    while ($Changed) {
        $Changed = $false
        foreach ($Process in $NewProcesses) {
            if ($OwnedIds.Contains([int]$Process.pid)) { continue }
            if ($OwnedIds.Contains([int]$Process.parent_pid)) {
                [void]$OwnedIds.Add([int]$Process.pid)
                $Changed = $true
            }
        }
    }

    foreach ($Process in $NewProcesses) {
        if ($OwnedIds.Contains([int]$Process.pid)) {
            Stop-Process -Id ([int]$Process.pid) -Force -ErrorAction Stop
            Wait-Process -Id ([int]$Process.pid) -Timeout 15 -ErrorAction SilentlyContinue
            if ($null -ne (Get-Process -Id ([int]$Process.pid) -ErrorAction SilentlyContinue)) {
                $Unclear.Add("$($Process.name) pid=$($Process.pid) remained alive after task-owned stop")
            }
        }
        else {
            $Unclear.Add("$($Process.name) pid=$($Process.pid) parent_pid=$($Process.parent_pid) lacks run-root, worktree, or task-owned-parent correlation")
        }
    }
    $RemainingIds = @(Get-OWSRelevantProcessSnapshot | Where-Object { [int]$_.pid -notin $BeforeIds } | ForEach-Object { [int]$_.pid })
    foreach ($RemainingId in $RemainingIds) {
        if (@($Unclear | Where-Object { $_ -match "pid=$RemainingId(?:\s|$)" }).Count -eq 0) { $Unclear.Add("relevant pid=$RemainingId remained after cleanup") }
    }
    return @($Unclear | ForEach-Object { $_ })
}

function ConvertTo-OWSComparableWorktreePath {
    param([Parameter(Mandatory)][string]$Path)

    $FullPath = [System.IO.Path]::GetFullPath($Path)
    if ([System.Environment]::OSVersion.Platform -eq [System.PlatformID]::Win32NT) {
        $SubstCommand = Get-Command subst.exe -ErrorAction SilentlyContinue
        $PathRoot = [System.IO.Path]::GetPathRoot($FullPath).TrimEnd('\')
        if ($null -ne $SubstCommand -and -not [string]::IsNullOrWhiteSpace($PathRoot)) {
            foreach ($Line in @(& $SubstCommand.Source 2>$null)) {
                if ([string]$Line -match '^([A-Za-z]:)\\: => (.+)$' -and
                    $Matches[1].Equals($PathRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
                    $RelativePath = $FullPath.Substring(([System.IO.Path]::GetPathRoot($FullPath)).Length)
                    $MappedRoot = [System.IO.Path]::GetFullPath($Matches[2].Trim())
                    $FullPath = if ([string]::IsNullOrWhiteSpace($RelativePath)) {
                        $MappedRoot
                    }
                    else {
                        [System.IO.Path]::GetFullPath((Join-Path $MappedRoot $RelativePath))
                    }
                    break
                }
            }
        }
    }
    return $FullPath.TrimEnd('\', '/').Replace('\', '/')
}

function Test-OWSRegisteredWorktree {
    param([Parameter(Mandatory)][string]$RepositoryRoot, [Parameter(Mandatory)][string]$WorktreeRoot)

    $Expected = ConvertTo-OWSComparableWorktreePath -Path $WorktreeRoot
    $Lines = @(& git -c core.longpaths=true -C $RepositoryRoot worktree list --porcelain 2>$null)
    if ($LASTEXITCODE -ne 0) { throw 'Unable to enumerate registered Git worktrees.' }
    foreach ($Line in $Lines) {
        if ([string]$Line -like 'worktree *') {
            $Actual = ConvertTo-OWSComparableWorktreePath -Path (([string]$Line).Substring(9).Trim())
            if ($Actual.Equals($Expected, [System.StringComparison]::OrdinalIgnoreCase)) { return $true }
        }
    }
    return $false
}

function Write-OWSReportChecksumSidecar {
    param([Parameter(Mandatory)][string]$ReportPath)

    $ResolvedReport = [System.IO.Path]::GetFullPath($ReportPath)
    $SidecarPath = $ResolvedReport + '.sha256'
    $Hash = Get-OWSFileSha256 -Path $ResolvedReport
    $Content = "$Hash`n"
    $Temporary = $SidecarPath + '.tmp-' + [guid]::NewGuid().ToString('N')
    try {
        [System.IO.File]::WriteAllText($Temporary, $Content, (New-Object System.Text.UTF8Encoding($false)))
        [System.IO.File]::Copy($Temporary, $SidecarPath, $true)
    }
    finally {
        if (Test-Path -LiteralPath $Temporary -PathType Leaf) { [System.IO.File]::Delete($Temporary) }
    }
    $WrittenContent = Get-Content -Raw -LiteralPath $SidecarPath
    if ($WrittenContent -cne $Content) { throw 'Report checksum sidecar verification failed.' }
    if ((ConvertTo-OWSSanitizedText -Text $WrittenContent) -cne $WrittenContent -or $WrittenContent -match '(?i)PRIVATE KEY|AUTHORIZATION|SIGNED') {
        throw 'Report checksum sidecar failed the delivery secret scan.'
    }
    return [pscustomobject]@{ path = $SidecarPath; sha256 = $Hash }
}

try {
    if ($StaticOnly) {
        $StaticWatch = [System.Diagnostics.Stopwatch]::StartNew()
        $Report = New-OWSStaticBaselineReport `
            -RepositoryRoot $ProjectRoot `
            -KnownGapsPath $KnownGapsPath `
            -Commands @() `
            -StartedUtc $RunStartedUtc `
            -StaticOnly
        $StaticWatch.Stop()
        $CommandRecords.Add([ordered]@{
            id = 'static_collectors'
            executable = 'PowerShell'
            argv = @('static baseline collectors')
            exit_code = 0
            result = 'PASS'
            duration_ms = [int64]$StaticWatch.ElapsedMilliseconds
            log_sha256 = Get-OWSSha256Text -Text 'Static collectors completed.'
            evidence_ref = 'command://static_collectors/log_sha256'
        })
        $Report.commands = @($CommandRecords | ForEach-Object { $_ })
        $Report.results.static['evidence_refs'] = @('command://static_collectors')
    }
    else {
        if (-not $OriginalPreState.clean) {
            throw 'A full baseline capture requires a clean source checkout. Use -StaticOnly for a working tree with changes.'
        }

        $ResolvedEngineRoot = Resolve-OWSEngineRoot -RequestedRoot $EngineRoot -RepositoryRoot $ProjectRoot
        $EngineVersion = Get-OWSEngineVersion -ResolvedEngineRoot $ResolvedEngineRoot
        $EngineBuildData = Get-Content -Raw -LiteralPath (Join-Path $ResolvedEngineRoot 'Engine\Build\Build.version') | ConvertFrom-Json
        $EngineChangelist = [long]$EngineBuildData.Changelist
        $CompatibleChangelist = [long]$EngineBuildData.CompatibleChangelist
        $InitialRelevantProcesses = @(Get-OWSRelevantProcessSnapshot)
        if ($InitialRelevantProcesses.Count -gt 0) {
            $ProcessNames = @($InitialRelevantProcesses.name | Sort-Object -Unique) -join ', '
            throw "A relevant Unreal/build process is already active ($ProcessNames); full capture is blocked to avoid a duplicate or ownership ambiguity."
        }
        $ScratchParent = [System.IO.Path]::GetDirectoryName($ResolvedOutputPath)
        [void][System.IO.Directory]::CreateDirectory($ScratchParent)
        $TempRoot = Join-Path $ScratchParent ('.ows-platform-baseline-' + [guid]::NewGuid().ToString('N'))
        [void][System.IO.Directory]::CreateDirectory($TempRoot)
        $WorktreePath = Assert-OWSContainedPath -Parent $TempRoot -Child (Join-Path $TempRoot 'worktree')
        $LogsPath = Assert-OWSContainedPath -Parent $TempRoot -Child (Join-Path $TempRoot 'logs')
        $AssetBaselinePath = Assert-OWSContainedPath -Parent $TempRoot -Child (Join-Path $TempRoot 'asset-registry.json')
        $SelectorReportPath = Assert-OWSContainedPath -Parent $TempRoot -Child (Join-Path $TempRoot 'selector-report')
        $StagePath = Assert-OWSContainedPath -Parent $TempRoot -Child (Join-Path $TempRoot 'stage')
        $ArchivePath = Assert-OWSContainedPath -Parent $TempRoot -Child (Join-Path $TempRoot 'archive')
        $AssetRegistryDumpPath = Assert-OWSContainedPath -Parent $TempRoot -Child (Join-Path $TempRoot 'cooked-asset-registry')
        $AssetSizeCsvPath = Assert-OWSContainedPath -Parent $TempRoot -Child (Join-Path $TempRoot 'asset-sizes.csv')
        $ContainerManifestPath = Assert-OWSContainedPath -Parent $TempRoot -Child (Join-Path $TempRoot 'container-manifests')
        [void][System.IO.Directory]::CreateDirectory($LogsPath)
        Set-OWSIsolatedProcessEnvironment -RunRoot $TempRoot

        $WorktreeAdd = Invoke-OWSRecordedCommand `
            -Id 'worktree_add' `
            -FilePath 'git' `
            -Arguments @('-c', 'core.longpaths=true', '-C', $ProjectRoot, 'worktree', 'add', '--detach', $WorktreePath, $OriginalPreState.commit) `
            -LogRoot $LogsPath
        $CommandRecords.Add($WorktreeAdd)
        $WorktreeAdded = Test-OWSRegisteredWorktree -RepositoryRoot $ProjectRoot -WorktreeRoot $WorktreePath
        if ($WorktreeAdded) { $WorktreeRemoved = $false }
        if ($WorktreeAdd.result -ne 'PASS' -or -not $WorktreeAdded -or -not (Test-Path -LiteralPath $WorktreePath -PathType Container)) {
            throw "Disposable worktree creation failed; exact_registration_observed=$WorktreeAdded."
        }

        $ProjectPath = Join-Path $WorktreePath 'OWS.uproject'
        $EditorCommand = Join-Path $ResolvedEngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
        $BuildCommand = Join-Path $ResolvedEngineRoot 'Engine\Build\BatchFiles\Build.bat'
        $UATCommand = Join-Path $ResolvedEngineRoot 'Engine\Build\BatchFiles\RunUAT.bat'
        $UnrealPakCommand = Join-Path $ResolvedEngineRoot 'Engine\Binaries\Win64\UnrealPak.exe'
        $PowerShellCommand = (Get-Command powershell.exe -ErrorAction Stop).Source

        $SelfTest = Invoke-OWSRecordedCommand -Id 'baseline_self_test' -FilePath $PowerShellCommand -Arguments @('-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', (Join-Path $WorktreePath 'Scripts\PlatformBaseline\TestOWSPlatformBaseline.ps1')) -LogRoot $LogsPath
        $CommandRecords.Add($SelfTest)
        $Lfs = Invoke-OWSRecordedCommand -Id 'lfs_fsck' -FilePath 'git' -Arguments @('-c', 'core.longpaths=true', '-C', $WorktreePath, 'lfs', 'fsck') -LogRoot $LogsPath
        $CommandRecords.Add($Lfs)

        if ($SelfTest.result -eq 'PASS' -and $Lfs.result -eq 'PASS') {
            $Build = Invoke-OWSRecordedCommand -Id 'build_editor' -FilePath $BuildCommand -Arguments @('OWSEditor', 'Win64', 'Development', "-Project=$ProjectPath", '-WaitMutex', '-NoHotReloadFromIDE', '-UTF8Output', '-NoLog') -LogRoot $LogsPath
        }
        else {
            $Build = New-OWSBlockedCommand -Id 'build_editor' -FilePath $BuildCommand -Arguments @('OWSEditor', 'Win64', 'Development')
        }
        $CommandRecords.Add($Build)

        if ($Build.result -eq 'PASS') {
            $ExporterPath = Join-Path $WorktreePath $AssetExporterRelativePath
            $AssetExport = Invoke-OWSRecordedCommand -Id 'asset_registry' -FilePath $EditorCommand -Arguments @($ProjectPath, '-run=pythonscript', "-script=$ExporterPath", "-OWSAssetBaselineOutput=$AssetBaselinePath", '-unattended', '-nop4', '-nosplash', '-nosound', '-nullrhi', '-notraceserver', '-nolog', '-stdout', '-FullStdOutLogOutput', '-UTF8Output') -LogRoot $LogsPath
            $CommandRecords.Add($AssetExport)
            if (-not (Test-OWSAssetExportEvidence -Command $AssetExport -Path $AssetBaselinePath -LogRoot $LogsPath)) { $AssetExport.result = 'FAIL' }
            $CleanStart = Invoke-OWSRecordedCommand -Id 'clean_start' -FilePath $PowerShellCommand -Arguments @('-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', (Join-Path $WorktreePath 'Scripts\RunOWSCleanStartSmoke.ps1'), '-EngineRoot', $ResolvedEngineRoot) -LogRoot $LogsPath
            $CommandRecords.Add($CleanStart)
            $Selector = Invoke-OWSRecordedCommand -Id 'selector_tests' -FilePath $EditorCommand -Arguments @($ProjectPath, '-ExecCmds=Automation RunTests OWS.Selector', '-TestExit=Automation Test Queue Empty', "-ReportExportPath=$SelectorReportPath", '-unattended', '-nop4', '-nosplash', '-nosound', '-RenderOffscreen', '-notraceserver', '-nolog', '-stdout', '-FullStdOutLogOutput', '-UTF8Output') -LogRoot $LogsPath
            $CommandRecords.Add($Selector)
            if ($Selector.result -eq 'PASS' -and -not (Test-OWSSelectorReport -ReportDirectory $SelectorReportPath)) { $Selector.result = 'FAIL' }
            $CharacterVehicle = Invoke-OWSRecordedCommand -Id 'character_vehicle_tests' -FilePath $PowerShellCommand -Arguments @('-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', (Join-Path $WorktreePath 'Scripts\RunOWSCharacterVehicleTests.ps1'), '-EngineRoot', $ResolvedEngineRoot) -LogRoot $LogsPath
            $CommandRecords.Add($CharacterVehicle)
            $UATArguments = @(
                'BuildCookRun', "-project=$ProjectPath", '-noP4', '-unattended', '-utf8output', '-NoCodeSign',
                '-target=OWS', '-platform=Win64', '-clientconfig=Development',
                '-build', '-clean', '-nocompileeditor', '-cook', '-stage', '-package', '-archive',
                '-pak', '-iostore', '-compressed', '-prereqs', '-manifests',
                "-stagingdirectory=$StagePath", "-archivedirectory=$ArchivePath",
                '-WriteBackMetadataToAssetRegistry=AdjacentFile',
                '-AdditionalIoStoreOptions=-WritePluginSizeSummaryJsons',
                '-AdditionalCookerOptions=-notraceserver'
            )
            $CookPackage = Invoke-OWSRecordedCommand -Id 'cook_package' -FilePath $UATCommand -Arguments $UATArguments -LogRoot $LogsPath
            $CommandRecords.Add($CookPackage)
            $StageWindowsPath = Join-Path $StagePath 'Windows'
            $ArchiveWindowsPath = Join-Path $ArchivePath 'Windows'
            $CookMetadataPath = Join-Path $WorktreePath 'Saved\Cooked\Windows\OWS\Metadata'
            if ($CookPackage.result -eq 'PASS') {
                $PackageCapture = Invoke-OWSPackageOutputOracle -StageRoot $StageWindowsPath -ArchiveRoot $ArchiveWindowsPath -MetadataRoot $CookMetadataPath
                $PackageOracle = $PackageCapture.command
                $PackageResult = $PackageCapture.result
            }
            else {
                $PackageOracle = New-OWSBlockedCommand -Id 'package_output_oracle' -FilePath 'PowerShell' -Arguments @('Test-OWSPackageEvidence')
                $PackageResult = [ordered]@{ status = 'BLOCKED'; detail = 'Package output oracle requires a successful BuildCookRun exit.'; missing = @(); evidence_refs = @('command://package_output_oracle') }
            }
            $CommandRecords.Add($PackageOracle)

            if ($CookPackage.result -eq 'PASS' -and $PackageResult.status -eq 'PASS') {
                [void][System.IO.Directory]::CreateDirectory($AssetRegistryDumpPath)
                [void][System.IO.Directory]::CreateDirectory($ContainerManifestPath)
                $StagedAssetRegistryPath = Join-Path $CookMetadataPath 'DevelopmentAssetRegistryStaged.bin'
                $CookedAssetRegistry = Invoke-OWSRecordedCommand -Id 'cooked_asset_registry_dump' -FilePath $EditorCommand -Arguments @($ProjectPath, '-run=DumpAssetRegistry', "-Path=$StagedAssetRegistryPath", "-OutDir=$AssetRegistryDumpPath", '-All', '-unattended', '-nop4', '-nosplash', '-nullrhi', '-notraceserver', '-nolog', '-stdout') -LogRoot $LogsPath
                $CommandRecords.Add($CookedAssetRegistry)
                if ($CookedAssetRegistry.result -eq 'PASS') {
                    $FirstPage = Join-Path $AssetRegistryDumpPath 'Page_00000.txt'
                    if (-not (Test-Path -LiteralPath $FirstPage -PathType Leaf) -or (Get-Item -LiteralPath $FirstPage).Length -le 0) { $CookedAssetRegistry.result = 'FAIL' }
                }
                $AssetSize = Invoke-OWSRecordedCommand -Id 'asset_size_query' -FilePath $EditorCommand -Arguments @($ProjectPath, '-run=AssetSizeQuery', "-AssetRegistry=$StagedAssetRegistryPath", '-Show=0', "-CSV=$AssetSizeCsvPath", '-CSVType=Assets', '-unattended', '-nop4', '-nosplash', '-nullrhi', '-notraceserver', '-nolog', '-stdout') -LogRoot $LogsPath
                $CommandRecords.Add($AssetSize)
                if ($AssetSize.result -eq 'PASS' -and -not (Test-OWSCsvHasData -Path $AssetSizeCsvPath)) { $AssetSize.result = 'FAIL' }

                $ContainerCommands = New-Object System.Collections.Generic.List[object]
                $ContainerIndex = 0
                foreach ($Container in @(Get-ChildItem -LiteralPath $ArchiveWindowsPath -Recurse -File -Filter '*.utoc' | Sort-Object FullName)) {
                    $CsvPath = Join-Path $ContainerManifestPath ("utoc-{0:D4}.csv" -f $ContainerIndex)
                    $Record = Invoke-OWSRecordedCommand -Id ("utoc_manifest_{0:D4}" -f $ContainerIndex) -FilePath $UnrealPakCommand -Arguments @("-ListContainer=$($Container.FullName)", "-CSV=$CsvPath") -LogRoot $LogsPath
                    $CommandRecords.Add($Record)
                    if ($Record.result -eq 'PASS' -and -not (Test-OWSCsvHasData -Path $CsvPath)) { $Record.result = 'FAIL' }
                    $ContainerCommands.Add($Record)
                    $ContainerIndex++
                }
                foreach ($Pak in @(Get-ChildItem -LiteralPath $ArchiveWindowsPath -Recurse -File -Filter '*.pak' | Sort-Object FullName)) {
                    $CsvPath = Join-Path $ContainerManifestPath ("pak-{0:D4}.csv" -f $ContainerIndex)
                    $Record = Invoke-OWSRecordedCommand -Id ("pak_manifest_{0:D4}" -f $ContainerIndex) -FilePath $UnrealPakCommand -Arguments @('-List', $Pak.FullName, "-CSV=$CsvPath") -LogRoot $LogsPath
                    $CommandRecords.Add($Record)
                    if ($Record.result -eq 'PASS' -and -not (Test-OWSCsvHasData -Path $CsvPath)) { $Record.result = 'FAIL' }
                    $ContainerCommands.Add($Record)
                    $ContainerIndex++
                }
                $ContainerEvidence = [ordered]@{
                    status = if ($ContainerCommands.Count -gt 0 -and @($ContainerCommands | Where-Object { $_.result -ne 'PASS' }).Count -eq 0) { 'PASS' } else { 'FAIL' }
                    detail = "UnrealPak emitted $($ContainerCommands.Count) non-empty container manifest(s)."
                    evidence_refs = @(@($ContainerCommands | ForEach-Object { "command://$($_.id)" }) + '#/inventories/container_manifests')
                }
            }
            else {
                $CookedAssetRegistry = New-OWSBlockedCommand -Id 'cooked_asset_registry_dump' -FilePath $EditorCommand -Arguments @('-run=DumpAssetRegistry')
                $AssetSize = New-OWSBlockedCommand -Id 'asset_size_query' -FilePath $EditorCommand -Arguments @('-run=AssetSizeQuery')
                $CommandRecords.Add($CookedAssetRegistry)
                $CommandRecords.Add($AssetSize)
                $ContainerEvidence = [ordered]@{ status = 'BLOCKED'; detail = 'Container manifests require a verified package archive.'; evidence_refs = @('command://package_output_oracle') }
            }
        }
        else {
            $AssetExport = New-OWSBlockedCommand -Id 'asset_registry' -FilePath $EditorCommand -Arguments @('-run=pythonscript')
            $CommandRecords.Add($AssetExport)
            $CleanStart = New-OWSBlockedCommand -Id 'clean_start' -FilePath $PowerShellCommand -Arguments @('-File', 'RunOWSCleanStartSmoke.ps1')
            $CommandRecords.Add($CleanStart)
            $Selector = New-OWSBlockedCommand -Id 'selector_tests' -FilePath $EditorCommand -Arguments @('-ExecCmds=Automation RunTests OWS.Selector')
            $CommandRecords.Add($Selector)
            $CharacterVehicle = New-OWSBlockedCommand -Id 'character_vehicle_tests' -FilePath $PowerShellCommand -Arguments @('-File', 'RunOWSCharacterVehicleTests.ps1')
            $CommandRecords.Add($CharacterVehicle)
            $CookPackage = New-OWSBlockedCommand -Id 'cook_package' -FilePath $UATCommand -Arguments @('BuildCookRun')
            $CommandRecords.Add($CookPackage)
            $PackageOracle = New-OWSBlockedCommand -Id 'package_output_oracle' -FilePath 'PowerShell' -Arguments @('Test-OWSPackageEvidence')
            $CommandRecords.Add($PackageOracle)
            $PackageResult = [ordered]@{ status = 'BLOCKED'; detail = 'Package output oracle requires a successful editor build and BuildCookRun.'; missing = @(); evidence_refs = @('command://package_output_oracle') }
            $CookedAssetRegistry = New-OWSBlockedCommand -Id 'cooked_asset_registry_dump' -FilePath $EditorCommand -Arguments @('-run=DumpAssetRegistry')
            $CommandRecords.Add($CookedAssetRegistry)
            $AssetSize = New-OWSBlockedCommand -Id 'asset_size_query' -FilePath $EditorCommand -Arguments @('-run=AssetSizeQuery')
            $CommandRecords.Add($AssetSize)
            $ContainerEvidence = [ordered]@{ status = 'BLOCKED'; detail = 'Container manifests require a verified package archive.'; evidence_refs = @('command://package_output_oracle') }
            $StageWindowsPath = $null
            $ArchiveWindowsPath = $null
            $CookMetadataPath = $null
        }
        $Results = [ordered]@{
            self_test = ConvertTo-OWSCommandResult -Command $SelfTest -PassDetail 'The non-Unreal baseline guardrail self-test passed in the exact disposable checkout.' -FailureDetail 'The baseline guardrail self-test did not pass.'
            static = [ordered]@{ status = 'PASS'; detail = 'Static baseline collectors completed.'; evidence_refs = @('command://static_collectors') }
            lfs = ConvertTo-OWSCommandResult -Command $Lfs -PassDetail 'Git LFS object integrity passed.' -FailureDetail 'Git LFS integrity did not pass.'
            build = ConvertTo-OWSCommandResult -Command $Build -PassDetail 'OWSEditor Win64 Development built.' -FailureDetail 'OWSEditor build did not pass.'
            asset_registry = ConvertTo-OWSCommandResult -Command $AssetExport -PassDetail 'Read-only Asset Registry export completed.' -FailureDetail 'Asset Registry export did not pass.'
            clean_start = ConvertTo-OWSCommandResult -Command $CleanStart -PassDetail 'Canonical clean-start smoke passed.' -FailureDetail 'Canonical clean-start smoke did not pass.'
            selector = ConvertTo-OWSCommandResult -Command $Selector -PassDetail 'Selector automation passed.' -FailureDetail 'Selector automation did not pass.'
            character_vehicle = ConvertTo-OWSCommandResult -Command $CharacterVehicle -PassDetail 'Character/Vehicle automation passed.' -FailureDetail 'Character/Vehicle automation did not pass.'
            cook = ConvertTo-OWSCommandResult -Command $CookPackage -PassDetail 'BuildCookRun, including the Win64 Development cook phase, returned exit code 0; package outputs are assessed separately.' -FailureDetail 'BuildCookRun did not return exit code 0; no cook pass is claimed.'
            package = $PackageResult
            cooked_asset_registry = ConvertTo-OWSCommandResult -Command $CookedAssetRegistry -PassDetail 'Cooked Asset Registry dump completed with a non-empty first page.' -FailureDetail 'Cooked Asset Registry dump did not pass.'
            asset_sizes = ConvertTo-OWSCommandResult -Command $AssetSize -PassDetail 'AssetSizeQuery emitted a non-empty CSV.' -FailureDetail 'AssetSizeQuery did not pass.'
            container_manifests = $ContainerEvidence
            packaged_launch = [ordered]@{ status = 'BLOCKED'; detail = 'No deterministic packaged-launch oracle exists yet; issue #17 owns the required future launch evidence.'; owner_issue = 17; owner_url = 'https://github.com/GameForgeStudio/Unreal-Open-World-Starter/issues/17'; evidence_refs = @('https://github.com/GameForgeStudio/Unreal-Open-World-Starter/issues/17') }
        }
        $Results.asset_registry['evidence_refs'] = @($Results.asset_registry.evidence_refs + '#/inventories/asset_registry')
        $Results.cooked_asset_registry['evidence_refs'] = @($Results.cooked_asset_registry.evidence_refs + '#/inventories/cooked_asset_registry_dump')
        $Results.asset_sizes['evidence_refs'] = @($Results.asset_sizes.evidence_refs + '#/inventories/asset_sizes')

        $FullStaticWatch = [System.Diagnostics.Stopwatch]::StartNew()
        $Report = New-OWSStaticBaselineReport `
            -RepositoryRoot $WorktreePath `
            -KnownGapsPath (Join-Path $WorktreePath 'Scripts\PlatformBaseline\known-gaps.json') `
            -AssetBaselinePath $AssetBaselinePath `
            -StageRoot $StageWindowsPath `
            -ArchiveRoot $ArchiveWindowsPath `
            -CookMetadataRoot $CookMetadataPath `
            -AssetRegistryDumpRoot $AssetRegistryDumpPath `
            -AssetSizeCsvPath $AssetSizeCsvPath `
            -ContainerManifestRoot $ContainerManifestPath `
            -Commands @($CommandRecords | ForEach-Object { $_ }) `
            -Results $Results `
            -StartedUtc $RunStartedUtc `
            -EngineRootLabel $ResolvedEngineRoot `
            -EngineVersion $EngineVersion `
            -EngineChangelist $EngineChangelist `
            -CompatibleChangelist $CompatibleChangelist
        $FullStaticWatch.Stop()
        $CommandRecords.Add([ordered]@{
            id = 'static_collectors'
            executable = 'PowerShell'
            argv = @('static baseline collectors', '-RepositoryRoot', '$WORKTREE')
            exit_code = 0
            result = 'PASS'
            duration_ms = [int64]$FullStaticWatch.ElapsedMilliseconds
            log_sha256 = Get-OWSSha256Text -Text 'Static collectors completed in the disposable exact-commit checkout.'
            evidence_ref = 'command://static_collectors/log_sha256'
        })
        $Report.source = $OriginalPreState
        $Report.source.checkout_mode = 'detached-disposable-worktree-at-invoking-commit'
    }
}
catch {
    $FatalMessage = ConvertTo-OWSSanitizedText -Text ($_.Exception.GetType().FullName + ': ' + $_.Exception.Message)
}
finally {
    if ($null -ne $TempRoot) {
        try {
            $UnclearProcesses = @(Stop-OWSTaskOwnedProcesses -Before $InitialRelevantProcesses -RunRoot $TempRoot -WorktreeRoot $WorktreePath)
            if ($UnclearProcesses.Count -gt 0) {
                throw ('New relevant processes could not be proven task-owned: ' + ($UnclearProcesses -join ', '))
            }
        }
        catch {
            $CleanupPassed = $false
            $CleanupIssue = ConvertTo-OWSSanitizedText -Text $_.Exception.Message
        }
        try { Protect-OWSGeneratedTaskFiles -IncludePackageArtifacts }
        catch {
            $CleanupPassed = $false
            $CleanupIssue = ConvertTo-OWSSanitizedText -Text $_.Exception.Message
        }
    }
    $RegisteredWorktreeAtCleanup = $false
    if (-not [string]::IsNullOrWhiteSpace($WorktreePath)) {
        try {
            [void](Assert-OWSContainedPath -Parent $TempRoot -Child $WorktreePath)
            $RegisteredWorktreeAtCleanup = Test-OWSRegisteredWorktree -RepositoryRoot $ProjectRoot -WorktreeRoot $WorktreePath
        }
        catch {
            $WorktreeRemoved = $false
            $CleanupPassed = $false
            $CleanupIssue = ConvertTo-OWSSanitizedText -Text $_.Exception.Message
        }
    }
    if ($RegisteredWorktreeAtCleanup) {
        try {
            $RemovalLogRoot = Join-Path $TempRoot 'logs'
            $RemoveRecord = Invoke-OWSRecordedCommand -Id 'worktree_remove' -FilePath 'git' -Arguments @('-c', 'core.longpaths=true', '-C', $ProjectRoot, 'worktree', 'remove', '--force', '--', $WorktreePath) -LogRoot $RemovalLogRoot
            $CommandRecords.Add($RemoveRecord)
            $WorktreeRemoved = (
                $RemoveRecord.result -eq 'PASS' -and
                -not (Test-Path -LiteralPath $WorktreePath) -and
                -not (Test-OWSRegisteredWorktree -RepositoryRoot $ProjectRoot -WorktreeRoot $WorktreePath)
            )
            if (-not $WorktreeRemoved) { throw 'The exact disposable worktree was not fully removed.' }
        }
        catch {
            $WorktreeRemoved = $false
            $CleanupPassed = $false
            $CleanupIssue = ConvertTo-OWSSanitizedText -Text $_.Exception.Message
        }
    }
    elseif ($CleanupPassed) {
        $WorktreeRemoved = $true
    }
    try {
        Restore-OWSProcessEnvironment
    }
    catch {
        $CleanupPassed = $false
        $CleanupIssue = ConvertTo-OWSSanitizedText -Text $_.Exception.Message
    }
    if ($null -ne $TempRoot -and (Test-Path -LiteralPath $TempRoot -PathType Container) -and $WorktreeRemoved) {
        try {
            $ValidatedRunRoot = Assert-OWSRunRootForDeletion -RunRoot $TempRoot -ExternalOutputPath $ResolvedOutputPath
            Get-ChildItem -LiteralPath $TempRoot -Force -Recurse -ErrorAction SilentlyContinue | ForEach-Object { $_.Attributes = 'Normal' }
            [System.IO.Directory]::Delete($ValidatedRunRoot, $true)
            if (Test-Path -LiteralPath $ValidatedRunRoot) { throw 'Controlled run root still exists after cleanup.' }
        }
        catch {
            $CleanupPassed = $false
            $CleanupIssue = ConvertTo-OWSSanitizedText -Text $_.Exception.Message
        }
    }
}

try {
    if ($null -eq $Report) {
        $FallbackSelfTestResult = if ($null -eq $SelfTest) {
            [ordered]@{ status = if ($StaticOnly) { 'NR' } else { 'BLOCKED' }; detail = 'The full baseline did not reach its standalone guardrail self-test.' }
        }
        else {
            ConvertTo-OWSCommandResult -Command $SelfTest -PassDetail 'The non-Unreal baseline guardrail self-test passed.' -FailureDetail 'The baseline guardrail self-test did not pass.'
        }
        $FallbackResults = [ordered]@{
            self_test = $FallbackSelfTestResult
            static = [ordered]@{ status = 'PASS'; detail = 'Fallback static baseline collectors completed; the attempted full capture remains invalid.' }
            lfs = [ordered]@{ status = 'BLOCKED'; detail = 'Full baseline orchestration failed.' }
            build = [ordered]@{ status = 'BLOCKED'; detail = 'Full baseline orchestration failed.' }
            asset_registry = [ordered]@{ status = 'BLOCKED'; detail = 'Full baseline orchestration failed.' }
            clean_start = [ordered]@{ status = 'BLOCKED'; detail = 'Full baseline orchestration failed.' }
            selector = [ordered]@{ status = 'BLOCKED'; detail = 'Full baseline orchestration failed.' }
            character_vehicle = [ordered]@{ status = 'BLOCKED'; detail = 'Full baseline orchestration failed.' }
            cook = [ordered]@{ status = 'BLOCKED'; detail = 'Full baseline orchestration failed.' }
            package = [ordered]@{ status = 'BLOCKED'; detail = 'Full baseline orchestration failed before the package-output oracle passed.'; missing = @() }
            cooked_asset_registry = [ordered]@{ status = 'BLOCKED'; detail = 'Full baseline orchestration failed.' }
            asset_sizes = [ordered]@{ status = 'BLOCKED'; detail = 'Full baseline orchestration failed.' }
            container_manifests = [ordered]@{ status = 'BLOCKED'; detail = 'Full baseline orchestration failed.' }
            packaged_launch = [ordered]@{ status = 'BLOCKED'; detail = 'No deterministic packaged-launch oracle exists yet; issue #17 owns the required future launch evidence.'; owner_issue = 17; owner_url = 'https://github.com/GameForgeStudio/Unreal-Open-World-Starter/issues/17' }
        }
        $FallbackStaticWatch = [System.Diagnostics.Stopwatch]::StartNew()
        $Report = New-OWSStaticBaselineReport -RepositoryRoot $ProjectRoot -KnownGapsPath $KnownGapsPath -Commands @($CommandRecords | ForEach-Object { $_ }) -Results $FallbackResults -StartedUtc $RunStartedUtc -StaticOnly:$StaticOnly
        $FallbackStaticWatch.Stop()
        if (@($CommandRecords | Where-Object { $_.id -eq 'static_collectors' }).Count -eq 0) {
            $CommandRecords.Add([ordered]@{
                id = 'static_collectors'
                executable = 'PowerShell'
                argv = @('static baseline collectors', '-RepositoryRoot', '$REPOSITORY', '-fallback-after-full-attempt')
                exit_code = 0
                result = 'PASS'
                duration_ms = [int64]$FallbackStaticWatch.ElapsedMilliseconds
                log_sha256 = Get-OWSSha256Text -Text 'Fallback static collectors completed.'
                evidence_ref = 'command://static_collectors/log_sha256'
            })
        }
        $Report.results.static['evidence_refs'] = @('command://static_collectors')
        $Report.summary.capture_status = 'INVALID'
    }

    $OriginalPostState = Get-OWSRepositoryState -RepositoryRoot $ProjectRoot
    $Unchanged = Compare-OWSRepositoryState -Before $OriginalPreState -After $OriginalPostState
    $Report.commands = @($CommandRecords | ForEach-Object { $_ })
    $Report.finished_utc = [DateTime]::UtcNow.ToString('yyyy-MM-ddTHH:mm:ssZ')
    $Report.generated_utc = $Report.finished_utc
    $Report.immutability = [ordered]@{
        pre_digest = $OriginalPreState.status_digest
        post_digest = $OriginalPostState.status_digest
        tracked_mutations = if ($Unchanged) { 0 } else { 1 }
        worktree_removed = [bool]$WorktreeRemoved
        cleanup_passed = [bool]($CleanupPassed -and $Unchanged -and $WorktreeRemoved)
        cleanup_issue = if ([string]::IsNullOrWhiteSpace($CleanupIssue)) { $null } else { ConvertTo-OWSPortableArgument -Value $CleanupIssue }
    }
    if (-not $Report.immutability.cleanup_passed -or $null -ne $FatalMessage) {
        $Report.summary.capture_status = 'INVALID'
    }

    $Report.results['delivery'] = [ordered]@{
        status = 'PASS'
        detail = 'The self-contained JSON report is delivered with a sibling SHA-256 sidecar.'
        report_path = $ResolvedOutputPath
        checksum_sidecar_path = $ResolvedOutputPath + '.sha256'
        embedded_evidence_references = @($Report.artifacts | ForEach-Object { $_.path })
        evidence_refs = @('delivery://report', 'delivery://sha256-sidecar')
    }

    $Validation = Test-OWSBaselineReport -Report $Report -RepositoryRoot $ProjectRoot
    if (-not $Validation.valid) {
        $Report.summary.capture_status = 'INVALID'
        throw ('Report validation failed: ' + ($Validation.errors -join '; '))
    }
    [void](Write-OWSBaselineReport -Report $Report -OutputPath $ResolvedOutputPath)
    $DeliveredJson = Get-Content -Raw -LiteralPath $ResolvedOutputPath
    if ((ConvertTo-OWSSanitizedText -Text $DeliveredJson) -cne $DeliveredJson -or
        $DeliveredJson -match '(?i)-----BEGIN [^-\r\n]*PRIVATE KEY-----|-----END [^-\r\n]*PRIVATE KEY-----') {
        throw 'Delivered report failed the final credential/signed-URL/private-key scan.'
    }
    $Delivery = Write-OWSReportChecksumSidecar -ReportPath $ResolvedOutputPath

    $FailedResults = @($Report.results.GetEnumerator() | Where-Object {
        $_.Value.status -in @('FAIL', 'BLOCKED') -and
        -not ($_.Key -eq 'packaged_launch' -and $_.Value.status -eq 'BLOCKED' -and [int]$_.Value.owner_issue -eq 17)
    })
    if ($Report.summary.capture_status -eq 'VALID' -and
        $Report.summary.new_unowned_gap_count -eq 0 -and
        $FailedResults.Count -eq 0) {
        [Console]::WriteLine("[OWS baseline] PASS: report and SHA-256 sidecar written; checksum $($Delivery.sha256); known gaps $($Report.summary.known_gap_count), observations $($Report.summary.observation_count), no repository mutation.")
        $ExitCode = 0
    }
    else {
        $Reason = if ($null -ne $FatalMessage) { $FatalMessage } elseif ($null -ne $CleanupIssue) { $CleanupIssue } else { 'The report records a failed/blocked step, an unowned finding, or a cleanup failure.' }
        [Console]::Error.WriteLine('[OWS baseline] FAIL: ' + $Reason)
    }
}
catch {
    [Console]::Error.WriteLine('[OWS baseline] FAIL: ' + (ConvertTo-OWSSanitizedText -Text $_.Exception.Message))
}

exit $ExitCode

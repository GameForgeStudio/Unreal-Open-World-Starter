#requires -Version 7.4

[CmdletBinding()]
param(
    [ValidateNotNullOrEmpty()]
    [string] $EngineRoot = 'C:\UE_5.8',

    [ValidateRange(0, [uint32]::MaxValue)]
    [uint32] $Seed = 1396788053,

    [ValidateNotNullOrEmpty()]
    [string] $ScratchRoot = (Join-Path ([IO.Path]::GetTempPath()) 'SakuraOpenWorldLab')
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$ProjectRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$EngineRoot = [IO.Path]::GetFullPath($EngineRoot)
$ProjectFile = Join-Path $ProjectRoot 'SakuraTactics.uproject'
$BuildBat = Join-Path $EngineRoot 'Engine\Build\BatchFiles\Build.bat'
$Editor = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor.exe'
$EditorCmd = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$BuildVersionFile = Join-Path $EngineRoot 'Engine\Build\Build.version'
$SourcePackage = '/Game/SakuraPrototype/Maps/L_SakuraTestLab'
$TargetPackage = '/Game/SakuraPrototype/Maps/L_SakuraTestLab_WP'
$SourceMap = Join-Path $ProjectRoot 'Content\SakuraPrototype\Maps\L_SakuraTestLab.umap'
$TargetMap = Join-Path $ProjectRoot 'Content\SakuraPrototype\Maps\L_SakuraTestLab_WP.umap'
$DefaultEngineIni = Join-Path $ProjectRoot 'Config\DefaultEngine.ini'
$LevelIni = Join-Path $ProjectRoot 'Content\SakuraPrototype\Maps\L_SakuraTestLab.ini'
$ExternalActors = Join-Path $ProjectRoot 'Content\__ExternalActors__\SakuraPrototype\Maps\L_SakuraTestLab_WP'
$ExternalObjects = Join-Path $ProjectRoot 'Content\__ExternalObjects__\SakuraPrototype\Maps\L_SakuraTestLab_WP'
$DataLayers = Join-Path $ProjectRoot 'Content\DataLayers\L_SakuraTestLab_WP'
$GeneratedAssets = Join-Path $ProjectRoot 'Content\SakuraPrototype\Terrain\OpenWorldGenerated'
$HLODInstanced = Join-Path $ProjectRoot 'Content\SakuraPrototype\Maps\L_SakuraTestLab_HLODLayer_Instanced.uasset'
$HLODMerged = Join-Path $ProjectRoot 'Content\SakuraPrototype\Maps\L_SakuraTestLab_HLODLayer_Merged.uasset'
$TargetHLODInstanced = Join-Path $ProjectRoot 'Content\SakuraPrototype\Maps\L_SakuraTestLab_WP_HLODLayer_Instanced.uasset'
$TargetHLODMerged = Join-Path $ProjectRoot 'Content\SakuraPrototype\Maps\L_SakuraTestLab_WP_HLODLayer_Merged.uasset'
$ContentRoot = [IO.Path]::GetFullPath((Join-Path $ProjectRoot 'Content'))
$MapDirectory = Split-Path -Parent $TargetMap
$PluginsRoot = Join-Path $ProjectRoot 'Plugins'
$BuildOutputRoots = @(
    (Join-Path $ProjectRoot 'Binaries')
    (Join-Path $ProjectRoot 'Intermediate')
)

$KnownOutputs = @(
    $TargetMap,
    $ExternalActors,
    $ExternalObjects,
    $DataLayers,
    $GeneratedAssets,
    $HLODInstanced,
    $HLODMerged,
    $TargetHLODInstanced,
    $TargetHLODMerged,
    $LevelIni
)

function Resolve-SafeContainedPath {
    param(
        [Parameter(Mandatory)][string] $Path,
        [Parameter(Mandatory)][string] $AllowedRoot,
        [Parameter(Mandatory)][string] $Label
    )

    $resolved = [IO.Path]::GetFullPath($Path)
    $allowed = [IO.Path]::GetFullPath($AllowedRoot).TrimEnd(
        [IO.Path]::DirectorySeparatorChar,
        [IO.Path]::AltDirectorySeparatorChar)
    $isContained = $resolved.Equals($allowed, [StringComparison]::OrdinalIgnoreCase) -or
        $resolved.StartsWith($allowed + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)
    if (-not $isContained) {
        throw "$Label escaped its allowed root '$allowed': $resolved"
    }

    # Lexical prefix checks are not sufficient for junctions/symlinks. Refuse
    # every existing reparse point in the path chain, including ancestors of an
    # as-yet-uncreated target, so deletion and rollback never traverse one.
    $cursor = $resolved
    while (-not [string]::IsNullOrEmpty($cursor)) {
        $exists = Test-Path -LiteralPath $cursor -ErrorAction Stop
        if ($exists) {
            $item = Get-Item -LiteralPath $cursor -Force -ErrorAction Stop
            if (($item.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
                throw "$Label contains a reparse point and is not safe to mutate: $($item.FullName)"
            }
        }

        $parent = [IO.Path]::GetDirectoryName($cursor)
        if ([string]::IsNullOrEmpty($parent) -or $parent.Equals($cursor, [StringComparison]::OrdinalIgnoreCase)) {
            break
        }
        $cursor = $parent
    }

    return $resolved
}

function Resolve-SafeBuildOutputPath {
    param(
        [Parameter(Mandatory)][string] $Path,
        [Parameter(Mandatory)][string] $Label
    )

    $resolved = [IO.Path]::GetFullPath($Path)
    foreach ($allowedRoot in @($ProjectRoot, $EngineRoot)) {
        $root = [IO.Path]::GetFullPath($allowedRoot).TrimEnd(
            [IO.Path]::DirectorySeparatorChar,
            [IO.Path]::AltDirectorySeparatorChar)
        if ($resolved.Equals($root, [StringComparison]::OrdinalIgnoreCase)) {
            throw "$Label may not target the project or engine root itself: $resolved"
        }
        if ($resolved.StartsWith($root + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) {
            return Resolve-SafeContainedPath -Path $resolved -AllowedRoot $root -Label $Label
        }
    }

    throw "$Label is outside both the project and selected EngineRoot: $resolved"
}

function Get-BuildOutputRoots {
    $roots = [Collections.Generic.List[string]]::new()
    foreach ($root in $BuildOutputRoots) {
        $roots.Add((Resolve-SafeBuildOutputPath -Path $root -Label 'Build-output path'))
    }

    $safePluginsRoot = Resolve-SafeContainedPath -Path $PluginsRoot -AllowedRoot $ProjectRoot -Label 'Plugins directory'
    $projectPluginNames = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
    if (Test-Path -LiteralPath $safePluginsRoot -PathType Container) {
        $pluginItems = @(Get-ChildItem -LiteralPath $safePluginsRoot -Recurse -Force -ErrorAction Stop)
        $pluginReparsePoint = $pluginItems | Where-Object {
            ($_.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0
        } | Select-Object -First 1
        if ($null -ne $pluginReparsePoint) {
            throw "Plugins tree contains a reparse point and cannot be checkpointed safely: $($pluginReparsePoint.FullName)"
        }
        foreach ($pluginDescriptor in $pluginItems | Where-Object { -not $_.PSIsContainer -and $_.Extension -ieq '.uplugin' }) {
            [void] (Resolve-SafeContainedPath -Path $pluginDescriptor.FullName -AllowedRoot $safePluginsRoot -Label 'Plugin descriptor')
            [void] $projectPluginNames.Add($pluginDescriptor.BaseName)
            $pluginDirectory = Split-Path -Parent $pluginDescriptor.FullName
            $roots.Add((Resolve-SafeBuildOutputPath -Path (Join-Path $pluginDirectory 'Binaries') -Label 'Plugin build-output path'))
            $roots.Add((Resolve-SafeBuildOutputPath -Path (Join-Path $pluginDirectory 'Intermediate') -Label 'Plugin build-output path'))
        }
    }

    # Explicitly enabled engine plugins can receive locally compiled binaries
    # and intermediates even though they live outside the project tree. Include
    # their own output roots in the same exact checkpoint/rollback transaction.
    $projectPluginConfiguration = Get-Content -LiteralPath $ProjectFile -Raw -ErrorAction Stop | ConvertFrom-Json
    $enabledPluginNames = @(
        $projectPluginConfiguration.Plugins |
            Where-Object { $_.Enabled -eq $true } |
            ForEach-Object { [string] $_.Name }
    )
    $enginePluginsRoot = Resolve-SafeContainedPath -Path (Join-Path $EngineRoot 'Engine\Plugins') -AllowedRoot $EngineRoot -Label 'Engine plugins directory'
    if (Test-Path -LiteralPath $enginePluginsRoot -PathType Container) {
        $engineReparsePoint = Get-ChildItem -LiteralPath $enginePluginsRoot -Recurse -Force -Attributes ReparsePoint -ErrorAction Stop |
            Select-Object -First 1
        if ($null -ne $engineReparsePoint) {
            throw "Engine plugins tree contains a reparse point and cannot be checkpointed safely: $($engineReparsePoint.FullName)"
        }

        $enginePluginDescriptors = @(
            Get-ChildItem -LiteralPath $enginePluginsRoot -Recurse -File -Filter '*.uplugin' -Force -ErrorAction Stop
        )
        foreach ($pluginName in $enabledPluginNames) {
            if ($projectPluginNames.Contains($pluginName)) {
                continue
            }
            $matches = @($enginePluginDescriptors | Where-Object { $_.BaseName -ieq $pluginName })
            if ($matches.Count -ne 1) {
                throw "Could not uniquely resolve enabled engine plugin '$pluginName' (matches=$($matches.Count))."
            }
            $pluginDescriptor = $matches[0]
            [void] (Resolve-SafeContainedPath -Path $pluginDescriptor.FullName -AllowedRoot $enginePluginsRoot -Label 'Enabled engine-plugin descriptor')
            $pluginDirectory = Split-Path -Parent $pluginDescriptor.FullName
            $roots.Add((Resolve-SafeBuildOutputPath -Path (Join-Path $pluginDirectory 'Binaries') -Label 'Enabled engine-plugin build-output path'))
            $roots.Add((Resolve-SafeBuildOutputPath -Path (Join-Path $pluginDirectory 'Intermediate') -Label 'Enabled engine-plugin build-output path'))
        }
    }

    @($roots | Sort-Object -Unique)
}

function Assert-RequiredFile {
    param([Parameter(Mandatory)][string] $Path)
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        throw "Required file is missing: $Path"
    }
}

function Assert-NoEditor {
    try {
        $processes = @(
            Get-Process -ErrorAction Stop |
                Where-Object {
                    $_.ProcessName.StartsWith('UnrealEditor', [StringComparison]::OrdinalIgnoreCase) -or
                    $_.ProcessName.Equals('LiveCodingConsole', [StringComparison]::OrdinalIgnoreCase)
                }
        )
    }
    catch {
        throw "Cannot prove Unreal Editor and Live Coding are stopped: $($_.Exception.Message)"
    }

    if ($processes.Count -gt 0) {
        $description = ($processes | ForEach-Object { "$($_.Id):$($_.ProcessName)" }) -join ', '
        throw "Unreal Editor and Live Coding must be closed before generation. Existing process(es): $description"
    }
}

function Assert-NoUnrealBuildTool {
    try {
        $processes = @(
            Get-CimInstance Win32_Process -ErrorAction Stop |
                Where-Object {
                    $_.Name -ieq 'UnrealBuildTool.exe' -or
                    ($_.Name -ieq 'dotnet.exe' -and $_.CommandLine -match 'UnrealBuildTool')
                }
        )
    }
    catch {
        throw "Cannot prove UnrealBuildTool is stopped: $($_.Exception.Message)"
    }

    if ($processes.Count -gt 0) {
        throw 'Another UnrealBuildTool process is already running.'
    }
}

function Get-ProtectedState {
    param([Parameter(Mandatory)][string] $Path)
    $item = Get-Item -LiteralPath $Path -Force
    $hash = (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash
    [pscustomobject]@{
        Path = $item.FullName
        Length = $item.Length
        LastWriteTimeUtc = $item.LastWriteTimeUtc.Ticks
        Hash = $hash
    }
}

function Assert-ProtectedState {
    param(
        [Parameter(Mandatory)] $Expected,
        [Parameter(Mandatory)][string] $Label
    )
    $actual = Get-ProtectedState -Path $Expected.Path
    if ($actual.Length -ne $Expected.Length -or
        $actual.LastWriteTimeUtc -ne $Expected.LastWriteTimeUtc -or
        $actual.Hash -ne $Expected.Hash) {
        throw "$Label changed during generation: $($Expected.Path)"
    }
}

function Assert-OutputsAbsent {
    foreach ($output in $KnownOutputs) {
        $safeOutput = Resolve-SafeContainedPath -Path $output -AllowedRoot $ContentRoot -Label 'Reserved generated output'
        if (Test-Path -LiteralPath $safeOutput) {
            throw "Refusing overwrite because output already exists: $safeOutput"
        }
    }

    $safeMapDirectory = Resolve-SafeContainedPath -Path $MapDirectory -AllowedRoot $ContentRoot -Label 'Generated map directory'
    $siblings = @(
        Get-ChildItem -LiteralPath $safeMapDirectory -Force -ErrorAction Stop |
            Where-Object {
                $_.Name.StartsWith('L_SakuraTestLab_WP', [StringComparison]::OrdinalIgnoreCase)
            }
    )
    if ($siblings.Count -gt 0) {
        throw "Refusing overwrite because a suffixed map asset already exists: $($siblings[0].FullName)"
    }
}

function Invoke-TrackedProcess {
    param(
        [Parameter(Mandatory)][string] $File,
        [Parameter(Mandatory)][string[]] $Arguments,
        [Parameter(Mandatory)][string] $Label,
        [bool] $RequireNoEditor = $true
    )

    if ($RequireNoEditor) {
        Assert-NoEditor
    }

    $startInfo = [Diagnostics.ProcessStartInfo]::new()
    $startInfo.FileName = $File
    $startInfo.UseShellExecute = $false
    foreach ($argument in $Arguments) {
        [void] $startInfo.ArgumentList.Add($argument)
    }

    $process = [Diagnostics.Process]::new()
    $process.StartInfo = $startInfo
    $started = $false
    $exitCode = -1
    try {
        $started = $process.Start()
        if (-not $started) {
            throw "$Label did not start."
        }
        $process.WaitForExit()
        $exitCode = $process.ExitCode
    }
    finally {
        if ($started -and -not $process.HasExited) {
            $process.Kill($true)
            $process.WaitForExit()
        }
        $process.Dispose()
    }

    if ($exitCode -ne 0) {
        throw "$Label failed with exit code $exitCode."
    }
    if ($RequireNoEditor) {
        Assert-NoEditor
    }
}

function Invoke-TrackedBatch {
    param(
        [Parameter(Mandatory)][string] $Command,
        [Parameter(Mandatory)][string] $Label
    )

    Assert-NoEditor
    $startInfo = [Diagnostics.ProcessStartInfo]::new()
    $startInfo.FileName = $env:ComSpec
    $startInfo.UseShellExecute = $false
    # cmd.exe /s requires one raw outer quote pair around a command whose
    # executable path is itself quoted. ArgumentList escapes those quotes and
    # makes cmd treat them literally, so batch execution intentionally uses the
    # raw Arguments property here.
    $startInfo.Arguments = "/d /s /c `"$Command`""

    $process = [Diagnostics.Process]::new()
    $process.StartInfo = $startInfo
    $started = $false
    $exitCode = -1
    try {
        $started = $process.Start()
        if (-not $started) {
            throw "$Label did not start."
        }
        $process.WaitForExit()
        $exitCode = $process.ExitCode
    }
    finally {
        if ($started -and -not $process.HasExited) {
            $process.Kill($true)
            $process.WaitForExit()
        }
        $process.Dispose()
    }

    if ($exitCode -ne 0) {
        throw "$Label failed with exit code $exitCode."
    }
    Assert-NoEditor
}

function Assert-LogSentinel {
    param(
        [Parameter(Mandatory)][string] $Log,
        [Parameter(Mandatory)][string] $Sentinel,
        [Parameter(Mandatory)][string] $Label
    )
    if (-not (Test-Path -LiteralPath $Log -PathType Leaf)) {
        throw "$Label did not create its controlled log: $Log"
    }
    $sentinelMatches = @(
        Select-String -LiteralPath $Log -SimpleMatch $Sentinel
    )
    if ($sentinelMatches.Count -lt 1) {
        throw "$Label exited without its success sentinel '$Sentinel'."
    }

    $zeroErrorSummaries = @(
        Select-String -LiteralPath $Log -Pattern 'LogInit: Display: Success - 0 error\(s\), [0-9]+ warning\(s\)\s*$'
    )
    if ($zeroErrorSummaries.Count -ne 1 -or
        $zeroErrorSummaries[0].LineNumber -le $sentinelMatches[-1].LineNumber) {
        throw "$Label did not finish after its sentinel with exactly one 'Success - 0 error(s)' commandlet summary."
    }
}

function Assert-EditorLogSentinel {
    param(
        [Parameter(Mandatory)][string] $Log,
        [Parameter(Mandatory)][string] $Sentinel,
        [Parameter(Mandatory)][string] $Label
    )
    if (-not (Test-Path -LiteralPath $Log -PathType Leaf)) {
        throw "$Label did not create its controlled log: $Log"
    }
    $sentinelMatches = @(
        Select-String -LiteralPath $Log -SimpleMatch $Sentinel
    )
    if ($sentinelMatches.Count -ne 1) {
        throw "$Label did not emit exactly one success sentinel '$Sentinel'."
    }
    $fatalMatches = @(
        Select-String -LiteralPath $Log -Pattern 'LogWindows: Error:|Sakura editor generation failed:'
    )
    if ($fatalMatches.Count -gt 0) {
        throw "$Label logged a fatal/editor-generation error despite its sentinel."
    }
}

function Show-FailureLogTail {
    param([Parameter(Mandatory)][string] $Log)
    if (Test-Path -LiteralPath $Log -PathType Leaf) {
        Write-Error "Failure log tail: $Log" -ErrorAction Continue
        Get-Content -LiteralPath $Log -Tail 80 | ForEach-Object {
            Write-Error $_ -ErrorAction Continue
        }
    }
}

function Get-TaskOwnedContentOutputs {
    $outputs = [Collections.Generic.List[string]]::new()
    foreach ($output in $KnownOutputs) {
        $outputs.Add([IO.Path]::GetFullPath($output))
    }

    # WorldPartitionConvertCommandlet can emit companion map assets whose exact
    # suffix depends on source content (for example, map build data). The initial
    # absence check reserves this entire target-name prefix for this run.
    $safeMapDirectory = Resolve-SafeContainedPath -Path $MapDirectory -AllowedRoot $ContentRoot -Label 'Generated map directory'
    if (Test-Path -LiteralPath $safeMapDirectory -PathType Container) {
        foreach ($item in Get-ChildItem -LiteralPath $safeMapDirectory -Force -ErrorAction Stop) {
            if ($item.Name.StartsWith('L_SakuraTestLab_WP', [StringComparison]::OrdinalIgnoreCase)) {
                $outputs.Add($item.FullName)
            }
        }
    }

    @($outputs | Sort-Object -Unique)
}

function Get-GeneratedSnapshot {
    $roots = @(Get-TaskOwnedContentOutputs)
    $items = [Collections.Generic.List[IO.FileSystemInfo]]::new()
    foreach ($root in $roots) {
        $safeRoot = Resolve-SafeContainedPath -Path $root -AllowedRoot $ContentRoot -Label 'Generated snapshot root'
        if (Test-Path -LiteralPath $safeRoot -PathType Leaf) {
            $items.Add((Get-Item -LiteralPath $safeRoot -Force -ErrorAction Stop))
        }
        elseif (Test-Path -LiteralPath $safeRoot -PathType Container) {
            $items.Add((Get-Item -LiteralPath $safeRoot -Force -ErrorAction Stop))
            foreach ($item in Get-ChildItem -LiteralPath $safeRoot -Recurse -Force -ErrorAction Stop) {
                [void] (Resolve-SafeContainedPath -Path $item.FullName -AllowedRoot $safeRoot -Label 'Generated snapshot item')
                $items.Add($item)
            }
        }
    }

    @(
        $items |
            Sort-Object FullName |
            ForEach-Object {
                [pscustomobject]@{
                    Path = $_.FullName
                    Kind = $_.PSIsContainer ? 'Directory' : 'File'
                    Length = $_.PSIsContainer ? [int64] 0 : [int64] $_.Length
                    # Directory write times are mutable filesystem bookkeeping:
                    # validation can observe delayed NTFS parent-directory timestamp
                    # updates even when no persistent child path or file changed.
                    # Path + Kind still detect directory additions/removals, while
                    # files retain strict timestamp, length, and content identity.
                    LastWriteTimeUtc = $_.PSIsContainer ? [int64] 0 : [int64] $_.LastWriteTimeUtc.Ticks
                    Hash = $_.PSIsContainer ? '' : (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash
                }
            }
    )
}

function Assert-SnapshotsEqual {
    param(
        [Parameter(Mandatory)][object[]] $Before,
        [Parameter(Mandatory)][object[]] $After
    )
    $differences = @(
        Compare-Object -ReferenceObject $Before -DifferenceObject $After -Property Path, Kind, Length, LastWriteTimeUtc, Hash
    )
    if ($differences.Count -gt 0) {
        $details = @(
            $differences |
                Select-Object -First 20 |
                ForEach-Object {
                    "$($_.SideIndicator) $($_.Kind) $($_.Path) length=$($_.Length) write=$($_.LastWriteTimeUtc) hash=$($_.Hash)"
                }
        ) -join [Environment]::NewLine
        throw "ValidateOnly modified generated output; validation must remain read-only.$([Environment]::NewLine)$details"
    }
}

function Get-TreeSnapshot {
    param([Parameter(Mandatory)][string] $Root)

    $safeRoot = Resolve-SafeBuildOutputPath -Path $Root -Label 'Build-output snapshot root'
    if (-not (Test-Path -LiteralPath $safeRoot)) {
        return @()
    }
    if (-not (Test-Path -LiteralPath $safeRoot -PathType Container)) {
        throw "Build-output root is not a directory: $safeRoot"
    }

    @(
        Get-ChildItem -LiteralPath $safeRoot -Recurse -Force -ErrorAction Stop |
            Sort-Object FullName |
            ForEach-Object {
                if (($_.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
                    throw "Build-output tree contains a reparse point and cannot be checkpointed safely: $($_.FullName)"
                }
                $relativePath = [IO.Path]::GetRelativePath($safeRoot, $_.FullName)
                if ($_.PSIsContainer) {
                    [pscustomobject]@{
                        RelativePath = $relativePath
                        Kind = 'Directory'
                        Length = [int64] 0
                        LastWriteTimeUtc = [int64] 0
                        Hash = ''
                    }
                }
                else {
                    [pscustomobject]@{
                        RelativePath = $relativePath
                        Kind = 'File'
                        Length = [int64] $_.Length
                        LastWriteTimeUtc = [int64] $_.LastWriteTimeUtc.Ticks
                        Hash = (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash
                    }
                }
            }
    )
}

function New-BuildOutputCheckpoint {
    param([Parameter(Mandatory)][string] $BackupRoot)

    $safeBackupRoot = Resolve-SafeContainedPath -Path $BackupRoot -AllowedRoot $runScratchFull -Label 'Build-output backup root'
    [void] [IO.Directory]::CreateDirectory($safeBackupRoot)
    $checkpoint = [Collections.Generic.List[object]]::new()
    $index = 0
    foreach ($root in $BuildOutputRoots) {
        $safeRoot = Resolve-SafeBuildOutputPath -Path $root -Label 'Build-output checkpoint root'
        $existed = Test-Path -LiteralPath $safeRoot
        if ($existed -and -not (Test-Path -LiteralPath $safeRoot -PathType Container)) {
            throw "Build-output root is not a directory: $safeRoot"
        }

        $snapshot = @(Get-TreeSnapshot -Root $safeRoot)
        $backupPath = Join-Path $safeBackupRoot ('root_{0:D3}' -f $index)
        [void] (Resolve-SafeContainedPath -Path $backupPath -AllowedRoot $safeBackupRoot -Label 'Build-output backup path')
        if ($existed) {
            Copy-Item -LiteralPath $safeRoot -Destination $backupPath -Recurse -Force -ErrorAction Stop
        }
        $checkpoint.Add([pscustomobject]@{
            Root = $safeRoot
            Existed = [bool] $existed
            Snapshot = $snapshot
            Backup = $backupPath
        })
        ++$index
    }

    @($checkpoint)
}

function Restore-BuildOutputCheckpoint {
    param(
        [Parameter(Mandatory)][object[]] $Checkpoint,
        [Parameter(Mandatory)][string] $BackupRoot
    )

    $restoreErrors = [Collections.Generic.List[string]]::new()
    foreach ($entry in $Checkpoint) {
        try {
            $safeRoot = Resolve-SafeBuildOutputPath -Path ([string] $entry.Root) -Label 'Build-output rollback root'
            if (Test-Path -LiteralPath $safeRoot) {
                Remove-Item -LiteralPath $safeRoot -Recurse -Force -ErrorAction Stop
            }

            if ([bool] $entry.Existed) {
                $safeBackup = Resolve-SafeContainedPath -Path ([string] $entry.Backup) -AllowedRoot $BackupRoot -Label 'Build-output rollback backup'
                if (-not (Test-Path -LiteralPath $safeBackup -PathType Container)) {
                    throw "Build-output rollback backup is missing: $safeBackup"
                }
                Copy-Item -LiteralPath $safeBackup -Destination $safeRoot -Recurse -Force -ErrorAction Stop
            }

            $actualExists = Test-Path -LiteralPath $safeRoot -PathType Container
            if ($actualExists -ne [bool] $entry.Existed) {
                throw "Build-output rollback did not restore the original existence state: $safeRoot"
            }
            if ([bool] $entry.Existed) {
                $actualSnapshot = @(Get-TreeSnapshot -Root $safeRoot)
                $differences = @(
                    Compare-Object -ReferenceObject @($entry.Snapshot) -DifferenceObject $actualSnapshot `
                        -Property RelativePath, Kind, Length, LastWriteTimeUtc, Hash
                )
                if ($differences.Count -gt 0) {
                    throw "Build-output rollback verification failed: $safeRoot"
                }
            }
        }
        catch {
            $restoreErrors.Add("$($entry.Root): $($_.Exception.Message)")
        }
    }
    if ($restoreErrors.Count -gt 0) {
        throw "One or more build-output roots failed rollback:$([Environment]::NewLine)$($restoreErrors -join [Environment]::NewLine)"
    }
}

function Remove-TaskOwnedOutput {
    param([Parameter(Mandatory)][string] $Path)
    $resolved = Resolve-SafeContainedPath -Path $Path -AllowedRoot $ContentRoot -Label 'Task-owned content cleanup target'
    if ($resolved.Equals($ContentRoot.TrimEnd([IO.Path]::DirectorySeparatorChar), [StringComparison]::OrdinalIgnoreCase)) {
        throw 'Task-owned cleanup may not target the project Content root itself.'
    }
    if (-not (Test-Path -LiteralPath $resolved)) {
        return
    }

    Remove-Item -LiteralPath $resolved -Recurse -Force -ErrorAction Stop
}

function Get-AbsentOutputAncestors {
    $ancestors = @()
    foreach ($output in $KnownOutputs) {
        $safeOutput = Resolve-SafeContainedPath -Path $output -AllowedRoot $ContentRoot -Label 'Generated-output ancestor path'
        $parent = [IO.Path]::GetDirectoryName($safeOutput)
        while ($parent.StartsWith($ContentRoot + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) {
            [void] (Resolve-SafeContainedPath -Path $parent -AllowedRoot $ContentRoot -Label 'Generated-output ancestor')
            if (-not (Test-Path -LiteralPath $parent)) {
                $ancestors += $parent
            }
            $parent = [IO.Path]::GetDirectoryName($parent)
        }
    }
    @($ancestors | Sort-Object -Unique | Sort-Object { $_.Length } -Descending)
}

Assert-RequiredFile -Path $ProjectFile
Assert-RequiredFile -Path $BuildBat
Assert-RequiredFile -Path $EditorCmd
Assert-RequiredFile -Path $BuildVersionFile
Assert-RequiredFile -Path $SourceMap
Assert-RequiredFile -Path $DefaultEngineIni
[void] (Resolve-SafeContainedPath -Path $ContentRoot -AllowedRoot $ProjectRoot -Label 'Project Content directory')

$buildVersion = Get-Content -LiteralPath $BuildVersionFile -Raw | ConvertFrom-Json
if ($buildVersion.MajorVersion -ne 5 -or $buildVersion.MinorVersion -ne 8) {
    throw "EngineRoot must point to Unreal Engine 5.8; found $($buildVersion.MajorVersion).$($buildVersion.MinorVersion)."
}
$projectDescriptor = Get-Content -LiteralPath $ProjectFile -Raw | ConvertFrom-Json
if ([string] $projectDescriptor.EngineAssociation -ne '5.8') {
    throw "Project EngineAssociation must be 5.8; found '$($projectDescriptor.EngineAssociation)'."
}
$BuildOutputRoots = @(Get-BuildOutputRoots)

Assert-NoEditor
Assert-NoUnrealBuildTool
Assert-OutputsAbsent
$absentOutputAncestors = @(Get-AbsentOutputAncestors)

$sourceState = Get-ProtectedState -Path $SourceMap
$configState = Get-ProtectedState -Path $DefaultEngineIni
$scratchRootFull = [IO.Path]::GetFullPath($ScratchRoot)
$scratchRootFull = Resolve-SafeContainedPath -Path $scratchRootFull -AllowedRoot $scratchRootFull -Label 'Controlled scratch root'
$scratchRootExisted = Test-Path -LiteralPath $scratchRootFull
$runScratchFull = $null
try {
    if ($scratchRootExisted -and -not (Test-Path -LiteralPath $scratchRootFull -PathType Container)) {
        throw "Controlled scratch root is not a directory: $scratchRootFull"
    }
    [void] [IO.Directory]::CreateDirectory($scratchRootFull)
    $scratchRootFull = Resolve-SafeContainedPath -Path $scratchRootFull -AllowedRoot $scratchRootFull -Label 'Controlled scratch root'
    $runScratch = Join-Path $scratchRootFull ([Guid]::NewGuid().ToString('N'))
    $runScratchFull = Resolve-SafeContainedPath -Path $runScratch -AllowedRoot $scratchRootFull -Label 'Controlled run scratch directory'
    [void] [IO.Directory]::CreateDirectory($runScratchFull)
    $runScratchFull = Resolve-SafeContainedPath -Path $runScratchFull -AllowedRoot $scratchRootFull -Label 'Controlled run scratch directory'
}
catch {
    $scratchBootstrapError = $_
    $scratchBootstrapCleanupErrors = [Collections.Generic.List[string]]::new()
    if (-not [string]::IsNullOrEmpty($runScratchFull)) {
        try {
            $safeRunScratch = Resolve-SafeContainedPath -Path $runScratchFull -AllowedRoot $scratchRootFull -Label 'Failed scratch-bootstrap cleanup'
            if (Test-Path -LiteralPath $safeRunScratch) {
                Remove-Item -LiteralPath $safeRunScratch -Recurse -Force -ErrorAction Stop
            }
        }
        catch {
            $scratchBootstrapCleanupErrors.Add("Could not clean failed run-scratch bootstrap '$runScratchFull': $($_.Exception.Message)")
        }
    }
    if (-not $scratchRootExisted) {
        try {
            $safeScratchRoot = Resolve-SafeContainedPath -Path $scratchRootFull -AllowedRoot $scratchRootFull -Label 'Failed scratch-root bootstrap cleanup'
            if ((Test-Path -LiteralPath $safeScratchRoot -PathType Container) -and
                @(Get-ChildItem -LiteralPath $safeScratchRoot -Force -ErrorAction Stop).Count -eq 0) {
                Remove-Item -LiteralPath $safeScratchRoot -Force -ErrorAction Stop
            }
        }
        catch {
            $scratchBootstrapCleanupErrors.Add("Could not clean failed scratch-root bootstrap '$scratchRootFull': $($_.Exception.Message)")
        }
    }
    if ($scratchBootstrapCleanupErrors.Count -gt 0) {
        throw "Scratch initialization failed: $($scratchBootstrapError.Exception.Message)$([Environment]::NewLine)Cleanup also failed:$([Environment]::NewLine)$($scratchBootstrapCleanupErrors -join [Environment]::NewLine)"
    }
    throw $scratchBootstrapError
}

$convertLog = Join-Path $runScratchFull 'convert.log'
$generateLog = Join-Path $runScratchFull 'generate.log'
$validateLog = Join-Path $runScratchFull 'validate.log'
$buildBackupRoot = Join-Path $runScratchFull 'build-backup'
$sourceGuard = $null
$configGuard = $null
$success = $false
$contentMutationStarted = $false
$buildMutationStarted = $false
$buildCheckpoint = @()
$retainScratchForRecovery = $false
$primaryError = $null
$cleanupErrors = [Collections.Generic.List[string]]::new()

try {
    # Permit reads but deny writes/replacement/deletion for the source map and startup config.
    $sourceGuard = [IO.File]::Open($SourceMap, [IO.FileMode]::Open, [IO.FileAccess]::Read, [IO.FileShare]::Read)
    $configGuard = [IO.File]::Open($DefaultEngineIni, [IO.FileMode]::Open, [IO.FileAccess]::Read, [IO.FileShare]::Read)

    $buildCheckpoint = @(New-BuildOutputCheckpoint -BackupRoot $buildBackupRoot)
    $buildMutationStarted = $true
    $buildCommand = '"{0}" SakuraUnrealPrototypeEditor Win64 Development -Project="{1}" -NoHotReload -NoEngineChanges -NoUBA -NoXGE -NoFASTBuild -NoSNDBS -NoLog' -f $BuildBat, $ProjectFile
    Invoke-TrackedBatch -Command $buildCommand -Label 'Sakura editor build'
    Assert-NoUnrealBuildTool
    Assert-NoEditor
    Assert-OutputsAbsent
    Assert-ProtectedState -Expected $sourceState -Label 'Source map'
    Assert-ProtectedState -Expected $configState -Label 'DefaultEngine.ini'

    $commonArguments = @(
        '-Unattended',
        '-NoP4',
        '-NoSplash',
        '-stdout',
        '-FullStdOutLogOutput',
        '-UTF8Output',
        '-AllowCommandletRendering',
        # Commandlets otherwise reserve no logical cores and can spawn roughly
        # one ShaderCompileWorker per thread. Four workers are sufficient for
        # this generator and keep memory/desktop impact bounded on this machine.
        '-ini:Engine:[DevOptions.Shaders]:NumUnusedShaderCompilingThreads=28',
        '-ini:Engine:[DevOptions.Shaders]:NumUnusedShaderCompilingThreadsDuringGame=28'
    )

    $contentMutationStarted = $true
    Invoke-TrackedProcess -File $EditorCmd -Arguments (@(
        $ProjectFile,
        '-run=WorldPartitionConvertCommandlet',
        $SourcePackage,
        '-ConversionSuffix',
        "-AbsLog=$convertLog"
    ) + $commonArguments) -Label 'World Partition conversion'
    Assert-LogSentinel -Log $convertLog -Sentinel '######## CONVERSION COMPLETED SUCCESSFULLY ########' -Label 'World Partition conversion'
    if (-not (Test-Path -LiteralPath $TargetMap -PathType Leaf)) {
        throw "World Partition conversion did not create $TargetMap"
    }
    $externalActorFiles = @(Get-ChildItem -LiteralPath $ExternalActors -Recurse -File -Filter '*.uasset' -ErrorAction Stop)
    if ($externalActorFiles.Count -lt 1) {
        throw 'World Partition conversion did not create any external actor packages.'
    }
    Assert-ProtectedState -Expected $sourceState -Label 'Source map'
    Assert-ProtectedState -Expected $configState -Label 'DefaultEngine.ini'

    # Landscape creation is intentionally performed in a normal editor world.
    # UE 5.8's Landscape renderer crashes when ALandscape::Import creates its
    # render proxies inside an -AllowCommandletRendering commandlet world.
    Invoke-TrackedProcess -File $Editor -Arguments (@(
        $ProjectFile,
        $TargetPackage,
        '-SakuraGenerateOpenWorldLabEditor',
        "-SakuraOpenWorldSeed=$Seed",
        "-AbsLog=$generateLog"
    ) + @($commonArguments | Where-Object { $_ -ne '-AllowCommandletRendering' })) -Label 'Sakura open-world generation'
    Assert-EditorLogSentinel -Log $generateLog -Sentinel 'SAKURA_EDITOR_GENERATION_SUCCESS' -Label 'Sakura open-world generation'

    $beforeValidation = @(Get-GeneratedSnapshot)
    Invoke-TrackedProcess -File $EditorCmd -Arguments (@(
        $ProjectFile,
        '-run=SakuraGenerateOpenWorldLab',
        $TargetPackage,
        '-ValidateOnly',
        "-Seed=$Seed",
        "-AbsLog=$validateLog"
    ) + $commonArguments) -Label 'Sakura open-world validation'
    Assert-LogSentinel -Log $validateLog -Sentinel 'SAKURA_VALIDATION_SUCCESS' -Label 'Sakura open-world validation'
    $afterValidation = @(Get-GeneratedSnapshot)
    Assert-SnapshotsEqual -Before $beforeValidation -After $afterValidation

    Assert-ProtectedState -Expected $sourceState -Label 'Source map'
    Assert-ProtectedState -Expected $configState -Label 'DefaultEngine.ini'
    $success = $true
}
catch {
    $primaryError = $_
    Show-FailureLogTail -Log $convertLog
    Show-FailureLogTail -Log $generateLog
    Show-FailureLogTail -Log $validateLog
}
finally {
    if ($null -ne $sourceGuard) {
        $sourceGuard.Dispose()
    }
    if ($null -ne $configGuard) {
        $configGuard.Dispose()
    }

    $rollbackIsSafe = $true
    try {
        Assert-NoEditor
        Assert-NoUnrealBuildTool
    }
    catch {
        $rollbackIsSafe = $false
        $cleanupErrors.Add("Could not prove rollback is safe: $($_.Exception.Message)")
    }

    if (-not $success -and $contentMutationStarted) {
        if ($rollbackIsSafe) {
            $contentRollbackOutputs = @($KnownOutputs)
            try {
                $contentRollbackOutputs = @(Get-TaskOwnedContentOutputs)
            }
            catch {
                $cleanupErrors.Add("Could not enumerate every task-owned conversion output: $($_.Exception.Message)")
            }
            foreach ($output in $contentRollbackOutputs) {
                try {
                    Remove-TaskOwnedOutput -Path $output
                }
                catch {
                    $cleanupErrors.Add("Could not remove task-owned partial output '$output': $($_.Exception.Message)")
                }
            }
            foreach ($ancestor in $absentOutputAncestors) {
                try {
                    $safeAncestor = Resolve-SafeContainedPath -Path $ancestor -AllowedRoot $ContentRoot -Label 'Task-created empty ancestor'
                    if ((Test-Path -LiteralPath $safeAncestor -PathType Container) -and
                        @(Get-ChildItem -LiteralPath $safeAncestor -Force -ErrorAction Stop).Count -eq 0) {
                        Remove-Item -LiteralPath $safeAncestor -Force -ErrorAction Stop
                    }
                }
                catch {
                    $cleanupErrors.Add("Could not prune task-created empty ancestor '$ancestor': $($_.Exception.Message)")
                }
            }
        }
        else {
            foreach ($output in $KnownOutputs) {
                try {
                    $safeOutput = Resolve-SafeContainedPath -Path $output -AllowedRoot $ContentRoot -Label 'Partial generated output'
                    if (Test-Path -LiteralPath $safeOutput) {
                        $cleanupErrors.Add("Rollback skipped while a process may be active; partial output remains: $safeOutput")
                    }
                }
                catch {
                    $cleanupErrors.Add("Could not safely inspect partial output '$output': $($_.Exception.Message)")
                }
            }
            $cleanupErrors.Add("Rollback could not inspect/remove possible companion conversion assets with prefix 'L_SakuraTestLab_WP' under '$MapDirectory'.")
            foreach ($ancestor in $absentOutputAncestors) {
                if (Test-Path -LiteralPath $ancestor) {
                    $cleanupErrors.Add("Rollback skipped while a process may be active; task-created ancestor may remain: $ancestor")
                }
            }
        }
    }

    if (-not $success -and $buildMutationStarted) {
        if ($rollbackIsSafe) {
            try {
                Restore-BuildOutputCheckpoint -Checkpoint $buildCheckpoint -BackupRoot $buildBackupRoot
            }
            catch {
                $retainScratchForRecovery = $true
                $cleanupErrors.Add("Could not restore/verify pre-build outputs: $($_.Exception.Message)")
            }
        }
        else {
            $retainScratchForRecovery = $true
            foreach ($entry in $buildCheckpoint) {
                $cleanupErrors.Add("Build-output rollback skipped while a process may be active; recovery checkpoint retained for: $($entry.Root)")
            }
        }
    }

    if ($retainScratchForRecovery) {
        $cleanupErrors.Add("Controlled recovery scratch was retained at '$runScratchFull'.")
    }
    else {
        try {
            $safeRunScratch = Resolve-SafeContainedPath -Path $runScratchFull -AllowedRoot $scratchRootFull -Label 'Controlled run scratch cleanup'
            if (Test-Path -LiteralPath $safeRunScratch) {
                Remove-Item -LiteralPath $safeRunScratch -Recurse -Force -ErrorAction Stop
            }
            $safeScratchRoot = Resolve-SafeContainedPath -Path $scratchRootFull -AllowedRoot $scratchRootFull -Label 'Controlled scratch-root cleanup'
            if (-not $scratchRootExisted -and
                (Test-Path -LiteralPath $safeScratchRoot -PathType Container) -and
                @(Get-ChildItem -LiteralPath $safeScratchRoot -Force -ErrorAction Stop).Count -eq 0) {
                Remove-Item -LiteralPath $safeScratchRoot -Force -ErrorAction Stop
            }
        }
        catch {
            $cleanupErrors.Add("Could not remove controlled scratch directory '$runScratchFull': $($_.Exception.Message)")
        }
    }
}

if ($cleanupErrors.Count -gt 0) {
    $cleanupMessage = $cleanupErrors -join [Environment]::NewLine
    if ($null -ne $primaryError) {
        throw "Generation failed: $($primaryError.Exception.Message)$([Environment]::NewLine)Cleanup also failed:$([Environment]::NewLine)$cleanupMessage"
    }
    throw "Cleanup failed:$([Environment]::NewLine)$cleanupMessage"
}
if ($null -ne $primaryError) {
    throw $primaryError
}

Write-Host 'Sakura open-world lab generation and fresh-process validation succeeded.' -ForegroundColor Green
Write-Host "Generated map: $TargetMap"
Write-Host 'The source map and Config\DefaultEngine.ini were verified unchanged.'
Write-Host "Required editor build outputs were retained under: $($BuildOutputRoots -join '; ')"

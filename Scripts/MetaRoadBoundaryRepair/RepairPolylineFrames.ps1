[CmdletBinding(SupportsShouldProcess)]
param(
    [Parameter(Mandatory)][string]$PluginDirectory,
    [switch]$Apply,
    [string]$BackupDirectory
)
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path -LiteralPath $PluginDirectory).Path
$manifest = Get-Content -LiteralPath (Join-Path $root 'MetaRoad.uplugin') -Raw | ConvertFrom-Json
if ($manifest.VersionName -ne '3.2.0') { throw 'Only reviewed MetaRoad 3.2.0 source is supported.' }
$path = Join-Path $root 'Source/MetaRoadEditor/Private/RoadMeshBuild/Ops/GraphOp.cpp'
$bytes = [IO.File]::ReadAllBytes($path)
$bom = $bytes.Length -ge 3 -and $bytes[0] -eq 239 -and $bytes[1] -eq 187 -and $bytes[2] -eq 191
$source = [IO.File]::ReadAllText($path)
$nl = if ($source.Contains("`r`n")) { "`r`n" } else { "`n" }
$oldLoop = "`tfor (int Index = 0; Index < OutPoints.Num(); ++Index)"
$newLoop = "`t// OWS #179: a lone projected sample has no geometric tangent.$nl`tfor (int Index = 0; OutPoints.Num() > 1 && Index < OutPoints.Num(); ++Index)"
$oldFrame = "`t`tOutPoints[Index].Quat = (FRotationMatrix::MakeFromXZ(ForwardVector, Normals[Index])).ToQuat();"
$priorFrame = @(
    "`t`t// Coincident samples or opposing segments can cancel the tangent."
    "`t`t// Keep the source spline orientation instead of making an invalid frame."
    "`t`tif (!ForwardVector.IsNearlyZero())"
    "`t`t{"
    "`t$oldFrame"
    "`t`t}"
) -join $nl
$newFrame = @(
    "`t`t// OWS #179: preserve the source orientation when projection cannot form a valid frame."
    "`t`tif (!ForwardVector.ContainsNaN() && !Normals[Index].ContainsNaN() &&"
    "`t`t`t!ForwardVector.IsNearlyZero() && !Normals[Index].IsNearlyZero())"
    "`t`t{"
    "`t`t`tconst FMatrix Frame = FRotationMatrix::MakeFromXZ(ForwardVector, Normals[Index]);"
    "`t`t`tconst bool bUnitFrame = !Frame.ContainsNaN() &&"
    "`t`t`t`tFMath::Abs(1.0 - Frame.GetScaledAxis(EAxis::X).SizeSquared()) <= UE_KINDA_SMALL_NUMBER &&"
    "`t`t`t`tFMath::Abs(1.0 - Frame.GetScaledAxis(EAxis::Y).SizeSquared()) <= UE_KINDA_SMALL_NUMBER &&"
    "`t`t`t`tFMath::Abs(1.0 - Frame.GetScaledAxis(EAxis::Z).SizeSquared()) <= UE_KINDA_SMALL_NUMBER;"
    "`t`t`tif (bUnitFrame) OutPoints[Index].Quat = Frame.ToQuat();"
    "`t`t}"
) -join $nl
if ($source.Contains($newLoop) -and $source.Contains($newFrame)) { 'Polyline frame repair already present; source unchanged.'; return }
$isPriorRepair = $source.Contains($newLoop) -and
    [regex]::Matches($source, [regex]::Escape($priorFrame)).Count -eq 1
if (-not $isPriorRepair -and ($source.Contains('// OWS #179:') -or $source.Contains($newFrame) -or
    [regex]::Matches($source, [regex]::Escape($oldLoop)).Count -ne 1 -or
    [regex]::Matches($source, [regex]::Escape($oldFrame)).Count -ne 1)) {
    throw 'Expected reviewed polyline frame statements. No source was changed.'
}
if (-not $Apply) { 'Reviewed frame construction found. Preview only; use -Apply and a controlled -BackupDirectory.'; return }
if (-not $BackupDirectory) { throw '-Apply requires an existing controlled -BackupDirectory.' }
$backupRoot = (Resolve-Path -LiteralPath $BackupDirectory).Path
if (-not (Test-Path -LiteralPath $backupRoot -PathType Container)) { throw 'BackupDirectory must be a directory.' }
$backup = Join-Path $backupRoot 'MetaRoad-GraphOp.cpp.before-179-frame-validation'
if (Test-Path -LiteralPath $backup) { throw 'Refusing to overwrite an existing backup.' }
if ($PSCmdlet.ShouldProcess($path, 'Preserve source orientation for degenerate projected tangents (OWS #179)')) {
    [IO.File]::WriteAllBytes($backup, $bytes)
    $updated = if ($isPriorRepair) { $source.Replace($priorFrame, $newFrame) }
        else { $source.Replace($oldLoop, $newLoop).Replace($oldFrame, $newFrame) }
    [IO.File]::WriteAllText($path, $updated, [Text.UTF8Encoding]::new($bom))
    "Source repaired. Backup: $backup. Rebuild MetaRoadEditor before use."
}

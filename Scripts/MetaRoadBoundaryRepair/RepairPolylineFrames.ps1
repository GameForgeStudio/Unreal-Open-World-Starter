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
$newFrame = @(
    "`t`t// Coincident samples or opposing segments can cancel the tangent."
    "`t`t// Keep the source spline orientation instead of making an invalid frame."
    "`t`tif (!ForwardVector.IsNearlyZero())"
    "`t`t{"
    "`t$oldFrame"
    "`t`t}"
) -join $nl
if ($source.Contains($newLoop) -and $source.Contains($newFrame)) { 'Polyline frame repair already present; source unchanged.'; return }
if ($source.Contains('// OWS #179:') -or $source.Contains($newFrame) -or
    [regex]::Matches($source, [regex]::Escape($oldLoop)).Count -ne 1 -or
    [regex]::Matches($source, [regex]::Escape($oldFrame)).Count -ne 1) {
    throw 'Expected reviewed polyline frame statements. No source was changed.'
}
if (-not $Apply) { 'Reviewed frame construction found. Preview only; use -Apply and a controlled -BackupDirectory.'; return }
if (-not $BackupDirectory) { throw '-Apply requires an existing controlled -BackupDirectory.' }
$backupRoot = (Resolve-Path -LiteralPath $BackupDirectory).Path
if (-not (Test-Path -LiteralPath $backupRoot -PathType Container)) { throw 'BackupDirectory must be a directory.' }
$backup = Join-Path $backupRoot 'MetaRoad-GraphOp.cpp.before-179'
if (Test-Path -LiteralPath $backup) { throw 'Refusing to overwrite an existing backup.' }
if ($PSCmdlet.ShouldProcess($path, 'Preserve source orientation for degenerate projected tangents (OWS #179)')) {
    [IO.File]::WriteAllBytes($backup, $bytes)
    [IO.File]::WriteAllText($path, $source.Replace($oldLoop, $newLoop).Replace($oldFrame, $newFrame), [Text.UTF8Encoding]::new($bom))
    "Source repaired. Backup: $backup. Rebuild MetaRoadEditor before use."
}

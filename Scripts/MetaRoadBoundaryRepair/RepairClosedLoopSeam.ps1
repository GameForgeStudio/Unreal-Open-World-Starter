[CmdletBinding(SupportsShouldProcess)]
param(
    [Parameter(Mandatory)][string]$PluginDirectory,
    [switch]$Apply,
    [string]$BackupDirectory
)
$ErrorActionPreference = 'Stop'
$pluginRoot = (Resolve-Path -LiteralPath $PluginDirectory).Path
$manifest = Get-Content -LiteralPath (Join-Path $pluginRoot 'MetaRoad.uplugin') -Raw | ConvertFrom-Json
if ($manifest.VersionName -ne '3.2.0') { throw 'Only the reviewed MetaRoad 3.2.0 source layout is supported.' }
$path = Join-Path $pluginRoot 'Source/MetaRoad/Private/RoadSplineComponent.cpp'
$bytes = [IO.File]::ReadAllBytes($path)
$bom = $bytes.Length -ge 3 -and $bytes[0] -eq 239 -and $bytes[1] -eq 187 -and $bytes[2] -eq 191
$source = [IO.File]::ReadAllText($path)
$nl = if ($source.Contains("`r`n")) { "`r`n" } else { "`n" }
$start = $source.IndexOf('FRoadPosition URoadSplineComponent::GetRoadPosition(double SOffset, double ROffset,')
$end = $source.IndexOf('FRoadPosition URoadSplineComponent::GetRoadPosition(int SectionIndex,', $start + 1)
if ($start -lt 0 -or $end -le $start) { throw 'Reviewed overload not found.' }
$body = $source.Substring($start, $end - $start)
$old = "`tconst float Param = SplineCurves.ReparamTable.Eval(SOffset, 0.0f);"
$replacement = @'
	// OWS #177: the terminal loop key can have a different linear tangent.
	// Use the start key at the exact seam; keep the caller's SOffset and ROffset.
	const float Param = IsClosedLoop() && !SplineCurves.Position.Points.IsEmpty() &&
		SOffset == SplineCurves.GetSplineLength()
		? SplineCurves.Position.Points[0].InVal
		: SplineCurves.ReparamTable.Eval(SOffset, 0.0f);
'@
$replacement = $replacement.Replace("`r`n", "`n").Replace("`n", $nl)
if ($body.Contains($replacement)) { 'Seam repair already present; source unchanged.'; return }
if ($body.Contains('// OWS #177:') -or [regex]::Matches($body, [regex]::Escape($old)).Count -ne 1) {
    throw 'Source differs from reviewed version. No source was changed.'
}
if (-not $Apply) { 'Reviewed seam site found. Preview only; use -Apply and a controlled -BackupDirectory.'; return }
if (-not $BackupDirectory) { throw '-Apply requires an existing controlled -BackupDirectory.' }
$backupRoot = (Resolve-Path -LiteralPath $BackupDirectory).Path
if (-not (Test-Path -LiteralPath $backupRoot -PathType Container)) { throw 'BackupDirectory must be a directory.' }
$backup = Join-Path $backupRoot 'MetaRoad-RoadSplineComponent.cpp.before-177'
if (Test-Path -LiteralPath $backup) { throw 'Refusing to overwrite the existing backup.' }
if ($PSCmdlet.ShouldProcess($path, 'Apply OWS #177 closed-loop seam repair')) {
    [IO.File]::WriteAllBytes($backup, $bytes)
    $updated = $source.Substring(0, $start) + $body.Replace($old, $replacement) + $source.Substring($end)
    [IO.File]::WriteAllText($path, $updated, [Text.UTF8Encoding]::new($bom))
    "Source repaired. Backup: $backup. Rebuild the MetaRoad runtime module and verify before deployment."
}

[CmdletBinding(SupportsShouldProcess)]
param(
    [Parameter(Mandatory)][string]$PluginDirectory,
    [switch]$Apply,
    [string]$BackupDirectory
)

# Opt-in maintenance of a separately licensed installation. No vendor code is bundled.
$ErrorActionPreference = 'Stop'
$pluginRoot = (Resolve-Path -LiteralPath $PluginDirectory).Path
$manifest = Get-Content -LiteralPath (Join-Path $pluginRoot 'MetaRoad.uplugin') -Raw | ConvertFrom-Json
if ($manifest.VersionName -ne '3.2.0') { throw 'Only the reviewed MetaRoad 3.2.0 source layout is supported.' }
$sourcePath = Join-Path $pluginRoot 'Source/MetaRoadEditor/Private/Utils/OpUtils.cpp'
$sourceBytes = [IO.File]::ReadAllBytes($sourcePath)
$hasBom = $sourceBytes.Length -ge 3 -and $sourceBytes[0] -eq 239 -and $sourceBytes[1] -eq 187 -and $sourceBytes[2] -eq 191
$source = [IO.File]::ReadAllText($sourcePath)
$newline = if ($source.Contains("`r`n")) { "`r`n" } else { "`n" }
$marker = '// OWS #174: filter the traversed edge, not its other incident edges.'
$old = @'
		for (int NbrVID : Graph.VtxVerticesItr(VID))
		{
			if (VertexHasGID(Graph, NbrVID, SkipEdges, GIDFilter) && (Boundary.Num() == 0 || Boundary.Last().A != NbrVID))
'@
$replacement = @'
		// OWS #174: filter the traversed edge, not its other incident edges.
		for (int EID : Graph.VtxEdgesItr(VID))
		{
			const auto Edge = Graph.GetEdgeCopy(EID);
			if (!GIDFilter(Edge.Group) || IsExist(SkipEdges, { Edge.A, Edge.B }))
			{
				continue;
			}
			const int NbrVID = Edge.A == VID ? Edge.B : Edge.A;
			if (Boundary.Num() == 0 || Boundary.Last().A != NbrVID)
'@
$old = $old.Replace("`r`n", "`n").Replace("`n", $newline)
$replacement = $replacement.Replace("`r`n", "`n").Replace("`n", $newline)
if ($source.Contains($replacement)) { Write-Output 'Repair already present; source unchanged.'; return }
if ($source.Contains($marker)) { throw 'Repair marker exists but the reviewed replacement differs; inspect manually.' }
if ([regex]::Matches($source, [regex]::Escape($old)).Count -ne 1) {
    throw 'Expected exactly one reviewed traversal block. No source was changed.'
}
if (-not $Apply) { Write-Output 'Reviewed traversal found. Preview only; use -Apply and a controlled -BackupDirectory to modify it.'; return }
if (-not $BackupDirectory) { throw '-Apply requires an existing controlled -BackupDirectory.' }
$backupRoot = (Resolve-Path -LiteralPath $BackupDirectory).Path
if (-not (Test-Path -LiteralPath $backupRoot -PathType Container)) { throw 'BackupDirectory must be a directory.' }
$backupPath = Join-Path $backupRoot 'MetaRoad-OpUtils.cpp.before-174'
if (Test-Path -LiteralPath $backupPath) { throw "Refusing to overwrite an existing backup: $backupPath" }
if ($PSCmdlet.ShouldProcess($sourcePath, 'Apply OWS #174 edge-filter repair')) {
    [IO.File]::WriteAllBytes($backupPath, $sourceBytes)
    [IO.File]::WriteAllText($sourcePath, $source.Replace($old, $replacement), [Text.UTF8Encoding]::new($hasBom))
    Write-Output "Source repaired. Backup: $backupPath. Recompile and verify before claiming the running plugin is repaired."
}

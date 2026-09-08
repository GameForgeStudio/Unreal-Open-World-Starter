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
$updated = $source
if (-not $updated.Contains($replacement)) {
    if ($updated.Contains($marker)) { throw 'Repair marker exists but the reviewed replacement differs; inspect manually.' }
    if ([regex]::Matches($updated, [regex]::Escape($old)).Count -ne 1) {
        throw 'Expected exactly one reviewed traversal block. No source was changed.'
    }
    $updated = $updated.Replace($old, $replacement)
}
$functionStart = $updated.IndexOf('bool OpUtils::FindBoundary(')
$functionEnd = $updated.IndexOf('int OpUtils::FindBoundaries(', $functionStart + 1)
if ($functionStart -lt 0 -or $functionEnd -le $functionStart) { throw 'Boundary function scope not found.' }
$body = $updated.Substring($functionStart, $functionEnd - $functionStart)
$edgeGuard = @'
			// OWS #176: shared vertices are valid; only a repeated directed edge ends the walk.
			const uint64 DirectedEdge = (uint64(uint32(VID)) << 32) | uint32(Candidate.VID);
			if (TraversedEdges.Contains(DirectedEdge))
			{
				bContoureFound = Boundary.Num() > 2 && Boundary[0].A == VID && Boundary[0].B == Candidate.VID;
				break;
			}
			TraversedEdges.Add(DirectedEdge);
'@
$edgeGuard = $edgeGuard.Replace("`r`n", "`n").Replace("`n", $newline)
if ($body.Contains('// OWS #176:')) {
    if (-not $body.Contains($edgeGuard) -or -not $body.Contains('TSet<uint64> TraversedEdges;') -or $body.Contains('bLoopDetected')) {
        throw 'Directed-edge repair differs from the reviewed version; inspect manually.'
    }
} else {
    # Match only the old termination block inside this function, without bundling vendor source.
    $loopPattern = '(?s)\t\t\tbool bLoopDetected = false;.*?(?=\r?\n\t\t}\r?\n\t\telse)'
    if ([regex]::Matches($body, $loopPattern).Count -ne 1 -or
        [regex]::Matches($body, [regex]::Escape('bool bContoureFound = false;')).Count -ne 1 -or
        [regex]::Matches($body, [regex]::Escape('Boundary.Add({ VID, Candidate.VID });')).Count -ne 1) {
        throw 'Expected exactly one reviewed termination block. No source was changed.'
    }
    $body = [regex]::Replace($body, $loopPattern, '')
    $body = $body.Replace('bool bContoureFound = false;', "bool bContoureFound = false;$newline`tTSet<uint64> TraversedEdges;")
    $body = $body.Replace("`t`t`tBoundary.Add({ VID, Candidate.VID });", "$edgeGuard$newline`t`t`tBoundary.Add({ VID, Candidate.VID });")
    $updated = $updated.Substring(0, $functionStart) + $body + $updated.Substring($functionEnd)
}
if ($updated -ceq $source) { Write-Output 'Both repairs already present; source unchanged.'; return }
if (-not $Apply) { Write-Output 'Reviewed repair sites found. Preview only; use -Apply and a controlled -BackupDirectory to modify them.'; return }
if (-not $BackupDirectory) { throw '-Apply requires an existing controlled -BackupDirectory.' }
$backupRoot = (Resolve-Path -LiteralPath $BackupDirectory).Path
if (-not (Test-Path -LiteralPath $backupRoot -PathType Container)) { throw 'BackupDirectory must be a directory.' }
$backupPath = Join-Path $backupRoot 'MetaRoad-OpUtils.cpp.before-174-176'
if (Test-Path -LiteralPath $backupPath) { throw "Refusing to overwrite an existing backup: $backupPath" }
if ($PSCmdlet.ShouldProcess($sourcePath, 'Apply OWS #174/#176 boundary repairs')) {
    [IO.File]::WriteAllBytes($backupPath, $sourceBytes)
    [IO.File]::WriteAllText($sourcePath, $updated, [Text.UTF8Encoding]::new($hasBom))
    Write-Output "Source repaired. Backup: $backupPath. Recompile and verify before claiming the running plugin is repaired."
}

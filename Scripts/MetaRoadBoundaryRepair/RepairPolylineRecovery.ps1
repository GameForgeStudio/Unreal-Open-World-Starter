[CmdletBinding(SupportsShouldProcess)]
param([Parameter(Mandatory)][string]$PluginDirectory, [switch]$Apply, [string]$BackupDirectory)
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path -LiteralPath $PluginDirectory).Path
$manifest = Get-Content -LiteralPath (Join-Path $root 'MetaRoad.uplugin') -Raw | ConvertFrom-Json
if ($manifest.VersionName -ne '3.2.0') { throw 'Only reviewed MetaRoad 3.2.0 is supported.' }
$ordered = @'
	if (LineInfo.SourceVertices.Num() > 1)
	{
		// Recover each authored segment, not a shortest route across the whole line.
		TArray<int> Ordered;
		Ordered.Add(LineInfo.SourceVertices[0]);
		for (int I = 1; I < LineInfo.SourceVertices.Num(); ++I)
		{
			const int A = LineInfo.SourceVertices[I - 1];
			const int B = LineInfo.SourceVertices[I];
			if (!Adj.Contains(A) || !Adj.Contains(B)) return false;
			if (A == B) continue;
			TArray<int> Segment = Dijkstra(Adj, A, B);
			if (Segment.Num() < 2) return false;
			Ordered.Append(Segment.GetData() + 1, Segment.Num() - 1);
		}
		VIDs = MoveTemp(Ordered);
		return VIDs.Num() > 0;
	}

'@
$edits = @(
    @{ File='Public/RoadMeshBuild/ProceduralPolygon.h'; Old="`t`tint VID_B = -1;"; New="`t`tint VID_B = -1;`n`t`t// Preserve authored order through self-overlaps; endpoints alone can shortcut a loop.`n`t`tTArray<int> SourceVertices;" },
    @{ File='Private/RoadMeshBuild/RoadArrangementBuilder.cpp'; Old="`tInfo.VID_B = Data.Arrangement->FindExistingVertex(Points.Last());"; New="`tInfo.VID_B = Data.Arrangement->FindExistingVertex(Points.Last());`n`tfor (const FVector2D& Point : Points)`n`t{`n`t`tInfo.SourceVertices.Add(Data.Arrangement->FindExistingVertex(Point));`n`t}" },
    @{ File='Private/RoadMeshBuild/PolylineRecovery.cpp'; Old="`t`tif (d > dist[u]) continue; // Already found a shorter path"; New="`t`tif (d > dist[u]) continue; // Already found a shorter path`n`t`tif (u == Destination) break;" },
    @{ File='Private/RoadMeshBuild/PolylineRecovery.cpp'; Old="`tif (LineInfo.IsLoop())"; New=($ordered.TrimEnd() + "`n`n`tif (LineInfo.IsLoop())") }
)
$files = @{}
$present = 0
foreach ($edit in $edits) {
    $path = Join-Path $root ('Source/MetaRoadEditor/' + $edit.File)
    if (-not $files.ContainsKey($path)) {
        $bytes = [IO.File]::ReadAllBytes($path)
        $source = [IO.File]::ReadAllText($path)
        $files[$path] = @{ Bytes=$bytes; Source=$source; Newline=$(if ($source.Contains("`r`n")) { "`r`n" } else { "`n" }); Bom=($bytes.Length -ge 3 -and $bytes[0] -eq 239 -and $bytes[1] -eq 187 -and $bytes[2] -eq 191) }
    }
    $entry = $files[$path]
    $old = $edit.Old.Replace("`r`n", "`n").Replace("`n", $entry.Newline)
    $new = $edit.New.Replace("`r`n", "`n").Replace("`n", $entry.Newline)
    if ($entry.Source.Contains($new)) { $present++; continue }
    if ([regex]::Matches($entry.Source, [regex]::Escape($old)).Count -ne 1) { throw "Source drift at $path; nothing changed." }
    $entry.Source = $entry.Source.Replace($old, $new)
}
if ($present -eq $edits.Count) { 'Ordered polyline recovery already present; unchanged.'; return }
if ($present -ne 0) { throw 'Partial repair found; nothing changed.' }
if (-not $Apply) { 'Reviewed source found; preview only. Use -Apply with a controlled -BackupDirectory.'; return }
if (-not $BackupDirectory) { throw 'An existing controlled BackupDirectory is required.' }
$backupRoot = (Resolve-Path -LiteralPath $BackupDirectory).Path
if (-not (Test-Path -LiteralPath $backupRoot -PathType Container)) { throw 'BackupDirectory must be a directory.' }
foreach ($path in $files.Keys) {
    $backup = Join-Path $backupRoot ((Split-Path $path -Leaf) + '.before-181')
    if (Test-Path -LiteralPath $backup) { throw "Refusing to overwrite $backup" }
}
if ($PSCmdlet.ShouldProcess($root, 'Preserve authored road samples during polyline recovery (OWS #181)')) {
    foreach ($path in $files.Keys) {
        $entry = $files[$path]
        [IO.File]::WriteAllBytes((Join-Path $backupRoot ((Split-Path $path -Leaf) + '.before-181')), $entry.Bytes)
    }
    foreach ($path in $files.Keys) {
        $entry = $files[$path]
        [IO.File]::WriteAllText($path, $entry.Source, [Text.UTF8Encoding]::new($entry.Bom))
    }
    'Source updated. Rebuild MetaRoadEditor and all dependent modules before use.'
}

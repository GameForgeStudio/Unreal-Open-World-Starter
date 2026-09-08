[CmdletBinding(SupportsShouldProcess)]
param([Parameter(Mandatory)][string]$PluginDirectory, [switch]$Apply, [string]$BackupDirectory)
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path -LiteralPath $PluginDirectory).Path
$manifest = Get-Content -LiteralPath (Join-Path $root 'MetaRoad.uplugin') -Raw | ConvertFrom-Json
if ($manifest.VersionName -ne '3.2.0') { throw 'Only reviewed MetaRoad 3.2.0 is supported.' }
$path = Join-Path $root 'Source/MetaRoadEditor/Private/Utils/OpUtils.cpp'
$source = [IO.File]::ReadAllText($path)
if ($source.Contains('// OWS #183: dangling edges')) { 'Repair already present; unchanged.'; return }
$old = 'bool OpUtils::FindBoundary(const MetaRoad::FDynamicGraph2d& Graph, const TArray<FIndex2i>& SkipEdges, TArray<FIndex2i>& Boundary, const TGIDFilter& GIDFilter)'
$anchor = $old + "`n{`n`tBoundary.Empty();"
$normalized = $source.Replace("`r`n", "`n")
if ([regex]::Matches($normalized, [regex]::Escape($anchor)).Count -ne 1) { throw 'Source drift; nothing changed.' }
$repair = @'

	// OWS #183: dangling edges cannot bound area. Exclude them only from this
	// boundary walk; keep the original graph and its triangulation constraints.
	TArray<FIndex2i> SkipEdges = InputSkipEdges;
	TSet<int> EligibleEdges;
	TMap<int, int> Degree;
	for (int EID : Graph.EdgeIndices())
	{
		const auto Edge = Graph.GetEdgeCopy(EID);
		if (GIDFilter(Edge.Group) && !IsExist(InputSkipEdges, { Edge.A, Edge.B }))
		{
			EligibleEdges.Add(EID);
			++Degree.FindOrAdd(Edge.A);
			++Degree.FindOrAdd(Edge.B);
		}
	}
	TArray<int> Leaves;
	for (const auto& Entry : Degree)
	{
		if (Entry.Value == 1) Leaves.Add(Entry.Key);
	}
	for (int I = 0; I < Leaves.Num(); ++I)
	{
		for (int EID : Graph.VtxEdgesItr(Leaves[I]))
		{
			if (!EligibleEdges.Remove(EID)) continue;
			const auto Edge = Graph.GetEdgeCopy(EID);
			SkipEdges.Add({ Edge.A, Edge.B });
			for (int V : { Edge.A, Edge.B })
			{
				if (--Degree.FindChecked(V) == 1) Leaves.Add(V);
			}
		}
	}
'@
$replacement = $anchor.Replace('& SkipEdges,', '& InputSkipEdges,') + "`n" + $repair
if (-not $Apply) { 'Reviewed anchor found; preview only. Use -Apply and a controlled -BackupDirectory.'; return }
if (-not $BackupDirectory) { throw 'An existing controlled BackupDirectory is required.' }
$backupRoot = (Resolve-Path -LiteralPath $BackupDirectory).Path
if (-not (Test-Path -LiteralPath $backupRoot -PathType Container)) { throw 'BackupDirectory must be a directory.' }
$backup = Join-Path $backupRoot 'OpUtils.cpp.before-183'
if (Test-Path -LiteralPath $backup) { throw "Refusing to overwrite $backup" }
if ($PSCmdlet.ShouldProcess($path, 'Repair boundary-only dangling edge selection (OWS #183)')) {
    $bytes = [IO.File]::ReadAllBytes($path)
    [IO.File]::WriteAllBytes($backup, $bytes)
    $updated = $normalized.Replace($anchor, $replacement)
    if ($source.Contains("`r`n")) { $updated = $updated.Replace("`n", "`r`n") }
    $bom = $bytes.Length -ge 3 -and $bytes[0] -eq 239 -and $bytes[1] -eq 187 -and $bytes[2] -eq 191
    [IO.File]::WriteAllText($path, $updated, [Text.UTF8Encoding]::new($bom))
    'Source updated. Rebuild MetaRoadEditor; this repair changes no public types.'
}

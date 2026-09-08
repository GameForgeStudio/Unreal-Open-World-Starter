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
$path = Join-Path $root 'Source/MetaRoadEditor/Public/Geometry/DynamicGraph.h'
$bytes = [IO.File]::ReadAllBytes($path)
$bom = $bytes.Length -ge 3 -and $bytes[0] -eq 239 -and $bytes[1] -eq 187 -and $bytes[2] -eq 191
$source = [IO.File]::ReadAllText($path)
$nl = if ($source.Contains("`r`n")) { "`r`n" } else { "`n" }
$old = "`t`tauto& el = edges.ElementAt(EID, FEdge{ A, B, GID, PID });"
$new = "`t`t// OWS #178: allocation may reuse a slot; overwrite its old edge data.$nl`t`tedges.InsertAt(FEdge{ A, B, GID, PID }, EID);"
if ($source.Contains($new)) { 'Edge-slot repair already present; source unchanged.'; return }
if ($source.Contains('// OWS #178:') -or [regex]::Matches($source, [regex]::Escape($old)).Count -ne 1) {
    throw 'Expected exactly one reviewed allocator statement. No source was changed.'
}
if (-not $Apply) { 'Reviewed allocator found. Preview only; use -Apply and a controlled -BackupDirectory.'; return }
if (-not $BackupDirectory) { throw '-Apply requires an existing controlled -BackupDirectory.' }
$backupRoot = (Resolve-Path -LiteralPath $BackupDirectory).Path
if (-not (Test-Path -LiteralPath $backupRoot -PathType Container)) { throw 'BackupDirectory must be a directory.' }
$backup = Join-Path $backupRoot 'MetaRoad-DynamicGraph.h.before-178'
if (Test-Path -LiteralPath $backup) { throw 'Refusing to overwrite an existing backup.' }
if ($PSCmdlet.ShouldProcess($path, 'Restore recycled-edge initialization (OWS #178)')) {
    [IO.File]::WriteAllBytes($backup, $bytes)
    [IO.File]::WriteAllText($path, $source.Replace($old, $new), [Text.UTF8Encoding]::new($bom))
    "Source repaired. Backup: $backup. Rebuild all MetaRoadEditor and consumer code using this inline graph header."
}

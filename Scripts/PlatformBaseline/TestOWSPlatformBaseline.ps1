[CmdletBinding()]
param()

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$ProjectRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
$ModulePath = Join-Path $PSScriptRoot 'OWSPlatformBaseline.psm1'
$SchemaPath = Join-Path $PSScriptRoot 'ows-platform-baseline.schema.json'
$KnownGapsPath = Join-Path $PSScriptRoot 'known-gaps.json'
$ScratchRoot = Join-Path ([System.IO.Path]::GetTempPath()) ('ows-platform-baseline-self-test-' + [guid]::NewGuid().ToString('N'))
$FixtureRoot = Join-Path $ScratchRoot 'fixture'
$Failures = New-Object System.Collections.Generic.List[string]

Import-Module -Force $ModulePath

function Assert-OWSTest {
    param([bool]$Condition, [string]$Message)
    if (-not $Condition) { $script:Failures.Add($Message) }
}

function Write-OWSTestFile {
    param([string]$RelativePath, [AllowEmptyString()][string]$Content)
    $Path = Join-Path $FixtureRoot $RelativePath
    [void][System.IO.Directory]::CreateDirectory([System.IO.Path]::GetDirectoryName($Path))
    [System.IO.File]::WriteAllText($Path, $Content, (New-Object System.Text.UTF8Encoding($false)))
}

function Invoke-OWSTestGit {
    param([string[]]$Arguments)
    $PriorPreference = $ErrorActionPreference
    try {
        $ErrorActionPreference = 'Continue'
        & git -c core.longpaths=true -C $FixtureRoot @Arguments 2>$null | Out-Null
        $GitExitCode = $LASTEXITCODE
    }
    finally {
        $ErrorActionPreference = $PriorPreference
    }
    if ($GitExitCode -ne 0) { throw "Fixture Git command failed: git $($Arguments -join ' ')" }
}

function Copy-OWSTestObject {
    param($InputObject)
    return ($InputObject | ConvertTo-Json -Depth 100 | ConvertFrom-Json)
}

try {
    [void][System.IO.Directory]::CreateDirectory($FixtureRoot)
    Write-OWSTestFile 'OWS.uproject' @'
{
  "FileVersion": 3,
  "EngineAssociation": "5.8",
  "Modules": [{ "Name": "OWS", "Type": "Runtime", "LoadingPhase": "Default" }],
  "Plugins": [
    { "Name": "OWSFramework", "Enabled": true },
    { "Name": "SaveExtension", "Enabled": true },
    { "Name": "SigilInventory", "Enabled": true }
  ]
}
'@
    Write-OWSTestFile 'Source/OWS/OWS.Build.cs' @'
using UnrealBuildTool;
public class OWS : ModuleRules {
  public OWS(ReadOnlyTargetRules Target) : base(Target) {
    PublicDependencyModuleNames.AddRange(new string[] { "Core", "KinetiForge" });
    if (Target.bBuildEditor) { PrivateDependencyModuleNames.Add("UnrealEd"); }
  }
}
'@
    Write-OWSTestFile 'Source/OWS/Public/FixtureVehicle.h' @'
#pragma once
class UModularVehicleBaseComponent;
class UVehicleDriveAssemblyComponent;
'@
    Write-OWSTestFile 'Source/OWS/Private/Tests/FixtureTest.cpp' @'
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFixtureTest, "OWS.Fixture.Test", 0)
#endif
'@
    Write-OWSTestFile 'Plugins/OWSFramework/OWSFramework.uplugin' @'
{ "FileVersion": 3, "CanContainContent": true, "Modules": [{ "Name": "OWSCore", "Type": "Runtime", "LoadingPhase": "Default" }] }
'@
    Write-OWSTestFile 'Plugins/OWSFramework/Source/OWSCore/OWSCore.Build.cs' @'
using UnrealBuildTool;
public class OWSCore : ModuleRules { public OWSCore(ReadOnlyTargetRules Target) : base(Target) { PublicDependencyModuleNames.Add("Core"); } }
'@
    Write-OWSTestFile 'Plugins/OWSFramework/Source/OWSCore/Private/Fixture.cpp' @'
const TCHAR* ProjectPath = TEXT("/Game/OWS/Fixture.Asset");
const TCHAR* CharacterPath = TEXT("/GASPALS/Input/Fixture.Asset");
'@
    Write-OWSTestFile 'Plugins/SaveExtension/SaveExtension.uplugin' @'
{
  "FileVersion": 3,
  "Modules": [
    { "Name": "SaveExtension", "Type": "Runtime", "LoadingPhase": "Default" },
    { "Name": "SaveExtensionEditor", "Type": "Editor", "LoadingPhase": "Default" }
  ]
}
'@
    Write-OWSTestFile 'Plugins/SaveExtension/Source/SaveExtension/SaveExtension.Build.cs' @'
using UnrealBuildTool;
public class SaveExtension : ModuleRules {
  public SaveExtension(ReadOnlyTargetRules Target) : base(Target) {
    if (Target.bBuildEditor) { PrivateDependencyModuleNames.Add("UnrealEd"); }
  }
}
'@
    Write-OWSTestFile 'Plugins/SaveExtension/Source/SaveExtensionEditor/Private/Fixture.cpp' @'
#include "Editor/ClassViewer/Private/ClassViewerNode.h"
'@
    Write-OWSTestFile 'Plugins/SigilInventory/SigilInventory.uplugin' @'
{ "FileVersion": 3, "Modules": [{ "Name": "SigilInventory", "Type": "Runtime", "LoadingPhase": "Default" }] }
'@
    Write-OWSTestFile 'Plugins/SigilInventory/Source/SigilInventory/SigilInventory.Build.cs' @'
using UnrealBuildTool;
public class SigilInventory : ModuleRules { public SigilInventory(ReadOnlyTargetRules Target) : base(Target) { PublicDependencyModuleNames.Add("Core"); } }
'@
    Write-OWSTestFile 'Plugins/SigilInventory/Source/SigilInventory/Public/Fixture.h' @'
#include "StructUtils/InstancedStruct.h"
'@
    Write-OWSTestFile 'Config/DefaultEngine.ini' @'
[Fixture]
SharedSetting=project
+DirectoriesToAlwaysCook=(Path="/Engine/Plugins/EnhancedInput/Content/Debug")

[/Script/AndroidFileServerEditor.AndroidFileServerRuntimeSettings]
SecurityToken=DO_NOT_PRINT_THIS_SYNTHETIC_VALUE
'@
    Write-OWSTestFile 'Plugins/OWSFramework/Config/DefaultEngine.ini' @'
[Fixture]
SharedSetting=plugin
'@
    Write-OWSTestFile 'Content/Fixture.uasset' "binary-prefix`0/Game/OWS/Fixture`0binary-suffix"

    Invoke-OWSTestGit @('init', '--initial-branch=main')
    Invoke-OWSTestGit @('config', 'core.autocrlf', 'false')
    Invoke-OWSTestGit @('add', '--all')
    Invoke-OWSTestGit @('-c', 'user.name=OWS Baseline Self-Test', '-c', 'user.email=self-test@example.invalid', 'commit', '-m', 'fixture')

    $FixedUtc = [DateTime]::Parse('2026-01-02T03:04:05Z').ToUniversalTime()
    $ReportA = New-OWSStaticBaselineReport -RepositoryRoot $FixtureRoot -KnownGapsPath $KnownGapsPath -GeneratedUtc $FixedUtc -StartedUtc $FixedUtc -StaticOnly
    $ReportB = New-OWSStaticBaselineReport -RepositoryRoot $FixtureRoot -KnownGapsPath $KnownGapsPath -GeneratedUtc $FixedUtc -StartedUtc $FixedUtc -StaticOnly
    $JsonA = ConvertTo-OWSJson -InputObject $ReportA
    $JsonB = ConvertTo-OWSJson -InputObject $ReportB
    Assert-OWSTest ($JsonA -ceq $JsonB) 'Identical fixture inputs did not produce byte-identical JSON.'

    $Validation = Test-OWSBaselineReport -Report $ReportA -RepositoryRoot $FixtureRoot -SchemaPath $SchemaPath
    Assert-OWSTest $Validation.valid ('Valid fixture report failed validation: ' + ($Validation.errors -join '; '))
    Assert-OWSTest (-not $JsonA.Contains('DO_NOT_PRINT_THIS_SYNTHETIC_VALUE')) 'Sensitive fixture value leaked into report JSON.'
    $SensitiveRows = @($ReportA.inventories.config_keys | Where-Object { $_.sensitive })
    Assert-OWSTest ($SensitiveRows.Count -eq 1) 'Sensitive-key presence was not inventoried exactly once.'
    Assert-OWSTest ($null -eq $SensitiveRows[0].value_digest) 'Sensitive-key inventory emitted a value-derived digest.'

    $ExpectedOwners = [ordered]@{
        'runtime-test-editor-mixing' = '116,122'
        'owscore-reverse-content-references' = '116,117,119,121,131'
        'ows-public-provider-leakage' = '108,120,122,163'
        'saveextension-private-classviewer-include' = '134'
        'configuration-and-cook-ownership' = '107,108,117,121,123'
        'android-file-server-sensitive-key' = '114'
        'sigil-structutils-transitive-dependency' = '129'
    }
    foreach ($FamilyId in $ExpectedOwners.Keys) {
        $Finding = @($ReportA.findings | Where-Object { $_.family_id -eq $FamilyId -and $_.classification -eq 'known_gap' })
        Assert-OWSTest ($Finding.Count -eq 1) "Known-gap family was not classified exactly once: $FamilyId"
        if ($Finding.Count -eq 1) {
            $OwnerList = @($Finding[0].owners.issue | Sort-Object) -join ','
            Assert-OWSTest ($OwnerList -eq $ExpectedOwners[$FamilyId]) "Owner mapping differed for $FamilyId."
        }
    }
    $BinaryObservation = @($ReportA.findings | Where-Object { $_.family_id -eq 'binary-game-reference-leads' -and $_.classification -eq 'observation' })
    Assert-OWSTest ($BinaryObservation.Count -eq 1) 'Binary /Game lead was not retained as an observation.'
    Assert-OWSTest ($ReportA.summary.candidate_resolved_count -eq 0) 'Fixture known gaps were incorrectly collapsed into candidate_resolved rows.'
    Assert-OWSTest ($ReportA.summary.new_unowned_gap_count -eq 0) 'Mapped fixture findings were incorrectly classified as unowned.'

    $Malformed = Copy-OWSTestObject -InputObject $ReportA
    $Malformed.PSObject.Properties.Remove('claim_scope')
    $MalformedValidation = Test-OWSBaselineReport -Report $Malformed -RepositoryRoot $FixtureRoot -SchemaPath $SchemaPath
    Assert-OWSTest (-not $MalformedValidation.valid) 'Schema validation accepted a report missing claim_scope.'

    $NAReport = Copy-OWSTestObject -InputObject $ReportA
    $NAReport.results | Add-Member -NotePropertyName outside_scope_fixture -NotePropertyValue ([pscustomobject]@{ status = 'N/A'; detail = 'Provably outside this fixture.' })
    $NAValidation = Test-OWSBaselineReport -Report $NAReport -RepositoryRoot $FixtureRoot -SchemaPath $SchemaPath
    Assert-OWSTest $NAValidation.valid ('Schema rejected the documented N/A result: ' + ($NAValidation.errors -join '; '))

    $PackagePassReport = Copy-OWSTestObject -InputObject $ReportA
    $PackagePassReport.results.package = [pscustomobject]@{
        status = 'PASS'
        detail = 'Synthetic package evidence passed.'
        package_store_marker = [pscustomobject]@{
            kind = 'zen_project_store'
            path = '$WORKTREE\Saved\Cooked\Windows\ue.projectstore'
            bytes = 21
            sha256 = ('a' * 64)
        }
    }
    $PackagePassValidation = Test-OWSBaselineReport -Report $PackagePassReport -RepositoryRoot $FixtureRoot -SchemaPath $SchemaPath
    Assert-OWSTest $PackagePassValidation.valid ('Schema rejected a package PASS with safe package-store marker evidence: ' + ($PackagePassValidation.errors -join '; '))

    $PackagePassWithoutMarker = Copy-OWSTestObject -InputObject $PackagePassReport
    $PackagePassWithoutMarker.results.package.PSObject.Properties.Remove('package_store_marker')
    $PackagePassWithoutMarkerValidation = Test-OWSBaselineReport -Report $PackagePassWithoutMarker -RepositoryRoot $FixtureRoot -SchemaPath $SchemaPath
    Assert-OWSTest (-not $PackagePassWithoutMarkerValidation.valid) 'Schema accepted a package PASS without package-store marker evidence.'

    $KnownFixture = [pscustomobject]@{ families = @([pscustomobject]@{ id = 'absent-family'; classification = 'known_gap'; owners = @([pscustomobject]@{ issue = 999; url = 'https://github.com/example/example/issues/999' }) }) }
    $Candidate = @(Resolve-OWSFindings -RawFindings @() -KnownGaps $KnownFixture)
    Assert-OWSTest ($Candidate.Count -eq 1 -and $Candidate[0].classification -eq 'candidate_resolved') 'Absent known gap did not become candidate_resolved.'

    $ManifestRoot = Join-Path $ScratchRoot 'manifest'
    [void][System.IO.Directory]::CreateDirectory((Join-Path $ManifestRoot 'nested'))
    [System.IO.File]::WriteAllText((Join-Path $ManifestRoot 'a.txt'), 'alpha')
    [System.IO.File]::WriteAllText((Join-Path $ManifestRoot 'nested\b.txt'), 'beta')
    $Manifest = Get-OWSFileManifest -RootPath $ManifestRoot
    Assert-OWSTest ($Manifest.status -eq 'PASS' -and $Manifest.count -eq 2 -and $Manifest.bytes -eq 9) 'File manifest did not capture deterministic count/bytes evidence.'

    $PackageRoot = Join-Path $ScratchRoot 'package-evidence'
    $PackageStageRoot = Join-Path $PackageRoot 'stage'
    $PackageArchiveRoot = Join-Path $PackageRoot 'archive'
    $PackageCookRoot = Join-Path $PackageRoot 'cook'
    $PackageMetadataRoot = Join-Path $PackageCookRoot 'OWS\Metadata'
    foreach ($Directory in @(
        $PackageStageRoot,
        $PackageArchiveRoot,
        $PackageCookRoot,
        $PackageMetadataRoot
    )) {
        [void][System.IO.Directory]::CreateDirectory($Directory)
    }
    [System.IO.File]::WriteAllText((Join-Path $PackageStageRoot 'stage.txt'), 'stage')
    foreach ($Name in @('OWS.exe', 'pakchunk0.utoc', 'pakchunk0.ucas', 'pakchunk0.pak')) {
        [System.IO.File]::WriteAllText((Join-Path $PackageArchiveRoot $Name), $Name)
    }
    foreach ($Name in @(
        'DevelopmentAssetRegistry.bin',
        'DevelopmentAssetRegistryStaged.bin',
        'CookMetadata.ucookmeta',
        'plugin_sizes.csv',
        'plugin_size_jsons.json'
    )) {
        [System.IO.File]::WriteAllText((Join-Path $PackageMetadataRoot $Name), $Name)
    }

    $NoPackageStoreMarker = Test-OWSPackageEvidence `
        -StageRoot $PackageStageRoot `
        -ArchiveRoot $PackageArchiveRoot `
        -PlatformCookRoot $PackageCookRoot `
        -MetadataRoot $PackageMetadataRoot
    Assert-OWSTest (-not $NoPackageStoreMarker.valid) 'Package evidence accepted no package-store marker.'
    Assert-OWSTest (
        @($NoPackageStoreMarker.missing).Count -eq 1 -and
        $NoPackageStoreMarker.missing[0] -match 'ue\.projectstore.+OR.+packagestore\.manifest'
    ) 'Missing package-store alternatives were not reported as one OR requirement.'

    $ZenMarkerPath = Join-Path $PackageCookRoot 'ue.projectstore'
    $SensitiveZenProjectStore = '{"zenserver":{"hostauth":{"password":"DO_NOT_PRINT_THIS_SYNTHETIC_VALUE"}}}'
    [System.IO.File]::WriteAllText($ZenMarkerPath, $SensitiveZenProjectStore)
    $UnsanitizedZenRejected = $false
    try {
        [void](Test-OWSPackageEvidence `
            -StageRoot $PackageStageRoot `
            -ArchiveRoot $PackageArchiveRoot `
            -PlatformCookRoot $PackageCookRoot `
            -MetadataRoot $PackageMetadataRoot)
    }
    catch {
        $UnsanitizedZenRejected = $_.Exception.Message -eq 'The Zen project-store marker has not been sanitized.'
    }
    Assert-OWSTest $UnsanitizedZenRejected 'Package evidence did not reject an unsanitized credential-bearing Zen project store with a content-free error.'

    $SanitizedZenProjectStore = ConvertTo-OWSSanitizedText -Text $SensitiveZenProjectStore
    [System.IO.File]::WriteAllText($ZenMarkerPath, $SanitizedZenProjectStore)
    Assert-OWSTest (-not (Get-Content -Raw -LiteralPath $ZenMarkerPath).Contains('DO_NOT_PRINT_THIS_SYNTHETIC_VALUE')) 'Synthetic Zen project-store credential was not sanitized.'
    $ZenPackageEvidence = Test-OWSPackageEvidence `
        -StageRoot $PackageStageRoot `
        -ArchiveRoot $PackageArchiveRoot `
        -PlatformCookRoot $PackageCookRoot `
        -MetadataRoot $PackageMetadataRoot
    Assert-OWSTest $ZenPackageEvidence.valid 'Package evidence rejected the Zen project-store marker.'
    Assert-OWSTest (
        $ZenPackageEvidence.package_store_marker.kind -eq 'zen_project_store' -and
        $ZenPackageEvidence.package_store_marker.path -eq $ZenMarkerPath -and
        $ZenPackageEvidence.package_store_marker.bytes -gt 0 -and
        $ZenPackageEvidence.package_store_marker.sha256 -eq (Get-OWSFileSha256 -Path $ZenMarkerPath)
    ) 'Zen project-store evidence did not retain safe marker metadata and checksum.'

    $LooseMarkerPath = Join-Path $PackageMetadataRoot 'packagestore.manifest'
    [System.IO.File]::WriteAllText($LooseMarkerPath, 'synthetic loose package store')
    [System.IO.File]::WriteAllText($ZenMarkerPath, '{"project":"stale-without-zenserver"}')
    $LoosePackageEvidence = Test-OWSPackageEvidence `
        -StageRoot $PackageStageRoot `
        -ArchiveRoot $PackageArchiveRoot `
        -PlatformCookRoot $PackageCookRoot `
        -MetadataRoot $PackageMetadataRoot
    Assert-OWSTest $LoosePackageEvidence.valid 'Package evidence did not fall through an invalid Zen marker to the loose package-store manifest.'
    Assert-OWSTest (
        $LoosePackageEvidence.package_store_marker.kind -eq 'loose_package_store_manifest' -and
        $LoosePackageEvidence.package_store_marker.path -eq $LooseMarkerPath -and
        $LoosePackageEvidence.package_store_marker.bytes -gt 0 -and
        $LoosePackageEvidence.package_store_marker.sha256 -eq (Get-OWSFileSha256 -Path $LooseMarkerPath)
    ) 'Loose package-store evidence did not retain safe marker metadata and checksum.'

    [System.IO.File]::WriteAllText($ZenMarkerPath, '')
    $EmptyZenRejected = $false
    try {
        [void](Test-OWSPackageEvidence `
            -StageRoot $PackageStageRoot `
            -ArchiveRoot $PackageArchiveRoot `
            -PlatformCookRoot $PackageCookRoot `
            -MetadataRoot $PackageMetadataRoot)
    }
    catch {
        $EmptyZenRejected = $_.Exception.Message -eq 'The Zen project-store marker is empty.'
    }
    Assert-OWSTest $EmptyZenRejected 'Package evidence fell through an empty Zen marker instead of rejecting it.'

    [System.IO.File]::WriteAllText($ZenMarkerPath, '{')
    $MalformedZenRejected = $false
    try {
        [void](Test-OWSPackageEvidence `
            -StageRoot $PackageStageRoot `
            -ArchiveRoot $PackageArchiveRoot `
            -PlatformCookRoot $PackageCookRoot `
            -MetadataRoot $PackageMetadataRoot)
    }
    catch {
        $MalformedZenRejected = $_.Exception.Message -eq 'The Zen project-store marker is not valid JSON.'
    }
    Assert-OWSTest $MalformedZenRejected 'Package evidence fell through a malformed Zen marker instead of rejecting it with a content-free error.'

    [System.IO.File]::Delete($ZenMarkerPath)
    [System.IO.File]::WriteAllText($LooseMarkerPath, '')
    $EmptyPackageStoreMarker = Test-OWSPackageEvidence `
        -StageRoot $PackageStageRoot `
        -ArchiveRoot $PackageArchiveRoot `
        -PlatformCookRoot $PackageCookRoot `
        -MetadataRoot $PackageMetadataRoot
    Assert-OWSTest (-not $EmptyPackageStoreMarker.valid) 'Package evidence accepted an empty package-store marker.'

    $InsideRejected = $false
    try { [void](Assert-OWSExternalOutputPath -RepositoryRoot $FixtureRoot -OutputPath (Join-Path $FixtureRoot 'report.json')) }
    catch { $InsideRejected = $true }
    Assert-OWSTest $InsideRejected 'Output-path guard accepted a report inside the repository.'
    $OutsidePath = Assert-OWSExternalOutputPath -RepositoryRoot $FixtureRoot -OutputPath (Join-Path $ScratchRoot 'report.json')
    Assert-OWSTest ($OutsidePath -eq [System.IO.Path]::GetFullPath((Join-Path $ScratchRoot 'report.json'))) 'Output-path guard changed a valid outside path.'

    $StateBefore = Get-OWSRepositoryState -RepositoryRoot $FixtureRoot
    [System.IO.File]::AppendAllText((Join-Path $FixtureRoot 'Config\DefaultEngine.ini'), "`nMutation=tracked`n")
    $StateAfter = Get-OWSRepositoryState -RepositoryRoot $FixtureRoot
    Assert-OWSTest (-not (Compare-OWSRepositoryState -Before $StateBefore -After $StateAfter)) 'Working-state digest did not detect changed tracked bytes.'
    $RuntimeDigestAfterTracked = $StateAfter.runtime_input_digest
    Write-OWSTestFile 'Source/OWS/Private/Untracked.cpp' 'int UntrackedFixture = 1;'
    $StateAfterUntracked = Get-OWSRepositoryState -RepositoryRoot $FixtureRoot
    Assert-OWSTest ($RuntimeDigestAfterTracked -ne $StateAfterUntracked.runtime_input_digest) 'Runtime-input digest did not detect an untracked source file.'

    $Sanitized = ConvertTo-OWSSanitizedText -Text "Authorization: Bearer synthetic bearer value`nAuthorization: Basic synthetic basic value`npassword='multi word synthetic value'`nsecret=unquoted multi word synthetic tail`napi_key=synthetic-value`nhttps://example.invalid/download?X-Amz-Signature=signed-url-value&safe=1`n-----BEGIN PRIVATE KEY-----`nsynthetic-private-key-body`n-----END PRIVATE KEY-----"
    Assert-OWSTest (-not $Sanitized.Contains('synthetic bearer value')) 'Bearer credential was not redacted.'
    Assert-OWSTest (-not $Sanitized.Contains('synthetic basic value')) 'Basic credential was not redacted.'
    Assert-OWSTest (-not $Sanitized.Contains('multi word synthetic value')) 'Quoted multi-word credential was not redacted.'
    Assert-OWSTest (-not $Sanitized.Contains('unquoted multi word synthetic tail')) 'Unquoted multi-word credential tail was not redacted.'
    Assert-OWSTest (-not $Sanitized.Contains('synthetic-value')) 'Assignment credential was not redacted.'
    Assert-OWSTest (-not $Sanitized.Contains('signed-url-value')) 'Signed URL parameter was not redacted.'
    Assert-OWSTest (-not $Sanitized.Contains('synthetic-private-key-body')) 'Private-key block was not redacted.'

    $SignedUrlReport = Copy-OWSTestObject -InputObject $ReportA
    $SignedUrlReport.results.static.detail = 'synthetic delivery probe https://example.invalid/file?X-Amz-Signature=DELIVERY_PROBE_VALUE&safe=1'
    $SignedUrlValidation = Test-OWSBaselineReport -Report $SignedUrlReport -RepositoryRoot $FixtureRoot -SchemaPath $SchemaPath
    Assert-OWSTest (-not $SignedUrlValidation.valid) 'Delivery validation accepted a signed-URL credential value.'

    $PrivateKeyReport = Copy-OWSTestObject -InputObject $ReportA
    $PrivateKeyReport.results.static.detail = "-----BEGIN PRIVATE KEY-----`nDELIVERY_PRIVATE_KEY_PROBE`n-----END PRIVATE KEY-----"
    $PrivateKeyValidation = Test-OWSBaselineReport -Report $PrivateKeyReport -RepositoryRoot $FixtureRoot -SchemaPath $SchemaPath
    Assert-OWSTest (-not $PrivateKeyValidation.valid) 'Delivery validation accepted private-key material.'

    if ($Failures.Count -gt 0) { throw ($Failures -join [Environment]::NewLine) }
    [Console]::WriteLine('[OWS baseline self-test] PASS: deterministic schema, ownership, redaction, digest, manifest, package-marker, and output guards passed without Unreal.')
}
catch {
    [Console]::Error.WriteLine('[OWS baseline self-test] FAIL: ' + (ConvertTo-OWSSanitizedText -Text $_.Exception.Message))
    [Console]::Error.WriteLine((ConvertTo-OWSSanitizedText -Text $_.ScriptStackTrace))
    exit 1
}
finally {
    if (Test-Path -LiteralPath $ScratchRoot -PathType Container) {
        Get-ChildItem -LiteralPath $ScratchRoot -Force -Recurse -ErrorAction SilentlyContinue | ForEach-Object { $_.Attributes = 'Normal' }
        [System.IO.Directory]::Delete($ScratchRoot, $true)
    }
    if (Test-Path -LiteralPath $ScratchRoot) {
        [Console]::Error.WriteLine('[OWS baseline self-test] FAIL: task-owned scratch cleanup did not pass.')
        exit 1
    }
}

exit 0

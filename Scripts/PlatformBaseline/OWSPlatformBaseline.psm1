Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Get-OWSSha256Text {
    [CmdletBinding()]
    param([AllowEmptyString()][string]$Text)

    $Algorithm = [System.Security.Cryptography.SHA256]::Create()
    try {
        $Encoding = New-Object System.Text.UTF8Encoding($false)
        $Bytes = $Encoding.GetBytes($Text)
        return ([System.BitConverter]::ToString($Algorithm.ComputeHash($Bytes))).Replace('-', '').ToLowerInvariant()
    }
    finally {
        $Algorithm.Dispose()
    }
}

function Get-OWSFileSha256 {
    [CmdletBinding()]
    param([Parameter(Mandatory)][string]$Path)

    return (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant()
}

function ConvertTo-OWSNormalizedPath {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)][string]$Root,
        [Parameter(Mandatory)][string]$Path
    )

    $ResolvedRoot = [System.IO.Path]::GetFullPath($Root).TrimEnd('\', '/')
    $ResolvedPath = [System.IO.Path]::GetFullPath($Path)
    if ($ResolvedPath.Equals($ResolvedRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
        return '.'
    }

    $Prefix = $ResolvedRoot + [System.IO.Path]::DirectorySeparatorChar
    if (-not $ResolvedPath.StartsWith($Prefix, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Path is outside the requested root."
    }
    return $ResolvedPath.Substring($Prefix.Length).Replace('\', '/')
}

function Get-OWSObjectPropertyValue {
    param(
        [Parameter(Mandatory)]$Object,
        [Parameter(Mandatory)][string]$Name,
        $Default = $null
    )

    if ($null -eq $Object) {
        return $Default
    }
    if ($Object -is [System.Collections.IDictionary]) {
        if ($Object.Contains($Name)) {
            return $Object[$Name]
        }
        return $Default
    }
    $Property = $Object.PSObject.Properties[$Name]
    if ($null -ne $Property) {
        return $Property.Value
    }
    return $Default
}

function Read-OWSJsonFile {
    [CmdletBinding()]
    param([Parameter(Mandatory)][string]$Path)

    $Text = Get-Content -Raw -LiteralPath $Path
    $Builder = New-Object System.Text.StringBuilder
    $InString = $false
    $Escaped = $false
    for ($Index = 0; $Index -lt $Text.Length; $Index++) {
        $Character = $Text[$Index]
        if ($InString) {
            [void]$Builder.Append($Character)
            if ($Escaped) { $Escaped = $false; continue }
            if ($Character -eq '\') { $Escaped = $true; continue }
            if ($Character -eq '"') { $InString = $false }
            continue
        }
        if ($Character -eq '"') {
            $InString = $true
            [void]$Builder.Append($Character)
            continue
        }
        if ($Character -eq ',') {
            $Lookahead = $Index + 1
            while ($Lookahead -lt $Text.Length -and [char]::IsWhiteSpace($Text[$Lookahead])) { $Lookahead++ }
            if ($Lookahead -lt $Text.Length -and $Text[$Lookahead] -in @('}', ']')) { continue }
        }
        [void]$Builder.Append($Character)
    }
    return ($Builder.ToString() | ConvertFrom-Json)
}

function Invoke-OWSGitLines {
    param(
        [Parameter(Mandatory)][string]$RepositoryRoot,
        [Parameter(Mandatory)][string[]]$Arguments,
        [switch]$AllowNoMatch
    )

    $Output = @(& git -c core.longpaths=true -C $RepositoryRoot @Arguments 2>$null)
    $ExitCode = $LASTEXITCODE
    if ($AllowNoMatch -and $ExitCode -eq 1) {
        return @()
    }
    if ($ExitCode -ne 0) {
        throw "Git command failed while collecting baseline metadata."
    }
    return @($Output | ForEach-Object { [string]$_ })
}

function Test-OWSRuntimeInputPath {
    param([Parameter(Mandatory)][string]$Path)

    $Normalized = $Path.Replace('\', '/').TrimStart('.', '/')
    return (
        $Normalized -eq 'OWS.uproject' -or
        $Normalized.StartsWith('Config/', [System.StringComparison]::OrdinalIgnoreCase) -or
        $Normalized.StartsWith('Content/', [System.StringComparison]::OrdinalIgnoreCase) -or
        $Normalized.StartsWith('Plugins/', [System.StringComparison]::OrdinalIgnoreCase) -or
        $Normalized.StartsWith('Source/', [System.StringComparison]::OrdinalIgnoreCase)
    )
}

function Get-OWSWorkingFileFingerprint {
    param(
        [Parameter(Mandatory)][string]$RepositoryRoot,
        [Parameter(Mandatory)][string]$RelativePath
    )

    $Normalized = $RelativePath.Replace('\', '/')
    $FullPath = Join-Path $RepositoryRoot $Normalized
    if (-not (Test-Path -LiteralPath $FullPath -PathType Leaf)) {
        return "$Normalized`tDELETED"
    }
    $Length = (Get-Item -LiteralPath $FullPath).Length
    return "$Normalized`t$Length`t$(Get-OWSFileSha256 -Path $FullPath)"
}

function Get-OWSRepositoryState {
    [CmdletBinding()]
    param([Parameter(Mandatory)][string]$RepositoryRoot)

    $Root = [System.IO.Path]::GetFullPath($RepositoryRoot)
    $CommitLines = @(Invoke-OWSGitLines -RepositoryRoot $Root -Arguments @('rev-parse', 'HEAD'))
    $TreeLines = @(Invoke-OWSGitLines -RepositoryRoot $Root -Arguments @('rev-parse', 'HEAD^{tree}'))
    $Commit = $CommitLines[0].Trim()
    $Tree = $TreeLines[0].Trim()
    $BranchLines = @(Invoke-OWSGitLines -RepositoryRoot $Root -Arguments @('symbolic-ref', '--quiet', '--short', 'HEAD') -AllowNoMatch)
    $RefLines = @(Invoke-OWSGitLines -RepositoryRoot $Root -Arguments @('symbolic-ref', '--quiet', 'HEAD') -AllowNoMatch)
    $Branch = if ($BranchLines.Count -gt 0) { $BranchLines[0].Trim() } else { $null }
    $Ref = if ($RefLines.Count -gt 0) { $RefLines[0].Trim() } else { 'HEAD' }
    $Status = @(Invoke-OWSGitLines -RepositoryRoot $Root -Arguments @('status', '--porcelain=v1', '--untracked-files=all'))
    $Index = @(Invoke-OWSGitLines -RepositoryRoot $Root -Arguments @('ls-files', '-s'))
    $WorkingPaths = @(Invoke-OWSGitLines -RepositoryRoot $Root -Arguments @('ls-files', '--modified', '--deleted', '--others', '--exclude-standard') | Sort-Object -Unique)
    $WorkingFingerprints = @($WorkingPaths | ForEach-Object {
        Get-OWSWorkingFileFingerprint -RepositoryRoot $Root -RelativePath $_
    })
    $RuntimeIndex = @($Index | Where-Object {
        $Parts = $_ -split "`t", 2
        $Parts.Count -eq 2 -and (Test-OWSRuntimeInputPath -Path $Parts[1])
    })
    $RuntimeWorkingFingerprints = @($WorkingPaths | Where-Object {
        Test-OWSRuntimeInputPath -Path $_
    } | ForEach-Object {
        Get-OWSWorkingFileFingerprint -RepositoryRoot $Root -RelativePath $_
    })

    $StatusMaterial = @(
        "commit=$Commit"
        "tree=$Tree"
        "ref=$Ref"
        "branch=$Branch"
        '--INDEX--'
        @($Index | Sort-Object)
        '--STATUS--'
        @($Status | Sort-Object)
        '--WORKTREE-FILES--'
        @($WorkingFingerprints | Sort-Object)
    ) -join "`n"
    $RuntimeMaterial = @(
        "commit=$Commit"
        '--RUNTIME-INDEX--'
        @($RuntimeIndex | Sort-Object)
        '--RUNTIME-WORKTREE-FILES--'
        @($RuntimeWorkingFingerprints | Sort-Object)
    ) -join "`n"
    return [ordered]@{
        repository = 'GameForgeStudio/Unreal-Open-World-Starter'
        commit = $Commit
        tree = $Tree
        branch = $Branch
        ref = $Ref
        checkout_mode = 'working-tree'
        clean = ($Status.Count -eq 0)
        dirty_entry_count = $Status.Count
        status_digest = Get-OWSSha256Text -Text $StatusMaterial
        runtime_input_digest = Get-OWSSha256Text -Text $RuntimeMaterial
    }
}

function Compare-OWSRepositoryState {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)]$Before,
        [Parameter(Mandatory)]$After
    )

    return (
        $Before.commit -eq $After.commit -and
        $Before.tree -eq $After.tree -and
        $Before.branch -eq $After.branch -and
        $Before.ref -eq $After.ref -and
        $Before.status_digest -eq $After.status_digest -and
        $Before.runtime_input_digest -eq $After.runtime_input_digest
    )
}

function Get-OWSDescriptorInventory {
    [CmdletBinding()]
    param([Parameter(Mandatory)][string]$RepositoryRoot)

    $Root = [System.IO.Path]::GetFullPath($RepositoryRoot)
    $Paths = New-Object System.Collections.Generic.List[string]
    $ProjectPath = Join-Path $Root 'OWS.uproject'
    if (Test-Path -LiteralPath $ProjectPath -PathType Leaf) {
        $Paths.Add($ProjectPath)
    }
    $PluginsPath = Join-Path $Root 'Plugins'
    if (Test-Path -LiteralPath $PluginsPath -PathType Container) {
        Get-ChildItem -LiteralPath $PluginsPath -Recurse -File -Filter '*.uplugin' |
            ForEach-Object { $Paths.Add($_.FullName) }
    }

    $Rows = foreach ($Path in @($Paths | Sort-Object)) {
        $Data = Read-OWSJsonFile -Path $Path
        $Modules = foreach ($Module in @(Get-OWSObjectPropertyValue -Object $Data -Name 'Modules' -Default @())) {
            [ordered]@{
                name = [string](Get-OWSObjectPropertyValue -Object $Module -Name 'Name' -Default '')
                type = [string](Get-OWSObjectPropertyValue -Object $Module -Name 'Type' -Default '')
                loading_phase = [string](Get-OWSObjectPropertyValue -Object $Module -Name 'LoadingPhase' -Default '')
                platform_allow_list = @((Get-OWSObjectPropertyValue -Object $Module -Name 'PlatformAllowList' -Default @()) | ForEach-Object { [string]$_ } | Sort-Object)
            }
        }
        $PluginReferences = foreach ($Plugin in @(Get-OWSObjectPropertyValue -Object $Data -Name 'Plugins' -Default @())) {
            [ordered]@{
                name = [string](Get-OWSObjectPropertyValue -Object $Plugin -Name 'Name' -Default '')
                enabled = [bool](Get-OWSObjectPropertyValue -Object $Plugin -Name 'Enabled' -Default $false)
                optional = [bool](Get-OWSObjectPropertyValue -Object $Plugin -Name 'Optional' -Default $false)
            }
        }
        [ordered]@{
            path = ConvertTo-OWSNormalizedPath -Root $Root -Path $Path
            kind = if ($Path.EndsWith('.uproject', [System.StringComparison]::OrdinalIgnoreCase)) { 'project' } else { 'plugin' }
            name = [System.IO.Path]::GetFileNameWithoutExtension($Path)
            can_contain_content = [bool](Get-OWSObjectPropertyValue -Object $Data -Name 'CanContainContent' -Default $false)
            modules = @($Modules | Sort-Object name)
            plugin_references = @($PluginReferences | Sort-Object name)
        }
    }
    return @($Rows | Sort-Object path)
}

function Get-OWSModuleTypeMap {
    param([Parameter(Mandatory)][object[]]$Descriptors)

    $Map = @{}
    foreach ($Descriptor in $Descriptors) {
        foreach ($Module in @($Descriptor.modules)) {
            if (-not [string]::IsNullOrWhiteSpace([string]$Module.name)) {
                $Map[[string]$Module.name] = [string]$Module.type
            }
        }
    }
    return $Map
}

function Get-OWSLineNumber {
    param(
        [Parameter(Mandatory)][string]$Text,
        [Parameter(Mandatory)][int]$Index
    )
    if ($Index -le 0) { return 1 }
    return ([regex]::Matches($Text.Substring(0, $Index), "`n").Count + 1)
}

function Test-OWSEditorConditionalContext {
    param(
        [Parameter(Mandatory)][string]$Text,
        [Parameter(Mandatory)][int]$Index
    )

    $Start = [Math]::Max(0, $Index - 900)
    $Prefix = $Text.Substring($Start, $Index - $Start)
    return ($Prefix -match '(?s)if\s*\(\s*(?:Target\.bBuildEditor|Target\.Type\s*==\s*TargetType\.Editor)\s*\)[^{]*\{[^{}]*$')
}

function Get-OWSBuildDependencyInventory {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)][string]$RepositoryRoot,
        [Parameter(Mandatory)][object[]]$Descriptors
    )

    $Root = [System.IO.Path]::GetFullPath($RepositoryRoot)
    $ModuleTypes = Get-OWSModuleTypeMap -Descriptors $Descriptors
    $Files = @()
    foreach ($Subtree in @('Source', 'Plugins')) {
        $SubtreePath = Join-Path $Root $Subtree
        if (Test-Path -LiteralPath $SubtreePath -PathType Container) {
            $Files += @(Get-ChildItem -LiteralPath $SubtreePath -Recurse -File -Filter '*.Build.cs')
        }
    }

    $Rows = New-Object System.Collections.Generic.List[object]
    foreach ($File in @($Files | Sort-Object FullName)) {
        $Text = Get-Content -Raw -LiteralPath $File.FullName
        $ClassMatch = [regex]::Match($Text, '(?m)public\s+class\s+([A-Za-z0-9_]+)\s*:\s*ModuleRules')
        $ModuleName = if ($ClassMatch.Success) { $ClassMatch.Groups[1].Value } else { $File.BaseName.Replace('.Build', '') }
        $ModuleType = if ($ModuleTypes.ContainsKey($ModuleName)) { $ModuleTypes[$ModuleName] } else { 'Unknown' }

        $RangePattern = '(?ms)(PublicDependencyModuleNames|PrivateDependencyModuleNames|DynamicallyLoadedModuleNames)\s*\.\s*AddRange\s*\((.*?)\)\s*;'
        foreach ($Match in [regex]::Matches($Text, $RangePattern)) {
            $Visibility = $Match.Groups[1].Value.Replace('DependencyModuleNames', '').Replace('DynamicallyLoadedModuleNames', 'Dynamic')
            foreach ($NameMatch in [regex]::Matches($Match.Groups[2].Value, '"([A-Za-z0-9_]+)"')) {
                $Rows.Add([ordered]@{
                    file = ConvertTo-OWSNormalizedPath -Root $Root -Path $File.FullName
                    line = Get-OWSLineNumber -Text $Text -Index $Match.Index
                    module = $ModuleName
                    module_type = $ModuleType
                    visibility = $Visibility
                    dependency = $NameMatch.Groups[1].Value
                    editor_conditional = [bool](Test-OWSEditorConditionalContext -Text $Text -Index $Match.Index)
                })
            }
        }

        $SinglePattern = '(?m)(PublicDependencyModuleNames|PrivateDependencyModuleNames|DynamicallyLoadedModuleNames)\s*\.\s*Add\s*\(\s*"([A-Za-z0-9_]+)"\s*\)'
        foreach ($Match in [regex]::Matches($Text, $SinglePattern)) {
            $Visibility = $Match.Groups[1].Value.Replace('DependencyModuleNames', '').Replace('DynamicallyLoadedModuleNames', 'Dynamic')
            $Rows.Add([ordered]@{
                file = ConvertTo-OWSNormalizedPath -Root $Root -Path $File.FullName
                line = Get-OWSLineNumber -Text $Text -Index $Match.Index
                module = $ModuleName
                module_type = $ModuleType
                visibility = $Visibility
                dependency = $Match.Groups[2].Value
                editor_conditional = [bool](Test-OWSEditorConditionalContext -Text $Text -Index $Match.Index)
            })
        }
    }
    return @($Rows | Sort-Object `
        { [string]$_['module'] },
        { [string]$_['dependency'] },
        { [string]$_['visibility'] },
        { [string]$_['file'] },
        { [int]$_['line'] } -Unique)
}

function Get-OWSTextSourceFiles {
    param([Parameter(Mandatory)][string]$RepositoryRoot)

    $Files = @()
    foreach ($Subtree in @('Source', 'Plugins')) {
        $Path = Join-Path $RepositoryRoot $Subtree
        if (Test-Path -LiteralPath $Path -PathType Container) {
            $Files += @(Get-ChildItem -LiteralPath $Path -Recurse -File | Where-Object {
                $_.Extension.ToLowerInvariant() -in @('.h', '.hpp', '.cpp', '.cc', '.cxx', '.cs') -and
                $_.FullName -notmatch '[\\/](Binaries|Intermediate|Saved|DerivedDataCache)[\\/]'
            })
        }
    }
    return @($Files | Sort-Object FullName)
}

function Get-OWSTextReferenceInventory {
    [CmdletBinding()]
    param([Parameter(Mandatory)][string]$RepositoryRoot)

    $Root = [System.IO.Path]::GetFullPath($RepositoryRoot)
    $Pattern = '(?<![A-Za-z0-9_])/(?:Game|GASPALS|KinetiForge)(?:/[A-Za-z0-9_.-]+)+|/Script/[A-Za-z0-9_]+(?:\.[A-Za-z0-9_]+)?'
    $Rows = New-Object System.Collections.Generic.List[object]
    foreach ($File in Get-OWSTextSourceFiles -RepositoryRoot $Root) {
        $LineNumber = 0
        foreach ($Line in Get-Content -LiteralPath $File.FullName) {
            $LineNumber++
            foreach ($Match in [regex]::Matches([string]$Line, $Pattern)) {
                $Prefix = ([string]$Line).Substring(0, $Match.Index).TrimStart()
                $Rows.Add([ordered]@{
                    file = ConvertTo-OWSNormalizedPath -Root $Root -Path $File.FullName
                    line = $LineNumber
                    reference = $Match.Value
                    is_comment = [bool]($Prefix.StartsWith('//') -or $Prefix.StartsWith('*') -or $Prefix.StartsWith('/*'))
                })
            }
        }
    }
    return @($Rows | Sort-Object `
        { [string]$_['file'] },
        { [int]$_['line'] },
        { [string]$_['reference'] } -Unique)
}

function Get-OWSPublicSurfaceInventory {
    [CmdletBinding()]
    param([Parameter(Mandatory)][string]$RepositoryRoot)

    $Root = [System.IO.Path]::GetFullPath($RepositoryRoot)
    $ProviderRows = New-Object System.Collections.Generic.List[object]
    $PrivateIncludeRows = New-Object System.Collections.Generic.List[object]
    $StructUtilsRows = New-Object System.Collections.Generic.List[object]
    $ProviderPattern = '\b(?:UModularVehicleBaseComponent|UVehicleDriveAssemblyComponent|KinetiForge|ChaosModularVehicle[A-Za-z0-9_]*)\b'

    foreach ($File in Get-OWSTextSourceFiles -RepositoryRoot $Root) {
        $Relative = ConvertTo-OWSNormalizedPath -Root $Root -Path $File.FullName
        $IsPublicHeader = $File.Extension -in @('.h', '.hpp') -and $Relative -match '/Public/'
        $LineNumber = 0
        foreach ($Line in Get-Content -LiteralPath $File.FullName) {
            $LineNumber++
            if ($IsPublicHeader) {
                foreach ($Match in [regex]::Matches([string]$Line, $ProviderPattern)) {
                    $ProviderRows.Add([ordered]@{ file = $Relative; line = $LineNumber; symbol = $Match.Value })
                }
                if ([string]$Line -match '#include\s*[<"]StructUtils/') {
                    $StructUtilsRows.Add([ordered]@{ file = $Relative; line = $LineNumber; module = if ($Relative -match '^Plugins/SigilInventory/') { 'SigilInventory' } else { 'Unknown' } })
                }
            }
            if ([string]$Line -match '#include\s*[<"]Editor/ClassViewer/Private/') {
                $PrivateIncludeRows.Add([ordered]@{ file = $Relative; line = $LineNumber; provider = 'ClassViewerPrivate' })
            }
        }
    }
    return [ordered]@{
        provider_types = @($ProviderRows | Sort-Object { [string]$_['file'] }, { [int]$_['line'] }, { [string]$_['symbol'] } -Unique)
        private_engine_includes = @($PrivateIncludeRows | Sort-Object { [string]$_['file'] }, { [int]$_['line'] } -Unique)
        structutils_includes = @($StructUtilsRows | Sort-Object { [string]$_['file'] }, { [int]$_['line'] } -Unique)
    }
}

function Get-OWSConfigInventory {
    [CmdletBinding()]
    param([Parameter(Mandatory)][string]$RepositoryRoot)

    $Root = [System.IO.Path]::GetFullPath($RepositoryRoot)
    $Files = @()
    $RootConfig = Join-Path $Root 'Config'
    if (Test-Path -LiteralPath $RootConfig -PathType Container) {
        $Files += @(Get-ChildItem -LiteralPath $RootConfig -Recurse -File -Filter '*.ini')
    }
    $PluginsRoot = Join-Path $Root 'Plugins'
    if (Test-Path -LiteralPath $PluginsRoot -PathType Container) {
        $Files += @(Get-ChildItem -LiteralPath $PluginsRoot -Recurse -File -Filter '*.ini' | Where-Object {
            $_.FullName -match '[\\/]Config[\\/]'
        })
    }

    $Rows = New-Object System.Collections.Generic.List[object]
    foreach ($File in @($Files | Sort-Object FullName)) {
        $Section = ''
        $LineNumber = 0
        foreach ($Line in Get-Content -LiteralPath $File.FullName) {
            $LineNumber++
            $Text = [string]$Line
            if ($Text -match '^\s*\[([^]]+)\]\s*$') {
                $Section = $Matches[1].Trim()
                continue
            }
            if ($Text -notmatch '^\s*([^;#][^=]*?)\s*=\s*(.*)$') {
                continue
            }
            $RawKey = $Matches[1].Trim()
            $Value = [string]$Matches[2]
            $Operation = ''
            if ($RawKey.Length -gt 0 -and '+-!.'.Contains($RawKey.Substring(0, 1))) {
                $Operation = $RawKey.Substring(0, 1)
                $RawKey = $RawKey.Substring(1).Trim()
            }
            if ([string]::IsNullOrWhiteSpace($RawKey)) {
                continue
            }
            $Sensitive = [bool]($RawKey -match '(?i)(token|secret|password|credential|authorization|api.?key|private.?key)')
            $PathReference = $null
            if (-not $Sensitive) {
                $PathMatch = [regex]::Match($Value, '(?i)Path\s*=\s*"([^"]+)"')
                if ($PathMatch.Success -and $PathMatch.Groups[1].Value -match '^/(?:Game|Engine|[A-Za-z0-9_]+)/[A-Za-z0-9_./-]+$') {
                    $PathReference = $PathMatch.Groups[1].Value
                }
            }
            $Rows.Add([ordered]@{
                file = ConvertTo-OWSNormalizedPath -Root $Root -Path $File.FullName
                line = $LineNumber
                section = $Section
                key = $RawKey
                operation = $Operation
                sensitive = $Sensitive
                value_present = -not [string]::IsNullOrEmpty($Value)
                value_digest = $null
                comparison_digest = if ($Sensitive) { $null } else { Get-OWSSha256Text -Text $Value }
                conflicting_values = $false
                path_reference = $PathReference
            })
        }
    }
    foreach ($Group in @($Rows | Group-Object { "$($_.section)|$($_.key)" })) {
        $Comparable = @($Group.Group | Where-Object { -not $_.sensitive } | ForEach-Object { $_.comparison_digest } | Sort-Object -Unique)
        $Conflicting = $Comparable.Count -gt 1
        foreach ($Row in $Group.Group) {
            $Row.conflicting_values = $Conflicting
            [void]$Row.Remove('comparison_digest')
        }
    }
    return @($Rows | Sort-Object file, line, section, key)
}

function Get-OWSCookRuleInventory {
    [CmdletBinding()]
    param([Parameter(Mandatory)][object[]]$ConfigInventory)

    return @($ConfigInventory | Where-Object {
        $_.key -match '^(DirectoriesToAlwaysCook|DirectoriesToNeverCook|PrimaryAssetTypesToScan)$'
    } | ForEach-Object {
        [ordered]@{
            file = $_.file
            line = $_.line
            section = $_.section
            key = $_.key
            operation = $_.operation
            path_reference = $_.path_reference
        }
    } | Sort-Object file, line, key)
}

function Get-OWSTestInventory {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)][string]$RepositoryRoot,
        [Parameter(Mandatory)][object[]]$Descriptors
    )

    $Root = [System.IO.Path]::GetFullPath($RepositoryRoot)
    $ModuleTypes = Get-OWSModuleTypeMap -Descriptors $Descriptors
    $Rows = New-Object System.Collections.Generic.List[object]
    foreach ($File in Get-OWSTextSourceFiles -RepositoryRoot $Root) {
        $Relative = ConvertTo-OWSNormalizedPath -Root $Root -Path $File.FullName
        $Text = Get-Content -Raw -LiteralPath $File.FullName
        if ($Relative -notmatch '/Tests?/' -and $Text -notmatch 'IMPLEMENT_(?:SIMPLE_)?AUTOMATION_TEST') {
            continue
        }
        $Module = 'Unknown'
        if ($Relative -match '^Source/([^/]+)/') {
            $Module = $Matches[1]
        }
        elseif ($Relative -match '^Plugins/[^/]+/Source/([^/]+)/') {
            $Module = $Matches[1]
        }
        $Rows.Add([ordered]@{
            file = $Relative
            module = $Module
            module_type = if ($ModuleTypes.ContainsKey($Module)) { $ModuleTypes[$Module] } else { 'Unknown' }
            with_editor_guard = [bool]($Text -match '\bWITH_EDITOR\b')
            with_automation_guard = [bool]($Text -match '\bWITH_(?:DEV_)?AUTOMATION_TESTS\b')
            named_test_count = [regex]::Matches($Text, 'IMPLEMENT_(?:SIMPLE_)?AUTOMATION_TEST').Count
        })
    }
    return @($Rows | Sort-Object file)
}

function Get-OWSBinaryReferenceLeads {
    [CmdletBinding()]
    param([Parameter(Mandatory)][string]$RepositoryRoot)

    $Rows = foreach ($Prefix in @('/Game/', '/GASPALS/', '/KinetiForge/')) {
        $Arguments = @('grep', '-a', '-l', '-e', $Prefix, '--', '*.uasset', '*.umap')
        $Paths = @(Invoke-OWSGitLines -RepositoryRoot $RepositoryRoot -Arguments $Arguments -AllowNoMatch)
        [ordered]@{
            prefix = $Prefix
            count = $Paths.Count
            sample_paths = @($Paths | Sort-Object | Select-Object -First 25)
            status = 'observation_pending_asset_registry'
        }
    }
    return @($Rows)
}

function Read-OWSAssetBaseline {
    [CmdletBinding()]
    param([string]$Path)

    if ([string]::IsNullOrWhiteSpace($Path) -or -not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        return [ordered]@{
            status = 'NR'
            detail = 'Static-only run; Unreal Asset Registry evidence was not generated.'
            schema_version = $null
            classification_capabilities = [ordered]@{}
            asset_count = 0
            assets = @()
        }
    }
    $Data = Get-Content -Raw -LiteralPath $Path | ConvertFrom-Json
    return [ordered]@{
        status = 'PASS'
        detail = 'Read-only Unreal Asset Registry export was loaded.'
        schema_version = [string](Get-OWSObjectPropertyValue -Object $Data -Name 'schema_version' -Default '')
        classification_capabilities = Get-OWSObjectPropertyValue -Object $Data -Name 'classification_capabilities' -Default ([ordered]@{})
        asset_count = [int](Get-OWSObjectPropertyValue -Object $Data -Name 'asset_count' -Default 0)
        assets = @((Get-OWSObjectPropertyValue -Object $Data -Name 'assets' -Default @()) | Sort-Object package_name, asset_name)
    }
}

function Get-OWSFileManifest {
    [CmdletBinding()]
    param([string]$RootPath)

    if ([string]::IsNullOrWhiteSpace($RootPath) -or -not (Test-Path -LiteralPath $RootPath -PathType Container)) {
        return [ordered]@{ status = 'NR'; detail = 'No staged/package directory was supplied.'; count = 0; bytes = 0; entries = @() }
    }
    $Root = [System.IO.Path]::GetFullPath($RootPath)
    $Entries = foreach ($File in Get-ChildItem -LiteralPath $Root -Recurse -File | Sort-Object FullName) {
        [ordered]@{
            path = ConvertTo-OWSNormalizedPath -Root $Root -Path $File.FullName
            bytes = [long]$File.Length
            sha256 = Get-OWSFileSha256 -Path $File.FullName
        }
    }
    $TotalBytes = [long]0
    foreach ($Entry in @($Entries)) { $TotalBytes += [long]$Entry['bytes'] }
    return [ordered]@{
        status = 'PASS'
        detail = 'Staged/package files were inventoried without reading their contents into the report.'
        count = @($Entries).Count
        bytes = $TotalBytes
        entries = @($Entries)
    }
}

function Read-OWSCsvEvidence {
    param([string]$Path, [string]$Detail)

    if ([string]::IsNullOrWhiteSpace($Path) -or -not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        return [ordered]@{ status = 'NR'; detail = 'No CSV evidence file was supplied.'; path = $null; bytes = 0; sha256 = $null; columns = @(); row_count = 0; rows = @() }
    }
    $File = Get-Item -LiteralPath $Path
    $Imported = @(Import-Csv -LiteralPath $File.FullName)
    $Rows = foreach ($Row in $Imported) {
        $Safe = [ordered]@{}
        foreach ($Name in @($Row.PSObject.Properties.Name | Sort-Object)) {
            $Safe[$Name] = ConvertTo-OWSSanitizedText -Text ([string]$Row.$Name)
        }
        $Safe
    }
    $Columns = @(
        if ($Imported.Count -gt 0) {
            $Imported[0].PSObject.Properties.Name | Sort-Object
        }
    )
    $SortedRows = @($Rows | Sort-Object { $_ | ConvertTo-Json -Compress })
    return [ordered]@{
        status = 'PASS'
        detail = $Detail
        path = $File.Name
        bytes = [long]$File.Length
        sha256 = Get-OWSFileSha256 -Path $File.FullName
        columns = $Columns
        row_count = $SortedRows.Count
        rows = $SortedRows
    }
}

function Read-OWSContainerManifestEvidence {
    param([string]$RootPath)

    if ([string]::IsNullOrWhiteSpace($RootPath) -or -not (Test-Path -LiteralPath $RootPath -PathType Container)) {
        return [ordered]@{ status = 'NR'; detail = 'No UnrealPak manifest directory was supplied.'; manifest_count = 0; row_count = 0; manifests = @() }
    }
    $Root = [System.IO.Path]::GetFullPath($RootPath)
    $Manifests = foreach ($File in Get-ChildItem -LiteralPath $Root -File -Filter '*.csv' | Sort-Object Name) {
        $Evidence = Read-OWSCsvEvidence -Path $File.FullName -Detail 'UnrealPak CSV rows embedded in the baseline report.'
        [ordered]@{
            path = ConvertTo-OWSNormalizedPath -Root $Root -Path $File.FullName
            kind = if ($File.Name.StartsWith('utoc-', [System.StringComparison]::OrdinalIgnoreCase)) { 'iostore' } else { 'pak' }
            bytes = $Evidence.bytes
            sha256 = $Evidence.sha256
            columns = $Evidence.columns
            row_count = $Evidence.row_count
            rows = $Evidence.rows
        }
    }
    $TotalRows = 0
    foreach ($Manifest in @($Manifests)) { $TotalRows += [int]$Manifest['row_count'] }
    return [ordered]@{
        status = 'PASS'
        detail = 'Normalized UnrealPak IoStore/pak rows are embedded; source CSV files are task-owned scratch.'
        manifest_count = @($Manifests).Count
        row_count = $TotalRows
        manifests = @($Manifests)
    }
}

function Read-OWSPluginSizeEvidence {
    param([string]$CookMetadataRoot)

    if ([string]::IsNullOrWhiteSpace($CookMetadataRoot) -or -not (Test-Path -LiteralPath $CookMetadataRoot -PathType Container)) {
        return [ordered]@{ status = 'NR'; detail = 'No cook metadata directory was supplied.'; csv = $null; json = $null }
    }
    $CsvPath = Join-Path $CookMetadataRoot 'plugin_sizes.csv'
    $JsonPath = Join-Path $CookMetadataRoot 'plugin_size_jsons.json'
    if (-not (Test-Path -LiteralPath $CsvPath -PathType Leaf) -or -not (Test-Path -LiteralPath $JsonPath -PathType Leaf)) {
        return [ordered]@{ status = 'NR'; detail = 'Plugin-size evidence files were not both present.'; csv = $null; json = $null }
    }
    $Csv = Read-OWSCsvEvidence -Path $CsvPath -Detail 'Plugin physical-size CSV rows embedded in the baseline report.'
    $SafeJsonText = ConvertTo-OWSSanitizedText -Text (Get-Content -Raw -LiteralPath $JsonPath)
    $JsonData = $SafeJsonText | ConvertFrom-Json
    return [ordered]@{
        status = 'PASS'
        detail = 'Plugin-size CSV and JSON data are embedded; source metadata remains task-owned scratch.'
        csv = $Csv
        json = [ordered]@{
            path = [System.IO.Path]::GetFileName($JsonPath)
            bytes = [long](Get-Item -LiteralPath $JsonPath).Length
            sha256 = Get-OWSFileSha256 -Path $JsonPath
            data = $JsonData
        }
    }
}

function New-OWSRawFinding {
    param(
        [Parameter(Mandatory)][string]$RuleId,
        [Parameter(Mandatory)][string]$FamilyId,
        [Parameter(Mandatory)][string]$Severity,
        [Parameter(Mandatory)][string]$Summary,
        [Parameter(Mandatory)][string[]]$Locations,
        [Parameter(Mandatory)][string]$Evidence,
        [bool]$Redacted = $false
    )
    return [ordered]@{
        rule_id = $RuleId
        family_id = $FamilyId
        severity = $Severity
        summary = $Summary
        locations = @($Locations | Where-Object { -not [string]::IsNullOrWhiteSpace($_) } | Sort-Object -Unique)
        evidence = $Evidence
        redacted = $Redacted
    }
}

function Get-OWSRawFindings {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)][object[]]$Descriptors,
        [Parameter(Mandatory)][object[]]$ModuleDependencies,
        [Parameter(Mandatory)][object[]]$TextReferences,
        [Parameter(Mandatory)]$PublicSurfaces,
        [Parameter(Mandatory)][object[]]$ConfigKeys,
        [Parameter(Mandatory)][object[]]$Tests,
        [Parameter(Mandatory)][object[]]$CookRules,
        [Parameter(Mandatory)][object[]]$BinaryReferenceLeads
    )

    $Raw = New-Object System.Collections.Generic.List[object]
    $EditorModules = @('UnrealEd', 'AssetTools', 'BlueprintGraph', 'ClassViewer', 'Kismet', 'LevelEditor', 'PropertyEditor', 'ToolMenus', 'UMGEditor')
    $RuntimeEditorDependencies = @($ModuleDependencies | Where-Object {
        $_.module_type -eq 'Runtime' -and $_.dependency -in $EditorModules
    })
    $RuntimeTests = @($Tests | Where-Object { $_.module_type -eq 'Runtime' })
    if ($RuntimeEditorDependencies.Count -gt 0 -or $RuntimeTests.Count -gt 0) {
        $Locations = @($RuntimeEditorDependencies | ForEach-Object { "$($_.file):$($_.line)" }) + @($RuntimeTests | ForEach-Object { $_.file })
        $Raw.Add((New-OWSRawFinding -RuleId 'BASE-BOUND-001' -FamilyId 'runtime-test-editor-mixing' -Severity 'warning' -Summary 'Runtime module boundaries contain editor-only dependencies or test sources.' -Locations $Locations -Evidence "runtime_editor_dependencies=$($RuntimeEditorDependencies.Count);runtime_test_files=$($RuntimeTests.Count)"))
    }

    $OWSCoreReverse = @($TextReferences | Where-Object {
        -not $_.is_comment -and $_.file -match '^Plugins/OWSFramework/Source/OWSCore/' -and
        ($_.reference.StartsWith('/Game/') -or $_.reference.StartsWith('/GASPALS/'))
    })
    if ($OWSCoreReverse.Count -gt 0) {
        $Raw.Add((New-OWSRawFinding -RuleId 'BASE-REF-TEXT-001' -FamilyId 'owscore-reverse-content-references' -Severity 'warning' -Summary 'OWSCore contains literal project or maintained-Character content references.' -Locations @($OWSCoreReverse | ForEach-Object { "$($_.file):$($_.line)" }) -Evidence "active_reverse_references=$($OWSCoreReverse.Count)"))
    }

    $OWSProviderDependencies = @($ModuleDependencies | Where-Object {
        $_.module -eq 'OWS' -and $_.dependency -in @('KinetiForge', 'Chaos', 'ChaosModularVehicleEngine', 'ChaosVehiclesCore')
    })
    $OWSProviderText = @($TextReferences | Where-Object {
        -not $_.is_comment -and $_.file -match '^Source/OWS/' -and
        ($_.reference.StartsWith('/KinetiForge/') -or $_.reference.StartsWith('/GASPALS/') -or $_.reference.StartsWith('/Script/KinetiForge'))
    })
    $OWSPublicTypes = @($PublicSurfaces.provider_types | Where-Object { $_.file -match '^Source/OWS/Public/' })
    if ($OWSProviderDependencies.Count -gt 0 -or $OWSProviderText.Count -gt 0 -or $OWSPublicTypes.Count -gt 0) {
        $Locations = @($OWSProviderDependencies | ForEach-Object { "$($_.file):$($_.line)" }) +
            @($OWSProviderText | ForEach-Object { "$($_.file):$($_.line)" }) +
            @($OWSPublicTypes | ForEach-Object { "$($_.file):$($_.line)" })
        $Raw.Add((New-OWSRawFinding -RuleId 'BASE-API-001' -FamilyId 'ows-public-provider-leakage' -Severity 'warning' -Summary 'The project shell directly consumes or exposes maintained Vehicle/provider implementation surfaces.' -Locations $Locations -Evidence "module_edges=$($OWSProviderDependencies.Count);literal_refs=$($OWSProviderText.Count);public_symbols=$($OWSPublicTypes.Count)"))
    }

    $PrivateClassViewer = @($PublicSurfaces.private_engine_includes)
    if ($PrivateClassViewer.Count -gt 0) {
        $Raw.Add((New-OWSRawFinding -RuleId 'BASE-API-001' -FamilyId 'saveextension-private-classviewer-include' -Severity 'warning' -Summary 'SaveExtension Editor includes a private ClassViewer header.' -Locations @($PrivateClassViewer | ForEach-Object { "$($_.file):$($_.line)" }) -Evidence "private_engine_includes=$($PrivateClassViewer.Count)"))
    }

    $DuplicateGroups = @($ConfigKeys | Group-Object { "$($_.section)|$($_.key)" } | Where-Object {
        @($_.Group.file | Sort-Object -Unique).Count -gt 1
    })
    $AlwaysCook = @($CookRules | Where-Object { $_.key -eq 'DirectoriesToAlwaysCook' })
    if ($DuplicateGroups.Count -gt 0 -or $AlwaysCook.Count -gt 0) {
        $Locations = @($DuplicateGroups | ForEach-Object { $_.Group | ForEach-Object { "$($_.file):$($_.line)" } }) +
            @($AlwaysCook | ForEach-Object { "$($_.file):$($_.line)" })
        $ConflictGroups = @($DuplicateGroups | Where-Object { @($_.Group | Where-Object { $_.conflicting_values }).Count -gt 0 })
        $Raw.Add((New-OWSRawFinding -RuleId 'BASE-CONFIG-001' -FamilyId 'configuration-and-cook-ownership' -Severity 'warning' -Summary 'Configuration ownership is duplicated or conflicting, or explicit always-cook entries remain.' -Locations $Locations -Evidence "multi_origin_keys=$($DuplicateGroups.Count);conflicting_key_groups=$($ConflictGroups.Count);always_cook_entries=$($AlwaysCook.Count)" -Redacted $true))
    }

    $SensitiveKeys = @($ConfigKeys | Where-Object { $_.sensitive -and $_.value_present })
    if ($SensitiveKeys.Count -gt 0) {
        $Locations = @($SensitiveKeys | ForEach-Object { "$($_.file):$($_.line)" })
        $KeyNames = @($SensitiveKeys.key | Sort-Object -Unique)
        $Raw.Add((New-OWSRawFinding -RuleId 'BASE-REDACT-001' -FamilyId 'android-file-server-sensitive-key' -Severity 'error' -Summary 'Sensitive configuration keys have values; values were omitted from every inventory and finding.' -Locations $Locations -Evidence "sensitive_key_names=$($KeyNames -join ',');value_present=true;values_serialized=false" -Redacted $true))
    }

    $BinaryCount = [int](($BinaryReferenceLeads | Measure-Object -Property count -Sum).Sum)
    if ($BinaryCount -gt 0) {
        $Locations = @($BinaryReferenceLeads | ForEach-Object { @($_.sample_paths) } | Select-Object -Unique)
        $Raw.Add((New-OWSRawFinding -RuleId 'BASE-REF-ASSET-001' -FamilyId 'binary-game-reference-leads' -Severity 'info' -Summary 'Binary packages contain mount-string leads pending authoritative Asset Registry classification.' -Locations $Locations -Evidence "binary_string_leads=$BinaryCount;classification=pending_asset_registry"))
    }

    $SigilStructUtils = @($PublicSurfaces.structutils_includes | Where-Object { $_.module -eq 'SigilInventory' })
    $SigilDeclaresStructUtils = @($ModuleDependencies | Where-Object { $_.module -eq 'SigilInventory' -and $_.dependency -eq 'StructUtils' }).Count -gt 0
    if ($SigilStructUtils.Count -gt 0 -and -not $SigilDeclaresStructUtils) {
        $Raw.Add((New-OWSRawFinding -RuleId 'BASE-MOD-001' -FamilyId 'sigil-structutils-transitive-dependency' -Severity 'warning' -Summary 'Sigil public headers use StructUtils without a direct Build.cs dependency.' -Locations @($SigilStructUtils | ForEach-Object { "$($_.file):$($_.line)" }) -Evidence "public_structutils_includes=$($SigilStructUtils.Count);direct_dependency=false"))
    }

    $OtherPluginReverse = @($TextReferences | Where-Object {
        -not $_.is_comment -and $_.file -match '^Plugins/' -and $_.file -notmatch '^Plugins/OWSFramework/' -and $_.reference.StartsWith('/Game/')
    })
    if ($OtherPluginReverse.Count -gt 0) {
        $Raw.Add((New-OWSRawFinding -RuleId 'BASE-REF-TEXT-001' -FamilyId 'unowned-plugin-reverse-reference' -Severity 'error' -Summary 'A plugin contains an active project-content reference without a mapped known-gap owner.' -Locations @($OtherPluginReverse | ForEach-Object { "$($_.file):$($_.line)" }) -Evidence "unowned_reverse_references=$($OtherPluginReverse.Count)"))
    }

    return @($Raw | ForEach-Object { $_ })
}

function Resolve-OWSFindings {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)][AllowEmptyCollection()][object[]]$RawFindings,
        [Parameter(Mandatory)]$KnownGaps
    )

    $FamilyMap = @{}
    foreach ($Family in @(Get-OWSObjectPropertyValue -Object $KnownGaps -Name 'families' -Default @())) {
        $FamilyMap[[string]$Family.id] = $Family
    }

    $SeenFamilies = @{}
    $Resolved = New-Object System.Collections.Generic.List[object]
    foreach ($Raw in $RawFindings) {
        $Family = if ($FamilyMap.ContainsKey([string]$Raw.family_id)) { $FamilyMap[[string]$Raw.family_id] } else { $null }
        $Classification = if ($null -eq $Family) { 'new_unowned_gap' } else { [string]$Family.classification }
        $Owners = @()
        if ($null -ne $Family) {
            $SeenFamilies[[string]$Raw.family_id] = $true
            $Owners = @($Family.owners | ForEach-Object {
                [ordered]@{ issue = [int]$_.issue; url = [string]$_.url }
            })
        }
        $Locations = @($Raw.locations | Sort-Object -Unique)
        $Material = "$($Raw.rule_id)`n$($Raw.family_id)`n$($Locations -join "`n")`n$($Raw.evidence)"
        $Resolved.Add([ordered]@{
            fingerprint = Get-OWSSha256Text -Text $Material
            rule_id = [string]$Raw.rule_id
            family_id = [string]$Raw.family_id
            classification = $Classification
            severity = [string]$Raw.severity
            summary = [string]$Raw.summary
            locations = $Locations
            evidence = [string]$Raw.evidence
            owners = $Owners
            redacted = [bool]$Raw.redacted
        })
    }

    foreach ($Family in @($FamilyMap.Values | Sort-Object id)) {
        if ([string]$Family.classification -ne 'known_gap' -or $SeenFamilies.ContainsKey([string]$Family.id)) {
            continue
        }
        $Owners = @($Family.owners | ForEach-Object { [ordered]@{ issue = [int]$_.issue; url = [string]$_.url } })
        $Evidence = 'No current finding matched this reviewed known-gap family; an owning issue must verify resolution before its baseline entry is retired.'
        $Resolved.Add([ordered]@{
            fingerprint = Get-OWSSha256Text -Text "BASE-OWN-001`n$($Family.id)`n$Evidence"
            rule_id = 'BASE-OWN-001'
            family_id = [string]$Family.id
            classification = 'candidate_resolved'
            severity = 'info'
            summary = "Known-gap family is absent and requires owner verification: $($Family.id)."
            locations = @()
            evidence = $Evidence
            owners = $Owners
            redacted = $false
        })
    }
    return @($Resolved | Sort-Object classification, rule_id, family_id, fingerprint)
}

function ConvertTo-OWSSanitizedText {
    [CmdletBinding()]
    param([AllowEmptyString()][string]$Text)

    if ($null -eq $Text) { return '' }
    $Sanitized = [regex]::Replace(
        $Text,
        '(?is)-----BEGIN [^-\r\n]*PRIVATE KEY-----.*?-----END [^-\r\n]*PRIVATE KEY-----',
        '[REDACTED PRIVATE KEY BLOCK]'
    )
    $Sanitized = [regex]::Replace(
        $Sanitized,
        '(?i)(?<prefix>[?&](?:x-amz-signature|x-goog-signature|signature|sig|access_token|auth_token)=)[^&\s"'']+',
        '${prefix}[REDACTED]'
    )
    $Sanitized = [regex]::Replace(
        $Sanitized,
        '(?im)(?<prefix>\bauthorization\s*:\s*)(?:bearer|basic)\s+[^\r\n]+',
        '${prefix}[REDACTED]'
    )
    $Sanitized = [regex]::Replace(
        $Sanitized,
        '(?im)(?<prefix>["'']?(?:token|secret|password|credential|authorization|api[-_]?key|private[-_]?key)["'']?\s*[:=]\s*)["''][^\r\n]*?["'']',
        '${prefix}"[REDACTED]"'
    )
    $Sanitized = [regex]::Replace(
        $Sanitized,
        '(?im)(?<prefix>(?:token|secret|password|credential|authorization|api[-_]?key|private[-_]?key)\s*[:=]\s*)(?<value>[^\r\n,;]+)',
        '${prefix}[REDACTED]'
    )
    return $Sanitized
}

function Get-OWSEngineAssociation {
    param([Parameter(Mandatory)][string]$RepositoryRoot)
    $Project = Join-Path $RepositoryRoot 'OWS.uproject'
    if (-not (Test-Path -LiteralPath $Project -PathType Leaf)) { return $null }
    $Data = Read-OWSJsonFile -Path $Project
    return [string](Get-OWSObjectPropertyValue -Object $Data -Name 'EngineAssociation' -Default '')
}

function New-OWSResult {
    param([string]$Status, [string]$Detail)
    return [ordered]@{ status = $Status; detail = $Detail }
}

function New-OWSStaticBaselineReport {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)][string]$RepositoryRoot,
        [Parameter(Mandatory)][string]$KnownGapsPath,
        [string]$AssetBaselinePath,
        [string]$StageRoot,
        [string]$ArchiveRoot,
        [string]$CookMetadataRoot,
        [string]$AssetRegistryDumpRoot,
        [string]$AssetSizeCsvPath,
        [string]$ContainerManifestRoot,
        [object[]]$Commands = @(),
        [System.Collections.IDictionary]$Results,
        [System.Collections.IDictionary]$Immutability,
        [DateTime]$GeneratedUtc = ([DateTime]::UtcNow),
        [DateTime]$StartedUtc = $GeneratedUtc,
        [string]$EngineRootLabel,
        [string]$EngineVersion,
        $EngineChangelist,
        $CompatibleChangelist,
        [string]$UBTVersion,
        [switch]$StaticOnly
    )

    $Root = [System.IO.Path]::GetFullPath($RepositoryRoot)
    $Source = Get-OWSRepositoryState -RepositoryRoot $Root
    $Descriptors = @(Get-OWSDescriptorInventory -RepositoryRoot $Root)
    $Dependencies = @(Get-OWSBuildDependencyInventory -RepositoryRoot $Root -Descriptors $Descriptors)
    $TextReferences = @(Get-OWSTextReferenceInventory -RepositoryRoot $Root)
    $PublicSurfaces = Get-OWSPublicSurfaceInventory -RepositoryRoot $Root
    $ConfigKeys = @(Get-OWSConfigInventory -RepositoryRoot $Root)
    $Tests = @(Get-OWSTestInventory -RepositoryRoot $Root -Descriptors $Descriptors)
    $CookRules = @(Get-OWSCookRuleInventory -ConfigInventory $ConfigKeys)
    $BinaryLeads = @(Get-OWSBinaryReferenceLeads -RepositoryRoot $Root)
    $AssetRegistry = Read-OWSAssetBaseline -Path $AssetBaselinePath
    $Stage = Get-OWSFileManifest -RootPath $StageRoot
    $Archive = Get-OWSFileManifest -RootPath $ArchiveRoot
    $CookMetadata = Get-OWSFileManifest -RootPath $CookMetadataRoot
    $AssetRegistryDump = Get-OWSFileManifest -RootPath $AssetRegistryDumpRoot
    $AssetSizes = Read-OWSCsvEvidence -Path $AssetSizeCsvPath -Detail 'Normalized AssetSizeQuery rows are embedded in the baseline report.'
    $ContainerManifests = Read-OWSContainerManifestEvidence -RootPath $ContainerManifestRoot
    $PluginSizes = Read-OWSPluginSizeEvidence -CookMetadataRoot $CookMetadataRoot
    $KnownGaps = Get-Content -Raw -LiteralPath $KnownGapsPath | ConvertFrom-Json

    $RawFindings = @(Get-OWSRawFindings -Descriptors $Descriptors -ModuleDependencies $Dependencies -TextReferences $TextReferences -PublicSurfaces $PublicSurfaces -ConfigKeys $ConfigKeys -Tests $Tests -CookRules $CookRules -BinaryReferenceLeads $BinaryLeads)
    $Findings = @(Resolve-OWSFindings -RawFindings $RawFindings -KnownGaps $KnownGaps)
    if ($null -eq $Results) {
        $Results = [ordered]@{
            self_test = New-OWSResult -Status 'NR' -Detail 'The standalone baseline self-test was not run by this capture.'
            static = New-OWSResult -Status 'PASS' -Detail 'Static baseline collectors completed.'
            lfs = New-OWSResult -Status 'NR' -Detail 'Git LFS validation was not requested.'
            build = New-OWSResult -Status 'NR' -Detail 'Build was not requested.'
            asset_registry = New-OWSResult -Status 'NR' -Detail 'Unreal Asset Registry export was not requested.'
            clean_start = New-OWSResult -Status 'NR' -Detail 'Unreal clean-start smoke was not requested.'
            selector = New-OWSResult -Status 'NR' -Detail 'Selector automation was not requested.'
            character_vehicle = New-OWSResult -Status 'NR' -Detail 'Character/Vehicle automation was not requested.'
            cook = New-OWSResult -Status 'NR' -Detail 'Cook was not requested.'
            package = New-OWSResult -Status 'NR' -Detail 'Package was not requested.'
            cooked_asset_registry = New-OWSResult -Status 'NR' -Detail 'Cooked Asset Registry evidence was not requested.'
            asset_sizes = New-OWSResult -Status 'NR' -Detail 'AssetSizeQuery evidence was not requested.'
            container_manifests = New-OWSResult -Status 'NR' -Detail 'UnrealPak manifest evidence was not requested.'
            packaged_launch = New-OWSResult -Status 'NR' -Detail 'No deterministic packaged-launch oracle has been defined; issue #17 owns that future evidence.'
        }
    }
    if ($null -eq $Immutability) {
        $Immutability = [ordered]@{
            pre_digest = $Source.status_digest
            post_digest = $Source.status_digest
            tracked_mutations = 0
            worktree_removed = $true
            cleanup_passed = $true
            cleanup_issue = $null
        }
    }

    $KnownCount = @($Findings | Where-Object { $_.classification -eq 'known_gap' }).Count
    $NewCount = @($Findings | Where-Object { $_.classification -eq 'new_unowned_gap' }).Count
    $ObservationCount = @($Findings | Where-Object { $_.classification -eq 'observation' }).Count
    $ResolvedCount = @($Findings | Where-Object { $_.classification -eq 'candidate_resolved' }).Count
    $Inventories = [ordered]@{
        descriptors = $Descriptors
        module_dependencies = $Dependencies
        text_references = $TextReferences
        public_surfaces = $PublicSurfaces
        config_keys = $ConfigKeys
        tests = $Tests
        cook_rules = $CookRules
        binary_reference_leads = $BinaryLeads
        asset_registry = $AssetRegistry
        stage = $Stage
        archive = $Archive
        cook_metadata = $CookMetadata
        cooked_asset_registry_dump = $AssetRegistryDump
        asset_sizes = $AssetSizes
        container_manifests = $ContainerManifests
        plugin_sizes = $PluginSizes
    }
    $Artifacts = New-Object System.Collections.Generic.List[object]
    foreach ($InventoryName in @('asset_registry', 'stage', 'archive', 'cook_metadata', 'cooked_asset_registry_dump', 'asset_sizes', 'container_manifests', 'plugin_sizes')) {
        $Inventory = $Inventories[$InventoryName]
        if ([string](Get-OWSObjectPropertyValue -Object $Inventory -Name 'status' -Default '') -ne 'PASS') { continue }
        $ArtifactJson = $Inventory | ConvertTo-Json -Depth 100 -Compress
        $Count = Get-OWSObjectPropertyValue -Object $Inventory -Name 'asset_count' -Default $null
        if ($null -eq $Count) { $Count = Get-OWSObjectPropertyValue -Object $Inventory -Name 'row_count' -Default $null }
        if ($null -eq $Count) { $Count = Get-OWSObjectPropertyValue -Object $Inventory -Name 'count' -Default $null }
        if ($null -eq $Count) { $Count = Get-OWSObjectPropertyValue -Object $Inventory -Name 'manifest_count' -Default $null }
        $Artifacts.Add([ordered]@{
            path = "#/inventories/$InventoryName"
            type = 'embedded-json-evidence'
            sha256 = Get-OWSSha256Text -Text $ArtifactJson
            bytes = [Text.Encoding]::UTF8.GetByteCount($ArtifactJson)
            count = if ($null -eq $Count) { $null } else { [int]$Count }
        })
    }
    $Report = [ordered]@{
        schema_version = '1.0.0'
        claim_scope = 'current-baseline-only'
        started_utc = $StartedUtc.ToUniversalTime().ToString('yyyy-MM-ddTHH:mm:ssZ')
        finished_utc = $GeneratedUtc.ToUniversalTime().ToString('yyyy-MM-ddTHH:mm:ssZ')
        generated_utc = $GeneratedUtc.ToUniversalTime().ToString('yyyy-MM-ddTHH:mm:ssZ')
        source = $Source
        environment = [ordered]@{
            engine_association = Get-OWSEngineAssociation -RepositoryRoot $Root
            engine_root = if ([string]::IsNullOrWhiteSpace($EngineRootLabel)) { $null } else { $EngineRootLabel }
            engine_version = if ([string]::IsNullOrWhiteSpace($EngineVersion)) { $null } else { $EngineVersion }
            engine_changelist = if ($null -eq $EngineChangelist) { $null } else { [long]$EngineChangelist }
            compatible_changelist = if ($null -eq $CompatibleChangelist) { $null } else { [long]$CompatibleChangelist }
            ubt_version = if ([string]::IsNullOrWhiteSpace($UBTVersion)) { $null } else { $UBTVersion }
            os = [System.Environment]::OSVersion.VersionString
            architecture = if ([string]::IsNullOrWhiteSpace($env:PROCESSOR_ARCHITECTURE)) { 'unknown' } else { $env:PROCESSOR_ARCHITECTURE }
            editor_target = [ordered]@{
                target = 'OWSEditor'
                platform = 'Win64'
                configuration = 'Development'
            }
            package_target = [ordered]@{
                target = 'OWS'
                platform = 'Win64'
                configuration = 'Development'
            }
            composition_profile = 'legacy-current-defaults-no-profile-system'
            static_only = [bool]$StaticOnly
        }
        commands = @($Commands)
        results = $Results
        inventories = $Inventories
        findings = $Findings
        artifacts = @($Artifacts | ForEach-Object { $_ })
        immutability = $Immutability
        summary = [ordered]@{
            capture_status = 'VALID'
            known_gap_count = $KnownCount
            new_unowned_gap_count = $NewCount
            observation_count = $ObservationCount
            candidate_resolved_count = $ResolvedCount
            target_conformance_claimed = $false
            statement = 'This report records current-state evidence only. It neither enforces nor claims the accepted target architecture.'
        }
    }
    return $Report
}

function ConvertTo-OWSJson {
    [CmdletBinding()]
    param([Parameter(Mandatory)]$InputObject)
    return (($InputObject | ConvertTo-Json -Depth 100) + "`n")
}

function Invoke-OWSJsonSchemaValidation {
    param(
        [Parameter(Mandatory)][string]$Json,
        [Parameter(Mandatory)][string]$SchemaPath
    )

    $ResolvedSchema = [System.IO.Path]::GetFullPath($SchemaPath)
    if (-not (Test-Path -LiteralPath $ResolvedSchema -PathType Leaf)) {
        return [pscustomobject]@{ valid = $false; error = 'JSON Schema file is missing.' }
    }

    $NativeValidator = Get-Command Test-Json -ErrorAction SilentlyContinue
    if ($null -ne $NativeValidator) {
        try {
            $Valid = [bool](Test-Json -Json $Json -SchemaFile $ResolvedSchema -ErrorAction Stop)
            return [pscustomobject]@{ valid = $Valid; error = if ($Valid) { $null } else { 'JSON Schema rejected the report.' } }
        }
        catch {
            return [pscustomobject]@{ valid = $false; error = ConvertTo-OWSSanitizedText -Text $_.Exception.Message }
        }
    }

    $Pwsh = Get-Command pwsh.exe -ErrorAction SilentlyContinue
    if ($null -eq $Pwsh) {
        $Pwsh = Get-Command pwsh -ErrorAction SilentlyContinue
    }
    if ($null -eq $Pwsh) {
        return [pscustomobject]@{ valid = $false; error = 'A draft-aware JSON Schema validator is unavailable; PowerShell 7 Test-Json is required.' }
    }

$ValidationScript = @'
[Console]::InputEncoding = [Text.Encoding]::GetEncoding([int]$env:OWS_BASELINE_SCHEMA_INPUT_CODE_PAGE)
$json = [Console]::In.ReadToEnd()
try {
    $valid = [bool](Test-Json -Json $json -SchemaFile $env:OWS_BASELINE_SCHEMA_VALIDATION_PATH -ErrorAction Stop)
    if ($valid) { exit 0 }
    exit 2
}
catch {
    [Console]::Error.WriteLine($_.Exception.Message)
    exit 3
}
'@
    $EncodedCommand = [Convert]::ToBase64String([Text.Encoding]::Unicode.GetBytes($ValidationScript))
    $StartInfo = New-Object System.Diagnostics.ProcessStartInfo
    $StartInfo.FileName = $Pwsh.Source
    $StartInfo.Arguments = "-NoLogo -NoProfile -NonInteractive -EncodedCommand $EncodedCommand"
    $StartInfo.UseShellExecute = $false
    $StartInfo.CreateNoWindow = $true
    $StartInfo.RedirectStandardInput = $true
    $StartInfo.RedirectStandardOutput = $true
    $StartInfo.RedirectStandardError = $true
    $StandardInputEncoding = New-Object System.Text.UTF8Encoding($false)
    $StartInfo.EnvironmentVariables['OWS_BASELINE_SCHEMA_VALIDATION_PATH'] = $ResolvedSchema
    $StartInfo.EnvironmentVariables['OWS_BASELINE_SCHEMA_INPUT_CODE_PAGE'] = [string]$StandardInputEncoding.CodePage
    $Process = New-Object System.Diagnostics.Process
    $Process.StartInfo = $StartInfo
    try {
        [void]$Process.Start()
        $InputBytes = $StandardInputEncoding.GetBytes($Json)
        $Process.StandardInput.BaseStream.Write($InputBytes, 0, $InputBytes.Length)
        $Process.StandardInput.BaseStream.Flush()
        $Process.StandardInput.Close()
        $StandardOutput = $Process.StandardOutput.ReadToEnd()
        $StandardError = $Process.StandardError.ReadToEnd()
        $Process.WaitForExit()
        if ($Process.ExitCode -eq 0) {
            return [pscustomobject]@{ valid = $true; error = $null }
        }
        $Message = ($StandardError + "`n" + $StandardOutput).Trim()
        if ([string]::IsNullOrWhiteSpace($Message)) { $Message = 'JSON Schema rejected the report.' }
        return [pscustomobject]@{ valid = $false; error = ConvertTo-OWSSanitizedText -Text $Message }
    }
    catch {
        return [pscustomobject]@{ valid = $false; error = ConvertTo-OWSSanitizedText -Text $_.Exception.Message }
    }
    finally {
        $Process.Dispose()
    }
}

function Test-OWSBaselineReport {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)]$Report,
        [Parameter(Mandatory)][string]$RepositoryRoot,
        [string]$SchemaPath = (Join-Path $PSScriptRoot 'ows-platform-baseline.schema.json')
    )

    $Errors = New-Object System.Collections.Generic.List[string]
    $Json = ConvertTo-OWSJson -InputObject $Report
    $SchemaValidation = Invoke-OWSJsonSchemaValidation -Json $Json -SchemaPath $SchemaPath
    if (-not $SchemaValidation.valid) {
        $Errors.Add('JSON Schema validation failed: ' + $SchemaValidation.error)
    }
    foreach ($Required in @('schema_version', 'claim_scope', 'started_utc', 'finished_utc', 'generated_utc', 'source', 'environment', 'commands', 'results', 'inventories', 'findings', 'artifacts', 'immutability', 'summary')) {
        $Present = if ($Report -is [System.Collections.IDictionary]) {
            $Report.Contains($Required)
        }
        else {
            $null -ne $Report.PSObject.Properties[$Required]
        }
        if (-not $Present) {
            $Errors.Add("Missing required report field: $Required")
        }
    }
    if ((Get-OWSObjectPropertyValue -Object $Report -Name 'schema_version' -Default '') -ne '1.0.0') {
        $Errors.Add('schema_version must be 1.0.0')
    }
    if ((Get-OWSObjectPropertyValue -Object $Report -Name 'claim_scope' -Default '') -ne 'current-baseline-only') {
        $Errors.Add('claim_scope must be current-baseline-only')
    }
    foreach ($Finding in @(Get-OWSObjectPropertyValue -Object $Report -Name 'findings' -Default @())) {
        if ([string]$Finding.classification -notin @('known_gap', 'new_unowned_gap', 'observation', 'candidate_resolved')) {
            $Errors.Add("Invalid finding classification: $($Finding.classification)")
        }
        if ($Finding.classification -eq 'known_gap' -and @($Finding.owners).Count -eq 0) {
            $Errors.Add("Known gap has no owning issue: $($Finding.family_id)")
        }
    }
    foreach ($Config in @($Report.inventories.config_keys)) {
        if ($Config.sensitive -and $null -ne $Config.value_digest) {
            $Errors.Add("Sensitive configuration entry serialized a digest: $($Config.file):$($Config.line)")
        }
    }

    $ResolvedRoot = [System.IO.Path]::GetFullPath($RepositoryRoot)
    if ($Json.IndexOf($ResolvedRoot, [System.StringComparison]::OrdinalIgnoreCase) -ge 0 -or
        $Json.IndexOf($ResolvedRoot.Replace('\', '/'), [System.StringComparison]::OrdinalIgnoreCase) -ge 0) {
        $Errors.Add('Report contains an absolute repository path.')
    }
    if ((ConvertTo-OWSSanitizedText -Text $Json) -cne $Json -or
        $Json -match '(?i)-----BEGIN [^-\r\n]*PRIVATE KEY-----|-----END [^-\r\n]*PRIVATE KEY-----') {
        $Errors.Add('Report contains text that the delivery redactor identifies as a credential, authorization value, signed URL, or private-key material.')
    }
    return [pscustomobject]@{ valid = ($Errors.Count -eq 0); errors = @($Errors) }
}

function Write-OWSBaselineReport {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)]$Report,
        [Parameter(Mandatory)][string]$OutputPath
    )

    $Resolved = [System.IO.Path]::GetFullPath($OutputPath)
    $Parent = [System.IO.Path]::GetDirectoryName($Resolved)
    if ([string]::IsNullOrWhiteSpace($Parent)) {
        throw 'OutputPath must include a parent directory.'
    }
    [void][System.IO.Directory]::CreateDirectory($Parent)
    $Temporary = $Resolved + '.tmp-' + [guid]::NewGuid().ToString('N')
    try {
        $Encoding = New-Object System.Text.UTF8Encoding($false)
        [System.IO.File]::WriteAllText($Temporary, (ConvertTo-OWSJson -InputObject $Report), $Encoding)
        [System.IO.File]::Copy($Temporary, $Resolved, $true)
    }
    finally {
        if (Test-Path -LiteralPath $Temporary -PathType Leaf) {
            [System.IO.File]::Delete($Temporary)
        }
    }
    return $Resolved
}

function Assert-OWSExternalOutputPath {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)][string]$RepositoryRoot,
        [Parameter(Mandatory)][string]$OutputPath
    )

    $Root = [System.IO.Path]::GetFullPath($RepositoryRoot).TrimEnd('\', '/')
    $Resolved = [System.IO.Path]::GetFullPath($OutputPath)
    $Prefix = $Root + [System.IO.Path]::DirectorySeparatorChar
    if ($Resolved.Equals($Root, [System.StringComparison]::OrdinalIgnoreCase) -or
        $Resolved.StartsWith($Prefix, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw 'OutputPath must be outside the repository.'
    }
    if ([System.IO.Path]::GetExtension($Resolved) -ne '.json') {
        throw 'OutputPath must name a JSON file.'
    }
    return $Resolved
}

Export-ModuleMember -Function @(
    'Assert-OWSExternalOutputPath',
    'Compare-OWSRepositoryState',
    'ConvertTo-OWSJson',
    'ConvertTo-OWSSanitizedText',
    'Get-OWSBuildDependencyInventory',
    'Get-OWSConfigInventory',
    'Get-OWSCookRuleInventory',
    'Get-OWSDescriptorInventory',
    'Get-OWSFileManifest',
    'Get-OWSFileSha256',
    'Get-OWSPublicSurfaceInventory',
    'Get-OWSRepositoryState',
    'Get-OWSSha256Text',
    'Get-OWSTestInventory',
    'Get-OWSTextReferenceInventory',
    'New-OWSStaticBaselineReport',
    'Read-OWSAssetBaseline',
    'Read-OWSJsonFile',
    'Resolve-OWSFindings',
    'Test-OWSBaselineReport',
    'Write-OWSBaselineReport'
)

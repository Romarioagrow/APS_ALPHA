# Read-only preflight and immutable evidence for paired surface-pipeline trials.
# This helper never builds, bakes, changes a project cvar, or starts Unreal.
# Candidate now observes the real unified default. Only Control opts out, in an
# editor/dev-automation process; pre-default evidence is not the same A/B pair.
function Get-APSSurfaceUnificationMode {
    param([Parameter(Mandatory)][ValidateSet('Control','Candidate')][string]$Mode)
    [pscustomobject]@{
        SchemaVersion = 2
        Mode = $Mode
        RuntimePipeline = $(if ($Mode -eq 'Control') { 'LegacyControl' } else { 'UnifiedDefault' })
        Flag = $(if ($Mode -eq 'Control') { '-APSLegacySurfacePipelineControl' } else { '' })
    }
}

function Get-APSSurfaceUnificationDiagnostic {
    param([Parameter(Mandatory)][string]$ProjectRoot,
        [Parameter(Mandatory)][ValidateSet('Control','Candidate')][string]$Mode)
    $dll = Get-Item -LiteralPath (Join-Path $ProjectRoot 'Binaries/Win64/UnrealEditor-APS_ALPHA.dll')
    $sources = @(
        'Core/Planetary/APSPlanetSurfaceMaterialPolicy.h',
        'Core/Planetary/APSTerrainContinuityMaterial.h',
        'Core/Planetary/APSCoastalWaterMaterial.h',
        'Core/Planetary/APSNativeTerrainMaterial.h',
        'Core/Planetary/APSUnifiedLavaSurface.h',
        'Core/Planetary/APSSharedTerrainMaterial.h',
        'Core/Planetary/APSSharedGeneratedLiquidMaterial.h',
        'Core/Planetary/APSShoreWaterMaterial.h',
        'Core/Planetary/APSPlanetSurfaceProfile.h',
        'Core/Planetary/APSPlanetSurfaceProfile.cpp',
        'Generation/APSWorldScapePlanetNoise.h',
        'Generation/APSWorldScapePlanetNoise.cpp',
        'Core/Planetary/APSCoastalMeshRefinement.h',
        'Generation/APSPreviewCoastalMeshRefinement.h',
        'Generation/AstroGenerator.cpp',
        'Generation/PlanetarySurfaceGeneratorStreaming.cpp',
        'Tests/APSPlanetSurfaceMaterialPolicyTests.cpp',
        'Tests/APSPlanetCryogenicGeometryTests.cpp',
        'Tests/APSPlanetCryogenicPreviewTests.cpp',
        'Tests/APSGeneratedGameplayHandoffSmokeTests.cpp',
        'Tests/APSOrbitalMacroVariationTests.cpp',
        'Tests/APSSharedGeneratedLiquidSelectionTests.cpp')
    $sourceFiles = foreach ($relative in $sources) {
        $source = Get-Item -LiteralPath (Join-Path $ProjectRoot ('Source/APS_ALPHA/' + $relative))
        if ($source.LastWriteTimeUtc -gt $dll.LastWriteTimeUtc) {
            throw "Surface unification source newer than DLL: $relative. Build before either comparison leg."
        }
        $source.FullName
    }
    $folders = @('Shared','ContinuityV1','UnifiedLava','WaterV1','Diagnostics/WaterShoreTransmission20261001')
    $protected = foreach ($folder in $folders) {
        $path = Join-Path $ProjectRoot ('Content/APS/APS_ALPHA/WSC/PlanetSurface/' + $folder)
        if (!(Test-Path -LiteralPath $path -PathType Container)) { throw "Missing protected surface assets: $path" }
        Get-ChildItem -LiteralPath $path -File -Recurse -Filter '*.uasset' | Select-Object -ExpandProperty FullName
    }
    $protected += @(Get-ChildItem -LiteralPath (Join-Path $ProjectRoot 'Config') -File -Filter '*.ini' |
        Select-Object -ExpandProperty FullName)
    $modeSelection = Get-APSSurfaceUnificationMode -Mode $Mode
    [pscustomobject]@{
        SchemaVersion = $modeSelection.SchemaVersion
        Mode = $modeSelection.Mode
        RuntimePipeline = $modeSelection.RuntimePipeline
        Flag = $modeSelection.Flag
        SourceHashes = @(Get-FileHash -LiteralPath $sourceFiles -Algorithm SHA256 | Select-Object Path,Hash)
        ProtectedHashes = @(Get-FileHash -LiteralPath ($protected | Sort-Object -Unique) -Algorithm SHA256 | Select-Object Path,Hash)
        DllHash = (Get-FileHash -LiteralPath $dll.FullName -Algorithm SHA256).Hash
        Acceptance = 'Unverified: Candidate observes the unified runtime default; Control explicitly selects editor-only legacy routes. Compare actual bound materials, ground/orbit frames and motion; contracts are not visual acceptance.'
    }
}

function Save-APSSurfaceUnificationDiagnostic {
    param([Parameter(Mandatory)]$Selection, [Parameter(Mandatory)][string]$RunDir)
    $path = Join-Path $RunDir 'surface-unification.json'
    if (Test-Path -LiteralPath $path) { throw 'Unification evidence already exists; refusing overwrite' }
    $Selection | ConvertTo-Json -Depth 5 | Out-File -LiteralPath $path -Encoding utf8
}

function Assert-APSSurfaceUnificationProtection {
    param([Parameter(Mandatory)][string]$RunDir)
    $evidence = Get-Content -LiteralPath (Join-Path $RunDir 'surface-unification.json') -Raw | ConvertFrom-Json
    foreach ($entry in $evidence.ProtectedHashes) {
        if (!(Test-Path -LiteralPath $entry.Path -PathType Leaf) -or
            (Get-FileHash -LiteralPath $entry.Path -Algorithm SHA256).Hash -ne $entry.Hash) {
            throw "Protected surface/config changed: $($entry.Path)"
        }
    }
    "Protected surface/config hashes unchanged ($($evidence.ProtectedHashes.Count)); visual acceptance remains separate."
}

# Call after BOTH owned processes have ended and their test reports were read.
# This checks experiment comparability/preservation only, not test/visual success.
function Assert-APSSurfaceUnificationPair {
    param([Parameter(Mandatory)][string]$ControlRunDir,
        [Parameter(Mandatory)][string]$CandidateRunDir)
    $control = Get-Content -LiteralPath (Join-Path $ControlRunDir 'surface-unification.json') -Raw | ConvertFrom-Json
    $candidate = Get-Content -LiteralPath (Join-Path $CandidateRunDir 'surface-unification.json') -Raw | ConvertFrom-Json
    if($control.SchemaVersion -ne 2 -or $candidate.SchemaVersion -ne 2 -or
        $control.Mode -ne 'Control' -or $candidate.Mode -ne 'Candidate' -or
        $control.RuntimePipeline -ne 'LegacyControl' -or $candidate.RuntimePipeline -ne 'UnifiedDefault' -or
        $control.Flag -ne '-APSLegacySurfacePipelineControl' -or $candidate.Flag -ne '') {
        throw 'Expected schema-2 LegacyControl/UnifiedDefault evidence; old opt-in candidate runs cannot establish default acceptance'
    }
    if($control.DllHash -ne $candidate.DllHash){throw 'Pair uses different DLLs'}
    foreach($field in @('SourceHashes','ProtectedHashes')) {
        $left = @($control.$field | Sort-Object Path | ForEach-Object { $_.Path+'|'+$_.Hash })
        $right = @($candidate.$field | Sort-Object Path | ForEach-Object { $_.Path+'|'+$_.Hash })
        if(($left -join "`n") -ne ($right -join "`n")){throw "Pair has different $field"}
    }
    $commands = foreach($dir in @($ControlRunDir,$CandidateRunDir)) {
        $command = Get-Content -LiteralPath (Join-Path $dir 'command.txt') -Raw
        $command = $command -replace '(?i)-(UserDir|ReportExportPath|abslog)="[^"]*"',''
        $legacyFlag = '(?i)(?<!\S)-APSLegacySurfacePipelineControl(?=\s|$)'
        $legacyCount = [regex]::Matches($command,$legacyFlag).Count
        $expectedCount = $(if ($dir -eq $ControlRunDir) { 1 } else { 0 })
        if($legacyCount -ne $expectedCount) {
            throw 'Actual command must contain the legacy flag exactly once for Control and never for the unified-default Candidate'
        }
        $command = $command -replace $legacyFlag,''
        # The former candidate argument is a backward-compatible runtime no-op.
        # Ignore only a whole argument, never a similarly named option/value.
        $command = $command -replace '(?i)(?<!\S)-APSUnifiedSurfacePipelineCandidate(?=\s|$)',''
        ($command -replace '\s+',' ').Trim()
    }
    if($commands[0] -ne $commands[1]){throw 'Pair changes camera/fixture/test arguments beyond the legacy-control flag and output paths'}
    Assert-APSSurfaceUnificationProtection -RunDir $ControlRunDir
    Assert-APSSurfaceUnificationProtection -RunDir $CandidateRunDir
    'LegacyControl/UnifiedDefault pair inputs match; inspect actual test reports, bound materials, frames and timing separately.'
}

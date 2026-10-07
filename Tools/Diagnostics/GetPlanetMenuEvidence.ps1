param(
    [Parameter(Mandatory)][ValidatePattern('^[a-z0-9-]+$')][string]$RunName,
    [ValidateRange(1,1000)][int]$ExpectedFrames=12
)
# Read-only evidence collection. Success here NEVER means visual acceptance.
$ErrorActionPreference='Stop'
$runDir='F:/ChatGPT/APOSFERA/work/planet_continuity_20260929/'+$RunName
$reportFile=$runDir+'/report/index.json'
if (-not(Test-Path -LiteralPath $reportFile)) { throw 'Automation report absent; run not complete' }
$report=Get-Content -LiteralPath $reportFile -Raw | ConvertFrom-Json
$batchFamilies=@()
if(Test-Path -LiteralPath ($runDir+'/family-batch.json')) {
    $batch=Get-Content -LiteralPath ($runDir+'/family-batch.json') -Raw | ConvertFrom-Json
    $batchFamilies=@($batch.Families)
    $batchFrames=12*$batchFamilies.Count
    if(-not $batchFamilies.Count -or $batch.ViewsPerFamily -ne 12 -or
       ($PSBoundParameters.ContainsKey('ExpectedFrames') -and $ExpectedFrames -ne $batchFrames)) {throw 'Invalid/mismatched batch evidence contract'}
    $ExpectedFrames=$batchFrames
}
$logLines=Get-Content -LiteralPath ($runDir+'/menu.log')
$currentFamily=$null
$gpuRows=@(foreach($line in $logLines) {
    if($line -notmatch 'LogAutomationController' -and $line -match 'PLANET_PROBE_SETUP family=(\S+)') { $currentFamily=$Matches[1] }
    # The automation report echoes each log entry; count the original only.
    if($line -notmatch 'LogAutomationController' -and
       $line -match 'PLANET_TERRAIN_AB_GPU view=(\S+) variant=(\S+) samples=(\d+) meanMs=([\d.-]+) p95Ms=([\d.-]+)') {
        [pscustomobject]@{
            Family=$currentFamily
            View=$Matches[1]; Variant=$Matches[2]; Samples=[int]$Matches[3]
            MeanMs=[double]::Parse($Matches[4],[Globalization.CultureInfo]::InvariantCulture)
            P95Ms=[double]::Parse($Matches[5],[Globalization.CultureInfo]::InvariantCulture)
        }
    }
})
$frames=@(Get-ChildItem -LiteralPath ($runDir+'/Saved/Automation/PlanetRefinement') -File -Recurse -Filter '*.png' -ErrorAction SilentlyContinue)
$before=@(Get-Content -LiteralPath ($runDir+'/assets-before.json') -Raw | ConvertFrom-Json)
$changedAssets=@(foreach($asset in $before) {
    if(-not(Test-Path -LiteralPath $asset.Path) -or
       (Get-FileHash -LiteralPath $asset.Path -Algorithm SHA256).Hash -ne $asset.Hash) {
        $asset.Path
    }
})
$recordedDll=$null
$matchesInstalledDll=$null
if(Test-Path -LiteralPath ($runDir+'/dll.json')) {
    $recordedDll=Get-Content -LiteralPath ($runDir+'/dll.json') -Raw | ConvertFrom-Json
    $matchesInstalledDll=(Get-FileHash -LiteralPath $recordedDll.Path -Algorithm SHA256).Hash -eq $recordedDll.Hash
}
$problems=@()
if($report.failed -gt 0) { $problems+='Automation failed' }
if($report.notRun -gt 0 -or $report.inProcess -gt 0) { $problems+='Automation incomplete' }
if($batchFamilies.Count) {
    foreach($family in $batchFamilies) {
        $path='APS.Rendered.PlanetRefinement.OrbitalFieldsFamily.'+$family
        $renderedTest=@($report.tests | Where-Object {$_.fullTestPath -eq $path})
        if($renderedTest.Count -ne 1 -or $renderedTest[0].state -ne 'Success') {$problems+='Rendered batch case did not succeed: '+$family}
        if(@($frames | Where-Object {$_.Directory.Parent.Name -eq $family}).Count -ne 12) {$problems+='Missing batch frames: '+$family}
        if(@($gpuRows | Where-Object {$_.Family -eq $family}).Count -ne 12) {$problems+='Missing batch GPU samples: '+$family}
    }
} else {
    $renderedTest=@($report.tests | Where-Object { $_.fullTestPath -eq 'APS.Rendered.PlanetRefinement.CausalLayers' })
    if($renderedTest.Count -ne 1 -or $renderedTest[0].state -ne 'Success') { $problems+='Rendered fixture did not succeed' }
}
if($frames.Count -ne $ExpectedFrames) { $problems+='Unexpected screenshot count' }
if($gpuRows.Count -ne $ExpectedFrames) { $problems+='Unexpected GPU sample row count' }
if(@($gpuRows | Where-Object { $_.Samples -le 0 -or $_.MeanMs -lt 0 -or $_.P95Ms -lt 0 }).Count) { $problems+='Missing GPU observations' }
if($before.Count -ne 8 -or $changedAssets.Count) { $problems+='Protected Shared asset set differs' }
[pscustomobject]@{
    Run=$runDir
    EvidenceChecksPassed=($problems.Count -eq 0)
    Problems=$problems
    VisualAcceptance='NOT EVALUATED: inspect all A/B frames and live traversal separately'
    BatchFamilies=$batchFamilies
    TimingScope='Settled menu async GPU counter; not live-flight FPS or hitch acceptance'
    Tests=@($report.tests | Select-Object fullTestPath,state)
    Warnings=$report.succeededWithWarnings
    FrameCount=$frames.Count
    Frames=@($frames | Sort-Object FullName | ForEach-Object { $_.FullName })
    Gpu=$gpuRows
    ProtectedSharedCount=$before.Count
    ChangedShared=$changedAssets
    RecordedDllHash=$(if($recordedDll){$recordedDll.Hash}else{$null})
    RecordedDllStillInstalled=$matchesInstalledDll
    FailureLog=@($logLines | Where-Object { $_ -match 'LogAutomationController: Error:|LogTemp: Error:' } | Select-Object -Last 8)
} | ConvertTo-Json -Depth 5
if($problems.Count) { exit 1 }

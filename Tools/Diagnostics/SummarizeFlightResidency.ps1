param([Parameter(Mandatory)][string[]]$Runs)
$ErrorActionPreference='Stop'
function Percentile($Values,[double]$P) {
    $sorted=@($Values | Sort-Object)
    if(-not $sorted.Count){return $null}
    return [math]::Round($sorted[[math]::Min($sorted.Count-1,[math]::Ceiling($sorted.Count*$P)-1)],3)
}
foreach($run in $Runs) {
    $frames=@(Import-Csv -LiteralPath (Join-Path $run 'Saved/Diagnostics/FlightResidency.csv'))
    $report=Get-Content -LiteralPath (Join-Path $run 'report/index.json') -Raw | ConvertFrom-Json
    $dll=Get-Content -LiteralPath (Join-Path $run 'dll.json') -Raw | ConvertFrom-Json
    $log=Get-Content -LiteralPath (Join-Path $run 'gameplay.log')
    # Automation repeats events in its report. Count original LogTemp entries only.
    $events=@($log | Where-Object {$_ -match ']LogTemp: Display: \[APS.FlightResidency\]'})
    $snapshots=@(foreach($line in $log) {
        if($line -match ']LogTemp: Display: \[APS.FlightResidency.Memory\] t=(\S+) roots=(\d+) registeredMeshes=(\d+) vertices=(\d+) publishedBytes=(\d+) workers=(\d+) processRAMMiB=(\S+) transit=(\d+)') {
            [pscustomobject]@{t=[double]$Matches[1];roots=[int]$Matches[2];meshes=[int]$Matches[3];vertices=[long]$Matches[4];publishedMiB=[long]$Matches[5]/1MB;workers=[int]$Matches[6];processRAMMiB=[double]$Matches[7];transit=[int]$Matches[8]}
        }
    })
    $metrics=@(foreach($phase in @('all','departure','transit_hold','return','near_hold')) {
        $rows=@($frames | Where-Object {
            $t=[double]$_.t
            ($phase -eq 'all') -or ($phase -eq 'departure' -and $t -lt 10) -or
            ($phase -eq 'transit_hold' -and $t -ge 10 -and $t -lt 18) -or
            ($phase -eq 'return' -and $t -ge 18 -and $t -lt 28) -or
            ($phase -eq 'near_hold' -and $t -ge 28)
        })
        $dt=@($rows | ForEach-Object {[double]$_.dt_ms})
        $gpu=@($rows | ForEach-Object {[double]$_.gpu_ms})
        $worst=$rows | Sort-Object {[double]$_.dt_ms} -Descending | Select-Object -First 1
        [pscustomobject]@{phase=$phase;frames=$rows.Count;frameP95=Percentile $dt .95;frameP99=Percentile $dt .99;max=Percentile $dt 1;worstAt=$worst.t;over33=@($dt | Where-Object {$_ -gt 33.333}).Count;over50=@($dt | Where-Object {$_ -gt 50}).Count;gpuP50=Percentile $gpu .5}
    })
    [pscustomobject]@{run=$run;dll=$dll.Hash;succeeded=$report.succeeded;warnings=$report.succeededWithWarnings;failed=$report.failed;preparing=@($events | Where-Object {$_ -match '] preparing '}).Count;commits=@($events | Where-Object {$_ -match '] commit '}).Count;retired=@($events | Where-Object {$_ -match '] retirement complete'}).Count;metrics=$metrics;snapshots=$snapshots}
}

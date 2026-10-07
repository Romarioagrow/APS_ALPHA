# Read-only preflight for this workstation. Conservative launch thresholds are
# not a prediction of peak use; never stop/unload another user's GPU workload.
$ErrorActionPreference='Stop'
$taskOs=Get-CimInstance Win32_OperatingSystem
$taskRamGiB=[double]$taskOs.FreePhysicalMemory/1MB
$taskCommitGiB=[double]$taskOs.FreeVirtualMemory/1MB
$taskSmi=Get-Command nvidia-smi -ErrorAction Stop
$taskRows=@(& $taskSmi.Source --query-gpu=utilization.gpu,memory.free --format=csv,noheader,nounits)
if($LASTEXITCODE -ne 0 -or $taskRows.Count -ne 1) {
    throw 'GPU headroom unavailable or multiple adapters: no automatic UE launch; inspect manually'
}
$taskParts=$taskRows[0].Split(',')
if($taskParts.Count -ne 2) { throw 'Unexpected GPU telemetry; no UE launch' }
$taskGpuBusy=[double]::Parse($taskParts[0].Trim(),[Globalization.CultureInfo]::InvariantCulture)
$taskVramGiB=[double]::Parse($taskParts[1].Trim(),[Globalization.CultureInfo]::InvariantCulture)/1024
$taskSummary='RAM={0:F2}GiB commit={1:F2}GiB VRAM={2:F2}GiB GPU={3:F0}%' -f $taskRamGiB,$taskCommitGiB,$taskVramGiB,$taskGpuBusy
if($taskRamGiB -lt 8 -or $taskCommitGiB -lt 12 -or $taskVramGiB -lt 6 -or $taskGpuBusy -ge 70) {
    throw ('Insufficient headroom for planet GPU validation; no UE process launched. '+$taskSummary)
}
Write-Host ('Planet GPU launch preflight passed: '+$taskSummary)

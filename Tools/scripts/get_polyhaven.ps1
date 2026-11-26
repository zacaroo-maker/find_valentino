# Download Poly Haven (CC0) models as glTF at a given resolution.
# usage: get_polyhaven.ps1 -Ids shrub_01,fern_02 [-Res 2k]
param([string[]]$Ids, [string]$Res = '2k', [string]$Dest = 'C:\Users\calif\Documents\Finding Valentino\Downloads\PolyHaven')
$ProgressPreference = 'SilentlyContinue'
$ua = 'FV-asset-fetch'
foreach ($id in $Ids) {
  $dir = Join-Path $Dest $id
  try {
    $f = Invoke-RestMethod -Uri "https://api.polyhaven.com/files/$id" -UserAgent $ua -TimeoutSec 60
    $r = $Res; if (-not $f.gltf.$r) { $r = ($f.gltf.PSObject.Properties.Name | Sort-Object)[0] }
    $g = $f.gltf.$r.gltf
    New-Item -ItemType Directory -Force $dir | Out-Null
    $main = Join-Path $dir ([IO.Path]::GetFileName($g.url))
    if (-not (Test-Path $main)) { Invoke-WebRequest -Uri $g.url -OutFile $main -UserAgent $ua -TimeoutSec 300 }
    $total = $g.size
    foreach ($p in $g.include.PSObject.Properties) {
      $out = Join-Path $dir ($p.Name -replace '/', '\')
      New-Item -ItemType Directory -Force (Split-Path $out) | Out-Null
      if (-not (Test-Path $out)) { Invoke-WebRequest -Uri $p.Value.url -OutFile $out -UserAgent $ua -TimeoutSec 300 }
      $total += $p.Value.size
    }
    "OK   $id ($r) $([math]::Round($total/1MB,1)) MB"
  } catch { "FAIL $id : $($_.Exception.Message)" }
}

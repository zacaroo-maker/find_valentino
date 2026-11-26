# Download Poly Haven (CC0) PBR textures (diffuse, normal-GL, ARM) as 2k JPG.
param([string[]]$Ids, [string]$Res = '2k', [string]$Dest = 'C:\Users\calif\Documents\Finding Valentino\Downloads\PolyHavenTex')
$ProgressPreference = 'SilentlyContinue'; $ua = 'FV-asset-fetch'
foreach ($id in $Ids) {
  try {
    $f = Invoke-RestMethod -Uri "https://api.polyhaven.com/files/$id" -UserAgent $ua -TimeoutSec 60
    $dir = Join-Path $Dest $id; New-Item -ItemType Directory -Force $dir | Out-Null
    $got = @()
    foreach ($map in @(@('Diffuse','diff'), @('nor_gl','nor_gl'), @('arm','arm'))) {
      $m = $f.($map[0]); if (-not $m) { continue }
      $e = $m.$Res.jpg; if (-not $e) { continue }
      $out = Join-Path $dir "${id}_$($map[1])_$Res.jpg"
      if (-not (Test-Path $out)) { Invoke-WebRequest -Uri $e.url -OutFile $out -UserAgent $ua -TimeoutSec 300 }
      $got += $map[1]
    }
    "OK   $id : $($got -join ',')"
  } catch { "FAIL $id : $($_.Exception.Message)" }
}

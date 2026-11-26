# Click "+" (Add to project) on one Fab My-Library card, then wait until its Megascans folder appears on disk
# and Unreal's game thread is ticking again. Usage: fab_add_one.ps1 -X <screen x> -Y <screen y> -Folder <name prefix>
param([int]$X, [int]$Y, [string]$Folder, [int]$TimeoutSec = 1500)
$scr = 'C:\Users\calif\AppData\Local\Temp\claude\60601468fcb4cac0\d09299d4-9ed3-4236-b876-e8beb4617b44\scratchpad'
$plants = 'D:\Unreal projects\FindingValentino_UE58\Content\Fab\Megascans'
$py = 'D:\Epic Games\UE_5.4\Engine\Binaries\ThirdParty\Python3\Win64\python.exe'
$probe = 'C:\Users\calif\Documents\Finding Valentino\Downloads\_fablist.py'   # read-only check (saving mid-import can hang the editor)
$save = 'C:\Users\calif\Documents\Finding Valentino\Downloads\_fabsave2.py'
& "$scr\click.ps1" -Title 'NOMATCH_x' -X $X -Y $Y -Wait 1000 | Out-Null
$t0 = Get-Date
while (((Get-Date) - $t0).TotalSeconds -lt $TimeoutSec) {
  Start-Sleep 20
  $o = & $py -I 'C:\Users\calif\Documents\Finding Valentino\scripts\ue_remote.py' $probe 2>$null
  if ($o -match $Folder) {
    Start-Sleep 30   # let the import finish its last steps
    & $py -I 'C:\Users\calif\Documents\Finding Valentino\scripts\ue_remote.py' $save | Out-Null
    "DONE $Folder after $([int]((Get-Date)-$t0).TotalSeconds)s"; exit 0
  }
}
"TIMEOUT $Folder after $TimeoutSec s"; exit 1


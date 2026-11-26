param([string]$Keys, [int]$HoldKey = 0, [int]$HoldMs = 0)
# Focus Unreal, then send SendKeys string and/or hold a virtual key (e.g. 0x57 = W)
Add-Type -AssemblyName System.Windows.Forms
Add-Type @"
using System; using System.Runtime.InteropServices;
public class K { [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
[DllImport("user32.dll")] public static extern void keybd_event(byte k, byte s, uint f, UIntPtr e); }
"@
$p = Get-Process UnrealEditor | Where-Object { $_.MainWindowTitle -match 'Unreal Editor' } | Select-Object -First 1
[K]::keybd_event(0x12,0,0,[UIntPtr]::Zero); [K]::keybd_event(0x12,0,2,[UIntPtr]::Zero)
[K]::SetForegroundWindow($p.MainWindowHandle) | Out-Null
Start-Sleep -Milliseconds 600
if ($Keys) { [System.Windows.Forms.SendKeys]::SendWait($Keys) }
if ($HoldKey) { [K]::keybd_event([byte]$HoldKey,0,0,[UIntPtr]::Zero); Start-Sleep -Milliseconds $HoldMs; [K]::keybd_event([byte]$HoldKey,0,2,[UIntPtr]::Zero) }
"sent"

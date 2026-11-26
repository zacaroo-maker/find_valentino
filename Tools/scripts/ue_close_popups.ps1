# Close Unreal Editor's floating tool windows (Content Browser, Message Log...) that steal keyboard input.
# Leaves the main editor window alone.
Add-Type @"
using System; using System.Text; using System.Runtime.InteropServices;
public class Popups {
  public delegate bool EnumProc(IntPtr h, IntPtr l);
  [DllImport("user32.dll")] public static extern bool EnumWindows(EnumProc cb, IntPtr l);
  [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr h, out uint pid);
  [DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr h);
  [DllImport("user32.dll")] public static extern int GetWindowText(IntPtr h, StringBuilder s, int n);
  [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr h, uint m, IntPtr w, IntPtr l);
}
"@
$main = Get-Process UnrealEditor -ErrorAction SilentlyContinue | Where-Object { $_.MainWindowTitle -match 'Unreal Editor' } | Select-Object -First 1
if (-not $main) { "no editor"; exit }
$closed = @()
$cb = [Popups+EnumProc]{
  param($h, $l)
  $procId = 0; [Popups]::GetWindowThreadProcessId($h, [ref]$procId) | Out-Null
  if ($procId -eq $main.Id -and $h -ne $main.MainWindowHandle -and [Popups]::IsWindowVisible($h)) {
    $sb = New-Object System.Text.StringBuilder 256; [Popups]::GetWindowText($h, $sb, 256) | Out-Null
    $t = $sb.ToString()
    if ($t -match 'Content Browser|Message Log') { [Popups]::PostMessage($h, 0x0010, [IntPtr]::Zero, [IntPtr]::Zero) | Out-Null; $script:closed += $t }
  }
  return $true
}
[Popups]::EnumWindows($cb, [IntPtr]::Zero) | Out-Null
"closed: " + ($closed -join ', ')

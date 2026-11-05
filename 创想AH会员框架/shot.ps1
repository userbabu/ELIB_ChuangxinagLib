# Capture the member window from the screen (what the user actually sees).
# usage: powershell -NoProfile -ExecutionPolicy Bypass -File shot.ps1 <out.png>
# Keep this file pure ASCII: non-ASCII text breaks Windows PowerShell parsing.
param([string]$Out = "shot.png", [string]$Cls = "CxAhMemberWindow")
Add-Type -AssemblyName System.Drawing
Add-Type @"
using System;
using System.Runtime.InteropServices;
public class Cap {
    [DllImport("user32.dll", CharSet = CharSet.Unicode)]
    static extern IntPtr FindWindowW(string cls, string title);
    [DllImport("user32.dll")] static extern bool GetWindowRect(IntPtr h, out RECT r);
    [DllImport("user32.dll")] static extern bool SetForegroundWindow(IntPtr h);
    [DllImport("user32.dll")] static extern bool ShowWindow(IntPtr h, int n);
    [StructLayout(LayoutKind.Sequential)] public struct RECT { public int L, T, R, B; }
    public static string Cls = "CxAhMemberWindow";
    public static IntPtr Find() { return FindWindowW(Cls, null); }
    public static int[] Rect(IntPtr h) { RECT r; GetWindowRect(h, out r); return new int[] { r.L, r.T, r.R - r.L, r.B - r.T }; }
    public static void Raise(IntPtr h) { ShowWindow(h, 3); SetForegroundWindow(h); }
}
"@
[Cap]::Cls = $Cls
$h = [IntPtr]::Zero
for ($i = 0; $i -lt 40; $i++) {
    $h = [Cap]::Find()
    if ($h -ne [IntPtr]::Zero) { break }
    Start-Sleep -Milliseconds 250
}
if ($h -eq [IntPtr]::Zero) { Write-Output "NO WINDOW"; exit 1 }
[Cap]::Raise($h)
Start-Sleep -Milliseconds 500
$rc = [Cap]::Rect($h)
$x = [int]$rc[0]; $y = [int]$rc[1]; $w = [int]$rc[2]; $hh = [int]$rc[3]
$bmp = New-Object System.Drawing.Bitmap $w, $hh
$g = [System.Drawing.Graphics]::FromImage($bmp)
$g.CopyFromScreen($x, $y, 0, 0, (New-Object System.Drawing.Size($w, $hh)))
$g.Dispose()
$dir = Split-Path $Out
if ($dir) { New-Item -ItemType Directory -Force -Path $dir | Out-Null }
$full = Join-Path (Get-Location).Path $Out
$bmp.Save($full, [System.Drawing.Imaging.ImageFormat]::Png)
$bmp.Dispose()
Write-Output "SAVED $Out ${w}x${hh}"

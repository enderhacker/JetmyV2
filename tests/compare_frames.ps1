param(
    [Parameter(Mandatory=$true)][string]$Before,
    [Parameter(Mandatory=$true)][string]$After
)
$ErrorActionPreference = 'Stop'
Add-Type -TypeDefinition @'
using System;
using System.IO;
public static class FrameDifference {
    public static double[] Compare(string before, string after) {
        byte[] a = File.ReadAllBytes(before), b = File.ReadAllBytes(after);
        if (a.Length != b.Length || a.Length % 4 != 0) throw new Exception("Frame dimensions differ");
        long changed = 0, total = 0; int maximum = 0;
        for (int i = 0; i < a.Length; i += 4) {
            bool different = false;
            for (int channel = 0; channel < 3; ++channel) {
                int delta = Math.Abs(a[i + channel] - b[i + channel]);
                different |= delta != 0; total += delta; maximum = Math.Max(maximum, delta);
            }
            if (different) ++changed;
        }
        return new double[] {100.0 * changed / (a.Length / 4), total / (3.0 * a.Length / 4), maximum};
    }
}
'@
Get-ChildItem -LiteralPath $Before -Filter '*.bin' | ForEach-Object {
    $diff = [FrameDifference]::Compare($_.FullName, (Join-Path $After $_.Name))
    [pscustomobject]@{
        Payload = $_.BaseName
        ChangedPercent = $diff[0]
        MeanChannelDelta = $diff[1]
        MaxChannelDelta = $diff[2]
    }
} | Format-Table -AutoSize

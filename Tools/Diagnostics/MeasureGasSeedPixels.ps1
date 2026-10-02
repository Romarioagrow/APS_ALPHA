param(
    [Parameter(Mandatory)][string]$First,
    [Parameter(Mandatory)][string]$Second
)
# Read-only comparison of the central planet disk in the fixed-framing gas probe.
# No image edits and no aesthetic PASS threshold: inspect the images as well.
$ErrorActionPreference='Stop'
Add-Type -AssemblyName System.Drawing
if (-not ('APSGasSeedPixelMetric' -as [type])) {
    $drawingReferences=@([System.Drawing.Bitmap].Assembly.Location,[System.Drawing.Color].Assembly.Location) | Select-Object -Unique
    # PowerShell's .NET10 Drawing implementation forwards Image interfaces here.
    $drawingReferences+=@(Get-ChildItem -LiteralPath ([IO.Path]::GetDirectoryName([System.Drawing.Bitmap].Assembly.Location)) -Filter 'System.Private.Windows*.dll' | Select-Object -ExpandProperty FullName)
    Add-Type -ReferencedAssemblies $drawingReferences -TypeDefinition @'
using System;
using System.Drawing;
public static class APSGasSeedPixelMetric {
    public static double[] Compare(string first, string second) {
        using (var a = new Bitmap(first)) using (var b = new Bitmap(second)) {
            if (a.Width != b.Width || a.Height != b.Height)
                throw new ArgumentException("Capture dimensions differ");
            double cx = a.Width * 0.5, cy = a.Height * 0.5;
            double radius = Math.Min(a.Width, a.Height) * 0.23;
            double sum = 0, max = 0, changed = 0, count = 0;
            for (int y = (int)(cy-radius); y <= cy+radius; ++y)
            for (int x = (int)(cx-radius); x <= cx+radius; ++x) {
                if ((x-cx)*(x-cx)+(y-cy)*(y-cy) > radius*radius) continue;
                Color p = a.GetPixel(x,y), q = b.GetPixel(x,y);
                int r = Math.Abs(p.R-q.R), g = Math.Abs(p.G-q.G), bl = Math.Abs(p.B-q.B);
                sum += r+g+bl; max = Math.Max(max, Math.Max(r, Math.Max(g,bl)));
                if (r+g+bl > 0) ++changed;
                ++count;
            }
            return new double[] {a.Width,a.Height,count,sum/(count*3),max,changed/count};
        }
    }
}
'@
}
$firstPath=(Resolve-Path -LiteralPath $First).Path
$secondPath=(Resolve-Path -LiteralPath $Second).Path
$metric=[APSGasSeedPixelMetric]::Compare($firstPath,$secondPath)
[pscustomobject]@{
    First=$firstPath; Second=$secondPath; Width=$metric[0]; Height=$metric[1]
    Region='Centered disk, radius 0.23 * min(width,height); fixed gas-probe framing only'
    Pixels=$metric[2]; MeanAbsoluteRgbByteDifference=$metric[3]
    MaximumChannelByteDifference=$metric[4]; ChangedPixelFraction=$metric[5]
} | ConvertTo-Json

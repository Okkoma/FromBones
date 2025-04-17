#!/bin/bash

INKSCAPE=inkscape

svg=Input/Launcher/ic_launcher.svg
output=Output/res
ldpi="$output/drawable-ldpi"
mdpi="$output/drawable-mdpi"
hdpi="$output/drawable-hdpi"
xhdpi="$output/drawable-xhdpi"
xxhdpi="$output/drawable-xxhdpi"
xxxhdpi="$output/drawable-xxxhdpi"

echo ldpi=$ldpi mdpi=$mdpi hdpi=$hdpi xhdpi=$xhdpi xxhdpi=$xxhdpi xxxhdpi=$xxxhdpi

mkdir -p "$output" "$ldpi" "$mdpi" "$hdpi" "$xhdpi" "$xxhdpi" "$xxxhdpi"

# configure parameters for Inkscape
inkscape_ver_maj=`$INKSCAPE --version | cut -d ' ' -f 2 | cut -d '.' -f 1`
inkscape_ver_min=`$INKSCAPE --version | cut -d ' ' -f 2 | cut -d '.' -f 2`

echo Export SVG : use inkscape version="$inkscape_ver_maj"."$inkscape_ver_min"
echo Export SVG : use inkscape version="$inkscape_ver_maj"."$inkscape_ver_min" > log.txt

if [ $inkscape_ver_maj -eq 0 ] && [ $inkscape_ver_min -lt 92 ]; then
    paramnogui=-z
    paramexport=--export-png
else
    paramexport=--export-filename
fi

obj=ic_launcher_red
$INKSCAPE $paramnogui --export-id="$obj" --export-dpi=68  $paramexport="$ldpi/$obj.png" "$svg" &>/dev/null
$INKSCAPE $paramnogui --export-id="$obj" --export-dpi=90  $paramexport="$mdpi/$obj.png" "$svg" &>/dev/null
$INKSCAPE $paramnogui --export-id="$obj" --export-dpi=135 $paramexport="$hdpi/$obj.png" "$svg" &>/dev/null
$INKSCAPE $paramnogui --export-id="$obj" --export-dpi=180 $paramexport="$xhdpi/$obj.png" "$svg" &>/dev/null
$INKSCAPE $paramnogui --export-id="$obj" --export-dpi=270 $paramexport="$xxhdpi/$obj.png" "$svg" &>/dev/null
$INKSCAPE $paramnogui --export-id="$obj" --export-dpi=360 $paramexport="$xxxhdpi/$obj.png" "$svg" &>/dev/null

obj=ic_launcher_round_red
$INKSCAPE $paramnogui --export-id="$obj" --export-dpi=68  $paramexport="$ldpi/$obj.png" "$svg" &>/dev/null
$INKSCAPE $paramnogui --export-id="$obj" --export-dpi=90  $paramexport="$mdpi/$obj.png" "$svg" &>/dev/null
$INKSCAPE $paramnogui --export-id="$obj" --export-dpi=135 $paramexport="$hdpi/$obj.png" "$svg" &>/dev/null
$INKSCAPE $paramnogui --export-id="$obj" --export-dpi=180 $paramexport="$xhdpi/$obj.png" "$svg" &>/dev/null
$INKSCAPE $paramnogui --export-id="$obj" --export-dpi=270 $paramexport="$xxhdpi/$obj.png" "$svg" &>/dev/null
$INKSCAPE $paramnogui --export-id="$obj" --export-dpi=360 $paramexport="$xxxhdpi/$obj.png" "$svg" &>/dev/null


#!/bin/bash

INKSCAPE=inkscape

# Parse Arguments
POSITIONAL=()
while [[ $# -gt 0 ]]; do
	key="$1"

	case $key in
		-s|--simulate)
		SIMULATE="true"
		shift # past argument
		;;
		*)    # unknown option
		POSITIONAL+=("$1") # save it in an array for later
		shift # past argument
		;;
	esac

<< 'Commentaire'
			-l|--lib)
			LIBPATH="$2"
			shift # past argument
			shift # past value
			;;
			--default)
			DEFAULT=YES
			shift # past argument
			;;
Commentaire

done

set -- "${POSITIONAL[@]}" # restore positional parameters

cd Input

# create folders
if [ "$SIMULATE" != "true" ]; then
	mkdir -p UI UI/ldpi UI/mdpi UI/hdpi
fi

# create all the pngs
cd SVGUI

inkscape_ver_maj=`$INKSCAPE --version | cut -d ' ' -f 2 | cut -d '.' -f 1`
inkscape_ver_min=`$INKSCAPE --version | cut -d ' ' -f 2 | cut -d '.' -f 2`

echo Export UI SVG : use inkscape version="$inkscape_ver_maj"."$inkscape_ver_min"
echo Export UI SVG : use inkscape version="$inkscape_ver_maj"."$inkscape_ver_min" > log.txt

if [ $inkscape_ver_maj -eq 0 ] && [ $inkscape_ver_min -lt 92 ]; then
	paramnogui=-z
	paramexport=--export-png
else
	paramexport=--export-filename
fi

echo Export UI SVG : use inkscape command parameters "$paramnogui" "$paramexport"
echo Export UI SVG : use inkscape command parameters "$paramnogui" "$paramexport" >> log.txt

for svg in *.svg; do
	echo . export "$svg" :
	echo . export "$svg" : >> log.txt
	
	# inkscape -z -S "$svg" | grep -Ewv '^path[0-9]*|^g[0-9]*|^svg[0-9]*|^layer[0-9]*|^rect[0-9]*|^ellipse[0-9]*|^circle[0-9]*|^use[0-9]*' | cut -d ',' -f 1 > objectids
	$INKSCAPE $paramnogui -S "$svg" | grep -Ewv '^path[0-9]*|^g[0-9]*|^svg[0-9]*|^layer[0-9]*|^rect[0-9]*|^ellipse[0-9]*|^circle[0-9]*|^use[0-9]*' | cut -d ',' -f 1 > objectids

	# extract the headername if exists and use it for each png filename
	header=`echo "$svg" | grep -Eo '_[a-z]*' | sed 's/_//'`

	# the svg filename has a header, add an underscore to it
	if [ -n "$header" ]; then
		header="$header"_
	fi

	for objid in `cat objectids`; do
		# check if the objectid has the header
		filename=`echo "$objid" | grep -Eo ^"$header"`
		# if the objectid has not the header, add the header to the png filename
		if [ "$filename" = "$header" ]; then
			filename="$objid"
		else
			filename="$header$objid"
		fi

		if [ "$SIMULATE" = "true" ]; then
			echo ... simulated export "$filename"
			echo ... simulated export "$filename" >> log.txt
		else
			echo ... export "$filename"
			echo ... export "$filename" >> log.txt
			# inkscape -z --export-id="$objid"  --export-dpi=68 --export-png="../UI/ldpi/$filename.png" "$svg" >> log.txt
			# inkscape -z --export-id="$objid"  --export-dpi=90 --export-png="../UI/mdpi/$filename.png" "$svg" >> log.txt
			# inkscape -z --export-id="$objid" --export-dpi=135 --export-png="../UI/hdpi/$filename.png" "$svg" >> log.txt
			$INKSCAPE $paramnogui --export-id="$objid" --export-dpi=68  $paramexport="../UI/ldpi/$filename.png" "$svg" >> log.txt &>/dev/null
			$INKSCAPE $paramnogui --export-id="$objid" --export-dpi=90  $paramexport="../UI/mdpi/$filename.png" "$svg" >> log.txt &>/dev/null
			$INKSCAPE $paramnogui --export-id="$objid" --export-dpi=135 $paramexport="../UI/hdpi/$filename.png" "$svg" >> log.txt &>/dev/null
		fi
	done
done

rm -f objectids

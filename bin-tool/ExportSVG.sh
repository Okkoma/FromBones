#!/bin/bash

INKSCAPE=inkscape

ALLFILES="*.svg"
FILE=$ALLFILES
EXPORTALL="true"

# Parse Arguments
POSITIONAL=()
while [[ $# -gt 0 ]]; do
	key="$1"

	case $key in
		-s|--simulate)
		SIMULATE="true"
		shift # past argument
		;;
		-f|--file)
		FILE="$2"
		EXPORTENTITIES="true"
		EXPORTALL="false"
		shift # past argument
		;;
		-e|--entities)
		EXPORTENTITIES="true"
		EXPORTALL="false"
		shift # past argument
		;;
		-t|--tiles)
		EXPORTTILES="true"
		EXPORTALL="false"
		shift
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
    
# create folders
if [ "$SIMULATE" != "true" ]; then
	mkdir -p Input/PNG Input/PNG/ldpi Input/PNG/mdpi Input/PNG/hdpi
fi
    
# create all the pngs
if [ "$EXPORTENTITIES" == "true" ] || [ "$EXPORTALL" == "true" ]; then 

    cd Input/SVG

    echo Export SVG : use inkscape command parameters "$paramnogui" "$paramexport"
    echo Export SVG : use inkscape command parameters "$paramnogui" "$paramexport" >> log.txt

    if [ "$FILE" != "$ALLFILES" ]; then
	    FILE=`ls | grep "$FILE"`
	    echo . entry file "$FILE"
	    echo . entry file "$FILE" >> log.txt
    fi

    for svg in $FILE; do
	    echo . export "$svg" :
	    echo . export "$svg" : >> log.txt
	    $INKSCAPE $paramnogui -S "$svg" | grep -Ewv '^path[0-9]*|^g[0-9]*|^svg[0-9]*|^layer[0-9]*|^rect[0-9]*|^ellipse[0-9]*|^circle[0-9]*|^use[0-9]*' | cut -d ',' -f 1 > objectids

	    # extract the headername if exists and use it for each png filename
	    header=`echo "$svg" | grep -Eo '_[a-z]*' | sed 's/_//'`
			    
	    # the svg filename has a header, add an underscore to it
	    if [ -n "$header" ]; then
		    header="$header"_
	    fi

        basedpi=`echo "$svg" | grep -Eo 'export[0-9]*' | sed 's/[a-z]*//'`
        if [ -n "$basedpi" ]; then
	        dpilow=$(( $basedpi * 75 / 100 ))
	        dpimed=$basedpi
	        dpihigh=$(( $basedpi * 15 / 10 ))
	    else
	    	dpilow=68
	        dpimed=90
	        dpihigh=135
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
			    echo ... simulated export "$filename" basedpi=$basedpi dpimed=$dpimed
			    echo ... simulated export "$filename" basedpi=$basedpi dpimed=$dpimed >> log.txt
		    else
			    echo ... export "$filename"
			    echo ... export "$filename" >> log.txt

			    $INKSCAPE $paramnogui --export-id="$objid"  --export-dpi=$dpilow $paramexport="../PNG/ldpi/$filename.png" "$svg" >> log.txt &>/dev/null
			    $INKSCAPE $paramnogui --export-id="$objid"  --export-dpi=$dpimed $paramexport="../PNG/mdpi/$filename.png" "$svg" >> log.txt &>/dev/null
			    $INKSCAPE $paramnogui --export-id="$objid" --export-dpi=$dpihigh $paramexport="../PNG/hdpi/$filename.png" "$svg" >> log.txt &>/dev/null
		    fi
	    done
    done

    rm -f objectids

    cd ../..
fi

# create tiles pngs
if [ "$EXPORTTILES" == "true" ] || [ "$EXPORTALL" == "true" ]; then 

    cd Input/Tiles

    for svg in $FILE; do
        filename=`echo "$svg" | cut -d '.' -f 1`.png
        if [ "$SIMULATE" = "true" ]; then
	        echo ... simulated export "$svg" to "$filename"
	        echo ... simulated export "$svg" to "$filename" >> log.txt
        else
	        echo ... export "$svg"
	        echo ... export "$svg" >> log.txt
	        $INKSCAPE $paramnogui --export-area-page --export-dpi=48 $paramexport="../../Output/Data/Graphics/ldpi/Textures/Grounds/$filename" "$svg" >> log.txt &>/dev/null
	        $INKSCAPE $paramnogui --export-area-page --export-dpi=48 $paramexport="../../Output/Data/Graphics/mdpi/Textures/Grounds/$filename" "$svg" >> log.txt &>/dev/null
	        $INKSCAPE $paramnogui --export-area-page --export-dpi=96 $paramexport="../../Output/Data/Graphics/hdpi/Textures/Grounds/$filename" "$svg" >> log.txt &>/dev/null
        fi
    done
    
    cd ../..
fi


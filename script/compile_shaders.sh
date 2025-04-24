#!/bin/bash

# NOTE : env var VULKAN_SDK must be set.

# Parse Arguments
POSITIONAL=()
while [[ $# -gt 0 ]]; do
	key="$1"

	case $key in
		-d|-debug)
		OPTIONPACK="-debug"
		shift
		;;
	
		*)    # unknown option
		POSITIONAL+=("$1") # save it in an array for later
		shift # past argument
		;;
	esac

done

# restore positional parameters
set -- "${POSITIONAL[@]}"



if [[ "$OSTYPE" == "linux-gnu"* ]]; then
	PLATFORM=linux	
	# check for WSL 
	WSL=`uname -r | grep Microsoft`
	if [[ "$WSL" != "" ]]; then
		PLATFORM=win64
		WINDOWS=1
		LINUX=0
	fi
elif [[ "$OSTYPE" == "darwin"* ]]; then
	PLATFORM=macosx
elif [[ "$OSTYPE" == "cygwin" ]] || [[ "$OSTYPE" == "msys" ]]; then
	PLATFORM=win64
fi

HOME=`pwd`
DIR_SHADERS="Shaders/Vulkan"
INPUT="$HOME/app/src/$DIR_SHADERS"
OUTPUT="$HOME/bin/Data/$DIR_SHADERS"
PACKER="$HOME/bin-tool/SpirvShaderPacker"
COMPILER="$VULKAN_SDK/x86_64/bin/glslc"

echo "compiler=$COMPILER $OPTIONCOMP"
echo "packer=$PACKER $OPTIONPACK"
echo "input=$INPUT"
echo "ouput=$OUTPUT"

mkdir -p $OUTPUT

cd $INPUT

for ext in vert frag
do
	if [[ $ext == "vert" ]]; then
		ext2=vs.spv
		ext3=vs5
	else	
		ext2=ps.spv
		ext3=ps5
	fi
	
	for file in $(ls *.$ext)
	do
		name="${file%.${ext}}" 
		#echo "  compiling $file to $name.$ext2"
		$COMPILER $OPTIONCOMP -o $name.$ext2 $file
		#echo "  packing $name.$ext2 to $name.$ext3"
		$PACKER $OPTIONPACK $name.$ext2
		echo "  generating $name.$ext3"
	done
done
	
mv *.vs5 $OUTPUT
mv *.ps5 $OUTPUT
rm -f *.spv

cd $HOME


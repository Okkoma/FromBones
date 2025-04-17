#!/bin/bash

#./ExportSVG.sh
#./ExportSVGUI.sh

mkdir -p Output Output/Data Output/Data/Graphics

mkdir -p Output/Data/Graphics/ldpi
mkdir -p Output/Data/Graphics/ldpi/UI Output/Data/Graphics/ldpi/Textures Output/Data/Graphics/ldpi/2D
mkdir -p Output/Data/Graphics/ldpi/Textures/Actors Output/Data/Graphics/ldpi/Textures/Grounds Output/Data/Graphics/ldpi/Textures/UI
mkdir -p Output/Data/UI/Graphics/ldpi/UI Output/Data/UI/Graphics/ldpi/Textures/UI

mkdir -p Output/Data/Graphics/mdpi
mkdir -p Output/Data/Graphics/mdpi/UI Output/Data/Graphics/mdpi/Textures Output/Data/Graphics/mdpi/2D
mkdir -p Output/Data/Graphics/mdpi/Textures/Actors Output/Data/Graphics/mdpi/Textures/Grounds Output/Data/Graphics/mdpi/Textures/UI
mkdir -p Output/Data/UI/Graphics/mdpi/UI Output/Data/UI/Graphics/mdpi/Textures/UI

mkdir -p Output/Data/Graphics/hdpi
mkdir -p Output/Data/Graphics/hdpi/UI Output/Data/Graphics/hdpi/Textures Output/Data/Graphics/hdpi/2D
mkdir -p Output/Data/Graphics/hdpi/Textures/Actors Output/Data/Graphics/hdpi/Textures/Grounds Output/Data/Graphics/hdpi/Textures/UI
mkdir -p Output/Data/UI/Graphics/hdpi/UI Output/Data/UI/Graphics/hdpi/Textures/UI

./TexturePacker -px 1 -py 1 -ox 1 -oy 1 "$@"


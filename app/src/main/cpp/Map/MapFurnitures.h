#pragma once

#include "EnumList.hpp"

#define MAP_FURNITURES_SIZE 10

#define MAP_FURNITURES \
	Vegetation01, Vegetation02, Vegetation03, \
	Vegetation04, Vegetation05, Vegetation06, \
	Vegetation07, Vegetation08, Vegetation09, \
	Rocks01

#define MAP_FURNITURES_STRINGS \
	ENUMSTRING( Vegetation01 ), ENUMSTRING( Vegetation02 ), ENUMSTRING( Vegetation03 ), \
	ENUMSTRING( Vegetation04 ), ENUMSTRING( Vegetation05 ), ENUMSTRING( Vegetation06 ), \
	ENUMSTRING( Vegetation07 ), ENUMSTRING( Vegetation08 ), ENUMSTRING( Vegetation09 ), \
	ENUMSTRING( Rocks01 )

LIST(MapFurnitureType, MAP_FURNITURES, MAP_FURNITURES_STRINGS);

#pragma once

#include "EnumList.hpp"

/// Tile Properties
const unsigned TILE_PROPERTIES_SIZE = 5;

#define TILEPROPERTIESFLAGS \
    ShapeType, Blocked, Movable, Breakable, \
    Transparent

#define TILEPROPERTIESFLAGS_STRINGS \
    ENUMSTRING( ShapeType ), ENUMSTRING( Blocked ), ENUMSTRING( Movable ), ENUMSTRING( Breakable ), \
    ENUMSTRING( Transparent )

#define TILEPROPERTIESFLAGS_VALUES \
    0x0003, 0x0004, 0x0008, 0x0010, \
    0x0020

LISTVALUE(TilePropertiesFlag, TILEPROPERTIESFLAGS, TILEPROPERTIESFLAGS_STRINGS, TILEPROPERTIESFLAGS_VALUES);


/// Tile Features
const unsigned MAPFEATURES_SIZE = 26;

#define MAPFEATURES_TYPES \
    NoMapFeature, CorridorInnerSpace, TunnelInnerSpace, RoofInnerSpace, RoomInnerSpace, \
    Door, Window, OuterSpace, InnerSpace, \
    InnerEmpty, Water, Threshold, NoDecal, NoRender,  \
    InnerRoof, OuterRoof, NoConnect, \
    InnerFloor, OuterFloor, \
    RoomFloor, TunnelFloor, \
    RoomWall, CorridorWall, TunnelWall, \
    CorridorPlateForm, RoomPlateForm

#define MAPFEATURES_STR \
    ENUMSTRING( NoMapFeature ), ENUMSTRING( CorridorInnerSpace ), ENUMSTRING( TunnelInnerSpace ), ENUMSTRING( RoofInnerSpace ), ENUMSTRING( RoomInnerSpace ), \
    ENUMSTRING( Door ), ENUMSTRING( Window ), ENUMSTRING( OuterSpace ), ENUMSTRING( InnerSpace ), \
    ENUMSTRING( InnerEmpty ), ENUMSTRING( Water ), ENUMSTRING( Threshold ), ENUMSTRING( NoDecal ), ENUMSTRING( NoRender ), \
    ENUMSTRING( InnerRoof ), ENUMSTRING( OuterRoof ), ENUMSTRING( NoConnect ), \
    ENUMSTRING( InnerFloor ), ENUMSTRING( OuterFloor ), \
    ENUMSTRING( RoomFloor ), ENUMSTRING( TunnelFloor ), \
    ENUMSTRING( RoomWall ), ENUMSTRING( CorridorWall ), ENUMSTRING( TunnelWall ), \
    ENUMSTRING( CorridorPlateForm ), ENUMSTRING( RoomPlateForm )


LIST(MapFeatureType, MAPFEATURES_TYPES, MAPFEATURES_STR);

#ifndef FeatureType
typedef unsigned char FeatureType;
#endif

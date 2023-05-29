#pragma once

#include "EnumList.hpp"

extern const unsigned MAPFEATURES_SIZE;

enum SideBit
{
    TopBit = 0,
    RightBit = 1,
    BottomBit = 2,
    LeftBit = 3,
    TopLeftCornerBit = 4,
    TopRightCornerBit = 5,
    BottomRightCornerBit = 6,
    BottomLeftCornerBit = 7
};

enum ContactSide
{
    NoneSide = 0,                   // 0
    TopSide = 1 << TopBit,          // 1
    RightSide = 1 << RightBit,      // 2
    BottomSide = 1 << BottomBit,    // 4
    LeftSide = 1 << LeftBit,        // 8
    AllSide = LeftSide | RightSide | TopSide | BottomSide, // 15
};

enum CornerBit
{
    TopLeftBit = 0,
    TopRightBit = 1,
    BottomRightBit = 2,
    BottomLeftBit = 3,
};

enum CornerValue
{
    NoCorner = 0,                             // 0
    TopLeftCorner = 1 << TopLeftBit,          // 1
    TopRightCorner = 1 << TopRightBit,        // 2
    BottomRightCorner = 1 << BottomRightBit,  // 4
    BottomLeftCorner = 1 << BottomLeftBit,    // 8
    FourCorners = TopLeftCorner | TopRightCorner | BottomRightCorner | BottomLeftCorner, // 15
};

enum Corner8Value
{
    TopLeftCornerValue = 1 << (TopLeftBit+4),           // 16
    TopRightCornerValue = 1 << (TopRightBit+4),          // 32
    BottomRightCornerValue = 1 << (BottomRightBit+4),    // 64
    BottomLeftCornerValue = 1 << (BottomLeftBit+4),      // 128
    FourCornersValue = TopLeftCornerValue | TopRightCornerValue | BottomRightCornerValue | BottomLeftCornerValue, // 240
};

/*                                                            BL-BR-TR-TL  LEFT-BOTTOM-RIGHT-TOP  CHECK CORNER                           FREE BORDER     ONE CHECK in Tile::GetCornerValue
TL = Corner TOP Left
TR = Corner TOP Right
BR = Corner BOTTOM Right
BL = Corner BOTTOM Left

4-Neighborhood :
    NoConnect = NoneSide 0,                                         0000     0000              No
    TopConnect = TopSide 1,                                         0000     0001              No
    RightConnect = RightSide 2,                                     0000     0010              No
    TopRightConnect = TopSide | RightSide 3,                        0000     0011              NO(1 : TopRight)
    BottomConnect = BottomSide 4,                                   0000     0100              No
    TopBottomConnect = TopSide | BottomSide 5,                      0000     0101              No
    BottomRightConnect = BottomSide | RightSide 6,                  0000     0110              NO(1 : BottomRight)
    TopRightBottomConnect = TopSide | RightSide | BottomSide 7,     0000     0111              Yes(2 : TopRight && BottomRight)    FreeLeft        Corner = IsEmpty(CellBottomRight) ? CornerBottomRight : CornerTopRight
    LeftConnect = LeftSide 8,                                       0000     1000              No
    TopLeftConnect = TopSide | LeftSide 9,                          0000     1001              NO(1 : TopLeft)
    RightLeftConnect = RightSide | LeftSide 10,                     0000     1010              No
    TopRightLeftConnect = TopSide | RightSide | LeftSide 11,        0000     1011              Yes(2 : TopRight && TopLeft)        FreeBottom      Corner = IsEmpty(CellTopRight) ? CornerTopRight : CornerTopLeft
    BottomLeftConnect = BottomSide | LeftSide 12,                   0000     1100              NO(1 : BottomLeft)
    TopBottomLeftConnect = TopSide | BottomSide | LeftSide 13,      0000     1101              Yes(2 : TopLeft && BottomLeft)      FreeRight       Corner = IsEmpty(CellTopLeft) ? CornerTopLeft : CornerBottomLeft
    RightBottomLeftConnect = RightSide | BottomSide | LeftSide 14,  0000     1110              Yes(2 : BottomLeft && BottomRight)  FreeTop         Corner = IsEmpty(CellBottomLeft) ?  CornerBottomLeft : CornerBottomRight
    AllConnect = AllSide 15,                                        0000     1111              NO
    Void = 16 = NoConnection & No Tiles                             0001     0000

Add corners for 8-Neighborhood :
 2 connections                                                BL-BR-TR-TL  LEFT-BOTTOM-RIGHT-TOP
    TopRightConnect_Corner = 17,                                    0010     0011              Value =
    BottomRightConnect_Corner = 18,                                 0100     0110              Value =
    TopLeftConnect_Corner = 19,                                     0001     1001              Value =
    BottomLeftConnect_Corner = 20                                   1000     1100              Value =
 3 connections                                                BL-BR-TR-TL  LEFT-BOTTOM-RIGHT-TOP
    TopRightBottomConnect_CornerBottom = 21,                        0100     0111              Value =
    TopRightBottomConnect_CornerTop = 22,                           0010     0111              Value =
    TopRightBottomConnect_CornerTopBottom = 23,                     0110     0111              Value =
    TopRightLeftConnect_CornerRight = 24,                           0010     1011              Value =
    TopRightLeftConnect_CornerLeft = 25,                            0001     1011              Value =
    TopRightLeftConnect_CornerRightLeft = 26,                       0011     1011              Value =
    TopBottomLeftConnect_CornerTop = 27,                            0001     1101              Value =
    TopBottomLeftConnect_CornerBottom = 28,                         1000     1101              Value =
    TopBottomLeftConnect_CornerTopBottom = 29,                      1001     1101              Value =
    RightBottomLeftConnect_CornerLeft = 30,                         1000     1110              Value =
    RightBottomLeftConnect_CornerRight = 31,                        0100     1110              Value =
    RightBottomLeftConnect_CornerRightLeft = 32,                    1100     1110              Value =
 4 connections                                                BL-BR-TR-TL  LEFT-BOTTOM-RIGHT-TOP
    AllConnect_C4 = 33,                                             1111     1111              Value = 255
    AllConnect_C1_TopLeft = 34,                                     0001     1111              Value =
    AllConnect_C1_TopRight = 35,                                    0010     1111              Value =
    AllConnect_C1_BottomRight = 36,                                 0100     1111              Value =
    AllConnect_C1_BottomLeft = 37,                                  1000     1111              Value =
    AllConnect_C2_TopLeftRight = 38,                                0011     1111              Value =
    AllConnect_C2_BottomRightLeft = 39,                             1100     1111              Value =
    AllConnect_C2_TopBottomRight = 40,                              0110     1111              Value =
    AllConnect_C2_TopBottomLeft = 41,                               1001     1111              Value =
    AllConnect_C2_TopLeft_BottomRight = 42,                         0101     1111              Value =
    AllConnect_C2_TopRight_BottomLeft = 43,                         1010     1111              Value =
    AllConnect_C3_NoBottomRightCorner = 44,                         1011     1111              Value =
    AllConnect_C3_NoBottomLeftCorner = 45,                          0111     1111              Value =
    AllConnect_C3_NoTopRightCorner = 46,                            1101     1111              Value =
    AllConnect_C3_NoTopLeftCorner = 47,                             1110     1111              Value =
*/

#define MAPTILES_4CONNECTED_SIZE MAPFEATURES_SIZE
#define MAPTILES_8CONNECTED_SIZE 48

#define MAPTILES_CONNECT_SIZE 48

#define MAPTILES_NOBORDER_TOPBIT      0
#define MAPTILES_NOBORDER_RIGHTBIT    1
#define MAPTILES_NOBORDER_BOTTOMBIT   2
#define MAPTILES_NOBORDER_LEFTBIT     3

#define MAPTILES_CONNECT \
    NoConnect = NoneSide, TopConnect = TopSide, RightConnect = RightSide, \
    TopRightConnect = TopSide | RightSide, BottomConnect = BottomSide, TopBottomConnect = TopSide | BottomSide, \
    BottomRightConnect = BottomSide | RightSide, TopRightBottomConnect = TopSide | RightSide | BottomSide, \
    LeftConnect = LeftSide, TopLeftConnect = TopSide | LeftSide, RightLeftConnect = RightSide | LeftSide, \
    TopRightLeftConnect = TopSide | RightSide | LeftSide, BottomLeftConnect = BottomSide | LeftSide, \
    TopBottomLeftConnect = TopSide | BottomSide | LeftSide, RightBottomLeftConnect = RightSide | BottomSide | LeftSide, \
    AllConnect = AllSide, Void = 16, \
    TopRightConnect_Corner = 17, BottomRightConnect_Corner = 18, TopLeftConnect_Corner = 19, BottomLeftConnect_Corner = 20, \
    TopRightBottomConnect_CornerBottom = 21, TopRightBottomConnect_CornerTop = 22, TopRightBottomConnect_CornerTopBottom = 23, \
    TopRightLeftConnect_CornerRight = 24, TopRightLeftConnect_CornerLeft = 25, TopRightLeftConnect_CornerRightLeft = 26, \
    TopBottomLeftConnect_CornerTop = 27, TopBottomLeftConnect_CornerBottom = 28, TopBottomLeftConnect_CornerTopBottom = 29, \
    RightBottomLeftConnect_CornerLeft = 30, RightBottomLeftConnect_CornerRight = 31, RightBottomLeftConnect_CornerRightLeft = 32, \
    AllConnect_C4 = 33, AllConnect_C1_TopLeft = 34, AllConnect_C1_TopRight = 35, AllConnect_C1_BottomRight = 36, AllConnect_C1_BottomLeft = 37, \
    AllConnect_C2_TopLeftRight = 38, AllConnect_C2_BottomRightLeft = 39, AllConnect_C2_TopBottomRight = 40, \
    AllConnect_C2_TopBottomLeft = 41, AllConnect_C2_TopLeft_BottomRight = 42, AllConnect_C2_TopRight_BottomLeft = 43, \
    AllConnect_C3_NoBottomRightCorner = 44, AllConnect_C3_NoBottomLeftCorner = 45, AllConnect_C3_NoTopRightCorner = 46, AllConnect_C3_NoTopLeftCorner = 47

#define MAPTILES_CONNECT_STRINGS \
    ENUMSTRING( NoConnect ), ENUMSTRING( TopConnect ), ENUMSTRING( RightConnect ), \
    ENUMSTRING( TopRightConnect ), ENUMSTRING( BottomConnect ), ENUMSTRING( TopBottomConnect ), \
    ENUMSTRING( BottomRightConnect ), ENUMSTRING( TopRightBottomConnect ), \
    ENUMSTRING( LeftConnect ), ENUMSTRING( TopLeftConnect ), ENUMSTRING( RightLeftConnect ), \
    ENUMSTRING( TopRightLeftConnect ), ENUMSTRING( BottomLeftConnect ), \
    ENUMSTRING( TopBottomLeftConnect ), ENUMSTRING( RightBottomLeftConnect ), \
    ENUMSTRING( AllConnect ), ENUMSTRING( Void ), \
    ENUMSTRING( TopRightConnect_Corner), ENUMSTRING( BottomRightConnect_Corner ), ENUMSTRING( TopLeftConnect_Corner ), ENUMSTRING( BottomLeftConnect_Corner ), \
    ENUMSTRING( TopRightBottomConnect_CornerBottom ), ENUMSTRING( TopRightBottomConnect_CornerTop ), ENUMSTRING( TopRightBottomConnect_CornerTopBottom ), \
    ENUMSTRING( TopRightLeftConnect_CornerRight ), ENUMSTRING( TopRightLeftConnect_CornerLeft ), ENUMSTRING( TopRightLeftConnect_CornerRightLeft ), \
    ENUMSTRING( TopBottomLeftConnect_CornerTop ), ENUMSTRING( TopBottomLeftConnect_CornerBottom ), ENUMSTRING( TopBottomLeftConnect_CornerTopBottom ), \
    ENUMSTRING( RightBottomLeftConnect_CornerLeft ), ENUMSTRING( RightBottomLeftConnect_CornerRight ), ENUMSTRING( RightBottomLeftConnect_CornerRightLeft ), \
    ENUMSTRING( AllConnect_C4 ), ENUMSTRING( AllConnect_C1_TopLeft ), ENUMSTRING( AllConnect_C1_TopRight ), ENUMSTRING( AllConnect_C1_BottomRight ), ENUMSTRING( AllConnect_C1_BottomLeft ), \
    ENUMSTRING( AllConnect_C2_TopLeftRight ), ENUMSTRING( AllConnect_C2_BottomRightLeft ), ENUMSTRING( AllConnect_C2_TopBottomRight ), \
    ENUMSTRING( AllConnect_C2_TopBottomLeft ), ENUMSTRING( AllConnect_C2_TopLeft_BottomRight ), ENUMSTRING( AllConnect_C2_TopRight_BottomLeft ), \
    ENUMSTRING( AllConnect_C3_NoBottomRightCorner ), ENUMSTRING( AllConnect_C3_NoBottomLeftCorner ), ENUMSTRING( AllConnect_C3_NoTopRightCorner ), ENUMSTRING( AllConnect_C3_NoTopLeftCorner )


#define MAPTILES_CONNECT_VALUES \
    NoConnect, TopSide, RightSide, \
    TopSide | RightSide, BottomSide, TopSide | BottomSide, \
    BottomSide | RightSide, TopSide | RightSide | BottomSide, \
    LeftSide, TopSide | LeftSide, RightSide | LeftSide, \
    TopSide | RightSide | LeftSide, BottomSide | LeftSide, \
    TopSide | BottomSide | LeftSide, RightSide | BottomSide | LeftSide, \
    AllSide, Void, \
    TopSide | RightSide | TopRightCornerValue, BottomSide | RightSide | BottomRightCornerValue, TopSide | LeftSide | TopLeftCornerValue, BottomSide | LeftSide | BottomLeftCornerValue, \
    TopSide | RightSide | BottomSide | BottomRightCornerValue, TopSide | RightSide | BottomSide | TopRightCornerValue, TopSide | RightSide | BottomSide | BottomRightCornerValue | TopRightCornerValue, \
    TopSide | RightSide | LeftSide | TopRightCornerValue, TopSide | RightSide | LeftSide | TopLeftCornerValue, TopSide | RightSide | LeftSide | TopRightCornerValue | TopLeftCornerValue, \
    TopSide | BottomSide | LeftSide | TopLeftCornerValue, TopSide | BottomSide | LeftSide | BottomLeftCornerValue, TopSide | BottomSide | LeftSide | TopLeftCornerValue | BottomLeftCornerValue, \
    RightSide | BottomSide | LeftSide | BottomLeftCornerValue, RightSide | BottomSide | LeftSide | BottomRightCornerValue, RightSide | BottomSide | LeftSide | BottomLeftCornerValue | BottomRightCornerValue, \
    AllSide | FourCorners, \
    AllSide | TopLeftCornerValue, AllSide | TopRightCornerValue, AllSide | BottomRightCornerValue, AllSide | BottomLeftCornerValue, \
    AllSide | TopLeftCornerValue | TopRightCornerValue, AllSide | BottomRightCornerValue | BottomLeftCornerValue, AllSide | TopRightCornerValue | BottomRightCornerValue, \
    AllSide | TopLeftCornerValue | BottomLeftCornerValue, AllSide | TopLeftCornerValue | BottomRightCornerValue, AllSide | TopRightCornerValue | BottomLeftCornerValue, \
    AllSide | TopLeftCornerValue | TopRightCornerValue | BottomLeftCornerValue, AllSide | TopLeftCornerValue | TopRightCornerValue | BottomRightCornerValue, \
    AllSide | TopLeftCornerValue | BottomRightCornerValue | BottomLeftCornerValue, AllSide | TopRightCornerValue | BottomRightCornerValue | BottomLeftCornerValue

LISTVALUE(MapTilesConnectType, MAPTILES_CONNECT, MAPTILES_CONNECT_STRINGS, MAPTILES_CONNECT_VALUES);

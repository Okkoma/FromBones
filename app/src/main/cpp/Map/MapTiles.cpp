#include <Urho3D/Urho3D.h>

#include <Urho3D/IO/Log.h>
#include <Urho3D/Scene/Node.h>
#include <Urho3D/Urho2D/StaticSprite2D.h>

#include "TerrainAtlas.h"
#include "Map.h"
#include "MapTiles.h"

#define MAPTILES_PROB_FURNITURES 40



const Tile Tile::EMPTY        = Tile(0, TILE_0);
const Tile Tile::INITTILE     = Tile(1, TILE_1X1);
const Tile Tile::DIMPART1X2   = Tile(0, TILE_1X2_PART);
const Tile Tile::DIMPART2X1   = Tile(0, TILE_2X1_PART);
const Tile Tile::DIMPART2X2R  = Tile(0, TILE_2X2_PARTR);
const Tile Tile::DIMPART2X2B  = Tile(0, TILE_2X2_PARTB);
const Tile Tile::DIMPART2X2BR = Tile(0, TILE_2X2_PARTBR);
TilePtr Tile::EMPTYPTR        = (TilePtr)&Tile::EMPTY;
TilePtr Tile::INITTILEPTR     = (TilePtr)&Tile::INITTILE;
TilePtr Tile::DIMPART1X2P     = (TilePtr)&Tile::DIMPART1X2;
TilePtr Tile::DIMPART2X1P     = (TilePtr)&Tile::DIMPART2X1;
TilePtr Tile::DIMPART2X2RP    = (TilePtr)&Tile::DIMPART2X2R;
TilePtr Tile::DIMPART2X2BP    = (TilePtr)&Tile::DIMPART2X2B;
TilePtr Tile::DIMPART2X2BRP   = (TilePtr)&Tile::DIMPART2X2BR;


const int Tile::DimensionNghIndex[11] = { 8, 1, 7, 7, 1, 0, 8, 8, 8, 8, 8 };

bool Tile::AddSprite(Sprite2D* sprite)
{
    SharedPtr<Sprite2D> s = SharedPtr<Sprite2D>(sprite);

    if (!sprites_.Contains(s))
    {
//        URHO3D_LOGINFOF("=> Sprite Added");
        sprites_.Push(s);
        return true;
    }

    return false;
}

int Tile::GetMaxDimension(ConnectIndex index)
{
    return GetMaxDimension(MapTilesConnectType::GetValue(index));
}

int Tile::GetMaxDimension(int value)
{
    if (value == 0 || value == 16) return TILE_0;
    if ((value & BottomRightCorner) == 0)
    {
        if (value & (BottomSide | RightSide)) return TILE_2X2;
        if (value & BottomSide) return TILE_1X2;
        if (value & RightSide) return TILE_2X1;
    }
    else
    {
        if (value & (BottomSide | RightSide)) return Random(0,100) > 50 ? TILE_2X1 : TILE_1X2;
        if (value & BottomSide) return TILE_1X2;
        if (value & RightSide) return TILE_2X1;
    }
    return TILE_0;
}

template< typename T >
int Tile::GetConnectIndexNghb2(const T* dataMap, unsigned addr, short int nghb1, short int nghb2)
{
    if (dataMap[addr] <= MapFeatureType::InnerSpace) return MapTilesConnectType::NoConnect;

    int connectIndex = MapTilesConnectType::NoConnect;

    if (dataMap[addr+MapInfo::info.neighbor4Ind[nghb1]] > MapFeatureType::InnerSpace) connectIndex = connectIndex | (1 << nghb1);
    if (dataMap[addr+MapInfo::info.neighbor4Ind[nghb2]] > MapFeatureType::InnerSpace) connectIndex = connectIndex | (1 << nghb2);
    return connectIndex;
}

template< typename T >
int Tile::GetConnectIndexNghb3(const T* dataMap, unsigned addr, short int nghb1, short int nghb2, short int nghb3)
{
    if (dataMap[addr] <= MapFeatureType::InnerSpace) return MapTilesConnectType::NoConnect;

    int connectIndex = MapTilesConnectType::NoConnect;

    if (dataMap[addr+MapInfo::info.neighbor4Ind[nghb1]] > MapFeatureType::InnerSpace) connectIndex = connectIndex | (1 << nghb1);
    if (dataMap[addr+MapInfo::info.neighbor4Ind[nghb2]] > MapFeatureType::InnerSpace) connectIndex = connectIndex | (1 << nghb2);
    if (dataMap[addr+MapInfo::info.neighbor4Ind[nghb3]] > MapFeatureType::InnerSpace) connectIndex = connectIndex | (1 << nghb3);
    return connectIndex;
}

template< typename T >
int Tile::GetConnectIndexNghb4(const T* dataMap, unsigned addr)
{
    if (dataMap[addr] <= MapFeatureType::InnerSpace) return MapTilesConnectType::NoConnect;

    int connectIndex = MapTilesConnectType::NoConnect;

    if (dataMap[addr+MapInfo::info.neighbor4Ind[TopBit]] > MapFeatureType::InnerSpace) connectIndex = connectIndex | TopSide;
    if (dataMap[addr+MapInfo::info.neighbor4Ind[RightBit]] > MapFeatureType::InnerSpace) connectIndex = connectIndex | RightSide;
    if (dataMap[addr+MapInfo::info.neighbor4Ind[BottomBit]] > MapFeatureType::InnerSpace) connectIndex = connectIndex | BottomSide;
    if (dataMap[addr+MapInfo::info.neighbor4Ind[LeftBit]] > MapFeatureType::InnerSpace) connectIndex = connectIndex | LeftSide;
    return connectIndex;
}

template< typename T >
int Tile::GetConnectIndexNghb2(short int* nghTable, const T* dataMap, unsigned addr, short int nghb1, short int nghb2)
{
    if (dataMap[addr] <= MapFeatureType::InnerSpace) return MapTilesConnectType::NoConnect;

    int connectIndex = MapTilesConnectType::NoConnect;

    if (dataMap[addr+nghTable[nghb1]] > MapFeatureType::InnerSpace) connectIndex = connectIndex | (1 << nghb1);
    if (dataMap[addr+nghTable[nghb2]] > MapFeatureType::InnerSpace) connectIndex = connectIndex | (1 << nghb2);

    return connectIndex;
}

template< typename T >
int Tile::GetConnectIndexNghb3(short int* nghTable, const T* dataMap, unsigned addr, short int nghb1, short int nghb2, short int nghb3)
{
    if (dataMap[addr] <= MapFeatureType::InnerSpace) return MapTilesConnectType::NoConnect;

    int connectIndex = MapTilesConnectType::NoConnect;

    if (dataMap[addr+nghTable[nghb1]] > MapFeatureType::InnerSpace) connectIndex = connectIndex | (1 << nghb1);
    if (dataMap[addr+nghTable[nghb2]] > MapFeatureType::InnerSpace) connectIndex = connectIndex | (1 << nghb2);
    if (dataMap[addr+nghTable[nghb3]] > MapFeatureType::InnerSpace) connectIndex = connectIndex | (1 << nghb3);

    return connectIndex;
}

template< typename T >
int Tile::GetConnectIndexNghb4(short int* nghTable, const T* dataMap, unsigned addr)
{
    if (dataMap[addr] <= MapFeatureType::InnerSpace) return MapTilesConnectType::NoConnect;

    int connectIndex = MapTilesConnectType::NoConnect;

    if (dataMap[addr+nghTable[TopBit]] > MapFeatureType::InnerSpace) connectIndex = connectIndex | TopSide;
    if (dataMap[addr+nghTable[RightBit]] > MapFeatureType::InnerSpace) connectIndex = connectIndex | RightSide;
    if (dataMap[addr+nghTable[BottomBit]] > MapFeatureType::InnerSpace) connectIndex = connectIndex | BottomSide;
    if (dataMap[addr+nghTable[LeftBit]] > MapFeatureType::InnerSpace) connectIndex = connectIndex | LeftSide;

    return connectIndex;
}

static bool cornerCheck1, cornerCheck2;

template< typename T >
void Tile::GetCornerIndex(const T* dataMap, unsigned addr, ConnectIndex& index)
{
    GetCornerIndex(MapInfo::info.cornerInd, dataMap, addr, index);
}

template< typename T >
void Tile::GetCornerIndex(short int* nghCornerTable, const T* dataMap, unsigned addr, ConnectIndex& index)
{
    /// Get Corner Value for Connected Tile
    switch (index)
    {
    /// 2-Connected cases
    case MapTilesConnectType::TopRightConnect :
        if (dataMap[addr+nghCornerTable[TopRightBit]] <= MapFeatureType::InnerSpace)
            index = MapTilesConnectType::TopRightConnect_Corner;
        break;
    case MapTilesConnectType::BottomRightConnect :
        if (dataMap[addr+nghCornerTable[BottomRightBit]] <= MapFeatureType::InnerSpace)
            index = MapTilesConnectType::BottomRightConnect_Corner;
        break;
    case MapTilesConnectType::TopLeftConnect :
        if (dataMap[addr+nghCornerTable[TopLeftBit]] <= MapFeatureType::InnerSpace)
            index = MapTilesConnectType::TopLeftConnect_Corner;
        break;
    case MapTilesConnectType::BottomLeftConnect :
        if (dataMap[addr+nghCornerTable[BottomLeftBit]] <= MapFeatureType::InnerSpace)
            index = MapTilesConnectType::BottomLeftConnect_Corner;
        break;
    /// 3-Connected cases
    //Left Border Free
    case MapTilesConnectType::TopRightBottomConnect :
        cornerCheck1 = dataMap[addr+nghCornerTable[BottomRightBit]] <= MapFeatureType::InnerSpace;
        cornerCheck2 = dataMap[addr+nghCornerTable[TopRightBit]] <= MapFeatureType::InnerSpace;
        if (cornerCheck1 || cornerCheck2)
        {
            if (cornerCheck1 && cornerCheck2)
                index = MapTilesConnectType::TopRightBottomConnect_CornerTopBottom;
            else
                index = cornerCheck1 ? MapTilesConnectType::TopRightBottomConnect_CornerBottom :
                        MapTilesConnectType::TopRightBottomConnect_CornerTop;
        }
        break;
    // Border Bottom Free
    case MapTilesConnectType::TopRightLeftConnect :
        cornerCheck1 = dataMap[addr+nghCornerTable[TopRightBit]] <= MapFeatureType::InnerSpace;
        cornerCheck2 = dataMap[addr+nghCornerTable[TopLeftBit]] <= MapFeatureType::InnerSpace;
        if (cornerCheck1 || cornerCheck2)
        {
            if (cornerCheck1 && cornerCheck2)
                index = MapTilesConnectType::TopRightLeftConnect_CornerRightLeft;
            else
                index = cornerCheck1 ? MapTilesConnectType::TopRightLeftConnect_CornerRight :
                        MapTilesConnectType::TopRightLeftConnect_CornerLeft;
        }
        break;
    // Right Border Free
    case MapTilesConnectType::TopBottomLeftConnect :
        cornerCheck1 = dataMap[addr+nghCornerTable[TopLeftBit]] <= MapFeatureType::InnerSpace;
        cornerCheck2 = dataMap[addr+nghCornerTable[BottomLeftBit]] <= MapFeatureType::InnerSpace;
        if (cornerCheck1 || cornerCheck2)
        {
            if (cornerCheck1 && cornerCheck2)
                index = MapTilesConnectType::TopBottomLeftConnect_CornerTopBottom;
            else
                index = cornerCheck1 ? MapTilesConnectType::TopBottomLeftConnect_CornerTop :
                        MapTilesConnectType::TopBottomLeftConnect_CornerBottom;
        }
        break;
    // Top Border Free
    case MapTilesConnectType::RightBottomLeftConnect :
        cornerCheck1 = dataMap[addr+nghCornerTable[BottomLeftBit]] <= MapFeatureType::InnerSpace;
        cornerCheck2 = dataMap[addr+nghCornerTable[BottomRightBit]] <= MapFeatureType::InnerSpace;
        if (cornerCheck1 || cornerCheck2)
        {
            if (cornerCheck1 && cornerCheck2)
                index = MapTilesConnectType::RightBottomLeftConnect_CornerRightLeft;
            else
                index = cornerCheck1 ? MapTilesConnectType::RightBottomLeftConnect_CornerLeft :
                        MapTilesConnectType::RightBottomLeftConnect_CornerRight;
        }
        break;
    /// 4-Connected cases = No free Border
    case MapTilesConnectType::AllConnect :
        int cornerValue = NoCorner;
        int numCorners = 0;
        if (dataMap[addr+nghCornerTable[TopLeftBit]] <= MapFeatureType::InnerSpace)
        {
            cornerValue = cornerValue | TopLeftCorner;
            numCorners++;
        }
        if (dataMap[addr+nghCornerTable[TopRightBit]] <= MapFeatureType::InnerSpace)
        {
            cornerValue = cornerValue | TopRightCorner;
            numCorners++;
        }
        if (dataMap[addr+nghCornerTable[BottomRightBit]] <= MapFeatureType::InnerSpace)
        {
            cornerValue = cornerValue | BottomRightCorner;
            numCorners++;
        }
        if (dataMap[addr+nghCornerTable[BottomLeftBit]] <= MapFeatureType::InnerSpace)
        {
            cornerValue = cornerValue | BottomLeftCorner;
            numCorners++;
        }

        if (!numCorners)
            break;

        if (numCorners == 4)
            index = MapTilesConnectType::AllConnect_C4;

        else if (numCorners == 1)
        {
            // 1 Corner
            if (cornerValue == TopLeftCorner)
                index = MapTilesConnectType::AllConnect_C1_TopLeft;
            else if (cornerValue == TopRightCorner)
                index = MapTilesConnectType::AllConnect_C1_TopRight;
            else if (cornerValue == BottomRightCorner)
                index = MapTilesConnectType::AllConnect_C1_BottomRight;
            else
                index = MapTilesConnectType::AllConnect_C1_BottomLeft;
        }
        else if (numCorners == 2)
        {
            // 2 Corners
            if (cornerValue == (TopLeftCorner | TopRightCorner))
                index = MapTilesConnectType::AllConnect_C2_TopLeftRight;
            else if (cornerValue == (BottomRightCorner | BottomLeftCorner))
                index = MapTilesConnectType::AllConnect_C2_BottomRightLeft;
            else if (cornerValue == (TopRightCorner | BottomRightCorner))
                index = MapTilesConnectType::AllConnect_C2_TopBottomRight;
            else if (cornerValue == (TopLeftCorner | BottomLeftCorner))
                index = MapTilesConnectType::AllConnect_C2_TopBottomLeft;
            else if (cornerValue == (TopLeftCorner | BottomRightCorner))
                index = MapTilesConnectType::AllConnect_C2_TopLeft_BottomRight;
            else
                index = MapTilesConnectType::AllConnect_C2_TopRight_BottomLeft;
        }
        else if (numCorners == 3)
        {
            // 3 Corners
            if (cornerValue == (TopLeftCorner | TopRightCorner | BottomLeftCorner))
                index = MapTilesConnectType::AllConnect_C3_NoBottomRightCorner;
            else if (cornerValue == (TopLeftCorner | TopRightCorner | BottomRightCorner))
                index = MapTilesConnectType::AllConnect_C3_NoBottomLeftCorner;
            else if (cornerValue == (BottomLeftCorner | BottomRightCorner | TopLeftCorner))
                index = MapTilesConnectType::AllConnect_C3_NoTopRightCorner;
            else
                index = MapTilesConnectType::AllConnect_C3_NoTopLeftCorner;
        }

        break;
    }
}

template< typename T >
int Tile::GetMaxDimensionRight(short int* nghTable, const T* dataMap, const TilePtr* render, unsigned addr)
{
    if (dataMap[addr] <= MapFeatureType::InnerSpace) return TILE_0;

    return dataMap[addr+nghTable[RightBit]] > MapFeatureType::NoRender &&
           render[addr]->GetTerrain() == render[addr+nghTable[RightBit]]->GetTerrain() &&
           render[addr+nghTable[RightBit]]->GetDimensions() > TILE_RENDER ? TILE_2X1 : TILE_1X1;
}

template< typename T >
int Tile::GetMaxDimensionBottom(short int* nghTable, const T* dataMap, const TilePtr* render, unsigned addr)
{
    if (dataMap[addr] <= MapFeatureType::InnerSpace) return TILE_0;

    return dataMap[addr+nghTable[BottomBit]] > MapFeatureType::NoRender &&
           render[addr]->GetTerrain() == render[addr+nghTable[BottomBit]]->GetTerrain() &&
           render[addr+nghTable[BottomBit]]->GetDimensions() > TILE_RENDER ? TILE_1X2 : TILE_1X1;
}

template< typename T >
int Tile::GetMaxDimensionBottomRight(short int* nghTable, const T* dataMap, const TilePtr* render, unsigned addr)
{
    if (dataMap[addr] <= MapFeatureType::InnerSpace) return TILE_0;

    int count = TILE_0;
    unsigned char terrain = render[addr]->GetTerrain();

    unsigned nghaddr = addr+nghTable[BottomBit];
    if (render[nghaddr]->GetDimensions() > TILE_RENDER)
        if (dataMap[nghaddr] > MapFeatureType::NoRender && terrain == render[nghaddr]->GetTerrain())
            count += TILE_1X2;

    nghaddr = addr+nghTable[RightBit];
    if (render[nghaddr]->GetDimensions() > TILE_RENDER)
        if (dataMap[nghaddr] > MapFeatureType::NoRender && terrain == render[nghaddr]->GetTerrain())
            count += TILE_2X1;

    nghaddr = addr+nghTable[BottomRightCornerBit];
    if (render[nghaddr]->GetDimensions() > TILE_RENDER)
        if (dataMap[nghaddr] > MapFeatureType::NoRender && terrain == render[nghaddr]->GetTerrain())
            count += TILE_2X2;

    if (count == 0 || count == TILE_2X2) // 0 or 10
        return TILE_1X1;

    if (count == TILE_1X2 + TILE_2X1 + TILE_2X2) // 8+9+10=27
        return TILE_2X2;

    if (count == TILE_1X2 + TILE_2X1) // 8+9=17
        return Random(0,100) > 50 ? TILE_2X1 : TILE_1X2;

    if (count == TILE_1X2 || count == TILE_1X2 + TILE_2X2) // 8 or 18
        return TILE_1X2;

    if (count == TILE_2X1 || count == TILE_2X1 + TILE_2X2) // 9 or 19
        return TILE_2X1;

    return TILE_0;
}

template int Tile::GetConnectIndexNghb2(const FeatureType* dataMap, unsigned addr, short int nghb1, short int nghb2);
template int Tile::GetConnectIndexNghb3(const FeatureType* dataMap, unsigned addr, short int nghb1, short int nghb2, short int nghb3);
template int Tile::GetConnectIndexNghb4(const FeatureType* dataMap, unsigned addr);
template int Tile::GetConnectIndexNghb2(const int* dataMap, unsigned addr, short int nghb1, short int nghb2);
template int Tile::GetConnectIndexNghb3(const int* dataMap, unsigned addr, short int nghb1, short int nghb2, const short int nghb3);
template int Tile::GetConnectIndexNghb4(const int* dataMap, unsigned addr);
template void Tile::GetCornerIndex(const FeatureType* dataMap, unsigned addr, ConnectIndex& index);
template void Tile::GetCornerIndex(const int* dataMap, unsigned addr, ConnectIndex& index);

template int Tile::GetConnectIndexNghb2(short int* nghTable, const FeatureType* dataMap, unsigned addr, short int nghb1, short int nghb2);
template int Tile::GetConnectIndexNghb3(short int* nghTable, const FeatureType* dataMap, unsigned addr, short int nghb1, short int nghb2, short int nghb3);
template int Tile::GetConnectIndexNghb4(short int* nghTable, const FeatureType* dataMap, unsigned addr);
template int Tile::GetConnectIndexNghb2(short int* nghTable, const int* dataMap, unsigned addr, short int nghb1, short int nghb2);
template int Tile::GetConnectIndexNghb3(short int* nghTable, const int* dataMap, unsigned addr, short int nghb1, short int nghb2, short int nghb3);
template int Tile::GetConnectIndexNghb4(short int* nghTable, const int* dataMap, unsigned addr);
template void Tile::GetCornerIndex(short int* nghTable, const FeatureType* dataMap, unsigned addr, ConnectIndex& index);
template void Tile::GetCornerIndex(short int* nghTable, const int* dataMap, unsigned addr, ConnectIndex& index);

template int Tile::GetMaxDimensionRight(short int* nghTable, const FeatureType* dataMap, const TilePtr* render, unsigned addr);
template int Tile::GetMaxDimensionBottom(short int* nghTable, const FeatureType* dataMap, const TilePtr* render, unsigned addr);
template int Tile::GetMaxDimensionBottomRight(short int* nghTable, const FeatureType* dataMap, const TilePtr* render, unsigned addr);
template int Tile::GetMaxDimensionRight(short int* nghTable, const int* dataMap, const TilePtr* render, unsigned addr);
template int Tile::GetMaxDimensionBottom(short int* nghTable, const int* dataMap, const TilePtr* render, unsigned addr);
template int Tile::GetMaxDimensionBottomRight(short int* nghTable, const int* dataMap, const TilePtr* render, unsigned addr);

//void Tile::SetTile(Tile* tile, sTile*& stile, bool furnish)
//{
//    if (tile)
//    {
//        if (tile->GetType() == TILE_SIMPLE)
//        {
//            if (stile)
//                if (stile->GetType() != TILE_SIMPLE)
//                {
//                    delete stile;
//                    stile = 0;
//                }
//                else
//                {
//                    if (&stile->GetTile() != tile)
//                    {
//                        stile->SetTile(tile);
//                        stile->SetRandomSpriteID();
//                    }
//                }
//            if (!stile)
//                stile = new sTile(tile, true);
//        }
//        else if (tile->GetType() == TILE_ANIMATED)
//        {
//            if (stile)
//                if (stile->GetType() != TILE_ANIMATED)
//                {
//                    delete stile;
//                    stile = 0;
//                }
//            if (!stile)
//                stile = new sAnimatedTile((AnimatedTile*) tile);
////            URHO3D_LOGINFOF("SetTile addr=%u, tileType=%d",addr,tiles_[addr]->GetType());
//        }
//        else
//        {
//            if (stile)
//                delete stile;
//            stile = 0;
//        }
//
//        if (furnish && stile)
//            if (Random(100) > MAPTILES_PROB_FURNITURES)
//                stile->SetRandomFurniture();
//    }
//    else
//    {
//        if (stile)
//            delete stile;
//        stile = 0;
//    }
//}
//
//void Tile::SetConnectedTile(sTile*& stile, int feature, int connectValue)
//{
//    if (feature == 0)
//    {
//        if (stile)
//            delete stile;
//        stile = 0;
//        return;
//    }
//
////    int terrain = 1;
//    int terrain = atlas_->GetRandomTerrainForFeature(feature);
//    if (terrain < 0)
//    {
//        if (stile)
//            delete stile;
//        stile = 0;
//        return;
//    }
//    Tile::SetTile(atlas_->GetTile(terrain, connectValue), stile);
//}


TerrainAtlas* sTile::atlas_ = 0;

sTile::sTile(Tile* tile, bool randSprite) :
    tile_(tile),
    node_(0),
    furniture_(-1)
{
    if (randSprite)
        SetRandomSpriteID();
    else
        spriteId_ = 0;
}

void sTile::SetRandomFurniture()
{
    furniture_ = atlas_->GetTerrain(tile_->GetTerrain()).GetRandomFurniture(tile_->GetConnectivity());
//    URHO3D_LOGINFOF("sTile() - SetRandomFurniture : furniture=%d connect=%u", furniture_, tile_->GetConnectivity());
}

void sTile::SetRandomSpriteID()
{
    spriteId_ = Random(tile_->GetNumSprite());
}

void sAnimatedTile::Update()
{
    if (!IsPlaying()) return;

    currentDelay_++;
//    URHO3D_LOGINFOF("sAnimatedTile() - Update");
    if (!(currentDelay_ % tile_->tickDelay_))
    {
        GetNode()->GetComponent<StaticSprite2D>()->SetSprite(atlas_->GetTileSprite(currentGid_));
        currentGid_ = currentGid_ % tile_->toGid_ ? ++currentGid_ : tile_->GetGid();
//        URHO3D_LOGINFOF("sAnimatedTile() - Update : gid=%d => currentGid_= %d", tile_->GetGid(), currentGid_);
    }
}

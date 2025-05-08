#include <Urho3D/Urho3D.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/Container/Sort.h>

#include "DefsViews.h"

#include "DefsWorld.h"

#include "GameOptionsTest.h"


/// Ellipse

void EllipseW::GenerateShape(int numpoints)
{
    // numpoints must be a multiple of 4. (4 quarters)
    numpoints = (numpoints/4) * 4;

    ellipseShape_.Resize(numpoints);

    for (int i = 0; i < numpoints; ++i)
    {
        const float angle = (float)i / numpoints * 360.0f;
        const float x = center_.x_ + radius_.x_ * Cos(angle);
        const float y = center_.y_ + radius_.y_ * Sin(angle);
        ellipseShape_[i] = Vector2(x, y);
    }

    // pre-calculate the deltas and angles for the first quarter only
    numpoints /= 4;

    ellipseXValues_.Resize(numpoints);
    for (int i = 0; i < numpoints; ++i)
        ellipseXValues_[i] = ellipseShape_[i].x_;

    ellipseAngles_.Resize(numpoints);

    // Tangent Angle -90°
    ellipseAngles_.Front().angle_ = Vector2(0.f, -1.f);
    ellipseAngles_.Front().pente_ = 0.f;
    // Tangent Angle 0°
    ellipseAngles_.Back().angle_ = Vector2(1.f, 0.f);
    ellipseAngles_.Back().pente_ = 0.f;
    for (int i = 1; i < numpoints-1; ++i)
    {
        const Vector2& p1 = ellipseShape_[i];
        const Vector2& p2 = ellipseShape_[i-1];
        const float dx = p2.x_ - p1.x_;
        const float dy = p2.y_ - p1.y_;
        const float length = sqrtf(dx * dx + dy * dy);
        ellipseAngles_[i].angle_ = Vector2(dx / length, dy / length);
        ellipseAngles_[i].pente_ = dy/dx;
    }
}

float EllipseW::GetX(float y) const
{
    y -= center_.y_;

    // Clamp Y
    if (y >= radius_.y_)
        return center_.x_;
    else if (y <= -radius_.y_)
        return center_.x_;

    return center_.x_ + (radius_.x_/radius_.y_) * sqrtf(radius_.y_*radius_.y_ - y*y);
}

float EllipseW::GetY(float x) const
{
    x -= center_.x_;

    // Clamp X
    if (x >= radius_.x_)
        return center_.y_;
    else if (x <= -radius_.x_)
        return center_.y_;

    return center_.y_ + (radius_.y_/radius_.x_) * sqrtf(radius_.x_*radius_.x_ - x*x);
}

void EllipseW::GetPointsInRectAtX(float x, const Rect& rect, Vector<Vector2>& points) const
{
    // the x is inside the bounds of the ellipse in x ?
    if (x >= aabb_.min_.x_ && x <= aabb_.max_.x_)
    {
        // intersection with ellipse and the vertical line x
        const float delta = GetDeltaY(x);

        // solution y1 = center.y_ + deltay
        float y = center_.y_ + delta;
        if (y >= rect.min_.y_ && y <= rect.max_.y_)
            points.Push(Vector2(x, y));

        if (delta)
        {
            // solution y2 = center.y_ - deltay
            y = center_.y_ - delta;
            if (y >= rect.min_.y_ && y <= rect.max_.y_)
                points.Push(Vector2(x, y));
        }
    }
}

void EllipseW::GetPointsInRectAtY(float y, const Rect& rect, Vector<Vector2>& points) const
{
    // the x is inside the bounds of the ellipse in x ?
    if (y >= aabb_.min_.y_ && y <= aabb_.max_.y_)
    {
        // intersection with ellipse and the horizontal line y
        const float delta = GetDeltaX(y);

        // solution x1 = center.x_ + deltax
        float x = center_.x_ + delta;
        if (x >= rect.min_.x_ && x <= rect.max_.x_)
            points.Push(Vector2(x, y));

        if (delta)
        {
            // solution x2 = center.x_ - deltax
            x = center_.x_ - delta;
            if (x >= rect.min_.x_ && x <= rect.max_.x_)
                points.Push(Vector2(x, y));
        }
    }
}

static inline bool CompareVector2onX(const Vector2& vl, const Vector2& vr)
{
    return vl.x_ < vr.x_;
}

// Get the intersections between the ellipse and the rectangle
void EllipseW::GetIntersections(const Rect& rect, Vector<Vector2>& points, bool sortX) const
{
    points.Clear();

    // rectangle is outside the ellipse
    if (rect.max_.x_ < aabb_.min_.x_ || rect.min_.x_ > aabb_.max_.x_ || rect.max_.y_ < aabb_.min_.y_ || rect.min_.y_ > aabb_.max_.y_)
        return;

    GetPointsInRectAtX(rect.min_.x_, rect, points);
    GetPointsInRectAtX(rect.max_.x_, rect, points);
    GetPointsInRectAtY(rect.min_.y_, rect, points);
    GetPointsInRectAtY(rect.max_.y_, rect, points);

    if (points.Size() > 1 && sortX)
        Sort(points.Begin(), points.End(), CompareVector2onX);
}

void EllipseW::GetPositionOn(float xi, float sidey, Vector2& point)
{
    float x = xi + point.x_;

    if (x == center_.x_)
    {
        point.y_ = center_.y_ + sidey * radius_.y_;
    }
    else
    {
        const float x_normalized = (x - center_.x_) / radius_.x_;
        point.y_ = center_.y_ + sidey * radius_.y_ * sqrtf(1.f - x_normalized * x_normalized);
    }
}

void EllipseW::GetPositionOnShape(float xi, float sidey, Vector2& point)
{
    // Return the y on the ellipse for a given x.

    float x = xi + point.x_;

    const bool xmirror = x < center_.x_;
    if (xmirror)
        x = center_.x_ + fabsf(center_.x_ - x);

    if (x == center_.x_)
    {
        point.y_ = center_.y_ + radius_.y_;
    }
    else if (x >= center_.x_ + radius_.x_)
    {
        // clamp X on ellipse
        point.x_ = xmirror ? center_.x_ - radius_.x_ : center_.x_ + radius_.x_;
        point.y_ = center_.y_;
    }
    else
    {
        // use pre-calculated ellipse points from ellipseShape_.
        // and only the first quarter of the ellipse (angle=0° to 90°) for optimization.
        // find by dichotomy the 2 points ellipseShape_[ifirst] and ellipseShape_[ilast] framing the value x
        // ellipseShape_[0] is the point of the ellipse at angle=0°.
        // ellipseShape_[ellipseShape_.Size()/4] is the point of the ellipse at angle=90°.
        int ilast = ellipseShape_.Size()/4;
        int i, ifirst = 0;
        for (;;)
        {
            i = (ifirst + ilast) / 2;

            if (x > ellipseShape_[i].x_)
            {
                if (ilast > ifirst + 1)
                    // reduce the range to the first indexes
                    ilast = i;
                else
                    // indexes found !
                    break;
            }
            else
            {
                if (ilast > ifirst + 1)
                    // reduce the range to the last indexes
                    ifirst = i;
                else
                    // indexes found !
                    break;
            }
        }

        // calculate the segment (p2-p1) which is the approximation of the ellipse on
        // reminder for the first quarter : p1.x_ < x < p2.x_ and p1.y_> y > p2.y_
        const Vector2& p1 = ellipseShape_[ilast];
        const Vector2& p2 = ellipseShape_[ifirst];
        const float dx = p2.x_ - p1.x_;
        const float dy = xmirror ? p1.y_ - p2.y_ : p2.y_ - p1.y_;

        // calculate the Y with the equation of the line y = a * x + b
        // pente a=dy/dx is always negative on the first quarter of the ellipse
        point.y_ = -fabsf(dy/dx) * (x - p1.x_) + p1.y_;
    }

    if (sidey < 0.f)
        point.y_ = center_.y_ - Abs(center_.y_ - point.y_);
}

void EllipseW::GetPositionOn(float xi, float sidey, Vector2& point, Vector2& angleinfo)
{
    float x = xi + point.x_;

    //sidey = point.y_ >= center_.y_ ? 1.f : -1.f;

    if (x == center_.x_)
    {
        // Angle 0°
        angleinfo.x_ = sidey;
        angleinfo.y_ = 0.f;
        point.y_ = center_.y_ + sidey * radius_.y_;
    }
    else
    {
        if (x <= center_.x_ - radius_.x_)
        {
            float dx = Abs(center_.x_ - radius_.x_ - x);
            x = center_.x_ - radius_.x_ + dx;
            point.x_ = x - xi;
        }
        else if (x >= center_.x_ + radius_.x_)
        {
            float dx = Abs(x - center_.x_ - radius_.x_);
            x = center_.x_ + radius_.x_ - dx;
            point.x_ = x - xi;
        }

        const float x_normalized = (x - center_.x_) / radius_.x_;
        const float dy = sidey * radius_.y_ * sqrtf(1.f - x_normalized * x_normalized);

        // Calculate the slope of the tangent to the ellipse at the given point
        const float slope = (center_.x_ - x) * radius_.y_ * radius_.y_ / (dy * radius_.x_ * radius_.x_);
        const float inversmagnitude = 1.f / sqrtf(slope * slope + 1.f);
        // Calculate the cosine and sine of the angle of the tangent
        angleinfo.x_ = sidey * inversmagnitude;
        angleinfo.y_ = dy ? sidey * slope * inversmagnitude : (x - center_.x_ > 0.f ? -1.f : 1.f);

        point.y_ = center_.y_ + dy;
    }
}

void EllipseW::GetPositionOnShape(float xi, float sidey, Vector2& point, Vector2& angleinfo)
{
    // Return the y on the upper half of the ellipse for a given x.
    // Return the angle of the tangente at this point(x,y) into angleinfo.x_=cos and angleinfo.sin_=sin

    float x = xi + point.x_;

    const bool xmirror = x < center_.x_;
    if (xmirror)
        x = center_.x_ + fabsf(center_.x_ - x);

    if (x == center_.x_)
    {
        // Angle 0°
        angleinfo.x_ = 1.f;
        angleinfo.y_ = 0.f;
        point.y_ = center_.y_ + radius_.y_;
    }
    else if (x >= center_.x_ + radius_.x_)
    {
        // Angle -90°/90°
        angleinfo.x_ = 0.f;
        angleinfo.y_ = xmirror ? 1.f : -1.f;
        // clamp X on ellipse
        point.x_ = xmirror ? center_.x_ - radius_.x_ : center_.x_ + radius_.x_;
        point.y_ = center_.y_;
    }
    else
    {
        // use pre-calculated ellipse points from ellipseShape_.
        // and only the first quarter of the ellipse (angle=0° to 90°) for optimization.
        // find by dichotomy the nearest point with xvalue just inferior to x.
        int ilast = Min(LowerBoundInverse(ellipseXValues_.Begin(), ellipseXValues_.End(), x) - ellipseXValues_.Begin(), ellipseXValues_.Size()-1);

        // get the last point
        const Vector2& p1 = ellipseShape_[ilast];
        // get the precalculated cos and sin and pente
        const AngleInfo& info = ellipseAngles_[ilast];

        angleinfo = info.angle_;
        if (xmirror)
            angleinfo.y_ = -angleinfo.y_;

        // calculate the Y with the equation of the line y = pente * x + b
        point.y_ = info.pente_ * (x - p1.x_) + p1.y_;
    }

    if (sidey < 0.f)
    {
        angleinfo.x_ = -angleinfo.x_;
        point.y_ = center_.y_ - Abs(center_.y_ - point.y_);
    }
}


void EllipseW::DrawDebugGeometry(DebugRenderer* debug, const Color& color, bool depthTest)
{
    unsigned uintColor = color.ToUInt();
    for (unsigned i=0; i < ellipseShape_.Size()-1; ++i)
        debug->AddLine(ellipseShape_[i], ellipseShape_[i+1], uintColor, false);
}


/// GeneratorParams

String GeneratorParams::ToString() const
{
    String str;
    if (params_.Size())
    {
        for (unsigned i=0; i<params_.Size(); ++i)
            str += Urho3D::ToString("p[%d]=%d ", i, params_[i]);
    }

    if (!params_.Size())
        str = "No Params !";

    return str;
}


/// World2DInfo

const String World2DInfo::ATLASSETDIR = DATALEVELSDIR;
const String World2DInfo::ATLASSETDEFAULT = "atlas_default.xml";
const String World2DInfo::WORLDMODELDEFAULT = "anlworld_default.xml";
SharedPtr<Material> World2DInfo::WATERMATERIAL_TILE;
SharedPtr<Material> World2DInfo::WATERMATERIAL_REFRACT;
SharedPtr<Material> World2DInfo::WATERMATERIAL_LINE;

long long World2DInfo::delayUpdateUsec_ = MAP_MAXDELAY;
WeakPtr<TerrainAtlas> World2DInfo::currentAtlas_;

World2DInfo::World2DInfo(const World2DInfo& winfo) :
    mapWidth_(winfo.mapWidth_),
    mapHeight_(winfo.mapHeight_),
    tileWidth_(winfo.tileWidth_),
    tileHeight_(winfo.tileHeight_),
    mWidth_(winfo.mWidth_),
    mHeight_(winfo.mHeight_),
    mTileWidth_(winfo.mTileWidth_),
    mTileHeight_(winfo.mTileHeight_),
    mTileRect_(winfo.mTileRect_),
    simpleGroundLevel_(winfo.simpleGroundLevel_),
    isScaled_(winfo.isScaled_),
    worldScale_(winfo.worldScale_),
    forcedShapeType_(winfo.forcedShapeType_),
    shapeType_(winfo.shapeType_),
    addObject_(winfo.addObject_),
    addFurniture_(winfo.addFurniture_),
    storageDir_(winfo.storageDir_),
    atlasSetFile_(winfo.atlasSetFile_),
    worldModelFile_(winfo.worldModelFile_),
    defaultGenerator_(winfo.defaultGenerator_),
    wBounds_(winfo.wBounds_),
    node_(winfo.node_),
    backgroundDrawableObjects_(winfo.backgroundDrawableObjects_),
    genParams_(winfo.genParams_),
    atlas_(winfo.atlas_),
    worldModel_(winfo.worldModel_)
{
    ;
}

World2DInfo& World2DInfo::operator=(const World2DInfo& winfo)
{
    mapWidth_ = winfo.mapWidth_;
    mapHeight_ = winfo.mapHeight_;
    tileWidth_ = winfo.tileWidth_;
    tileHeight_ = winfo.tileHeight_;
    mWidth_ = winfo.mWidth_;
    mHeight_ = winfo.mHeight_;
    mTileWidth_ = winfo.mTileWidth_;
    mTileHeight_ = winfo.mTileHeight_;
    mTileRect_ = winfo.mTileRect_;
    simpleGroundLevel_ = winfo.simpleGroundLevel_;
    isScaled_ = winfo.isScaled_;
    worldScale_ = winfo.worldScale_;
    forcedShapeType_ = winfo.forcedShapeType_;
    shapeType_ = winfo.shapeType_;
    addObject_ = winfo.addObject_;
    addFurniture_ = winfo.addFurniture_;
    storageDir_ = winfo.storageDir_;
    atlasSetFile_ = winfo.atlasSetFile_;
    worldModelFile_ = winfo.worldModelFile_;
    defaultGenerator_ = winfo.defaultGenerator_;
    wBounds_ = winfo.wBounds_;
    node_ = winfo.node_;
    backgroundDrawableObjects_ = winfo.backgroundDrawableObjects_;
    genParams_ = winfo.genParams_;
    atlas_ = winfo.atlas_;
    worldModel_ = winfo.worldModel_;
    return *this;
}

void World2DInfo::SetMapSize(int mapWidth, int mapHeight)
{
    mapWidth_ = mapWidth;
    mapHeight_ = mapHeight;

    mWidth_ = mapWidth_ * mTileWidth_;
    mHeight_ = mapHeight_ * mTileHeight_;
}

void World2DInfo::Update(const Vector3& scale)
{
    worldScale_ = scale.ToVector2();

    // update with scale
    mTileWidth_ = tileWidth_ * scale.x_;
    mTileHeight_ = tileHeight_ * scale.y_;
    mTileRect_ = Rect(0.f, 0.f, mTileWidth_, mTileHeight_);
    mWidth_ = mapWidth_ * mTileWidth_;
    mHeight_ = mapHeight_ * mTileHeight_;
    isScaled_ = (scale.x_ != 1.f && scale.y_ != 1.f);

    if (worldModel_)
    {
        Vector2 wMapSize(mWidth_, mHeight_);
        worldGroundRef_.Define(2.f * wMapSize * worldModel_->GetOffset(), 2.f * wMapSize * worldModel_->GetRadius() / worldModel_->GetScale() - wMapSize);

        // Adjust ellipse if V1
        if (worldModel_->GetAnlVersion() == ANLV1)
        {
            worldGroundRef_.radius_.x_ -= 300.f;
            worldGroundRef_.radius_.y_ -= 50.f;
        }

        worldGroundRef_.GenerateShape(64);
        worldGround_.Define(worldGroundRef_.center_, worldGroundRef_.radius_ - Vector2(5.f*mWidth_, 1.f*mHeight_));

#ifdef ACTIVE_WORLDELLIPSES
        worldAtmosphere_.Define(worldGroundRef_.center_, worldGroundRef_.radius_ + Vector2(4.f*mHeight_, 8.f*mHeight_));
        worldHillTop_.Define(worldGroundRef_.center_, worldGroundRef_.radius_ + Vector2(-5.f*mWidth_, 0.f));

        const unsigned numpoints = 256U;

        worldCenter_.Define(worldGroundRef_.center_, worldGround_.radius_ * 0.5f);
        worldAtmosphere_.GenerateShape(numpoints);
        worldHillTop_.GenerateShape(numpoints);
        worldGround_.GenerateShape(numpoints);
        worldCenter_.GenerateShape(numpoints);

        wBounds_.left_   = (int)floor(worldAtmosphere_.GetMinX() / mWidth_);
        wBounds_.top_    = (int)floor(worldAtmosphere_.GetMinY() / mHeight_);
        wBounds_.right_  = (int)floor(worldAtmosphere_.GetMaxX() / mWidth_);
        wBounds_.bottom_ = (int)floor(worldAtmosphere_.GetMaxY() / mHeight_);
#endif
    }
    else
        worldGround_.Define(Vector2::ZERO, Vector2::ZERO);
}

void World2DInfo::ParseParams(const String& params)
{
    Vector<String> vString = params.Split('|');
    if (vString.Size() == 0)
        return;

    int z=-1;
    String intStr;

    for (Vector<String>::Iterator it = vString.Begin(); it != vString.End(); ++it)
    {
        Vector<String> s = it->Split(':');
        if (s.Size() == 0)
            continue;

        String str = s[0].Trimmed();

        if (str == "z")
        {
            String str2 = s[1].Trimmed();
            if (str2 == "FRONTVIEW")
                z = FRONTVIEW;
            else if (str2 == "INNERVIEW")
                z = INNERVIEW;
            else if (str2 == "BACKGROUND")
                z = BACKGROUND;
            else
                z = ToInt(str2);
        }
        else if (str == "i")
        {
            intStr = s[1].Trimmed();
            if (z == -1 || intStr.Empty())
                continue;

            Vector<String> values = intStr.Split(';');
            GeneratorParams& gparams = genParams_[z];
            gparams.params_.Resize(values.Size());

            for (unsigned i=0; i<values.Size(); i++)
            {
                gparams.params_[i] = ToInt(values[i].Trimmed());
//                URHO3D_LOGWARNINGF("World2DInfo() - ParseParams last int param=%d (str=%s)", gparams.params_[i], values[i].CString());
            }
        }
    }
}

const GeneratorParams* World2DInfo::GetMapGenParams(int z) const
{
    HashMap<int, GeneratorParams >::ConstIterator g = genParams_.Find(z);
    return g != genParams_.End() ? &(g->second_) : 0;
}

Vector2 World2DInfo::GetPosition(int x, int y) const
{
    return Vector2((float)x * tileWidth_, (float)(mapHeight_ - 1 - y) * tileHeight_);
}

Vector2 World2DInfo::GetWorldPositionFromMapCenter(int x, int y, bool centertile) const
{
    return Vector2((float)x * mTileWidth_, (float)(mapHeight_ - 1 - y) * mTileHeight_) + (centertile ? Vector2(0.5f*mTileWidth_, 0.5f*mTileHeight_) : Vector2::ZERO);
}

Vector2 World2DInfo::GetCenteredWorldPosition(const ShortIntVector2& mPoint) const
{
    Vector2 position;
    position.x_ = ((float)(mapWidth_/2 + mPoint.x_ * mapWidth_) + 0.5f) * mTileWidth_;
    position.y_ = ((float)(mapHeight_*(mPoint.y_+1) - mapHeight_/2 - 1) + 0.5f) * mTileHeight_;
    return position;
}

ShortIntVector2 World2DInfo::GetMapPoint(const Vector2& position) const
{
    return ShortIntVector2((short int)floor(position.x_ / mWidth_), (short int)floor(position.y_ / mHeight_));
}

short int World2DInfo::GetMapPointCoordX(float x) const
{
    return (short int)floor(x / mWidth_);
}

short int World2DInfo::GetMapPointCoordY(float y) const
{
    return (short int)floor(y / mHeight_);
}

void World2DInfo::Convert2WorldTileCoords(const Vector2& position, int& x, int& y) const
{
    x = (int)(position.x_ / tileWidth_);
    y = mapHeight_ - 1 - int(position.y_ / tileHeight_);
}

void World2DInfo::Convert2WorldTileIndex(const ShortIntVector2& mPoint, const Vector2& position, unsigned& index) const
{
    index = (mapHeight_ - 1 - ((int)floor(position.y_ / mTileHeight_) - mPoint.y_ * mapHeight_)) * mapWidth_
            + (int)floor(position.x_ / mTileWidth_) - mPoint.x_ * mapWidth_;
}

void World2DInfo::Convert2WorldPosition(const ShortIntVector2& mPoint, const IntVector2& mPosition, Vector2& position) const
{
    position.x_ = ((float)(mPosition.x_ + mPoint.x_ * mapWidth_) + 0.5f) * mTileWidth_;
    position.y_ = ((float)(mapHeight_*(mPoint.y_+1) - mPosition.y_ - 1) + 0.5f) * mTileHeight_;
}

void World2DInfo::Convert2WorldPosition(const ShortIntVector2& mPoint, const IntVector2& mPosition, const Vector2& posInTile, Vector2& position) const
{
    position.x_ = ((float)(mPosition.x_ + mPoint.x_ * mapWidth_) + posInTile.x_) * mTileWidth_;
    position.y_ = ((float)(mapHeight_*(mPoint.y_+1) - mPosition.y_ - 1) + posInTile.y_) * mTileHeight_;
}

void World2DInfo::Convert2WorldPosition(const ShortIntVector2& mPoint, const Vector2& mPosition, Vector2& position) const
{
    position.x_ = mPosition.x_ + (float)(mPoint.x_ * mapWidth_) * mTileWidth_;
    position.y_ = mPosition.y_ + (float)(mPoint.y_ * mapHeight_) * mTileHeight_;
}

void World2DInfo::Convert2WorldPosition(const WorldMapPosition& wmPosition, Vector2& position)
{
//    position = wmPosition.position_;
    position.x_ = ((float)(wmPosition.mPosition_.x_ + wmPosition.mPoint_.x_ * mapWidth_) + 0.5f) * mTileWidth_;
    position.y_ = ((float)(mapHeight_*(wmPosition.mPoint_.y_+1) - wmPosition.mPosition_.y_ - 1) + 0.5f) * mTileHeight_;
}

void World2DInfo::UpdateWorldPosition(WorldMapPosition& wmPosition)
{
    wmPosition.defined_ = true;
    wmPosition.tileIndex_ = wmPosition.mPosition_.y_ * mapWidth_ + wmPosition.mPosition_.x_;
    wmPosition.position_.x_ = ((float)(wmPosition.mPosition_.x_ + wmPosition.mPoint_.x_ * mapWidth_) + 0.5f) * mTileWidth_;
    wmPosition.position_.y_ = ((float)(mapHeight_*(wmPosition.mPoint_.y_+1) - wmPosition.mPosition_.y_ - 1) + 0.5f) * mTileHeight_;
}

void World2DInfo::Convert2WorldMapPoint(float worldX, float worldY, ShortIntVector2& mPoint) const
{
    mPoint.x_ = (short int)floor(worldX / mWidth_);
    mPoint.y_ = (short int)floor(worldY / mHeight_);
}

void World2DInfo::Convert2WorldMapPoint(const Vector2& worldPosition, ShortIntVector2& mPoint) const
{
    mPoint.x_ = (short int)floor(worldPosition.x_ / mWidth_);
    mPoint.y_ = (short int)floor(worldPosition.y_ / mHeight_);
}

void World2DInfo::Convert2WorldMapPosition(const ShortIntVector2& mPoint, float worldX, float worldY, IntVector2& mPosition) const
{
    mPosition.x_ = (int)floor(worldX / mTileWidth_) - mPoint.x_ * mapWidth_;
    mPosition.y_ = mapHeight_ - 1 - ((int)floor(worldY / mTileHeight_) - mPoint.y_ * mapHeight_);
}

void World2DInfo::Convert2WorldMapPosition(const ShortIntVector2& mPoint, const Vector2& worldPosition, IntVector2& mPosition) const
{
    mPosition.x_ = (int)floor(worldPosition.x_ / mTileWidth_) - mPoint.x_ * mapWidth_;
    mPosition.y_ = mapHeight_ - 1 - ((int)floor(worldPosition.y_ / mTileHeight_) - mPoint.y_ * mapHeight_);

//    URHO3D_LOGINFOF("World2DInfo::ConvertToMapPosition() - worldPosition=%s mPoint=%s => mPosition=%s", worldPosition.ToString().CString(), mPoint.ToString().CString(),
//             mPosition.ToString().CString());
}

//void World2DInfo::Convert2WorldMapPosition(const ShortIntVector2& mPoint, const Vector2& worldPosition, IntVector2& mPosition, Vector2& posInTile) const
//{
//    float x, y;
//    x = worldPosition.x_/mTileWidth_ - (float)(mPoint.x_ * mapWidth_);
//    mPosition.x_ = (int)floor(x);
//    x -= (float)mPosition.x_;
//
//    y = worldPosition.y_/mTileHeight_ - (float)(mPoint.y_ * mapHeight_);
//    mPosition.y_ = (int)floor(y);
//    y -= (float)mPosition.y_;
//    mPosition.y_ = mapHeight_ - 1 - mPosition.y_;
//
//    posInTile.x_ = x / mTileWidth_;
//    posInTile.y_ = y / mTileHeight_;
//
////    URHO3D_LOGINFOF("World2DInfo::ConvertToMapPosition() - worldPosition=%s mPoint=%s => mPosition=%s posInTile=%s mTileWidth_=%f mTileHeight_=%f",
////                    worldPosition.ToString().CString(), mPoint.ToString().CString(), mPosition.ToString().CString(), posInTile.ToString().CString(),
////                    mTileWidth_, mTileHeight_);
//}

void World2DInfo::Convert2WorldMapPosition(const ShortIntVector2& mPoint, const Vector2& worldPosition, IntVector2& mPosition, Vector2& posInTile) const
{
    float coord;

    coord = worldPosition.x_/mTileWidth_ - (float)(mPoint.x_ * mapWidth_);
    mPosition.x_ = (int)floor(coord);
    posInTile.x_ = (coord - (float)mPosition.x_)*mTileWidth_;

    coord = worldPosition.y_/mTileHeight_ - (float)(mPoint.y_ * mapHeight_);
    mPosition.y_ = (int)floor(coord);
    posInTile.y_ = (coord - (float)mPosition.y_)*mTileHeight_;
    mPosition.y_ = mapHeight_ - 1 - mPosition.y_;

//    URHO3D_LOGINFOF("World2DInfo::ConvertToMapPosition() - worldPosition=%s mPoint=%s => mPosition=%s posInTile=%s mTileWidth_=%f mTileHeight_=%f",
//                    worldPosition.ToString().CString(), mPoint.ToString().CString(), mPosition.ToString().CString(), posInTile.ToString().CString(),
//                    mTileWidth_, mTileHeight_);
}

void World2DInfo::Convert2WorldMapPosition(float worldX, float worldY, WorldMapPosition& wmPosition) const
{
    Convert2WorldMapPoint(worldX, worldY, wmPosition.mPoint_);
    Convert2WorldMapPosition(wmPosition.mPoint_, worldX, worldY, wmPosition.mPosition_);
    wmPosition.defined_ = true;
    wmPosition.tileIndex_ = wmPosition.mPosition_.y_ * mapWidth_ + wmPosition.mPosition_.x_;
    wmPosition.position_ = Vector2(worldX, worldY);
}

void World2DInfo::Convert2WorldMapPosition(const Vector2& worldPosition, WorldMapPosition& wmPosition) const
{
    Convert2WorldMapPoint(worldPosition, wmPosition.mPoint_);
    Convert2WorldMapPosition(wmPosition.mPoint_, worldPosition, wmPosition.mPosition_);
    wmPosition.defined_ = true;
    wmPosition.tileIndex_ = wmPosition.mPosition_.y_ * mapWidth_ + wmPosition.mPosition_.x_;
    wmPosition.position_ = worldPosition;
}

void World2DInfo::Convert2WorldMapPosition(const Vector2& worldPosition, WorldMapPosition& wmPosition, Vector2& posInTile) const
{
    Convert2WorldMapPoint(worldPosition, wmPosition.mPoint_);
    Convert2WorldMapPosition(wmPosition.mPoint_, worldPosition, wmPosition.mPosition_, posInTile);
    wmPosition.defined_ = true;
    wmPosition.tileIndex_ = wmPosition.mPosition_.y_ * mapWidth_ + wmPosition.mPosition_.x_;
    wmPosition.position_ = worldPosition;
}

void World2DInfo::Convert2WorldMapPosition(const Vector2& worldPosition, ShortIntVector2& mPoint, IntVector2& mPosition) const
{
    mPoint.x_ = (short int)floor(worldPosition.x_ / mWidth_);
    mPoint.y_ = (short int)floor(worldPosition.y_ / mHeight_);

    mPosition.x_ = (int)floor(worldPosition.x_/mTileWidth_ - mPoint.x_ * mapWidth_);
    mPosition.y_ = mapHeight_ - 1 - (int)floor(worldPosition.y_ / mTileHeight_ - mPoint.y_ * mapHeight_);
}

void World2DInfo::Convert2WorldMapPosition(const Vector2& worldPosition, ShortIntVector2& mPoint, unsigned& tileindex) const
{
    mPoint.x_ = (short int)floor(worldPosition.x_ / mWidth_);
    mPoint.y_ = (short int)floor(worldPosition.y_ / mHeight_);

    int x = (int)floor(worldPosition.x_/mTileWidth_ - mPoint.x_ * mapWidth_);
    int y = mapHeight_ - 1 - (int)floor(worldPosition.y_ / mTileHeight_ - mPoint.y_ * mapHeight_);

    tileindex = y * mapWidth_ + x;

//    URHO3D_LOGINFOF("World2DInfo::Convert2WorldMapPosition() - worldPosition=%s => mPoint=%s x=%d y=%d tileindex=%u mHeight=%f mTileWidth_=%f mTileHeight_=%f",
//                    worldPosition.ToString().CString(), mPoint.ToString().CString(), x, y, tileindex, mHeight_, mTileWidth_, mTileHeight_);
}

unsigned World2DInfo::GetTileIndex(const IntVector2& mPosition) const
{
    return mPosition.y_ * mapWidth_ + mPosition.x_;
}

unsigned World2DInfo::GetTileIndex(int xi, int yi) const
{
    return yi * mapWidth_ + xi;
}

IntVector2 World2DInfo::GetTileCoords(unsigned tileindex) const
{
    return IntVector2(tileindex%mapWidth_, tileindex/mapWidth_);
}

Vector2 World2DInfo::GetPositionInTile(const Vector2& worldPosition) const
{
    float x = worldPosition.x_/mTileWidth_ - floor(worldPosition.x_ / mWidth_) * mapWidth_;
    x -= floor(x);
    x *= mTileWidth_;

    float y = worldPosition.y_/mTileHeight_ - floor(worldPosition.y_ / mHeight_) * mapHeight_;
    y -= floor(y);
    y *= mTileHeight_;

    return Vector2(x,y);
}

void World2DInfo::ConvertMapRect2WorldRect(const IntRect& rect, Rect& wRect) const
{
//    position.x_ = ((float)(mPosition.x_ + mPoint.x_ * mapWidth_) + 0.5f) * mTileWidth_;
//    position.y_ = ((float)(mapHeight_*(mPoint.y_+1) - mPosition.y_ - 1) + 0.5f) * mTileHeight_;

    wRect.min_.x_ = (float)(rect.left_ * mapWidth_) * mTileWidth_;
    wRect.min_.y_ = (float)((rect.top_) * mapHeight_) * mTileHeight_;

    wRect.max_.x_ = (float)((rect.right_+1) * mapWidth_) * mTileWidth_;
    wRect.max_.y_ = (float)((rect.bottom_+1) * mapHeight_) * mTileHeight_;
}

void World2DInfo::Convert2WorldRect(const IntRect& rect, const Vector2& startwposition, Rect& wRect) const
{
    wRect.min_.x_ = startwposition.x_ + (float)(rect.left_) * mTileWidth_;
    wRect.min_.y_ = startwposition.y_ + (float)(mapHeight_ - rect.top_) * mTileHeight_;

    wRect.max_.x_ = startwposition.x_ + (float)(rect.right_+1) * mTileWidth_;
    wRect.max_.y_ = startwposition.y_ + (float)(mapHeight_ - rect.bottom_-1) * mTileHeight_;
}

void World2DInfo::Dump() const
{
    URHO3D_LOGINFOF("World2DInfo() - Dump : this=%u", this);

    // dump params
    for (HashMap<int, GeneratorParams >::ConstIterator it=genParams_.Begin(); it!=genParams_.End(); ++it)
    {
        URHO3D_LOGINFOF("World2DInfo() - ParseParams : View=%d", it->first_);
        const GeneratorParams& gparams = it->second_;
        // dump params int
        for (unsigned i=0; i < gparams.params_.Size(); i++)
            URHO3D_LOGINFOF("World2DInfo() - ParseParams : INT param(%d) = %d", i, gparams.params_[i]);
    }
}






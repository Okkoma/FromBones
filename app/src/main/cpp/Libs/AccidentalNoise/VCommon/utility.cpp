#include "utility.h"

CoordPair closest_point(float vx, float vy, float x, float  y)
{
    float len=std::sqrt(vx*vx+vy*vy);
    float u=(x*vx+y*vy)/len;
    CoordPair c;
    c.x=u*vx;
    c.y=u*vy;
    return c;
}

float deg_to_rad(float deg)
{
    return deg*(3.14159265f/180.f);
}

float rad_to_deg(float rad)
{
    return rad*(180.0f/3.14159265f);
}

float hex_function(float x, float y)
{
    if(x==0 && y==0) return 1.f;

    float len=std::sqrt(x*x+y*y);
    float dx=x/len, dy=y/len;
    float angle_degrees=rad_to_deg(std::atan2(dy,dx));

    float angleincrement=60.f;
    float t=(angle_degrees/angleincrement);
    float a1=std::floor(t)*angleincrement;
    float a2=a1+angleincrement;

    float ax1=std::cos(deg_to_rad(a1));
    float ay1=std::sin(deg_to_rad(a1));
    float ax2=std::cos(deg_to_rad(a2));
    float ay2=std::sin(deg_to_rad(a2));

    CoordPair p1=closest_point(ax1,ay1,x,y);
    CoordPair p2=closest_point(ax2,ay2,x,y);

    float dist1=std::sqrt((x-p1.x)*(x-p1.x)+(y-p1.y)*(y-p1.y));
    float dist2=std::sqrt((x-p2.x)*(x-p2.x)+(y-p2.y)*(y-p2.y));

    if(dist1<dist2)
    {
        float d1=std::sqrt(p1.x*p1.x+p1.y*p1.y);
        return d1/0.86602540378443864676372317075294f;
    }
    else
    {
        float d1=std::sqrt(p2.x*p2.x+p2.y*p2.y);
        return d1/0.86602540378443864676372317075294f;
    }

}

TileCoord calcHexPointTile(float px, float py)
{
    TileCoord tile;
    float rise=0.5f;
    float slope=rise/0.8660254f;
    int X=(int)(px/1.732051f);
    int Y=(int)(py/1.5f);

    float offsetX=px-(float)X*1.732051f;
    float offsetY=py-(float)Y*1.5f;

    if(fmod(Y, 2)==0)
    {
        if(offsetY < (-slope * offsetX+rise))
        {
            --X;
            --Y;
        }
        else if(offsetY < (slope*offsetX-rise))
        {
            --Y;
        }
    }
    else
    {
        if(offsetX >= 0.8660254f)
        {
            if(offsetY <(-slope*offsetX+rise*2))
            {
                --Y;
            }
        }
        else
        {
            if(offsetY<(slope*offsetX))
            {
                --Y;
            }
            else
            {
                --X;
            }
        }
    }
    tile.x=X;
    tile.y=Y;
    return tile;
}

CoordPair calcHexTileCenter(int tx, int ty)
{
    CoordPair origin;
    float ymod=fmod(ty, 2.0f);
    if(ymod==1.0f) ymod=0.8660254f;
    else ymod=0.0f;
    origin.x=(float)tx*1.732051f+ymod;
    origin.y=(float)ty*1.5f;
    CoordPair center;
    center.x=origin.x+0.8660254f;
    center.y=origin.y+1.f;
    //std::cout << "Hex Coords: " << tx << ", " << ty << "  Center: " << center[0] << ", " << center[1] << std::endl;
    return center;
}


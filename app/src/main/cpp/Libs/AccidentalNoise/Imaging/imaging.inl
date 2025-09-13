#include <fstream>
#include <iostream>
#include <string>
#include <algorithm>
#include <cctype>
#include <vector>

#include <Urho3D/Urho3D.h>

#include <Urho3D/IO/Log.h>

#include "Predefined.h"

#ifndef URHO3D_STATIC_DEFINE
    #ifndef STB_IMAGE_IMPLEMENTATION
        #define STB_IMAGE_IMPLEMENTATION
    #endif

    #ifndef STB_IMAGE_WRITE_IMPLEMENTATION
        #define STB_IMAGE_WRITE_IMPLEMENTATION
    #endif
#else
    #undef STB_IMAGE_IMPLEMENTATION
    #undef STB_IMAGE_WRITE_IMPLEMENTATION
#endif


#include "stb_image.h"
#include "stb_image_write.h"


std::string StringToUpper(std::string strToConvert)
{
    std::transform(strToConvert.begin(), strToConvert.end(), strToConvert.begin(), ::toupper);
    return strToConvert;
}

namespace anl
{

const int SNAPGENSLOT = 1;

/// V0 Interface
void map2D(int seamlessmode, CArray2Dd &a, CImplicitModuleBase &m, SMappingRanges &ranges, float z)
{
    static float pi2=3.141592f*2.f;

    unsigned w=a.width();
    unsigned h=a.height();
    unsigned x,y;

    for(x=0; x<w; ++x)
    {
        for(y=0; y<h; ++y)
        {
            float p=(float)x / (float)w;
            float q=(float)y / (float)h;
            float r;
            float nx,ny,nz,nw,nu,nv,val=0.0;
            float dx, dy, dz;
            switch(seamlessmode)
            {
            case SEAMLESS_NONE:
            {
                nx=ranges.mapx0 + p*(ranges.mapx1-ranges.mapx0);
                ny=ranges.mapy0 + q*(ranges.mapy1-ranges.mapy0);
                nz=z;
                val=m.get(nx,ny,nz);
            }
            break;
            case SEAMLESS_X:
            {
                dx=ranges.loopx1-ranges.loopx0;
                dy=ranges.mapy1-ranges.mapy0;
                p=p*(ranges.mapx1-ranges.mapx0)/(ranges.loopx1-ranges.loopx0);
                nx=ranges.loopx0 + cos(p*pi2) * dx/pi2;
                ny=ranges.loopx0 + sin(p*pi2) * dx/pi2;
                nz=ranges.mapy0 + q*dy;
                nw=z;
                val=m.get(nx,ny,nz,nw);
            }
            break;
            case SEAMLESS_Y:
            {
                dx=ranges.mapx1-ranges.mapx0;
                dy=ranges.loopy1-ranges.loopy0;
                q=q*(ranges.mapy1-ranges.mapy0)/(ranges.loopy1-ranges.loopy0);
                nx=ranges.mapx0 + p*dx;
                ny=ranges.loopy0 + cos(q*pi2) * dy/pi2;
                nz=ranges.loopy0 + sin(q*pi2) * dy/pi2;
                nw=z;
                val=m.get(nx,ny,nz,nw);
            }
            break;
            case SEAMLESS_Z:
            {
                dx=ranges.mapx1-ranges.mapx0;
                dy=ranges.mapy1-ranges.mapy0;
                dz=ranges.loopz1-ranges.loopz0;
                nx=ranges.mapx0 + p*dx;
                ny=ranges.mapy0 + p*dx;
                r=(z-ranges.mapz0)/(ranges.mapz1-ranges.mapz0);
                float zval=r*(ranges.mapz1-ranges.mapz0)/(ranges.loopz1-ranges.loopz0);
                nz=ranges.loopz0 + cos(zval*pi2) * dz/pi2;
                nw=ranges.loopz0 + sin(zval*pi2) * dz/pi2;
                val=m.get(nx,ny,nz,nw);
            }
            break;
            case SEAMLESS_XY:
            {
                dx=ranges.loopx1-ranges.loopx0;
                dy=ranges.loopy1-ranges.loopy0;
                p=p*(ranges.mapx1-ranges.mapx0)/(ranges.loopx1-ranges.loopx0);
                q=q*(ranges.mapy1-ranges.mapy0)/(ranges.loopy1-ranges.loopy0);
                nx=ranges.loopx0 + cos(p*pi2) * dx/pi2;
                ny=ranges.loopx0 + sin(p*pi2) * dx/pi2;
                nz=ranges.loopy0 + cos(q*pi2) * dy/pi2;
                nw=ranges.loopy0 + sin(q*pi2) * dy/pi2;
                nu=z;
                val=m.get(nx,ny,nz,nw,nu,0);
            }
            break;
            case SEAMLESS_XZ:
            {
                dx=ranges.loopx1-ranges.loopx0;
                dy=ranges.mapy1-ranges.mapy0;
                dz=ranges.loopz1-ranges.loopz0;
                r=(z-ranges.mapz0)/(ranges.mapz1-ranges.mapz0);
                float zval=r*(ranges.mapx1-ranges.mapz0)/(ranges.loopz1-ranges.loopz0);
                p=p*(ranges.mapx1-ranges.mapx0)/(ranges.loopx1-ranges.loopx0);
                nx=ranges.loopx0 + cos(p*pi2) * dx/pi2;
                ny=ranges.loopx0 + sin(p*pi2) *dx/pi2;
                nz=ranges.mapy0 + q*dy;
                nw=ranges.loopz0 + cos(zval*pi2)*dz/pi2;
                nu=ranges.loopz0 + sin(zval*pi2)*dz/pi2;
                val=m.get(nx,ny,nz,nw,nu,0);
            }
            break;
            case SEAMLESS_YZ:
            {
                dx=ranges.mapx1-ranges.mapx0;
                dy=ranges.loopy1-ranges.loopy0;
                dz=ranges.loopz1-ranges.loopz0;
                r=(z-ranges.mapz0)/(ranges.mapz1-ranges.mapz0);
                float zval=r*(ranges.mapz1-ranges.mapz0)/(ranges.loopz1-ranges.loopz0);
                q=q*(ranges.mapy1-ranges.mapy0)/(ranges.loopy1-ranges.loopy0);
                nx=ranges.mapx0+p*dx;
                ny=ranges.loopy0 + cos(q*pi2) * dy/pi2;
                nz=ranges.loopy0 + sin(q*pi2) * dy/pi2;
                nw=ranges.loopz0 + cos(zval*pi2) * dz/pi2;
                nu=ranges.loopz0 + sin(zval*pi2) * dz/pi2;
                val=m.get(nx,ny,nz,nw,nu,0);
            }
            break;
            case SEAMLESS_XYZ:
            {
                dx=ranges.loopx1-ranges.loopx0;
                dy=ranges.loopy1-ranges.loopy0;
                dz=ranges.loopz1-ranges.loopz0;
                p=p*(ranges.mapx1-ranges.mapx0)/(ranges.loopx1-ranges.loopx0);
                q=q*(ranges.mapy1-ranges.mapy0)/(ranges.loopy1-ranges.loopy0);
                r=(z-ranges.mapz0)/(ranges.mapz1-ranges.mapz0);
                float zval=r*(ranges.mapz1-ranges.mapz0)/(ranges.loopz1-ranges.loopz0);
                nx=ranges.loopx0 + cos(p*pi2)*dx/pi2;
                ny=ranges.loopx0 + sin(p*pi2)*dx/pi2;
                nz=ranges.loopy0 + cos(q*pi2)*dy/pi2;
                nw=ranges.loopy0 + sin(q*pi2)*dy/pi2;
                nu=ranges.loopz0 + cos(zval*pi2)*dz/pi2;
                nv=ranges.loopz0 + sin(zval*pi2)*dz/pi2;
                val=m.get(nx,ny,nz,nw,nu,nv);
            }
            break;

            default:
                break;
            }
            a.set(x,y,val);
        }
    }
}

void map2DNoZ(int seamlessmode, CArray2Dd &a, CImplicitModuleBase &m, SMappingRanges &ranges)
{
    static float pi2=3.141592f*2.f;

    unsigned w=a.width();
    unsigned h=a.height();
    unsigned x,y;

    for(x=0; x<w; ++x)
    {
        for(y=0; y<h; ++y)
        {
            float p=(float)x / (float)w;
            float q=(float)y / (float)h;
            float nx,ny,nz,nw,val=0.0;
            float dx, dy;
            switch(seamlessmode)
            {
            case SEAMLESS_NONE:
            {
                nx=ranges.mapx0 + p*(ranges.mapx1-ranges.mapx0);
                ny=ranges.mapy0 + q*(ranges.mapy1-ranges.mapy0);
                val=m.get(nx,ny);
            }
            break;
            case SEAMLESS_X:
            {
                dx=ranges.loopx1-ranges.loopx0;
                dy=ranges.mapy1-ranges.mapy0;
                p=p*(ranges.mapx1-ranges.mapx0)/(ranges.loopx1-ranges.loopx0);
                nx=ranges.loopx0 + cos(p*pi2) * dx/pi2;
                ny=ranges.loopx0 + sin(p*pi2) * dx/pi2;
                nz=ranges.mapy0 + q*dy;
                val=m.get(nx,ny,nz);
            }
            break;
            case SEAMLESS_Y:
            {
                dx=ranges.mapx1-ranges.mapx0;
                dy=ranges.loopy1-ranges.loopy0;
                q=q*(ranges.mapy1-ranges.mapy0)/(ranges.loopy1-ranges.loopy0);
                nx=ranges.mapx0 + p*dx;
                ny=ranges.loopy0 + cos(q*pi2) * dy/pi2;
                nz=ranges.loopy0 + sin(q*pi2) * dy/pi2;
                val=m.get(nx,ny,nz);
            }
            break;

            case SEAMLESS_XY:
            {
                dx=ranges.loopx1-ranges.loopx0;
                dy=ranges.loopy1-ranges.loopy0;
                p=p*(ranges.mapx1-ranges.mapx0)/(ranges.loopx1-ranges.loopx0);
                q=q*(ranges.mapy1-ranges.mapy0)/(ranges.loopy1-ranges.loopy0);
                nx=ranges.loopx0 + cos(p*pi2) * dx/pi2;
                ny=ranges.loopx0 + sin(p*pi2) * dx/pi2;
                nz=ranges.loopy0 + cos(q*pi2) * dy/pi2;
                nw=ranges.loopy0 + sin(q*pi2) * dy/pi2;
                val=m.get(nx,ny,nz,nw);
            }
            break;
            default:
                break;
            }
            a.set(x,y,val);
        }
    }
}

void map3D(int seamlessmode, CArray3Dd &a, CImplicitModuleBase &m, SMappingRanges &ranges)
{
    static float pi2=3.14159265f*2.f;

    unsigned w=a.width();
    unsigned h=a.height();
    unsigned d=a.depth();
    unsigned x,y,z;

    for(x=0; x<w; ++x)
    {
        for(y=0; y<h; ++y)
        {
            for(z=0; z<d; ++z)
            {
                float p=(float)x/(float)w;
                float q=(float)y/(float)h;
                float r=(float)z/(float)d;
                float nx,ny,nz,nw,nu,nv;
                float dx,dy,dz;
                float val=0.0;

                switch(seamlessmode)
                {
                case SEAMLESS_NONE:
                {
                    dx=ranges.mapx1-ranges.mapx0;
                    dy=ranges.mapy1-ranges.mapy0;
                    dz=ranges.mapz1-ranges.mapz0;
                    nx=ranges.mapx0 + p*dx;
                    ny=ranges.mapy0 + q*dy;
                    nz=ranges.mapz0 + r*dz;
                    val=m.get(nx,ny,nz);
                }
                break;
                case SEAMLESS_X:
                {
                    dx=ranges.loopx1-ranges.loopx0;
                    dy=ranges.mapy1-ranges.mapy0;
                    dz=ranges.mapz1-ranges.mapz0;
                    p=p*(ranges.mapx1-ranges.mapx0)/(ranges.loopx1-ranges.loopx0);
                    nx=ranges.loopx0 + cos(p*pi2)*dx/pi2;
                    ny=ranges.loopx0 + sin(p*pi2)*dx/pi2;
                    nz=ranges.mapy0 + q*dy;
                    nw=ranges.mapz0 + d*dz;
                    val=m.get(nx,ny,nz,nw);
                }
                case SEAMLESS_Y:
                {
                    dx=ranges.mapx1-ranges.mapx0;
                    dy=ranges.loopy1-ranges.loopy0;
                    dz=ranges.mapz1-ranges.mapz0;
                    q=q*(ranges.mapy1-ranges.mapy0)/(ranges.loopy1-ranges.loopy0);
                    nx=ranges.mapx0 + p*dx;
                    ny=ranges.loopy0 + cos(q*pi2)*dy/pi2;
                    nz=ranges.loopy0 + sin(q*pi2)*dy/pi2;
                    nw=ranges.mapz0 + r*dz;
                    val=m.get(nx,ny,nz,nw);
                }
                break;
                case SEAMLESS_Z:
                {
                    dx=ranges.mapx1-ranges.mapx0;
                    dy=ranges.mapy1-ranges.mapy0;
                    dz=ranges.loopz1-ranges.loopz0;
                    r=r*(ranges.mapz1-ranges.mapz0)/(ranges.loopz1-ranges.loopz0);
                    nx=ranges.mapx0 + p*dx;
                    ny=ranges.mapy0 + q*dy;
                    nz=ranges.loopz0 + cos(r*pi2)*dz/pi2;
                    nw=ranges.loopz0 + sin(r*pi2)*dz/pi2;
                    val=m.get(nx,ny,nz,nw);
                }
                break;
                case SEAMLESS_XY:
                {
                    dx=ranges.loopx1-ranges.loopx0;
                    dy=ranges.loopy1-ranges.loopy0;
                    dz=ranges.mapz1-ranges.mapz0;
                    p=p*(ranges.mapx1-ranges.mapx0)/(ranges.loopx1-ranges.loopx0);
                    q=q*(ranges.mapy1-ranges.mapy0)/(ranges.loopy1-ranges.loopy0);
                    nx=ranges.loopx0 + cos(p*pi2)*dx/pi2;
                    ny=ranges.loopx0 + sin(p*pi2)*dx/pi2;
                    nz=ranges.loopy0 + cos(q*pi2)*dy/pi2;
                    nw=ranges.loopy0 + sin(q*pi2)*dy/pi2;
                    nu=ranges.mapz0 + r*dz;
                    val=m.get(nx,ny,nz,nw,nu,0);
                }
                break;
                case SEAMLESS_XZ:
                {
                    dx=ranges.loopx1-ranges.loopx0;
                    dy=ranges.mapy1-ranges.mapy0;
                    dz=ranges.loopz1-ranges.loopz0;
                    p=p*(ranges.mapx1-ranges.mapx0)/(ranges.loopx1-ranges.loopx0);
                    r=r*(ranges.mapz1-ranges.mapz0)/(ranges.loopz1-ranges.loopz0);
                    nx=ranges.loopx0 + cos(p*pi2)*dx/pi2;
                    ny=ranges.loopx0 + sin(p*pi2)*dx/pi2;
                    nz=ranges.mapy0 + q*dy;
                    nw=ranges.loopz0 + cos(r*pi2)*dz/pi2;
                    nu=ranges.loopz0 + sin(r*pi2)*dz/pi2;
                    val=m.get(nx,ny,nz,nw,nu,0);
                }
                break;
                case SEAMLESS_YZ:
                {
                    dx=ranges.mapx1-ranges.mapx0;
                    dy=ranges.loopy1-ranges.loopy0;
                    dz=ranges.loopz1-ranges.loopz0;
                    q=q*(ranges.mapy1-ranges.mapy0)/(ranges.loopy1-ranges.loopy0);
                    r=r*(ranges.mapz1-ranges.mapz0)/(ranges.loopz1-ranges.loopz0);
                    nx=ranges.mapx0 + p*dx;
                    ny=ranges.loopy0 + cos(q*pi2)*dy/pi2;
                    nz=ranges.loopy0 + sin(q*pi2)*dy/pi2;
                    nw=ranges.loopz0 + cos(r*pi2)*dz/pi2;
                    nu=ranges.loopz0 + sin(r*pi2)*dz/pi2;
                    val=m.get(nx,ny,nz,nw,nu,0);
                }
                break;
                case SEAMLESS_XYZ:
                {
                    dx=ranges.loopx1-ranges.loopx0;
                    dy=ranges.loopy1-ranges.loopy0;
                    dz=ranges.loopz1-ranges.loopz0;
                    p=p*(ranges.mapx1-ranges.mapx0)/(ranges.loopx1-ranges.loopx0);
                    q=q*(ranges.mapy1-ranges.mapy0)/(ranges.loopy1-ranges.loopy0);
                    r=r*(ranges.mapz1-ranges.mapz0)/(ranges.loopz1-ranges.loopz0);
                    nx=ranges.loopx0 + cos(p*pi2)*dx/pi2;
                    ny=ranges.loopx0 + sin(p*pi2)*dx/pi2;
                    nz=ranges.loopy0 + cos(q*pi2)*dy/pi2;
                    nw=ranges.loopy0 + sin(q*pi2)*dy/pi2;
                    nu=ranges.loopz0 + cos(r*pi2)*dz/pi2;
                    nv=ranges.loopz0 + sin(r*pi2)*dz/pi2;
                    val=m.get(nx,ny,nz,nw,nu,nv);
                }
                break;
                default:
                    break;
                }
                a.set(x,y,z,val);
            }
        }
    }
}

void mapRGBA2D(int seamlessmode, CArray2Drgba &a, CRGBAModuleBase &m, SMappingRanges &ranges, float z)
{
    static float pi2=3.141592f*2.0f;

    unsigned w=a.width();
    unsigned h=a.height();
    unsigned x,y;

    for(x=0; x<w; ++x)
    {
        for(y=0; y<h; ++y)
        {
            float p=(float)x / (float)w;
            float q=(float)y / (float)h;
            float r;
            float nx,ny,nz,nw,nu,nv=0.0;
            SRGBA val;
            float dx, dy, dz;
            switch(seamlessmode)
            {
            case SEAMLESS_NONE:
            {
                nx=ranges.mapx0 + p*(ranges.mapx1-ranges.mapx0);
                ny=ranges.mapy0 + q*(ranges.mapy1-ranges.mapy0);
                nz=z;
                val=m.get(nx,ny,nz);
            }
            break;
            case SEAMLESS_X:
            {
                dx=ranges.loopx1-ranges.loopx0;
                dy=ranges.mapy1-ranges.mapy0;
                p=p*(ranges.mapx1-ranges.mapx0)/(ranges.loopx1-ranges.loopx0);
                nx=ranges.loopx0 + cos(p*pi2) * dx/pi2;
                ny=ranges.loopx0 + sin(p*pi2) * dx/pi2;
                nz=ranges.mapy0 + q*dy;
                nw=z;
                val=m.get(nx,ny,nz,nw);
            }
            break;
            case SEAMLESS_Y:
            {
                dx=ranges.mapx1-ranges.mapx0;
                dy=ranges.loopy1-ranges.loopy0;
                q=q*(ranges.mapy1-ranges.mapy0)/(ranges.loopy1-ranges.loopy0);
                nx=ranges.mapx0 + p*dx;
                ny=ranges.loopy0 + cos(q*pi2) * dy/pi2;
                nz=ranges.loopy0 + sin(q*pi2) * dy/pi2;
                nw=z;
                val=m.get(nx,ny,nz,nw);
            }
            break;
            case SEAMLESS_Z:
            {
                dx=ranges.mapx1-ranges.mapx0;
                dy=ranges.mapy1-ranges.mapy0;
                dz=ranges.loopz1-ranges.loopz0;
                nx=ranges.mapx0 + p*dx;
                ny=ranges.mapy0 + p*dx;
                r=(z-ranges.mapz0)/(ranges.mapz1-ranges.mapz0);
                float zval=r*(ranges.mapz1-ranges.mapz0)/(ranges.loopz1-ranges.loopz0);
                nz=ranges.loopz0 + cos(zval*pi2) * dz/pi2;
                nw=ranges.loopz0 + sin(zval*pi2) * dz/pi2;
                val=m.get(nx,ny,nz,nw);
            }
            break;
            case SEAMLESS_XY:
            {
                dx=ranges.loopx1-ranges.loopx0;
                dy=ranges.loopy1-ranges.loopy0;
                p=p*(ranges.mapx1-ranges.mapx0)/(ranges.loopx1-ranges.loopx0);
                q=q*(ranges.mapy1-ranges.mapy0)/(ranges.loopy1-ranges.loopy0);
                nx=ranges.loopx0 + cos(p*pi2) * dx/pi2;
                ny=ranges.loopx0 + sin(p*pi2) * dx/pi2;
                nz=ranges.loopy0 + cos(q*pi2) * dy/pi2;
                nw=ranges.loopy0 + sin(q*pi2) * dy/pi2;
                nu=z;
                val=m.get(nx,ny,nz,nw,nu,0);
            }
            break;
            case SEAMLESS_XZ:
            {
                dx=ranges.loopx1-ranges.loopx0;
                dy=ranges.mapy1-ranges.mapy0;
                dz=ranges.loopz1-ranges.loopz0;
                r=(z-ranges.mapz0)/(ranges.mapz1-ranges.mapz0);
                float zval=r*(ranges.mapx1-ranges.mapz0)/(ranges.loopz1-ranges.loopz0);
                p=p*(ranges.mapx1-ranges.mapx0)/(ranges.loopx1-ranges.loopx0);
                nx=ranges.loopx0 + cos(p*pi2) * dx/pi2;
                ny=ranges.loopx0 + sin(p*pi2) *dx/pi2;
                nz=ranges.mapy0 + q*dy;
                nw=ranges.loopz0 + cos(zval*pi2)*dz/pi2;
                nu=ranges.loopz0 + sin(zval*pi2)*dz/pi2;
                val=m.get(nx,ny,nz,nw,nu,0);
            }
            break;
            case SEAMLESS_YZ:
            {
                dx=ranges.mapx1-ranges.mapx0;
                dy=ranges.loopy1-ranges.loopy0;
                dz=ranges.loopz1-ranges.loopz0;
                r=(z-ranges.mapz0)/(ranges.mapz1-ranges.mapz0);
                float zval=r*(ranges.mapz1-ranges.mapz0)/(ranges.loopz1-ranges.loopz0);
                q=q*(ranges.mapy1-ranges.mapy0)/(ranges.loopy1-ranges.loopy0);
                nx=ranges.mapx0+p*dx;
                ny=ranges.loopy0 + cos(q*pi2) * dy/pi2;
                nz=ranges.loopy0 + sin(q*pi2) * dy/pi2;
                nw=ranges.loopz0 + cos(zval*pi2) * dz/pi2;
                nu=ranges.loopz0 + sin(zval*pi2) * dz/pi2;
                val=m.get(nx,ny,nz,nw,nu,0);
            }
            break;
            case SEAMLESS_XYZ:
            {
                dx=ranges.loopx1-ranges.loopx0;
                dy=ranges.loopy1-ranges.loopy0;
                dz=ranges.loopz1-ranges.loopz0;
                p=p*(ranges.mapx1-ranges.mapx0)/(ranges.loopx1-ranges.loopx0);
                q=q*(ranges.mapy1-ranges.mapy0)/(ranges.loopy1-ranges.loopy0);
                r=(z-ranges.mapz0)/(ranges.mapz1-ranges.mapz0);
                float zval=r*(ranges.mapz1-ranges.mapz0)/(ranges.loopz1-ranges.loopz0);
                nx=ranges.loopx0 + cos(p*pi2)*dx/pi2;
                ny=ranges.loopx0 + sin(p*pi2)*dx/pi2;
                nz=ranges.loopy0 + cos(q*pi2)*dy/pi2;
                nw=ranges.loopy0 + sin(q*pi2)*dy/pi2;
                nu=ranges.loopz0 + cos(zval*pi2)*dz/pi2;
                nv=ranges.loopz0 + sin(zval*pi2)*dz/pi2;
                val=m.get(nx,ny,nz,nw,nu,nv);
            }
            break;

            default:
                break;
            }
            a.set(x,y,val);
        }
    }
}

void mapRGBA2DNoZ(int seamlessmode, CArray2Drgba &a, CRGBAModuleBase &m, SMappingRanges &ranges)
{
    static float pi2=3.141592f*2.0f;

    unsigned w=a.width();
    unsigned h=a.height();
    unsigned x,y;

    for(x=0; x<w; ++x)
    {
        for(y=0; y<h; ++y)
        {
            float p=(float)x / (float)w;
            float q=(float)y / (float)h;
            float nx,ny,nz,nw=0.0;
            SRGBA val;
            float dx, dy;
            switch(seamlessmode)
            {
            case SEAMLESS_NONE:
            {
                nx=ranges.mapx0 + p*(ranges.mapx1-ranges.mapx0);
                ny=ranges.mapy0 + q*(ranges.mapy1-ranges.mapy0);
                val=m.get(nx,ny);
            }
            break;
            case SEAMLESS_X:
            {
                dx=ranges.loopx1-ranges.loopx0;
                dy=ranges.mapy1-ranges.mapy0;
                p=p*(ranges.mapx1-ranges.mapx0)/(ranges.loopx1-ranges.loopx0);
                nx=ranges.loopx0 + cos(p*pi2) * dx/pi2;
                ny=ranges.loopx0 + sin(p*pi2) * dx/pi2;
                nz=ranges.mapy0 + q*dy;
                val=m.get(nx,ny,nz);
            }
            break;
            case SEAMLESS_Y:
            {
                dx=ranges.mapx1-ranges.mapx0;
                dy=ranges.loopy1-ranges.loopy0;
                q=q*(ranges.mapy1-ranges.mapy0)/(ranges.loopy1-ranges.loopy0);
                nx=ranges.mapx0 + p*dx;
                ny=ranges.loopy0 + cos(q*pi2) * dy/pi2;
                nz=ranges.loopy0 + sin(q*pi2) * dy/pi2;
                val=m.get(nx,ny,nz);
            }
            break;

            case SEAMLESS_XY:
            {
                dx=ranges.loopx1-ranges.loopx0;
                dy=ranges.loopy1-ranges.loopy0;
                p=p*(ranges.mapx1-ranges.mapx0)/(ranges.loopx1-ranges.loopx0);
                q=q*(ranges.mapy1-ranges.mapy0)/(ranges.loopy1-ranges.loopy0);
                nx=ranges.loopx0 + cos(p*pi2) * dx/pi2;
                ny=ranges.loopx0 + sin(p*pi2) * dx/pi2;
                nz=ranges.loopy0 + cos(q*pi2) * dy/pi2;
                nw=ranges.loopy0 + sin(q*pi2) * dy/pi2;
                val=m.get(nx,ny,nz,nw);
            }
            break;
            default:
                break;
            }
            a.set(x,y,val);
        }
    }
}

void mapRGBA3D(int seamlessmode, CArray3Drgba &a, CRGBAModuleBase &m, SMappingRanges &ranges)
{
    static float pi2=3.141592f*2.0f;

    unsigned w=a.width();
    unsigned h=a.height();
    unsigned d=a.depth();
    unsigned x,y,z;

    for(x=0; x<w; ++x)
    {
        for(y=0; y<h; ++y)
        {
            for(z=0; z<d; ++z)
            {
                float p=(float)x/(float)w;
                float q=(float)y/(float)h;
                float r=(float)z/(float)d;
                float nx,ny,nz,nw,nu,nv;
                float dx,dy,dz;
                SRGBA val;

                switch(seamlessmode)
                {
                case SEAMLESS_NONE:
                {
                    dx=ranges.mapx1-ranges.mapx0;
                    dy=ranges.mapy1-ranges.mapy0;
                    dz=ranges.mapz1-ranges.mapz0;
                    nx=ranges.mapx0 + p*dx;
                    ny=ranges.mapy0 + q*dy;
                    nz=ranges.mapz0 + r*dz;
                    val=m.get(nx,ny,nz);
                }
                break;
                case SEAMLESS_X:
                {
                    dx=ranges.loopx1-ranges.loopx0;
                    dy=ranges.mapy1-ranges.mapy0;
                    dz=ranges.mapz1-ranges.mapz0;
                    p=p*(ranges.mapx1-ranges.mapx0)/(ranges.loopx1-ranges.loopx0);
                    nx=ranges.loopx0 + cos(p*pi2)*dx/pi2;
                    ny=ranges.loopx0 + sin(p*pi2)*dx/pi2;
                    nz=ranges.mapy0 + q*dy;
                    nw=ranges.mapz0 + d*dz;
                    val=m.get(nx,ny,nz,nw);
                }
                case SEAMLESS_Y:
                {
                    dx=ranges.mapx1-ranges.mapx0;
                    dy=ranges.loopy1-ranges.loopy0;
                    dz=ranges.mapz1-ranges.mapz0;
                    q=q*(ranges.mapy1-ranges.mapy0)/(ranges.loopy1-ranges.loopy0);
                    nx=ranges.mapx0 + p*dx;
                    ny=ranges.loopy0 + cos(q*pi2)*dy/pi2;
                    nz=ranges.loopy0 + sin(q*pi2)*dy/pi2;
                    nw=ranges.mapz0 + r*dz;
                    val=m.get(nx,ny,nz,nw);
                }
                break;
                case SEAMLESS_Z:
                {
                    dx=ranges.mapx1-ranges.mapx0;
                    dy=ranges.mapy1-ranges.mapy0;
                    dz=ranges.loopz1-ranges.loopz0;
                    r=r*(ranges.mapz1-ranges.mapz0)/(ranges.loopz1-ranges.loopz0);
                    nx=ranges.mapx0 + p*dx;
                    ny=ranges.mapy0 + q*dy;
                    nz=ranges.loopz0 + cos(r*pi2)*dz/pi2;
                    nw=ranges.loopz0 + sin(r*pi2)*dz/pi2;
                    val=m.get(nx,ny,nz,nw);
                }
                break;
                case SEAMLESS_XY:
                {
                    dx=ranges.loopx1-ranges.loopx0;
                    dy=ranges.loopy1-ranges.loopy0;
                    dz=ranges.mapz1-ranges.mapz0;
                    p=p*(ranges.mapx1-ranges.mapx0)/(ranges.loopx1-ranges.loopx0);
                    q=q*(ranges.mapy1-ranges.mapy0)/(ranges.loopy1-ranges.loopy0);
                    nx=ranges.loopx0 + cos(p*pi2)*dx/pi2;
                    ny=ranges.loopx0 + sin(p*pi2)*dx/pi2;
                    nz=ranges.loopy0 + cos(q*pi2)*dy/pi2;
                    nw=ranges.loopy0 + sin(q*pi2)*dy/pi2;
                    nu=ranges.mapz0 + r*dz;
                    val=m.get(nx,ny,nz,nw,nu,0);
                }
                break;
                case SEAMLESS_XZ:
                {
                    dx=ranges.loopx1-ranges.loopx0;
                    dy=ranges.mapy1-ranges.mapy0;
                    dz=ranges.loopz1-ranges.loopz0;
                    p=p*(ranges.mapx1-ranges.mapx0)/(ranges.loopx1-ranges.loopx0);
                    r=r*(ranges.mapz1-ranges.mapz0)/(ranges.loopz1-ranges.loopz0);
                    nx=ranges.loopx0 + cos(p*pi2)*dx/pi2;
                    ny=ranges.loopx0 + sin(p*pi2)*dx/pi2;
                    nz=ranges.mapy0 + q*dy;
                    nw=ranges.loopz0 + cos(r*pi2)*dz/pi2;
                    nu=ranges.loopz0 + sin(r*pi2)*dz/pi2;
                    val=m.get(nx,ny,nz,nw,nu,0);
                }
                break;
                case SEAMLESS_YZ:
                {
                    dx=ranges.mapx1-ranges.mapx0;
                    dy=ranges.loopy1-ranges.loopy0;
                    dz=ranges.loopz1-ranges.loopz0;
                    q=q*(ranges.mapy1-ranges.mapy0)/(ranges.loopy1-ranges.loopy0);
                    r=r*(ranges.mapz1-ranges.mapz0)/(ranges.loopz1-ranges.loopz0);
                    nx=ranges.mapx0 + p*dx;
                    ny=ranges.loopy0 + cos(q*pi2)*dy/pi2;
                    nz=ranges.loopy0 + sin(q*pi2)*dy/pi2;
                    nw=ranges.loopz0 + cos(r*pi2)*dz/pi2;
                    nu=ranges.loopz0 + sin(r*pi2)*dz/pi2;
                    val=m.get(nx,ny,nz,nw,nu,0);
                }
                break;
                case SEAMLESS_XYZ:
                {
                    dx=ranges.loopx1-ranges.loopx0;
                    dy=ranges.loopy1-ranges.loopy0;
                    dz=ranges.loopz1-ranges.loopz0;
                    p=p*(ranges.mapx1-ranges.mapx0)/(ranges.loopx1-ranges.loopx0);
                    q=q*(ranges.mapy1-ranges.mapy0)/(ranges.loopy1-ranges.loopy0);
                    r=r*(ranges.mapz1-ranges.mapz0)/(ranges.loopz1-ranges.loopz0);
                    nx=ranges.loopx0 + cos(p*pi2)*dx/pi2;
                    ny=ranges.loopx0 + sin(p*pi2)*dx/pi2;
                    nz=ranges.loopy0 + cos(q*pi2)*dy/pi2;
                    nw=ranges.loopy0 + sin(q*pi2)*dy/pi2;
                    nu=ranges.loopz0 + cos(r*pi2)*dz/pi2;
                    nv=ranges.loopz0 + sin(r*pi2)*dz/pi2;
                    val=m.get(nx,ny,nz,nw,nu,nv);
                }
                break;
                default:
                    break;
                }
                a.set(x,y,z,val);
            }
        }
    }
}


/// VM Interface : New Interface

extern bool DebugVM;

void mapArray2DNoZ(SChunk& chunk)
{
    if (chunk.finished_)
        return;

    static float pi2=3.141592f*2.f;

    unsigned iid = chunk.instruct_;

    // create an executor for the chunk
    CNoiseExecutor nexec(*chunk.kernel_, SNAPGENSLOT, chunk.chunkid_);
    chunk.start(nexec);

    float* pbuffer = chunk.buffer_;

    const SMappingRanges& range = chunk.info_.range_;
    const float& mapx0 = range.mapx0;
    const float& mapy0 = range.mapy0;
    const float dmapx = (range.mapx1 - range.mapx0) / (float)chunk.info_.width_;
    const float dmapy = (range.mapy1 - range.mapy0) / (float)chunk.info_.height_;

    const unsigned w  = chunk.info_.width_;
    const unsigned h  = chunk.chunksize_;
    const int yoffset = chunk.chunkstart_;

    unsigned x, y;
    CCoordinate coord;
    float val;

    DebugVM = true;

    switch(chunk.info_.smode_)
    {
    case SEAMLESS_NONE:
    {
        coord.dimension_ = 2;
        for(y=0; y < h; ++y)
        {
            coord.y_ = mapy0 + ((float)(y+yoffset) * dmapy);
            for(x=0; x < w; ++x)
            {
#ifdef USE_CACHESTAT
                DebugVM = ((y % 32 == 0 || y == h-1) && (x == 0 || x == w/2 || x == w-1));
#endif
                coord.x_ = mapx0 + (float)x * dmapx;
                val = nexec.EvaluateAt(iid, coord).outfloat_;
                *pbuffer = val;
                pbuffer++;
#ifdef USE_CACHESTAT
                if (DebugVM) printf("mapArray2DNoZ:  instruct=%u x=%u y=%u value=%f \n", iid, x, y, val);
#endif
                nexec.NextCacheAccessFastIndex();
            }
        }
    }
    break;
    case SEAMLESS_X:
    {
        coord.dimension_ = 3;
        const float loopx0 = range.loopx0;
        const float dloopx = range.loopx1 - loopx0;
        float v;
        for(y=0; y < h; ++y)
        {
            coord.z_ = mapy0 + ((float)(y+yoffset) * dmapy);
            for(x=0; x < w; ++x)
            {
                v = (float)x * dmapx / dloopx;
                coord.x_ = loopx0 + cos(v*pi2) * dloopx/pi2;
                coord.y_ = loopx0 + sin(v*pi2) * dloopx/pi2;
                val = nexec.EvaluateAt(iid, coord).outfloat_;
                *pbuffer = val;
                pbuffer++;
                nexec.NextCacheAccessFastIndex();
            }
        }
    }
    break;
    case SEAMLESS_Y:
    {
        coord.dimension_ = 3;
        const float loopy0 = range.loopy0;
        const float dloopy = range.loopy1 - loopy0;
        float v;
        for(y=0; y < h; ++y)
        {
            v = (float)(y+yoffset) * dmapy / dloopy;
            coord.y_ = loopy0 + cos(v*pi2) * dloopy/pi2;
            coord.z_ = loopy0 + sin(v*pi2) * dloopy/pi2;
            for(x=0; x < w; ++x)
            {
                coord.x_ = mapx0 + ((float)x * dmapx);
                val = nexec.EvaluateAt(iid, coord).outfloat_;
                *pbuffer = val;
                pbuffer++;
                nexec.NextCacheAccessFastIndex();
            }
        }
    }
    break;
    case SEAMLESS_XY:
    {
        coord.dimension_ = 4;
        const float loopx0 = range.loopx0;
        const float dloopx = range.loopx1 - loopx0;
        const float loopy0 = range.loopy0;
        const float dloopy = range.loopy1 - loopy0;
        float v;
        for(y=0; y < h; ++y)
        {
            v = (float)(y+yoffset) * dmapy / dloopy;
            coord.z_ = loopy0 + cos(v*pi2) * dloopy/pi2;
            coord.w_ = loopy0 + sin(v*pi2) * dloopy/pi2;
            for(x=0; x < w; ++x)
            {
                v = (float)x * dmapx / dloopx;
                coord.x_ = loopx0 + cos(v*pi2) * dloopx/pi2;
                coord.y_ = loopx0 + sin(v*pi2) * dloopx/pi2;
                val = nexec.EvaluateAt(iid, coord).outfloat_;
                *pbuffer = val;
                pbuffer++;
                nexec.NextCacheAccessFastIndex();
            }
        }
    }
    break;

    default:
        break;
    }

    // after first module set cache available
    nexec.SetCacheAvailable(iid);

    URHO3D_LOGDEBUGF("mapArray2DNoZ : chunkid=%u iid=%u ... finished !", chunk.chunkid_, chunk.instruct_);

    chunk.finished_ = true;
}

void mapArray2D(SChunk& chunk)
{
    /// TODO
    mapArray2DNoZ(chunk);
}

void mapArray2Drgba(SChunk& chunk)
{
    /// TODO
    mapArray2DNoZ(chunk);
}

void mapArray2DNoZrgba(SChunk& chunk)
{
    /// TODO
    mapArray2DNoZ(chunk);
}

void mapArray3D(SChunk& chunk)
{
    /// TODO
    mapArray2DNoZ(chunk);
}

void mapArray3Drgba(SChunk& chunk)
{
    /// TODO
    mapArray2DNoZ(chunk);
}

extern int DebugV2;

void mapArray2DNoZ_V2(SChunk& chunk)
{
    static float pi2=3.141592f*2.f;

    unsigned iid = chunk.instruct_;

    // create an executor for the chunk
    Evaluator nexec(*chunk.kernelV2_, SNAPGENSLOT, chunk.chunkid_);
    chunk.start(nexec);

    float* pbuffer = chunk.buffer_;

    const SMappingRanges& range = chunk.info_.range_;
    const float& mapx0 = range.mapx0;
    const float& mapy0 = range.mapy0;
    const float dmapx = (range.mapx1 - range.mapx0) / (float)chunk.info_.width_;
    const float dmapy = (range.mapy1 - range.mapy0) / (float)chunk.info_.height_;

    const unsigned w  = chunk.info_.width_;
    const unsigned h  = chunk.chunksize_;
    const int yoffset = chunk.chunkstart_;

    unsigned x, y;
    CCoordinate coord;
    float val;

    DebugV2 = 0;

    switch(chunk.info_.smode_)
    {
    case SEAMLESS_NONE:
    {
        coord.dimension_ = 2;
        for(y=0; y < h; ++y)
        {
            coord.y_ = mapy0 + ((float)(y+yoffset) * dmapy);
            for(x=0; x < w; ++x)
            {
                coord.x_ = mapx0 + (float)x * dmapx;
                val = nexec.EvaluateAt(coord);
                *pbuffer = val;
                pbuffer++;
//                printf("mapArray2DNoZ_V2: (x=%u y=%u) => instruct=%u iaccessor=%ld val=%f \n", x, y, iid, nexec.GetCacheAccessIndex(), val);
                nexec.NextCacheAccessFastIndex();
            }
#ifdef USE_CACHESTAT
//                if (y % 32 == 0 || y == h-1)
//                    printf("mapArray2DNoZ_V2: y=%u => instruct=%u iaccessor=%ld \n", y, iid, nexec.GetCacheAccessIndex());
#endif
        }
    }
    break;
    case SEAMLESS_X:
    {
        coord.dimension_ = 3;
        const float loopx0 = range.loopx0;
        const float dloopx = range.loopx1 - loopx0;
        float v;
        for(y=0; y < h; ++y)
        {
            coord.z_ = mapy0 + ((float)(y+yoffset) * dmapy);
            for(x=0; x < w; ++x)
            {
                v = (float)x * dmapx / dloopx;
                coord.x_ = loopx0 + cos(v*pi2) * dloopx/pi2;
                coord.y_ = loopx0 + sin(v*pi2) * dloopx/pi2;
                val = nexec.EvaluateAt(coord);
                *pbuffer = val;
                pbuffer++;
                nexec.NextCacheAccessFastIndex();
            }
        }
    }
    break;
    case SEAMLESS_Y:
    {
        coord.dimension_ = 3;
        const float loopy0 = range.loopy0;
        const float dloopy = range.loopy1 - loopy0;
        float v;
        for(y=0; y < h; ++y)
        {
            v = (float)(y+yoffset) * dmapy / dloopy;
            coord.y_ = loopy0 + cos(v*pi2) * dloopy/pi2;
            coord.z_ = loopy0 + sin(v*pi2) * dloopy/pi2;
            for(x=0; x < w; ++x)
            {
                coord.x_ = mapx0 + ((float)x * dmapx);
                val = nexec.EvaluateAt(coord);
                *pbuffer = val;
                pbuffer++;
                nexec.NextCacheAccessFastIndex();
            }
        }
    }
    break;
    case SEAMLESS_XY:
    {
        coord.dimension_ = 4;
        const float loopx0 = range.loopx0;
        const float dloopx = range.loopx1 - loopx0;
        const float loopy0 = range.loopy0;
        const float dloopy = range.loopy1 - loopy0;
        float v;
        for(y=0; y < h; ++y)
        {
            v = (float)(y+yoffset) * dmapy / dloopy;
            coord.z_ = loopy0 + cos(v*pi2) * dloopy/pi2;
            coord.w_ = loopy0 + sin(v*pi2) * dloopy/pi2;
            for(x=0; x < w; ++x)
            {
                v = (float)x * dmapx / dloopx;
                coord.x_ = loopx0 + cos(v*pi2) * dloopx/pi2;
                coord.y_ = loopx0 + sin(v*pi2) * dloopx/pi2;
                val = nexec.EvaluateAt(coord);
                *pbuffer = val;
                pbuffer++;
                nexec.NextCacheAccessFastIndex();
            }
        }
    }
    break;

    default:
        break;
    }

    // after first module set cache available
    nexec.SetCacheAvailable(iid);

    chunk.finished_ = true;
}

void mapArray2D_V2(SChunk& chunk)
{
    /// TODO
    mapArray2DNoZ_V2(chunk);
}

void mapArray2Drgba_V2(SChunk& chunk)
{
    /// TODO
    mapArray2DNoZ_V2(chunk);
}

void mapArray2DNoZrgba_V2(SChunk& chunk)
{
    /// TODO
    mapArray2DNoZ_V2(chunk);
}

void mapArray3D_V2(SChunk& chunk)
{
    /// TODO
    mapArray2DNoZ_V2(chunk);
}

void mapArray3Drgba_V2(SChunk& chunk)
{
    /// TODO
    mapArray2DNoZ_V2(chunk);
}


static std::vector<SChunk> sSnapShotChunks_;

bool isChunkWorkersFinished()
{
    if (!sSnapShotChunks_.size())
        return true;

    for (unsigned i=0; i < sSnapShotChunks_.size(); i++)
        if (!sSnapShotChunks_[i].finished_)
            return false;

    sSnapShotChunks_.clear();
    return true;
}

void StopChunkWorkers()
{
#if defined(IMAGING_USE_WORKQUEUE)
    Urho3D::WorkQueue* queue = GameContext::Get().gameWorkQueue_;

    queue->Pause();

    Vector<SharedPtr<WorkItem> > items;
    for (unsigned i=0; i < sSnapShotChunks_.size(); i++)
    {
        if (!sSnapShotChunks_[i].started_)
        {
            sSnapShotChunks_[i].finished_ = true;
            items.Push(sSnapShotChunks_[i].item_);
        }
    }

    if (items.Size())
        queue->RemoveWorkItems(items);

    queue->Resume();

    queue->Complete(ANL_IMAGING_WORKITEM_PRIORITY);
#endif
}

SChunk::SChunk(const SMappingInfo& info, CKernel& kernel, unsigned instruct, int chunkid, int size, int offset) :
    info_(info),
    kernel_(&kernel),
    kernelV2_(0),
    instruct_(instruct),
    chunkfunc_(0),
    chunkid_(chunkid),
    chunksize_(size),
    chunkstart_(offset),
    finished_(false),
    started_(false)
{
    // set chunk function
    switch ((SCHUNKTYPE)info.type_)
    {
    case SCHUNK2DNOZ:
        chunkfunc_ = mapArray2DNoZ;
        break;
    case SCHUNK2D:
        chunkfunc_ = mapArray2D;
        break;
    case SCHUNK3D:
        chunkfunc_ = mapArray3D;
        break;
    case SRGBACHUNK2DNOZ:
        chunkfunc_ = mapArray2DNoZrgba;
        break;
    case SRGBACHUNK2D:
        chunkfunc_ = mapArray2Drgba;
        break;
    case SRGBACHUNK3D:
        chunkfunc_ = mapArray3Drgba;
        break;
    }
}

SChunk::SChunk(const SMappingInfo& info, InstructionList& kernel, unsigned instruct, int chunkid, int size, int offset) :
    info_(info),
    kernel_(0),
    kernelV2_(&kernel),
    instruct_(instruct),
    chunkfunc_(0),
    chunkid_(chunkid),
    chunksize_(size),
    chunkstart_(offset),
    finished_(false),
    started_(false)
{
    // set chunk function
    switch ((SCHUNKTYPE)info.type_)
    {
    case SCHUNK2DNOZ:
        chunkfunc_ = mapArray2DNoZ_V2;
        break;
    case SCHUNK2D:
        chunkfunc_ = mapArray2D_V2;
        break;
    case SCHUNK3D:
        chunkfunc_ = mapArray3D_V2;
        break;
    case SRGBACHUNK2DNOZ:
        chunkfunc_ = mapArray2DNoZrgba_V2;
        break;
    case SRGBACHUNK2D:
        chunkfunc_ = mapArray2Drgba_V2;
        break;
    case SRGBACHUNK3D:
        chunkfunc_ = mapArray3Drgba_V2;
        break;
    }
}

SChunk::SChunk(const SChunk& c) :
    info_(c.info_),
    kernel_(c.kernel_),
    instruct_(c.instruct_),
    chunkfunc_(c.chunkfunc_),
    chunkid_(c.chunkid_),
    chunksize_(c.chunksize_),
    chunkstart_(c.chunkstart_),
    finished_(false),
    started_(false)
{ }

void SChunk::start(CNoiseExecutor& nexec)
{
    {
        long address = chunkstart_*info_.width_;

        if (info_.type_ == SCHUNK3D || info_.type_ == SRGBACHUNK3D) address *= info_.depth_;
        if (info_.type_ > SCHUNK3D) address *= 4;

        buffer_ = info_.buffer_ + address;
    }

    long startindex = chunkstart_ * info_.width_;
    long endindex = startindex + chunksize_ * info_.width_ -1;

    nexec.SetCacheAccessor(SNAPGENSLOT, startindex, endindex);
//    printf("chunkid=%d startindex=%ld endindex=%ld \n", chunkid_, startindex, endindex);

    started_ = true;
    nexec.StartEvaluation();
}

void SChunk::start(Evaluator& nexec)
{
    {
        long address = chunkstart_*info_.width_;

        if (info_.type_ == SCHUNK3D || info_.type_ == SRGBACHUNK3D) address *= info_.depth_;
        if (info_.type_ > SCHUNK3D) address *= 4;

        buffer_ = info_.buffer_ + address;
    }

    long startindex = chunkstart_ * info_.width_;
    long endindex = startindex + chunksize_ * info_.width_ -1;

    nexec.SetCacheAccessor(SNAPGENSLOT, startindex, endindex);
//    printf("chunkid=%d startindex=%ld endindex=%ld \n", chunkid_, startindex, endindex);

    started_ = true;
    nexec.StartEvaluation(kernelV2_->GetTree(instruct_));
}


void *mapChunkp(void *p)
{
    SChunk& chunk(*((SChunk*)p));
    (*chunk.chunkfunc_)(chunk);
    return NULL;
}

void SChunkThread(const WorkItem* item, unsigned threadIndex)
{
    SChunk& chunk(*((SChunk*)item->aux_));
    chunk.finished_ = false;
    (*chunk.chunkfunc_)(chunk);
    chunk.finished_ = true;
}

void maptobuffer(int numthreads, const SMappingInfo& mapping, CKernel &k, const CInstructionIndex& instruct)
{
    printf("mapbuffer instruction=%u ...\n", instruct.GetIndex());

    // set maximum number of threads
    if (numthreads)
    {
#if defined(IMAGING_USE_STDTHREAD)
        numthreads = std::min(std::thread::hardware_concurrency(), (unsigned)numthreads);
#elif defined(IMAGING_USE_WORKQUEUE)
        numthreads = Clamp(numthreads, 1, (int)GameContext::Get().gameWorkQueue_->GetNumThreads());
#endif
    }

    // no threads
    if (numthreads == 0)
    {
        printf("mapbuffer instruction=%u ... no thread ... \n", instruct.GetIndex());

        // resize the vm cache arrays
        CNoiseExecutor::ResizeCache(k, SNAPGENSLOT, mapping.width_*mapping.depth_, mapping.height_, 1);
        SChunk chunk(mapping, k, instruct.GetIndex(), 0, mapping.height_, 0);
        (*chunk.chunkfunc_)(chunk);

        printf("mapbuffer instruction=%u ... OK ! \n", instruct.GetIndex());
    }
#if defined(IMAGING_USE_STDTHREAD) || defined(IMAGING_USE_PTHREAD) || defined(IMAGING_USE_WORKQUEUE)
    else
    {
        // create the chunk datas for the threads
        sSnapShotChunks_.clear();

#if defined(IMAGING_USE_WORKQUEUE)
        Urho3D::WorkQueue* queue = GameContext::Get().gameWorkQueue_;
        queue->Pause();

        const int MAXCHUNKSIZE = 2048;
        unsigned numchunks;
        int chunkheight;
        int size = mapping.height_ * mapping.width_;
        if (size > MAXCHUNKSIZE)
        {
            numchunks = size / MAXCHUNKSIZE;
            chunkheight = mapping.height_ / numchunks;
            if (size % MAXCHUNKSIZE != 0)
                numchunks++;
        }
        else
        {
            numchunks = 1;
            chunkheight = mapping.height_;
        }
#else
        int chunkheight = std::floor(mapping.height_ / numthreads);
        unsigned numchunks = numthreads;
#endif

        CNoiseExecutor::ResizeCache(k, SNAPGENSLOT, mapping.width_*mapping.depth_, mapping.height_, numchunks);

        for (int  i=0; i < numchunks-1; ++i)
            sSnapShotChunks_.push_back(SChunk(mapping, k, instruct.GetIndex(), i, chunkheight, i*chunkheight));
        sSnapShotChunks_.push_back(SChunk(mapping, k, instruct.GetIndex(), numchunks-1, mapping.height_-(numchunks-1)*chunkheight, (numchunks-1)*chunkheight));

        // create the threads
#if defined(IMAGING_USE_STDTHREAD)
        std::vector<std::thread> threads;
        for(int i=0; i < numthreads; ++i)
            threads.push_back(std::thread(sSnapShotChunks_[i].chunkfunc_, std::ref(sSnapShotChunks_[i])));
#elif defined(IMAGING_USE_PTHREAD)
        std::vector<pthread_t> threads(numthreads);
        for (int  i=0; i < numthreads; ++i)
            pthread_create(&threads[i], NULL, &mapChunkp, &sSnapShotChunks_[i]);
#else
        for (int i = 0; i < numchunks; ++i)
        {
            SharedPtr<WorkItem> item = queue->GetFreeItem();
            item->priority_ = ANL_IMAGING_WORKITEM_PRIORITY;
            item->workFunction_ = &SChunkThread;
            item->aux_ = &sSnapShotChunks_[i];
            sSnapShotChunks_[i].item_ = item;
            queue->AddWorkItem(item);
        }
#endif

        printf("mapbuffer start threads for instruction=%u ...\n", instruct.GetIndex());

        // launch the threads
#if defined(IMAGING_USE_STDTHREAD)
        for(int i=0; i < threads.size(); ++i)
            threads[i].join();
#elif defined(IMAGING_USE_PTHREAD)
        for(int i=0; i < threads.size(); ++i)
            pthread_join(threads[i], NULL);
#else
        queue->Resume();
#endif
    }

#endif
}

void maptobuffer(int numthreads, CArray2Dd &buffer, CKernel &k, const CInstructionIndex& instruct, int seamlessmode, const SMappingRanges& ranges, float z, bool noz)
{
    SMappingInfo mappinginfo(noz ? SCHUNK2DNOZ : SCHUNK2D, buffer.getData(), buffer.width(), buffer.height(), 0, seamlessmode, ranges, z);
    maptobuffer(numthreads, mappinginfo, k, instruct);
}

void maptobuffer(int numthreads, CArray3Dd &buffer, CKernel &k, const CInstructionIndex& instruct, int seamlessmode, const SMappingRanges& ranges, float z)
{
    SMappingInfo mappinginfo(SCHUNK3D, buffer.getData(), buffer.width(), buffer.height(), buffer.depth(), seamlessmode, ranges, z);
    maptobuffer(numthreads, mappinginfo, k, instruct);
}

void maptobuffer(int numthreads, CArray2Drgba &buffer, CKernel &k, const CInstructionIndex& instruct, int seamlessmode, const SMappingRanges& ranges, float z, bool noz)
{
    SMappingInfo mappinginfo(noz ? SRGBACHUNK2DNOZ : SRGBACHUNK2D, (float*)buffer.getData(), buffer.width(), buffer.height(), 0, seamlessmode, ranges, z);
    maptobuffer(numthreads, mappinginfo, k, instruct);
}

void maptobuffer(int numthreads, CArray3Drgba &buffer, CKernel &k, const CInstructionIndex& instruct, int seamlessmode, const SMappingRanges& ranges, float z)
{
    SMappingInfo mappinginfo(SRGBACHUNK3D, (float*)buffer.getData(), buffer.width(), buffer.height(), buffer.depth(), seamlessmode, ranges, z);
    maptobuffer(numthreads, mappinginfo, k, instruct);
}


void maptobufferV2(int numthreads, const SMappingInfo& mapping, InstructionList &k, unsigned instruct)
{
    printf("maptobufferV2 instruction=%u ...\n", instruct);

    int chunksize = mapping.height_;

    // set number of threads
#if defined(IMAGING_USE_STDTHREAD)
    numthreads = std::min(std::thread::hardware_concurrency(), (unsigned)numthreads);
    if (numthreads > 1)
        chunksize = std::floor(mapping.height_ / numthreads);
#endif

    printf("numthreads = %d \n", numthreads);

    // resize the vm cache arrays
    Evaluator::ResizeCache(k, SNAPGENSLOT, mapping.width_*mapping.depth_, mapping.height_, numthreads);

    // no threads
    if (numthreads < 2)
    {
        SChunk chunk(mapping, k, instruct, 0, chunksize, 0);
        (*chunk.chunkfunc_)(chunk);
    }
#if defined(IMAGING_USE_STDTHREAD) || defined(IMAGING_USE_PTHREAD)
    else
    {
        // create the chunk datas for the threads
        sSnapShotChunks_.clear();
        for (int  i=0; i < numthreads-1; ++i)
            sSnapShotChunks_.push_back(SChunk(mapping, k, instruct, i, chunksize, i*chunksize));
        sSnapShotChunks_.push_back(SChunk(mapping, k, instruct, numthreads-1, mapping.height_-(numthreads-1)*chunksize, (numthreads-1)*chunksize));

        // create the threads
#if defined(IMAGING_USE_STDTHREAD)
        std::vector<std::thread> threads;
        for(int i=0; i < numthreads; ++i)
            threads.push_back(std::thread(sSnapShotChunks_[i].chunkfunc_, std::ref(sSnapShotChunks_[i])));
#else
        std::vector<pthread_t> threads(numthreads);
        for (int  i=0; i < numthreads; ++i)
            pthread_create(threads.data()+i, NULL, &mapChunkp, sSnapShotChunks_.data()+i);
#endif

        printf("start threads ...\n");
        // launch the threads
#if defined(IMAGING_USE_STDTHREAD)
        for(int i=0; i < threads.size(); ++i)
            threads[i].join();
#else
        for(int i=0; i < threads.size(); ++i)
            pthread_join(threads[i], NULL);
#endif
        printf("... end threads ! \n");
    }
#endif

    printf("maptobufferV2 instruction=%u ... OK ! \n", instruct);
}

void maptobufferV2(int numthreads, CArray2Dd& buffer, InstructionList& k, unsigned instruct, int seamlessmode, const SMappingRanges& ranges, float z, bool noz)
{
    SMappingInfo mappinginfo(noz ? SCHUNK2DNOZ : SCHUNK2D, buffer.getData(), buffer.width(), buffer.height(), 0, seamlessmode, ranges, z);
    maptobufferV2(numthreads, mappinginfo, k, instruct);
}


/// Serializing

void savefloatArray(std::string filename, CArray2Dd *array)
{
    if(!array) return;
    int width=array->width();
    int height=array->height();

    unsigned char *data=new unsigned char[width*height*4];
    for(int x=0; x<width; ++x)
    {
        for(int y=0; y<height; ++y)
        {
            unsigned char *c=&data[y*width*4+x*4];
            float v=array->get(x,y);
            c[0]=c[1]=c[2]=(unsigned char)(v*255.0);
            c[3]=255;
        }
    }

    if(StringToUpper(filename.substr(filename.size()-3, std::string::npos)) == "TGA")
    {
        stbi_write_tga(filename.c_str(), width, height, 4, data);
    }
    else
    {
        stbi_write_png(filename.c_str(), width, height, 4, data, width*4);
    }

    delete[] data;
}

void saveRGBAArray(std::string filename, CArray2Drgba *array)
{
    if(!array) return;
    int width=array->width();
    int height=array->height();

    unsigned char *data=new unsigned char[width*height*4];
    for(int x=0; x<width; ++x)
    {
        for(int y=0; y<height; ++y)
        {
            unsigned char *c=&data[y*width*4+x*4];
            SRGBA color=array->get(x,y);
            c[0]=(unsigned char)(color.r*255.0);
            c[1]=(unsigned char)(color.g*255.0);
            c[2]=(unsigned char)(color.b*255.0);
            c[3]=(unsigned char)(color.a*255.0);
        }
    }

    if(StringToUpper(filename.substr(filename.size()-3, std::string::npos)) == "TGA")
    {
        stbi_write_tga(filename.c_str(), width, height, 4, data);
    }
    else
    {
        stbi_write_png(filename.c_str(), width, height, 4, data, width*4);
    }
    delete[] data;
}

void loadfloatArray(std::string filename, CArray2Dd *array)
{
    if(!array) return;
    int w,h,n;
    unsigned char *data=stbi_load(filename.c_str(), &w, &h, &n, 4);
    if(!data) return;

    array->resize(w,h);
    for(int x=0; x<w; ++x)
    {
        for(int y=0; y<h; ++y)
        {
            unsigned char *a=&data[y*w*4+x*4];
            array->set(x,y,(float)(a[0])/255.0);
        }
    }

    stbi_image_free(data);
}

void loadRGBAArray(std::string filename, CArray2Drgba *array)
{
    if(!array) return;
    int w,h,n;
    unsigned char *data=stbi_load(filename.c_str(), &w, &h, &n, 4);
    if(!data) return;

    array->resize(w,h);
    for(int x=0; x<w; ++x)
    {
        for(int y=0; y<h; ++y)
        {
            unsigned char *a=&data[y*w*4+x*4];
            SRGBA color((float)a[0]/255.0, (float)a[1]/255.0, (float)a[2]/255.0, (float)a[3]/255.0);
            array->set(x,y,color);
        }
    }

    stbi_image_free(data);
}

void saveHeightmap(std::string filename, CArray2Dd *array)
{
    if(!array) return;
    int width=array->width();
    int height=array->height();

    unsigned char *data=new unsigned char[width*height*4];
    for(int x=0; x<width; ++x)
    {
        for(int y=0; y<height; ++y)
        {
            unsigned char *c=&data[y*width*4+x*4];
            float v=array->get(x,y);
            float expht=std::floor(v*255.0);
            float rm=v*255.0-expht;
            float r=expht/255.0;
            float g=rm;
            c[0]=(unsigned char)(r*255.0);
            c[1]=(unsigned char)(g*255.0);
            c[2]=0;
            c[3]=255;
        }
    }

    if(StringToUpper(filename.substr(filename.size()-3, std::string::npos)) == "TGA")
    {
        stbi_write_tga(filename.c_str(), width, height, 4, data);
    }
    else
    {
        stbi_write_png(filename.c_str(), width, height, 4, data, width*4);
    }


    delete[] data;
}


/// Computing

void normalizeVec3(float v[3])
{
    float len=std::sqrt(v[0]*v[0]+v[1]*v[1]+v[2]*v[2]);
    v[0]/=len;
    v[1]/=len;
    v[2]/=len;
}

void calcNormalMap(CArray2Dd *map, CArray2Drgba *bump, float spacing, bool normalize, bool wrap)
{
    if(!map || !bump) return;
    int mw=map->width(), mh=map->height();
    if(mw!=bump->width() || mh!=bump->height()) bump->resize(mw,mh);

    for(int x=0; x<mw; ++x)
    {
        for(int y=0; y<mh; ++y)
        {
            float n[3]= {0.0, 1.0, 0.0};

            if(!wrap)
            {
                if(x==0 || y==0 || x==mw-1 || y==mh-1)
                {
                    n[0]=0.0;
                    n[2]=0.0;
                }
                else
                {
                    n[0]=(map->get(x-1,y)-map->get(x+1,y)) / spacing;
                    n[2]=(map->get(x,y-1)-map->get(x,y+1)) / spacing;
                }
                normalizeVec3(n);
            }
            else
            {
                int x1,x2,y1,y2;
                if(x==0) x1=mw-1;
                else x1=x-1;

                if(y==0) y1=mh-1;
                else y1=y-1;

                if(x==mw-1) x2=0;
                else x2=x+1;

                if(y==mh-1) y2=0;
                else y2=y+1;

                n[0]=(map->get(x1,y)-map->get(x2,y)) / spacing;
                n[2]=(map->get(x,y1)-map->get(x,y2)) / spacing;
                normalizeVec3(n);
            }
            if(normalize)
            {
                n[0]=n[0]*0.5 + 0.5;
                n[1]=n[1]*0.5 + 0.5;
                n[2]=n[2]*0.5 + 0.5;
            }
            bump->set(x,y,SRGBA((float)n[0], (float)n[2], (float)n[1], 1.0));
        }
    }
}

void calcBumpMap(CArray2Dd *map, CArray2Dd *bump, float light[3], float spacing, bool wrap)
{
    if(!map || !bump) return;
    int mw=map->width(), mh=map->height();
    if(mw!=bump->width() || mh!=bump->height()) bump->resize(mw,mh);
    normalizeVec3(light);

    for(int x=0; x<mw; ++x)
    {
        for(int y=0; y<mh; ++y)
        {
            float n[3]= {0.0, 1.0, 0.0};

            if(!wrap)
            {
                if(x==0 || y==0 || x==mw-1 || y==mh-1)
                {
                    n[0]=0.0;
                    n[2]=0.0;
                }
                else
                {
                    n[0]=(map->get(x-1,y)-map->get(x+1,y)) / spacing;
                    n[2]=(map->get(x,y-1)-map->get(x,y+1)) / spacing;
                }
                normalizeVec3(n);
            }
            else
            {
                int x1,x2,y1,y2;
                if(x==0) x1=mw-1;
                else x1=x-1;

                if(y==0) y1=mh-1;
                else y1=y-1;

                if(x==mw-1) x2=0;
                else x2=x+1;

                if(y==mh-1) y2=0;
                else y2=y+1;

                n[0]=(map->get(x1,y)-map->get(x2,y)) / spacing;
                n[2]=(map->get(x,y1)-map->get(x,y2)) / spacing;
                normalizeVec3(n);
            }
            float b = light[0]*n[0] + light[1]*n[1] + light[2]*n[2];
            if(b<0.0) b=0.0;
            if(b>1.0) b=1.0;
            bump->set(x,y,b);
        }
    }
}

/*
float highresTime()
{
    using namespace std::chrono;
    high_resolution_clock::time_point t=high_resolution_clock::now();
    high_resolution_clock::duration d=t.time_since_epoch();
    return (float)d.count() * (float)high_resolution_clock::period::num / (float)high_resolution_clock::period::den;
}
*/

};

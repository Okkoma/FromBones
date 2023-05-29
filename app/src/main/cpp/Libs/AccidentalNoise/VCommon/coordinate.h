#ifndef COORDINATE_H
#define COORDINATE_H

#include <algorithm>


namespace anl
{

struct CCoordinate
{
    CCoordinate()
    {
        set(0.f,0.f);
    }

    CCoordinate(float x, float y)
    {
        set(x,y);
    }

    CCoordinate(float x, float y, float z)
    {
        set(x,y,z);
    }

    CCoordinate(float x, float y, float z, float w)
    {
        set(x,y,z,w);
    }

    CCoordinate(float x, float y, float z, float w, float u, float v)
    {
        set(x,y,z,w,u,v);
    }

//    CCoordinate(int dimension)
//    {
//        dimension_ = dimension;
//    }

    CCoordinate(int dimension, float val)
    {
        dimension_ = dimension;
        if (dimension == 2) x_ = y_ = val;
        else if (dimension == 3) x_= y_ = z_ = val;
        else if (dimension == 4) x_= y_ = z_ = w_ = val;
        else x_= y_ = z_ = w_ = u_ = v_ = val;
    }

    CCoordinate(const CCoordinate &c)
    {
        dimension_ = c.dimension_;

        if (dimension_ == 2)
        {
            x_ = c.x_;
            y_ = c.y_;
        }
        else if (dimension_ == 3)
        {
            x_ = c.x_;
            y_ = c.y_;
            z_ = c.z_;
        }
        else if (dimension_ == 4)
        {
            x_ = c.x_;
            y_ = c.y_;
            z_ = c.z_;
            w_ = c.w_;
        }
        else
        {
            x_ = c.x_;
            y_ = c.y_;
            z_ = c.z_;
            w_ = c.w_;
            u_ = c.u_;
            v_ = c.v_;
        }
    }

    void set(float x, float y)
    {
        dimension_=2;
        x_=x;
        y_=y;
        z_=0.f;
        w_=0.f;
        u_=0.f;
        v_=0.f;
    }

    void set(float x, float y, float z)
    {
        dimension_=3;
        x_=x;
        y_=y;
        z_=z;
        w_=0.f;
        u_=0.f;
        v_=0.f;
    }

    void set(float x, float y, float z, float w)
    {
        dimension_=4;
        x_=x;
        y_=y;
        z_=z;
        w_=w;
        u_=0.f;
        v_=0.f;
    }

    void set(float x, float y, float z, float w, float u, float v)
    {
        dimension_=6;
        x_=x;
        y_=y;
        z_=z;
        w_=w;
        u_=u;
        v_=v;
    }

    CCoordinate operator *(float rhs) const
    {
        CCoordinate ret;
        ret.dimension_=dimension_;
        ret.x_=x_*rhs;
        ret.y_=y_*rhs;
        ret.z_=z_*rhs;
        ret.w_=w_*rhs;
        ret.u_=u_*rhs;
        ret.v_=v_*rhs;
        return ret;
    }

    CCoordinate operator *(const CCoordinate &rhs) const
    {
        CCoordinate ret;
        ret.dimension_=dimension_;
        ret.x_=x_*rhs.x_;
        ret.y_=y_*rhs.y_;
        ret.z_=z_*rhs.z_;
        ret.w_=w_*rhs.w_;
        ret.u_=u_*rhs.u_;
        ret.v_=v_*rhs.v_;
        return ret;
    }

    CCoordinate operator +(const CCoordinate &rhs) const
    {
        CCoordinate ret;
        ret.dimension_=dimension_;
        ret.x_=x_+rhs.x_;
        ret.y_=y_+rhs.y_;
        ret.z_=z_+rhs.z_;
        ret.w_=w_+rhs.w_;
        ret.u_=u_+rhs.u_;
        ret.v_=v_+rhs.v_;
        return ret;
    }

    CCoordinate& operator *=(float rhs)
    {
        if (dimension_ == 2)
        {
            x_ *= rhs;
            y_ *= rhs;
        }
        else if (dimension_ == 3)
        {
            x_ *= rhs;
            y_ *= rhs;
            z_ *= rhs;
        }
        else if (dimension_ == 4)
        {
            x_ *= rhs;
            y_ *= rhs;
            z_ *= rhs;
            w_ *= rhs;
        }
        else
        {
            x_ *= rhs;
            y_ *= rhs;
            z_ *= rhs;
            w_ *= rhs;
            u_ *= rhs;
            v_ *= rhs;
        }
        return *this;
    }

    CCoordinate& operator +=(const CCoordinate& rhs)
    {
        dimension_ = std::min(dimension_, rhs.dimension_);

        if (dimension_ == 2)
        {
            x_ += rhs.x_;
            y_ += rhs.y_;
        }
        else if (dimension_ == 3)
        {
            x_ += rhs.x_;
            y_ += rhs.y_;
            z_ += rhs.z_;
        }
        else if (dimension_ == 4)
        {
            x_ += rhs.x_;
            y_ += rhs.y_;
            z_ += rhs.z_;
            w_ += rhs.w_;
        }
        else
        {
            x_ += rhs.x_;
            y_ += rhs.y_;
            z_ += rhs.z_;
            w_ += rhs.w_;
            u_ += rhs.u_;
            v_ += rhs.v_;
        }
        return *this;
    }
    CCoordinate& operator *=(const CCoordinate& rhs)
    {
        dimension_ = std::min(dimension_, rhs.dimension_);

        if (dimension_ == 2)
        {
            x_ *= rhs.x_;
            y_ *= rhs.y_;
        }
        else if (dimension_ == 3)
        {
            x_ *= rhs.x_;
            y_ *= rhs.y_;
            z_ *= rhs.z_;
        }
        else if (dimension_ == 4)
        {
            x_ *= rhs.x_;
            y_ *= rhs.y_;
            z_ *= rhs.z_;
            w_ *= rhs.w_;
        }
        else
        {
            x_ *= rhs.x_;
            y_ *= rhs.y_;
            z_ *= rhs.z_;
            w_ *= rhs.w_;
            u_ *= rhs.u_;
            v_ *= rhs.v_;
        }
        return *this;
    }

    CCoordinate& operator =(const CCoordinate &rhs)
    {
        dimension_ = rhs.dimension_;

        if (dimension_ == 2)
        {
            x_ = rhs.x_;
            y_ = rhs.y_;
        }
        else if (dimension_ == 3)
        {
            x_ = rhs.x_;
            y_ = rhs.y_;
            z_ = rhs.z_;
        }
        else if (dimension_ == 4)
        {
            x_ = rhs.x_;
            y_ = rhs.y_;
            z_ = rhs.z_;
            w_ = rhs.w_;
        }
        else
        {
            x_ = rhs.x_;
            y_ = rhs.y_;
            z_ = rhs.z_;
            w_ = rhs.w_;
            u_ = rhs.u_;
            v_ = rhs.v_;
        }

        return *this;
    }

    bool operator ==(const CCoordinate &c)
    {
        switch(dimension_)
        {
        case 2:
            return x_==c.x_ && y_==c.y_;
            break;
        case 3:
            return x_==c.x_ && y_==c.y_ && z_==c.z_;
            break;
        case 4:
            return x_==c.x_ && y_==c.y_ && z_==c.z_ && w_==c.w_;
            break;
        default:
            return x_==c.x_ && y_==c.y_ && z_==c.z_ && w_==c.w_ && u_==c.u_ && v_==c.v_;
            break;
        };
    }

    float x_, y_, z_, w_, u_, v_;
    int dimension_;

    static const CCoordinate EMPTY;
};

};

#endif

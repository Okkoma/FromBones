#ifndef RGBA_CONSTANT_H
#define RGBA_CONSTANT_H
#include "rgbamodulebase.h"

namespace anl
{
class CRGBAConstant : public CRGBAModuleBase
{
public:
    CRGBAConstant();
    CRGBAConstant(ANLV1_SRGBA &r);
    CRGBAConstant(float r, float g, float b, float a);
    ~CRGBAConstant();

    void set(float r, float g, float b, float a);
    void set(ANLV1_SRGBA &r);

    ANLV1_SRGBA get(float x, float y);
    ANLV1_SRGBA get(float x, float y, float z);
    ANLV1_SRGBA get(float x, float y, float z, float w);
    ANLV1_SRGBA get(float x, float y, float z, float w, float u, float v);

protected:
    ANLV1_SRGBA m_rgba;
};
};

#endif

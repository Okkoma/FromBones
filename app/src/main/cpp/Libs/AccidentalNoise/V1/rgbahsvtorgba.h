#ifndef RGBA_HSVTORGBA_H
#define RGBA_HSVTORGBA_H
#include "rgbamodulebase.h"

namespace anl
{
class CRGBAHSVToRGBA :public CRGBAModuleBase
{
public:
    CRGBAHSVToRGBA();

    void setSource(CRGBAModuleBase *m);
    void setSource(float r, float g, float b, float a);

    ANLV1_SRGBA get(float x, float y);
    ANLV1_SRGBA get(float x, float y, float z);
    ANLV1_SRGBA get(float x, float y, float z, float w);
    ANLV1_SRGBA get(float x, float y, float z, float w, float u, float v);

protected:
    CRGBAParameter m_source;
};
};

#endif


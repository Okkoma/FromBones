#ifndef RGBA_COMPOSE_CHANNELS_H
#define RGBA_COMPOSE_CHANNELS_H
#include "rgbamodulebase.h"
#include "implicitmodulebase.h"

namespace anl
{
enum CompositeChannelsMode
{
    RGB,
    HSV
};

class CRGBACompositeChannels : public CRGBAModuleBase
{
public:
    CRGBACompositeChannels();
    CRGBACompositeChannels(int mode);
    ~CRGBACompositeChannels();

    void setMode(int mode)
    {
        m_mode=mode;
    }

    void setRedSource(CImplicitModuleBase *m);
    void setGreenSource(CImplicitModuleBase *m);
    void setBlueSource(CImplicitModuleBase *m);
    void setHueSource(CImplicitModuleBase *m);
    void setSatSource(CImplicitModuleBase *m);
    void setValSource(CImplicitModuleBase *m);
    void setAlphaSource(CImplicitModuleBase *m);

    void setRedSource(float r);
    void setGreenSource(float g);
    void setBlueSource(float b);
    void setAlphaSource(float a);
    void setHueSource(float h);
    void setSatSource(float s);
    void setValSource(float v);

    ANLV1_SRGBA get(float x, float y);
    ANLV1_SRGBA get(float x, float y, float z);
    ANLV1_SRGBA get(float x, float y, float z, float w);
    ANLV1_SRGBA get(float x, float y, float z, float w, float u, float v);

protected:
    CScalarParameter m_c1, m_c2, m_c3, m_c4;
    int m_mode;
};
};

#endif

#ifndef IMPLICIT_EXTRACTRGBACHANNEL_H
#define IMPLICIT_EXTRACTRGBACHANNEL_H
#include "implicitmodulebase.h"
#include "rgbamodulebase.h"

namespace anl
{
enum EExtractChannel
{
    RED,
    GREEN,
    BLUE,
    ALPHA
};

class CImplicitExtractRGBAChannel : public CImplicitModuleBase
{
public:
    CImplicitExtractRGBAChannel();
    CImplicitExtractRGBAChannel(int channel);

    void setSource(CRGBAModuleBase *m);
    void setSource(float r, float g, float b, float a);

    void setChannel(int channel);

    float get(float x, float y);
    float get(float x, float y, float z);
    float get(float x, float y, float z, float w);
    float get(float x, float y, float z, float w, float u, float v);

protected:
    CRGBAParameter m_source;
    int m_channel;
};
};

#endif

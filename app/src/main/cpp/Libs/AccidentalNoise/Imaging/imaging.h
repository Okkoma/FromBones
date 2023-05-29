#ifndef IMAGING_H
#define IMAGING_H

#define IMAGING_NUMTHREADS 2

#include "../VM/vm.h"
#include "mapping.h"

#include <Urho3D/Core/WorkQueue.h>

namespace anl
{

const unsigned ANL_IMAGING_WORKITEM_PRIORITY = 999U;

struct SMappingInfo
{
    SMappingInfo() { }
    SMappingInfo(int type, float* buffer, int width, int height, int depth, int smode, const SMappingRanges& range, float z=0.f) :
        type_(type),
        smode_(smode),
        width_(width),
        height_(height),
        depth_(depth ? depth:1),
        range_(range),
        z_(z),
        buffer_(buffer) { }
    SMappingInfo(const SMappingInfo& i) :
        type_(i.type_),
        smode_(i.smode_),
        width_(i.width_),
        height_(i.height_),
        depth_(i.depth_),
        range_(i.range_),
        z_(i.z_),
        buffer_(i.buffer_) { }

    int type_, smode_;
    int width_, height_, depth_;

    SMappingRanges range_;
    float z_;
    float* buffer_;
};

struct SChunk;

typedef void (*mapChunkFunc)(SChunk&);

enum SCHUNKTYPE
{
    SCHUNK2DNOZ = 0,
    SCHUNK2D,
    SCHUNK3D,
    SRGBACHUNK2DNOZ,
    SRGBACHUNK2D,
    SRGBACHUNK3D,
};

struct SChunk
{
    SChunk(const SMappingInfo& info, CKernel& kernel, unsigned instruct, int chunkid, int size, int offset);
    SChunk(const SMappingInfo& info, InstructionList& kernel, unsigned instruct, int chunkid, int size, int offset);
    SChunk(const SChunk& c);

    void start(CNoiseExecutor& nexec);
    void start(Evaluator& nexec);

    CKernel* kernel_;
    InstructionList* kernelV2_;
    SMappingInfo info_;
    unsigned instruct_;

    mapChunkFunc chunkfunc_;

    int chunkid_;
    int chunksize_;
    int chunkstart_;

    Urho3D::SharedPtr<Urho3D::WorkItem> item_;

    float *buffer_;

    bool started_, finished_;
};

bool isChunkWorkersFinished();
void StopChunkWorkers();

/// V1 Interface
void map2D(int seamlessmode, CArray2Dd &a, CImplicitModuleBase &m, SMappingRanges &ranges, float z);
void map2DNoZ(int seamlessmode, CArray2Dd &a, CImplicitModuleBase &m, SMappingRanges &ranges);
void map3D(int seamlessmode, CArray3Dd &a, CImplicitModuleBase &m, SMappingRanges &ranges);
void mapRGBA2D(int seamlessmode, CArray2Drgba &a, CRGBAModuleBase &m, SMappingRanges &ranges, float z);
void mapRGBA2DNoZ(int seamlessmode, CArray2Drgba &a, CRGBAModuleBase &m, SMappingRanges &ranges);
void mapRGBA3D(int seamlessmode, CArray3Drgba &a, CRGBAModuleBase &m, SMappingRanges &ranges);

/// VM Interface
void maptobuffer(int numthreads, CArray2Dd &buffer, CKernel &k, const CInstructionIndex& instruct, int seamlessmode, const SMappingRanges& ranges, float z=0.f, bool noz=true);
void maptobuffer(int numthreads, CArray3Dd &buffer, CKernel &k, const CInstructionIndex& instruct, int seamlessmode, const SMappingRanges& ranges, float z);
void maptobuffer(int numthreads, CArray2Drgba &buffer, CKernel &k, const CInstructionIndex& instruct, int seamlessmode, const SMappingRanges& ranges, float z=0.f, bool noz=true);
void maptobuffer(int numthreads, CArray3Drgba &buffer, CKernel &k, const CInstructionIndex& instruct, int seamlessmode, const SMappingRanges& ranges, float z);
void maptobufferV2(int numthreads, CArray2Dd& buffer, InstructionList& k, unsigned instruct, int seamlessmode, const SMappingRanges& ranges, float z=0.f, bool noz=true);

/// Serializing
void savefloatArray(std::string filename, CArray2Dd *array);
void saveRGBAArray(std::string filename, CArray2Drgba *array);
void loadfloatArray(std::string filename, CArray2Dd *array);
void loadRGBAArray(std::string filename, CArray2Drgba *array);
void saveHeightmap(std::string filename, CArray2Dd *array);

/// Computing
void calcNormalMap(CArray2Dd *map, CArray2Drgba *bump, float spacing, bool normalize, bool wrap);
void calcBumpMap(CArray2Dd *map, CArray2Dd *bump, float light[3], float spacing, bool wrap);
void multRGBAByfloat(CArray2Drgba *rgba, CArray2Dd *map);

//float highresTime();

};

#include "imaging.inl"

#endif

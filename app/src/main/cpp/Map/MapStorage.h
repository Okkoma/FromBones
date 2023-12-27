#pragma once

#include <Urho3D/Core/Thread.h>
#include <Urho3D/Core/Mutex.h>

#include "DefsCore.h"
#include "DefsMap.h"
#include "DefsWorld.h"

#define USE_LOADINGLISTS

namespace Urho3D
{
class TmxFile2D;
class FileSystem;
class VectorBuffer;
class XMLFile;
class RefCounted;
}

using namespace Urho3D;


class MapBase;
class Map;
class MapPool;
class MapCreator;
class MapStorage;


enum MapCreatingMode
{
    MCM_ASYNCHRONOUS = 0,
    MCM_INSTANT,
    MCM_SYNCHRONOUS
};


/// Background serializer of MapData.

const unsigned SERIALIZER_WORKITEM_PRIORITY = 1001U;

class SerializerWorkInfo : public Object
{
    URHO3D_OBJECT(SerializerWorkInfo, Object);

public:
    SerializerWorkInfo() :
        Object(GameContext::Get().context_),
        finished_(true),
        mapdata_(0),
        state_(MAPASYNC_NONE) { }

    SerializerWorkInfo(const SerializerWorkInfo& e) :
        Object(GameContext::Get().context_),
        finished_(e.finished_),
        mapdata_(e.mapdata_),
        state_(e.state_) { }

    ~SerializerWorkInfo() { }

    bool finished_;

    MapData* mapdata_;
    MapAsynState state_;
};

class MapSerializer : public Object
{
    URHO3D_OBJECT(MapSerializer, Object);

public:
    MapSerializer(Context* context);
    ~MapSerializer();

    void Start();
    void Stop();

    bool LoadMapData(MapData* mapdata, bool async=true);
    bool SaveMapData(MapData* mapdata, bool async=true);

    // Return amount of maps in the load queue.
    SerializerWorkInfo& GetFreeWorkInfo();
    unsigned GetNumQueuedMaps() const;
    bool IsInQueue(const ShortIntVector2& mpoint) const;
    bool IsRunning() const;

private:
    void HandleWorkItemComplete(StringHash eventType, VariantMap& eventData);

    bool run_;

    List<SerializerWorkInfo > workInfos_;
};

/// Maps Storage
class FROMBONES_API MapStorage : public Object
{
    URHO3D_OBJECT(MapStorage, Object);

public:
    static void InitTable(Context* context);
    static void DeInitTable(Context* context);

    MapStorage(Context* context);
    MapStorage(Context* context, const IntVector2& wPoint);
    ~MapStorage();

    void Clear();
    void SetMapSeed(unsigned seed);
    void SetBufferDirty(bool dirty);
    void SetMapBufferOffset(int offset);
    void SetNode(Node* node);
    void SetCreatingMode(MapCreatingMode mode) { creatingMode_ = (int)mode; }

    Map* InitializeMap(const ShortIntVector2& mPoint, HiresTimer* timer=0);
    void ForceMapToUnload(Map* map);
    bool UnloadMapAt(const ShortIntVector2& mPoint, HiresTimer* timer=0);

    bool LoadMaps(bool loadEntities, bool async=true);
    bool SaveMaps(bool saveEntities, bool async=true);

    int GetCreatingMode() const { return creatingMode_; }
    unsigned GetCurrentWorldIndex() const { return currentWorldIndex_; }
    const String& GetCurrentWorldName() const { return worldName_; }
    Map* GetMapAt(const ShortIntVector2& mPoint) const;
    Map* GetAvailableMapAt(const ShortIntVector2& mPoint) const;
    int GetNumMapsToLoad() const { return mapsToLoadInMemory_.Size(); }
    unsigned GetNumMaxMaps() const { return maxMapsInMemory_; }
    const Rect& GetBufferedAreaRect(int viewport) const { return bufferedAreaRect_[viewport]; }
    const HashMap<ShortIntVector2, Map* >& GetMapsInMemory() const { return mapsInMemory_; }
    unsigned GetMapSeed() const { return sseed_; }
    Node* GetNode() const { return node_; }
    const HashMap<ShortIntVector2, MapData* >& GetMapDatas() const { return storage_->mapDatas_; }

    bool IsInsideBufferedArea(const ShortIntVector2& mPoint) const;
    void UpdateBufferedArea();
    void UpdateAllMaps();
    bool UpdateMapsInMemory(HiresTimer* timer=0);

    void MarkMapDirty();

    void DumpMapsMemory() const;

    static unsigned RegisterWorldPath(Context* context, const String& shortPathName,
                                      const IntVector2& worldPoint, float tilewidth=WORLD_TILE_WIDTH,
                                      float tileheight=WORLD_TILE_HEIGHT, const String& tileset=World2DInfo::ATLASSETDEFAULT,
                                      const String& worldmodel=World2DInfo::WORLDMODELDEFAULT, World2DInfo* info=0);

    static const IntVector2& CheckWorld2DPoint(Context* context, const IntVector2& worldPoint, World2DInfo* info);

    static int GetWorldIndex(const IntVector2& worldPoint);
    static World2DInfo& GetWorld2DInfo(unsigned index) { return registeredWorld2DInfo_[index]; }
    static String& GetWorldName(unsigned index) { return registeredWorldName_[index]; }

    static Vector<World2DInfo>& GetAllWorld2DInfos() { return registeredWorld2DInfo_; }
    static const Vector<IntVector2>& GetAllWorldPoints() { return registeredWorldPoint_; }
    static MapStorage* Get() { return storage_; }
    static MapPool* GetPool() { return mapPool_; }
    static MapCreator* GetCreator() { return mapCreator_; }
    static MapSerializer* GetMapSerializer() { return mapSerializer_; }
    static TerrainAtlas* GetAtlas() { return atlas_; }
    static MapModel* GetMapModel(int model) { return &mapModels_[model]; }
    static MapData* GetMapDataAt(const ShortIntVector2& mpoint, bool createIfMissing = false);
    static bool RemoveMapDataAt(const ShortIntVector2& mpoint) { return storage_->mapDatas_.Erase(mpoint); }
    static const IntVector2& GetWorldPoint(int worldindex);
    static String GetMapFileName(const String& worldName, const ShortIntVector2& mPoint, const char* ext);
    static const String& GetWorldName(const IntVector2& worldPoint);
    static String GetWorldDirName(const String& worldName);
    static String GetWorldFileName(int worldindex);
    static bool MapFileExist(const String& worldName, const ShortIntVector2& mPoint, const char* ext);
    static void DeleteWorldFiles(const IntVector2& wPoint, const char* ext="*");
    static void SaveWorldFiles(const IntVector2& wPoint);
    static void LoadWorldFiles(const IntVector2& wPoint);
    static void CopyInitialWorldFiles(const IntVector2& wPoint);
    static void DumpRegisterWorldPath();

    static const String storagePrefixDir;
    static const String storagePrefixFile;
    static String storageDir_;

private:
    inline void PushMapToLoad(const ShortIntVector2& mPoint);

    SharedPtr<TmxFile2D> GetTmxFile(const String& worldName, const ShortIntVector2& mPoint);

    int width_;
    int height_;

    WeakPtr<Node> node_;

    // current World Name
    unsigned currentWorldIndex_;
    World2DInfo* currentWorld2DInfo_;
    String worldName_;
    String defaultAtlasSet_;

    unsigned maxMapsInMemory_;
    int bOffset_;
    bool bufferedAreaDirty_;

    IntRect wBounds_;
    ShortIntVector2 forcedMapToUnload_;

    HiresTimer timer_;

    unsigned sseed_;
    int creatingMode_;

    Vector<MapData> mapDatasPool_;
    HashMap<ShortIntVector2, MapData* > mapDatas_;

    Rect bufferedAreaRect_[MAX_VIEWPORTS];     // BufferArea Marker For DrawDebug
    Vector<BufferExpandInfo> bufferExpandInfos_;
    Vector<IntRect> bufferAreas_;

    HashMap<ShortIntVector2, Map* > mapsInMemory_;
#ifdef USE_LOADINGLISTS
    List<ShortIntVector2> mapsToLoadInMemory_;
    List<ShortIntVector2> mapsToUnloadFromMemory_;
#else
    Vector<ShortIntVector2> mapsToLoadInMemory_;
    Vector<ShortIntVector2> mapsToUnloadFromMemory_;
#endif

    static Vector<String> registeredWorldName_;
    static Vector<World2DInfo> registeredWorld2DInfo_;
    static Vector<IntVector2> registeredWorldPoint_;
    static WeakPtr<TerrainAtlas> atlas_;

    static Vector<MapModel> mapModels_;

    static MapStorage* storage_;
    static MapCreator* mapCreator_;
    static MapPool* mapPool_;
    static MapSerializer* mapSerializer_;

    long long& delayUpdateUsec_;
};

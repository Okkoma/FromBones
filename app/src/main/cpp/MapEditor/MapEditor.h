#pragma once

#include "GameOptions.h"

#ifdef ACTIVE_CREATEMODE

namespace Urho3D
{
class Scene;
}

using namespace Urho3D;

class MapEditor : public Object
{
    URHO3D_OBJECT(MapEditor, Object);

public:
    MapEditor(Context* context);
    ~MapEditor();

    void Start();
    void Stop();

    static void Toggle();
    static void Release();
    static MapEditor* Get()
    {
        return mapEditor_;
    }

private:
    bool LoadLibrary();
    bool UnloadLibrary();

    void SubscribeToEvents();

    void HandleScriptReloadStarted(StringHash eventType, VariantMap& eventData);
    void HandleScriptReloadFinished(StringHash eventType, VariantMap& eventData);
    void HandleScriptReloadFailed(StringHash eventType, VariantMap& eventData);

    void HandleUpdate(StringHash eventType, VariantMap& eventData);
    void HandleRender(StringHash eventType, VariantMap& eventData);
    void HandleEndFrame(StringHash eventType, VariantMap& eventData);

    String libName_;
    bool started_;

    static MapEditor* mapEditor_;
};

#endif

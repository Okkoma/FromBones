#include "GameOptions.h"

#ifdef ACTIVE_CREATEMODE

#include <Urho3D/Urho3D.h>

#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Graphics/GraphicsEvents.h>

#include <Urho3D/Input/Input.h>

#include <Urho3D/IO/Log.h>

#include <Urho3D/Scene/SceneEvents.h>

#include <Urho3D/UI/UI.h>
#include <Urho3D/UI/UIEvents.h>

#include "GameOptions.h"
#include "GameContext.h"

#include "ViewManager.h"

#ifdef EDITORMODE1
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/AngelScript/ScriptFile.h>
#include <Urho3D/AngelScript/Script.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Resource/ResourceEvents.h>
#include "FromBonesAPI.h"
#else
    #ifdef EDITORMODE2
#include <dlfcn.h>
    #endif
#endif

#include "MapEditor.h"
#include "MapEditorLib.h"

#ifdef EDITORMODE1
SharedPtr<ScriptFile> scriptFile_;
#else
    #ifdef EDITORMODE2
// the types of the class factories
typedef MapEditorLib* CreateMapEditorLibFunc(Context*);
typedef void DestroyMapEditorLibFunc(MapEditorLib*);
void* editorDynLibraryHandle_ = 0;
#else
#include "../cppeditor/MapEditorLib.cpp"
    #endif
#endif

MapEditorLib* mapEditorLib_ = 0;
MapEditor* MapEditor::mapEditor_ = 0;
bool sMapEditorSavedFocusState_;

MapEditor::MapEditor(Context* context) :
    Object(context),
    started_(false)
{
    URHO3D_LOGINFOF("MapEditor()");

    Start();
}

MapEditor::~MapEditor()
{
    Stop();

    URHO3D_LOGINFOF("~MapEditor() ... OK !");
}

void MapEditor::Start()
{
    if (started_)
        return;

    if (LoadLibrary())
        URHO3D_LOGINFOF("MapEditor() - Start : load Editor Library ... OK !");
    else
        URHO3D_LOGERRORF("MapEditor() - Start : load Editor Script ... NOK !");

    // disable the node focus
    sMapEditorSavedFocusState_ = ViewManager::Get()->GetFocusEnable(0);
    ViewManager::Get()->SetFocusEnable(false, 0);

    // remove UI Scale
    GameContext::Get().ui_->SetScale(1.f);

    GameContext::Get().createModeOn_ = true;

    // Inactive Cursor Visibility Handle
    GameContext::Get().UnsubscribeFromEvent(E_SCENEUPDATE);
    GameContext::Get().UnsubscribeFromEvent(E_MOUSEMOVE);

    GameContext::Get().gameConfig_.debugRenderEnabled_ = true;
//    GameContext::Get().rootScene_->GetOrCreateComponent<DebugRenderer>(LOCAL);

    SubscribeToEvents();

    started_ = true;

    GameContext::Get().InitMouse(MM_FREE);
    GameContext::Get().SetMouseVisible(false, false, false);
    GameContext::Get().SetMouseVisible(false, true, false);
}

void MapEditor::Stop()
{
    if (!started_)
        return;

    UnsubscribeFromAllEvents();

    if (UnloadLibrary())
        URHO3D_LOGINFOF("MapEditor() - Stop : unload Editor Library ... OK !");
    else
        URHO3D_LOGERRORF("MapEditor() - Stop : unload Editor Script ... NOK !");

    // add UI Scale
    GameContext::Get().ui_->SetScale(GameContext::Get().uiScale_);

    // restore the node focus
    URHO3D_LOGINFOF("MapEditor() - Stop : restorefocus=%u ... ", sMapEditorSavedFocusState_);
    ViewManager::Get()->SetFocusEnable(sMapEditorSavedFocusState_, 0);
    sMapEditorSavedFocusState_ = false;

    GameContext::Get().createModeOn_ = false;

    // Unlock Mouse Vibilility on True
    GameContext::Get().SetMouseVisible(false, false, false);
    GameContext::Get().SetMouseVisible(true, false, false);

    started_ = false;
}


bool MapEditor::LoadLibrary()
{
#ifdef EDITORMODE1
    libName_ = Urho3D::GetInternalPath(String("Editor/Editor.as"));
    if (libName_.Empty())
    {
        URHO3D_LOGERRORF("MapEditor() - Start : can't find %s !", libName_.CString());
        return false;
    }

    // Instantiate and register the AngelScript subsystem
    Script* scriptsystem = new Script(context_);
    context_->RegisterSubsystem(scriptsystem);

    // Add FromBones API
    RegisterFromBonesAPI(scriptsystem->GetScriptEngine());

    // Set the Default Scene for the Script System
    scriptsystem->SetDefaultScene(GameContext::Get().rootScene_);

    // Hold a shared pointer to the script file to make sure it is not unloaded during runtime
    scriptFile_ = GetSubsystem<ResourceCache>()->GetTempResource<ScriptFile>(libName_);

    if (!scriptFile_  || !scriptFile_->Execute("void Start()"))
    {
        return false;
    }

    // Subscribe to script's reload event to allow live-reload of the application
    SubscribeToEvent(scriptFile_, E_RELOADSTARTED, URHO3D_HANDLER(MapEditor, HandleScriptReloadStarted));
    SubscribeToEvent(scriptFile_, E_RELOADFINISHED, URHO3D_HANDLER(MapEditor, HandleScriptReloadFinished));
    SubscribeToEvent(scriptFile_, E_RELOADFAILED, URHO3D_HANDLER(MapEditor, HandleScriptReloadFailed));

    return true;

#else
    #ifdef EDITORMODE2
    URHO3D_LOGINFOF("MapEditor() - LoadLibrary : load Editor Lib ... restorefocus=%u", sMapEditorSavedFocusState_);
#ifdef URHO3D_VULKAN
    libName_ = "../lib/vk/libFromBonesEditor.so";
#else
    libName_ = "../lib/gl/libFromBonesEditor.so";
#endif
    editorDynLibraryHandle_ = dlopen(libName_.CString(), RTLD_NOW);
    if (!editorDynLibraryHandle_)
    {
        URHO3D_LOGERRORF("MapEditor() - LoadLibrary : loading ... error %s !", dlerror());
        return false;
    }

    dlerror();
    CreateMapEditorLibFunc* CreateMapEditorLib = (CreateMapEditorLibFunc*) dlsym(editorDynLibraryHandle_, "mapeditorcreate");
    const char* dlsym_error1 = dlerror();
    if (dlsym_error1)
    {
        URHO3D_LOGERRORF("MapEditor() - LoadLibrary : finding 'mapeditorcreate' ... error %s !", dlsym_error1);
        return false;
    }

    DestroyMapEditorLibFunc* DestroyMapEditorLib = (DestroyMapEditorLibFunc*) dlsym(editorDynLibraryHandle_, "mapeditordestroy");
    const char* dlsym_error2 = dlerror();
    if (dlsym_error2)
    {
        URHO3D_LOGERRORF("MapEditor() - LoadLibrary : finding 'mapeditordestroy' ... error %s !", dlsym_error2);
        return false;
    }

    // create an instance of the class
    mapEditorLib_ = CreateMapEditorLib(context_);

    URHO3D_LOGINFOF("MapEditor() - LoadLibrary : mapEditorLib_=%u", mapEditorLib_);

    return true;
#else
    mapEditorLib_ = new MapEditorLibImpl(context_);
    return true;
    #endif
#endif
}

bool MapEditor::UnloadLibrary()
{
#ifdef EDITORMODE1
    if (scriptFile_)
    {
        // Execute the optional stop function
        if (scriptFile_->GetFunction("void Stop()"))
            scriptFile_->Execute("void Stop()");
    }

    context_->RemoveSubsystem<Script>();
    return true;

#else
    #ifdef EDITORMODE2
    if (editorDynLibraryHandle_)
    {
        if (mapEditorLib_)
        {
            DestroyMapEditorLibFunc* DestroyMapEditorLib = (DestroyMapEditorLibFunc*) dlsym(editorDynLibraryHandle_, "mapeditordestroy");
            DestroyMapEditorLib(mapEditorLib_);
        }

        dlclose(editorDynLibraryHandle_);
        editorDynLibraryHandle_ = 0;
    }

    return true;
#else
    delete mapEditorLib_;
    return true;
    #endif
#endif
}

void MapEditor::Toggle()
{
    if (!mapEditor_)
    {
        mapEditor_ = new MapEditor(GameContext::Get().context_);
        mapEditor_->Start();
    }
    else
    {
        Release();
    }
}

void MapEditor::Release()
{
    if (mapEditor_)
    {
        delete mapEditor_;
        mapEditor_ = 0;
    }
}

void MapEditor::HandleScriptReloadStarted(StringHash eventType, VariantMap& eventData)
{
#ifdef EDITORMODE1
    if (scriptFile_->GetFunction("void Stop()"))
        scriptFile_->Execute("void Stop()");
#endif
}

void MapEditor::HandleScriptReloadFinished(StringHash eventType, VariantMap& eventData)
{
#ifdef EDITORMODE1
    // Restart the script application after reload
    if (!scriptFile_->Execute("void Start()"))
    {
        scriptFile_.Reset();
        URHO3D_LOGERROR("MapEditor() - HandleScriptReloadFinished : Error !");
    }
#endif
}

void MapEditor::HandleScriptReloadFailed(StringHash eventType, VariantMap& eventData)
{
#ifdef EDITORMODE1
    scriptFile_.Reset();
    URHO3D_LOGERROR("MapEditor() - HandleScriptReloadFailed : Error !");
#endif
}

void MapEditor::SubscribeToEvents()
{
    SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(MapEditor, HandleUpdate));
    SubscribeToEvent(E_ENDALLVIEWSRENDER, URHO3D_HANDLER(MapEditor, HandleRender));
//    SubscribeToEvent(E_ENDFRAME, URHO3D_HANDLER(MapEditor, HandleEndFrame));
}

void MapEditor::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
    if (mapEditorLib_)
        mapEditorLib_->Update(eventData[Update::P_TIMESTEP].GetFloat());
}

void MapEditor::HandleRender(StringHash eventType, VariantMap& eventData)
{
    if (mapEditorLib_)
        mapEditorLib_->Render();
}

void MapEditor::HandleEndFrame(StringHash eventType, VariantMap& eventData)
{
    if (mapEditorLib_)
        mapEditorLib_->Clean();
}

#endif

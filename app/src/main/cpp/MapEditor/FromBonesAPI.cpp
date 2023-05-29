#include "GameOptions.h"

#ifdef EDITORMODE1

#include <AngelScript/angelscript.h>
#include <Urho3D/AngelScript/Script.h>
#include <Urho3D/AngelScript/APITemplates.h>

#include <Urho3D/Engine/Engine.h>

#include <Urho3D/Input/Input.h>

#include <Urho3D/Urho2D/RigidBody2D.h>

#include "GameOptions.h"
#include "GameHelpers.h"
#include "GameContext.h"

#include "RenderShape.h"
#include "GEF_Scrolling.h"
#include "ObjectTiled.h"
#include "MapWorld.h"
#include "Map.h"
#include "ViewManager.h"

#include "FromBonesAPI.h"

static World2D* GetWorld2D()
{
    return World2D::GetWorld();
}

static Component* GetBodyAtCursor()
{
    return static_cast<Component*>(GameHelpers::GetBodyAtCursor());
}

static void UpdateEntityPosition(Node* node)
{
    World2D::GetWorld()->GetCurrentMap()->RefreshEntityPosition(node);
}

//static Vector2 GetWorldPositionAtCursor()
//{
//    return GameHelpers::ScreenToWorld2D(GameContext::Get().cursor_ ? GameContext::Get().cursor_->GetPosition() : GameContext::Get().input_->GetMousePosition());
//}
//
//static int GetWorldCurrentViewZ()
//{
//    return ViewManager::Get()->GetCurrentViewZ();
//}

static void RegisterWorld2D(asIScriptEngine* engine)
{
    RegisterDrawable2D<RenderShape>(engine, "RenderShape");

    RegisterDrawable2D<ScrollingShape>(engine, "ScrollingShape");
    RegisterComponent<GEF_Scrolling>(engine, "GEF_Scrolling");
    RegisterStaticSprite2D<DrawableScroller>(engine, "DrawableScroller");
#ifdef USE_TILERENDERING
    RegisterDrawable2D<ObjectTiled>(engine, "ObjectTiled");
#endif
    RegisterObject<World2D>(engine, "World2D");
//    engine->RegisterObjectMethod("World2D", "Node@+ SpawnEntity(const StringHash&in, const Vector2&in, int& viewZ)", asMETHODPR(World2D, SpawnEntity, (const StringHash&, const Vector2&, int), Node*), asCALL_THISCALL);
    engine->RegisterObjectMethod("World2D", "Node@+ SpawnEntity(const StringHash&in)", asMETHODPR(World2D, SpawnEntity, (const StringHash&), Node*), asCALL_THISCALL);
    engine->RegisterObjectMethod("World2D", "Node@+ SpawnFurniture(const StringHash&in)", asMETHODPR(World2D, SpawnFurniture, (const StringHash&), Node*), asCALL_THISCALL);
    engine->RegisterObjectMethod("World2D", "Node@+ SpawnActor()", asMETHODPR(World2D, SpawnActor, (), Node*), asCALL_THISCALL);

    engine->RegisterGlobalFunction("World2D@+ get_world2D()", asFUNCTION(GetWorld2D), asCALL_CDECL);
    engine->RegisterGlobalFunction("Component@+ get_bodyAtCursor()", asFUNCTION(GetBodyAtCursor), asCALL_CDECL);

    engine->RegisterGlobalFunction("void UpdateEntityPosition(Node@+ node)", asFUNCTION(UpdateEntityPosition), asCALL_CDECL);

//    engine->RegisterGlobalFunction("Vector2 get_positionAtCursor()", asFUNCTION(GetWorldPositionAtCursor), asCALL_CDECL);
//    engine->RegisterGlobalFunction("int get_currentViewZ()", asFUNCTION(GetWorldCurrentViewZ), asCALL_CDECL);
}

void RegisterFromBonesAPI(asIScriptEngine* engine)
{
    RegisterWorld2D(engine);
}

#endif

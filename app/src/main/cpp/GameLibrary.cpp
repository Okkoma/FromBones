#include <Urho3D/Urho3D.h>

#include <Urho3D/IO/Log.h>

#include "GameOptions.h"
#include "GameRand.h"
#include "GameAttributes.h"
#include "GameEvents.h"
#include "GameContext.h"
#include "GameGoal.h"
#include "MapWorld.h"

#include "GOC_Animator2D.h"
#include "GOC_Attack.h"
#include "GOC_BodyExploder2D.h"
#ifdef GOC_COMPLETED
#include "GOC_BodyFaller2D.h"
#include "GOC_Collectable.h"
#endif
#include "GOC_Collide2D.h"
#include "GOC_Controller.h"
#include "GOC_ControllerAI.h"
#include "GOC_ControllerPlayer.h"
#include "GOC_Destroyer.h"
#include "GOC_Detector.h"
#ifdef GOC_COMPLETED
#include "GOC_DropZone.h"
#include "GOC_Inventory.h"
#endif
#include "GOC_Life.h"
#include "GOC_Move2D.h"
#ifdef GOC_COMPLETED
#include "GOC_SoundEmitter.h"
#include "GOC_ZoneEffect.h"
#include "GOC_Spawner.h"
#include "GOC_EntityAdder.h"
#include "GOC_EntityFollower.h"
#include "GOC_PhysicRope.h"
#include "GOC_StaticRope.h"
#include "GOC_Portal.h"
#include "GOC_Abilities.h"
#include "Actor.h"
#endif
#include "SimpleActor.h"
#ifdef GOC_COMPLETED
#include "Equipment.h"
#include "UISlotPanel.h"
#include "UIC_StatusPanel.h"
#include "UIC_BagPanel.h"
#include "UIC_EquipmentPanel.h"
#include "UIC_CraftPanel.h"
#include "UIC_ShopPanel.h"
#include "UIC_AbilityPanel.h"
#include "UIC_MissionPanel.h"
#include "UIC_MiniMap.h"
#include "GEF_Rain.h"
#include "RenderShape.h"
#include "GEF_Scrolling.h"
#include "GEF_NodeShaker.h"
#include "WaterLayer.h"
#endif
#include "ScrapsEmitter.h"
#include "GEF_Scrolling.h"
#include "LSystem.h"

#ifdef USE_TILERENDERING
#include "ObjectTiled.h"
#endif
#include "ObjectMaped.h"
#include "MapCreator.h"
#include "MapStorage.h"
#include "TileSheet2D.h"
#include "ViewManager.h"
#ifdef GOC_COMPLETED
#include "Ability.h"
#include "GO_Pool.h"
#endif
#include "ObjectPool.h"
#ifdef GOC_COMPLETED
#include "Behavior.h"
#include "GOB_Patrol.h"
#include "GOB_Follow.h"
#include "GOB_MountOn.h"
#include "GOB_PlayerCPU.h"
#endif

#include "DialogueFrame.h"
#include "DialogueData.h"
#include "JournalData.h"
#include "DefsEffects.h"
#include "DefsShapes.h"

#include "GameLibrary.h"



//extern const Vector3 OUTZONE(-10000.f, -10000.f, -10000.f);
//extern const IntRect OUTMAP(-1000, -1000, -1000, -1000);

void RegisterGameLibrary(Context* context)
{
    URHO3D_LOGINFO("RegisterGameLibrary : ... ");

    GameRand::InitTable();

    PolyShape::Allocate();

    GOA::InitAttributeTable();
    GOA::RegisterToContext(context);

    GOS::InitStateTable();
    COT::InitCategoryTables();
    GOT::InitDefaultTables();
    GOE::InitEventTable();

#ifdef GOC_COMPLETED
    EffectType::InitTable();
    ActorStats::InitTable();
    MissionGenerator::InitTable();
#endif
    ScrapsEmitter::RegisterObject(context);
    RenderShape::RegisterObject(context);
    ScrollingShape::RegisterObject(context);
    LSystem2D::RegisterObject(context);

    GOC_Life::RegisterObject(context);
    GOC_Attack::RegisterObject(context);
#ifdef GOC_COMPLETED
    GOC_Collectable::RegisterObject(context);
    GOC_Inventory::RegisterObject(context);
#endif
    GOC_Destroyer::RegisterObject(context);
    GOC_Destroyer::SetEnableWorld2D(true);
    GOC_Detector::RegisterObject(context);
    GOC_Move2D::RegisterObject(context);
    GOC_Controller::RegisterObject(context);
    GOC_AIController::RegisterObject(context);
    GOC_PlayerController::RegisterObject(context);
    GOC_Collide2D::RegisterObject(context);
    GOC_Animator2D::RegisterObject(context);
    GOC_BodyExploder2D::RegisterObject(context);
#ifdef GOC_COMPLETED
    GOC_BodyFaller2D::RegisterObject(context);
    GOC_SoundEmitter::RegisterObject(context);
    GOC_ZoneEffect::RegisterObject(context);
    GOC_DropZone::RegisterObject(context);
    GOC_Spawner::RegisterObject(context);
    GOC_EntityAdder::RegisterObject(context);
    GOC_EntityFollower::RegisterObject(context);
    GOC_PhysicRope::RegisterObject(context);
    GOC_StaticRope::RegisterObject(context);
    GOC_Portal::RegisterObject(context);
    GOC_Abilities::RegisterObject(context);
#endif
    SimpleActor::RegisterObject(context);
#ifdef GOC_COMPLETED
    Actor::RegisterObject(context);
    Equipment::RegisterObject(context);
//    Mission::RegisterObject(context);
    MissionManager::RegisterObject(context);
    ViewManager::RegisterObject(context);

    UIPanel::RegisterObject(context);
    UISlotPanel::RegisterObject(context);
    UIC_StatusPanel::RegisterObject(context);
    UIC_BagPanel::RegisterObject(context);
    UIC_EquipmentPanel::RegisterObject(context);
    UIC_CraftPanel::RegisterObject(context);
    UIC_ShopPanel::RegisterObject(context);
    UIC_AbilityPanel::RegisterObject(context);
    UIC_MissionPanel::RegisterObject(context);
    UIC_MiniMap::RegisterObject(context);

    GEF_Rain::RegisterObject(context);
    GEF_Scrolling::RegisterObject(context);
    DrawableScroller::RegisterObject(context);
    GEF_NodeShaker::RegisterObject(context);
    WaterLayer::RegisterObject(context);
#endif
#ifdef USE_TILERENDERING
    ObjectTiled::RegisterObject(context);
#endif
    ObjectMaped::RegisterObject(context);

    TerrainAtlas::RegisterObject(context);
    TileSheet2D::RegisterObject(context);
    AnlWorldModel::RegisterObject(context);

    World2D::RegisterObject(context);
    MapCreator::InitTable();
    MapStorage::InitTable(context);

    DialogueData::RegisterObject(context);
    JournalData::RegisterObject(context);
    DialogueFrame::RegisterObject(context);

#ifdef GOC_COMPLETED
    Ability::RegisterLibraryAbility(context);

    GO_Pool::RegisterObject(context);

    // Register All Behaviors here !
    Behaviors::Clear();
    Behaviors::Register(new GOB_Patrol(context));
    Behaviors::Register(new GOB_WaitAndDefend(context));
    Behaviors::Register(new GOB_Follow(context));
    Behaviors::Register(new GOB_FollowAttack(context));
    Behaviors::Register(new GOB_MountOn(context));
    Behaviors::Register(new GOB_PlayerCPU(context));
#endif

    EffectAction::RegisterEffectActionLibrary(context);

    URHO3D_LOGINFO("RegisterGameLibrary : ... OK !");
}

void UnRegisterGameLibrary(Context* context)
{
    URHO3D_LOGINFO("UnRegisterGameLibrary : ... ");

    UIPanel::ClearRegisteredPanels();
    GOC_SoundEmitter::ClearTemplates();

#ifdef GOC_COMPLETED
    Ability::Clear();
    Behaviors::Clear();
//    MapCreator::DeInitTable();
    MapStorage::DeInitTable(context);
    MapCreator::DeInitTable();
#endif

    PolyShape::DeAllocate();

    URHO3D_LOGINFO("UnRegisterGameLibrary : ... OK !");
}

void DumpGameLibrary()
{
    URHO3D_LOGINFO("DumpGameLibrary : ...");

#ifdef DUMP_ATTRIBUTES
    GOA::DumpAll();
    GOE::DumpAll();
    GOS::DumpAll();
    GOT::DumpAll();
    COT::DumpAll();
#endif // DUMP_ATTRIBUTES
#ifdef DUMP_ANIMATORTEMPLATES
    GOC_Animator2D_Template::DumpAll();
#endif
#ifdef DUMP_INVENTORYTEMPLATES
    GOC_SoundEmitter_Template::DumpAll();
#endif
#ifdef DUMP_SOUNDTEMPLATES
    GOC_Inventory_Template::DumpAll();
#endif
#ifdef DUMP_EFFECTTEMPLATES
    EffectType::DumpAll();
#endif

    URHO3D_LOGINFO("DumpGameLibrary : ... OK !");
}




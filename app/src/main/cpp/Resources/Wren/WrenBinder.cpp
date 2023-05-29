#include <Urho3D/Urho3D.h>

#include <Urho3D/IO/Log.h>


#include "DialogueData.h"
#include "DialogueFrame.h"
#include "Actor.h"
#include "Player.h"
#include "GameGoal.h"

#include "wren.hpp"
#include "WrenBinder.h"

#include "game.wren.inc"

/// Steps for add/remove/rename binder functions
/// 1.Modify Resources/Wren/game.wren
/// 2.Launch python script to generate game.wren.inc

/*
void speakerAllocate(WrenVM* vm)
{

}

void speakerFinalize(void* data)
{

}
*/
void speakerSetEnableShop(WrenVM* vm)
{
//    URHO3D_LOGINFOF("Speaker SetEnableShop !");
    DialogueData::GetSpeaker()->SetEnableShop(wrenGetSlotBool(vm, 1));
}

void speakerToggleDialogueFrame(WrenVM* vm)
{
//    URHO3D_LOGINFOF("Speaker ToggleDialogueFrame !");
    DialogueData::GetSpeaker()->GetAvatar()->GetComponent<DialogueFrame>()->ToggleFrame(wrenGetSlotBool(vm, 1), wrenGetSlotDouble(vm, 2));
}

void contributorAddMission(WrenVM* vm)
{
    String questname = wrenGetSlotString(vm, 1);
    URHO3D_LOGINFOF("Contributor addMission : %s", questname.CString());
    ((Player*)DialogueData::GetContributor())->missionManager_->AddMission(questname);
}

/// Don't forget to change MAX_METHODS_PER_CLASS && MAX_CLASSES_PER_MODULE in wren_modules.h

// The array of built-in modules.
ModuleRegistry wrenModules[] =
{
    WREN_MODULE(game)
    WREN_CLASS(Speaker)
    /*
      WREN_STATIC_METHOD("<allocate>", speakerAllocate)
      WREN_FINALIZER(speakerFinalize)
    */
    WREN_STATIC_METHOD("setEnableShop(_)", speakerSetEnableShop)
    WREN_STATIC_METHOD("toggleDialogueFrame(_,_)", speakerToggleDialogueFrame)
    WREN_END_CLASS
    WREN_CLASS(Contributor)
    /*
      WREN_STATIC_METHOD("<allocate>", contributorAllocate)
      WREN_FINALIZER(contributorFinalize)
    */
    WREN_STATIC_METHOD("addMission(_)", contributorAddMission)
    WREN_END_CLASS
    WREN_END_MODULE

    WREN_SENTINEL_MODULE
};

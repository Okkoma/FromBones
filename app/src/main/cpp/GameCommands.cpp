#include <iostream>
#include <cstdio>

#include <Urho3D/Urho3D.h>

#include <Urho3D/IO/Log.h>

#include <Urho3D/Resource/ResourceCache.h>

#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/Scene/Scene.h>

#include <Urho3D/Urho2D/Renderer2D.h>
#include <Urho3D/Urho2D/AnimatedSprite2D.h>
#include <Urho3D/Urho2D/RigidBody2D.h>

#include "DefsGame.h"

#include "GameOptions.h"
#include "GameGoal.h"
#include "GameAttributes.h"
#include "GameContext.h"
#include "GameHelpers.h"
#include "GameNetwork.h"
#include "GameStateManager.h"
#include "sPlay.h"
#include "sMainMenu.h"

#include "ViewManager.h"
#include "MAN_Go.h"
#include "MAN_Ai.h"
#include "Player.h"
#include "Equipment.h"
#include "Map.h"
#include "MapStorage.h"
#include "MapWorld.h"
#include "MapColliderGenerator.h"
#include "MapGeneratorDungeon.h"
#include "AnlWorldModel.h"
#include "ObjectPool.h"
#include "GEF_Scrolling.h"
#include "CommonComponents.h"

#include "GameCommands.h"


void GameCommands::Launch(const String& input)
{
    String inputLower = input.ToLower().Trimmed();
    if (inputLower.Empty())
        return;

    Vector<String> inputs = inputLower.Split('=');
    const String& input0 = inputs[0];

    if (inputLower == "quit" || inputLower == "exit")
    {
        GameContext::Get().Exit();
        return;
    }
    else if (inputLower == "break" || inputLower == "menu")
    {
        GameContext::Get().stateManager_->PopStack();
    }
    else if (inputLower == "statics")
    {
        GameContext::Get().Dump();
    }
    else if (inputLower == "aicontrol")
    {
        URHO3D_LOGINFOF("Num active players=%u", GOManager::GetNumActivePlayers());
        URHO3D_LOGINFOF("Num active enemies=%u", GOManager::GetNumActiveEnemies());
        URHO3D_LOGINFOF("Num active bots=%u", GOManager::GetNumActiveBots());
        URHO3D_LOGINFOF("Num active controlled Nodes=%u", GOManager::GetNumActiveAiNodes());
    }
    else if (inputLower == "seed")
    {
        if (GameContext::Get().stateManager_ && GameContext::Get().stateManager_->GetActiveState()->GetStateId() == "Play")
        {
            unsigned wseed = ((PlayState*)GameContext::Get().stateManager_->GetActiveState())->GetWorldSeed();
            const ShortIntVector2& mpoint = World2D::GetCurrentMapPoint();
            unsigned mseed = mpoint.ToHash();
            URHO3D_LOGINFOF("MapPoint=%s WorldSeed=%u MapSeed=%u(wseed+%u)", mpoint.ToString().CString(), wseed, wseed+mseed, mseed);
        }
    }
    else if (inputLower == "pool")
    {
        if (ObjectPool::Get())
            ObjectPool::Get()->DumpCategories();
    }
    else if (inputLower == "render")
    {
        GameContext::Get().rootScene_->GetComponent<Renderer2D>()->Dump();
    }
    else if (inputLower == "renderrtt")
    {
        GameContext::Get().rttScene_->GetComponent<Renderer2D>()->Dump();
    }
    else if (inputLower == "camera")
    {
        URHO3D_LOGINFOF("camnode=%u cam=%u(%u) viewport=%u(cam=%u cullcam=%u) viewmask=%u visiblerectsize=%s",
                        GameContext::Get().cameraNode_.Get(), GameContext::Get().cameraNode_->GetComponent<Camera>(), GameContext::Get().camera_,
                        GameContext::Get().context_->GetSubsystem<Renderer>()->GetViewport(0), GameContext::Get().context_->GetSubsystem<Renderer>()->GetViewport(0)->GetCamera(),
                        GameContext::Get().context_->GetSubsystem<Renderer>()->GetViewport(0)->GetCullCamera(),
                        GameContext::Get().camera_->GetViewMask(), World2D::GetVisibleRect().Size().ToString().CString());
    }
    else if (inputLower == "worldstorage")
    {
        if (GameContext::Get().stateManager_->GetActiveState()->GetStateId() != "Play")
            return;

        World2D::GetStorage()->DumpMapsMemory();
    }
    else if (inputLower == "worldobjects")
    {
        if (GameContext::Get().stateManager_->GetActiveState()->GetStateId() != "Play")
            return;

        World2D::GetWorld()->DumpEntitiesInMemory();
    }
    else if (inputLower == "worldvisibility")
    {
        if (GameContext::Get().stateManager_->GetActiveState()->GetStateId() != "Play")
            return;

        if (World2D::GetWorld())
            World2D::GetWorld()->DumpMapVisibilityProgress();
    }
    else if (inputLower == "worldtravelers")
    {
        if (GameContext::Get().stateManager_->GetActiveState()->GetStateId() != "Play")
            return;

        if (World2D::GetWorld())
            World2D::GetWorld()->DumpTravelers();
    }
    else if (inputLower == "fluidenable")
    {
        GameContext::Get().gameConfig_.fluidEnabled_ = !GameContext::Get().gameConfig_.fluidEnabled_;
        URHO3D_LOGINFOF("fluidenable = %s", GameContext::Get().gameConfig_.fluidEnabled_ ? "true" : "false");
    }
    else if (inputLower == "fluidpull")
    {
        if (GameContext::Get().stateManager_->GetActiveState()->GetStateId() != "Play")
            return;

        GameContext::Get().gameConfig_.fluidEnabled_ = false;

        const HashMap<ShortIntVector2, Map* >&  maps = World2D::GetWorld()->GetStorage()->GetMapsInMemory();
        for (HashMap<ShortIntVector2, Map* >::ConstIterator it=maps.Begin(); it != maps.End(); ++it)
        {
            if (it->second_)
                it->second_->PullFluids();
        }
    }
    else if (inputLower == "clients")
    {
        if (GameNetwork::Get())
            GameNetwork::Get()->DumpClientInfos();
    }
    else if (inputLower == "killclients")
    {
        if (GameNetwork::Get())
        {
            GameNetwork::Get()->Server_KillConnections();
            GameNetwork::Get()->DumpClientInfos();
        }
    }
    else if (inputLower == "dumpgos")
    {
        GOS::DumpAll();
    }
    else if (inputLower == "dumpview")
    {
        if (GameContext::Get().stateManager_->GetActiveState()->GetStateId() != "Play")
            return;

        ViewManager::Get()->Dump();
    }
    else if (inputLower == "tips" || inputLower == "help")
    {
        URHO3D_LOGINFOF("Tip Commands :");
        URHO3D_LOGINFOF(" => Add Tile      : Ctrl-Left + LeftMouse");
        URHO3D_LOGINFOF(" => Remove Tile   : Ctrl-Left + RightMouse");
        URHO3D_LOGINFOF(" => Dump Tile     : Ctrl-Left + MiddleMouse");
        URHO3D_LOGINFOF(" => Add Actor     : Shift-Left + LeftMouse + Key_1");
        URHO3D_LOGINFOF(" => Add Entity    : Shift-Left + LeftMouse + Key_2");
        URHO3D_LOGINFOF(" => Kill Entity   : Shift-Left + LeftMouse + Key_3");
        URHO3D_LOGINFOF(" => Create ObjectMap From Map  : Shift-Left + RightMouse + Key_1");
        URHO3D_LOGINFOF(" => Create ObjectMap From Data : Shift-Left + RightMouse + Key_2");
        URHO3D_LOGINFOF(" => Create ObjectMap From Gene : Shift-Left + RightMouse + Key_3");
        URHO3D_LOGINFOF(" => Test Asteroids Fall        : Shift-Left + RightMouse + Key_4");
    }
    else if (inputLower == "netstate")
    {
        if (GameNetwork::Get())
        {
            GameNetwork::Get()->Dump();
        }
    }
    else if (inputLower == "netsync")
    {
        if (GameContext::Get().ClientMode_)
            GameNetwork::Get()->GetConnection()->SynchronizeObjectCommands();
    }
    else if (inputLower == "netlog")
    {
        if (GameNetwork::Get())
        {
            GameNetwork::Get()->SetLogStatistics(!GameNetwork::Get()->GetLogStatistics());
        }
    }
#ifdef ACTIVE_NETWORK_SIMULATOR
    else if (inputLower == "netsimulator")
    {
        if (GameNetwork::Get())
        {
            GameNetwork::Get()->SetSimulatorActive(true);
            URHO3D_LOGINFOF("netsimulator = %s", GameNetwork::Get()->GameNetwork::IsSimulatorActive() ? "true":"false");
        }
    }
#endif
    else if (input0 == "dumpmap")
    {
        if (GameContext::Get().stateManager_->GetActiveState()->GetStateId() != "Play")
            return;

        ShortIntVector2 mpoint = World2D::GetCurrentMapPoint();
        if (inputs.Size() == 2)
        {
            Vector<String> coords = inputs[1].Split(',');
            if (coords.Size() < 2)
                return;

            mpoint.x_ = ToInt(coords[0]);
            mpoint.y_ = ToInt(coords[1]);
        }

        Map* map = World2D::GetMapAt(mpoint);
        if (map)
            map->Dump();
    }
    else if (input0 == "setlevel")
    {
        int level = GameContext::Get().SetLevel(inputs.Size() > 1 ? ToInt(inputs[1]) : 1);
        URHO3D_LOGINFOF("SetLevel=%d", level);
    }
    else if (input0 == "numplayers")
    {
        GameContext::Get().numPlayers_ = inputs.Size() > 1 ? Min( Max(ToInt(inputs[1]), 0), GameContext::Get().MAX_NUMPLAYERS) : 1;
        URHO3D_LOGINFOF("Set NumLocalPlayers=%d", GameContext::Get().numPlayers_);
        for (unsigned i=0; i < GameContext::Get().numPlayers_; i++)
            GameContext::Get().playerState_[i].avatar = i;
    }
    else if (input0 == "player1" || input0 == "player2" || input0 == "player3" || input0 == "player4")
    {
        int playerindex = Min(Max(ToInt(input0.Substring(6))-1, 0), 4);
        int avatarindex = Min(inputs.Size() > 1 ? Max(ToInt(inputs[1]), 0) : 0, 5);
        GameContext::Get().playerState_[playerindex].avatar = avatarindex;

        URHO3D_LOGINFOF("Set Player%d avatar=%d", playerindex, avatarindex);
        if (GameContext::Get().stateManager_)
        {
            const String& stateId = GameContext::Get().stateManager_->GetActiveState()->GetStateId();
            if (stateId == "Play")
            {
                PlayState* playstate = (PlayState*)GameContext::Get().stateManager_->GetActiveState();
                if (playstate->GetLocalPlayers().Size() > playerindex)
                    playstate->GetLocalPlayers()[playerindex]->ChangeAvatar(avatarindex);
            }
        }
    }
    else if (input0 == "dumpplayer")
    {
        if (GameContext::Get().stateManager_)
        {
            const String& stateId = GameContext::Get().stateManager_->GetActiveState()->GetStateId();
            if (stateId == "Play")
            {
                PlayState* playstate = (PlayState*)GameContext::Get().stateManager_->GetActiveState();
                if (playstate->GetLocalPlayers().Size())
                    playstate->GetLocalPlayers()[0]->Dump();

            }
        }
    }
    else if (input0 == "dumpbody")
    {
        unsigned id = ToUInt(inputs[1]);

        if (GameContext::Get().rootScene_->GetNode(id))
            GameContext::Get().rootScene_->GetNode(id)->GetComponent<RigidBody2D>()->GetBody()->Dump();
        else
            ((RigidBody2D*)GameContext::Get().rootScene_->GetComponent(id))->GetBody()->Dump();
    }
    else if (input0 == "dumprbody")
    {
        unsigned id = ToUInt(inputs[1]);
        GameHelpers::DumpRigidBody(GameContext::Get().rootScene_->GetNode(id));
    }
    else if (input0 == "dumpcmaps")
    {
        Node* node = GameContext::Get().rootScene_->GetNode(ToUInt(inputs[1]));
        if (node)
        {
            AnimatedSprite2D* anim = node->GetComponent<AnimatedSprite2D>();
            if (anim)
            {
                URHO3D_LOGINFOF("Dump Applied Character Mappings for Node=%s(%u) :", node->GetName().CString(), node->GetID());
                const VariantVector& cmaps = anim->GetAppliedCharacterMapsAttr();
                unsigned i = 0;
                for (VariantVector::ConstIterator it = cmaps.Begin(); it != cmaps.End(); ++it, i++)
                {
                    const StringHash& hashname = it->GetStringHash();
                    HashMap<StringHash, String>::ConstIterator jt = CharacterMappingNames_.Find(hashname);
                    URHO3D_LOGINFOF(" cmaps[%u] = %s(%u)", i, jt != CharacterMappingNames_.End() ? jt->second_.CString() : "NotRegistered", hashname.Value());
                }
            }
        }
    }
    else if (input0 == "dumpanimator")
    {
        Node* node = GameContext::Get().rootScene_->GetNode(ToUInt(inputs[1]));
        if (node)
        {
            GOC_Animator2D* animator = node->GetComponent<GOC_Animator2D>();
            if (animator)
            {
                URHO3D_LOGINFOF("Dump Animator Template for Node=%s(%u) :", node->GetName().CString(), node->GetID());
                animator->DumpTemplate();
            }
        }
    }
    else if (input0 == "dumpai")
    {
        const String& stateId = GameContext::Get().stateManager_->GetActiveState()->GetStateId();
        if (stateId == "Play")
        {
            AIManager::DumpAll();
        }
    }
    else if (input0 == "dumpmissions")
    {
        if (GameContext::Get().stateManager_)
        {
            const String& stateId = GameContext::Get().stateManager_->GetActiveState()->GetStateId();
            if (stateId == "Play")
            {
                PlayState* playstate = (PlayState*)GameContext::Get().stateManager_->GetActiveState();
                if (playstate->GetLocalPlayers().Size())
                {
                    Player* player = playstate->GetLocalPlayers()[0];
                    if (player->missionManager_)
                        player->missionManager_->Dump();
                }
            }
        }
    }
    else if (input0 == "scene")
    {
        CreateMode mode = REPLICATED;
        if (inputs.Size() > 1)
            if (inputs[1] == "local")
                mode = LOCAL;

        GameHelpers::DumpNode(GameContext::Get().rootScene_, true);
    }
    else if (input0 == "rttscene")
    {
        CreateMode mode = REPLICATED;
        if (inputs.Size() > 1)
            if (inputs[1] == "local")
                mode = LOCAL;

        GameHelpers::DumpNode(GameContext::Get().rttScene_, true);
    }
    else if (input0 == "node")
    {
        if (inputs.Size() > 1)
            GameHelpers::DumpNode(ToUInt(inputs[1]), false);
    }
    else if (input0 == "nodeai")
    {
        if (inputs.Size() > 1)
        {
            unsigned id = ToUInt(inputs[1]);
            Node* node = GameContext::Get().rootScene_->GetNode(id);
            GOC_AIController* controller = node ? node->GetComponent<GOC_AIController>() : 0;
            if (controller)
                controller->Dump();
        }
    }
    else if (input0 == "netnode")
    {
        if (inputs.Size() > 1)
        {
            unsigned nodeid = ToUInt(inputs[1]);

            const ObjectControlInfo* cinfo = GameNetwork::Get()->GetObjectControl(nodeid);
            if (cinfo)
                cinfo->Dump();
            else
                URHO3D_LOGERRORF("netnode=%u : can't find objectcontrolinfo !", nodeid);
//            Node* node = GameContext::Get().rootScene_->GetNode(id);
        }
    }
    else if (input0 == "netplayers")
    {
        const Vector<NetPlayerInfo >& netplayers = GameNetwork::Get()->GetNetPlayersInfos();
        for (Vector<NetPlayerInfo >::ConstIterator it = netplayers.Begin(); it != netplayers.End(); ++it)
        {
            const NetPlayerInfo& info = *it;
            URHO3D_LOGINFOF("netplayers[%d] : node=%s(%u)", it-netplayers.Begin(), info.node_ ? info.node_->GetName().CString() : "none", info.node_ ? info.node_->GetID() : 0);
        }
    }
    else if (input0 == "activenode")
    {
        if (inputs.Size() > 1)
        {
            Node* node = GameContext::Get().rootScene_->GetNode(ToUInt(inputs[1]));
            if (node) node->SetEnabled(true);
        }
    }
    else if (input0 == "nodeposition")
    {
        if (inputs.Size() > 1)
        {
            Node* node = GameContext::Get().rootScene_->GetNode(ToUInt(inputs[1]));
            if (node)
            {
                GOC_Destroyer* gocdestroyer = node->GetComponent<GOC_Destroyer>();
                if (gocdestroyer) gocdestroyer->DumpWorldMapPositions();
            }
        }
    }
    else if (input0 == "dumpmove")
    {
        if (inputs.Size() > 1)
        {
            Node* node = GameContext::Get().rootScene_->GetNode(ToUInt(inputs[1]));
            if (node)
            {
                GOC_Move2D* gocmove = node->GetComponent<GOC_Move2D>();
                if (gocmove) gocmove->Dump();
            }
        }
    }
    else if (input0 == "playerposition")
    {
        if (GameContext::Get().stateManager_)
        {
            const String& stateId = GameContext::Get().stateManager_->GetActiveState()->GetStateId();
            if (stateId == "Play")
            {
                PlayState* playstate = (PlayState*)GameContext::Get().stateManager_->GetActiveState();
                if (playstate->GetLocalPlayers().Size())
                {
                    Node* avatar = playstate->GetLocalPlayers()[0]->GetAvatar();
                    GOC_Destroyer* gocdestroyer = avatar ? avatar->GetComponent<GOC_Destroyer>() : 0;

                    if (gocdestroyer)
                        gocdestroyer->DumpWorldMapPositions();
                }
            }
        }
    }
    else if (input0 == "nodeattr")
    {
        if  (inputs.Size() > 1)
            GameHelpers::DumpNode(ToUInt(inputs[1]), true);
    }
    else if (input0 == "rttnodeattr")
    {
        if  (inputs.Size() > 1)
            GameHelpers::DumpNode(ToUInt(inputs[1]), true, true);
    }
    else if (input0 == "vars")
    {
        if  (inputs.Size() > 1)
        {
            unsigned id = ToUInt(inputs[1]);
            Node* node = GameContext::Get().rootScene_->GetNode(id);
            if (node)
            {
                URHO3D_LOGINFOF("Node %s(%u) enabled=%s Parent %s(%u)", node->GetName().CString(), id, node->IsEnabled() ? "true" : "false",
                                node->GetParent() ? node->GetParent()->GetName().CString() : "NULL", node->GetParent() ? node->GetParent()->GetID() : 0);
                const VariantMap& vars = node->GetVars();
                for (VariantMap::ConstIterator it=vars.Begin(); it!=vars.End(); ++it)
                {
                    const String& attrName = GameContext::Get().context_->GetUserAttributeName(it->first_);
                    URHO3D_LOGINFOF(" => Var %s(%u) = %s(%s)", attrName.CString(), it->first_.Value(), it->second_.ToString().CString(), it->second_.GetTypeName().CString());
                }
            }
            else
                URHO3D_LOGINFOF("No Node id=%u", id);
        }
    }
    else if (input0 == "component")
    {
        if  (inputs.Size() > 1)
        {
            unsigned id = ToUInt(inputs[1]);
            Component* component = GameContext::Get().rootScene_->GetComponent(id);
            if (component)
                URHO3D_LOGINFOF("Component %s(%u) in node %s(%u)", component->GetTypeName().CString(), id,
                                component->GetNode()->GetName().CString(), component->GetNode()->GetID());
            else
                URHO3D_LOGINFOF("No Component id=%u", id);
        }
    }
    else if (input0 =="componentenable")
    {
        if  (inputs.Size() > 1)
        {
            unsigned id = ToUInt(inputs[1]);
            Component* component = GameContext::Get().rootScene_->GetComponent(id);
            if (component)
            {
                component->SetEnabled(!component->IsEnabled());
                URHO3D_LOGINFOF("Component %s(%u) in node %s(%u) switch to %s", component->GetTypeName().CString(), id,
                                component->GetNode()->GetName().CString(), component->GetNode()->GetID(), component->IsEnabled() ? "enable":"disable");
            }
            else
                URHO3D_LOGINFOF("No Component id=%u", id);
        }
    }
    else if (input0 == "componentattr")
    {
        if  (inputs.Size() > 1)
        {
            unsigned id = ToUInt(inputs[1]);
            Component* component = GameContext::Get().rootScene_->GetComponent(id);
            if (component)
            {
                URHO3D_LOGINFOF("Component %s(%u) in node %s(%u) enabled=%s", component->GetTypeName().CString(), id,
                                component->GetNode()->GetName().CString(), component->GetNode()->GetID(), component->IsEnabled() ? "true":"false");
                const Vector<AttributeInfo>* attrptr = component->GetAttributes();
                if (attrptr)
                {
                    const Vector<AttributeInfo>& attributes = *attrptr;
                    for (unsigned i=0; i < attributes.Size(); i++)
                    {
                        Variant variant;
                        component->OnGetAttribute(attributes[i], variant);
                        URHO3D_LOGINFOF(" => Attribute : %s value(%s)=%s", attributes[i].name_.CString(), variant.GetTypeName().CString(), variant.ToString().CString());
                    }
                }
                if (component->GetType() == AnimatedSprite2D::GetTypeStatic())
                {
                    AnimatedSprite2D* animatedSprite = (AnimatedSprite2D*)component;
                    URHO3D_LOGINFOF("WBBox=%s BBox=%s", animatedSprite->GetWorldBoundingBox().ToString().CString(),
                                    animatedSprite->GetBoundingBox().ToString().CString());
                }
            }
            else
                URHO3D_LOGINFOF("No Component id=%u", id);
        }
    }
    else if (input0 == "collider")
    {
        if  (inputs.Size() > 1)
        {
            unsigned id = ToUInt(inputs[1]);
            Node* node = GameContext::Get().rootScene_->GetNode(id);
            if (node)
            {
                URHO3D_LOGINFOF("Node %s(%u) Dump Collider", node->GetName().CString(), id);
                GOC_Collide2D* goccollide= node->GetComponent<GOC_Collide2D>();
                if (goccollide)
                    goccollide->DumpContacts();
            }
            else
                URHO3D_LOGINFOF("No Node id=%u", id);
        }
    }
    else if (input0 == "tracecollider")
    {
        MapColliderGenerator::Get()->ActiveDebugTraceOnce();
    }
    else if (input0 == "focus")
    {
        if (inputs.Size() == 1 || !ViewManager::Get())
            return;

        unsigned id = ToUInt(inputs[1]);
        Node* node = GameContext::Get().rootScene_->GetNode(id);
        if (node)
        {
            ViewManager::Get()->ResetFocus(0);
            ViewManager::Get()->AddFocus(node, true, 0);
            World2D::GetWorld()->UpdateInstant(0);
            URHO3D_LOGINFOF("Focus On Node=%u", id);
        }
        else
            URHO3D_LOGINFOF("No Node=%u", id);
    }
    else if (input0 == "inventory")
    {
        if (inputs.Size() == 1)
            return;

        unsigned id = ToUInt(inputs[1]);
        Node* node = GameContext::Get().rootScene_->GetNode(id);
        if (node)
        {
            GOC_Inventory* gocinventory = node->GetComponent<GOC_Inventory>();
            if (gocinventory)
            {
                URHO3D_LOGINFOF("Node %s(%u) Dump Inventory", node->GetName().CString(), id);
                gocinventory->Dump();
            }
            else
                URHO3D_LOGINFOF("Node %s(%u) No Inventory !", node->GetName().CString(), id);
        }
        else
            URHO3D_LOGINFOF("No Node=%u", id);
    }
    else if (input0 == "equipment")
    {
        if (inputs.Size() == 1)
            return;

        unsigned id = ToUInt(inputs[1]);
        Node* node = GameContext::Get().rootScene_->GetNode(id);
        if (node)
        {
            GOC_Controller* controller = node->GetDerivedComponent<GOC_Controller>();
            Actor* actor = controller->GetThinker();
            if (controller && actor && actor->GetType() == Player::GetTypeStatic())
            {
                Player* player = (Player*)actor;

                URHO3D_LOGINFOF("Node %s(%u) Dump Equipment", node->GetName().CString(), id);
                player->equipment_->Dump();
            }
            else
                URHO3D_LOGINFOF("Node %s(%u) No Equipment !", node->GetName().CString(), id);
        }
        else
            URHO3D_LOGINFOF("No Node=%u", id);
    }
    else if (input0 == "life")
    {
        if (inputs.Size() == 1)
            return;

        unsigned id = ToUInt(inputs[1]);
        Node* node = GameContext::Get().rootScene_->GetNode(id);
        if (node)
        {
            GOC_Life* life = node->GetComponent<GOC_Life>();
            if (life)
            {
                URHO3D_LOGINFOF("Node %s(%u) Dump Life", node->GetName().CString(), id);
                life->Dump();
            }
            else
                URHO3D_LOGINFOF("Node %s(%u) No Life !", node->GetName().CString(), id);
        }
        else
            URHO3D_LOGINFOF("No Node=%u", id);
    }
    else if (input0 == "arena" || input0 == "test" || input0 == "create" || input0 == "new" || input0 == "new+" || input0 == "change")
    {
        if (!GameContext::Get().stateManager_)
            return;

        GameLevelMode mode = LVL_NEW;
        unsigned seed = 0, incseed = 0;

        if (input0 == "new+")
        {
            seed = 0;
            mode = LVL_NEW;
            incseed = inputs.Size()  > 1 ? ToUInt(inputs[1]) : 1;
        }
        else
        {
            if (input0 == "arena")
            {
                mode = LVL_ARENA;
            }
            else if (input0 == "test")
            {
                mode = LVL_TEST;
            }
    #ifdef ACTIVE_CREATEMODE
            else if (input0 == "create")
            {
                mode = LVL_CREATE;
            }
    #endif
            else if (input0 == "new")
            {
                mode = LVL_NEW;
                incseed = 1;
            }
            else if (input0 == "change")
            {
                mode = LVL_CHANGE;
                incseed = 1;
            }

            if (inputs.Size() > 1)
            {
                seed = ToUInt(inputs[1]);
                incseed = 0;
            }
            else
            {
                incseed = 1;
            }
        }

        const String& stateId = GameContext::Get().stateManager_->GetActiveState()->GetStateId();
        if (stateId == "Play")
        {
            PlayState* playstate = (PlayState*)GameContext::Get().stateManager_->GetActiveState();
            if (inputs.Size() == 1)
                seed = playstate->GetWorldSeed();

            playstate->BeginNewLevel(mode, seed+incseed);
        }
        else if (stateId == "MainMenu")
        {
            ((MenuState*)GameContext::Get().stateManager_->GetActiveState())->BeginNewLevel(mode, seed+incseed);
        }
    }
    else if (input0 == "save")
    {
        const String& stateId = GameContext::Get().stateManager_->GetActiveState()->GetStateId();
        if (stateId == "Play")
            ((PlayState*)GameContext::Get().stateManager_->GetActiveState())->SaveGame();
    }
    else if (input0 == "load")
    {
        GameContext::Get().loadSavedGame_ = true;
        GameContext::Get().SetLevel();
        GameContext::Get().SetConsoleVisible(false);
        // Lock Mouse Vibilility on False
        GameContext::Get().SetMouseVisible(false, false, true);
        const String& stateId = GameContext::Get().stateManager_->GetActiveState()->GetStateId();
        if (stateId == "MainMenu")
            GameContext::Get().stateManager_->PushToStack("Play");
        else if (stateId == "Play")
            ((PlayState*)GameContext::Get().stateManager_->GetActiveState())->ReloadLevel();
    }
    else if (input0 == "playarena")
    {
        const String& stateId = GameContext::Get().stateManager_->GetActiveState()->GetStateId();
        if (stateId == "MainMenu")
            ((MenuState*)GameContext::Get().stateManager_->GetActiveState())->Launch(1);
    }
    else if (input0 == "playworld")
    {
        const String& stateId = GameContext::Get().stateManager_->GetActiveState()->GetStateId();
        if (stateId == "MainMenu")
            ((MenuState*)GameContext::Get().stateManager_->GetActiveState())->Launch(2);
    }
    else if (input0 == "levelwin")
    {
        const String& stateId = GameContext::Get().stateManager_->GetActiveState()->GetStateId();
        if (stateId == "Play")
        {
            PlayState* playstate = (PlayState*)GameContext::Get().stateManager_->GetActiveState();
            GameContext::Get().tipsWinLevel_ = true;
            playstate->SetGameWin();
        }
    }
    else if (input0 == "go2map")
    {
        if (GameContext::Get().stateManager_->GetActiveState()->GetStateId() != "Play")
            return;

        if (inputs.Size() < 2)
            return;

        Vector<String> coords = inputs[1].Split(',');
        if (coords.Size() < 2)
            return;

        ShortIntVector2 mpoint(ToInt(coords[0]), ToInt(coords[1]));
        URHO3D_LOGINFOF("go2map(%s) => waiting ...", mpoint.ToString().CString());

        GameHelpers::TransferPlayersToMap(mpoint);
    }
    else if (input0 == "convert2map")
    {
        if (inputs.Size() < 2)
            return;

        Vector<String> coords = inputs[1].Split(',');
        if (coords.Size() < 2)
            return;

        Vector2 position(ToFloat(coords[0]),ToFloat(coords[1]));
        WorldMapPosition wmposition;
        World2D::GetWorldInfo()->Convert2WorldMapPosition(position, wmposition);
        URHO3D_LOGINFOF("convert2map(%s) => map(%s)", position.ToString().CString(), wmposition.mPoint_.ToString().CString());
    }
    else if (input0 == "random")
    {
        URHO3D_LOGINFOF("random() => seed=%u", ToUInt(inputs[1]));
        GameRand::SetSeedRand(ALLRAND, ToUInt(inputs[1]));
        GameRand::Dump10Value();
    }
    else if (input0 == "anltest")
    {
        URHO3D_LOGINFOF("anltest :");

//        AnlWorldModel* anlmodel = context->GetSubsystem<ResourceCache>()->GetResource<AnlWorldModel>(DATALEVELSDIR + "anltest.xml");
//        AnlWorldModel* anlmodel = context->GetSubsystem<ResourceCache>()->GetResource<AnlWorldModel>(DATALEVELSDIR + "anlworldVM-ellipsoid-zone2.xml");
        AnlWorldModel* anlmodel = GameContext::Get().context_->GetSubsystem<ResourceCache>()->GetResource<AnlWorldModel>(DATALEVELSDIR + "anlworldVM-asteroid1.xml");
        if (!anlmodel)
            return;

        Vector2 modelsize = anlmodel->GetSize();
        unsigned wseed = anlmodel->GetSeed();
        if (inputs.Size() > 1)
            wseed = ToInt(inputs[1]);

        MapGeneratorStatus genstatus;
        genstatus.Clear();
        genstatus.x_ = 0;
        genstatus.y_ = 0;
        genstatus.width_ = 16;
        genstatus.height_ = 16;
        genstatus.wseed_ = wseed;
        genstatus.cseed_ = ShortIntVector2(genstatus.x_, genstatus.y_).ToHash();
        genstatus.rseed_ = genstatus.wseed_ + genstatus.cseed_;
        genstatus.genSpots_ = false;
        genstatus.viewZindexes_[genstatus.activeslot_] = ViewManager::GetViewZIndex(FRONTVIEW);

        AnlMappingRange& mappingRange = genstatus.mappingRange_;
        mappingRange.mapx0 = (float)genstatus.x_ - modelsize.x_ * 0.5f;
        mappingRange.mapx1 = (float)genstatus.x_ + modelsize.x_ * 0.5f;
        mappingRange.mapy0 = (float)genstatus.y_ - modelsize.y_ * 0.5f;
        mappingRange.mapy1 = (float)genstatus.y_ + modelsize.y_ * 0.5f;

        URHO3D_LOGINFOF("anltest start at map=%d %d w=%d h=%d modelsize=%s wseed=%u cseed=%u...", genstatus.x_, genstatus.y_, genstatus.width_, genstatus.height_, modelsize.ToString().CString(), genstatus.wseed_, genstatus.cseed_);

//        genstatus.activeslot_ = TESTMAP;
//        PODVector<float> floatmap;
//        floatmap.Resize(genstatus.width_ * genstatus.height_);
//        genstatus.currentMap_ = &floatmap[0];

        genstatus.activeslot_ = GROUNDMAP;
        PODVector<FeatureType> featuremap;
        featuremap.Resize(genstatus.width_ * genstatus.height_);
        genstatus.currentMap_ = &featuremap[0];

        genstatus.features_[genstatus.activeslot_] = genstatus.currentMap_;

        GameRand::SetSeedRand(MAPRAND, genstatus.rseed_);
        anlmodel->SetSeedAllModules(genstatus.wseed_);

        HiresTimer timer;
        while (!anlmodel->GenerateModules(genstatus, &timer, MAP_MAXDELAY))
        {
            timer.Reset();
        }

        URHO3D_LOGINFOF("anltest dump result :");

        GAME_SETGAMELOGENABLE(GAMELOG_MAPCREATE, true);
        GameHelpers::DumpData(&featuremap[0], 1, 2, genstatus.width_, genstatus.height_);
//        GameHelpers::DumpData(&floatmap[0], -1, 2, genstatus.width_, genstatus.height_);
        GAME_SETGAMELOGENABLE(GAMELOG_MAPCREATE, false);

        URHO3D_LOGINFOF("anltest : ... end !");
    }
    else if (input0 == "pixelshape")
    {
        URHO3D_LOGINFOF("pixelshape : ... start :");
        int type = inputs.Size() > 1 ? ToUInt(inputs[1]) : PIXSHAPE_SPIRAL;
        int width = 10;
        int height = 10;
        PixelShape* shape = GameHelpers::GetPixelShape(type, width, height);
        if (shape)
        {
            GAME_SETGAMELOGENABLE(GAMELOG_MAPCREATE, true);
            GameHelpers::DumpData(&shape->data_[0], 1, 2, width, height);
            GAME_SETGAMELOGENABLE(GAMELOG_MAPCREATE, false);
        }
        URHO3D_LOGINFOF("pixelshape : ... end : shapeptr=%u !", shape);
    }
    else if (input0 == "scrollers")
    {
        URHO3D_LOGINFOF("DrawableScrollers viewport=0 ... Dump ...");
        DrawableScroller::DumpDrawables(0);
    }
}


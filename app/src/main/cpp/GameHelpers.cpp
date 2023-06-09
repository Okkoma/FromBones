#include <iostream>
#include <cstdio>

#include <Urho3D/Urho3D.h>

#include <Urho3D/Core/Context.h>
#include <Urho3D/Container/Ptr.h>

#include <Urho3D/IO/Log.h>
#include <Urho3D/IO/FileSystem.h>

#include <Urho3D/Audio/Audio.h>

#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/Graphics/DebugRenderer.h>
#include <Urho3D/Graphics/Viewport.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/Light.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/StaticModel.h>
#include <Urho3D/Graphics/Texture.h>
#include <Urho3D/Graphics/Texture2D.h>

#include <Urho3D/Resource/Resource.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Resource/XMLFile.h>
#include <Urho3D/Resource/Localization.h>

#include <Urho3D/Input/Input.h>

#include <Urho3D/UI/UI.h>
#include <Urho3D/UI/UIEvents.h>
#include <Urho3D/UI/UIElement.h>
#include <Urho3D/UI/BorderImage.h>
#include <Urho3D/UI/DropDownList.h>
#include <Urho3D/UI/Window.h>
#include <Urho3D/UI/Button.h>
#include <Urho3D/UI/Sprite.h>
#include <Urho3D/UI/Font.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/UI/ListView.h>
#include <Urho3D/UI/ScrollBar.h>

#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/ObjectAnimation.h>
#include <Urho3D/Scene/ValueAnimation.h>
#include <Urho3D/Scene/SmoothedTransform.h>

#include <Urho3D/Urho2D/Renderer2D.h>
#include <Urho3D/Urho2D/AnimatedSprite2D.h>
#include <Urho3D/Urho2D/AnimationSet2D.h>
#include <Urho3D/Urho2D/ParticleEmitter2D.h>
#include <Urho3D/Urho2D/ParticleEffect2D.h>
#include <Urho3D/Urho2D/Sprite2D.h>
#include <Urho3D/Urho2D/SpriteSheet2D.h>
#include <Urho3D/Urho2D/StaticSprite2D.h>
#include <Urho3D/Urho2D/PhysicsWorld2D.h>
#include <Urho3D/Urho2D/RigidBody2D.h>
#include <Urho3D/Urho2D/CollisionShape2D.h>
#include <Urho3D/Urho2D/CollisionBox2D.h>
#include <Urho3D/Urho2D/CollisionCircle2D.h>
#include <Urho3D/Urho2D/CollisionChain2D.h>

#include <Urho3D/Audio/Sound.h>
#include <Urho3D/Audio/SoundSource.h>
#include <Urho3D/Audio/SoundSource3D.h>

#ifndef MPE_POLY2TRI_IMPLEMENTATION
#define MPE_POLY2TRI_IMPLEMENTATION
#endif
#include "Libs/fastpoly2tri.h"

#include "DefsFluids.h"
#include "DefsColliders.h"
#include "DefsNetwork.h"

#include "GameContext.h"
#include "GameOptions.h"
#include "GameAttributes.h"
#include "GameEvents.h"

#include "TimerRemover.h"

#include "GOC_Animator2D.h"
#include "GOC_Collectable.h"
#include "GOC_Destroyer.h"
#include "GOC_ControllerPlayer.h"
#include "GOC_Move2D.h"
#include "GOC_Collide2D.h"
#include "GOC_ZoneEffect.h"
#include "ScrapsEmitter.h"

#include "GEF_NodeShaker.h"

#include "Map.h"
#include "ObjectMaped.h"
#include "RenderShape.h"
#include "MapWorld.h"
#include "MapStorage.h"
#include "ViewManager.h"
#include "GO_Pool.h"
#include "ObjectPool.h"
#include "Player.h"

#include "TextMessage.h"

#include "GameHelpers.h"


void GetDirection(const Vector2& p0, const Vector2& p1, const Vector2& p2, int& dir)
{
    Vector2 v0(p1 - p0);
    Vector2 v1(p2 - p1);
    float cross = Cross(v0, v1);

    if (cross < 0.f)
        dir = -1;
    else if (cross > 0.f)
        dir = 1;
    else
        dir = 0;
}

bool Intersect(const Vector2& x0, const Vector2& x1, const Vector2& y0, const Vector2& y1, Vector2& res)
{
    Vector2 dx = x1 - x0;
    Vector2 dy = y1 - y0;

    // x0 + a dx = y0 + b dy ->
    //   x0 X dx = y0 X dx + b dy X dx ->
    //   b = (x0 - y0) X dx / (dy X dx)

    float dyx = Cross(dy, dx);
    if (!dyx)
        return false;

    dyx = Cross(x0 - y0, dx) / dyx;
    if (dyx <= 0 || dyx >= 1)
        return false;

    res.x_ = y0.x_ + dyx * dy.x_;
    res.y_ = y0.y_ + dyx * dy.y_;
    return true;
}

/// Serialize Data Helpers

int GetEnumValue(const String& value, const char** enumPtr)
{
    // If enums specified, do enum lookup and int assignment. Otherwise assign the variant directly
    bool enumFound = false;
    int enumValue = 0;

    while (*enumPtr)
    {
        if (value.Compare(*enumPtr, false) == 0)
        {
            enumFound = true;
            break;
        }
        ++enumPtr;
        ++enumValue;
    }

    if (enumFound)
        return enumValue;
    else
        return -1;
}

bool IsDigital(const String& value)
{
    if (!value.Length())
        return false;

    const char* tab = value.CString();

    char c = tab[0];

    // check sign first char or dot
    if (c != '-' && c != '.' && !isdigit(c))
        return false;

    // just dot
    bool findDot = c == '.';
    if (findDot && value.Length() == 1)
        return false;

    // check if other chars are digital or dot (stop if more than one dot)
    unsigned i = 1;
    while (i < value.Length())
    {
        if (!findDot)
        {
            c = tab[i];
            if (c != '.' && !isdigit(c))
                return false;
        }
        else if (!isdigit(tab[i]))
            return false;

        i++;
    }

    return true;
}

static unsigned GameLogFilters_;

void GameHelpers::SetGameLogFilter(unsigned logfilter)
{
    GameLogFilters_ = logfilter;
    GameContext::Get().GameLogLock_ = 0;
}

unsigned GameHelpers::GetGameLogFilter()
{
    return GameLogFilters_;
}

void GameHelpers::SetGameLogLevel(int level)
{
    Log* log = GameContext::Get().context_->GetSubsystem<Log>();
    if (log)
    {
        if (level == 0)
        {
            // reset
            level = GameContext::Get().gameConfig_.logLevel_;
            GameContext::Get().GameLogLock_ = 0;
        }

        log->SetLevel(level);
    }
}

int GameHelpers::GetGameLogLevel()
{
    Log* log = GameContext::Get().context_->GetSubsystem<Log>();
    return log ? log->GetLevel() : 0;
}

void GameHelpers::SetGameLogEnable(unsigned filterbits, bool state)
{
    if ((GameLogFilters_ & filterbits) != 0)
    {
        Log* log = GameContext::Get().context_->GetSubsystem<Log>();
        if (!log || log->GetLevel() == LOG_NONE)
            return;

        if (!state)
        {
            GameContext::Get().GameLogLock_++;
            if (GameContext::Get().GameLogLock_ == 1)
            {
                log->SetLevel(GAMELOGLEVEL_MINIMAL);
//                URHO3D_LOGERRORF("GameHelpers() - SetGameLogEnable : filter=%u enable=%u gameloglock=%d !", filterbits, state, GameContext::Get().GameLogLock_);
            }
        }
        else
        {
            GameContext::Get().GameLogLock_--;
            if (GameContext::Get().GameLogLock_ == 0)
            {
                log->SetLevel(GameContext::Get().gameConfig_.logLevel_);
//                URHO3D_LOGERRORF("GameHelpers() - SetGameLogEnable : filter=%u enable=%u gameloglock=%d !", filterbits, state, GameContext::Get().GameLogLock_);
            }
        }
    }
}

void GameHelpers::LoadData(Context* context, const char* fileName, void* data, bool allocate)
{
    URHO3D_LOGINFOF("GameHelpers() - LoadData : from %s ...", fileName);

    File f(context, fileName, FILE_READ);
    if (allocate)
    {
//        data = (void*) new (f.GetSize());
    }
    f.Read(data, f.GetSize());
    f.Close();

    URHO3D_LOGINFO("GameHelpers() - LoadData OK");
}

void GameHelpers::SaveData(Context* context, void* data, unsigned size, const char* fileName)
{
//    URHO3D_LOGINFOF("GameHelpers() - SaveData : to %s ...", fileName);

    File f(context, fileName, FILE_WRITE);
    f.Write(data, size);
    f.Close();

//    URHO3D_LOGINFO("GameHelpers() - SaveData : ... OK");
}

char* GameHelpers::GetTextFileToBuffer(const char* fileName, size_t& s)
{
    FILE* pFile;
    char* newbuffer=0;
    char buf[128];
    unsigned lSize;

    memset(buf,0,128*sizeof(char));

    pFile = fopen(fileName, "r");

    fseek (pFile, 0, SEEK_END);
    lSize = ftell (pFile);
    rewind (pFile);
    newbuffer = (char*) malloc (sizeof(char)*lSize);
    if (!newbuffer) return 0;
    s = lSize;

    rewind(pFile);
    size_t index=0;

    while (fgets(buf, 128, pFile))
    {
        unsigned i=0;
        memcpy(newbuffer+index,buf, strlen(buf));
        index += strlen(buf);
    }

    fclose(pFile);

    return newbuffer;
}

void GameHelpers::SaveOffsetSpriteSheet(SpriteSheet2D* spriteSheet, const IntVector2& offset, const String& filename)
{
    if (!spriteSheet)
        return;

    // offset the sprites and save in xml file

    File f(GameContext::Get().context_, filename.CString(), FILE_WRITE);
    SharedPtr<XMLFile> xml(new XMLFile(GameContext::Get().context_));
    XMLElement rootElem = xml->CreateRoot("TextureAtlas");
    bool success = rootElem.SetAttribute("imagePath", spriteSheet->GetTexture()->GetName());

    unsigned numsprites = 0;
    const HashMap<String, SharedPtr<Sprite2D> >& sprites = spriteSheet->GetSpriteMapping();
    for (HashMap<String, SharedPtr<Sprite2D> >::ConstIterator it=sprites.Begin(); it != sprites.End(); ++it)
    {
        Sprite2D* sprite = it->second_.Get();
        int x = sprite->GetRectangle().left_;
        int y = sprite->GetRectangle().top_;
        int width = sprite->GetRectangle().right_-x;
        int height = sprite->GetRectangle().bottom_-y;

        XMLElement textureElem = rootElem.CreateChild("SubTexture");
        success &= textureElem.SetAttribute("name", it->first_);
        success &= textureElem.SetUInt("x", x + offset.x_);
        success &= textureElem.SetUInt("y", y + offset.y_);
        success &= textureElem.SetUInt("width", width);
        success &= textureElem.SetUInt("height", height);

        if (sprite->GetOffset() != IntVector2::ZERO)
        {
            success &= textureElem.SetInt("frameX", sprite->GetOffset().x_);
            success &= textureElem.SetInt("frameY", sprite->GetOffset().y_);
            success &= textureElem.SetUInt("frameWidth", width);
            success &= textureElem.SetUInt("frameHeight", height);
        }

        numsprites++;
    }

    xml->Save(f, String("\t"));

    f.Close();

    URHO3D_LOGINFOF("GameHelpers() - SaveOffsetSpriteSheet : spriteSheet=%s offset=%s => output=%s ... numsprites=%u success=%s !",
                    spriteSheet->GetName().CString(), offset.ToString().CString(), filename.CString(), numsprites, success ? "true":"false");
}

/// Serialize Scene/Node Helpers

bool GameHelpers::LoadSceneXML(Context* context, Node* node, CreateMode mode)
{
    URHO3D_LOGINFOF("GameHelpers() - LoadSceneXML : loadSavedGame=%s saveDir_=%s", GameContext::Get().loadSavedGame_ ? "true":"false",
                    GameContext::Get().gameConfig_.saveDir_.CString());

    String file;
    String file0(GameContext::Get().gameConfig_.saveDir_ + GameContext::Get().savedSceneFile_);
    String file1(GameContext::Get().sceneToLoad_[GameContext::Get().indexLevel_-1][0]);

    ResourceCache* cache = context->GetSubsystem<ResourceCache>();

    if (GameContext::Get().loadSavedGame_)
    {
        file = file0;
        GameContext::Get().loadSavedGame_ = cache->Exists(file0);
    }

    if (!GameContext::Get().loadSavedGame_)
        file = file1;

    URHO3D_LOGINFOF("GameHelpers() - LoadSceneXML : Loading %s ...", file.CString());

    SharedPtr<XMLFile> xmlFile = cache->GetTempResource<XMLFile>(file);
    if (!xmlFile)
    {
        URHO3D_LOGERRORF("GameHelpers() - LoadSceneXML : Can not find XML file %s !", file.CString());
        return false;
    }

    const XMLElement& xmlNode = xmlFile->GetRoot("node");

    if (!node->LoadXML(xmlNode, mode))
    {
        URHO3D_LOGERRORF("GameHelpers() - LoadSceneXML : Can not Load XML %s file for the node", file.CString());
        return false;
    }

    return true;
}

bool GameHelpers::SaveSceneXML(Context* context, Scene* scene, const char* fileName)
{
    // Save to XML File for test

    URHO3D_LOGINFOF("GameHelpers() - SaveSceneXML : Saving to %s ...", fileName);

    File f(context, fileName, FILE_WRITE);
    if (!scene->SaveXML(f))
    {
        URHO3D_LOGERRORF("GameHelpers() - SaveSceneXML : Can not create XML file %s for the scene !", fileName);
        f.Close();
        return false;
    }

    f.Close();
    return true;
}

bool GameHelpers::SaveNodeXML(Context* context, Node* node, const char* fileName)
{
//    LOGINFOF("GameHelpers() - SaveNodeXML() : file %s ...", fileName);
    if (!node) return false;

    File f(context, fileName, FILE_WRITE);
    if (!node->SaveXML(f))
    {
        URHO3D_LOGERROR("GameHelpers() - SaveNodeXML : Can not create XML file for the node");
        f.Close();
        return false;
    }

    f.Close();
//    URHO3D_LOGINFO("GameHelpers() - SaveNodeXML() : ... OK");

    return true;
}

bool GameHelpers::LoadNodeXML(Context* context, Node* node, const String& fileName, CreateMode mode, bool applyAttr)
{
    XMLFile* xmlFile = context->GetSubsystem<ResourceCache>()->GetResource<XMLFile>(fileName);
    if (!xmlFile)
    {
        URHO3D_LOGERRORF("GameHelpers() - LoadNodeXML : Can not find XML file %s !", fileName.CString());
        return false;
    }

    const XMLElement& xmlNode = xmlFile->GetRoot("node");

    if (!node->LoadXML(xmlNode, mode, false, applyAttr))
    {
        URHO3D_LOGERRORF("GameHelpers() - LoadNodeXML : Can not Load XML %s file for the node", fileName.CString());
        return false;
    }

    URHO3D_LOGINFOF("GameHelpers() - LoadNodeXML : Loading %s on node %u... OK !", fileName.CString(), node->GetID());
    return true;
}



/// Node Attributes Helpers

//#define LoadNodeAttributes_ActiveLog_1
//#define LoadNodeAttributes_ActiveLog_2
//#define LoadNodeAttributes_ActiveLog_3
//#define CopyNodeAttributes_ActiveLog_1
//#define CopyNodeAttributes_ActiveLog_2
//#define CopyNodeAttributes_ActiveLog_3

void LoadAttributes(Serializable* seria, const VariantMap& attrMap)
{
    const Vector<AttributeInfo>* attributes = seria->GetAttributes();

    if (attributes)
    {
        for (unsigned j = 0; j < attributes->Size(); ++j)
        {
            const AttributeInfo& attr = attributes->At(j);

#ifdef LoadNodeAttributes_ActiveLog_2
            URHO3D_LOGERRORF("  -> Attribute %s", attr.name_.CString());
#endif
            VariantMap::ConstIterator it = attrMap.Find(StringHash(attr.name_));
            if (it != attrMap.End())
            {
                const Variant& value = it->second_;
#ifdef LoadNodeAttributes_ActiveLog_3
                URHO3D_LOGERRORF("    -> Value(%s)=%s", value.GetTypeName().CString(), value.ToString().CString());
#endif
                seria->OnSetAttribute(attr, value);
            }
        }
    }
}

void SaveAttributes(Serializable* seria, VariantMap& attrMap, Variant& value)
{
    const Vector<AttributeInfo>* attributes = seria->GetAttributes();

    if (attributes)
    {
        for (unsigned j = 0; j < attributes->Size(); ++j)
        {
            const AttributeInfo& attr = attributes->At(j);
            if (!(attr.mode_ & AM_FILE))
                continue;

#ifdef LoadNodeAttributes_ActiveLog_2
            URHO3D_LOGERRORF("  -> Attribute %s", attr.name_.CString());
#endif

            seria->OnGetAttribute(attr, value);
#ifdef LoadNodeAttributes_ActiveLog_3
            URHO3D_LOGERRORF("    -> Value(%s)=%s", value.GetTypeName().CString(), value.ToString().CString());
#endif

            attrMap[StringHash(attr.name_)] = value;
        }
    }
}

void GameHelpers::LoadNodeAttributes(Node* node, const NodeAttributes& nodeAttr, bool applyAttr)
{
    /// node must have the same composition than nodeAttr

#ifdef LoadNodeAttributes_ActiveLog_1
    URHO3D_LOGERRORF("GameHelpers() - LoadNodeAttributes : node %s(%u) ...", node->GetName().CString(), node->GetID());
#endif

    /// Set node attributes in node
    {
        const VariantMap& attrMap = nodeAttr[0];

        LoadAttributes(node, attrMap);
    }

    /// Set components attributes in node
    if (nodeAttr.Size() >= node->GetComponents().Size())
    {
        int index=0;
        const Vector<SharedPtr<Component> >& components = node->GetComponents();
        for (Vector<SharedPtr<Component> >::ConstIterator it = components.Begin(); it != components.End(); ++it)
        {
            Component* component = *it;

            if (component->IsTemporary())
                continue;

#ifdef LoadNodeAttributes_ActiveLog_2
            URHO3D_LOGERRORF(" -> Component %s", component->GetTypeName().CString());
#endif

            if (index >= nodeAttr.Size())
            {
                URHO3D_LOGERRORF("GameHelpers() - LoadNodeAttributes : node %s(%u) -> Component %s NodeAttr.Size=%u < index=%u ... Error !", node->GetName().CString(), node->GetID(), component->GetTypeName().CString(), nodeAttr.Size(), index);
                continue;
            }

            index++;
            const VariantMap& attrMap = nodeAttr[index];

            LoadAttributes(component, attrMap);
        }
    }

#ifdef LoadNodeAttributes_ActiveLog_1
    URHO3D_LOGERRORF("GameHelpers() - LoadNodeAttributes : node %s(%u) ... OK !", node->GetName().CString(), node->GetID());
#endif

    if (applyAttr)
        node->ApplyAttributes();
}

void GameHelpers::SaveNodeAttributes(Node* node, NodeAttributes& nodeAttr)
{
    nodeAttr.Clear();

    Variant value;

    /// Get node attributes
    {
        /// create default attributes nodeAttr[0]
        nodeAttr.Push(VariantMap());
        VariantMap& attrMap = nodeAttr.Back();

#ifdef LoadNodeAttributes_ActiveLog_1
        URHO3D_LOGERRORF("GameHelpers() - SaveNodeAttributes node %s(%u) ...", node->GetName().CString(), node->GetID());
#endif

        SaveAttributes(node, attrMap, value);
    }

    /// Get components attributes
    {
        const Vector<SharedPtr<Component> >& components = node->GetComponents();

        for (Vector<SharedPtr<Component> >::ConstIterator it = components.Begin(); it != components.End(); ++it)
        {
            Component* component = *it;

            if (component->IsTemporary())
                continue;

#ifdef LoadNodeAttributes_ActiveLog_2
            URHO3D_LOGERRORF(" -> Component %s", component->GetTypeName().CString());
#endif

            nodeAttr.Push(VariantMap());
            VariantMap& attrMap = nodeAttr.Back();

            SaveAttributes(component, attrMap, value);
        }
    }

#ifdef LoadNodeAttributes_ActiveLog_1
    URHO3D_LOGERRORF("GameHelpers() - SaveNodeAttributes node %s(%u) ... OK !", node->GetName().CString(), node->GetID());
#endif
}

const Variant& GameHelpers::GetNodeAttributeValue(const StringHash& attr, const NodeAttributes& nodeAttr, unsigned index)
{
    /// NodeAttributes = Vector<VariantMap >
    /// index=0 => get in node attributes
    /// index>0 => get in component attributes

    if (index >= nodeAttr.Size())
        return Variant::EMPTY;

    VariantMap::ConstIterator it = nodeAttr[index].Find(attr);

    return it != nodeAttr[index].End() ? it->second_ : Variant::EMPTY;
}

Vector<unsigned> GameHelpers::sRemovableComponentsOnCopyAttributes_;

void GameHelpers::CopyAttributes(Node* source, Node* dest, bool apply, bool removeUnusedComponents)
{
#ifdef CopyNodeAttributes_ActiveLog_1
    URHO3D_LOGERRORF("GameHelpers() - CopyAttributes : source=%s(%u) to dest=%s(%u) ...", source->GetName().CString(), source->GetID(), dest->GetName().CString(), dest->GetID());
#endif

    PODVector<Node*> srcnodes;
    PODVector<Node*> destnodes;
    source->GetChildren(srcnodes, false);

    if (removeUnusedComponents)
        dest->RemoveAllChildren();
    else
        dest->GetChildren(destnodes, false);

    srcnodes.Insert(0, source);
    destnodes.Insert(0, dest);

    SceneResolver resolver;

    for (unsigned n=0; n < srcnodes.Size(); n++)
    {
        Node* srcnode = srcnodes[n];

        if (n >= destnodes.Size())
            destnodes.Push(dest->CreateChild(String::EMPTY, dest->GetID() >= FIRST_LOCAL_ID ? LOCAL : REPLICATED));

#ifdef CopyNodeAttributes_ActiveLog_1
        URHO3D_LOGERRORF("> srcnode[%u] ... copy attributes ...", n);
#endif

        Node* destnode = destnodes[n];

        /// Copy node attributes
        const Vector<AttributeInfo>* attributes = srcnode->GetAttributes();
        for (unsigned j = 0; j < attributes->Size(); ++j)
        {
            const AttributeInfo& attr = attributes->At(j);
            // Do not copy network-only attributes, as they may have unintended side effects
            if (attr.mode_ & AM_FILE)
            {
                Variant value;
                srcnode->OnGetAttribute(attr, value);
#ifdef CopyNodeAttributes_ActiveLog_1
                URHO3D_LOGERRORF("-> attr[%u] %s=%s ...", j, attr.name_.CString(), value.ToString().CString());
#endif
                destnode->OnSetAttribute(attr, value);
            }
        }

        const Vector<SharedPtr<Component> >& tcomponents = srcnode->GetComponents();
        const Vector<SharedPtr<Component> >& components = destnode->GetComponents();
        unsigned componentSize = components.Size();

#ifdef CopyNodeAttributes_ActiveLog_1
        URHO3D_LOGERRORF("-> Num Template Components=%u, Num Dest Components=%u", tcomponents.Size(), componentSize);
#endif

        /// copy components attributes
        {
            unsigned j = 0;
            Vector<SharedPtr<Component> >::ConstIterator it = tcomponents.Begin();
            for (;;)
            {
                Component* tcomponent = it != tcomponents.End() ? it->Get() : 0;
                Component* component = j < components.Size() ? components[j].Get() : 0;

                if (!tcomponent)
                    break;

                if (tcomponent->IsTemporary())
                {
#ifdef CopyNodeAttributes_ActiveLog_1
                    URHO3D_LOGERRORF("-> Skip Temporary Component=%s(%u)", tcomponent->GetTypeName().CString(), tcomponent->GetID());
#endif
                    it++;
                    continue;
                }

                if (component && component->IsTemporary())
                {
                    j++;
                    continue;
                }

                // Set Attributes
                if (component && tcomponent->GetType() == component->GetType())
                {
#ifdef CopyNodeAttributes_ActiveLog_1
                    URHO3D_LOGERRORF("-> Template Component=%s(%u), Dest Components=%s(%u)", tcomponent ? tcomponent->GetTypeName().CString() : "", tcomponent ? tcomponent->GetID() : 0,
                                     component ? component->GetTypeName().CString() : "", component ? component->GetID() : 0);
#endif

                    const Vector<AttributeInfo>* tAttributes = component->GetAttributes();

                    if (tAttributes)
                    {
                        if (tAttributes->Size() == 0)
                        {
                            it++;
                            j++;
                            continue;
                        }

#ifdef CopyNodeAttributes_ActiveLog_2
                        URHO3D_LOGERRORF(" -> Copy %s attributes size=%u", tcomponent->GetTypeName().CString(), tAttributes->Size());
#endif

                        for (unsigned i = 0; i < tAttributes->Size(); ++i)
                        {
                            const AttributeInfo& tattr = tAttributes->At(i);
                            if (tattr.mode_ & AM_FILE)
                            {
                                Variant value;
                                tcomponent->OnGetAttribute(tattr, value);
                                component->OnSetAttribute(tattr, value);
#ifdef CopyNodeAttributes_ActiveLog_3
                                URHO3D_LOGERRORF("   -> Attribute=%s(%s) Value=%s", tattr.name_.CString(), tattr.name_.CString(), value.ToString().CString());
#endif
                            }
                        }
                    }
                }
                // Clone Component
                else
                {
                    component = destnode->CloneComponent(tcomponent, tcomponent->GetID() < FIRST_LOCAL_ID ? REPLICATED : LOCAL, 0, apply);
                    // clone must be inserted at the same index than template
                    destnode->ReorderComponent(components.Size()-1, j);
                    if (component)
                    {
                        resolver.AddComponent(tcomponent->GetID(), component);
#ifdef CopyNodeAttributes_ActiveLog_1
                        URHO3D_LOGERRORF("-> Clone Template Component=%s(%u)", tcomponent->GetTypeName().CString(), tcomponent->GetID());
#endif
                    }
                }

                it++;
                j++;
            }
        }

#ifdef CopyNodeAttributes_ActiveLog_1
        {
            URHO3D_LOGERRORF("-> Dump Dest before remove : start Dest size = %u (after add clones=%u)", componentSize, destnode->GetComponents().Size());
            for (unsigned i = 0; i < destnode->GetComponents().Size(); i++)
                URHO3D_LOGERRORF(" -> Dest Component[%d] %s(%u)", i, destnode->GetComponents()[i]->GetTypeName().CString(), destnode->GetComponents()[i]->GetID());
        }
#endif

        /// Remove components that are marked Removable
        if (removeUnusedComponents && GameHelpers::sRemovableComponentsOnCopyAttributes_.Size())// && tcomponents.Size() < destnode->GetComponents().Size())
        {
            PODVector<Component*> componentToremove;

            for (unsigned i = tcomponents.Size(); i < destnode->GetComponents().Size(); i++)
            {
                if (GameHelpers::sRemovableComponentsOnCopyAttributes_.Contains(destnode->GetComponents()[i]->GetType().Value()))
                    componentToremove.Push(destnode->GetComponents()[i].Get());
            }

#ifdef CopyNodeAttributes_ActiveLog_1
            URHO3D_LOGERRORF("-> Num Dest Components to remove=%u", componentToremove.Size());
#endif

            if (componentToremove.Size())
            {
                for (int i = componentToremove.Size()-1; i >= 0 ; i--)
                {
#ifdef CopyNodeAttributes_ActiveLog_1
                    URHO3D_LOGERRORF(" -> Dest Remove Component %s(%u)", componentToremove[i]->GetTypeName().CString(), componentToremove[i]->GetID());
#endif
                    destnode->RemoveComponent(componentToremove[i]);
                }
            }
        }
    }

    resolver.Resolve();

    if (apply)
        dest->ApplyAttributes();

#ifdef CopyNodeAttributes_ActiveLog_1
    URHO3D_LOGERRORF("GameHelpers() - CopyAttributes ... OK ! ");
#endif
}



/// Scene/Node Remover Helpers

/// clean scene and keep the nodes preloader and pools
bool GameHelpers::CleanScene(Scene* rootscene, const String& stateToClean, int step)
{
    URHO3D_LOGINFOF("GameHelpers() - CleanScene : ... state=%s step=%d", stateToClean.CString(), step);

    if (!rootscene)
    {
        URHO3D_LOGERROR("GameHelpers() - CleanScene : no RootScene !");
        return false;
    }

//    Context* context = rootscene->GetContext();

    GAME_SETGAMELOGENABLE(GAMELOG_PRELOAD, false);

    bool initialPhysicIsUpdateEnable = false;

    // Stop the physics
    PhysicsWorld2D* physicworld = rootscene->GetComponent<PhysicsWorld2D>();
    if (physicworld)
    {
        initialPhysicIsUpdateEnable = physicworld->IsUpdateEnabled();
        physicworld->SetUpdateEnabled(false);
    }

    if (!step)
    {
        URHO3D_LOGINFOF("GameHelpers() - CleanScene : ... CleanupNetwork ...");

        rootscene->CleanupNetwork();

        URHO3D_LOGINFOF("GameHelpers() - CleanScene : ... ObjectPool Restore ...");

        if (ObjectPool::Get())
            ObjectPool::Get()->RestoreCategories();
        else
            URHO3D_LOGWARNINGF(" ... no ObjectPool to restore !");

        URHO3D_LOGINFOF("GameHelpers() - CleanScene : ... GO_Pools Restore ...");

        GO_Pools::Restore();

        PODVector<Node*> nodesToRemove;

        if (stateToClean == "Play")
        {
            nodesToRemove.Push(rootscene->GetChild("Scene"));
            nodesToRemove.Push(rootscene->GetChild("Controllables"));
        }

        nodesToRemove.Push(rootscene->GetChild("LocalScene"));
//        nodesToRemove.Push(rootscene->GetChild("StaticScene"));

        URHO3D_LOGINFOF("GameHelpers() - CleanScene : ... Removing Temp Nodes ...");

        for (unsigned i=0; i < nodesToRemove.Size(); ++i)
            if (nodesToRemove[i])
                GameHelpers::RemoveNodeSafe(nodesToRemove[i], false);

        /// TODO : Test this with network
        // Clear Replicated Nodes
        rootscene->Clear(true, false);
//        rootscene_->Clear(!GameContext::Get().ClientMode_, false);

        /// For Debug
        GameHelpers::DumpNode(rootscene, 2, true);

        URHO3D_LOGINFOF("GameHelpers() - CleanScene : ... stateToClean=%s step=0 OK !", stateToClean.CString());
    }
    else
    {
//        /// For Debug
//        GameHelpers::DumpNode(rootscene, 2, true);

        PODVector<Node*> nodesToRemove;

        if (stateToClean != "Play")
            nodesToRemove.Push(rootscene->GetChild(stateToClean));

        for (unsigned i=0; i < nodesToRemove.Size(); ++i)
            if (nodesToRemove[i])
                GameHelpers::RemoveNodeSafe(nodesToRemove[i], false);

//        /// For Debug
//        GameHelpers::DumpNode(rootscene, 2, true);

        URHO3D_LOGINFOF("GameHelpers() - CleanScene : ... stateToClean=%s step=1 OK !", stateToClean.CString());
    }

    // Start the physics
    if (initialPhysicIsUpdateEnable)
        rootscene->GetComponent<PhysicsWorld2D>()->SetUpdateEnabled(true);

    GAME_SETGAMELOGENABLE(GAMELOG_PRELOAD, true);

    return true;
}

void ScanNodeRecursive(Node* node, Vector<WeakPtr<Node> >& nodes, Vector<WeakPtr<Component> >& components)
{
    if (!node)
        return;

    nodes.Push(WeakPtr<Node>(node));

    // Scan Components
    for (Vector<SharedPtr<Component> >::ConstIterator it=node->GetComponents().Begin(); it!=node->GetComponents().End(); ++it)
        if (*it)
            components.Push(WeakPtr<Component>(*it));

    // Scan Childrens
    for (Vector<SharedPtr<Node> >::ConstIterator it=node->GetChildren().Begin(); it!=node->GetChildren().End(); ++it)
        if (*it)
            ScanNodeRecursive(*it, nodes, components);
}

void GameHelpers::RemoveNodeSafe(Node* node, bool dumpnodes)
{
    if (!node)
        return;

    Vector<WeakPtr<Node> > nodes;
    Vector<WeakPtr<Component> > components;

    ScanNodeRecursive(node, nodes, components);

    URHO3D_LOGINFOF("GameHelpers() - RemoveNodeSafe : name=%s id=%u numref=%d(w=%d) numNodes=%u numComponents=%u ...",
                    node->GetName().CString(), node->GetID(), node->Refs(), node->WeakRefs(), nodes.Size(), components.Size());

    if (dumpnodes)
        GameHelpers::DumpNode(node, 50, true);

    // Remove All Components
    for (unsigned i=components.Size()-1; i < components.Size(); --i)
        if (components[i])
            components[i]->Remove();
        else
            URHO3D_LOGINFOF("GameHelpers() - RemoveNodeSafe : c%u Already Empty ... OK !", i);

    // Remove All Nodes
    for (unsigned i=nodes.Size()-1; i < nodes.Size(); --i)
        if (nodes[i])
            nodes[i]->Remove();
        else
            URHO3D_LOGINFOF("GameHelpers() - RemoveNodeSafe : n%u Already Empty ... OK !", i);

    URHO3D_LOGINFOF("GameHelpers() - RemoveNodeSafe : nodesRemoved=%u componentsRemoved=%u ... OK !", nodes.Size(), components.Size());
}



/// Spawn Node Helpers

void GameHelpers::SpawnGOtoNode(Context* context, StringHash type, Node* node)
{
    const String& fileName = GOT::GetObjectFile(type);
    if (fileName.Empty())
    {
        URHO3D_LOGERRORF("GameHelpers() - SpawnGOtoNode : No XML file for the type %s(%u)", GOT::GetType(type).CString(),type.Value());
        return;
    }
    ResourceCache* cache = context->GetSubsystem<ResourceCache>();
    XMLFile* xmlFile = cache->GetResource<XMLFile>(fileName);
    const XMLElement& xmlNode = xmlFile->GetRoot("node");
    if (!node->LoadXML(xmlNode))
        URHO3D_LOGERROR("GameHelpers() - SpawnGOtoNode : Can not Load XML file for the node");
}

void GameHelpers::SpawnCollectableFromSlot(Context *context, Slot& slot, Node* node, unsigned int qty)
{
    // Create from XML File
    if (GOT::HasObject(slot.type_))
    {
        SpawnGOtoNode(context, slot.type_, node);
    }
    // Or, create with default Slot Sprite
    else
    {
        if (!slot.sprite_)
        {
            URHO3D_LOGERROR("GameHelpers() - SpawnCollectableFromSlot : No sprite in slot !");
            return;
        }
        node->SetWorldScale(slot.scale_);

        ResourceCache* cache = context->GetSubsystem<ResourceCache>();

        StaticSprite2D* staticSprite = node->CreateComponent<StaticSprite2D>();
        staticSprite->SetSprite(slot.sprite_);
#ifdef ACTIVE_LAYERMATERIALS
        staticSprite->SetCustomMaterial(GameContext::Get().layerMaterials_[LAYERACTORS]);
#endif
        //staticSprite->SetHotSpot(slot.hotspot_);
        staticSprite->SetColor(slot.color_);
        staticSprite->SetLayer2(IntVector2(1000,-1));

        Vector3 size = staticSprite->GetWorldBoundingBox().Size();
        Vector3 center = staticSprite->GetBoundingBox().Center();

        RigidBody2D *r = node->CreateComponent<RigidBody2D>();
        r->SetBodyType(BT_DYNAMIC);
        r->SetFixedRotation(false);
        r->SetMass(0.5f * size.x_ * size.y_);

        CollisionCircle2D* c = node->CreateComponent<CollisionCircle2D>();
        c->SetRadius(Min(size.x_, size.y_) * 0.5f * 0.6f);
        c->SetCenter(center.x_,center.y_);
        c->SetFriction(0.2f);
        c->SetExtraContactBits(CONTACT_STABLE);
    }

    node->SetName(GOT::GetType(slot.type_));

    Slot* newslot = node->GetOrCreateComponent<GOC_Collectable>()->GetSlot();
    newslot->quantity_ = 0U;

//    // get one item if apply to a holder
//    GOC_ZoneEffect* zoneEffect = node->GetComponent<GOC_ZoneEffect>();
//    if (zoneEffect)
//    {
//        if (zoneEffect->GetApplyToHolder())
//            qty = 1;
//    }

    qty = slot.TransferTo(newslot, qty);

    node->GetOrCreateComponent<GOC_Destroyer>();
}

void GameHelpers::SpawnParticleEffect(Context* context, const String& effectName, int layer, int viewmask, const Vector2& position, float angle, float worldScale, bool removeTimer, float duration, const Color& color, CreateMode mode, Material* material, const float zf)
{
#ifdef USE_PARTICULES
    if (effectName.Empty())
        return;

    // Create the particle emitter
    SharedPtr<ParticleEffect2D> particleEffect(context->GetSubsystem<ResourceCache>()->GetResource<ParticleEffect2D>(effectName));
    if (!particleEffect)
    {
        URHO3D_LOGERRORF("GameHelpers() - SpawnParticleEffect : no particle Effect %s", effectName.CString());
        return;
    }

    Node* newNode = GameContext::Get().rootScene_->CreateChild("PE", mode);
    newNode->SetTemporary(true);
    newNode->SetWorldTransform2D(position, angle, Vector2::ONE * worldScale);
    if (zf > 0.f)
        newNode->SetWorldPosition(Vector3(newNode->GetWorldPosition2D(), zf));

    particleEffect = particleEffect->Clone();

    if (color != Color::WHITE)
    {
        particleEffect->SetStartColor(color);
        particleEffect->SetFinishColor(Color::TRANSPARENT);
    }

    if (angle > 90.f)
        particleEffect->SetGravity(-particleEffect->GetGravity());

    // angle is already set by worldtransform (don't set Angle for particle)
    if (angle)
    {
        particleEffect->SetRotationStart(particleEffect->GetRotationStart()+angle);
        particleEffect->SetRotationEnd(particleEffect->GetRotationEnd()+angle);
    }

    ParticleEmitter2D* particleEmitter = newNode->CreateComponent<ParticleEmitter2D>();
    particleEmitter->SetLooped(!removeTimer && duration == -1.f);
    particleEmitter->SetEffect(particleEffect);
    particleEmitter->SetLayer2(IntVector2(layer,-1));
    particleEmitter->SetViewMask(viewmask);
//    particleEmitter->SetColor(color);
    int blendmode = particleEffect->GetBlendMode();
#ifdef ACTIVE_LAYERMATERIALS
    if (!material)
        particleEmitter->SetCustomMaterial(blendmode == BLEND_ALPHA ? GameContext::Get().layerMaterials_[LAYERACTORS] : GameContext::Get().layerMaterials_[LAYEREFFECTS]);
    else
        particleEmitter->SetCustomMaterial(material);
#endif
    particleEmitter->SetOrderInLayer(blendmode == BLEND_ALPHA ? 1000 : 2000);
    particleEmitter->SetTextureFX(UNLIT); // UNLIT

    if (removeTimer)
    {
        if (duration == -1.0f)
            duration = particleEffect->GetDuration();

        // add alpha fade out
        SharedPtr<ObjectAnimation> objectAnimation(new ObjectAnimation(context));
        SharedPtr<ValueAnimation> colorAnimation(new ValueAnimation(context));
        colorAnimation->SetKeyFrame(0.f, 1.f);
        colorAnimation->SetKeyFrame(duration, 0.f);
        objectAnimation->AddAttributeAnimation("Alpha", colorAnimation, WM_ONCE);
        particleEmitter->SetObjectAnimation(objectAnimation);

        TimerRemover::Get()->Start(newNode, duration + particleEffect->GetParticleLifeSpan());
    }

//    URHO3D_LOGERRORF("GameHelpers() - SpawnParticleEffect : %s node=%u at position=%s(entry=%s)", effectName.CString(), newNode->GetID(), newNode->GetWorldPosition2D().ToString().CString(), position.ToString().CString());
#endif // USE_PARTICULES
}

Node* GameHelpers::SpawnParticleEffectInNode(Context* context, Node* node, const String& effectName, int layer, int viewmask, const Vector2& wposition, float angle, float fscale, bool removeTimer, float duration, const Color& color, CreateMode mode)
{
#ifdef USE_PARTICULES
    if (effectName.Empty())
        return 0;

    // Create the particle emitter
    SharedPtr<ParticleEffect2D> particleEffect(context->GetSubsystem<ResourceCache>()->GetResource<ParticleEffect2D>(effectName));
    if (!particleEffect)
    {
        URHO3D_LOGERRORF("GameHelpers() - SpawnParticleEffectInNode : no particle Effect %s", effectName.CString());
        return 0;
    }

    Node* newNode = node->CreateChild("PE", mode);
    newNode->SetTemporary(true);
    newNode->SetWorldTransform2D(wposition != Vector2::ZERO ? wposition : node->GetWorldPosition2D(), angle, node->GetWorldScale2D() * fscale);

    particleEffect = particleEffect->Clone();

    if (color != Color::WHITE)
    {
        particleEffect->SetStartColor(color);
        particleEffect->SetFinishColor(Color::TRANSPARENT);
    }

    // angle is already set by worldtransform (don't set Angle for particle)
    if (angle)
    {
        if (angle > 90.f)
            particleEffect->SetGravity(-particleEffect->GetGravity());
        particleEffect->SetRotationStart(particleEffect->GetRotationStart()+angle);
        particleEffect->SetRotationEnd(particleEffect->GetRotationEnd()+angle);
    }

    ParticleEmitter2D* particleEmitter = newNode->CreateComponent<ParticleEmitter2D>();
    particleEmitter->SetLooped(!removeTimer && duration == -1.f);
    particleEmitter->SetEffect(particleEffect);
    particleEmitter->SetLayer2(IntVector2(layer,-1));
    particleEmitter->SetViewMask(viewmask);
    int blendmode = particleEffect->GetBlendMode();
#ifdef ACTIVE_LAYERMATERIALS
    particleEmitter->SetCustomMaterial(blendmode == BLEND_ALPHA ? GameContext::Get().layerMaterials_[LAYERACTORS] : GameContext::Get().layerMaterials_[LAYEREFFECTS]);
#endif
    particleEmitter->SetOrderInLayer(blendmode == BLEND_ALPHA ? 1000 : 2000);

    if (removeTimer)
    {
        if (duration == -1.f)
            duration = particleEffect->GetDuration();

        // add alpha fade out
        SharedPtr<ObjectAnimation> objectAnimation(new ObjectAnimation(context));
        SharedPtr<ValueAnimation> colorAnimation(new ValueAnimation(context));
        colorAnimation->SetKeyFrame(0.f, 1.f);
        colorAnimation->SetKeyFrame(duration, 0.f);
        objectAnimation->AddAttributeAnimation("Alpha", colorAnimation, WM_ONCE);
        particleEmitter->SetObjectAnimation(objectAnimation);

        TimerRemover::Get()->Start(newNode, duration + particleEffect->GetParticleLifeSpan());
    }

//    URHO3D_LOGERRORF("GameHelpers() - SpawnParticleEffectInNode : node=%s(%u) effect=%s layer=%d !", node->GetName().CString(), node->GetID(), effectName.CString(), layer);

    return newNode;
#else
    return 0;
#endif
}

void GameHelpers::TransferPlayersToMap(const ShortIntVector2& mPoint)
{
    if (!World2D::GetWorld())
        return;

    // check if in world bounds
    if (!World2D::IsInsideWorldBounds(mPoint))
        return;

    IntVector2 dMap(mPoint.x_, mPoint.y_);
    IntVector2 dPosition;
    int dViewZ = ViewManager::Get()->GetCurrentViewZ(0);

    URHO3D_LOGINFOF("GameHelpers() - TransferPlayerToMap : map=%s ...",  mPoint.ToString().CString());

    ViewManager::Get()->SetFocusEnable(false);

    // Just Load instant map (no move camera)
    World2D::GetWorld()->GoToMap(mPoint, 0);
    World2D::GetWorld()->UpdateInstant(0, Vector2::ZERO, 0.f, false);

    // Find a default start position on this map
    Node* destinationArea = GameContext::Get().FindMapPositionAt("GOT_START", mPoint, dPosition, dViewZ);
    if (!destinationArea)
    {
        // Reset to Default Point
        dPosition.x_ = 1;
        dPosition.y_ = 2;
        if (dViewZ != INNERVIEW)
            dViewZ = FRONTVIEW;
    }

    if (GameContext::Get().gameConfig_.multiviews_)
    {
        // Transfer Players
        for (int i=0; i < GameContext::Get().numPlayers_; i++)
        {
            if (GameContext::Get().playerAvatars_[i])
            {
                int viewport = i;

                GameContext::Get().players_[i]->SetWorldPosition(WorldMapPosition(World2D::GetWorldInfo(), dMap, dPosition+IntVector2(i,0), dViewZ));

                // Transfer Camera
                World2D::GetWorld()->GoToMap(mPoint, dPosition, dViewZ, viewport);
            }
        }

        for (int i=0; i < GameContext::Get().numPlayers_; i++)
            if (GameContext::Get().playerAvatars_[i])
                World2D::GetWorld()->GoCameraToDestinationMap(i, false);
    }
    else
    {
        // Transfer Players
        for (int i=0; i < GameContext::Get().numPlayers_; i++)
            if ( GameContext::Get().playerAvatars_[i])
                GameContext::Get().players_[i]->SetWorldPosition(WorldMapPosition(World2D::GetWorldInfo(), dMap, dPosition+IntVector2(i,0), dViewZ));

        // Transfer Camera
        World2D::GetWorld()->GoToMap(mPoint, dPosition, dViewZ, 0);

        World2D::GetWorld()->UpdateStep(0.f);

        World2D::GetWorld()->GoCameraToDestinationMap(0, false);
    }

    ViewManager::Get()->SetFocusEnable(true);

    World2D::GetWorld()->SendEvent(WORLD_CAMERACHANGED);
    GameContext::Get().rootScene_->SendEvent(WORLD_MAPUPDATE);

    URHO3D_LOGINFOF("GameHelpers() - TransferPlayerToMap : map=%s dviewZ=%d... OK !", mPoint.ToString().CString(), dViewZ);

    MapStorage::Get()->DumpMapsMemory();
}

bool GameHelpers::SpawnScraps(const StringHash& type, unsigned numscraps, const Color& color, bool iscollider, int layer, int viewZ, int viewMask, const Vector2& position, const Vector2& wscale, float maxsize, float lifetime, float impulse, float density)
{
    if (!GameContext::Get().rootScene_)
        return false;

    // 30/12/2019 : Patch a crash
    if (type.Value() == 0)
        return true;

    /// Create the Scraps Nodes with RigidBodies on the fly

//    URHO3D_LOGINFOF("GameHelpers() - SpawnScraps : ...");

    const Vector<WeakPtr<Sprite2D> >& spriteList = ScrapsEmitter::GetScrapSpriteList(type);

    Vector2 force;
    StaticSprite2D* staticSprite;
    Node* node;
    Sprite2D* sprite;
    RigidBody2D* body;
    CollisionCircle2D* shape;
    Vector3 bbsize;
    IntVector2 size;
    Rect drawRect, textureRect;
    float scale, r1, r2, r3;
    int categoryBits, maskBits;

    maxsize /= (float) numscraps;

    if (!iscollider)
    {
        if (viewZ == INNERVIEW)
        {
            categoryBits = CC_INSIDEEFFECT;
            maskBits = CM_INSIDEEFFECT;
        }
        else
        {
            categoryBits = CC_OUTSIDEEFFECT;
            maskBits = CM_OUTSIDEEFFECT;
        }
    }
    else
    {
        if (viewZ == INNERVIEW)
        {
            categoryBits = CC_INSIDEMONSTER;
            maskBits = CM_INSIDEMONSTER;
        }
        else
        {
            categoryBits = CC_OUTSIDEMONSTER;
            maskBits = CM_OUTSIDEMONSTER;
        }
    }

    for (unsigned i=0; i < numscraps; i++)
    {
        int entityid = 0;
        node = ObjectPool::CreateChildIn(GOT::PART, entityid, GameContext::Get().rootScene_);

        if (!node)
            break;

        node->SetEnabled(true);

        r1 = Random(0.5f, 1.f);
        r2 = Random(-1.f, 1.f);
        r3 = Random(-1.f, 1.f);

//        URHO3D_LOGINFOF("size=%s Scrapsize=%s numScraps=%u", size_.ToString().CString(), size.ToString().CString(), numscraps);

        /// Set Random Sprite
        sprite = spriteList[i % spriteList.Size()];
        size = sprite->GetRectangle().Size();
        scale = Min(Max(0.5f, maxsize / (float) Min(size.x_, size.y_)), 1.f);

        staticSprite = node->GetComponent<StaticSprite2D>();
#ifdef ACTIVE_LAYERMATERIALS
        staticSprite->SetCustomMaterial(GameContext::Get().layerMaterials_[LAYERACTORS]);
#endif
        staticSprite->SetSprite(sprite);
        staticSprite->SetColor(color);
        staticSprite->SetFlip(r2 > 0.f, r3 > 0.f);
        staticSprite->SetLayer2(IntVector2(layer,-1));
        staticSprite->SetViewMask(viewMask);

        staticSprite->SetUseHotSpot(true);
        staticSprite->SetHotSpot(Vector2::HALF);
        sprite->GetDrawRectangle(drawRect, staticSprite->GetHotSpot(), staticSprite->GetFlipX(), staticSprite->GetFlipY());
        sprite->GetTextureRectangle(textureRect, staticSprite->GetFlipX(), staticSprite->GetFlipY());

        staticSprite->SetUseDrawRect(true);
        staticSprite->SetDrawRect(drawRect);
        staticSprite->SetUseTextureRect(true);
        staticSprite->SetTextureRect(textureRect);

        bbsize = drawRect.Size();;// / sprite->GetTexture()->GetDpiRatio();

        /// Initial Physic Properties
//        bbsize = staticSprite->GetWorldBoundingBox().Size();
        force.x_ = r1 * impulse * Sign(r2);
        force.y_ = r1 * impulse * Sign(r3);

        /// Set RigidBody
        body = node->GetComponent<RigidBody2D>();
        body->SetFixedRotation(false);
        body->SetUseFixtureMass(false);
        body->SetMass(Clamp(bbsize.x_ * bbsize.y_, 0.25f, 5.f));
        body->SetWorldTransform(position + Vector2(maxsize*r2, maxsize*r1)*0.01f, 45.f * r2, wscale * scale);
        body->SetLinearDamping(1.f);
        body->SetAngularDamping(5.f);

        body->ApplyForce(force, position, true);
        body->SetAngularVelocity(force.x_ * force.y_);

        /// Set Shape
        shape = node->GetComponent<CollisionCircle2D>();
        shape->SetRadius(Min(bbsize.x_, bbsize.y_) * 0.5f);
        shape->SetDensity(density);
        shape->SetFilterBits(categoryBits, maskBits);
        shape->SetViewZ(viewZ);

        node->SetVar(GOA::ONVIEWZ, viewZ);

        node->SetWorldScale2D(wscale * scale);

        if (lifetime)
            TimerRemover::Get()->Start(node, r1 * lifetime, POOLRESTORE);
    }

//    URHO3D_LOGINFOF("GameHelpers() - SpawnScraps : ... OK !");

    return true;
}

void GameHelpers::SetControllabledNode(Node* node, int controlID, int movetype, int numjumps, int viewZ)
{
    GOC_Move2D* gocmove = node->CreateComponent<GOC_Move2D>();
    gocmove->SetMoveType((MoveTypeMode)movetype);
    gocmove->SetNumJumps(numjumps);
//    gocmove->SetPhysicReduce(true);

    GOC_Collide2D* goccollide = node->CreateComponent<GOC_Collide2D>();

    GOC_PlayerController* controller = node->CreateComponent<GOC_PlayerController>();
    controller->SetKeyControls(controlID);
    controller->SetMainController(true);

    GOC_Destroyer* destroyer = node->GetOrCreateComponent<GOC_Destroyer>();
    destroyer->SetDestroyMode(FREEMEMORY);
    destroyer->SetEnableUnstuck(false);
    destroyer->Reset(true);

    if (viewZ != -1)
        destroyer->SetViewZ(viewZ);
    else
        node->SetVar(GOA::ONVIEWZ, ViewManager::Get()->GetCurrentViewZ());

    if (World2D::GetWorldInfo())
        node->SendEvent(WORLD_ENTITYCREATE);
}

void GameHelpers::SetDrawableLayerView(Node* node, int viewZ)
{
    PODVector<Drawable2D*> drawables;
    node->GetDerivedComponents(drawables, true);

    if (drawables.Size())
    {
        int nodeViewZ = viewZ ? viewZ : node->GetVar(GOA::ONVIEWZ).GetInt();
        int nodeAlign = node->GetVar(GOA::LAYERALIGNMENT).GetInt();

        for (unsigned i=0; i < drawables.Size(); i++)
        {
            Drawable2D* drawable = drawables[i];

            // Never change Layering for the RenderShapes
            if (drawable->IsInstanceOf<RenderShape>())
                continue;

            int drawableViewZ = drawable->GetNode()->GetVar(GOA::ONVIEWZ).GetInt();
            if (!drawableViewZ)
                drawableViewZ = nodeViewZ;

            // if viewZ==0 don't Set Layer other than innerview or frontview => biomes furnitures are skipped
            if (viewZ || drawableViewZ == INNERVIEW || drawableViewZ == FRONTVIEW)
            {
                int drawableAlign = node->GetVar(GOA::LAYERALIGNMENT).GetInt();
                if (!drawableAlign)
                    drawableAlign = nodeAlign;

                // if no layermodifier, apply the actor layer modifier otherwise Drawable2D calculate the layer order
                int layermodifier = !drawable->GetLayerModifier() ? LAYER_ACTOR : 0;
                // set the modifier for the backlayerview : if LAYER_ACTOR use the BACKACTORVIEW with drawableAlign else use BACKACTORVIEW-1
                // note : Drawable2D doesn't use layermodifier for the backlayer, so force it.
                int backlayermodifier = layermodifier ? drawableAlign : -1;

                drawable->SetViewMask(ViewManager::GetLayerMask(drawableViewZ));
                drawable->SetLayer2(IntVector2(drawableViewZ + layermodifier + drawableAlign, BACKACTORVIEW + backlayermodifier));
//                URHO3D_LOGINFOF("GameHelpers() - SetDrawableLayerView : node=%s(%u) ... drawable=%u viewz=%d layer=%s layermodifier=%d ... OK !",
//                                node->GetName().CString(), node->GetID(), drawable, drawableViewZ, drawable->GetLayer2().ToString().CString(), drawable->GetLayerModifier());
            }
        }
    }
}



void GameHelpers::GetInputPosition(int& x, int& y, UIElement** uielt)
{
    if (GameContext::Get().gameConfig_.touchEnabled_ && GameContext::Get().input_->GetTouch(0))
    {
        x = GameContext::Get().input_->GetTouch(0)->position_.x_;
        y = GameContext::Get().input_->GetTouch(0)->position_.y_;
    }
    else if (GameContext::Get().cursor_)
    {
        x = GameContext::Get().cursor_->GetPosition().x_ * GameContext::Get().ui_->GetScale();
        y = GameContext::Get().cursor_->GetPosition().y_ * GameContext::Get().ui_->GetScale();
    }
    else
    {
        x = GameContext::Get().input_->GetMousePosition().x_;
        y = GameContext::Get().input_->GetMousePosition().y_;
    }

    if (uielt)
        *uielt = GameContext::Get().ui_->GetElementAt(x, y);
}

RigidBody2D* GameHelpers::GetBodyAtCursor()
{
    int x, y;
    GameHelpers::GetInputPosition(x, y);

    Viewport* viewport = GameContext::Get().context_->GetSubsystem<Renderer>()->GetViewport(ViewManager::Get()->GetViewportAt(x, y));
    RigidBody2D* body = 0;
    CollisionShape2D* shape = 0;
    Vector3 wpoint = viewport->ScreenToWorldPoint(x, y, viewport->GetScreenRay(x, y).HitDistance(GameContext::Get().GroundPlane_));
    GameContext::Get().physicsWorld_->GetPhysicElements(Vector2(wpoint.x_, wpoint.y_), body, shape);

    return body && shape ? body : 0;
}

bool GameHelpers::IsNodeImmuneToEffect(Node* node, const EffectType& effect)
{
    if (effect.name == "CaC")
    {
        return false;
    }

    if (effect.effectElt == GOA::FIRE)
    {
        if (node->GetVar(GOA::INFLUID).GetBool())
            return true;
    }

    if (node->GetVar(GOA::TYPECONTROLLER) == Variant::EMPTY)
    {
        return true;
    }
    else
    {
        const Variant& var = node->GetVar(GOA::GOT);
        if (var != Variant::EMPTY)
        {
            if (GOT::GetTypeProperties(var.GetStringHash()) & (GOT_Effect | GOT_Animation | GOT_Static))
                return true;

            /// TODO : create immune property for gottype
//            if (GOT::GetImmunities(var.GetStringHash()))
//                return true;
        }
        else
        {
            /// TODO : Player immunities
        }
    }

    return false;
}


/// Physics Helpers

#include <Urho3D/Urho2D/PhysicsUtils2D.h>
#include "DefsMove.h"

bool GameHelpers::SetPhysicProperties(Node* node, const PhysicEntityInfo& physicInfo, bool init, bool setvelocity)
{
    float dx = physicInfo.positionx_ - node->GetPosition().x_;
    float dy = physicInfo.positiony_ - node->GetPosition().y_;

    if (!init)
    {
        // Use smoothing
        SmoothedTransform* smooth = node->GetComponent<SmoothedTransform>();
        if (smooth)
        {
            // only if no big gap
            if (Abs(dx) < GameContext::Get().NetMaxDistance_ && Abs(dy) < GameContext::Get().NetMaxDistance_)
            {
                Vector3 position = node->GetPosition();

                // Set Rotation
                if (Abs(physicInfo.rotation_- node->GetRotation2D()) > PIXEL_SIZE)
                    node->SetNetRotation(Quaternion(physicInfo.rotation_));

                // Set New Position if not on ground or dy is enough
                if (Abs(dy) > 0.1f || (node->GetVar(GOA::MOVESTATE).GetUInt() & MV_TOUCHGROUND) == 0)
                {
                    // set position coord only if coherent with the velocity (prevent bounce)
                    if ((physicInfo.vely_ <= 0.f && dy < 0) || (physicInfo.vely_ >= 0.f && dy > 0))
                        position.y_ = physicInfo.positiony_;

                    if (Abs(dx) > PIXEL_SIZE)
                    {
                        if ((physicInfo.velx_ <= 0.f && dx < 0) || (physicInfo.velx_ >= 0.f && dx > 0))
                            position.x_ = physicInfo.positionx_;
                    }

                    node->SetNetPositionAttr(position);

                    if (setvelocity)
                    {
                        RigidBody2D* body = node->GetComponent<RigidBody2D>();
                        if (body)
                            body->GetBody()->SetVelocity(physicInfo.velx_, physicInfo.vely_);
                    }
                }
                // don't apply movement on y, if already on the ground and small dy (remove the bounce)
                else
                {
                    if (Abs(dx) > 0.01f)
                    {
                        if ((physicInfo.velx_ <= 0.f && dx < 0) || (physicInfo.velx_ >= 0.f && dx > 0))
                            position.x_ = physicInfo.positionx_;

                        node->SetNetPositionAttr(position);
                    }
                    // that's stopped smoothing
                    else
                    {
                        if (smooth && smooth->IsInProgress())
                        {
                            smooth->StopUpdatePosition();
                            node->SetPosition(position);
                        }
                    }

                    if (setvelocity)
                    {
                        RigidBody2D* body = node->GetComponent<RigidBody2D>();
                        if (body)
                            body->GetBody()->SetVelocity(physicInfo.velx_, 0.f);
                    }
                }

                return true;
            }
        }
    }

    // Set Initial Position
    if (Abs(dx) > 0.01f || Abs(dy) > 0.01f)
    {
        node->SetPosition(Vector3(physicInfo.positionx_, physicInfo.positiony_, 0.f));

        RigidBody2D* body = node->GetComponent<RigidBody2D>();
        if (body)
        {
            body->SetWorldTransform(node->GetWorldPosition2D(), node->GetWorldRotation2D());

            if (setvelocity)
                body->GetBody()->SetVelocity(physicInfo.velx_, physicInfo.vely_);
        }
    }

    if (physicInfo.rotation_ > PIXEL_SIZE || physicInfo.rotation_ < -PIXEL_SIZE)
    {
        if (!node->HasComponent<GOC_Move2D>())// && node->GetVar(GOA::BULLET) == Variant::EMPTY)
        {
            node->SetRotation2D(physicInfo.rotation_);
//            URHO3D_LOGERRORF("GameHelpers() - SetPhysicProperties : node=%s(%u) set rotation=%f !", node->GetName().CString(), node->GetID(), node->GetRotation2D());
        }
        else if (node->GetRotation2D() != 0.f)
        {
//            URHO3D_LOGERRORF("GameHelpers() - SetPhysicProperties : node=%s(%u) rotation=%f => reset !", node->GetName().CString(), node->GetID(), node->GetRotation2D());
            node->SetRotation2D(0.f);
        }
    }
    else
    {
        if (node->GetRotation2D() != 0.f)
        {
//            URHO3D_LOGERRORF("GameHelpers() - SetPhysicProperties : node=%s(%u) rotation=%f => reset !", node->GetName().CString(), node->GetID(), node->GetRotation2D());
            node->SetRotation2D(0.f);
        }
    }

//    URHO3D_LOGINFOF("GameHelpers() - SetPhysicProperties : node=%s(%u) init=true position=%s rotation=%F !", node->GetName().CString(), node->GetID(), node->GetPosition2D().ToString().CString(), node->GetRotation2D());

    return true;
}

bool GameHelpers::SetPhysicProperties(Node* node, const void* physicData, bool init, bool setvelocity)
{
    return SetPhysicProperties(node, *((PhysicEntityInfo*) physicData), init, setvelocity);
}

void GameHelpers::SetPhysicVelocity(Node* node, float velx, float vely)
{
    RigidBody2D* body = node->GetComponent<RigidBody2D>();
    if (body)
        body->GetBody()->SetVelocity(velx, vely);
}

// flip collisionboxes if have angle or decenter in x
void GameHelpers::SetPhysicFlipX(Node* node)
{
    if (!node)
        return;

    RigidBody2D* body = node->GetComponent<RigidBody2D>();
    if (!body)
        return;

    if (node->IsEnabled() && body->IsEnabledEffective())
    {
        const Vector<WeakPtr<CollisionShape2D> >& shapes = body->GetCollisionsShapes();
        for (Vector<WeakPtr<CollisionShape2D> >::ConstIterator it=shapes.Begin(); it!=shapes.End(); ++it)
        {
            const WeakPtr<CollisionShape2D>& shape = *it;
            if (shape)
            {
                if (shape->IsInstanceOf<CollisionBox2D>())
                {
                    CollisionBox2D* box = StaticCast<CollisionBox2D>(shape);
                    if (box->GetCenter().x_ != 0.f)
                    {
                        box->SetCenter(Vector2(-box->GetCenter().x_, box->GetCenter().y_));
                    }
                    if (box->GetAngle() != 0.f)
                    {
                        box->SetAngle(-box->GetAngle());
                    }
                }
                else if (shape->IsInstanceOf<CollisionCircle2D>())
                {
                    CollisionCircle2D* circle = StaticCast<CollisionCircle2D>(shape);
                    if (circle->GetCenter().x_ != 0.f)
                    {
                        circle->SetCenter(Vector2(-circle->GetCenter().x_, circle->GetCenter().y_));
                    }
                }
            }
        }
    }
    else
    {
        PODVector<CollisionShape2D*> shapes;
        node->GetDerivedComponents(shapes, true);

        for (unsigned i=0; i < shapes.Size(); i++)
        {
            CollisionShape2D* shape = shapes[i];
            if (shape)
            {
                if (shape->IsTrigger())
                    continue;

                if (shape->IsInstanceOf<CollisionBox2D>())
                {
                    CollisionBox2D* box = static_cast<CollisionBox2D*>(shape);
                    if (box->GetCenter().x_ != 0.f)
                    {
                        box->SetCenter(Vector2(-box->GetCenter().x_, box->GetCenter().y_));
                    }
                    if (box->GetAngle() != 0.f)
                    {
                        box->SetAngle(-box->GetAngle());
                    }
                }
                else if (shape->IsInstanceOf<CollisionCircle2D>())
                {
                    CollisionCircle2D* circle = static_cast<CollisionCircle2D*>(shape);
                    if (circle->GetCenter().x_ != 0.f)
                    {
                        circle->SetCenter(Vector2(-circle->GetCenter().x_, circle->GetCenter().y_));
                    }
                }

                shape->ReleaseFixture();
            }
        }
    }
}

Vector2 GameHelpers::GetWorldMassCenter(Node* node, CollisionShape2D* shape)
{
    return node->GetWorldPosition2D() + shape->GetMassCenter() * node->GetWorldScale2D();
}

void GameHelpers::GetDropPoint(Node* holder, float& x, float& y)
{
    bool isdead = holder->GetVar(GOA::ISDEAD).GetBool();
    GOC_Destroyer* destroyer = holder->GetComponent<GOC_Destroyer>();
    if (!isdead && destroyer)
    {
        Vector2 direction = holder->GetVar(GOA::DIRECTION).GetVector2();
        Rect holderRect = holder->GetComponent<GOC_Destroyer>()->GetWorldShapesRect();
        x = direction.x_ >= 0.f ? holderRect.max_.x_ + 0.2f : holderRect.min_.x_ - 0.5f;
        y = direction.y_ >= 0.f ? holderRect.max_.y_ + 0.2f : holderRect.min_.y_ - 0.5f;
    }
    else
    {
        x = holder->GetWorldPosition2D().x_;
        y = holder->GetWorldPosition2D().y_ + 0.5f;
    }
}


/// Component Helpers

void GameHelpers::ShakeNode(Node* node, int numshakes, float duration, const Vector2& amplitude)
{
//    URHO3D_LOGINFOF("GameHelpers() - ShakeNode : node=%s(%u) initialposition=%s addNewObjectAnimation=%s ! ",
//                    node->GetName().CString(), node->GetID(), position.ToString().CString(),addnewobject ? "true":"false");

    GEF_NodeShaker* shaker = node->GetOrCreateComponent<GEF_NodeShaker>();
    shaker->SetTemporary(true);
    shaker->SetNumShakes(numshakes);
    shaker->SetDuration(duration);
    shaker->SetAmplitude(amplitude);
    shaker->Start();
}

void GameHelpers::MoveCameraTo(const Vector2& position, int viewport, float duration)
{
    Node* node = ViewManager::Get()->GetCameraNode(viewport);

    const Vector3& initialpos = node->GetPosition();
    const Vector3 finalpos(position, initialpos.z_);

    bool addnewobject = !node->GetObjectAnimation();

    node->SetAnimationEnabled(false);

    SharedPtr<ObjectAnimation> objectAnimation(addnewobject ? new ObjectAnimation(node->GetContext()) : node->GetObjectAnimation());

    objectAnimation->RemoveAttributeAnimation("Position");

    SharedPtr<ValueAnimation> animation(new ValueAnimation(node->GetContext()));
    animation->SetKeyFrame(0.f, initialpos);
    animation->SetKeyFrame(duration, finalpos);
    objectAnimation->AddAttributeAnimation("Position", animation, WM_ONCE);

    if (addnewobject)
        node->SetObjectAnimation(objectAnimation);

    node->SetAnimationEnabled(true);
}

void GameHelpers::ZoomCameraTo(float zoom, int viewport, float duration)
{
    Camera* camera = ViewManager::Get()->GetCamera(viewport);

    float initialzoom = camera->GetZoom();

    bool addnewobject = !camera->GetObjectAnimation();

    camera->SetAnimationEnabled(false);

    SharedPtr<ObjectAnimation> objectAnimation(addnewobject ? new ObjectAnimation(camera->GetContext()) : camera->GetObjectAnimation());

    objectAnimation->RemoveAttributeAnimation("Zoom");

    SharedPtr<ValueAnimation> animation(new ValueAnimation(camera->GetContext()));
    animation->SetKeyFrame(0.f, initialzoom);
    animation->SetKeyFrame(duration, zoom);
    objectAnimation->AddAttributeAnimation("Zoom", animation, WM_ONCE);

    if (addnewobject)
        camera->SetObjectAnimation(objectAnimation);

    camera->SetAnimationEnabled(true);
}

float GameHelpers::GetZoomToFitTo(int numtilesx, int numtilesy, int viewport)
{
    float size = Max(numtilesx * World2D::GetWorld()->GetWorldInfo()->mTileWidth_, numtilesy * World2D::GetWorld()->GetWorldInfo()->mTileHeight_) * 1.05f;
    Camera* camera = ViewManager::Get()->GetCamera(viewport);
    return /*camera->GetZoom() * */camera->GetOrthoSize() / size;
}

void GameHelpers::AddPlane(const char *nodename, Node* node, const char *materialname,const Vector3& pos, const Vector2& scale, CreateMode mode)
{
    Node* planeNode = node->CreateChild(nodename, mode);

    planeNode->SetPosition(pos);
    planeNode->Pitch(-90);
    planeNode->SetScale(Vector3(scale.x_, 1.0f, scale.y_));
    StaticModel* planeObject = planeNode->CreateComponent<StaticModel>();
    planeObject->SetModel(GameContext::Get().resourceCache_->GetResource<Model>("Models/Plane.mdl"));
    planeObject->SetMaterial(GameContext::Get().resourceCache_->GetResource<Material>(materialname));
}

void GameHelpers::AddStaticSprite2D(const char *nodename, Node* node, const char *texturename, const char *materialname, const Vector3& position, const Vector2& scale, CreateMode mode)
{
    Node* planeNode = node->CreateChild(nodename, mode);
    planeNode->SetPosition(position);
    planeNode->SetScale(Vector3(scale.x_*PIXEL_SIZE, scale.y_*PIXEL_SIZE, 1.f));

//    Sprite2D* sprite = planeNode->CreateComponent<Sprite2D>();
//    sprite->SetTexture (cache->GetResource<Texture2D>(texturename));
//    sprite->SetRectangle (const IntRect &rectangle);
//    sprite->SetHotSpot (const Vector2 &hotSpot);
//    sprite->SetOffset (const IntVector2 &offset);
    Sprite2D* sprite = GameContext::Get().resourceCache_->GetResource<Sprite2D>(texturename);
    StaticSprite2D* staticSprite = planeNode->CreateComponent<StaticSprite2D>();
    staticSprite->SetBlendMode(BLEND_ALPHA);
    staticSprite->SetSprite(sprite);
    if (materialname)
        staticSprite->SetCustomMaterial(GameContext::Get().resourceCache_->GetResource<Material>(materialname));
//    staticSprite->SetUseHotSpot(true);

//    staticSprite->SetHotSpot(hotSpot_);
//    staticSprite->SetLayer2(layer_);
}

void GameHelpers::AddLight(Node *node, LightType type, const Vector3& position, const Vector3& direction, const float& fov, const float& range, float brightness, bool pervertex, bool colorAnimated)
{
    if (!node)
    {
        URHO3D_LOGERROR("GameHelpers() - AddLight : node = 0 !");
        return;
    }

//    URHO3D_LOGINFOF("GameHelpers() - AddLight : on Node %s(%u) ...", node->GetName().CString(), node->GetID());

    Node* nodeLight = node->CreateChild("Light");
    nodeLight->SetPosition(position);
    nodeLight->SetDirection(direction);

    Light* light = nodeLight->CreateComponent<Light>();
    light->SetLightType(type);
    light->SetPerVertex(pervertex);
    light->SetBrightness(brightness);

    //if (type==LIGHT_POINT)
    {
        light->SetRange(range);
        light->SetFov(fov);
    }
    if (colorAnimated)
    {
        // Create light animation
        SharedPtr<ObjectAnimation> lightAnimation(new ObjectAnimation(node->GetContext()));
        // Create light color animation
        SharedPtr<ValueAnimation> colorAnimation(new ValueAnimation(node->GetContext()));
        colorAnimation->SetKeyFrame(0.0f, Color::WHITE);
        colorAnimation->SetKeyFrame(2.0f, Color::YELLOW);
        colorAnimation->SetKeyFrame(4.0f, Color::WHITE);
        // Set Light component's color animation
        lightAnimation->AddAttributeAnimation("@Light/Color", colorAnimation);
        // Apply light animation to light node
        nodeLight->SetObjectAnimation(lightAnimation);
    }
//    light->SetEnabled(true);
}

bool GameHelpers::SetLightActivation(Node* node, int viewport)
{
    static PODVector<Light*> sLights_;
    node->GetComponents<Light>(sLights_, true);

    bool state = false;

    if (sLights_.Size())
    {
        int viewz = ViewManager::Get()->GetCurrentViewZ(viewport);
        bool enlightstate = node->GetVar(GOA::ONVIEWZ).GetInt() == viewz;

        if (enlightstate)
            enlightstate = GameContext::Get().luminosity_ < ENLIGHTTHRESHOLD || viewz == INNERVIEW;

        // Modify logic for the Chest when closed or opened with empty stuff => the light must be disabled
        const Variant& varlight = node->GetVar(GOA::LIGHTSTATE);
        if (varlight != Variant::EMPTY)
            enlightstate = enlightstate && varlight.GetBool();

        state = enlightstate;

        if (GameContext::Get().gameConfig_.fluidEnabled_)
        {
            for (PODVector<Light*>::ConstIterator it = sLights_.Begin(); it != sLights_.End(); ++it)
            {
                Light* light = *it;
                bool lightstate = enlightstate;
                if (lightstate)
                {
                    // check if is a fire and if in Fluid
                    const Variant& varIsFire = light->GetNode()->GetVar(GOA::FIRE);
                    if (varIsFire != Variant::EMPTY)
                    {
                        // Method 1 : with GOA::INFLUID
                        const Variant& varInFluid = node->GetVar(GOA::INFLUID);
                        if (varInFluid != Variant::EMPTY)
                        {
                            lightstate = !varInFluid.GetBool();
                            URHO3D_LOGINFOF("GameHelpers() - SetLightActivation : %s(%u) firelight check varInFluid => switch %s the light !",
                                            node->GetName().CString(), node->GetID(), lightstate ? "on":"off");
                        }
                        // Method 2 : with FluidCell
                        else if (World2D::GetWorldInfo())
                        {
                            WorldMapPosition wmPosition;
                            World2D::GetWorldInfo()->Convert2WorldMapPosition(light->GetNode()->GetWorldPosition2D(), wmPosition, wmPosition.positionInTile_);
                            Map* map = World2D::GetWorld()->GetMapAt(wmPosition.mPoint_);
                            if (map)
                            {
                                lightstate = map->GetFluidCellPtr(wmPosition.tileIndex_, map->GetObjectFeatured()->GetIndexViewZ(viewz))->type_ < WATER;
                                URHO3D_LOGINFOF("GameHelpers() - SetLightActivation : %s(%u) firelight check fluidcell => switch %s the light !",
                                                node->GetName().CString(), node->GetID(), lightstate ? "on":"off");
                            }
                        }
                    }
                }

                light->SetEnabled(lightstate && GameContext::Get().gameConfig_.enlightScene_);
                state &= lightstate;
            }
        }
        else
        {
            for (PODVector<Light*>::ConstIterator it = sLights_.Begin(); it != sLights_.End(); ++it)
            {
                Light* light = *it;
                light->SetEnabled(enlightstate && GameContext::Get().gameConfig_.enlightScene_);
                state &= enlightstate;
            }
        }
    }

    return state;
}

bool GameHelpers::SetLightActivation(Player* player)
{
    const PODVector<Light*> lights = player->GetLights();
    if (!lights.Size())
    {
        URHO3D_LOGERRORF("GameHelpers() - SetLightActivation : player=%d nolight !", player->GetID());
        return false;
    }

    bool enlight = GameContext::Get().gameConfig_.enlightScene_ && (GameContext::Get().luminosity_ < ENLIGHTTHRESHOLD || player->GetViewZ() == INNERVIEW);
    URHO3D_LOGERRORF("GameHelpers() - SetLightActivation : player=%d numlights=%u lit=%s !", player->GetID(), lights.Size(), enlight ? "true":"false");

    for (PODVector<Light*>::ConstIterator it=lights.Begin(); it != lights.End(); ++it)
    {
        Light* light = *it;

        bool lit = enlight;

#ifndef URHO3D_VULKAN
        // inactive perpixel light
        if (lit && !light->GetPerVertex())
            lit = false;
#endif

        light->SetEnabled(lit);
        if (light->GetNode() != player->GetAvatar())
            light->GetNode()->SetEnabled(lit);

        // Always display the light of the player
        if (lit && light->GetPerVertex())
            light->SetFixedSortValue(-M_LARGE_VALUE);
    }

    return true;
}

void GameHelpers::UpdateLayering(Node* node, int viewZ)
{
    {
        static PODVector<GOC_Animator2D*> gocanimators;
        node->GetComponents<GOC_Animator2D>(gocanimators, true);

        if (gocanimators.Size())
        {
            for (unsigned i=0; i < gocanimators.Size(); i++)
                gocanimators[i]->CheckFireLight();
        }
        else
        {
            bool lightstate = SetLightActivation(node);
        }
    }

    SetDrawableLayerView(node, viewZ);
}

Sprite2D* GameHelpers::GetSpriteForType(const StringHash& type, const StringHash& parentType, const String& partname)
{
    Sprite2D* sprite=0;

    Node* templatenode = GOT::GetObject(type);

    if (templatenode)
    {
        StaticSprite2D* drawable = templatenode->GetDerivedComponent<StaticSprite2D>();
        if (!drawable)
        {
            URHO3D_LOGWARNING("GameHelpers() - GetSpriteForType : no drawable !");
            return 0;
        }

        sprite = drawable->GetSprite();
        if (!sprite)
        {
            URHO3D_LOGWARNING("GameHelpers() - GetSpriteForType : no sprite !");
            return 0;
        }
    }

    if (!sprite && (GOT::GetTypeProperties(type) & GOT_Part))
    {
        const StringHash& buildable = parentType ? parentType : GOT::GetBuildableType(type);

        if (buildable)
        {
            Node* node = GOT::GetObject(buildable);
            if (node)
            {
                AnimatedSprite2D* animatedSprite = node->GetComponent<AnimatedSprite2D>();
                if (animatedSprite)
                {
                    sprite = animatedSprite->GetAnimationSet()->GetSprite(partname);
                }
            }
            else
                URHO3D_LOGWARNINGF("GameHelpers() - GetSpriteForType : no buildable(%s(%u)) Node for type=%s(%u)", GOT::GetType(buildable).CString(), buildable.Value(), GOT::GetType(type).CString(), type.Value());
        }
        else
            URHO3D_LOGWARNINGF("GameHelpers() - GetSpriteForType : no buildable for type=%s(%u)", GOT::GetType(type).CString(), type.Value());
    }

    if (!sprite)
        URHO3D_LOGWARNINGF("GameHelpers() - GetSpriteForType : %s(%u) => No GOT Object and No Buildable Part !", GOT::GetType(type).CString(), type.Value());

    return sprite;
}

Sprite2D* GameHelpers::GetSpriteForType(const StringHash& type, const StringHash& parentType, int partIndex)
{
    Sprite2D* sprite=0;

    Node* templatenode = GOT::GetObject(type);

    if (templatenode)
    {
        StaticSprite2D* drawable = templatenode->GetDerivedComponent<StaticSprite2D>();
        if (!drawable)
        {
            URHO3D_LOGWARNING("GameHelpers() - GetSpriteForType : no drawable !");
            return 0;
        }

        sprite = drawable->GetSprite();
        if (!sprite)
        {
            URHO3D_LOGWARNING("GameHelpers() - GetSpriteForType : no sprite !");
            return 0;
        }
    }

    if (!sprite && (GOT::GetTypeProperties(type) & GOT_Part))
    {
        const StringHash& buildable = parentType ? parentType : GOT::GetBuildableType(type);

        if (buildable)
        {
            Node* node = GOT::GetObject(buildable);
            if (node)
            {
                AnimatedSprite2D* animatedSprite = node->GetComponent<AnimatedSprite2D>();
                if (animatedSprite)
                {
                    const Vector<String>& partNames = *GOT::GetPartsNamesFor(buildable);
                    if (partIndex < 0)
                    {
                        partIndex = Random((int)partNames.Size());
                    }

                    sprite = animatedSprite->GetAnimationSet()->GetSprite(partNames[partIndex]);

//                    URHO3D_LOGINFOF("GameHelpers() - GetSpriteForType : type=%s(%u) partname=%s partindex=%d/%u sprite=%u", GOT::GetType(type).CString(), type.Value(), partNames[partIndex].CString(), partIndex, partNames.Size(), sprite);
                }
            }
            else
                URHO3D_LOGWARNINGF("GameHelpers() - GetSpriteForType : no buildable(%s(%u)) Node for type=%s(%u)", GOT::GetType(buildable).CString(), buildable.Value(), GOT::GetType(type).CString(), type.Value());
        }
        else
            URHO3D_LOGWARNINGF("GameHelpers() - GetSpriteForType : no buildable for type=%s(%u)", GOT::GetType(type).CString(), type.Value());
    }

    if (!sprite)
        URHO3D_LOGWARNINGF("GameHelpers() - GetSpriteForType : %s(%u) => No GOT Object and No Buildable Part !", GOT::GetType(type).CString(), type.Value());

    return sprite;
}

void GameHelpers::SetCollectableProperties(GOC_Collectable* collectable, const StringHash& got, VariantMap* slotDataPtr)
{
    if (!collectable)
        return;

    unsigned props = GOT::GetTypeProperties(got);

    if (!(props & GOT_Collectable))
        return;

    // Set Collectable Slot
    Slot& slot = collectable->Get();
    Node* node = collectable->GetNode();
    StringHash type;

    if (slotDataPtr)
    {
        VariantMap& slotData = *slotDataPtr;
        type = StringHash(slotData[Net_ObjectCommand::P_CLIENTOBJECTTYPE].GetUInt());
        slot.Set(type, slotData[Net_ObjectCommand::P_SLOTQUANTITY].GetUInt(), StringHash(slotData[Net_ObjectCommand::P_SLOTPARTFROMTYPE].GetUInt()), slotData[Net_ObjectCommand::P_SLOTSPRITE].GetResourceRef());
        slot.color_ = slotData[Net_ObjectCommand::P_SLOTCOLOR].GetColor();
        slot.scale_ = slotData[Net_ObjectCommand::P_SLOTSCALE].GetVector3();

        // Add Effect Type if exist
        int effect = slotData[Net_ObjectCommand::P_SLOTEFFECT].GetInt();
        if (effect > -1)
            node->SetVar(GOA::EFFECTID1, effect);
    }
    else
    {
        type = got;
        slot.Set(type, 1, GOT::GetBuildableType(type));
        slot.color_ = Color::WHITE;
        slot.scale_ = Vector3::ONE;
    }

    if (node->GetVar(GOA::EFFECTID1) != Variant::EMPTY)
        slot.effect_ = node->GetVar(GOA::EFFECTID1).GetInt();

    if (!slot.sprite_)
        URHO3D_LOGWARNINGF("GameHelpers() - SetCollectableProperties : node=%s(%u) no sprite in slot !", node->GetName().CString(), node->GetID());

    URHO3D_LOGINFOF("GameHelpers() - SetCollectableProperties : got=%s(%u) sprite=%u qty=%d ispart=%s",
                    GOT::GetType(type).CString(), type.Value(), slot.sprite_.Get(), slot.quantity_, (props & GOT_Part) ? "true":"false");

    // Set Collectable Part
    if (props & GOT_Part)
    {
        // Set Node
        node->SetName(GOT::GetType(type));
        node->SetWorldScale(slot.scale_);
//        node->SetWorldScale(Vector3(0.5f, 0.5f, 1.f));

        // Set Sprite Component
        StaticSprite2D* staticSprite = node->GetDerivedComponent<StaticSprite2D>();
        if (staticSprite)
        {
            // force clearing the old sprite to update the sprite draw rectangle
            staticSprite->SetSprite(0);
            staticSprite->SetSprite(slot.sprite_);
#ifdef ACTIVE_LAYERMATERIALS
            staticSprite->SetCustomMaterial(GameContext::Get().layerMaterials_[LAYERACTORS]);
#endif
            staticSprite->SetColor(slot.color_);
            staticSprite->SetUseHotSpot(true);
            staticSprite->SetHotSpot(Vector2(0.5f, 0.5f));
        }

        // Set Physics Components
        RigidBody2D* body = node->GetComponent<RigidBody2D>();
        if (body)
        {
            const Rect& drawrect = staticSprite->GetDrawRectangle();
            Vector2 size = drawrect.Size();

            if (size.x_)
            {
                body->SetUseFixtureMass(false);
                body->SetMass(0.5f * size.x_ * size.y_);
                body->SetAllowSleep(false);
                body->SetMassCenter(drawrect.Center());

                CollisionCircle2D* shape = node->GetComponent<CollisionCircle2D>();
                if (shape)
                {
                    shape->SetRadius(Min(size.x_, size.y_) * 0.5f * 0.6f);
                    shape->SetCenter(drawrect.Center());
                }
            }
            else
            {
                URHO3D_LOGWARNINGF("GameHelpers() - SetCollectableProperties : node=%s(%u) staticsprite rectsize=%s!",
                                   node->GetName().CString(), node->GetID(), size.ToString().CString());
            }
        }

        // Set ScrapsEmitter Component
        ScrapsEmitter* scrapsEmitter = node->GetComponent<ScrapsEmitter>();
        if (scrapsEmitter)
        {
            IntVector2 rectsize = slot.sprite_->GetRectangle().Size();
            scrapsEmitter->SetScrapsType(node->GetName().Empty() ? String::EMPTY : "Scraps_" + node->GetName());
            scrapsEmitter->SetSize(rectsize);
            scrapsEmitter->SetNumScraps(Min(7, Max(rectsize.x_, rectsize.y_)/2));
            scrapsEmitter->SetLifeTime(2.f + Random(2.f));
            scrapsEmitter->SetInitialImpulse(5.f);
            scrapsEmitter->SetTrigEvent(GO_RECEIVEEFFECT);
            scrapsEmitter->ApplyAttributes();
//            URHO3D_LOGINFOF("GameHelpers() - SetCollectableProperties : node=%s(%u) setting Scraps Emitter Type=%s",
//                                node->GetName().CString(), node->GetID(), scrapsEmitter->GetScrapsTypeName().CString());
        }
    }
}

void GameHelpers::GetRandomizedEquipment(AnimatedSprite2D* animatedSprite, EquipmentList* equipmentlist)
{
    equipmentlist->Clear();

    if (animatedSprite->HasCharacterMap(CMAP_WEAPON1))
    {
        const StringHash& weapon1 = GameRand::GetRand(OBJRAND, 100) > 35 ? COT::GetRandomTypeFrom(COT::MELEEWEAPONS, GameRand::GetRand(OBJRAND, 100)) : StringHash::ZERO;
        equipmentlist->Push(EquipmentPart("Weapon1", weapon1));
    }
    if (animatedSprite->HasCharacterMap(CMAP_WEAPON2))
    {
        bool canshoot = animatedSprite->HasAnimation("shoot0");
        const StringHash& weapon2 = canshoot ? COT::GetRandomTypeFrom(COT::RANGEDWEAPONS, GameRand::GetRand(OBJRAND, 100)) :
                                    (GameRand::GetRand(OBJRAND, 100) > 50 ? COT::GetRandomTypeFrom(COT::MELEEWEAPONS, GameRand::GetRand(OBJRAND, 100)) : StringHash::ZERO);

        equipmentlist->Push(EquipmentPart("Weapon2", weapon2));
    }
    if (animatedSprite->HasCharacterMap(CMAP_ARMOR))
    {
        const StringHash& armor = GameRand::GetRand(OBJRAND, 100) > 50 ? COT::GetRandomTypeFrom(COT::ARMORS, GameRand::GetRand(OBJRAND, 100)) : StringHash::ZERO;
        equipmentlist->Push(EquipmentPart("Armor", armor));
    }
    if (animatedSprite->HasCharacterMap(CMAP_HELMET))
    {
        const StringHash& helmet = GameRand::GetRand(OBJRAND, 100) > 65 ? COT::GetRandomTypeFrom(COT::HELMETS, GameRand::GetRand(OBJRAND, 100)) : StringHash::ZERO;
        equipmentlist->Push(EquipmentPart("Helmet", helmet));
    }
    if (animatedSprite->HasCharacterMap(CMAP_BELT))
    {
        const StringHash& belt = GameRand::GetRand(OBJRAND, 100) > 35 ? COT::GetRandomTypeFrom(COT::BELTS, GameRand::GetRand(OBJRAND, 100)) : StringHash::ZERO;
        equipmentlist->Push(EquipmentPart("Belt", belt));
    }
    if (animatedSprite->HasCharacterMap(CMAP_CAPE))
    {
        const StringHash& cape = GameRand::GetRand(OBJRAND, 100) > 65 ? COT::GetRandomTypeFrom(COT::CAPES, GameRand::GetRand(OBJRAND, 100)) : StringHash::ZERO;
        equipmentlist->Push(EquipmentPart("Cape", cape));
    }
    if (animatedSprite->HasCharacterMap(CMAP_HEADBAND))
    {
        const StringHash& headband = GameRand::GetRand(OBJRAND, 100) > 65 ? COT::GetRandomTypeFrom(COT::HEADBANDS, GameRand::GetRand(OBJRAND, 100)) : StringHash::ZERO;
        equipmentlist->Push(EquipmentPart("Headband", headband));
    }
}

void GameHelpers::SetEquipmentList(AnimatedSprite2D* animatedSprite, EquipmentList* equipmentlist)
{
    if (!animatedSprite || !equipmentlist)
        return;

    URHO3D_LOGINFOF("GameHelpers() - SetEquipmentList : node=%s(%u) equipment size=%u ...",
                    animatedSprite->GetNode()->GetName().CString(), animatedSprite->GetNode()->GetID(),
                    equipmentlist->Size());

    PODVector<Sprite2D*> spritesList;
    for (EquipmentList::ConstIterator it=equipmentlist->Begin(); it!=equipmentlist->End(); ++it)
    {
        const EquipmentPart& part = *it;

        spritesList.Clear();

        if (part.objecttype_)
        {
            URHO3D_LOGINFOF("GameHelpers() - SetEquipmentList : GOT=%s MapName=%s ...", GOT::GetType(part.objecttype_).CString(), part.mapname_.CString());

            Sprite2D::LoadFromResourceRefList(animatedSprite->GetContext(), GOT::GetResourceList(part.objecttype_), spritesList);
            if (spritesList.Size())
            {
                animatedSprite->SwapSprites(part.mapname_, spritesList, !part.mapname_.StartsWith("Armor") && !part.mapname_.StartsWith("Helm"));
                animatedSprite->ApplyCharacterMap(part.mapname_);
                URHO3D_LOGINFOF("GameHelpers() - SetEquipmentList : Swap Sprites on MapName=%s spriteListSize=%u ...", part.mapname_.CString(), spritesList.Size());
            }
        }

        if (!spritesList.Size())
        {
            if (animatedSprite->IsCharacterMapApplied(part.mapname_))
                animatedSprite->ApplyCharacterMap(String("No_" + part.mapname_));
            URHO3D_LOGINFOF("GameHelpers() - SetEquipmentList : Remove MapName=%s ...", part.mapname_.CString());
        }
    }

    animatedSprite->ResetAnimation();

    URHO3D_LOGINFOF("GameHelpers() - SetEquipmentList : node=%s(%u) ... OK !", animatedSprite->GetNode()->GetName().CString(), animatedSprite->GetNode()->GetID());
}

void GameHelpers::SetEntityVariation(AnimatedSprite2D* animatedsprite, int& entityid, const GOTInfo* gotinfo, bool forceRandomEntity, bool forceRandomMapping)
{
    if (!animatedsprite)
        return;

    int entityidFlags = entityid & EntityFlags;
    int entityidValue = entityid & EntityValue;

    // Randomize Entity
    if (((gotinfo && gotinfo->entityVariation_) && (entityidFlags & RandomEntityFlag)) || forceRandomEntity)
    {
        entityidValue = 0;
        if (animatedsprite->GetNumSpriterEntities() > 1)
        {
            entityidValue = GameRand::GetRand(OBJRAND, 100) % animatedsprite->GetNumSpriterEntities();
            if (!entityidValue && animatedsprite->GetSpriterEntity(0) == "Custom")
                entityidValue = 1;

//            URHO3D_LOGINFOF("GameHelpers() - SetEntityVariation : node=%s(%u) randomize entityid Value=%d ...", animatedsprite->GetNode()->GetName().CString(), animatedsprite->GetNode()->GetID(), entityidValue);
        }

        entityidValue = entityidValue % Max(animatedsprite->GetNumSpriterEntities(), MaxEntityIds);

        // Set Entity
        if (entityidValue != animatedsprite->GetSpriterEntityIndex())
        {
            animatedsprite->SetSpriterEntity(entityidValue);

            GOC_Animator2D* animator = animatedsprite->GetComponent<GOC_Animator2D>();
            if (animator)
                animator->UpdateEntity();
        }
    }
    else if ((entityidFlags & SetEntityFlag) && entityidValue != animatedsprite->GetSpriterEntityIndex())
    {
        // Set Entity

//        URHO3D_LOGERRORF("GameHelpers() - SetEntityVariation : node=%s(%u) set Entity=%d old=%d ...", animatedsprite->GetNode()->GetName().CString(), animatedsprite->GetNode()->GetID(), entityidValue, animatedsprite->GetSpriterEntityIndex());
        animatedsprite->SetSpriterEntity(entityidValue);

        GOC_Animator2D* animator = animatedsprite->GetComponent<GOC_Animator2D>();
        if (animator)
            animator->UpdateEntity();
    }

    // Set Mapping
    if (animatedsprite->HasCharacterMapping())
    {
        bool randomMapping = ((gotinfo && gotinfo->mappingVariation_) && (entityidFlags & RandomMappingFlag)) || forceRandomMapping;

        // Set Default Mapping
        if (randomMapping || !animatedsprite->GetAppliedCharacterMapsAttr().Size())
        {
            animatedsprite->ResetCharacterMapping();
            animatedsprite->ApplyCharacterMap(CMAP_NAKED);
            animatedsprite->ApplyCharacterMap(CMAP_HEAD2);
        }
        // Important to randomize the heads for the preparation of the nodes in GOC_BodyExploder2D.
        if (randomMapping)
        {
            GameRand& Random = GameRand::GetRandomizer(OBJRAND);
            animatedsprite->ApplyCharacterMap(String("Head")+String(Random.Get(3) + 1));
        }
        // Randomize Equipment
        if (randomMapping)
        {
            EquipmentList equipmentlist;
            GetRandomizedEquipment(animatedsprite, &equipmentlist);
            SetEquipmentList(animatedsprite, &equipmentlist);
        }
    }

    entityid = animatedsprite->GetSpriterEntityIndex() + entityidFlags;
}

void GameHelpers::SetRenderedAnimation(AnimatedSprite2D* animatedSprite, const String& cmap, const StringHash& got)
{
    if (got)
    {
        Node* templateNode = GOT::GetObject(got);
        if (!templateNode)
        {
            URHO3D_LOGERRORF("GameHelpers() - SetRenderedAnimation : Node=%s(%u) no templateNode for cmap=%s", animatedSprite->GetNode()->GetName().CString(), animatedSprite->GetNode()->GetID(),
                             cmap.CString());
            return;
        }

        AnimatedSprite2D* templateAnimation = templateNode->GetComponent<AnimatedSprite2D>();
        if (!templateAnimation)
        {
            URHO3D_LOGERRORF("GameHelpers() - SetRenderedAnimation : Node=%s(%u) no animation for cmap=%s", animatedSprite->GetNode()->GetName().CString(), animatedSprite->GetNode()->GetID(),
                             cmap.CString());
            return;
        }

        AnimatedSprite2D* renderedAnimation = animatedSprite->AddRenderedAnimation(cmap, templateAnimation->GetAnimationSet(), templateAnimation->GetTextureFX());
        if (!renderedAnimation)
        {
            URHO3D_LOGERRORF("GameHelpers() - SetRenderedAnimation : Node=%s(%u) no renderedanimation for cmap=%s", animatedSprite->GetNode()->GetName().CString(), animatedSprite->GetNode()->GetID(),
                             cmap.CString());
            return;
        }

        renderedAnimation->GetNode()->SetEnabled(true);
    }
    else
    {
        animatedSprite->RemoveRenderedAnimation(cmap);
    }
}

String GameHelpers::GetMoveStateString(unsigned movestate)
{
    String str;

    if (movestate == MV_NONE)
        str = "MV_NONE";
    else
    {
        // MoveType Flags
        if (movestate & MV_WALK)
            str += "MV_WALK|";
        if (movestate & MV_FLY)
            str += "MV_FLY|";
        if (movestate & MV_SWIM)
            str += "MV_SWIM|";
        if (movestate & MV_CLIMB)
            str += "MV_CLIMB|";
        if (movestate & MV_CLIMBFIRSTBOX)
            str += "MV_CLIMBFIRSTBOX|";
        if (movestate & MV_MOUNT)
            str += "MV_MOUNT|";

        // Motion States
        if (movestate & MV_INMOVE)
            str += "MV_INMOVE|";
        if (movestate & MV_INJUMP)
            str += "MV_INJUMP|";
        if (movestate & MV_INFALL)
            str += "MV_INFALL|";

        // Area States
        if (movestate & MV_INAIR)
            str += "MV_INAIR|";
        if (movestate & MV_INLIQUID)
            str += "MV_INLIQUID|";

        // Collision States
        if (movestate & MV_TOUCHGROUND)
            str += "MV_TOUCHGROUND|";
        if (movestate & MV_TOUCHWALL)
            str += "MV_TOUCHWALL|";
        if (movestate & MV_TOUCHROOF)
            str += "MV_TOUCHROOF|";
        if (movestate & MV_TOUCHOBJECT)
            str += "MV_TOUCHOBJECT|";

        // Direction States
        if (movestate & MV_KEEPDIRECTION)
            str += "MV_KEEPDIRECTION|";
        if (movestate & MV_DIRECTION)
            str += "MV_DIRECTION|";
    }

    return str;
}


/// Map / Tile Helpers

void GameHelpers::GetWorldMapPosition(const IntVector2& screenposition, WorldMapPosition& position)
{
    World2D::GetWorldInfo()->Convert2WorldMapPosition(ScreenToWorld2D(screenposition), position, position.positionInTile_);
    position.viewZIndex_ = ViewManager::Get()->GetCurrentViewZIndex(0);
}

CollisionShape2D* GameHelpers::GetMapCollisionShapeAt(const WorldMapPosition& position)
{
    Map* map = World2D::GetMapAt(position.mPoint_);

    if (!map)
        return 0;

//    URHO3D_LOGINFOF("GameHelpers() : GetMapCollisionShapeAt : map=%s position=%s ...",
//                    map->GetMapPoint().ToString().CString(), position.ToString().CString());

    int indv = map->GetColliderIndex(ViewManager::Get()->GetCurrentViewZ());
    if (indv != -1)
    {
        Vector2 localPosition = (position.position_ - map->GetStaticNode()->GetWorldPosition2D()) / World2D::GetWorldInfo()->worldScale_;

        PODVector<MapCollider*> colliders;
        map->GetColliders(PHYSICCOLLIDERTYPE, indv, colliders);
        for (unsigned i=0; i < colliders.Size(); i++)
        {
            PhysicCollider& collider = *((PhysicCollider*)colliders[i]);
            for (List<void* >::ConstIterator it=collider.chains_.Begin(); it != collider.chains_.End(); ++it)
            {
                CollisionChain2D* collisionChain = (CollisionChain2D*)(*it);
                //        URHO3D_LOGINFOF("GameHelpers() : GetMapCollisionShapeAt : checkchain=%u at localposition=%s ...", collisionChain, localPosition.ToString().CString());

                if (IsInsidePolygon(localPosition, collisionChain->GetVertices()))
                    return collisionChain;
            }
        }
    }

    return 0;
}

RenderShape* GameHelpers::GetMapRenderShapeAt(const WorldMapPosition& position, int& shapeid, int& contourid, RenderCollider*& rendercollider, bool clipped)
{
    Map* map = World2D::GetMapAt(position.mPoint_);
    contourid = -1;
    shapeid = -1;
    rendercollider = 0;

    if (!map)
        return 0;

//    URHO3D_LOGINFOF("GameHelpers() : GetMapRenderShapeAt : map=%s position=%s ...",
//                    map->GetMapPoint().ToString().CString(), position.ToString().CString());

    const Vector2 localPosition = (position.position_ - map->GetStaticNode()->GetWorldPosition2D()) / World2D::GetWorldInfo()->worldScale_;
    const int indz = ViewManager::Get()->GetCurrentViewZIndex();

    PODVector<MapCollider*> colliders;
    map->GetCollidersAtZIndex(RENDERCOLLIDERTYPE, indz, colliders);

    for (int i=colliders.Size()-1; i >= 0; i--)
    {
        RenderCollider* collider = (RenderCollider*)colliders[i];
        RenderShape* renderShape = collider->renderShape_;

        if (!renderShape)
            continue;
        if (!renderShape->IsEnabled())
            continue;

        const Vector<PolyShape >& shapes = renderShape->GetShapes();
        for (Vector<PolyShape >::ConstIterator it=shapes.Begin(); it != shapes.End(); ++it)
        {
            const PolyShape& shape = *it;

            const Vector<PODVector<Vector2 > >& contours = clipped ? shape.clippedContours_ : shape.contours_;

            for (unsigned j=0; j < contours.Size(); j++)
            {
                if (!contours[j].Size())
                    continue;

//                    URHO3D_LOGINFOF("GameHelpers() : GetMapRenderShapeAt : checkrendershape=%u at localposition=%s ...", renderShape, localPosition.ToString().CString());

                if (IsInsidePolygon(localPosition, contours[j]))
                {
                    shapeid = it - shapes.Begin();
                    contourid = j;
                    rendercollider = collider;
                    return renderShape;
                }
            }
        }
    }

    return 0;
}

// en frontview
// => pouvoir creer des tiles de background et des tiles de frontview
// => pouvoir detruire des tiles de background et des tiles de frontview
// => pouvoir detruire des tiles de outerview qui detruit automatiquement l'innerview et le backview
// => ajouter une option, pour ne jamais creer de tiles de background

// en innerview
// => pouvoir creer des tiles d'innerview qui ajoute automatiquement des tiles en backview et outerview
// => pouvoir creer des tiles de background
// => pouvoir detruire des tiles d'innerview solid qui seront remplacés par InnerSpace
// => pouvoir detruire des tiles de background si pas de tiles en frontview

bool GameHelpers::AddTile(const WorldMapPosition& position, bool backgroundAdding)
{
    MapBase* map;

    ObjectMaped::GetPhysicObjectAt(position.position_, map, true);
    if (!map)
    {
        map = World2D::GetMapAt(position.mPoint_);
        if (!map)
            return false;
    }

    const WorldMapPosition& wpos = map->GetType() == Map::GetTypeStatic() ? position : ObjectMaped::GetWorldMapPositionResult();

    int layerZ = ViewManager::Get()->GetViewZ(position.viewZIndex_);
    FeatureType feat;
    bool result = map->FindEmptyTile(wpos.tileIndex_, feat, layerZ);

    if (result)
    {
        if (layerZ > BACKGROUND && layerZ < FRONTVIEW)
        {
            // Always Create Tile in INNERVIEW except for FRONTVIEW
            // Map::SetTile and Map::ApplyFeatures will update the BACKVIEW and OUTERVIEW tiles
            layerZ = INNERVIEW;
        }

        if (layerZ == BACKGROUND && !backgroundAdding)
        {
            layerZ = INNERVIEW;
        }

        if (layerZ == BACKGROUND || layerZ >= OUTERVIEW)
            feat = MapFeatureType::OuterFloor;
        else
            feat = map->GetColliderType() == DUNGEONINNERTYPE || map->GetColliderType() == MOBILECASTLETYPE ? MapFeatureType::RoomWall : MapFeatureType::OuterFloor;

        int viewid;
        result = SetTile(map, feat, wpos.tileIndex_, layerZ, viewid);
    }

    return result;
}

static const String ScrapTerrain = "Scraps_Terrain";

int GameHelpers::RemoveTile(const WorldMapPosition& position, bool addeffects, bool adddoor, bool permutesametiles, bool onsameview)
{
    MapBase* map;

    ObjectMaped::GetPhysicObjectAt(position.position_, map, true);
    if (!map)
    {
        map = World2D::GetMapAt(position.mPoint_);
        if (!map)
            return -1;
    }

    const WorldMapPosition& wpos = map->GetType() == Map::GetTypeStatic() ? position : ObjectMaped::GetWorldMapPositionResult();

    int viewZ = ViewManager::Get()->GetViewZ(position.viewZIndex_);
    int layerZ = viewZ;
    int terrainid = -1;
    FeatureType feat;

    bool result = map->FindSolidTile(wpos.tileIndex_, feat, layerZ);

    if (result)
    {
        if (onsameview && layerZ != viewZ)
            return -1;

        // only remove a Tile in BackGround if it's a TileModifier
        // => keep the landscape so no problem with MapTopography and DrawableScroller
//        if (layerZ == BACKGROUND && !map->GetTileModifier(position.mPosition_.x_, position.mPosition_.y_, layerZ))
//        {
//            return -1;
//        }

        if (viewZ == INNERVIEW && layerZ == BACKGROUND)
        {
            if (map->GetFeatures(map->GetViewId(FRONTVIEW))[wpos.tileIndex_] > MapFeatureType::NoRender)
                return -1;
        }

        int viewid = map->GetViewId(layerZ);

        if (layerZ == BACKGROUND || layerZ == FRONTVIEW)
        {
            feat = MapFeatureType::NoMapFeature;
        }
        else
        {
            if (layerZ != INNERVIEW || feat == MapFeatureType::Door)
            {
                feat = MapFeatureType::NoMapFeature;
                layerZ = INNERVIEW;
            }
            else
            {
                feat = map->GetColliderType() == DUNGEONINNERTYPE || map->GetColliderType() == MOBILECASTLETYPE ? MapFeatureType::InnerSpace : MapFeatureType::TunnelInnerSpace;
            }
        }

        URHO3D_LOGINFOF("GameHelpers - RemoveTile : mpoint=%s x=%d y=%d z=%d layerZ=%d feat=%s viewid=%s(%d)...", map->GetMapPoint().ToString().CString(), wpos.mPosition_.x_,
                        wpos.mPosition_.y_, viewZ, layerZ, MapFeatureType::GetName(feat), ViewManager::GetViewName(viewid).CString(), viewid);

        Tile* removedtile = 0;
        result = SetTile(map, feat, wpos.tileIndex_, layerZ, permutesametiles, &removedtile);
        if (result)
        {
            terrainid = removedtile ? removedtile->GetTerrain() : map->GetTerrain(wpos.tileIndex_, viewid);

            // Add Scraps Effects
            if (addeffects)
            {
                // Spawn Wall Scraps
                const MapTerrain* terrain = &World2DInfo::currentAtlas_->GetTerrain(terrainid);
                const Color& terraincolor = terrain ? terrain->GetColor() : Color::WHITE;

                GameHelpers::SpawnScraps(StringHash(ScrapTerrain+String(terrainid)), 20, terraincolor, false, viewZ+LAYER_ACTOR, viewZ, ViewManager::Get()->layerMask_[viewZ], position.position_,
                                         Vector2(2.f,2.f), World2D::GetWorldInfo()->mTileWidth_, 5.f, 50.f, 100.f);

                URHO3D_LOGINFOF("GameHelpers - RemoveTile : scrap=%s terrainid=%d(%d) tile=%u gid=%d...", String(ScrapTerrain+String(terrainid)).CString(), terrainid, removedtile ? removedtile->GetTerrain() : 0, removedtile, removedtile ? removedtile->GetGid() : 0);
            }

            // Add door furniture if feature = door
            if (adddoor && feat == MapFeatureType::Door)
            {
                bool dooratleft;

                if (CheckForDoorPlacement(map, viewid, wpos, dooratleft))
                {
                    EntityData entitydata;
                    //EntityData::Set(short unsigned gotindex=0, short unsigned tileindex=USHRT_MAX, char tilepositionx=0, char tilepositiony_=0, char sstype=-1, int layerZ=0, bool rotate=false, bool flipX=false, bool flipY=false);
                    entitydata.Set(GOT::GetIndex(COT::GetTypeFrom(DUNGEONTHRESHOLD, 0)), wpos.tileIndex_, dooratleft ? 127 : -127, -127, -1, THRESHOLDVIEW, false, !dooratleft, false);
                    Node* node = map->AddFurniture(entitydata);
                }
            }
        }
    }

    return terrainid;
}

bool GameHelpers::SetTile(MapBase* map, FeatureType feat, unsigned tileindex, int layerZ, bool permutesametiles, Tile** removedtile)
{
    int x = map->GetTileCoordX(tileindex);
    int y = map->GetTileCoordY(tileindex);
#ifdef DUMP_MAPDEBUG_SETTILE
    URHO3D_LOGINFOF("GameHelpers - SetTile : mpoint=%s x=%d y=%d z=%d ...",
                    map->GetMapPoint().ToString().CString(), x, y, layerZ);
#endif
    bool result = map->CanSetTile(feat, x, y, layerZ, permutesametiles);
    if (result)
    {
        World2D::DestroyFurnituresAt(map, tileindex);

        map->SetTile(feat, x, y, layerZ, removedtile);

        int viewid = map->GetViewId(layerZ);
        feat = map->GetFeatureType(tileindex, viewid);
#ifdef DUMP_MAPDEBUG_SETTILE
        URHO3D_LOGINFOF("GameHelpers - SetTile : mpoint=%s x=%d y=%d ... add feat=%s(%u) on layerZ=%d viewid=%s(%d) OK !",
                        map->GetMapPoint().ToString().CString(), x, y, MapFeatureType::GetName(feat), feat, layerZ, ViewManager::GetViewName(viewid).CString(), viewid);
#endif
    }

    return result;
}

bool GameHelpers::SetTiles(MapBase* map, FeatureType feat, const Vector<unsigned>& tileindexes, int layerZ, bool entitymode)
{
    Tile* tile;
    unsigned tileindex;
    int x, y;
    Vector<unsigned> acceptedTileIndexes;

    for (unsigned i=0; i < tileindexes.Size(); i++)
    {
        tileindex = tileindexes[i];
        x = map->GetTileCoordX(tileindex);
        y = map->GetTileCoordY(tileindex);
        if (map->CanSetTile(feat, x, y, layerZ))
            acceptedTileIndexes.Push(tileindex);
    }

    if (acceptedTileIndexes.Size())
    {
        int viewid = map->GetViewId(layerZ);

        if (entitymode)
        {
            for (unsigned i=0; i < acceptedTileIndexes.Size(); i++)
            {
                tileindex = acceptedTileIndexes[i];

                if (map->SetTileEntity(feat, tileindex, layerZ))
                {
                    World2D::DestroyFurnituresAt(map, tileindex);

                    Vector2 position = map->GetWorldTilePosition(IntVector2(map->GetTileCoordX(tileindex), map->GetTileCoordY(tileindex)));

                    int terrainid = map->GetTerrain(tileindex, viewid);
                    if (terrainid == -1)
                        continue;

                    const MapTerrain* terrain = &World2DInfo::currentAtlas_->GetTerrain(terrainid);
                    const Color& terraincolor = terrain ? terrain->GetColor() : Color::WHITE;

                    GameHelpers::SpawnScraps(StringHash(ScrapTerrain+String(terrainid)), 20, terraincolor, false, layerZ+LAYER_ACTOR, layerZ, ViewManager::Get()->layerMask_[layerZ], position,
                                             Vector2(2.f,2.f), World2D::GetWorldInfo()->mTileWidth_, 5.f, 50.f, 100.f);
                }
            }
        }
        else
        {
            Vector<Tile*> removedTiles;
            for (unsigned i=0; i < acceptedTileIndexes.Size(); i++)
                removedTiles.Push(map->GetTile(acceptedTileIndexes[i], viewid));

            map->SetTiles(feat, layerZ, acceptedTileIndexes);

            Vector2 position;

            for (unsigned i=0; i < acceptedTileIndexes.Size(); i++)
            {
                tileindex = acceptedTileIndexes[i];

                World2D::DestroyFurnituresAt(map, tileindex);

                position = map->GetWorldTilePosition(IntVector2(map->GetTileCoordX(tileindex), map->GetTileCoordY(tileindex)));

                int terrainid = removedTiles[i]->GetTerrain();
                const MapTerrain* terrain = &World2DInfo::currentAtlas_->GetTerrain(terrainid);
                const Color& terraincolor = terrain ? terrain->GetColor() : Color::WHITE;

                GameHelpers::SpawnScraps(StringHash(ScrapTerrain+String(terrainid)), 20, terraincolor, false, layerZ+LAYER_ACTOR, layerZ, ViewManager::Get()->layerMask_[layerZ], position,
                                         Vector2(2.f,2.f), World2D::GetWorldInfo()->mTileWidth_, 5.f, 50.f, 100.f);
            }
        }
    }

    return acceptedTileIndexes.Size();
}

bool GameHelpers::CheckForDoorPlacement(MapBase* map, int viewid, const WorldMapPosition& wpos, bool& dooratleft)
{
    FeatureType* features = map->GetFeatures(viewid);

    // check if on border
    if (wpos.mPosition_.y_ >= map->GetHeight()-1 || wpos.mPosition_.x_ <= 0 || wpos.mPosition_.x_ >= map->GetWidth()-1)
        return false;

    // check if bottom tile is not solid
    if (features[wpos.tileIndex_ + map->GetWidth()] <= MapFeatureType::NoRender)
        return false;

    FeatureType featleft = features[wpos.tileIndex_ - 1];
    FeatureType featright = features[wpos.tileIndex_ + 1];

    dooratleft = (featleft == MapFeatureType::NoMapFeature && featright == MapFeatureType::RoomInnerSpace);
    bool dooratright = (featright == MapFeatureType::NoMapFeature && featleft == MapFeatureType::RoomInnerSpace);

    if (dooratleft || dooratright)
    {
        return true;
    }

    return false;
}

void GameHelpers::DumpTile(const WorldMapPosition& position)
{
    int layerZ = ViewManager::Get()->GetViewZ(position.viewZIndex_);
    Map* map = World2D::GetMapAt(position.mPoint_);

    if (!map)
        return;

    unsigned chunk = MapInfo::info.chinfo_->GetChunk(position.mPosition_.x_, position.mPosition_.y_);

    PODVector<FeatureType> features;
    String featureString, maskString;
    map->GetObjectFeatured()->GetAllViewFeatures(position.tileIndex_, features);
    for (unsigned i=0; i < features.Size(); i++)
    {
        featureString += ToString("%s(%d)=>%s(%d);",ViewManager::GetViewName(i).CString(), i, MapFeatureType::GetName(features[i]), features[i]);
    }

    FeatureType feat;
    bool result = map->FindSolidTile(position.tileIndex_, feat, layerZ);
    String solidTileString;
    if (result)
    {
        int viewid = map->GetViewId(layerZ);
        solidTileString = ToString("findsolidtile feat=%s(%u) at (%s(%d) Z=%d)", MapFeatureType::GetName(feat), feat, ViewManager::GetViewName(viewid).CString(), viewid, layerZ);
    }

    const Vector<FeaturedMap>& masks = map->GetObjectFeatured()->GetMaskedViews(position.viewZIndex_);
    for (int i=masks.Size()-1; i >= 0; i--)
    {
        maskString += ToString("indexV=%d=>%s(%d);", i, MapFeatureType::GetName(masks[i][position.tileIndex_]), masks[i][position.tileIndex_]);
    }

    URHO3D_LOGINFOF("GameHelpers() : DumpTile => map=%s pos=%s ti=%u chunkid=%u : ...", position.mPoint_.ToString().CString(), position.mPosition_.ToString().CString(), position.tileIndex_, chunk);
    URHO3D_LOGINFOF("   %s", solidTileString.CString());
    URHO3D_LOGINFOF("   features = %s", featureString.CString());
    URHO3D_LOGINFOF("   masks    = %s", maskString.CString());
}

Node* GameHelpers::CreateObjectMapedFrom(const ObjectMapedInfo& info)
{
    URHO3D_LOGINFOF("GameHelpers - CreateObjectFrom : ...");

    Node* node = GameContext::Get().rootScene_->CreateChild();

    node->CreateComponent<ObjectMaped>()->CreateFrom(info);

    URHO3D_LOGINFOF("GameHelpers - CreateObjectFrom : ... OK !");

    return node;
}

const StringHash& GameHelpers::GetRandomMonsters(Vector<StringHash>* authorizedCategories)
{
    GameRand& ORand = GameRand::GetRandomizer(OBJRAND);

    if (authorizedCategories)
        return COT::GetRandomTypeFrom(authorizedCategories->At(ORand.Get(authorizedCategories->Size())), ORand.Get(100));

    return COT::GetRandomTypeFrom(COT::MONSTERS, ORand.Get(100));
}

/// Check if the tiles are free (no block) at world position
bool GameHelpers::CheckFreeTilesAtViewZ(const GOC_Destroyer* destroyer, MapBase* map, unsigned tileindex, int viewZ)
{
    const Vector2& center = destroyer->GetWorldMapPosition().positionInTile_;
    const Rect& rect = destroyer->GetWorldMapPosition().shapeRectInTile_;

    // First : check if the mass center is on a blocktile
    FluidCell* cell = map->GetFluidCellPtr(tileindex, map->GetObjectFeatured()->GetIndexViewZ(viewZ));
    if (!cell)
        return true;

    if (cell && cell->type_ == BLOCK)
        return false;

//    URHO3D_LOGERRORF("Map() - CheckFreeTilesAtViewZ : map=%s viewZ=%d center=%s rect=%s !", GetMapPoint().ToString().CString(), viewZ,
//                     center.ToString().CString(), rect.ToString().CString());

    // Check Intersect Bottom Tile
    if (center.y_+rect.min_.y_ < 0.f && cell->Bottom && cell->Bottom->type_ == BLOCK)
        return false;
    // Check Intersect Top Tile
    if (center.y_+rect.max_.y_ > Map::info_->mTileHeight_ && cell->Top && cell->Top->type_ == BLOCK)
        return false;
    // Check Intersect Left Tile
    if (center.x_+rect.min_.x_ < 0.f && cell->Left && cell->Left->type_ == BLOCK)
        return false;
    // Check Intersect Right Tile
    if (center.x_+rect.max_.x_ > Map::info_->mTileWidth_ && cell->Right && cell->Right->type_ == BLOCK)
        return false;

    return true;
}

void GameHelpers::AdjustPositionInTile(GOC_Destroyer* destroyer, MapBase* map, const WorldMapPosition& initialposition)
{
    bool updateposition = false;

    WorldMapPosition& position = destroyer->GetWorldMapPosition();

    if (!position.shapeRectInTile_.Defined())
        destroyer->UpdateShapesRect();

    const Vector2& center = initialposition.positionInTile_;
    const Rect& rect = position.shapeRectInTile_;

    int viewZ = initialposition.viewZ_;

    if (viewZ == -1)
    {
        URHO3D_LOGERRORF("GameHelpers() - AdjustPositionInTile : map=%s destroyer=%s viewZ=-1 => set to FRONTVIEW !", map->GetMapPoint().ToString().CString(), destroyer->GetNode()->GetName().CString());
        viewZ = FRONTVIEW;
    }

    // First : check if the mass center is on a blocktile
    FluidCell* cell = map->GetFluidCellPtr(initialposition.tileIndex_, map->GetObjectFeatured()->GetIndexViewZ(viewZ));
    if (cell && cell->type_ == BLOCK)
    {
        URHO3D_LOGERRORF("GameHelpers() - AdjustPositionInTile : map=%s center=%s rect=%s is in a BLOCK Tile at %s !", map->GetMapPoint().ToString().CString(),
                         center.ToString().CString(), rect.ToString().CString(), initialposition.ToString().CString());
        return;
    }

    Vector2 offset;

    // Adjust if intersects with Bottom Tile
    if (center.y_+rect.min_.y_ < 0.f && cell->Bottom && cell->Bottom->type_ == BLOCK)
    {
        offset.y_ += Abs(rect.min_.y_);
        offset.y_ += 0.05f;
        updateposition = true;

//        URHO3D_LOGINFOF("GameHelpers() - AdjustPositionInTile : Map=%s intersection with Bottom Cell => adjust position with offset=%s !", map->GetMapPoint().ToString().CString(),
//                         offset.ToString().CString());
    }
    // Adjust if intersects with Top Tile
    if (center.y_+rect.max_.y_ > Map::info_->mTileHeight_ && cell->Top && cell->Top->type_ == BLOCK)
    {
        offset.y_ -= Abs(rect.max_.y_);
        offset.y_ -= 0.05f;
        updateposition = true;

//        URHO3D_LOGINFOF("GameHelpers() - AdjustPositionInTile : Map=%s intersection with Top Cell => adjust position with offset=%s !", map->GetMapPoint().ToString().CString(),
//                         offset.ToString().CString());
    }
    // Adjust if intersects with Left Tile
    if (center.x_+rect.min_.x_ < 0.f && cell->Left && cell->Left->type_ == BLOCK)
    {
        offset.x_ += Abs(rect.min_.x_);
        offset.x_ += 0.05f;
        updateposition = true;

//        URHO3D_LOGINFOF("GameHelpers() - AdjustPositionInTile : Map=%s intersection with Left Cell => adjust position with offset=%s !", map->GetMapPoint().ToString().CString(),
//                         offset.ToString().CString());
    }
    // Adjust if intersects with Right Tile
    if (center.x_+rect.max_.x_ > Map::info_->mTileWidth_ && cell->Right && cell->Right->type_ == BLOCK)
    {
        offset.x_ -= Abs(rect.max_.x_);
        offset.x_ -= 0.05f;
        updateposition = true;

//        URHO3D_LOGINFOF("GameHelpers() - AdjustPositionInTile : Map=%s intersection with Right Cell => adjust position with offset=%s !", map->GetMapPoint().ToString().CString(),
//                         offset.ToString().CString());
    }

    if (updateposition)
    {
        Vector2 newposition = initialposition.position_ + offset;
        position.position_ = newposition;

        destroyer->SetWorldMapPosition(position);

        URHO3D_LOGERRORF("GameHelpers() - AdjustPositionInTile : Map=%s update position=%s(%s) !", map->GetMapPoint().ToString().CString(),
                        newposition.ToString().CString(), position.ToString().CString());
    }
}

/// Pixel Shape

void SetPixelShapeBlock(PixelShape& shape, int x, int y, unsigned char value)
{
    // adjust if even
    if (shape.width_ % 2 == 0)
        x += -Sign(x) * 0.5f;
    if (shape.height_ % 2 == 0)
        y += -Sign(y) * 0.5f;

    // recenter
    x += shape.centerx_;
    y = shape.centery_ - y;

    shape.data_[y*shape.width_+x] = value;

    if (value > MapFeatureType::NoRender)
        shape.pixelcoords_.Push(IntVector2(x, y));
}

void SetPixelShapeLine(PixelShape& shape, int x1, int y1, int x2, int y2, int steps, unsigned char value)
{
    float xstep = (x2 - x1) / steps;
    float ystep = (y2 - y1) / steps;
    for (int count = 0; count < steps; count ++)
        SetPixelShapeBlock(shape, Round(x1 + count * xstep), Round(y1 + count * ystep), value);
}

PixelShape* GameHelpers::GetPixelShape(int geometry, int width, int height, unsigned char value1, unsigned char value2, int rand)
{
    if (geometry >= MAX_PIXSHAPETYPE)
        return 0;

    unsigned shapeid = (geometry << 16) + (width << 8) + height;

    PixelShape& shape = GameContext::Get().sConstPixelShapes_[shapeid];

    if (shape.data_.Size())
    {
//        GAME_SETGAMELOGENABLE(GAMELOG_MAPCREATE, true);
//        URHO3D_LOGINFOF("GameHelpers() - GetPixelShape : reuse PIXSHAPE_SUPERELLIPSE id=%u w=%d,h%d !", id, width, height);
//        GAME_SETGAMELOGENABLE(GAMELOG_MAPCREATE, false);
        return &shape;
    }

    shape.width_ = width;
    shape.height_ = height;
    shape.centerx_ = width / 2;
    shape.centery_ = height / 2;
    shape.data_ = PODVector<unsigned char>(width*height, 0);

    if (geometry == PIXSHAPE_RECT)
    {
        SetPixelShapeLine(shape, 0, 0, width-1, 0, 100, value1);
        SetPixelShapeLine(shape, 0, height-1, width-1, height-1, 100, value1);
        SetPixelShapeLine(shape, 0, 1, 0, height-2, 100, value1);
        SetPixelShapeLine(shape, width-1, 1, width-1, height-2, 100, value1);

        return &shape;
    }
    else if (geometry == PIXSHAPE_SPIRAL)
    {
//        URHO3D_LOGINFOF("GameHelpers() - GetPixelShape : make a new PIXSHAPE_SPIRAL !");

        float rot = 0.f;
        float loops = 2.5f;
        float a = Min(width, height) / 4.f / loops / 180.f;
        float r = rot * M_DEGTORAD;
        for (float t = 0.f; t < loops * 360.f; t += 0.1f)
        {
            SetPixelShapeBlock(shape, Round(a * t * Cos(t + rot)), Round(a * t * Sin(t + rot)), value1);
        }

        return &shape;
    }
    else if (geometry == PIXSHAPE_SUPERELLIPSE)
    {
        /// Lame Curves
        /// https://fr.wikipedia.org/wiki/Courbe_de_Lam%C3%A9

//        URHO3D_LOGERRORF("GameHelpers() - GetPixelShape : make a new PIXSHAPE_SUPERELLIPSE id=%u !", id);
        rand = rand%100;
        float step = 0.1f;
        float exponent = 2.f / (1.65f + 0.01f * rand); // 1.65f to 2.65f
        int hw = width/2;
        int hh = height/2;
        for (float t = 90.f-step; t >= 0.f; t -= step)
        {
            float cospowered = Pow(Cos(t), exponent);
            float sinpowered = Pow(Sin(t), exponent);
            SetPixelShapeBlock(shape, Round(-hw * cospowered), Round(hh * sinpowered), value1);
            SetPixelShapeBlock(shape, Round(hw * cospowered), Round(-hh * sinpowered), value1);
        }
        for (float t = 0.f; t < 90.f; t += step)
        {
            float cospowered = Pow(Cos(t), exponent);
            float sinpowered = Pow(Sin(t), exponent);
            SetPixelShapeBlock(shape, Round(hw * cospowered), Round(hh * sinpowered), value1);
            SetPixelShapeBlock(shape, Round(-hw* cospowered), Round(-hh * sinpowered), value1);
        }

        // then connect the curves
        float cos1powered = Pow(Cos(1.f), exponent);
        float sin1powered = Pow(Sin(1.f), exponent);
        float cos89powered = Pow(Cos(89.f), exponent);
        float sin89powered = Pow(Sin(89.f), exponent);
        SetPixelShapeLine(shape, Round(hw * cos89powered), Round(hh * sin89powered), Round(-hw * cos89powered), Round(hh * sin89powered), 100, value1);
        SetPixelShapeLine(shape, Round(-hw * cos1powered), Round(hh * sin1powered), Round(-hw * cos1powered), Round(-hh * sin1powered), 100, value1);
        SetPixelShapeLine(shape, Round(-hw * cos89powered), Round(-hh * sin89powered), Round(hw * cos89powered), Round(-hh * sin89powered), 100, value1);
        SetPixelShapeLine(shape, Round(hw * cos1powered), Round(-hh * sin1powered), Round(hw * cos1powered), Round(hh * sin1powered), 100, value1);

        if (value2 > 0)
        {
            Matrix2D<unsigned char> buffer(width, height, &shape.data_[0]);
            bool filled = GameHelpers::FloodFill(buffer, GameContext::Get().sMapStack_, (unsigned char)0, value2, shape.centerx_, shape.centery_);
            if (!filled)
                URHO3D_LOGERRORF("GameHelpers() - GetPixelShape : PIXSHAPE_SUPERELLIPSE not filled !");
        }

//        GAME_SETGAMELOGENABLE(GAMELOG_MAPCREATE, true);
//        URHO3D_LOGINFOF("GameHelpers() - GetPixelShape : make a new PIXSHAPE_SUPERELLIPSE id=%u w=%d,h%d !", id, width, height);
//        GameHelpers::DumpData(&shape.data_[0], 1, 2, width, height);
//        GAME_SETGAMELOGENABLE(GAMELOG_MAPCREATE, false);

        return &shape;
    }

    URHO3D_LOGERRORF("GameHelpers() - GetPixelShape : make an empty shape !");
    return 0;
}

PixelShape* GameHelpers::GetPixelShape(int id)
{
    int shapetype = (id >> 16) & 0xFF;
    int width = (id >> 8) & 0xFF;
    int height = id & 0xFF;

    return GameHelpers::GetPixelShape(shapetype, width, height);
}


/// Audio Helpers

void GameHelpers::SpawnSound(Node* node, const char* fileName, float gain)
{
    if (!node || !GameContext::Get().gameConfig_.soundEnabled_)
        return;

    Sound* sound = node->GetContext()->GetSubsystem<ResourceCache>()->GetResource<Sound>(fileName);

    SoundSource* soundSource = node->GetOrCreateComponent<SoundSource>(LOCAL);
    soundSource->SetSoundType(SOUND_EFFECT);
    soundSource->SetAutoRemoveMode(REMOVE_COMPONENT);
    soundSource->SetGain(gain);
    soundSource->Play(sound);
}

void GameHelpers::SpawnSound3D(Node* node, const char* fileName, float gain)
{
    if (!node || !GameContext::Get().gameConfig_.soundEnabled_)
        return;

    Sound* sound = node->GetContext()->GetSubsystem<ResourceCache>()->GetResource<Sound>(fileName);

    SoundSource3D* soundSource = node->GetOrCreateComponent<SoundSource3D>(LOCAL);
    soundSource->SetSoundType(SOUND_EFFECT);
    soundSource->SetAutoRemoveMode(REMOVE_COMPONENT);
    soundSource->SetGain(gain);
    soundSource->SetAttenuation(0.7f);
    soundSource->SetFarDistance(12.f);
    soundSource->Play(sound);
}

void GameHelpers::CreateMusicNode(Scene* scene)
{
    if (!GameContext::Get().gameConfig_.musicEnabled_ || !scene)
        return;

    // Create a scene node and a sound source for the music
    Node* musicNode = scene->GetChild("Music");
    if (!musicNode)
        musicNode = scene->CreateChild("Music");
}

void GameHelpers::PlayMusic(Scene* scene, const char* fileName, bool loop, float gain)
{
    if (!GameContext::Get().gameConfig_.musicEnabled_ || !scene)
        return;

    Context* context = scene->GetContext();

    ResourceCache* cache = context->GetSubsystem<ResourceCache>();
    Sound* music = cache->GetResource<Sound>(fileName);
    if (!music)
    {
        URHO3D_LOGERRORF("GameContext() - PlayMusic : no fileName=%s !", fileName);
        return;
    }

    // Set the song to loop
    music->SetLooped(loop);

    // Create a scene node and a sound source for the music
    Node* musicNode = scene->GetChild("Music", LOCAL);
    if (!musicNode)
        musicNode = scene->CreateChild("Music", LOCAL);

    // Set the sound type to music so that master volume control works correctly
    SoundSource* musicSource = musicNode->GetOrCreateComponent<SoundSource>(LOCAL);
    musicSource->SetSoundType(SOUND_MUSIC);
//    musicSource->SetGain(gain);

    if (musicSource->IsPlaying())
        musicSource->Stop();

    musicSource->Play(music);
}

void GameHelpers::StopMusic(Scene* scene)
{
    // Remove the music player node from the scene
    Node* musicNode = scene->GetChild("Music", LOCAL);

    if (musicNode)
    {
        SoundSource* musicSource = musicNode->GetComponent<SoundSource>(LOCAL);
        if (musicSource && musicSource->IsPlaying())
            musicSource->Stop();
    }
}

void GameHelpers::SetSoundVolume(float volume)
{
    GameContext::Get().context_->GetSubsystem<Audio>()->SetMasterGain(SOUND_EFFECT, volume);
}

void GameHelpers::SetMusicVolume(float volume)
{
    GameContext::Get().context_->GetSubsystem<Audio>()->SetMasterGain(SOUND_MUSIC, volume);
}

//void GameHelpers::HandleSoundVolume(StringHash eventType, VariantMap& eventData)
//{
//    using namespace SliderChanged;
//
//    float newVolume = eventData[P_VALUE].GetFloat();
//    GetSubsystem<Audio>()->SetMasterGain(SOUND_EFFECT, newVolume);
//}
//
//void GameHelpers::HandleMusicVolume(StringHash eventType, VariantMap& eventData)
//{
//    using namespace SliderChanged;
//
//    float newVolume = eventData[P_VALUE].GetFloat();
//    GetSubsystem<Audio>()->SetMasterGain(SOUND_MUSIC, newVolume);
//}


/// Math / Converter Helpers

bool GameHelpers::IsInsidePolygon(const Vector2& point, const PODVector<Vector2>& polygon)
{
    int i, j, c = 0;
    int numvertices = polygon.Size();

    for (i = 0, j = numvertices-1; i < numvertices; j = i++)
    {
        if ( ((polygon[i].y_ > point.y_) != (polygon[j].y_ > point.y_)) &&
                (point.x_ < (polygon[j].x_ - polygon[i].x_) * (point.y_ - polygon[i].y_) / (polygon[j].y_ - polygon[i].y_) + polygon[i].x_) )
            c = !c;
    }

    // impair : le point est à l'interieur
    return c%2 != 0;
}

void GameHelpers::ClampSizeTo(IntVector2& size, int dimension)
{
    float ratioSize = float(size.y_) / float(size.x_);
    if (ratioSize > 1.f)
    {
        size.y_ = Clamp(size.y_, 1, dimension);
        size.x_ = int(float(size.y_) / ratioSize);
    }
    else
    {
        size.x_ = Clamp(size.x_, 1, dimension);
        size.y_ = int(float(size.x_) * ratioSize);
    }
}

Vector2 GameHelpers::ScreenToWorld2D(int screenx, int screeny)
{
    Viewport* viewport = GameContext::Get().context_->GetSubsystem<Renderer>()->GetViewport(ViewManager::Get()->GetViewportAt(screenx, screeny));
    if (viewport)
    {
        Vector3 wpoint = viewport->ScreenToWorldPoint(screenx, screeny, 0.f);
        return Vector2(wpoint.x_, wpoint.y_);
    }

    return Vector2::ZERO;
}

Vector2 GameHelpers::ScreenToWorld2D(const IntVector2& screenpoint)
{
    return ScreenToWorld2D(screenpoint.x_, screenpoint.y_);
}

void GameHelpers::OrthoWorldToScreen(IntVector2& point, Node* node, int viewport)
{
    // Bug in Camera WorldToScreenPoint in Orthographic mode ?
    // do it right !

    const IntRect& viewportrect = ViewManager::Get()->GetViewportRect(viewport);
    Rect viewrect = ViewManager::Get()->GetViewRect(viewport);

    Vector2 position = node->GetWorldPosition2D();
    point.x_ = viewportrect.left_ + (int)((position.x_ - viewrect.min_.x_) / (viewrect.max_.x_ - viewrect.min_.x_) * (float)viewportrect.Size().x_);
    point.y_ = viewportrect.top_ + (int)((1.f - (position.y_ - viewrect.min_.y_) / (viewrect.max_.y_ - viewrect.min_.y_)) * (float)viewportrect.Size().y_);
}

void GameHelpers::OrthoWorldToScreen(IntRect& rect, const BoundingBox& b, int vport)
{
    // Bug in Camera WorldToScreenPoint in Orthographic mode ?
    // do it right !

    Vector3 point1(b.min_.x_, b.min_.y_, 1.f);
    Vector3 point2(b.max_.x_, b.max_.y_, 1.f);

    Viewport* viewport = GameContext::Get().context_->GetSubsystem<Renderer>()->GetViewport(vport);

    float screenwidth = viewport->GetRect().Size().x_;
    float screenheight = viewport->GetRect().Size().y_;
    float screenratio = screenheight/screenwidth;

    Camera* camera = viewport->GetCamera();
    Vector3 screenSpacePos = camera->GetProjection() * camera->GetView() * point1;
    point1.x_ = (screenSpacePos.x_*screenratio + 1.f) * 0.5f;
#ifdef URHO3D_VULKAN
    point1.y_ = (screenSpacePos.y_ + 1.f) * 0.5f;
#else
    point1.y_ = 1.f - (screenSpacePos.y_ + 1.f) * 0.5f;
#endif

    screenSpacePos = camera->GetProjection() * camera->GetView() * point2;
    point2.x_ = (screenSpacePos.x_*screenratio + 1.f) * 0.5f;
#ifdef URHO3D_VULKAN
    point2.y_ = (screenSpacePos.y_ + 1.f) * 0.5f;
#else
    point2.y_ = 1.f - (screenSpacePos.y_ + 1.f) * 0.5f;
#endif

    rect.left_ = point1.x_ * screenwidth; // / GameContext::Get().uiScale_;
    rect.bottom_ = point1.y_ * screenheight;// / GameContext::Get().uiScale_;
    rect.right_ = point2.x_ * screenwidth;// / GameContext::Get().uiScale_;
    rect.top_ = point2.y_ * screenheight;// /  GameContext::Get().uiScale_;

//    URHO3D_LOGINFOF("GameHelpers - OrthoWorldToScreen : bbox=%s => ar=%f screenrect=%s",
//                    b.ToString().CString(), screenratio, rect.ToString().CString());
}

void GameHelpers::SpriterCorrectorScaleFactor(Context* context, const char *filename, const float& scale_factor)
{
    // file
    String savename = GetParentPath(filename) + GetFileName(filename) + "_fixed.scml";

    File f(context, filename, FILE_READ);
    File f2(context, savename, FILE_WRITE);

    XMLFile* xmlFile = new XMLFile(context);
    if (!xmlFile->Load(f))
    {
        URHO3D_LOGERROR("Load XML failed " + f.GetName());
        f.Close();
        return;
    }

    if (!xmlFile)
    {
        f.Close();
        return;
    }

    XMLElement rootElem = xmlFile->GetRoot("spriter_data");

    XMLElement entityElem = rootElem.GetChild("entity");
    if (!entityElem)
    {
        URHO3D_LOGERROR("Could not find entity");
        return;
    }

    for (XMLElement animElem = entityElem.GetChild("animation"); animElem; animElem = animElem.GetNext("animation"))
    {
        for (XMLElement timeElem = animElem.GetChild("timeline"); timeElem; timeElem = timeElem.GetNext("timeline"))
        {
//            String ObjType = "";
//            if (timeElem.HasAttribute("object_type")) String ObjType = timeElem.GetAttribute("object_type");
//            if (ObjType == "box" || ObjType == "" ) // "" => par defaut = objet sprite
//            {
            if (!(timeElem.HasAttribute("object_type")))
            {
                for (XMLElement keyElem = timeElem.GetChild("key"); keyElem; keyElem = keyElem.GetNext("key"))
                {
                    XMLElement objectElem = keyElem.GetChild("object");
                    if (objectElem.HasAttribute("scale_x")) objectElem.SetFloat("scale_x",objectElem.GetFloat("scale_x")*scale_factor);
                    if (objectElem.HasAttribute("scale_y")) objectElem.SetFloat("scale_y",objectElem.GetFloat("scale_y")*scale_factor);
                }
            }
        }
    }
    if (!xmlFile->Save(f2)) URHO3D_LOGERROR("Can not save XML");
    f2.Close();
    f.Close();
}

void GameHelpers::EqualizeValues(PODVector<float>& values, int coeff, bool incbleft, bool incbright)
{
    unsigned numvalues = values.Size();

    if (numvalues < 2)
        return;

    unsigned starti = (incbleft || numvalues < 3) ? 0 : 1;
    unsigned endi = (incbright || numvalues < 3) ? numvalues : numvalues-1;

    // get the average

    float average = 0.f;
    for (unsigned i=0; i < numvalues; i++)
    {
        average += values[i];
    }
    average /= numvalues;

    // smooth each value to the average
    int sumcoeff = coeff+1;
    for (unsigned i=starti; i < endi; i++)
        values[i] = (values[i] + coeff*average) / sumcoeff;
}

/// FloodFill : remplit les zones continues definies par la valeur patternToFill avec le motif fillPattern
/// si bordercheck est spécifié alors stop le floodfill si franchissement de cette bordure
template< typename T >
bool GameHelpers::FloodFill(Matrix2D<T>& buffer, Stack<unsigned>& stack, const T& patternToFill, const T& fillPattern, const int& xo, const int& yo, const IntRect& bordercheck)
{
//    URHO3D_LOGINFOF("FloodFill ... x=%d y=%d patternToFill=%d fillPattern=%d", xo, yo, (int)patternToFill, (int)fillPattern);

    int width = buffer.Width();
    int height = buffer.Height();
    int y1, num = 0;
    bool spanLeft, spanRight;
    const bool checkborder = (&bordercheck != &IntRect::ZERO);

//    URHO3D_LOGINFOF("FloodFill ... Init OK ...");

    stack.Clear();

    int x = xo, y = yo;
    unsigned addr = width*y + x;

    if (!stack.Push(addr))
    {
        URHO3D_LOGERROR("FloodFill ... Memory Stack Error");
        return false;
    }

//    URHO3D_LOGINFOF("FloodFill ... Push x=%d y=%d", x, y);

    while (stack.Pop(addr))
    {
        x = addr % width;
        y = addr / width;
        y1 = y;
        while(y1 >= 0 && buffer(x,y1) == patternToFill) y1--;
        y1++;

        spanLeft = spanRight = 0;
        while (y1 < height && buffer(x,y1) == patternToFill)
        {
            if (checkborder && (y1 < bordercheck.top_ || y1 > bordercheck.bottom_ || x > bordercheck.right_ || x < bordercheck.left_))
                return false;

            buffer(x,y1) = fillPattern;
//            URHO3D_LOGINFOF("FloodFill ... Fill x=%d y=%d with fillPattern=%d result=%d", x, y1, (int)fillPattern, (int)buffer(x,y1));
            num++;
            if (!spanLeft && x > 0 && buffer(x-1,y1) == patternToFill)
            {
                if (!stack.Push(y1*width+x-1)) return false;
//                URHO3D_LOGINFOF("FloodFill ... Push x=%d y=%d num=%d Left", x-1, y1, num);
                spanLeft = 1;
            }
            else if (spanLeft && x > 0 && buffer(x-1,y1) != patternToFill)
            {
                spanLeft = 0;
            }
            if (!spanRight && x < width - 1 && buffer(x+1,y1) == patternToFill)
            {
                if (!stack.Push(y1*width+x+1)) return false;
//                URHO3D_LOGINFOF("FloodFill ... Push x=%d y=%d num=%d Right", x+1, y1, num);
                spanRight = 1;
            }
            else if (spanRight && x < width - 1 && buffer(x+1,y1) != patternToFill)
            {
                spanRight = 0;
            }
            y1++;
        }
    }

//    URHO3D_LOGINFOF("FloodFill ... OK ! num = %d", num);

    return num!=0;
}

/// FloodFill : remplit les zones continues definit par les valeurs comprises entre patternToFillFrom et patternToFillTo avec le motif fillPattern
template< typename T >
bool GameHelpers::FloodFill(Matrix2D<T>& buffer, Stack<unsigned>& stack, const T& patternToFillFrom, const T& patternToFillTo, const T& fillPattern, const int& xo, const int& yo)
{
//    URHO3D_LOGINFOF("FloodFill ... x=%d y=%d patternToFill=%d fillPattern=%d", xo, yo, (int)patternToFill, (int)fillPattern);

    int width = buffer.Width();
    int height = buffer.Height();
    int y1, num = 0;
    bool spanLeft, spanRight;

//    URHO3D_LOGINFOF("FloodFill ... Init OK ...");

    stack.Clear();

    int x = xo, y = yo;
    unsigned addr = width*y + x;

    if (!stack.Push(addr))
    {
        URHO3D_LOGERROR("FloodFill ... Memory Stack Error");
        return false;
    }

//    URHO3D_LOGINFOF("FloodFill ... Push x=%d y=%d", x, y);

    while (stack.Pop(addr))
    {
        x = addr % width;
        y = addr / width;
        y1 = y;
        while(y1 >= 0 && (buffer(x,y1) >= patternToFillFrom && buffer(x,y1) <= patternToFillTo)) y1--;
        y1++;
        spanLeft = spanRight = 0;
        while (y1 < height && (buffer(x,y1) >= patternToFillFrom && buffer(x,y1) <= patternToFillTo))
        {
            buffer(x,y1) = fillPattern;
//            URHO3D_LOGINFOF("FloodFill ... Fill x=%d y=%d with fillPattern=%d result=%d", x, y1, (int)fillPattern, (int)buffer(x,y1));
            num++;
            if (!spanLeft && x > 0 && (buffer(x-1,y1) >= patternToFillFrom && buffer(x-1,y1) <= patternToFillTo))
            {
                if (!stack.Push(y1*width+x-1)) return false;
//                URHO3D_LOGINFOF("FloodFill ... Push x=%d y=%d num=%d Left", x-1, y1, num);
                spanLeft = 1;
            }
            else if (spanLeft && x > 0 && !(buffer(x-1,y1) >= patternToFillFrom && buffer(x-1,y1) <= patternToFillTo))
            {
                spanLeft = 0;
            }
            if (!spanRight && x < width - 1 && (buffer(x+1,y1) >= patternToFillFrom && buffer(x+1,y1) <= patternToFillTo))
            {
                if (!stack.Push(y1*width+x+1)) return false;
//                URHO3D_LOGINFOF("FloodFill ... Push x=%d y=%d num=%d Right", x+1, y1, num);
                spanRight = 1;
            }
            else if (spanRight && x < width - 1 && !(buffer(x+1,y1) >= patternToFillFrom && buffer(x+1,y1) <= patternToFillTo))
            {
                spanRight = 0;
            }
            y1++;
        }
    }

//    URHO3D_LOGINFOF("FloodFill ... OK ! num = %d", num);

    return num!=0;
}

// FloodFill Type use
template bool GameHelpers::FloodFill(Matrix2D<unsigned>& buffer, Stack<unsigned>& stack, const unsigned& patternToFill, const unsigned& fillPattern, const int& xo, const int& yo, const IntRect& bordercheck);
template bool GameHelpers::FloodFill(Matrix2D<unsigned char>& buffer, Stack<unsigned>& stack, const unsigned char& patternToFill, const unsigned char& fillPattern, const int& xo, const int& yo, const IntRect& bordercheck);
template bool GameHelpers::FloodFill(Matrix2D<FeatureType>& buffer, Stack<unsigned>& stack, const FeatureType& patternToFillFrom, const FeatureType& patternToFillTo, const FeatureType& fillPattern, const int& xo, const int& yo);


/// FloodFillInBorder : remplit les zones continues avec le motif fillPattern jusqu'à la bordure définie par blockPattern
/// si bordercheck est spécifié alors stop le floodfill si franchissement de cette bordure
template< typename T >
bool GameHelpers::FloodFillInBorder(const Matrix2D<T>& bufferin, Matrix2D<T>& buffer, Stack<unsigned>& stack, const T& borderPattern, const T& fillPattern, const int& xo, const int& yo, const IntRect& bordercheck)
{
//    URHO3D_LOGINFOF("FloodFillInBorder ... x=%d y=%d patternToFill=%d fillPattern=%d", xo, yo, (int)patternToFill, (int)fillPattern);

    int width = bufferin.Width();
    int height = bufferin.Height();
    unsigned maxsize = bordercheck.Width() * bordercheck.Height();

    buffer.CopyValueFrom(borderPattern, bufferin, true);

    int y1, num = 0;
    bool spanLeft, spanRight;
    const bool checkborder = (&bordercheck != &IntRect::ZERO);

//    URHO3D_LOGINFOF("FloodFillInBorder ... Init OK ...");

    stack.Clear();

    int x = xo, y = yo;
    unsigned addr = width*y + x;

    if (!stack.Push(addr))
    {
        URHO3D_LOGERROR("FloodFillToBorder ... Memory Stack Error");
        return false;
    }

//    URHO3D_LOGINFOF("FloodFillInBorder ... Initial fill at x=%d y=%d", x, y);

    while (stack.Pop(addr))
    {
        x = addr % width;
        y = addr / width;
        y1 = y;

        while(y1 >= 0 && buffer(x,y1) != borderPattern)
            y1--;

        y1++;

        spanLeft = spanRight = 0;

        while (y1 < height && buffer(x,y1) != borderPattern)
        {
            if (checkborder && (y1 < bordercheck.top_ || y1 > bordercheck.bottom_ || x > bordercheck.right_ || x < bordercheck.left_))
                return false;

            if (num > maxsize)
            {
                return false;
            }

            buffer(x,y1) = fillPattern;

//            URHO3D_LOGINFOF("FloodFillToBorder ... Fill x=%d y=%d with fillPattern=%d result=%d", x, y1, (int)fillPattern, (int)buffer(x,y1));
            num++;

            if (x > 0)
            {
                if (!spanLeft)
                {
                    if (buffer(x-1,y1) != borderPattern && buffer(x-1,y1) != fillPattern)
                    {
                        if (!stack.Push(y1*width+x-1))
                            return false;

//                        URHO3D_LOGINFOF("FloodFillToBorder ... Push x=%d y=%d num=%d Left", x-1, y1, num);
                        spanLeft = 1;
                    }
                }
                else
                {
                    if (buffer(x-1,y1) == borderPattern || buffer(x-1,y1) == fillPattern)
                    {
                        spanLeft = 0;
                    }
                }
            }

            if (x < width - 1)
            {
                if (!spanRight)
                {
                    if (buffer(x+1,y1) != borderPattern && buffer(x+1,y1) != fillPattern)
                    {
                        if (!stack.Push(y1*width+x+1))
                            return false;

//                        URHO3D_LOGINFOF("FloodFillToBorder ... Push x=%d y=%d num=%d Right", x+1, y1, num);
                        spanRight = 1;
                    }
                }
                else
                {
                    if (buffer(x+1,y1) == borderPattern || buffer(x+1,y1) == fillPattern)
                    {
                        spanRight = 0;
                    }
                }
            }

            y1++;
        }
    }

//    URHO3D_LOGINFOF("FloodFillInBorder ... OK ! num = %d", num);

    return num > 0;
}

// FloodFillInBorder Type use
template bool GameHelpers::FloodFillInBorder(const Matrix2D<unsigned>& bufferin, Matrix2D<unsigned>& buffer, Stack<unsigned>& stack, const unsigned& borderPattern, const unsigned& fillPattern, const int& xo, const int& yo, const IntRect& bordercheck);


/// Check if IntRect Border contains a value and return the border(direction type) if find
template< typename T >
IntVector2 GameHelpers::BorderContains(Matrix2D<T>& buffer, const T& value, IntVector2& point, const IntRect& borderToCheck, int expand)
{
    int width = buffer.Width();
    int height = buffer.Height();
    int xstart, xend, ystart, yend;

    if (borderToCheck != IntRect::ZERO)
    {
        xstart = borderToCheck.left_ - expand;
        xend = borderToCheck.right_ + expand;
        ystart = borderToCheck.top_ - expand;
        yend = borderToCheck.bottom_ + expand;

        if (xstart < 0) xstart = 0;
        if (xend >= width) xend = width-1;
        if (ystart < 0) ystart = 0;
        if (yend >= height) yend = height-1;
    }
    else
    {
        xstart = 0;
        ystart = 0;
        xend = width-1;
        yend = height-1;
    }

    // Check Vertical
    for (int y = ystart+2; y <= yend-2; y++)
    {
        if (buffer(xstart, y) == value)
        {
            point.x_ = xstart;
            point.y_ = y;
            return IntVector2(-1,0);
        }
        if (buffer(xend, y) == value)
        {
            point.x_ = xend;
            point.y_ = y;
            return IntVector2(1,0);
        }
    }

    // Check Horizontal
    for (int x = xstart+2; x <= xend-2; x++)
    {
        if (buffer(x, ystart) == value)
        {
            point.x_ = x;
            point.y_ = ystart;
            return IntVector2(0,-1);
        }
        if (buffer(x, yend) == value)
        {
            point.x_ = x;
            point.y_ = yend;
            return IntVector2(0,1);
        }
    }

    return IntVector2::ZERO;
}

// BorderContains Type use
template IntVector2 GameHelpers::BorderContains(Matrix2D<FeatureType>& buffer, const FeatureType& value, IntVector2& point, const IntRect& borderToCheck, int expand);
template IntVector2 GameHelpers::BorderContains(Matrix2D<unsigned>& buffer, const unsigned& value, IntVector2& point, const IntRect& borderToCheck, int expand);


/// Graphics Helpers

void GameHelpers::ScaleManhattanContour(float value, PODVector<Vector2>& contour)
{
    int numvertices = contour.Size();

    if (numvertices < 3)
        return;

    PODVector<Vector2> offset(numvertices);

    // Point 0
    offset[0].x_ = -value * Sign(contour[1].y_ - contour[numvertices-1].y_);
    offset[0].y_ = value * Sign(contour[1].x_ - contour[numvertices-1].x_);
    // Point 1 to numvertices-2
    for (unsigned i=1; i < numvertices-1; i++)
    {
        offset[i].x_ = -value * Sign(contour[i+1].y_ - contour[i-1].y_);
        offset[i].y_ = value * Sign(contour[i+1].x_ - contour[i-1].x_);
    }
    // Point numface-1
    offset[numvertices-1].x_ = -value * Sign(contour[0].y_ - contour[numvertices-2].y_);
    offset[numvertices-1].y_ = value * Sign(contour[0].x_ - contour[numvertices-2].x_);

    for (unsigned i=0; i < numvertices; i++)
        contour[i] += offset[i];
}

Vector2 GameHelpers::GetManhattanVector(unsigned index, const PODVector<Vector2>& vertices)
{
    if (vertices.Size() < 3)
        return Vector2::ZERO;
    if (index == 0)
        return Vector2(-Sign(vertices[1].y_ - vertices[vertices.Size()-1].y_), Sign(vertices[1].x_ - vertices[vertices.Size()-1].x_));
    if (index == vertices.Size()-1)
        return Vector2(-Sign(vertices[0].y_ - vertices[vertices.Size()-2].y_), Sign(vertices[0].x_ - vertices[vertices.Size()-2].x_));

    return Vector2(-Sign(vertices[index+1].y_ - vertices[index-1].y_), Sign(vertices[index+1].x_ - vertices[index-1].x_));
}

void GameHelpers::OffsetEqualVertices(PODVector<Vector2>& contour, const float epsilon, bool shrink)
{
    const float twoepsilon = 2.f * epsilon * (shrink ? -1.f : 1.f);

    for (int i=0; i < contour.Size()-1; i++)
    {
        for (int j=i+1; j < contour.Size(); j++)
        {
            if (AreEquals(contour[i], contour[j], epsilon))
                contour[i] += GameHelpers::GetManhattanVector(i, contour) * twoepsilon;
        }
    }
}

void GameHelpers::OffsetEqualVertices(const PODVector<Vector2>& contour1, PODVector<Vector2>& contour2, const float epsilon, bool shrink)
{
    const float twoepsilon = 2.f * epsilon * (shrink ? -1.f : 1.f);

    for (int i=0; i < contour1.Size(); ++i)
    {
        for (int j=0; j < contour2.Size(); ++j)
        {
            if (AreEquals(contour1[i], contour2[j], epsilon))
            {
                contour2[j] += GameHelpers::GetManhattanVector(j, contour2) * twoepsilon;
            }
        }
    }
}

void GameHelpers::GetCircleShape(const Vector2& center, float radius, int numpoints, PODVector<Vector2>& shape)
{
    Vector2 vertex;
    shape.Clear();
    shape.Reserve(numpoints);
    for(int i = 0; i < numpoints; ++i)
    {
        const float angle = (float)i / (float)numpoints * 360.0f;
        vertex.x_ = center.x_ + radius * Cos(angle);
        vertex.y_ = center.y_ + radius * Sin(angle);
        shape.Push(vertex);
    }
}

void GameHelpers::GetEllipseShape(const Vector2& center, const Vector2& radius, int numpoints, PODVector<Vector2>& shape)
{
    Vector2 vertex;
    shape.Clear();
    shape.Reserve(numpoints);
    for(int i = 0; i < numpoints; ++i)
    {
        const float angle = (float)i / (float)numpoints * 360.0f;
        vertex.x_ = center.x_ + radius.x_ * Cos(angle);
        vertex.y_ = center.y_ + radius.y_ * Sin(angle);
        shape.Push(vertex);
    }
}

void GameHelpers::TriangulateShape(const PODVector<Vector2>& contour, const Vector<PODVector<Vector2> >& holes, PODVector<Vector2>& vertices, PODVector<unsigned char>& memory)
{
    unsigned numpoints = contour.Size();

    if (numpoints < 3)
        return;

    if (holes.Size())
    {
        for (int i=0; i < holes.Size(); i++)
        {
            if (holes[i].Size() < 3)
                continue;
            numpoints += holes[i].Size();
        }
    }

    size_t memoryRequired = MPE_PolyMemoryRequired(numpoints);
    if (memory.Size() < memoryRequired)
        memory.Resize(memoryRequired);

    memset(memory.Buffer(), 0, memoryRequired);

    MPEPolyContext polygon;

    if (MPE_PolyInitContext(&polygon, memory.Buffer(), numpoints))
    {
//        URHO3D_LOGINFOF("GameHelpers() - TriangulateShape : numcontourpoints=%u numholepoints=%d totalpoints=%u ...", contour.Size(), numpoints-contour.Size(), numpoints);

        // Add Polygon Shape
        {
            MPEPolyPoint* contourShape = MPE_PolyPushPointArray(&polygon, contour.Size());
            for (int i=0; i < contour.Size(); i++)
            {
                contourShape[i].X = contour[i].x_;
                contourShape[i].Y = contour[i].y_;
            }
            MPE_PolyAddEdge(&polygon);
        }

        // Add Holes
        if (holes.Size())
        {
            for (int i=0; i < holes.Size(); i++)
            {
                const PODVector<Vector2>& hole = holes[i];
                if (hole.Size() >= 3)
                {
                    MPEPolyPoint* holeShape = MPE_PolyPushPointArray(&polygon, hole.Size());
                    for (int j=0; j < hole.Size(); j++)
                    {
                        holeShape[j].X = hole[j].x_;
                        holeShape[j].Y = hole[j].y_;
                    }
                    MPE_PolyAddHole(&polygon);
                }
            }
        }

        // Triangulate the shape
        MPE_PolyTriangulate(&polygon);

        unsigned startvertice = vertices.Size();
        vertices.Resize(startvertice + polygon.TriangleCount*3);

        // Transfert Triangles list
        for (uxx i = 0; i < polygon.TriangleCount; ++i)
        {
            MPEPolyTriangle* triangle = polygon.Triangles[i];
            for (int j=0; j < 3; ++j)
            {
                float x = triangle->Points[j]->X;
                float y = triangle->Points[j]->Y;
                vertices[startvertice + 3*i+j].x_ = x;
                vertices[startvertice + 3*i+j].y_ = y;
            }
        }

//        URHO3D_LOGINFOF("GameHelpers() - TriangulateShape : numtriangles=%u", polygon.TriangleCount);
    }
}

void GameHelpers::DrawDebugVertices(DebugRenderer* debug, const PODVector<Vector2>& vertices, const Color& color, const Matrix3x4& worldTransform)
{
    for (unsigned i=0; i < vertices.Size(); i++)
        debug->AddCross(Vector3(worldTransform * vertices[i]), 0.2f, color, false);
}

void GameHelpers::DrawDebugShape(DebugRenderer* debug, const PODVector<Vector2>& shape, const Color& color, const Matrix3x4& worldTransform)
{
    if (!shape.Size())
        return;

    unsigned count = 0;
    unsigned uintColor = color.ToUInt();

    while (++count < shape.Size())
        debug->AddLine(worldTransform * Vector3(shape[count-1]), worldTransform * Vector3(shape[count]), uintColor, false);

    debug->AddLine(worldTransform * Vector3(shape.Back()), worldTransform * Vector3(shape.Front()), uintColor, false);
    debug->AddCircle(worldTransform * Vector3(shape[0]), Vector3::FORWARD, 0.1f, color, 8, false, false);

    debug->AddCross(worldTransform * Vector3(shape[shape.Size() > 1 ? 1 : 0]), 0.2f, color, false);
}

void GameHelpers::DrawDebugMesh(DebugRenderer* debug, const PODVector<Vector2>& vertices, const Color& color, const Matrix3x4& worldTransform)
{
    if (!vertices.Size())
        return;

    unsigned count = 0;
    unsigned size = vertices.Size();
    unsigned uintColor = color.ToUInt();

    while (count < size)
    {
        Vector3 v0 = worldTransform * Vector3(vertices[count]);
        Vector3 v1 = worldTransform * Vector3(vertices[count+1]);
        Vector3 v2 = worldTransform * Vector3(vertices[count+2]);

        debug->AddLine(v0, v1, uintColor, false);
        debug->AddLine(v1, v2, uintColor, false);
        debug->AddLine(v2, v0, uintColor, false);
        count += 3;
    }
}

void GameHelpers::DrawDebugMesh(DebugRenderer* debug, const Vector<Vertex2D>& vertices, const Color& color, bool quad)
{
    if (!vertices.Size())
        return;

    unsigned count = 0;
    unsigned size = vertices.Size();
    unsigned uintColor = color.ToUInt();

    if (!quad)
        while (count < size)
        {
            Vector3 v0 = vertices[count].position_;
            Vector3 v1 = vertices[count+1].position_;
            Vector3 v2 = vertices[count+2].position_;
            v0.z_ = v1.z_ = v2.z_= 0.f;

            debug->AddLine(v0, v1, uintColor, false);
            debug->AddLine(v1, v2, uintColor, false);
            debug->AddLine(v2, v0, uintColor, false);
            count += 3;
        }
    else
        while (count < size)
        {
            Vector3 v0 = vertices[count].position_;
            Vector3 v1 = vertices[count+1].position_;
            Vector3 v2 = vertices[count+2].position_;
            Vector3 v3 = vertices[count+3].position_;
            v0.z_ = v1.z_ = v2.z_= v3.z_= 0.f;

            debug->AddLine(v0, v1, uintColor, false);
            debug->AddLine(v1, v2, uintColor, false);
            debug->AddLine(v2, v3, uintColor, false);
            debug->AddLine(v3, v0, uintColor, false);
            count += 4;
        }
}


/// UI Helpers

void GameHelpers::ApplySizeRatio(int w, int h, IntVector2& size)
{
    float aratio = (float)w / h;
    if (size.x_ < size.y_)
        size.x_ = size.y_ * aratio;
    else
        size.y_ = size.x_ / aratio;
}

UIElement* GameHelpers::AddUIElement(const String& layout, const IntVector2& position, HorizontalAlignment hAlign, VerticalAlignment vAlign, float alpha, UIElement* root)
{
    UIElement* elt = 0;

//    URHO3D_LOGINFOF("GameHelpers() - AddUIElement : %s ...", layout.CString());

    UI* ui = GameContext::Get().context_->GetSubsystem<UI>();
    SharedPtr<UIElement> uielement = ui->LoadLayout(GameContext::Get().context_->GetSubsystem<ResourceCache>()->GetResource<XMLFile>(layout));
    elt = uielement.Get();

    if (!elt)
    {
        URHO3D_LOGERRORF("GameHelpers() - AddUIElement : Can't Load Layout=%s !", layout.CString());
        return 0;
    }

    if (!root)
        root = ui->GetRoot();

    root->AddChild(elt);
    elt->SetHorizontalAlignment(hAlign);
    elt->SetVerticalAlignment(vAlign);
    elt->SetPosition(position);

    if (alpha)
        elt->SetOpacity(alpha);

    elt->SetVisible(false);

    return elt;
}

void GameHelpers::SetMoveAnimationUI(UIElement* elt, const IntVector2& from, const IntVector2& to, float start, float delay)
{
    if (!elt)
        return;

    elt->SetPosition(from);

    SharedPtr<ObjectAnimation> objectAnimation(new ObjectAnimation(GameContext::Get().context_));
    SharedPtr<ValueAnimation> positionAnimation(new ValueAnimation(GameContext::Get().context_));
    positionAnimation->SetKeyFrame(start, from);
    positionAnimation->SetKeyFrame(start+delay, to);
    objectAnimation->AddAttributeAnimation("Position", positionAnimation, WM_ONCE);

    elt->SetObjectAnimation(objectAnimation);

    elt->SetVisible(true);
}

void GameHelpers::SetUIScale(UIElement* element, const IntVector2& defaultsize, float scale)
{
    if (scale == 0.f)
        scale = GameContext::Get().uiScale_;

    const IntVector2& currentsize = element->GetSize();
    IntVector2 newsize((int)(scale * (float)defaultsize.x_), (int)(scale * (float)defaultsize.y_));
    float currentscale = (float)currentsize.x_ / (float)defaultsize.x_;
//
//    URHO3D_LOGINFOF("GameHelpers() - SetUIScale : element=%s defaultsize=%s currentsize=%s newsize=%s (currentscale=%f) newscale=%f",
//                    element->GetName().CString(), defaultsize.ToString().CString(), currentsize.ToString().CString(),
//                    newsize.ToString().CString(), currentscale, scale);
//
//    if (newsize != currentsize)
//        SetScaleChildRecursive(element, scale / currentscale);
//
//    UpdateScrollBars(element);

    if (newsize != currentsize)
    {
        element->SetPosition(floor((float)element->GetPosition().x_ * scale), floor((float)element->GetPosition().y_ * scale));
    }
}

void GameHelpers::SetScaleChildRecursive(UIElement* elt, float scale)
{
    elt->SetPosition(floor((float)elt->GetPosition().x_ * scale), floor((float)elt->GetPosition().y_ * scale));

    const Vector<SharedPtr<UIElement> >& children = elt->GetChildren();
    for (Vector<SharedPtr<UIElement> >::ConstIterator it=children.Begin(); it != children.End(); ++it)
        SetScaleChildRecursive(it->Get(), scale);

    if (elt->IsInstanceOf<Text>())
    {
        Text* text = static_cast<Text*>(elt);
        Font* font = text->GetFont();

        if (font->GetFontType() == FONT_BITMAP)
        {
            int fontsize = (int)((float)text->GetFontSize()*scale);
            text->SetFont(GameContext::Get().uiFonts_[fontsize <= 12 ? UIFONTS_ABY12 : fontsize >= 32 ? UIFONTS_ABY32 : UIFONTS_ABY22]);
            text->SetFontSize(Clamp(fontsize, 22, 96));

//            URHO3D_LOGINFOF("GameHelpers() - SetScaleChildRecursive : ptr=%u parent=%u name=%s type=%s scale=%f fontsize=%d size=%s",
//                            text, text->GetParent(), text->GetName().CString(), text->GetTypeName().CString(), scale,
//                            text->GetFontSize(), text->GetSize().ToString().CString());
        }
    }
    else if (elt->IsInstanceOf<DropDownList>())
    {
        int fontsize;
        DropDownList* control = static_cast<DropDownList*>(elt);
        PODVector<UIElement*> items = control->GetItems();
        for (PODVector<UIElement*>::ConstIterator it = items.Begin(); it != items.End(); ++it)
        {
            if ((*it)->IsInstanceOf<Text>())
            {
                Text* text = static_cast<Text*>(*it);
                Font* font = text->GetFont();

                if (font->GetFontType() == FONT_BITMAP)
                {
                    fontsize = (int)((float)text->GetFontSize()*scale);
                    text->SetFont(GameContext::Get().uiFonts_[fontsize <= 12 ? UIFONTS_ABY12 : fontsize >= 32 ? UIFONTS_ABY32 : UIFONTS_ABY22]);
                    text->SetFontSize(Clamp(fontsize, 22, 96));

//                    URHO3D_LOGINFOF("GameHelpers() - SetScaleChildRecursive : ptr=%u parent=%u name=%s type=%s scale=%f fontsize=%d size=%s",
//                                    text, text->GetParent(), text->GetText().CString(), text->GetTypeName().CString(), scale,
//                                    text->GetFontSize(), text->GetSize().ToString().CString());
                }
            }
        }

        fontsize = (fontsize <= 12 ? 12 : fontsize >= 32 ? 32 : 22) * Max(1.f, GameContext::Get().uiDpiScale_);

        control->GetPopup()->SetSize(control->GetPopup()->GetWidth(), Min(items.Size()+1, 10) * (fontsize + 2));
        elt->SetMinSize((float)elt->GetMinSize().x_ * scale, fontsize + 8);
        elt->SetMaxSize((float)elt->GetMaxSize().x_ * scale, fontsize + 8);

        elt->SetVerticalAlignment(VA_CENTER);
        elt->SetLayoutMode(LM_VERTICAL);
    }
    else
    {
        if (!elt->HasTag("NoScaleX"))
        {
            if (elt->GetMinWidth() != 0)
                elt->SetMinWidth((float)elt->GetMinWidth() * scale);
            if (elt->GetMaxWidth() != M_MAX_INT)
                elt->SetMaxWidth((float)elt->GetMaxWidth() * scale);

            elt->SetWidth((float)elt->GetWidth() * scale);
        }
        if (!elt->HasTag("NoScaleY"))
        {
            if (elt->GetMinHeight() != 0)
                elt->SetMinHeight((float)elt->GetMinHeight() * scale);
            if (elt->GetMaxHeight() != M_MAX_INT)
                elt->SetMaxHeight((float)elt->GetMaxHeight() * scale);

            elt->SetHeight((float)elt->GetHeight() * scale);
        }
    }

//    URHO3D_LOGINFOF("GameHelpers() - SetScaleChildRecursive : ptr=%u parent=%u name=%s type=%s size=%s(min=%s,max=%s)",
//                        elt, elt->GetParent(), elt->GetName().CString(), elt->GetTypeName().CString(),
//                        elt->GetSize().ToString().CString(), elt->GetMinSize().ToString().CString(), elt->GetMaxSize().ToString().CString());

    elt->UpdateLayout();
}

void GameHelpers::UpdateScrollBars(UIElement* elt)
{
    // TODO : see Urho3D for the update method of the ScrollBar
    const Vector<SharedPtr<UIElement> >& children = elt->GetChildren();
    for (Vector<SharedPtr<UIElement> >::ConstIterator it=children.Begin(); it != children.End(); ++it)
    {
        UIElement* child = it->Get();
        if (child->IsInstanceOf<ScrollBar>())
            child->UpdateLayout();
        else
            UpdateScrollBars(child);
    }
}

extern const char * spriteSheetFiles_[];

template< typename T >
void GameHelpers::SetUIElementFrom(T* uielement, int uitextureid, const String& name, int clampsize)
{
    Sprite2D* sprite2d;

    if (uitextureid == -1)
    {
        ResourceRef ref(SpriteSheet2D::GetTypeStatic(), name);
        sprite2d = Sprite2D::LoadFromResourceRef(uielement->GetContext(), ref);
    }
    else
    {
        sprite2d = GameContext::Get().spriteSheets_[uitextureid]->GetSprite(name);
    }

    if (!sprite2d)
    {
        URHO3D_LOGERRORF("GameHelpers() - SetUIElementFrom : name=%s no Sprite2D", name.CString());
        return;
    }

    uielement->SetTexture(uitextureid == -1 ? sprite2d->GetTexture() : GameContext::Get().textures_[uitextureid]);
//    uielement->SetEnableDpiScale(false);

    uielement->SetImageRect(sprite2d->GetRectangle());

    IntVector2 size = sprite2d->GetRectangle().Size();

    if (clampsize)
        ClampSizeTo(size, clampsize);

    uielement->SetSize(size);

    URHO3D_LOGERRORF("GameHelpers() - SetUIElementFrom : elt=%s sprite=%s(%s) texture=%s", uielement->GetName().CString(), name.CString(), String((void*)sprite2d).CString(), String((void*)sprite2d->GetTexture()).CString());
}

template< typename T >
void GameHelpers::SetUIElementFrom(T* uielement, int uitextureid, const ResourceRef& ref, int clampsize)
{
    if (ref.name_.Empty())
    {
        URHO3D_LOGERRORF("GameHelpers() - SetUIElementFrom : no ref=%s !");
        return;
    }

    Sprite2D* spriteui = 0;
    bool useSpriteFrom2D = false;

    if (uitextureid >= 0)
    {
        Vector<String> names = ref.name_.Split('@');
        spriteui = GameContext::Get().spriteSheets_[uitextureid]->GetSprite(names[names.Size()-1]);
    }

    if (!spriteui)
    {
        useSpriteFrom2D = true;
        spriteui = Sprite2D::LoadFromResourceRef(uielement->GetContext(), ref);
    }

    if (!spriteui)
    {
        URHO3D_LOGERRORF("GameHelpers() - SetUIElementFrom : ref=%s no spriteui => use Marker", ref.ToString().CString());
        uitextureid = UIEQUIPMENT;
        spriteui = GameContext::Get().spriteSheets_[uitextureid]->GetSprite("markcheck2");
    }

    uielement->SetTexture(useSpriteFrom2D ? spriteui->GetTexture() : GameContext::Get().textures_[uitextureid]);
    uielement->SetImageRect(spriteui->GetRectangle());

    IntVector2 size = spriteui->GetRectangle().Size();
    if (clampsize)
        ClampSizeTo(size, clampsize);
    uielement->SetSize(size);

    URHO3D_LOGERRORF("GameHelpers() - SetUIElementFrom : elt=%s sprite=%s(%s) texture=%s", uielement->GetName().CString(), ref.name_.CString(), String((void*)spriteui).CString(), String((void*)spriteui->GetTexture()).CString());
}

template void GameHelpers::SetUIElementFrom(BorderImage* uielement, int uitextureid, const String& spritename, int clampsize);
template void GameHelpers::SetUIElementFrom(Sprite* uielement, int uitextureid, const String& spritename, int clampsize);
template void GameHelpers::SetUIElementFrom(Button* uielement, int uitextureid, const String& spritename, int clampsize);
template void GameHelpers::SetUIElementFrom(Window* uielement, int uitextureid, const String& spritename, int clampsize);
template void GameHelpers::SetUIElementFrom(BorderImage* uielement, int uitextureid, const ResourceRef& ref, int clampsize);
template void GameHelpers::SetUIElementFrom(Sprite* uielement, int uitextureid, const ResourceRef& ref, int clampsize);
template void GameHelpers::SetUIElementFrom(Button* uielement, int uitextureid, const ResourceRef& ref, int clampsize);
template void GameHelpers::SetUIElementFrom(Window* uielement, int uitextureid, const ResourceRef& ref, int clampsize);

void GameHelpers::FindParentWithAttribute(const StringHash& attribute, UIElement* element, UIElement*& parent, Variant& var)
{
    bool ok = false;
    parent = element;
//    URHO3D_LOGINFOF("element name=%s ok=%u", element->GetName().CString(), ok);
    while (!ok)
    {
        if (parent)
        {
            var = parent->GetVar(attribute);
            ok = (var == Variant::EMPTY ? false : true);
//            URHO3D_LOGINFOF("parent name=%s ok=%u", parent->GetName().CString(), ok);
            if (!ok)
                parent = parent->GetParent();
        }
        else
            ok = true;
    }
//    URHO3D_LOGINFOF("FindParentWithAttribute ok=%u", ok);
    //if (parent == GetSubsystem<UI>()->GetRoot() || !parent) return 0;
}

void GameHelpers::FindParentWhoContainsName(const String& name, UIElement* element, UIElement*& parent)
{
    bool ok = false;
    parent = element;
//    URHO3D_LOGINFOF("element name=%s ok=%u", element->GetName().CString(), ok);
    while (!ok)
    {
        if (parent)
        {
            ok = (parent->GetName().Contains(name) ? true : false);
//            URHO3D_LOGINFOF("parent name=%s ok=%u", parent->GetName().CString(), ok);
            if (!ok)
                parent = parent->GetParent();
        }
        else
            ok = true;
    }
//    URHO3D_LOGINFOF("FindParentWhoContainsName ok=%u", ok);
    //if (parent == GetSubsystem<UI>()->GetRoot() || !parent) return 0;
}

void GameHelpers::ClampPositionToScreen(IntVector2& position)
{
    Graphics* graphics = GameContext::Get().context_->GetSubsystem<Graphics>();
    const int width = graphics->GetWidth();
    const int height = graphics->GetHeight();

    if (position.x_ < 0)
        position.x_ = 0;
    else if (position.x_ > width)
        position.x_ = width/2;
    if (position.y_ < 0)
        position.y_ = 0;
    else if (position.y_ > height)
        position.y_ = height/2;
}

void GameHelpers::ClampPositionUIElementToScreen(UIElement* element)
{
    if (!element)
        return;

    Graphics* graphics = GameContext::Get().context_->GetSubsystem<Graphics>();
    const int width = graphics->GetWidth();
    const int height = graphics->GetHeight();

    IntRect rect = element->GetCombinedScreenRect();
    IntVector2 position = element->GetPosition();
    if (rect.left_ < 0)
        position.x_ -= rect.left_;
    else if (rect.right_ > width)
        position.x_ -= (rect.right_ - width);
    if (rect.top_ < 0)
        position.y_ -= rect.top_;
    else if (rect.bottom_ > height)
        position.y_ -= (rect.bottom_ - height);
    element->SetPosition(position);
}

IntVector2 GameHelpers::GetUIExitPoint(UIElement* element, const IntVector2& currentposition)
{
    if (!element)
        return IntVector2::ZERO;

    IntVector2 exitpoint;
    IntRect rect = element->GetCombinedScreenRect();
    IntVector2 center = rect.Center();
    int dX = center.x_ < GameContext::Get().screenwidth_ / 2 ? rect.left_ : GameContext::Get().screenwidth_ - rect.right_;
    int dY = center.y_ < GameContext::Get().screenheight_ / 2 ? rect.top_ : GameContext::Get().screenheight_ -  rect.bottom_;

    if (dX < dY)
    {
        exitpoint.x_ = currentposition.x_ < GameContext::Get().screenwidth_ / 2 ? -element->GetSize().x_ : GameContext::Get().screenwidth_;
        exitpoint.y_ = currentposition.y_;
    }
    else
    {
        exitpoint.x_ = currentposition.x_;
        exitpoint.y_ = currentposition.y_ < GameContext::Get().screenheight_ / 2 ? -element->GetSize().y_ : GameContext::Get().screenheight_;
    }

    return exitpoint;
}

WeakPtr<Window> GameHelpers::backupModal_;

void GameHelpers::ToggleModalWindow(Object* owner, UIElement* elt, bool force)
{
    UI* ui = GameContext::Get().ui_;

//    URHO3D_LOGINFOF("GameHelpers() - ToggleModalWindow elt=%u ... ", elt);

    // Add elt as Modal Window
    if (elt)
    {
        if (elt->GetType() != Window::GetTypeStatic())
            return;

//        URHO3D_LOGINFOF("GameHelpers() - ToggleModalWindow = %s ... ", elt->GetName().CString());

        if (ui->HasModalElement())
        {
            // Backup current Modal if it's not a messagebox or if not the same elt
            Window* backup = static_cast<Window*>(ui->GetRootModalElement()->GetChild(0));
            if (backup != elt && backup && backup->GetName() != "MessageBox")
            {
                backup->SetModal(false);
                backupModal_ = backup;
            }
        }

        Window* window = static_cast<Window*>(elt);
        if (!window->IsModal() && window->GetName() != "MessageBox")
            window->SetModal(true);

        window->SetFocus(true);

//        URHO3D_LOGINFOF("GameHelpers() - ToggleModalWindow = %s ... OK !", elt->GetName().CString());
    }
    // Restore Last Modal (Backup)
    else if (backupModal_)
    {
//        URHO3D_LOGINFOF("GameHelpers() - ToggleModalWindow BackupModal = %s ...", backupModal_->GetName().CString());

        if (!backupModal_->IsModal())
        {
            backupModal_->SetModal(true);
            backupModal_->SetFocus(true);
        }

//        URHO3D_LOGINFOF("GameHelpers() - ToggleModalWindow BackupModal = %s ... OK !", backupModal_->GetName().CString());

        backupModal_.Reset();
    }
    // Remove the modal window
    else if (ui->HasModalElement())
    {
        Window* window = static_cast<Window*>(ui->GetRootModalElement()->GetChild(0));

        URHO3D_LOGINFOF("GameHelpers() - ToggleModalWindow Reset Modal ... window=%u", window);

        if (force && owner)
            owner->UnsubscribeFromEvent(window, E_MODALCHANGED);

        if (window->GetName() != "MessageBox" || force)
        {
            window->SetModal(false);
            window->SetFocus(false);
            // 20200516 - Important : to not be grab to an element at start (ensure a reactive ui)
            GameContext::Get().context_->GetSubsystem<Input>()->SetMouseGrabbed(false, true);
        }

        URHO3D_LOGINFOF("GameHelpers() - ToggleModalWindow Reset Modal ... OK !");
    }

//    URHO3D_LOGINFOF("GameHelpers() - ToggleModalWindow elt=%u ... OK !", elt);
}

Window* GameHelpers::GetModalWindow()
{
    UIElement* elt =  GameContext::Get().ui_->HasModalElement() ?  GameContext::Get().ui_->GetRootModalElement()->GetChild(0) : 0;
    if (elt && elt->GetType() == Window::GetTypeStatic())
        return static_cast<Window*>(GameContext::Get().ui_->GetRootModalElement()->GetChild(0));

    return 0;
}

void GameHelpers::SetEnableScissor(UIElement* elt, bool enable)
{
    elt->SetClipChildren(enable);
    const Vector<SharedPtr<UIElement> >& elements = elt->GetChildren();
    for (unsigned i=0; i < elements.Size(); ++i)
        SetEnableScissor(elements[i], enable);
}

TextMessage* GameHelpers::ShowUIMessage(const String& maintext, const String& endtext, bool localize, int fontsize, const IntVector2& position, float fadescale, float duration, float delaystart)
{
//    void Set(const String& message, const char *fontName, int fontSize, float duration, IntVector2 position=IntVector2::ZERO, float fadescale=1.f,
//            bool autoRemove=true, float delayedStart=0.f, float delayedRemove=0.f);

    TextMessage* message = new TextMessage(GameContext::Get().context_);
    message->Set(localize ? GameContext::Get().context_->GetSubsystem<Localization>()->Get(maintext) + endtext : maintext+endtext, GameContext::Get().txtMsgFont_,
                 fontsize * GameContext::Get().uiScale_, duration, position, fadescale, true, delaystart);
    return message;
}

TextMessage* GameHelpers::ShowUIMessage3D(const String& maintext, const String& endtext, bool localize, int fontsize, const IntVector2& position, float fadescale, float duration, float delaystart)
{
//    void Set(Node* node, const String& message, const char *fontName, int fontSize,
//             float duration, float fadescale=1.f, bool follow=true, bool autoRemove=true, bool loop=false, float delayedStart=0.f, float delayedRemove=0.f);
    TextMessage* message = new TextMessage(GameContext::Get().context_);
    message->Set(ViewManager::Get()->GetCameraNode(), localize ? GameContext::Get().context_->GetSubsystem<Localization>()->Get(maintext) + endtext : maintext+endtext, GameContext::Get().txtMsgFont_,
                 fontsize * GameContext::Get().uiScale_, duration, fadescale, true, true, delaystart);
    return message;
}

void GameHelpers::ParseText(const String& text, Vector<String>& lines, int nummaxchar)
{
//    lines = text.Split('|');

    Vector<String> templines;
    templines = text.Split('|');

    lines.Clear();

    Vector<String> words;
    String currline;
    for (Vector<String>::Iterator it = templines.Begin(); it != templines.End(); ++it)
    {
        words = it->Split(' ');

        for (Vector<String>::Iterator jt = words.Begin(); jt != words.End(); ++jt)
        {
            String& word = *jt;
            if (currline.Length() + word.Length() < nummaxchar)
            {
                if (!currline.Empty())
                    currline += " ";
                currline += word;
            }
            else
            {
                lines.Push(currline);
                currline = word;
            }
        }

        if (!currline.Empty())
        {
            lines.Push(currline);
            currline.Clear();
        }
    }
}


/// Dump Helpers

static String dumpDeepth;
static unsigned dumpComponentIndex;
static unsigned dumpNodeIndex;

void GameHelpers::DumpText(const char* buffer)
{
    if (buffer) std::cout << buffer << std::endl;
}

void GameHelpers::DumpMap(int width, int height, const int* data)
{
    unsigned Size = width * height;
    char blockChar;
    String str;
    str.Reserve(Size + 25);
    str += "GameHelpers() - DumpMap\n";
    for (unsigned i = 0; i < Size; i++)
    {
        if (i > 0 && i % width == 0) str += "\n";

        if (data[i] == 0)
            blockChar = '.';
        else
            blockChar = '*';

        str += blockChar;
        //str += " ";
    }
    URHO3D_LOGINFOF("%s",str.CString());
}

void GameHelpers::DumpObject(Object* ref)
{
    if (ref)
    {
        URHO3D_LOGINFOF("GameHelpers() - DumpObject : ptr=%u numRefs=%d(w=%d) objectType=%s", ref, ref->Refs(), ref->WeakRefs(),ref->GetTypeName().CString());
    }
    else
        URHO3D_LOGINFO("GameHelpers() - DumpObject : ptr=0");
}

void GameHelpers::DumpNodeRecursive(Node* node, unsigned currentDeepth, bool withcomponent, int deepth)
{
    if (!node)
        return;

    dumpDeepth = "";
    {
        int i = 0;
        while (i++ < currentDeepth) dumpDeepth += "=";
    }

    if (withcomponent)
    {
        for (Vector<SharedPtr<Component> >::ConstIterator it=node->GetComponents().Begin(); it!=node->GetComponents().End(); ++it, ++dumpComponentIndex)
            URHO3D_LOGINFOF(" %s=> [c%u] = %s(%u) enabled=%s", dumpDeepth.CString(), dumpComponentIndex, (*it)->GetTypeName().CString(), (*it)->GetID(), (*it)->IsEnabled() ? "true" : "false");
    }

    for (Vector<SharedPtr<Node> >::ConstIterator it=node->GetChildren().Begin(); it!=node->GetChildren().End(); ++it)
    {
        ++dumpNodeIndex;

        URHO3D_LOGINFOF(" %s=> [n%u] = %s(%u) enabled=%s", dumpDeepth.CString(), dumpNodeIndex, (*it)->GetName().CString(), (*it)->GetID(), (*it)->IsEnabled() ? "true" : "false");
        if (deepth)
            GameHelpers::DumpNodeRecursive(*it, currentDeepth+1, withcomponent, deepth-1);
    }
}

void GameHelpers::DumpNode(Node* node, int maxrecursdeepth, bool withcomponent)
{
    if (!node)
        return;

    // force log
//    GameHelpers::SetGameLogLevel(0);

    URHO3D_LOGINFOF("GameHelpers() - DumpNode : name=%s id=%u numref=%d(w=%d) numchild=%u numcomponents=%u",
                    node->GetName().CString(), node->GetID(), node->Refs(), node->WeakRefs(), node->GetNumChildren(), node->GetNumComponents());

    unsigned currentDeepth = 0;
    dumpComponentIndex = 0;
    dumpNodeIndex = 0;

    if (withcomponent)
    {
        for (Vector<SharedPtr<Component> >::ConstIterator it=node->GetComponents().Begin(); it!=node->GetComponents().End(); ++it, ++dumpComponentIndex)
            URHO3D_LOGINFOF(" => [c%u] = %s(%u) enabled=%s", dumpComponentIndex, (*it)->GetTypeName().CString(), (*it)->GetID(), (*it)->IsEnabled() ? "true" : "false");
    }

    for (Vector<SharedPtr<Node> >::ConstIterator it=node->GetChildren().Begin(); it!=node->GetChildren().End(); ++it)
    {
        if (*it == 0)
        {
            URHO3D_LOGWARNINGF("GameHelpers() - DumpNode : NodeIndex=n%u Empty !", dumpNodeIndex);
            continue;
        }

        ++dumpNodeIndex;

        String name = (*it)->GetName();
        URHO3D_LOGINFOF(" => [n%u] = %s(%u) enable=%s", dumpNodeIndex, name.CString(), (*it)->GetID(), (*it)->IsEnabled() ? "true" : "false");

        // skip preloader and pool
        if (name == "PreLoad" || name == "PreLoadGOT" || name == "PrepareNodes")// || name == "StaticScene")
            continue;

        if (maxrecursdeepth)
            GameHelpers::DumpNodeRecursive(*it, currentDeepth+1, withcomponent, maxrecursdeepth-1);
    }

    if (dumpNodeIndex+dumpComponentIndex > 0)
        URHO3D_LOGINFOF(" ------------ ");
}

void GameHelpers::DumpNode(unsigned id, bool withcomponentattr, bool rtt)
{
    Node* node = rtt ? GameContext::Get().rttScene_->GetNode(id) : GameContext::Get().rootScene_->GetNode(id);
    if (!node)
    {
        URHO3D_LOGINFOF("GameHelpers() - DumpNode : No Node id=%u", id);
        return;
    }
    URHO3D_LOGINFOF("GameHelpers() - DumpNode : ---------------");
    URHO3D_LOGINFOF("GameHelpers() - DumpNode : %s(%u) ...", node->GetName().CString(), node->GetID());
    // Dump Variables
    {
        const VariantMap& variables = node->GetVars();
        Context* context = node->GetContext();
        for (VariantMap::ConstIterator it=variables.Begin(); it!=variables.End(); ++it)
        {
            const String& attrname = context->GetUserAttributeName(it->first_);
            URHO3D_LOGINFOF(" => Var %s(%u) type=%s value=%s", attrname.CString(), it->first_.Value(), it->second_.GetTypeName().CString(), it->second_.ToString().CString());
        }
    }
    if (!withcomponentattr)
    {
        WorldMapPosition mapPosition = node->GetComponent<GOC_Destroyer>() ? node->GetComponent<GOC_Destroyer>()->GetWorldMapPosition() : WorldMapPosition();

        URHO3D_LOGINFOF(" => Main States : enabled=%s position=%s (posdefined=%s map=%s zview=%d) Parent %s(%u)", node->IsEnabled() ? "true" : "false",
                        node->GetWorldPosition().ToString().CString(), mapPosition.defined_ ? "true" : "false", mapPosition.mPoint_.ToString().CString(), mapPosition.viewZ_,
                        node->GetParent() ? node->GetParent()->GetName().CString() : "NULL", node->GetParent() ? node->GetParent()->GetID() : 0);

        const Vector<SharedPtr<Component> >& components = node->GetComponents();
        for (unsigned i=0; i < components.Size(); i++)
        {
            URHO3D_LOGINFOF(" => Component %s(%u) enabled=%s", components[i]->GetTypeName().CString(), components[i]->GetID(), components[i]->IsEnabled() ? "true" : "false");
            if (components[i]->IsInstanceOf<Drawable2D>())
            {
                Drawable2D* drawable = (Drawable2D*) components[i].Get();
                URHO3D_LOGINFOF("  => Drawable2D BoundingBox World=%s Local=%s", drawable->GetWorldBoundingBox().ToString().CString(), drawable->GetBoundingBox().ToString().CString());
            }
        }
        const Vector<SharedPtr<Node> >& children = node ->GetChildren();
        for (unsigned i=0; i < children.Size(); i++)
            URHO3D_LOGINFOF(" => Child %s(%u)", children[i]->GetName().CString(), children[i]->GetID());
    }
    else
    {
        URHO3D_LOGINFOF(" => Main States : enabled=%s Parent %s(%u)", node->IsEnabled() ? "true" : "false",
                        node->GetParent() ? node->GetParent()->GetName().CString() : "NULL", node->GetParent() ? node->GetParent()->GetID() : 0);

        const Vector<SharedPtr<Component> >& components = node->GetComponents();

        for (unsigned i=0; i < components.Size(); i++)
        {
            URHO3D_LOGINFOF(" => Component %s(%u)", components[i]->GetTypeName().CString(), components[i]->GetID());
            Component* component = components[i];
            if (component)
            {
                const Vector<AttributeInfo>* attrptr = component->GetAttributes();
                if (attrptr)
                {
                    const Vector<AttributeInfo>& attributes = *attrptr;
                    for (unsigned i=0; i < attributes.Size(); i++)
                    {
                        Variant variant;
                        component->OnGetAttribute(attributes[i], variant);
                        URHO3D_LOGINFOF("   => Attribute : %s value(%s)=%s", attributes[i].name_.CString(), variant.GetTypeName().CString(), variant.ToString().CString());
                    }
                }
                if (component->IsInstanceOf<Drawable2D>())
                {
                    Drawable2D* drawable = (Drawable2D*) component;
                    URHO3D_LOGINFOF("  => Drawable2D BoundingBox World=%s Local=%s", drawable->GetWorldBoundingBox().ToString().CString(), drawable->GetBoundingBox().ToString().CString());
                }
            }
        }
        const Vector<SharedPtr<Node> >& children = node ->GetChildren();
        for (unsigned i=0; i < children.Size(); i++)
            URHO3D_LOGINFOF(" => Child %s(%u)", children[i]->GetName().CString(), children[i]->GetID());
    }
    URHO3D_LOGINFO("GameHelpers() - DumpNode : ---------------");
}

void GameHelpers::DumpVariantMap(const VariantMap& varmap)
{
//    if (!varmap.Size())
//        return;

    String str = ToString("GameHelpers() - DumpVariantMap : VariantMap Size = %u :\n", varmap.Size());

    for (VariantMap::ConstIterator it=varmap.Begin(); it!= varmap.End(); ++it)
    {
        StringHash hash = it->first_;
        const Variant& variant = it->second_;
        str += ToString(" hash=%u variant(type=%s;value=%s) \n", hash.Value(), variant.GetTypeName().CString(), variant.ToString().CString());
    }

    URHO3D_LOGINFOF(" %s", str.CString());
}

void GameHelpers::AppendBufferToString(String& string, const char* format, ...)
{
    static char buffer[256];

    va_list args;
    va_start (args, format);
    unsigned length = vsprintf(buffer, format, args);
    va_end (args);

    string.Append(buffer, length);
}


/// DumpData
template< typename T >
void GameHelpers::DumpData(const T* buffer, int startpattern, int numargs, ...)
{
//    T mk = (T) startpattern;

    const unsigned char* data = (const unsigned char*) buffer;
    unsigned sizeT = sizeof(T);

    if (numargs<2) return ;

    va_list args;
    va_start(args, numargs);

    bool highlight = numargs == 3;
    unsigned width = va_arg(args, unsigned);
    unsigned height = va_arg(args, unsigned);
    unsigned Size = width * height;
    unsigned pattern = startpattern;

//    URHO3D_LOGINFOF("DumpData : ");

    String str;
    str.Reserve(Size);

    // Pattern Mode : Get args (for MapColliderGenerator Marking)
    if (startpattern > 0)
    {
        unsigned neighborPoint = 0;
        unsigned contourStartPoint = 0;
        unsigned backtrackStartPoint = 0;
        unsigned boundaryPoint = 0;
        unsigned backtrackPoint = 0;

        if (numargs > 2)
        {
            pattern = va_arg(args, unsigned);

            if (!highlight)
            {
                str += "\n";
                char c = 'A' + (pattern - startpattern);
                str.AppendWithFormat("- BlockID = %c(c=%u) :\n", c, c);
            }
            else
            {
                if (pattern == startpattern)
                    str += "- Highlighted = X :\n";
                else
                    str += "- Highlighted = @@ :\n";
            }
        }
        if (numargs > 3)
        {
            neighborPoint = va_arg(args, unsigned);
            IntVector2 coord = IntVector2((int)(neighborPoint % width)-1, (int)(neighborPoint / width)-1);
            str.AppendWithFormat("currentPoint(*) at %s[%u] data= %u\n", coord.ToString().CString(), neighborPoint, data[neighborPoint]);
        }
        if (numargs > 4)
        {
            contourStartPoint = va_arg(args, unsigned);
            IntVector2 coord = IntVector2((int)(contourStartPoint % width)-1, (int)(contourStartPoint / width)-1);
            str.AppendWithFormat("startPoint(!) at %s\n", coord.ToString().CString());
        }
        if (numargs > 5)
        {
            backtrackStartPoint = va_arg(args, unsigned);
            IntVector2 coord = IntVector2((int)(backtrackStartPoint % width)-1, (int)(backtrackStartPoint / width)-1);
            str.AppendWithFormat("startBackTrackPoint(#) at %s\n", coord.ToString().CString());
        }
        if (numargs > 6)
        {
            boundaryPoint = va_arg(args, unsigned);
            IntVector2 coord = IntVector2((int)(boundaryPoint % width)-1, (int)(boundaryPoint / width)-1);
            str.AppendWithFormat("boundaryPoint(+) at %s[%u]\n", coord.ToString().CString(), boundaryPoint);
        }
        if (numargs > 7)
        {
            backtrackPoint = va_arg(args, unsigned);
            IntVector2 coord = IntVector2((int)(backtrackPoint % width)-1, (int)(backtrackPoint / width)-1);
            str.AppendWithFormat("boundaryBackTrackPoint(&) to %s[%u]\n",coord.ToString().CString(), backtrackPoint);
        }
    }

    va_end(args);

    str += "\n";

    // Pattern Mode
    if (startpattern > 0)
    {
        if (highlight && startpattern == pattern)
        {
            char mark = (char)startpattern;
            // Display only the pattern mark
            for (unsigned i = 0; i < Size; i++)
            {
                if (i > 0 && i % width == 0) str += "\n";

                char currChar = (char)data[i*sizeT];

                if (currChar == mark)
                    str += "X";
                else if (currChar == 0)
                    str += ".";
                else
                    str += "¤";
            }
        }
        else
        {
            for (unsigned i = 0; i < Size; i++)
            {
                if (i > 0 && i % width == 0) str += "\n";

                char currChar = (char)data[i*sizeT];

                if (highlight && currChar >= pattern)
                {
                    str += "@@";
                    continue;
                }

                if (currChar == 0)
                    currChar = '.';
                else if (currChar == 11) // NoDecal
                    currChar = ';';
                else if (currChar == 12) // NoRender
                    currChar = ':';
                else if (currChar - startpattern < 0)
                    currChar = '\?';
                else if (numargs > 7 && (currChar == '*' || currChar == '!' || currChar == '!' || currChar == '#' || currChar == '+' || currChar == '&'))
                    ;
                else
                    currChar = 'A' + (currChar - startpattern);

                // Write escape sequence too
                if (currChar == 0x22)
                    str += '\'';
                else if (currChar == 0x27)
                    str += '\"';
                else if (currChar == 0x5C)
                    str += '\\';
                else
                    str += currChar;

                str += " ";
            }
        }
    }
    else
        // No Mark Pattern - Show Simple Mark (if MarkPattern = 0) Or Show Number (if MarkPattern < 0)
    {
        for (unsigned i = 0; i < Size; i++)
        {
            if (i > 0 && i % width == 0) str += "\n";

            if (data[i*sizeT] == 0)
            {
                if (!startpattern)
                    str += ". ";
                else
                    str += "... ";
            }
            else
            {
                if (!startpattern)
                    str += "X ";
                else
                    AppendBufferToString(str, "%03d ", data[i*sizeT]);
            }
        }
    }

    URHO3D_LOGINFOF("%s", str.CString());
}

// specialize DumpData for float
template <>
void GameHelpers::DumpData<float>(const float* buffer, int markpattern, int numargs,  ...)
{
    if (numargs < 2)
        return;

    va_list args;
    va_start(args, numargs);

    unsigned width = va_arg(args, unsigned);
    unsigned height = va_arg(args, unsigned);

    va_end(args);

//    URHO3D_LOGINFOF("DumpData : ");

    String str;

    str += "\n";

    unsigned size = width * height;
    for (unsigned i = 0; i < size; i++)
    {
        if (i > 0 && i % width == 0)
            str += "\n";

        static char tempBuffer[8];
        sprintf(tempBuffer, "%+6.3f", buffer[i]);
        String s = tempBuffer;
        str += s;
        str += " ";
    }

    URHO3D_LOGINFOF("%s", str.CString());
}

template void GameHelpers::DumpData(const FeatureType* data, int markpattern, int n,  ...);
template void GameHelpers::DumpData(const unsigned* data, int markpattern, int n,  ...);
template void GameHelpers::DumpData(const int* data, int markpattern, int n,  ...);
template void GameHelpers::DumpData(const FluidCell* data, int markpattern, int n,  ...);
template void GameHelpers::DumpData(Tile* const *data, int markpattern, int n,  ...);

void GameHelpers::DumpVertices(const PODVector<Vector2>& vertices)
{
    String str;

    for (unsigned i=0; i < vertices.Size(); i++)
        str.AppendWithFormat("v[%d]=%F,%F ", i, vertices[i].x_, vertices[i].y_);

    URHO3D_LOGINFOF("GameHelpers() - DumpVertices : num=%u values=%s", vertices.Size(), str.CString());
}

/// Graphic Dump

void GameHelpers::DrawDebugRect(const Rect& rect, DebugRenderer* debugRenderer, bool depthTest, const Color& color)
{
    debugRenderer->AddLine(Vector3(rect.min_.x_, rect.min_.y_, 0.f), Vector3(rect.min_.x_, rect.max_.y_, 0.f), color, depthTest);
    debugRenderer->AddLine(Vector3(rect.min_.x_, rect.max_.y_, 0.f), Vector3(rect.max_.x_, rect.max_.y_, 0.f), color, depthTest);
    debugRenderer->AddLine(Vector3(rect.max_.x_, rect.max_.y_, 0.f), Vector3(rect.max_.x_, rect.min_.y_, 0.f), color, depthTest);
    debugRenderer->AddLine(Vector3(rect.max_.x_, rect.min_.y_, 0.f), Vector3(rect.min_.x_, rect.min_.y_, 0.f), color, depthTest);
}

void GameHelpers::DrawDebugUIRect(const IntRect& rect, const Color& color, bool filled)
{
    PODVector<UIBatch>* batches;
    PODVector<float>* vertexData;

    GameContext::Get().context_->GetSubsystem<UI>()->GetDebugBatchesPtr(batches, vertexData);

    UIBatch batch(0, BLEND_ALPHA, rect, 0, vertexData);

    if (filled)
    {
        Color colora = color;
        colora.a_ = 0.5f;
        batch.SetColor(colora);
        batch.AddQuad(rect.left_, rect.top_, rect.Width(), rect.Height(), 0, 0);
    }
    else
    {
        batch.SetColor(color);
        int horizontalThickness = 2;
        int verticalThickness = 2;
        // Left
        batch.AddQuad(rect.left_, rect.top_, horizontalThickness, rect.Height(), 0, 0);
        // Top
        batch.AddQuad(rect.left_, rect.top_, rect.Width(), verticalThickness, 0, 0);
        // Right
        batch.AddQuad(rect.right_ - horizontalThickness, rect.top_, horizontalThickness, rect.Height(), 0, 0);
        // Bottom
        batch.AddQuad(rect.left_, rect.bottom_ - verticalThickness, rect.Width(), verticalThickness, 0, 0);
    }

    UIBatch::AddOrMerge(batch, *batches);
}


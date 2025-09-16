#include <Urho3D/Urho3D.h>

#include <Urho3D/Core/CoreEvents.h>

#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Graphics/GraphicsEvents.h>

#include <Urho3D/Graphics/ShaderVariation.h>
#include <Urho3D/Graphics/VertexBuffer.h>
#include <Urho3D/Graphics/Texture2D.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/Graphics/Viewport.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/Graphics/RenderPath.h>

#include <Urho3D/Input/Input.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/IO/FileSystem.h>

#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Resource/XMLFile.h>

#include <Urho3D/Scene/Scene.h>

#include <Urho3D/UI/BorderImage.h>
#include <Urho3D/UI/Button.h>
#include <Urho3D/UI/CheckBox.h>
#include <Urho3D/UI/DropDownList.h>
#include <Urho3D/UI/Font.h>
#include <Urho3D/UI/LineEdit.h>
#include <Urho3D/UI/ListView.h>
#include <Urho3D/UI/Slider.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/UI/Text3D.h>
#include <Urho3D/UI/ToolTip.h>
#include <Urho3D/UI/View3D.h>
#include <Urho3D/UI/UI.h>
#include <Urho3D/UI/UIElement.h>
#include <Urho3D/UI/UIEvents.h>

#include <Urho3D/Urho2D/PhysicsWorld2D.h>
#include <Urho3D/Urho2D/RigidBody2D.h>
#include <Urho3D/Urho2D/CollisionShape2D.h>
#include <Urho3D/Urho2D/CollisionChain2D.h>
#include <Urho3D/Urho2D/AnimationSet2D.h>
#include <Urho3D/Urho2D/AnimatedSprite2D.h>
#include <Urho3D/Urho2D/ParticleEffect2D.h>
#include <Urho3D/Urho2D/Renderer2D.h>
#include <Urho3D/Urho2D/Sprite2D.h>
#include <Urho3D/Urho2D/SpriteSheet2D.h>
#include <Urho3D/Urho2D/SpriterInstance2D.h>
#include <Urho3D/Urho2D/SpriterData2D.h>

#include "DefsCore.h"
#include "DefsEntityInfo.h"
#include "DefsColliders.h"

#include "TimerRemover.h"

#include "GameOptions.h"
#include "GameContext.h"
#include "GameHelpers.h"

#include "MapWorld.h"
#include "Map.h"
#include "Urho3D/Input/InputConstants.h"
#include "ViewManager.h"

#include "ObjectTiled.h"
#include "GEF_Scrolling.h"
#include "RenderShape.h"



#define linewidth 5.f
#define pointsize 10.f

#if defined(DESKTOP_GRAPHICS) && defined(URHO3D_OPENGL)
#include <GLEW/glew.h>
#define MAPEDITOR_OGL
#endif

#include "MapEditorLib.h"


enum SpawnCategory
{
    SPAWN_NONE = 0,
    SPAWN_ACTOR,
    SPAWN_MONSTER,
    SPAWN_PLANT,
    SPAWN_TILE,
    SPAWN_WATER,
    SPAWN_DOOR,
    SPAWN_LIGHT,
    SPAWN_DECORATION,
    SPAWN_CONTAINER,
    SPAWN_COLLECTABLE,
    SPAWN_TOOL,

    MAX_SPAWNCATEGORIES
};

const char* SpawnCategoryNameStr[] =
{
    "None", "Actor", "Monster", "Plant", "Tile", "Water", "Door", "Light", "Decoration", "Container", "Collectable", "Tool", 0
};

enum ClickMode
{
    CLICK_NONE = 0,
    CLICK_SELECT,
    CLICK_SPAWN,
    CLICK_MOVE,
};

enum PickMode
{
    PICK_GEOMETRIES = 0,
    PICK_RIGIDBODIES,
    PICK_COLLISIONSHAPES,
    PICK_RENDERSHAPES,
#if defined(ACTIVE_CLIPPING) && defined(ACTIVE_RENDERSHAPE_CLIPPING)
    PICK_RENDERSHAPES_CLIPPED,
#endif
    PICK_LIGHTS,
    MAX_PICK_MODES
};

enum GeomEditMode
{
    GE_TRANSLATE = 0,
    GE_SCALE,
    GE_ROTATE
};

enum ResetSelectionMode
{
    RESETALLSELECT = 0,
    RESETHOVERSELECT,
    RESETCLICKSELECT
};

const char* PickModeStr[] =
{
    "PICK_GEOMETRIES",
    "PICK_RIGIDBODIES",
    "PICK_COLLISIONSHAPES",
    "PICK_RENDERSHAPES",
#if defined(ACTIVE_CLIPPING) && defined(ACTIVE_RENDERSHAPE_CLIPPING)
    "PICK_RENDERSHAPES_CLIPPED",
#endif
    "PICK_LIGHTS",
    0
};

unsigned pickModeDrawableFlags[] =
{
    DRAWABLE_GEOMETRY,
    0,
    0,
    0,
#if defined(ACTIVE_CLIPPING) && defined(ACTIVE_RENDERSHAPE_CLIPPING)
    0,
#endif
    DRAWABLE_LIGHT,
    0,
};

enum UIStyleType
{
    UISTYLE_GAMEDEFAULT = 0,
    UISTYLE_EDITORDEFAULT,
    UISTYLE_TOOLBARICONS,
    UISTYLE_ICONS,
    UISTYLE_INSPECTOR,

    MAX_UISTYLES
};

String UIStyleXmlFilenames[] =
{
    "",
    "Editor/DefaultStyle.xml",
    "Editor/FromBonesSpawnIcons.xml",
    "Editor/EditorIcons.xml",
    "Editor/EditorInspector_Style.xml",
};

enum EditorPanel
{
    PANEL_MAINTOOLBAR = 0,
    PANEL_INSPECTORWINDOW,
    PANEL_MONSTERPOPLIST,
    PANEL_PLANTPOPLIST,
    PANEL_DOORPOPLIST,
    PANEL_LIGHTPOPLIST,
    PANEL_DECORATIONPOPLIST,
    PANEL_CONTAINERPOPLIST,
    PANEL_COLLECTABLEPOPLIST,
    PANEL_TOOLPOPLIST,
    PANEL_ANIMATOR2D_MAIN,
    PANEL_ANIMATOR2D_CMAPS,
    PANEL_ANIMATOR2D_NEWCMAPS,
    PANEL_ANIMATOR2D_SWSPRITES,
    PANEL_ANIMATOR2D_COLORMAPPING,
    PANEL_ANIMATOR2D_SSELECTLIST,
    PANEL_ANIMATOR2D_ANIMS,
//    PANEL_HIERARCHYWINDOW,

    MAX_EDITORPANELS
};

String EditorPanelXmlFilenames[] =
{
    "",
    "Editor/EditorInspectorWindow.xml",
    "Editor/EditorPopList.xml",
    "Editor/EditorPopList.xml",
    "Editor/EditorPopList.xml",
    "Editor/EditorPopList.xml",
    "Editor/EditorPopList.xml",
    "Editor/EditorPopList.xml",
    "Editor/EditorPopList.xml",
    "Editor/EditorPopList.xml",
    "Editor/EditorAnimator2D_Main.xml",
    "Editor/EditorAnimator2D_CharacterMaps.xml",
    "Editor/EditorAnimator2D_NewCMap.xml",
    "Editor/EditorAnimator2D_SwapSprites.xml",
    "Editor/EditorAnimator2D_ColorMapping.xml",
    "Editor/EditorAnimator2D_SpritesSelectionList.xml",
    "Editor/EditorAnimator2D_Animations.xml",
//    "Editor/EditorHierarchyWindow.xml",
};

enum EditorElement
{
    EDITORELT_INSPECTORATTRIBUTE,
    EDITORELT_INSPECTORTAG,
    EDITORELT_INSPECTORVARIABLE,
    EDITORELT_ANIMATORMAPSWAPSPRITE,
    EDITORELT_ANIMATORSPRITECOLOR,

    MAX_EDITORELEMENTS
};

String EditorElementXmlFilenames[] =
{
    "Editor/EditorInspector_Attribute.xml",
    "Editor/EditorInspector_Tags.xml",
    "Editor/EditorInspector_Variable.xml",
    "Editor/EditorAnimator2D_MapSwapSpriteElt.xml",
    "Editor/EditorAnimator2D_ColorMappingElt.xml"
};

static Color EditorSpriteColors_[] =
{
    Color::WHITE,
    Color::GRAY,
    Color::BLACK,
    Color::YELLOW,
    Color::RED,
    Color::GREEN,
    Color::BLUE,
    Color::CYAN,
    Color::MAGENTA,
};

const int ITEM_NONE = 0;
const int ITEM_NODE = 1;
const int ITEM_COMPONENT = 2;
const unsigned NO_ITEM = M_MAX_UNSIGNED;
const unsigned SPRITEKEY_REMOVE = 0xFFFFFFFF;
const unsigned SPRITEKEY_IGNORE = 0xFFFF0000;
const unsigned SPRITEKEY_SWAP   = 0x0000FFFF;

const String STRIKED_OUT = "——";   // Two unicode EM DASH (U+2014)
const String TITLETEXT("TitleText");
const String ATTRIBUTELIST("AttributeList");
const String ICONSPANEL("IconsPanel");
const String PARENTCONTAINER("ParentContainer");
const String RESETTODEFAULT("ResetToDefault");
const String TAGSEDIT("TagsEdit");
const String ICON("Icon");
const String UNKNOWN("Unknown");

const StringHash KEYTIME("KeyTime");
const StringHash MAPPINGSPRITE_KEY("SpriteKey");
const StringHash SWAPPINGSPRITE_PTR("SwappingSprite");
const StringHash UIELEMENT_CHILDINDEX("UIElementChildIndex");
const StringHash PANEL_ID("Panel_ID");
const StringHash DISABLEVIEWRAYCAST("DisableViewRayCast");
const StringHash NODE_IDS_VAR("NodeIDs");
const StringHash COMPONENT_IDS_VAR("ComponentIDs");
const StringHash NODE_ID_VAR("NodeID");
const StringHash COMPONENT_ID_VAR("ComponentID");
const StringHash UI_ELEMENT_ID_VAR("UIElementID");
const StringHash SERIALIZABLE_PTRS_VAR("SerializablePtr");
const StringHash DRAGDROPCONTENT_VAR("DragDropContent");
const StringHash TYPE_VAR("Type");
const StringHash UI_ELEMENT_TYPE("UIElement");
const StringHash WINDOW_TYPE("Window");
const StringHash MENU_TYPE("Menu");
const StringHash TEXT_TYPE("Text");
const StringHash CURSOR_TYPE("Cursor");
const StringHash INDENT_MODIFIED_BY_ICON_VAR("IconIndented");

bool attributesDirty = false;
bool attributesFullDirty = false;

const unsigned numEditableComponentsPerNode = 1;

const unsigned MIN_NODE_ATTRIBUTES = 4;
const unsigned MAX_NODE_ATTRIBUTES = 8;
const int SPAWNBAR_HEIGHT = 68;
const int INSPECTOR_WIDTH = 440;
const int LABEL_WIDTH = 30;
const int ATTRNAME_WIDTH = 150;
const int ATTR_HEIGHT = 19;
const StringHash TEXT_CHANGED_EVENT_TYPE("TextChanged");

Color normalTextColor(1.0f, 1.0f, 1.0f);
Color modifiedTextColor(1.0f, 1.f, 0.0f);
Color nonEditableTextColor(0.7f, 0.7f, 0.7f);
Color nonEditableBitSelectorColor(0.5f, 0.5f, 0.5f);
Color editableBitSelectorColor(1.0f, 1.0f, 1.0f);
const unsigned BoneColor_Selected = Color(0.f, 1.f, 1.f, 0.4f).ToUInt();
const unsigned BoneColorBorder_Selected = Color(1.f, 1.f, 0.f, 1.f).ToUInt();
const unsigned BoneColor = Color(0.5f, 0.5f, 0.5f, 0.4f).ToUInt();
const unsigned BoneColorBorder = Color(1.f, 0.f, 1.f, 1.f).ToUInt();

unsigned inspectorNodeContainerIndex_;
unsigned inspectorComponentContainerStartIndex_;
bool inspectorShowNonEditableAttribute = false;
bool inspectorInLoadAttributeEditor;
bool inspectorShowID = true;
bool inspectorLocked_ = false;

float waterQuantity_;

SharedPtr<XMLFile> GetEditorUIXMLFile(const String& fileName)
{
    String fullFileName = GameContext::Get().fs_->GetProgramDir() + "Data/" + fileName;
    if (GameContext::Get().fs_->FileExists(fullFileName))
    {
        File file(GameContext::Get().context_, fullFileName, FILE_READ);
        SharedPtr<XMLFile> xml(new XMLFile(GameContext::Get().context_));
        xml->SetName(fileName);
        if (xml->Load(file))
            return xml;
    }

    return SharedPtr<XMLFile>();
}

struct EditorPoint
{
    EditorPoint() { }
    EditorPoint(const Vector2& point, unsigned color) :
        point_(point),
        color_(color) { }

    Vector3 point_;
    unsigned color_;
};

struct EditorLine
{
    EditorLine() { }
    EditorLine(const Vector2& start, const Vector2& end, unsigned color) :
        start_(start),
        end_(end),
        color_(color) { }

    Vector3 start_, end_;
    unsigned color_;
};

struct EditorTriangle
{
    EditorTriangle() { }
    EditorTriangle(const Vector2& v0, const Vector2& v1, const Vector2& v2, unsigned color) :
        v0_(v0),
        v1_(v1),
        v2_(v2),
        color_(color) { }

    Vector2 v0_, v1_, v2_;
    unsigned color_;
};

class FROMBONES_API MapEditorLibImpl : public MapEditorLib
{
public :
    MapEditorLibImpl(Context* context);
    virtual ~MapEditorLibImpl();

    virtual void Update(float timestep);
    virtual void Clean();
    virtual void Render();

    void SetVisible(int panel, bool visible);

    static XMLFile* GetUIStyle(int index) { return uiStyles_[index]; }
    static XMLElement GetXmlElement(int index) { return xmlFiles_[index]->GetRoot(); }
    static MapEditorLibImpl* Get() { return editor_; }
    static UIElement* GetPanel(int panelid);

    void SubscribeToUIEvent();
    void HandleResizeUI(StringHash eventType, VariantMap& eventData);
    void HandleToolBarCheckBoxToogle(StringHash eventType, VariantMap& eventData);
    void HandleToolBarLayerAlignChanged(StringHash eventType, VariantMap& eventData);
    void HandleCategoryTypeSelected(StringHash eventType, VariantMap& eventData);
    void HandleWindowLayoutUpdated(StringHash eventType, VariantMap& eventData);
    void HandleHidePanel(StringHash eventType, VariantMap& eventData);
    void HandleToggleInspectorLock(StringHash eventType, VariantMap& eventData);
    void HandleInspectorResetToDefault(StringHash eventType, VariantMap& eventData);
    void HandleInspectorTagsEdit(StringHash eventType, VariantMap& eventData);
    void HandleInspectorTagsSelect(StringHash eventType, VariantMap& eventData);
    void HandleInspectorEditAttribute(StringHash eventType, VariantMap& eventData);
    void HandleInspectorPickResource(StringHash eventType, VariantMap& eventData);
    void HandleInspectorOpenResource(StringHash eventType, VariantMap& eventData);
    void HandleInspectorEditResource(StringHash eventType, VariantMap& eventData);
    void HandleInspectorTestResource(StringHash eventType, VariantMap& eventData);

    void EditResource(Resource* resource);

private:
    void CreateUI();
    void CreateSpawnToolBar();
    void SetCategoryPopList(int spawnCategory);
    void RemoveUI();
    void ResizeUI();

    void UpdatePanel(int panel, bool force=false);
    void UpdateSpawnToolBar();
    void UpdateInspector(bool fullupdate);
    void UpdateNodesAttributes();

    void SpawnObject(const WorldMapPosition& position);
    bool MoveObjects(const Vector2& adjust);

    void DeleteSelectedObjects();
    void UnSelectObject();
    void SelectObject(Drawable* shape);
    void SelectObject(CollisionShape2D* shape);
    void SelectObject(RenderShape* shape, RenderCollider* collider);

    void UpdateSelectShape();

    void AddShapeToRender(Drawable* shape, unsigned linecolor, unsigned pointcolor);
    void AddShapeToRender(CollisionShape2D* shape, unsigned linecolor, unsigned pointcolor);
    void AddShapeToRender(RenderShape* shape, unsigned linecolor, unsigned pointcolor, bool addBoundingRect=false);
    void AddAnimatorBonesToRender();
    void AddVerticesToRender(const PODVector<Vector2>& vertices, const Matrix2x3& transform, unsigned linecolor, unsigned pointcolor, bool addpoints=true, bool addpointtext=true, const String& tag=String::EMPTY);
    void AddDebugText(const String& text, const Vector2& position);

    Vector<Node*> selectedNodes_;
    Vector<Component*> selectedComponents_;
    SharedPtr<Node> editNode_;
    Vector<Node*> editNodes_;
    Vector<Component*> editComponents_;

    Drawable* selectedDrawable_;
    CollisionShape2D* selectedShape_;
    RenderShape* selectedRenderShape_;
    RenderCollider* selectedRenderCollider_;
    int selectedPolyShapeId_;
    int selectedRenderContourId_;
    unsigned selectRenderShapeColor_;
    bool selectRenderShapeClipped_;
    bool multiSelection_;
    int disableViewRayCast_;

    /// Points to render
    PODVector<EditorPoint> points_;
    /// Lines to render
    PODVector<EditorLine> lines_;
    /// Triangles to render
    PODVector<EditorTriangle> triangles_;
    /// Contour (linewidth=1) to render
    PODVector<EditorLine> contours_;

    /// Vertex buffer.
    SharedPtr<VertexBuffer> vertexBuffer_;

    SharedPtr<Texture2D> linetexture_, pointtexture_;

    UIElement* editorUI_;
    Vector<SharedPtr<UIElement> > uiPanels_;
    Vector<bool > uiPanelDirty_;

    static Vector<SharedPtr<XMLFile> > uiStyles_;
    static Vector<SharedPtr<XMLFile> > xmlFiles_;
    static MapEditorLibImpl* editor_;

    Scene* scene_;
    WeakPtr<Node> debugTextRootNode_;
    Rect lastClippedRect_;

    float cameraBaseSpeed;
    float cameraBaseRotationSpeed;
    float cameraShiftSpeedMultiplier;
    float moveStep_;
    bool cameraViewMoving, moveObjects_;

    float uiMaxOpacity_;

    int pickMode_, clickMode_;
    int spawnCategory_;
    StringHash spawnObject_;
    int editorLayerModifier_;
};

// Resource picker functionality
const unsigned ACTION_PICK = 1;
const unsigned ACTION_OPEN = 2;
const unsigned ACTION_EDIT = 4;
const unsigned ACTION_TEST = 8;

struct ResourcePicker
{
    ResourcePicker() { }

    static void Init()
    {
        resourcePickers_.Clear();
        resourceTargets_.Clear();
        resourcePickIndex_ = 0;
        resourcePickSubIndex_ = 0;
        resourcePicker_ = 0;

        Register("Font", "*.ttf;*.otf;*.fnt;*.xml;*.sdf");
        Register("Image", "*.png;*.jpg;*.bmp;*.tga");
        Register("ScriptFile", "*.as;*.asc");
        Register("Sound", "*.wav;*.ogg");
        Register("Technique", "*.xml");
        Register("Texture2D", "*.dds;*.png;*.jpg;*.bmp;*.tga;*.ktx;*.pvr");
        Register("XMLFile", "*.xml");
        Register("Material", "*.xml;*.material", ACTION_PICK | ACTION_OPEN);
        Register("Sprite2D", "*.xml;*.material", ACTION_PICK | ACTION_OPEN);
        Register("AnimationSet2D", "*.scml", ACTION_PICK | ACTION_OPEN | ACTION_EDIT);
        Register("SpriteSheet2D", "*.xml", ACTION_PICK | ACTION_OPEN);
        Register("ParticleEffect2D", "*.pex", ACTION_PICK | ACTION_OPEN);
    }
    static void Register(const String& typeName, const String& filters, unsigned actions = ACTION_PICK | ACTION_OPEN)
    {
        StringHash type(typeName);
        ResourcePicker& picker = resourcePickers_[type];

        picker.typeName_ = typeName;
        picker.type_     = type;
        picker.actions_  = actions;
        Vector<String> vfilters = filters.Split(';');
        for (unsigned i=0; i < vfilters.Size(); i++)
            picker.filters_.Push(vfilters[i]);
        picker.filters_.Push("*.*");
        picker.lastFilter_ = 0;
    }
    static const ResourcePicker* Get(StringHash hash)
    {
        HashMap<StringHash, ResourcePicker>::ConstIterator it = resourcePickers_.Find(hash);
        return it != resourcePickers_.End() ? &it->second_ : 0;
    }

    String typeName_;
    StringHash type_;
    String lastPath_;
    unsigned lastFilter_;
    Vector<String> filters_;
    unsigned actions_;

private:

    static HashMap<StringHash, ResourcePicker> resourcePickers_;
    static Vector<Serializable*> resourceTargets_;
    static unsigned resourcePickIndex_;
    static unsigned resourcePickSubIndex_;
    static ResourcePicker* resourcePicker_;
};

HashMap<StringHash, ResourcePicker> ResourcePicker::resourcePickers_;
Vector<Serializable*> ResourcePicker::resourceTargets_;
unsigned ResourcePicker::resourcePickIndex_;
unsigned ResourcePicker::resourcePickSubIndex_;
ResourcePicker* ResourcePicker::resourcePicker_;


class BoneGizmo : public Object
{
    URHO3D_OBJECT(BoneGizmo, Object);

public :
    BoneGizmo(Context* context) : Object(context), length_(1.f), width_(0.1f), selected_(false) { node_ = GameContext::Get().rootScene_->CreateChild("BoneGizmo", LOCAL); }
    virtual ~BoneGizmo() { if (node_) node_->Remove(); };

    void SetVisible(bool enable)
    {
        node_->SetEnabled(enable);
    }

    void SetSelected(bool enable)
    {
        selected_ = enable;
    }

    void Update(Node* node, Spriter::BoneTimelineKey* key, bool force=false)
    {
        if (node_->GetParent() != node)
        {
            node_->SetParent(node);
            animatedSprite_ = node->GetComponent<AnimatedSprite2D>();
        }

        node_->SetPosition2D(animatedSprite_->GetFlipX() ? -key->info_.x_ * PIXEL_SIZE : key->info_.x_ * PIXEL_SIZE, animatedSprite_->GetFlipY() ? -key->info_.y_ * PIXEL_SIZE : key->info_.y_ * PIXEL_SIZE);
        node_->SetRotation2D(animatedSprite_->GetFlipX() != animatedSprite_->GetFlipY() ? 180.f-key->info_.angle_ : key->info_.angle_);
        node_->SetScale2D(key->info_.scaleX_, key->info_.scaleY_);

        if (timelineHash_ != key->timeline_->hashname_ || force)
        {
            timelineHash_ = key->timeline_->hashname_;
            const Spriter::ObjInfo& objinfo = animatedSprite_->GetSpriterInstance()->GetEntity()->objInfos_[timelineHash_];
            length_ = Clamp(objinfo.width_ * PIXEL_SIZE, 0.05f, 2.f);
            width_  = Clamp(objinfo.height_ * PIXEL_SIZE * 0.5f, 0.0025f, 0.5f);
            // reduce width to keep good proportion with the length
            if (width_ > 0.125f * length_)
                width_ = 0.125f * length_;
        }
    }

    void Render(PODVector<EditorTriangle>& triangles, PODVector<EditorLine>& contours, float scale)
    {
        if (!node_->IsEnabled())
            return;

        const Matrix2x3& transform = node_->GetWorldTransform2D();
        v0_ = transform * Vector2(-width_, 0.f);
        v1_ = transform * Vector2(0.f, width_);
        v2_ = transform * Vector2(0.f, -width_);
        v3_ = transform * Vector2(length_, 0.f);

        if (!selected_)
        {
            triangles.Push(EditorTriangle(v0_, v1_, v2_, BoneColor));
            triangles.Push(EditorTriangle(v1_, v3_, v2_, BoneColor));
            contours.Push(EditorLine(v0_, v1_, BoneColorBorder));
            contours.Push(EditorLine(v0_, v2_, BoneColorBorder));
            contours.Push(EditorLine(v1_, v3_, BoneColorBorder));
            contours.Push(EditorLine(v2_, v3_, BoneColorBorder));
        }
        else
        {
            triangles.Push(EditorTriangle(v0_, v1_, v2_, BoneColor_Selected));
            triangles.Push(EditorTriangle(v1_, v3_, v2_, BoneColor_Selected));
            contours.Push(EditorLine(v0_, v1_, BoneColorBorder_Selected));
            contours.Push(EditorLine(v0_, v2_, BoneColorBorder_Selected));
            contours.Push(EditorLine(v1_, v3_, BoneColorBorder_Selected));
            contours.Push(EditorLine(v2_, v3_, BoneColorBorder_Selected));
        }
    }

    int IsInside(const Vector2& position) const
    {
        if (GameHelpers::IsInsideTriangle(position, v0_, v1_, v2_))
            return 0;

        if (GameHelpers::IsInsideTriangle(position, v1_, v3_, v2_))
            return 1;

        return -1;
    }

private :
    WeakPtr<Node> node_;
    WeakPtr<AnimatedSprite2D> animatedSprite_;
    StringHash timelineHash_;
    float length_, width_;
    Vector2 v0_, v1_, v2_, v3_;
    bool selected_;
};

class AnimatorEditor : public Object
{
    URHO3D_OBJECT(AnimatorEditor, Object);

public :
    AnimatorEditor(Context* context);
    virtual ~AnimatorEditor();

    void Edit(Node* node);
    void EditInPreview(AnimationSet2D* animationSet);
//    void Edit(AnimationSprite2D* animatedSprite);

    AnimatedSprite2D* GetAnimatedSprite() const { return editedAnimatedSprite_.Get(); }
    const Vector<SharedPtr<BoneGizmo> >& GetBonesGizmos() const { return bonesGizmos_; }

    static AnimatorEditor* Get()
    {
        if (!animatorEditor_)
            animatorEditor_ = new AnimatorEditor(GameContext::Get().context_);
        return animatorEditor_;
    }

    void UpdateMapping(bool updatesprites, bool updatecolors);

    void Update(const Vector2& worldposition);
    void ResetSelection(int mode);
    void Select(const Vector2& worldposition, bool findbottomsprite, bool hover);
    void MoveSelection(const Vector2& worldposition, const Vector2& delta, int mode);
    bool HasSelection() const { return lastSelectedTimelineId_ != -1 || lastSelectedBoneIndex_ != -1; }

private :

    void FinishEdit();

    void SetCharacterMappingPanel();
    UIElement* AddMap(const String& mapname);
    UIElement* ApplyMap(const String& mapname, bool isAColorMap);
    UIElement* PurgeAndApplyMap(UIElement* elt);
    void HandleResize(StringHash eventType, VariantMap& eventData);
    void HandleVisible(StringHash eventType, VariantMap& eventData);
    void HandleAnimationSetSave(StringHash eventType, VariantMap& eventData);
    void HandleEntitySelected(StringHash eventType, VariantMap& eventData);
    void HandleCMapSelected(StringHash eventType, VariantMap& eventData);
    void HandleEditCMap(StringHash eventType, VariantMap& eventData);
    void HandleApplyCMap(StringHash eventType, VariantMap& eventData);
    void HandleNewCMap(StringHash eventType, VariantMap& eventData);
    void HandleNewCMapSelectType(StringHash eventType, VariantMap& eventData);
    void HandleNewCMapCreate(StringHash eventType, VariantMap& eventData);
    void HandleCMapSave(StringHash eventType, VariantMap& eventData);
    void HandleDelCMap(StringHash eventType, VariantMap& eventData);
    void HandleMoveUpAppliedCMap(StringHash eventType, VariantMap& eventData);
    void HandleMoveDownAppliedCMap(StringHash eventType, VariantMap& eventData);
    void HandleRemoveAppliedCMap(StringHash eventType, VariantMap& eventData);

    void SetAnimationPanel();
    void SetTimeKeys();
    void PlayAnimation(bool enable=true);
    Spriter::Timeline* AddTimeline(Spriter::SpriterObjectType type, float time);
    Spriter::MainlineKey* AddMainlineKey(unsigned newmainkeyid, float time);
    void AddSpriterObject(Spriter::SpriterObjectType type, const Vector2& worldposition, bool finish);
    void UpdateRefs(float time, unsigned timelineid, int parentid);
    void UpdateBonesGizmos(bool force=false);
    void UpdateTimeKeys();
    void UpdateTimeKeysIcons();
    void UpdateTimelineInfos();
    void AdjustSpinAt(Spriter::Timeline* timeline, unsigned key);
    void HandleAnimationKeyValueChanged(StringHash eventType, VariantMap& eventData);
    void HandleAnimationModifyTimeKey(StringHash eventType, VariantMap& eventData);
    void HandleTimeKeyContainerResized(StringHash eventType, VariantMap& eventData);
    void HandleAnimationSelect(StringHash eventType, VariantMap& eventData);
    void HandleAnimationToggleShowBones(StringHash eventType, VariantMap& eventData);
    void HandleAnimationToggleTime(StringHash eventType, VariantMap& eventData);
    void HandleAnimationChangeTime(StringHash eventType, VariantMap& eventData);
    void HandleAnimationPlay(StringHash eventType, VariantMap& eventData);

    void SetColorSpritePanel();
    void SetSpriteSlotColor(UIElement* slot, const Color& color);
    void UpdateColorSpritePanel();
    void HandleColorPressed(StringHash eventType, VariantMap& eventData);

    void SetSpriteMappingPanel();
    UIElement* GetSpriteMappingListItem(unsigned key) const;
    void UpdateSpriteMappingPanel();
    void UpdateSpriteMappingListItem(UIElement* item, Spriter::MapInstruction* instruct);
    void HandleSpriteMappingPanelToggleShow(StringHash eventType, VariantMap& eventData);
    void HandleSpriteMappingPanelSelectMappedSprite(StringHash eventType, VariantMap& eventData);
    void HandleSpriteMappingPanelSelectSwappedSprite(StringHash eventType, VariantMap& eventData);

    void HandleSpriteMappingPanelSelectionUnMap(StringHash eventType, VariantMap& eventData);
    void HandleSpriteMappingPanelSelectionHide(StringHash eventType, VariantMap& eventData);
    void HandleSpriteMappingPanelSelectionUnSwap(StringHash eventType, VariantMap& eventData);

    void SetSpriteSelectionList(SpriteSheet2D* spritesheet=0);
    void HandleSpriteSelectionSpriteSelected(StringHash eventType, VariantMap& eventData);
    void HandleSpriteSelectionStartEndMovePivot(StringHash eventType, VariantMap& eventData);
    void HandleSpriteSelectionMovePivot(StringHash eventType, VariantMap& eventData);
    void HandleSpriteSelectionShowPivots(StringHash eventType, VariantMap& eventData);

    SharedPtr<Scene> mainScene_, previewScene_;
    UIElement* panel_;
    View3D* preview_;
    ListView* spriteMappingList_;
    Slider* timeSlider_;

    WeakPtr<Node> editNode_;
    WeakPtr<AnimatedSprite2D> editedAnimatedSprite_;
    WeakPtr<Node> cloneNode_;
    unsigned entityid_;
    int editedSlotIndex_;

    int lastSelectedTimelineId_, lastHoverTimelineId_;
    int lastSelectedBoneIndex_, lastSelectBoneMoveMode_;
    PODVector<Spriter::Ref*> lastSelectedTimelineRefs_, lastHoverTimelineRefs_;
    PODVector<Color> lastSelectedTimelineRefsColors_, lastHoverTimelineRefsColors_;

    bool isPlaying_;
    bool bonesVisible_;
    Vector<SharedPtr<BoneGizmo> > bonesGizmos_;

    static AnimatorEditor* animatorEditor_;
};

AnimatorEditor* AnimatorEditor::animatorEditor_ = 0;



extern "C"
{
    extern FROMBONES_API MapEditorLib* mapeditorcreate(Context* context)
    {
        return new MapEditorLibImpl(context);
    }
    extern FROMBONES_API void mapeditordestroy(MapEditorLib* p)
    {
        delete p;
    }
}



Vector<SharedPtr<XMLFile> > MapEditorLibImpl::uiStyles_;
Vector<SharedPtr<XMLFile> > MapEditorLibImpl::xmlFiles_;
MapEditorLibImpl* MapEditorLibImpl::editor_;

MapEditorLibImpl::MapEditorLibImpl(Context* context) :
    MapEditorLib(context),
    selectedDrawable_(0),
    selectedShape_(0),
    selectedRenderShape_(0),
    selectedRenderCollider_(0),
    selectedPolyShapeId_(0),
    selectedRenderContourId_(0),
    selectRenderShapeColor_(0),
    selectRenderShapeClipped_(false),
    disableViewRayCast_(0),
    cameraBaseSpeed(5.f),
    cameraBaseRotationSpeed(0.2f),
    cameraShiftSpeedMultiplier(5.f),
    moveStep_(0.5f),
    cameraViewMoving(false),
    moveObjects_(false),
    uiMaxOpacity_(0.9f),
    pickMode_(PICK_GEOMETRIES),
    clickMode_(CLICK_NONE),
    spawnCategory_(SPAWN_NONE)
{
    editor_ = this;
    editorLayerModifier_ = 1;

    ResourcePicker::Init();

    vertexBuffer_ = new VertexBuffer(context);

    scene_ = GameContext::Get().rootScene_.Get();

//    scene_->SetUpdateEnabled(false);

    debugTextRootNode_ = scene_->GetChild("EditorDebugText", LOCAL);
    if (!debugTextRootNode_)
        debugTextRootNode_ = scene_->CreateChild("EditorDebugText", LOCAL);

    URHO3D_LOGDEBUGF("MapEditorLibImpl()");

    CreateUI();

    SetVisible(PANEL_MAINTOOLBAR, true);

    SubscribeToUIEvent();
}

MapEditorLibImpl::~MapEditorLibImpl()
{
    AnimatorEditor::Get()->ResetSelection(RESETALLSELECT);

    AnimatorEditor::Get()->UpdateMapping(true, true);

    if (debugTextRootNode_)
        debugTextRootNode_->Remove();

    RemoveUI();

    if (AnimatorEditor::Get())
        delete AnimatorEditor::Get();

    scene_->SetUpdateEnabled(true);

    URHO3D_LOGDEBUGF("~MapEditorLibImpl()");
}


void MapEditorLibImpl::CreateUI()
{
    // Set the UISyle
    uiStyles_.Resize(MAX_UISTYLES);
    uiStyles_[UISTYLE_GAMEDEFAULT] = GameContext::Get().ui_->GetRoot()->GetDefaultStyle();
    for (int i=1; i < MAX_UISTYLES; i++)
        uiStyles_[i] = GetEditorUIXMLFile(UIStyleXmlFilenames[i]);

    GameContext::Get().ui_->GetRoot()->SetDefaultStyle(uiStyles_[UISTYLE_EDITORDEFAULT]);

    editorUI_ = GameContext::Get().ui_->GetRoot()->CreateChild<UIElement>("EditorUI");
    editorUI_->SetFixedSize(GameContext::Get().ui_->GetRoot()->GetWidth(), GameContext::Get().ui_->GetRoot()->GetHeight());

    xmlFiles_.Resize(MAX_EDITORELEMENTS);
    for (int i = 0; i < MAX_EDITORELEMENTS; i++)
    {
        if (!EditorElementXmlFilenames[i].Empty())
            xmlFiles_[i] = GetEditorUIXMLFile(EditorElementXmlFilenames[i]);
    }

    uiPanels_.Resize(MAX_EDITORPANELS);
    uiPanelDirty_.Resize(MAX_EDITORPANELS);

    CreateSpawnToolBar();

    for (int i = 0; i < MAX_EDITORPANELS; i++)
    {
        SharedPtr<UIElement>& panel = uiPanels_[i];
        if (!EditorPanelXmlFilenames[i].Empty())
            panel = GameContext::Get().ui_->LoadLayout(GetEditorUIXMLFile(EditorPanelXmlFilenames[i]));
        panel->SetVar(PANEL_ID, i);

        editorUI_->AddChild(panel);

        uiPanelDirty_[i] = true;

        SetVisible(i, false);

        if (panel->GetPriority() == 0)
            panel->SetPriority(i+1);

        UIElement* closebutton = panel->GetChild("CloseButton", true);
        if (closebutton)
            SubscribeToEvent(closebutton, E_PRESSED, URHO3D_HANDLER(MapEditorLibImpl, HandleHidePanel));
    }

    uiPanels_[PANEL_ANIMATOR2D_MAIN]->SetVar(DISABLEVIEWRAYCAST, true);

    SetCategoryPopList(SPAWN_MONSTER);
    SetCategoryPopList(SPAWN_PLANT);
    SetCategoryPopList(SPAWN_DOOR);
    SetCategoryPopList(SPAWN_LIGHT);
    SetCategoryPopList(SPAWN_DECORATION);
    SetCategoryPopList(SPAWN_CONTAINER);
    SetCategoryPopList(SPAWN_COLLECTABLE);
    SetCategoryPopList(SPAWN_TOOL);

    ResizeUI();
}

void MapEditorLibImpl::RemoveUI()
{
    for (int i = 0; i < MAX_EDITORPANELS; i++)
    {
        editorUI_->RemoveChild(uiPanels_[i]);
        uiPanels_[i].Reset();
    }

    for (int i = 0; i < MAX_EDITORELEMENTS; i++)
        xmlFiles_[i].Reset();

    for (int i = 1; i < MAX_UISTYLES; i++)
        uiStyles_[i].Reset();

    GameContext::Get().ui_->GetRoot()->RemoveChild(editorUI_);
    GameContext::Get().ui_->GetRoot()->SetDefaultStyle(uiStyles_[UISTYLE_GAMEDEFAULT]);

    uiStyles_.Clear();
    xmlFiles_.Clear();

    editorUI_ = 0;
}

void MapEditorLibImpl::ResizeUI()
{
    int width = GameContext::Get().GetSubsystem<Graphics>()->GetWidth();
    int height = GameContext::Get().GetSubsystem<Graphics>()->GetHeight();

    editorUI_->SetFixedSize(width, height);

    // Resize tool bar
    UIElement* toolbar = uiPanels_[PANEL_MAINTOOLBAR];
    toolbar->SetFixedWidth(width);
    toolbar->SetPosition(0, height - SPAWNBAR_HEIGHT);
    int categoryToPanelOffset = 0;
    for (int spawnCategory=SPAWN_MONSTER; spawnCategory < MAX_SPAWNCATEGORIES; spawnCategory++)
    {
        if (spawnCategory == SPAWN_TILE)
        {
            categoryToPanelOffset = -1;
            continue;
        }

        UIElement* panel = GetPanel(spawnCategory+categoryToPanelOffset);
        IntVector2 posbutton = GetPanel(PANEL_MAINTOOLBAR)->GetChild(0)->GetChild(spawnCategory-1)->GetPosition();
        panel->SetPosition(posbutton.x_, height - SPAWNBAR_HEIGHT - panel->GetSize().y_);
    }

    // Resize inspector
    UIElement* inspector = uiPanels_[PANEL_INSPECTORWINDOW];
    inspector->SetSize(INSPECTOR_WIDTH, height - SPAWNBAR_HEIGHT);
    inspector->SetPosition(width - INSPECTOR_WIDTH, 0);
    inspector->SetOpacity(uiMaxOpacity_);
    inspector->BringToFront();

    URHO3D_LOGINFOF("MapEditorLibImpl() - ResizeUI : width=%d height=%d", width, height);

//    UpdateAttributeInspector();
//    ResizeHierarchyWindow();
}

void MapEditorLibImpl::SetVisible(int panelid, bool visible)
{
    UIElement* panel = uiPanels_[panelid];
    if (visible)
    {
        panel->SetVisible(true);
        panel->BringToFront();
        if (panel->GetVar(DISABLEVIEWRAYCAST).GetBool())
            disableViewRayCast_++;
    }
    else
    {
        panel->SetVisible(false);
        if (panel->GetVar(DISABLEVIEWRAYCAST).GetBool())
            disableViewRayCast_--;
    }
}

UIElement* MapEditorLibImpl::GetPanel(int panelid)
{
    UIElement* panel = editor_->uiPanels_[panelid];
    if (!panel)
        URHO3D_LOGERRORF("No Panel %d", panelid);
    return panel;
}

/// SpawnBar

void CreateToolBarIcon(UIElement* element, int size, int styleicons=UISTYLE_TOOLBARICONS)
{
    BorderImage* icon = new BorderImage(GameContext::Get().context_);
    icon->SetName("Icon");
    icon->SetDefaultStyle(MapEditorLibImpl::GetUIStyle(styleicons));
    icon->SetStyle(element->GetName());
    icon->SetFixedSize(size, size);
    icon->SetAlignment(HA_CENTER, VA_CENTER);
    icon->SetBlendMode(BLEND_ALPHA);
    element->AddChild(icon);
}

void CreateToolTip(UIElement* element, const String& title, const IntVector2& offset, bool l10)
{
    ToolTip* toolTip = new ToolTip(GameContext::Get().context_);
    toolTip->SetName(title + "ToolTip");
    toolTip->SetPosition(offset);
    element->AddChild(toolTip);

    BorderImage* textHolder = new BorderImage(GameContext::Get().context_);
    textHolder->SetName("BorderImage");
    textHolder->SetStyle("ToolTipBorderImage");
    textHolder->SetHorizontalAlignment(HA_CENTER);
    toolTip->AddChild(textHolder);

    Text* toolTipText = new Text(GameContext::Get().context_);
    toolTipText->SetName("Text");
    toolTipText->SetStyle("ToolTipText");
    toolTipText->SetAutoLocalizable(l10);
    toolTipText->SetText(title);
    textHolder->AddChild(toolTipText);
}

Button* CreateToolBarButton(const String& title, int size, int styleicons=UISTYLE_TOOLBARICONS, bool l10=true)
{
    Button* button = new Button(GameContext::Get().context_);
    button->SetName(title);
    button->SetDefaultStyle(MapEditorLibImpl::GetUIStyle(UISTYLE_EDITORDEFAULT));
    button->SetStyleAuto();
    button->SetMinSize(size, size);
    button->SetMaxSize(2147483647, size);
    button->SetLayoutMode(LM_FREE);
    button->SetFocusMode(FM_NOTFOCUSABLE);

    CreateToolBarIcon(button, size, styleicons);
    CreateToolTip(button, title, IntVector2(button->GetWidth()/2, -20), l10);

    return button;
}

CheckBox* CreateToolBarToggle(const String& title, int size, int styleicons=UISTYLE_TOOLBARICONS, bool l10=true)
{
    CheckBox* toggle = new CheckBox(GameContext::Get().context_);
    toggle->SetName(title);
    toggle->SetDefaultStyle(MapEditorLibImpl::GetUIStyle(UISTYLE_EDITORDEFAULT));
    toggle->SetStyle("ToolBarToggle");

    CreateToolBarIcon(toggle, size, styleicons);
    CreateToolTip(toggle, title, IntVector2(toggle->GetWidth()/2, -20), l10);

    return toggle;
}

UIElement* CreateGroup(const String& title, LayoutMode layoutMode)
{
    UIElement* group = new UIElement(GameContext::Get().context_);
    group->SetName(title);
    group->SetDefaultStyle(MapEditorLibImpl::GetUIStyle(UISTYLE_EDITORDEFAULT));
    group->SetLayoutMode(layoutMode);
    return group;
}

UIElement* CreateToolBarSpacer(unsigned width)
{
    UIElement* spacer = new UIElement(GameContext::Get().context_);
    spacer->SetFixedWidth(width);
    return spacer;
}

DropDownList* CreateDropDownList(const String& title, const Vector<String>& listElements, const IntVector2& size, int defaultSelection)
{
    DropDownList* dropdownlist = new DropDownList(GameContext::Get().context_);
    dropdownlist->SetName(title);
    dropdownlist->SetDefaultStyle(MapEditorLibImpl::GetUIStyle(UISTYLE_EDITORDEFAULT));
    dropdownlist->SetStyle("DropDownList");
    // ddl must be LM_free, otherwise the placeholder will not be centered as expected.
    dropdownlist->SetLayout(LM_FREE);
    dropdownlist->SetVerticalAlignment(VA_CENTER);
    dropdownlist->SetResizePopup(true);
    dropdownlist->GetPopup()->GetChild(0)->SetHorizontalAlignment(HA_CENTER);
    dropdownlist->SetMinSize(size);
    dropdownlist->SetMaxSize(size);
    // set the list items
    for (unsigned i=0; i < listElements.Size(); i++)
    {
        Text* text = new Text(GameContext::Get().context_);
        text->SetStyle("Text");
        text->SetText(listElements[i]);
        text->SetHorizontalAlignment(HA_CENTER);
        text->SetTextAlignment(HA_CENTER);
        dropdownlist->AddItem(text);
    }
    // set default selection
    dropdownlist->SetSelection(defaultSelection);

    // set the placeholder
    dropdownlist->SetPlaceholderEditable(false);
    // center the placeholder
    dropdownlist->GetPlaceholder()->SetVerticalAlignment(VA_CENTER);
    Text* holdertext = static_cast<Text*>(dropdownlist->GetPlaceholder()->GetChild(0));
    holdertext->SetTextAlignment(HA_CENTER);

    // after setting manually the selection, resize place holder to correctly show it
    // put that in the engine ?
    UIElement* selectedItem = dropdownlist->GetSelectedItem();
    if (selectedItem)
        dropdownlist->GetPlaceholder()->SetSize(selectedItem->GetSize());
    // Be sure to have prepare the batch of the popup, because PlaceHolder copy it
    // it's necessary for the obtention of the good alignment of placeholder text
    dropdownlist->ShowPopup(true);
    dropdownlist->ShowPopup(false);

    return dropdownlist;
}

void FinalizeGroupHorizontal(UIElement* group, const String& baseStyle)
{
    unsigned numchildren =  group->GetNumChildren();

    for (unsigned i = 0; i < numchildren; ++i)
    {
        UIElement* child = group->GetChild(i);

        if (i == 0 && i < numchildren - 1)
            child->SetStyle(baseStyle + "GroupLeft");
        else if (i < numchildren - 1)
            child->SetStyle(baseStyle + "GroupMiddle");
        else
            child->SetStyle(baseStyle + "GroupRight");
    }

    group->SetMaxSize(group->GetSize());
}

void MapEditorLibImpl::CreateSpawnToolBar()
{
    uiPanels_[PANEL_MAINTOOLBAR] = new BorderImage(GameContext::Get().context_);

    UIElement* toolbar = uiPanels_[PANEL_MAINTOOLBAR];

    toolbar->SetName("ToolBar");
    toolbar->SetStyle("EditorToolBar");
    toolbar->SetLayout(LM_HORIZONTAL);
    toolbar->SetLayoutSpacing(4);
    toolbar->SetLayoutBorder(IntRect(8, 4, 4, 8));
    toolbar->SetOpacity(uiMaxOpacity_);
    toolbar->SetFixedSize(GameContext::Get().ui_->GetRoot()->GetWidth(), 68);
    toolbar->SetPosition(0, 0);

    UIElement* spawngroup = CreateGroup("SpawnGroup", LM_HORIZONTAL);
    for (int i = 1; i < MAX_SPAWNCATEGORIES; i++)
        spawngroup->AddChild(CreateToolBarToggle(SpawnCategoryNameStr[i], 60));

    FinalizeGroupHorizontal(spawngroup, "ToolBarToggle");
    toolbar->AddChild(spawngroup);

    // Add Spawn Alignment DropDownList
    Vector<String> layerAlignements;
    layerAlignements.Push("Z Back");
    layerAlignements.Push("Z Middle");
    layerAlignements.Push("Z Front");
    DropDownList* layerAlignDdl = CreateDropDownList("LayerAlign", layerAlignements, IntVector2(105,60), editorLayerModifier_);
    toolbar->AddChild(layerAlignDdl);
}

UIElement* AddListButton(const String& name, Sprite2D* sprite, bool addpivot=false, int defaultstyleid=0, const String& defaulticon = String::EMPTY)
{
    Button* button = new Button(GameContext::Get().context_);
    button->SetDefaultStyle(MapEditorLibImpl::GetUIStyle(UISTYLE_EDITORDEFAULT));
    button->SetStyleAuto();
    button->SetMinSize(64, 64);
    button->SetMaxSize(64, 64);

    BorderImage* spritebtn = new BorderImage(GameContext::Get().context_);
    spritebtn->SetMinSize(60, 60);
    spritebtn->SetMaxSize(60, 60);
    spritebtn->SetAlignment(HA_CENTER, VA_CENTER);
    if (sprite)
    {
        spritebtn->SetTexture(sprite->GetTexture());
        spritebtn->SetImageRect(sprite->GetRectangle());
    }
    else
    {
        spritebtn->SetDefaultStyle(MapEditorLibImpl::GetUIStyle(defaultstyleid));
        spritebtn->SetStyle(defaulticon);
    }

    button->AddChild(spritebtn);

    if (addpivot)
    {
        BorderImage* pivot = new BorderImage(GameContext::Get().context_);
        pivot->SetName("Pivot");
        pivot->SetDefaultStyle(MapEditorLibImpl::GetUIStyle(UISTYLE_ICONS));
        pivot->SetStyle("AnimPivotPoint");
        pivot->SetMinSize(16, 16);
        pivot->SetMaxSize(16, 16);
        pivot->SetPosition(30, 30);
        pivot->SetPivot(0.5f, 0.5f);
        pivot->SetVisible(false);
        spritebtn->AddChild(pivot);
    }

    Text* text = new Text(GameContext::Get().context_);
    text->SetDefaultStyle(MapEditorLibImpl::GetUIStyle(UISTYLE_EDITORDEFAULT));
    text->SetStyleAuto();
    text->SetAlignment(HA_CENTER, VA_TOP);
    text->SetFontSize(7);
    text->SetTextEffect(TE_SHADOW);
    if (!name.Empty())
        text->SetText(name);
    button->AddChild(text);

    return button;
}

void MapEditorLibImpl::SetCategoryPopList(int spawnCategory)
{
    int panelid = 0;
    StringHash cot;
    EventHandlerImpl<MapEditorLibImpl>::HandlerFunctionPtr funcptr = 0;

    if (spawnCategory == SPAWN_MONSTER)
    {
        panelid = PANEL_MONSTERPOPLIST;
        cot = COT::MONSTERS;
        funcptr = &MapEditorLibImpl::HandleCategoryTypeSelected;
    }
    else if (spawnCategory == SPAWN_PLANT)
    {
        panelid = PANEL_PLANTPOPLIST;
        cot = COT::PLANTS;
        funcptr = &MapEditorLibImpl::HandleCategoryTypeSelected;
    }
    else if (spawnCategory == SPAWN_DOOR)
    {
        panelid = PANEL_DOORPOPLIST;
        cot = COT::DOORS;
        funcptr = &MapEditorLibImpl::HandleCategoryTypeSelected;
    }
    else if (spawnCategory == SPAWN_LIGHT)
    {
        panelid = PANEL_LIGHTPOPLIST;
        cot = COT::LIGHTS;
        funcptr = &MapEditorLibImpl::HandleCategoryTypeSelected;
    }
    else if (spawnCategory == SPAWN_DECORATION)
    {
        panelid = PANEL_DECORATIONPOPLIST;
        cot = COT::DECORATIONS;
        funcptr = &MapEditorLibImpl::HandleCategoryTypeSelected;
    }
    else if (spawnCategory == SPAWN_CONTAINER)
    {
        panelid = PANEL_CONTAINERPOPLIST;
        cot = COT::CONTAINERS;
        funcptr = &MapEditorLibImpl::HandleCategoryTypeSelected;
    }
    else if (spawnCategory == SPAWN_COLLECTABLE)
    {
        panelid = PANEL_COLLECTABLEPOPLIST;
        cot = COT::ITEMS;
        funcptr = &MapEditorLibImpl::HandleCategoryTypeSelected;
    }
    else if (spawnCategory == SPAWN_TOOL)
    {
        panelid = PANEL_TOOLPOPLIST;
        cot = COT::TOOLS;
        funcptr = &MapEditorLibImpl::HandleCategoryTypeSelected;
    }

    int defaultstyleid = UISTYLE_TOOLBARICONS;
    const String& defaulticon = SpawnCategoryNameStr[spawnCategory];
    UIElement* panel = GetPanel(panelid);
    IntVector2 posbutton = GetPanel(PANEL_MAINTOOLBAR)->GetChild(0)->GetChild(spawnCategory-1)->GetPosition();
    panel->SetPosition(posbutton.x_, GameContext::Get().GetSubsystem<Graphics>()->GetHeight() - SPAWNBAR_HEIGHT - panel->GetSize().y_);

    ListView* spritesList = static_cast<ListView*>(panel->GetChild("SpritesList", true));
    spritesList->RemoveAllItems();

    const Vector<StringHash>& types = COT::GetTypesInCategory(cot);

    const unsigned numSpritesByRow = 4;
    const unsigned numSprites = types.Size();
    unsigned numRows = numSprites / numSpritesByRow;
    const int numSpritesLastRow = numSprites - numRows * numSpritesByRow;

    PODVector<UIElement*> buttons;

    unsigned index = 0;

    // set sprites row
    for (unsigned i = 0; i < numRows; i++)
    {
        UIElement* row = new UIElement(context_);
        row->SetLayoutMode(LM_HORIZONTAL);
        row->SetLayoutSpacing(4);
        row->SetHorizontalAlignment(HA_CENTER);
        for (unsigned j = 0; j < numSpritesByRow; j++)
        {
            StringHash hash = types[index];
            const String& name = GOT::GetType(hash);
            Sprite2D* sprite = GameContext::Get().spriteSheets_[UIEQUIPMENT]->GetSprite(name);
            if (!sprite)
            {
                Node* objectnode = GOT::GetObject(hash);
                StaticSprite2D* ssprite = objectnode ? objectnode->GetDerivedComponent<StaticSprite2D>() : 0;
                sprite = ssprite ? ssprite->GetSprite() : 0;
            }
            UIElement* elt = AddListButton(name, sprite, false, defaultstyleid, defaulticon);
            buttons.Push(elt);
            row->AddChild(elt);
            index++;
        }
        spritesList->AddItem(row);
    }
    // set sprites last row
    if (numSpritesLastRow)
    {
        UIElement* row = new UIElement(context_);
        row->SetLayoutMode(LM_HORIZONTAL);
        row->SetLayoutSpacing(4);
        row->SetHorizontalAlignment(HA_CENTER);
        for (unsigned i = 0; i < numSpritesLastRow; i++)
        {
            StringHash hash = types[index];
            const String& name = GOT::GetType(hash);
            Sprite2D* sprite = GameContext::Get().spriteSheets_[UIEQUIPMENT]->GetSprite(name);
            if (!sprite)
            {
                Node* objectnode = GOT::GetObject(hash);
                StaticSprite2D* ssprite = objectnode ? objectnode->GetDerivedComponent<StaticSprite2D>() : 0;
                sprite = ssprite ? ssprite->GetSprite() : 0;
            }
            UIElement* elt = AddListButton(name, sprite, false, defaultstyleid, defaulticon);
            buttons.Push(elt);
            row->AddChild(elt);
            index++;
        }
        spritesList->AddItem(row);
    }

    // subscribe To Event
    if (funcptr)
    {
        for (unsigned i=0; i < buttons.Size(); i++)
        {
            buttons[i]->SetVar(TYPE_VAR, types[i]);
            SubscribeToEvent(buttons[i], E_PRESSED, new Urho3D::EventHandlerImpl<MapEditorLibImpl>(this, funcptr));
        }
    }
}


/// Inspector

int GetType(Serializable* serializable)
{
    if (static_cast<Node*>(serializable))
        return ITEM_NODE;
    else if (static_cast<Component*>(serializable))
        return ITEM_COMPONENT;
    else
        return ITEM_NONE;
}

/// Get back the human-readable variable name from the StringHash.
String GetVarName(StringHash hash)
{
    // First try to get it from scene
    String name = GameContext::Get().rootScene_->GetVarName(hash);
    // Then from the user variable reverse mappings
    if (name.Empty())
        name = GameContext::Get().context_->GetUserAttributeName(hash);
    return name;
}

String GetComponentTitle(Component* component)
{
    String ret = component->GetTypeName();
    if (inspectorShowID)
    {
        if (component->GetID() >= FIRST_LOCAL_ID)
            ret += " (Local)";

        if (component->IsTemporary())
            ret += " (Temp)";
    }
    return ret;
}

unsigned GetAttributeEditorCount(const Vector<Serializable*>& serializables)
{
    unsigned count = 0;

    if (!serializables.Empty())
    {
        /// \todo When multi-editing, this only counts the editor count of the first serializable
        bool isUIElement = static_cast<UIElement*>(serializables.Front());
        for (unsigned i = 0; i < serializables.Front()->GetNumAttributes(); ++i)
        {
            const AttributeInfo& info = serializables.Front()->GetAttributes()->At(i);
            if (!inspectorShowNonEditableAttribute && (info.mode_ & AM_NOEDIT) != 0)
                continue;
            // "Is Enabled" is not inserted into the main attribute list, so do not count
            // Similarly, for UIElement, "Is Visible" is not inserted
            if (info.name_ == (isUIElement ? "Is Visible" : "Is Enabled"))
                continue;
            // Tags are also handled separately
            if (info.name_ == "Tags")
                continue;
            if (info.type_ == VAR_RESOURCEREFLIST)
                count += serializables.Front()->GetAttribute(i).GetResourceRefList().names_.Size();
//            else if (info.type == VAR_VARIANTVECTOR && GetVectorStruct(serializables, i) !is null)
//                count += serializables.Front()->GetAttribute(i).GetVariantVector().Size();
            else if (info.type_ == VAR_VARIANTMAP)
                count += serializables.Front()->GetAttribute(i).GetVariantMap().Size();
            else
                ++count;
        }
    }

    return count;
}

UIElement* GetAttributeEditorParent(UIElement* parent, unsigned index, unsigned subIndex)
{
    return parent->GetChild("Edit" + String(index) + "_" + String(subIndex), true);
}

/// Store the IDs of the actual serializable objects into user-defined variable of the 'attribute editor' (e.g. line-edit, drop-down-list, etc).
void SetAttributeEditorID(UIElement* elt, const Vector<Serializable*>& serializables)
{
    if (!serializables.Size())
        return;

    // All target serializables must be either nodes, ui-elements, or components
    Vector<Variant> ids;
    Variant var;
    switch (GetType(serializables.Front()))
    {
    case ITEM_NODE:
        for (unsigned i = 0; i < serializables.Size(); ++i)
            ids.Push(static_cast<Node*>(serializables[i])->GetID());
        elt->SetVar(NODE_IDS_VAR, ids);
        break;

    case ITEM_COMPONENT:
        for (unsigned i = 0; i < serializables.Size(); ++i)
            ids.Push(static_cast<Component*>(serializables[i])->GetID());
        elt->SetVar(COMPONENT_IDS_VAR, ids);
        break;

    default:
        for (unsigned i = 0; i < serializables.Size(); ++i)
        {
            var = serializables[i];
            ids.Push(var);
        }
        elt->SetVar(SERIALIZABLE_PTRS_VAR, ids);
        break;
    }
}

/// Return the actual serializable objects based on the IDs stored in the user-defined variable of the 'attribute editor'.
void GetAttributeEditorTargets(UIElement* attrEdit, Vector<Serializable*>& serializables)
{
    if (!attrEdit->GetVar(NODE_IDS_VAR).IsEmpty())
    {
        const Vector<Variant>& ids = attrEdit->GetVar(NODE_IDS_VAR).GetVariantVector();
        for (unsigned i = 0; i < ids.Size(); ++i)
        {
            Node* node = GameContext::Get().rootScene_->GetNode(ids[i].GetUInt());
            if (node)
                serializables.Push(node);
        }
    }
    else if (!attrEdit->GetVar(COMPONENT_IDS_VAR).IsEmpty())
    {
        const Vector<Variant>& ids = attrEdit->GetVar(COMPONENT_IDS_VAR).GetVariantVector();
        for (unsigned i = 0; i < ids.Size(); ++i)
        {
            Component* component = GameContext::Get().rootScene_->GetComponent(ids[i].GetUInt());
            if (component)
                serializables.Push(component);
        }
    }
//    else if (!attrEdit->GetVar(UI_ELEMENT_ID_VAR).Empty())
//    {
//        const Vector<Variant>& ids = attrEdit->GetVar(UI_ELEMENT_ID_VAR).GetVariantVector();
//        for (unsigned i = 0; i < ids.Size(); ++i)
//        {
//            UIElement* element = editorUIElement->GetChild(UI_ELEMENT_ID_VAR, ids[i], true);
//            if (element)
//                ret.Push(element);
//        }
//    }

}

void IconizeUIElement(UIElement* element, const String& iconType)
{
    // Check if the icon has been created before
    BorderImage* icon = static_cast<BorderImage*>(element->GetChild(ICON));

    // If iconType is empty, it is a request to remove the existing icon
    if (iconType.Empty())
    {
        // Remove the icon if it exists
        if (icon)
            icon->Remove();

        // Revert back the indent but only if it is indented by this function
        if (element->GetVar(INDENT_MODIFIED_BY_ICON_VAR).GetBool())
            element->SetIndent(0);

        return;
    }

    // The UI element must itself has been indented to reserve the space for the icon
    if (element->GetIndent() == 0)
    {
        element->SetIndent(1);
        element->SetVar(INDENT_MODIFIED_BY_ICON_VAR, true);
    }

    // If no icon yet then create one with the correct indent and size in respect to the UI element
    if (!icon)
    {
        // The icon is placed at one indent level less than the UI element
        icon = new BorderImage(element->GetContext());
        icon->SetName(ICON);
        icon->SetIndent(element->GetIndent() - 1);
        icon->SetFixedSize(element->GetIndentWidth() - 2, 14);
        element->InsertChild(0, icon);   // Ensure icon is added as the first child
    }

    // Set the icon type
    if (!icon->SetStyle(iconType, MapEditorLibImpl::GetUIStyle(UISTYLE_ICONS)))
        icon->SetStyle(UNKNOWN, MapEditorLibImpl::GetUIStyle(UISTYLE_ICONS));    // If fails then use an 'unknown' icon type
    icon->SetColor(Color(1,1,1,1)); // Reset to enabled color
}

void SetIconEnabledColor(UIElement* element, bool enabled, bool partial = false)
{
    BorderImage* icon = static_cast<BorderImage*>(element->GetChild(ICON));
    if (icon)
    {
        if (partial)
        {
            icon->SetColor(C_TOPLEFT, Color(1,1,1,1));
            icon->SetColor(C_BOTTOMLEFT, Color(1,1,1,1));
            icon->SetColor(C_TOPRIGHT, Color(1,0,0,1));
            icon->SetColor(C_BOTTOMRIGHT, Color(1,0,0,1));
        }
        else
        {
            icon->SetColor(enabled ? Color(1,1,1,1) : Color(1,0,0,1));
        }
    }
}

UIElement* SetEditable(UIElement* element, bool editable)
{
    if (!element)
        return element;

    editable = true;

    element->SetEditable(editable);
    element->SetColor(C_TOPLEFT, editable ? element->GetColor(C_BOTTOMRIGHT) : nonEditableTextColor);
    element->SetColor(C_BOTTOMLEFT, element->GetColor(C_TOPLEFT));
    element->SetColor(C_TOPRIGHT, element->GetColor(C_TOPLEFT));

    return element;
}

UIElement* SetValue(UIElement* element, const String& value, bool sameValue)
{
    LineEdit* elt = static_cast<LineEdit*>(element);
    if (!elt)
        return 0;

    elt->SetText(sameValue ? value : STRIKED_OUT);
    elt->SetCursorPosition(0);

    return element;
}

UIElement* SetValue(UIElement* element, bool value, bool sameValue)
{
    CheckBox* elt = static_cast<CheckBox*>(element);
    if (!elt)
        return 0;

    elt->SetChecked(sameValue ? value : false);
    return element;
}

UIElement* SetValue(UIElement* element, int value, bool sameValue)
{
    DropDownList* elt = static_cast<DropDownList*>(element);
    if (!elt)
        return 0;

    elt->SetSelection(sameValue ? value : M_MAX_UNSIGNED);
    return elt;
}

void CreateDragSlider(LineEdit* parent)
{
    Button* dragSld = new Button(parent->GetContext());
    dragSld->SetStyle("EditorDragSlider");
    dragSld->SetFixedHeight(ATTR_HEIGHT - 3);
    dragSld->SetFixedWidth(dragSld->GetHeight());
    dragSld->SetAlignment(HA_RIGHT, VA_TOP);
    dragSld->SetFocusMode(FM_NOTFOCUSABLE);
    parent->AddChild(dragSld);

//    SubscribeToEvent(dragSld, "DragBegin", "LineDragBegin");
//    SubscribeToEvent(dragSld, "DragMove", "LineDragMove");
//    SubscribeToEvent(dragSld, "DragEnd", "LineDragEnd");
//    SubscribeToEvent(dragSld, "DragCancel", "LineDragCancel");
}

UIElement* CreateAttributeEditorParentWithSeparatedLabel(ListView* list, const String& name, unsigned index, unsigned subIndex, bool suppressedSeparatedLabel = false)
{
    UIElement* editorParent = new UIElement(list->GetContext());
    editorParent->SetName("Edit" + String(index) + "_" + String(subIndex));
    editorParent->SetVar("Index", index);
    editorParent->SetVar("SubIndex", subIndex);
    editorParent->SetLayout(LM_VERTICAL, 2);
    list->AddItem(editorParent);

    if (suppressedSeparatedLabel)
    {
        UIElement* placeHolder = new UIElement(list->GetContext());
        placeHolder->SetName(name);
        editorParent->AddChild(placeHolder);
    }
    else
    {
        Text* attrNameText = new Text(list->GetContext());
        editorParent->AddChild(attrNameText);
        attrNameText->SetStyle("EditorAttributeText");
        attrNameText->SetText(name);
    }

    return editorParent;
}

UIElement* CreateAttributeEditorParentAsListChild(ListView* list, const String& name, unsigned index, unsigned subIndex)
{
    UIElement* editorParent = new UIElement(list->GetContext());
    editorParent->SetName("Edit" + String(index) + "_" + String(subIndex));
    editorParent->SetVar("Index", index);
    editorParent->SetVar("SubIndex", subIndex);
    editorParent->SetLayout(LM_HORIZONTAL);
    list->AddChild(editorParent);

    UIElement* placeHolder = new UIElement(list->GetContext());
    placeHolder->SetName(name);
    editorParent->AddChild(placeHolder);

//    URHO3D_LOGINFOF(" CreateAttributeEditorParentAsListChild ... indexes=%d %d name=%s ...", index, subIndex, name.CString());
    return editorParent;
}

UIElement* CreateAttributeEditorParent(ListView* list, const String& name, unsigned index, unsigned subIndex)
{
    UIElement* editorParent = new UIElement(list->GetContext());
    editorParent->SetName("Edit" + String(index) + "_" + String(subIndex));
    editorParent->SetVar("Index", index);
    editorParent->SetVar("SubIndex", subIndex);
    editorParent->SetLayout(LM_HORIZONTAL);
    editorParent->SetFixedHeight(ATTR_HEIGHT);
    list->AddItem(editorParent);

    Text* attrNameText = new Text(list->GetContext());
    editorParent->AddChild(attrNameText);
    attrNameText->SetStyle("EditorAttributeText");
    attrNameText->SetText(name);
    attrNameText->SetFixedWidth(ATTRNAME_WIDTH);

//    URHO3D_LOGINFOF(" CreateAttributeEditorParent ... indexes=%d %d name=%s ...", index, subIndex, name.CString());
    return editorParent;
}

LineEdit* CreateAttributeLineEdit(UIElement* parent, const Vector<Serializable*>& serializables, unsigned index, unsigned subIndex)
{
    LineEdit* attrEdit = new LineEdit(parent->GetContext());
    parent->AddChild(attrEdit);
    attrEdit->SetDragDropMode(DD_TARGET);
    attrEdit->SetStyle("EditorAttributeEdit");
    attrEdit->SetFixedHeight(ATTR_HEIGHT - 2);
    attrEdit->SetVar("Index", index);
    attrEdit->SetVar("SubIndex", subIndex);
    SetAttributeEditorID(attrEdit, serializables);

    return attrEdit;
}

UIElement* CreateStringAttributeEditor(ListView* list, const Vector<Serializable*>& serializables, const AttributeInfo& info, unsigned index, unsigned subIndex)
{
    UIElement* parent = CreateAttributeEditorParent(list, info.name_, index, subIndex);
    LineEdit* attrEdit = CreateAttributeLineEdit(parent, serializables, index, subIndex);
    attrEdit->SetDragDropMode(DD_TARGET);
    // Do not subscribe to continuous edits of certain attributes (script class names) to prevent unnecessary errors getting printed
//    if (noTextChangedAttrs.Find(info.name_) == -1)
//        SubscribeToEvent(attrEdit, "TextChanged", "EditAttribute");

    MapEditorLibImpl::Get()->SubscribeToEvent(attrEdit, E_TEXTFINISHED, new Urho3D::EventHandlerImpl<MapEditorLibImpl>(MapEditorLibImpl::Get(), &MapEditorLibImpl::HandleInspectorEditAttribute));

    return parent;
}

UIElement* CreateBoolAttributeEditor(ListView* list, const Vector<Serializable*>& serializables, const AttributeInfo& info, unsigned index, unsigned subIndex)
{
    bool isUIElement = static_cast<UIElement*>(serializables.Front()) != 0;
    UIElement* parent;
    if (info.name_ == (isUIElement ? "Is Visible" : "Is Enabled"))
        parent = CreateAttributeEditorParentAsListChild(list, info.name_, index, subIndex);
    else
        parent = CreateAttributeEditorParent(list, info.name_, index, subIndex);

    CheckBox* attrEdit = new CheckBox(list->GetContext());
    parent->AddChild(attrEdit);
    attrEdit->SetStyle("CheckBox");
    attrEdit->SetVar("Index", index);
    attrEdit->SetVar("SubIndex", subIndex);
    SetAttributeEditorID(attrEdit, serializables);

    MapEditorLibImpl::Get()->SubscribeToEvent(attrEdit, E_TOGGLED, new Urho3D::EventHandlerImpl<MapEditorLibImpl>(MapEditorLibImpl::Get(), &MapEditorLibImpl::HandleInspectorEditAttribute));

//    URHO3D_LOGINFOF(" CreateBoolAttributeEditor ... indexes=%d %d name=%s ...", index, subIndex, info.name_.CString());

    return parent;
}

UIElement* CreateNumAttributeEditor(ListView* list, const Vector<Serializable*>& serializables, const AttributeInfo& info, unsigned index, unsigned subIndex)
{
    UIElement* parent = CreateAttributeEditorParent(list, info.name_, index, subIndex);
    VariantType type = info.type_;
    unsigned numCoords = 1;
    if (type == VAR_VECTOR2 || type == VAR_INTVECTOR2)
        numCoords = 2;
    if (type == VAR_VECTOR3 || type == VAR_QUATERNION)
        numCoords = 3;
    else if (type == VAR_VECTOR4 || type == VAR_COLOR || type == VAR_INTRECT || type == VAR_RECT)
        numCoords = 4;

    for (unsigned i = 0; i < numCoords; ++i)
    {
        LineEdit* attrEdit = CreateAttributeLineEdit(parent, serializables, index, subIndex);
        attrEdit->SetVar("Coordinate", i);

        CreateDragSlider(attrEdit);

        MapEditorLibImpl::Get()->SubscribeToEvent(attrEdit, E_TEXTCHANGED, new Urho3D::EventHandlerImpl<MapEditorLibImpl>(MapEditorLibImpl::Get(), &MapEditorLibImpl::HandleInspectorEditAttribute));
        MapEditorLibImpl::Get()->SubscribeToEvent(attrEdit, E_TEXTFINISHED, new Urho3D::EventHandlerImpl<MapEditorLibImpl>(MapEditorLibImpl::Get(), &MapEditorLibImpl::HandleInspectorEditAttribute));
    }

    return parent;
}

UIElement* CreateIntAttributeEditor(ListView* list, const Vector<Serializable*>& serializables, const AttributeInfo& info, unsigned index, unsigned subIndex)
{
    UIElement* parent = CreateAttributeEditorParent(list, info.name_, index, subIndex);

    // Check for masks and layers
//    if (bitSelectionAttrs.Find(info.name_) > -1)
//    {
//        LineEdit* attrEdit = CreateAttributeBitSelector(parent, serializables, index, subIndex);
//    }
    // Check for enums
//    else if (info.enumNames_ == 0 || *info.enumNames_ == 0)
    if (info.enumNames_ == 0 || *info.enumNames_ == 0)
    {
        // No enums, create a numeric editor
        LineEdit* attrEdit = CreateAttributeLineEdit(parent, serializables, index, subIndex);
        CreateDragSlider(attrEdit);
        // If the attribute is a counter for things like billboards or animation states, disable apply at each change
//        if (info.name_.Find(" Count", 0, false) == NPOS)
//            SubscribeToEvent(attrEdit, "TextChanged", "EditAttribute");
        MapEditorLibImpl::Get()->SubscribeToEvent(attrEdit, E_TEXTFINISHED, new Urho3D::EventHandlerImpl<MapEditorLibImpl>(MapEditorLibImpl::Get(), &MapEditorLibImpl::HandleInspectorEditAttribute));
        // If the attribute is a node ID, make it a drag/drop target
        if (info.name_.Contains("NodeID", false) || info.name_.Contains("Node ID", false) || (info.mode_ & AM_NODEID) != 0)
            attrEdit->SetDragDropMode(DD_TARGET);
    }
    else
    {
        DropDownList* attrEdit = new DropDownList(list->GetContext());
        parent->AddChild(attrEdit);
        attrEdit->SetStyleAuto();
        attrEdit->SetFixedHeight(ATTR_HEIGHT - 2);
        attrEdit->SetResizePopup(true);
        attrEdit->SetPlaceholderText(STRIKED_OUT);
        attrEdit->SetVar("Index", index);
        attrEdit->SetVar("SubIndex", subIndex);
        attrEdit->SetLayout(LM_HORIZONTAL, 0, IntRect(4, 1, 4, 1));
        SetAttributeEditorID(attrEdit, serializables);

        const char ** enumPtr = info.enumNames_;
        while (*enumPtr)
        {
            Text* choice = new Text(list->GetContext());
            attrEdit->AddItem(choice);
            choice->SetStyle("EditorEnumAttributeText");
            choice->SetText(*enumPtr);
            ++enumPtr;
        }

        MapEditorLibImpl::Get()->SubscribeToEvent(attrEdit, E_ITEMSELECTED, new Urho3D::EventHandlerImpl<MapEditorLibImpl>(MapEditorLibImpl::Get(), &MapEditorLibImpl::HandleInspectorEditAttribute));
    }

    return parent;
}

Button* CreateResourcePickerButton(UIElement* container, const Vector<Serializable*>& serializables, unsigned index, unsigned subIndex, const String& text)
{
    Button* button = new Button(container->GetContext());
    container->AddChild(button);
    button->SetStyleAuto();
    button->SetFixedSize(36, ATTR_HEIGHT - 2);
    button->SetVar("Index", index);
    button->SetVar("SubIndex", subIndex);
    SetAttributeEditorID(button, serializables);

    Text* buttonText = new Text(container->GetContext());
    button->AddChild(buttonText);
    buttonText->SetStyle("EditorAttributeText");
    buttonText->SetAlignment(HA_CENTER, VA_CENTER);
    buttonText->SetAutoLocalizable(true);
    buttonText->SetText(text);

    return button;
}

UIElement* CreateResourceRefAttributeEditor(ListView* list, const Vector<Serializable*>& serializables, const AttributeInfo& info, unsigned index, unsigned subIndex, bool suppressedSeparatedLabel = false)
{
    UIElement* parent;
    StringHash resourceType;

    // Get the real attribute info from the serializable for the correct resource type
    const AttributeInfo& attrInfo = serializables.Front()->GetAttributes()->At(index);
    if (attrInfo.type_ == VAR_RESOURCEREF)
        resourceType = serializables.Front()->GetAttribute(index).GetResourceRef().type_;
    else if (attrInfo.type_ == VAR_RESOURCEREFLIST)
        resourceType = serializables.Front()->GetAttribute(index).GetResourceRefList().type_;
    else if (attrInfo.type_ == VAR_VARIANTVECTOR)
        resourceType = serializables.Front()->GetAttribute(index).GetVariantVector()[subIndex].GetResourceRef().type_;

//    URHO3D_LOGINFOF(" CreateResourceRefAttributeEditor ... attribute[%u] rscType=%s(%u)...", index, GameContext::Get().context_->GetTypeName(resourceType).CString(), resourceType.Value());

    // Create the attribute name on a separate non-interactive line to allow for more space
    parent = CreateAttributeEditorParentWithSeparatedLabel(list, info.name_, index, subIndex, suppressedSeparatedLabel);

    UIElement* container = new UIElement(list->GetContext());
    container->SetLayout(LM_HORIZONTAL, 4, IntRect(info.name_.StartsWith("   ") ? 20 : 10, 0, 4, 0));    // Left margin is indented more when the name is so
    container->SetFixedHeight(ATTR_HEIGHT);
    parent->AddChild(container);

    LineEdit* attrEdit = CreateAttributeLineEdit(container, serializables, index, subIndex);
    attrEdit->SetVar(TYPE_VAR, resourceType.Value());
    MapEditorLibImpl::Get()->SubscribeToEvent(attrEdit, E_TEXTFINISHED, new Urho3D::EventHandlerImpl<MapEditorLibImpl>(MapEditorLibImpl::Get(), &MapEditorLibImpl::HandleInspectorEditAttribute));

    const ResourcePicker* picker = ResourcePicker::Get(resourceType);
    if (picker)
    {
        if ((picker->actions_ & ACTION_PICK) != 0)
        {
            Button* pickButton = CreateResourcePickerButton(container, serializables, index, subIndex, "editor_pick");
            MapEditorLibImpl::Get()->SubscribeToEvent(pickButton, E_RELEASED, new Urho3D::EventHandlerImpl<MapEditorLibImpl>(MapEditorLibImpl::Get(), &MapEditorLibImpl::HandleInspectorPickResource));
        }
        if ((picker->actions_ & ACTION_OPEN) != 0)
        {
            Button* openButton = CreateResourcePickerButton(container, serializables, index, subIndex, "editor_open");
            MapEditorLibImpl::Get()->SubscribeToEvent(openButton, E_RELEASED, new Urho3D::EventHandlerImpl<MapEditorLibImpl>(MapEditorLibImpl::Get(), &MapEditorLibImpl::HandleInspectorOpenResource));
        }
        if ((picker->actions_ & ACTION_EDIT) != 0)
        {
            Button* editButton = CreateResourcePickerButton(container, serializables, index, subIndex, "editor_edit");
            MapEditorLibImpl::Get()->SubscribeToEvent(editButton, E_RELEASED, new Urho3D::EventHandlerImpl<MapEditorLibImpl>(MapEditorLibImpl::Get(), &MapEditorLibImpl::HandleInspectorEditResource));
        }
        if ((picker->actions_ & ACTION_TEST) != 0)
        {
            Button* testButton = CreateResourcePickerButton(container, serializables, index, subIndex, "editor_test");
            MapEditorLibImpl::Get()->SubscribeToEvent(testButton, E_RELEASED, new Urho3D::EventHandlerImpl<MapEditorLibImpl>(MapEditorLibImpl::Get(), &MapEditorLibImpl::HandleInspectorTestResource));
        }
    }

    return parent;
}

//unsigned nestedSubIndex_;
//Vector<Variant>* nestedVector_;

UIElement* CreateAttributeEditor(ListView* list, const Vector<Serializable*>& serializables, const AttributeInfo& info, unsigned index, unsigned subIndex, bool suppressedSeparatedLabel = false)
{
    UIElement* parent = 0;

    VariantType type = info.type_;
    if (type == VAR_STRING || type == VAR_BUFFER)
        parent = CreateStringAttributeEditor(list, serializables, info, index, subIndex);
    else if (type == VAR_BOOL)
        parent = CreateBoolAttributeEditor(list, serializables, info, index, subIndex);
    else if ((type >= VAR_FLOAT && type <= VAR_VECTOR4) || type == VAR_QUATERNION || type == VAR_COLOR || type == VAR_INTVECTOR2 || type == VAR_INTRECT || type == VAR_DOUBLE || type == VAR_RECT)
        parent = CreateNumAttributeEditor(list, serializables, info, index, subIndex);
    else if (type == VAR_INT)
        parent = CreateIntAttributeEditor(list, serializables, info, index, subIndex);
    else if (type == VAR_RESOURCEREF)
        parent = CreateResourceRefAttributeEditor(list, serializables, info, index, subIndex, suppressedSeparatedLabel);
    else if (type == VAR_RESOURCEREFLIST)
    {
        unsigned numRefs = serializables.Front()->GetAttribute(index).GetResourceRefList().names_.Size();

        // Straightly speaking the individual resource reference in the list is not an attribute of the serializable
        // However, the AttributeInfo structure is used here to reduce the number of parameters being passed in the function
        AttributeInfo refInfo;
        refInfo.name_ = info.name_;
        refInfo.type_ = VAR_RESOURCEREF;
        for (unsigned i = 0; i < numRefs; ++i)
            CreateAttributeEditor(list, serializables, refInfo, index, i, i > 0);
    }
    else if (type == VAR_VARIANTVECTOR)
    {
//        unsigned nameIndex = 0;
//        unsigned repeat = M_MAX_UNSIGNED;
//
//        VectorStruct@ vectorStruct;
//        Array<Variant>@ vector;
//        bool emptyNestedVector = false;
//        if (info.name.Contains('>'))
//        {
//            @vector = @nestedVector_;
//            vectorStruct = GetNestedVectorStruct(serializables, info.name);
//            repeat = vector[subIndex].GetUInt();    // Nested VariantVector must have a predefined repeat count at the start of the vector
//            emptyNestedVector = repeat == 0;
//        }
//        else
//        {
//            @vector = serializables[0].attributes[index].GetVariantVector();
//            vectorStruct = GetVectorStruct(serializables, index);
//            subIndex = 0;
//        }
//        if (vectorStruct is null)
//            return null;
//
//        for (unsigned i = subIndex; i < vector.length; ++i)
//        {
//            // The individual variant in the vector is not an attribute of the serializable, the structure is reused for convenience
//            AttributeInfo vectorInfo;
//            vectorInfo.name = vectorStruct.variableNames[nameIndex++];
//            bool nested = vectorInfo.name.Contains('>');
//            if (nested)
//            {
//                vectorInfo.type = VAR_VARIANTVECTOR;
//                nestedVector_ = @vector;
//            }
//            else
//                vectorInfo.type = vector[i].type;
//            CreateAttributeEditor(list, serializables, vectorInfo, index, i);
//            if (nested)
//            {
//                i = nestedSubIndex_;
//                nestedVector_ = 0;
//            }
//            if (emptyNestedVector)
//            {
//                nestedSubIndex_ = i;
//                break;
//            }
//            if (nameIndex >= vectorStruct.variableNames.length)
//            {
//                if (--repeat == 0)
//                {
//                    nestedSubIndex_ = i;
//                    break;
//                }
//                nameIndex = vectorStruct.restartIndex;
//
//                // Create small divider for repeated instances
//                UIElement@ divider = UIElement();
//                divider.SetFixedHeight(8);
//                list.AddItem(divider);
//            }
//        }
    }
    else if (type == VAR_VARIANTMAP)
    {
        const VariantMap& map = serializables.Front()->GetAttribute(index).GetVariantMap();
        Vector<StringHash> keys = map.Keys();
        for (unsigned i = 0; i < keys.Size(); ++i)
        {
            String varName = GetVarName(keys[i]);
            bool shouldHide = false;

            if (varName.Empty())
            {
                // UIElements will contain internal vars, which do not have known mappings. Hide these
                if (static_cast<UIElement*>(serializables.Front()))
                    shouldHide = true;
                // Else, for scene nodes, show as hexadecimal hashes if nothing else is available
                varName = keys[i].ToString();
            }
            Variant value = map[keys[i]];

            // The individual variant in the map is not an attribute of the serializable, the structure is reused for convenience
            AttributeInfo mapInfo;
            mapInfo.name_ = varName + " (Var)";
            mapInfo.type_ = value.GetType();
            parent = CreateAttributeEditor(list, serializables, mapInfo, index, i);
            // Add the variant key to the parent. We may fail to add the editor in case it is unsupported
            if (parent)
            {
                parent->SetVar("Key", keys[i].Value());
                // If variable name is not registered (i.e. it is an editor internal variable) then hide it
                if (varName.Empty() || shouldHide)
                    parent->SetVisible(false);
            }
        }
    }

    return parent;
}

UIElement* Inspector_GetNodeContainer(UIElement* parent)
{
    if (inspectorNodeContainerIndex_ < parent->GetNumChildren())
        return parent->GetChild(inspectorNodeContainerIndex_);

    UIElement* container = parent->LoadChildXML(MapEditorLibImpl::GetXmlElement(EDITORELT_INSPECTORATTRIBUTE), MapEditorLibImpl::GetUIStyle(UISTYLE_EDITORDEFAULT));
    container->LoadChildXML(MapEditorLibImpl::GetXmlElement(EDITORELT_INSPECTORVARIABLE), MapEditorLibImpl::GetUIStyle(UISTYLE_EDITORDEFAULT));
    parent->LoadChildXML(MapEditorLibImpl::GetXmlElement(EDITORELT_INSPECTORTAG), MapEditorLibImpl::GetUIStyle(UISTYLE_EDITORDEFAULT));
    parent->GetChild("TagsLabel", true)->SetFixedWidth(LABEL_WIDTH);

    MapEditorLibImpl::Get()->SubscribeToEvent(container->GetChild("ResetToDefault", true), E_RELEASED, new Urho3D::EventHandlerImpl<MapEditorLibImpl>(MapEditorLibImpl::Get(), &MapEditorLibImpl::HandleInspectorResetToDefault));
//    SubscribeToEvent(container->GetChild("NewVarDropDown", true), E_ITEMSELECTED, URHO3D_HANDLER(MapEditorLibImpl, HandleInspectorTagsSelect)"CreateNodeVariable");
//    SubscribeToEvent(container->GetChild("DeleteVarButton", true), E_RELEASED, URHO3D_HANDLER(MapEditorLibImpl, HandleInspectorTagsSelect)"DeleteNodeVariable");
    MapEditorLibImpl::Get()->SubscribeToEvent(parent->GetChild("TagsEdit", true), E_TEXTCHANGED, new Urho3D::EventHandlerImpl<MapEditorLibImpl>(MapEditorLibImpl::Get(), &MapEditorLibImpl::HandleInspectorTagsEdit));
    MapEditorLibImpl::Get()->SubscribeToEvent(parent->GetChild("TagsSelect", true), E_RELEASED, new Urho3D::EventHandlerImpl<MapEditorLibImpl>(MapEditorLibImpl::Get(), &MapEditorLibImpl::HandleInspectorTagsSelect));

    inspectorNodeContainerIndex_ = parent->GetNumChildren();
    inspectorComponentContainerStartIndex_ += 2;

    return container;
}

UIElement* Inspector_GetComponentContainer(UIElement* parent, unsigned index)
{
    if (inspectorComponentContainerStartIndex_ + index < parent->GetNumChildren())
        return parent->GetChild(inspectorComponentContainerStartIndex_ + index);

    UIElement* container = 0;
    for (unsigned i = parent->GetNumChildren(); i <= inspectorComponentContainerStartIndex_ + index; ++i)
    {
        container = parent->LoadChildXML(MapEditorLibImpl::GetXmlElement(EDITORELT_INSPECTORATTRIBUTE), MapEditorLibImpl::GetUIStyle(UISTYLE_EDITORDEFAULT));
        MapEditorLibImpl::Get()->SubscribeToEvent(container->GetChild("ResetToDefault", true), E_RELEASED, new Urho3D::EventHandlerImpl<MapEditorLibImpl>(MapEditorLibImpl::Get(), &MapEditorLibImpl::HandleInspectorResetToDefault));
    }

    return container;
}

void LoadAttributeEditor(UIElement* parent, const Variant& value, const AttributeInfo& info, bool editable, bool sameValue, const Vector<Variant>& values)
{
    if (!parent)
        return;

    unsigned index = parent->GetVar("Index").GetUInt();

    // Assume the first child is always a text label element or a container that containing a text label element
    UIElement* label = parent->GetChild(0);
    if (label->GetType() == UI_ELEMENT_TYPE && label->GetNumChildren() > 0)
        label = label->GetChild(0);
    if (label->GetType() == TEXT_TYPE)
    {
        bool modified;
        if (info.defaultValue_.GetType() == VAR_NONE || info.defaultValue_.GetType() == VAR_RESOURCEREFLIST)
            modified = !value.IsZero();
        else
            modified = value != info.defaultValue_;
        static_cast<Text*>(label)->SetColor(editable ? (modified ? modifiedTextColor : normalTextColor) : nonEditableTextColor);
    }

    VariantType type = info.type_;

    if (type == VAR_FLOAT || type == VAR_DOUBLE || type == VAR_STRING || type == VAR_BUFFER)
        SetEditable(SetValue(parent->GetChild(1), value.ToString(), sameValue), editable && sameValue);
    else if (type == VAR_BOOL)
        SetEditable(SetValue(parent->GetChild(1), value.GetBool(), sameValue), editable && sameValue);
    else if (type == VAR_INT)
    {
        /*if (bitSelectionAttrs.Find(info.name_) > -1)
            SetEditable(SetValue(parent->GetChild("LineEdit", true), value.ToString(), sameValue), editable && sameValue);
        else */if (!info.enumNames_ || *info.enumNames_ == 0)
            SetEditable(SetValue(parent->GetChild(1), value.ToString(), sameValue), editable && sameValue);
        else
            SetEditable(SetValue(parent->GetChild(1), value.GetInt(), sameValue), editable && sameValue);
    }
    else if (type == VAR_RESOURCEREF)
    {
        SetEditable(SetValue(parent->GetChild(1)->GetChild(0), value.GetResourceRef().name_, sameValue), editable && sameValue);
        SetEditable(parent->GetChild(1)->GetChild(1), editable && sameValue);  // If editable then can pick
        for (unsigned i = 2; i < parent->GetChild(1)->GetNumChildren(); ++i)
            SetEditable(parent->GetChild(1)->GetChild(i), sameValue); // If same value then can open/edit/test
    }
    else if (type == VAR_RESOURCEREFLIST)
    {
        UIElement* list = parent->GetParent();
        const ResourceRefList& refList = value.GetResourceRefList();
        for (unsigned subIndex = 0; subIndex < refList.names_.Size(); ++subIndex)
        {
            parent = GetAttributeEditorParent(list, index, subIndex);
            if (!parent)
                break;

            const String& firstName = refList.names_[subIndex];
            bool nameSameValue = true;
            if (!sameValue)
            {
                // Reevaluate each name in the list
                for (unsigned i = 0; i < values.Size(); ++i)
                {
                    const ResourceRefList& rList = values[i].GetResourceRefList();
                    if (subIndex >= rList.names_.Size() || rList.names_[subIndex] != firstName)
                    {
                        nameSameValue = false;
                        break;
                    }
                }
            }
            SetEditable(SetValue(parent->GetChild(1)->GetChild(0), firstName, nameSameValue), editable && nameSameValue);
        }
    }
    else if (type == VAR_VARIANTVECTOR)
    {
//        UIElement* list = parent->GetParent();
//        Array<Variant>@ vector = value.GetVariantVector();
//        for (unsigned subIndex = 0; subIndex < vector..Size();; ++subIndex)
//        {
//            parent = GetAttributeEditorParent(list, index, subIndex);
//            if (parent is null)
//                break;
//
//            Variant firstValue = vector[subIndex];
//            bool sameVal = true;
//            Array<Variant> varValues;
//
//            // Reevaluate each variant in the vector
//            for (unsigned i = 0; i < values..Size();; ++i)
//            {
//                Array<Variant>@ vec = values[i].GetVariantVector();
//                if (subIndex < vec..Size();)
//                {
//                    Variant v = vec[subIndex];
//                    varValues.Push(v);
//                    if (v != firstValue)
//                        sameVal = false;
//                }
//                else
//                    sameVal = false;
//            }
//
//            // The individual variant in the list is not an attribute of the serializable, the structure is reused for convenience
//            AttributeInfo ai;
//            ai.type = firstValue.type;
//            LoadAttributeEditor(parent, firstValue, ai, editable, sameVal, varValues);
//        }
    }
    else if (type == VAR_VARIANTMAP)
    {
        UIElement* list = parent->GetParent();
        const VariantMap& map = value.GetVariantMap();
        Vector<StringHash> keys = map.Keys();
        for (unsigned subIndex = 0; subIndex < keys.Size(); ++subIndex)
        {
            parent = GetAttributeEditorParent(list, index, subIndex);
            if (!parent)
                break;

            String varName = GetVarName(keys[subIndex]);
            if (varName.Empty())
                varName = keys[subIndex].ToString(); // Use hexadecimal if nothing else is available

            Variant firstValue = map[keys[subIndex]];
            bool sameVal = true;
            Vector<Variant> varValues;

            // Reevaluate each variant in the map
            for (unsigned i = 0; i < values.Size(); ++i)
            {
                const VariantMap& m = values[i].GetVariantMap();
                if (m.Contains(keys[subIndex]))
                {
                    Variant v = m[keys[subIndex]];
                    varValues.Push(v);
                    if (v != firstValue)
                       sameVal = false;
                }
                else
                    sameVal = false;
            }

            // The individual variant in the map is not an attribute of the serializable, the structure is reused for convenience
            AttributeInfo ai;
            ai.type_ = firstValue.GetType();
            LoadAttributeEditor(parent, firstValue, ai, editable, sameVal, varValues);
        }
    }
/*
    // Pb with this original code from Urho3D-1.7
    else
    {
        Array<Array<String> > coordinates;
        for (unsigned i = 0; i < values.length; ++i)
        {
            Variant v = values[i];
            log.Info("LoadAttributeEditor : Values=" + v.ToString());

            // Convert Quaternion value to Vector3 value first
            if (type == VAR_QUATERNION)
                v = v.GetQuaternion().eulerAngles;

            /// Pb HERE !
            coordinates.Push(v.ToString().Split(' '));
        }
        for (unsigned i = 0; i < coordinates[0].length; ++i)
        {
            String str = coordinates[0][i];
            bool coordinateSameValue = true;
            if (!sameValue)
            {
                // Reevaluate each coordinate
                for (unsigned j = 1; j < coordinates.length; ++j)
                {
                    if (coordinates[j][i] != str)
                    {
                        coordinateSameValue = false;
                        break;
                    }
                }
            }
            log.Info("LoadAttributeEditor : Value=" + str);
            SetEditable(SetValue(parent.children[i + 1], str, coordinateSameValue), editable && coordinateSameValue);
        }
    }
*/
    // Pb with the original code from Urho3D-1.7 or Urho3D-1.5 above (but no pb with the original code inside the real URHO3D editors ...)
    // the allocation of Array of Array with CScriptArray::Resize must be made step by step for the moment
    else if (values.Size() > 0)
    {
        Vector<Vector<String > > coordinates;
        coordinates.Resize(values.Size());

        for (unsigned i = 0; i < values.Size(); ++i)
        {
            Variant v = values[i];

            // Convert Quaternion value to Vector3 value first
            if (type == VAR_QUATERNION)
                v = v.GetQuaternion().EulerAngles();

            Vector<String> vsplit = v.ToString().Split(' ');
            coordinates[i].Resize(vsplit.Size());
            for (unsigned j = 0; j < vsplit.Size(); ++j)
            {
                coordinates[i][j] = vsplit[j];
//                log.Info("LoadAttributeEditor : vsplit=" + vsplit[j]);
            }
        }
        for (unsigned i = 0; i < coordinates[0].Size(); ++i)
        {
            String str = coordinates[0][i];
            bool coordinateSameValue = true;
            if (!sameValue)
            {
                // Reevaluate each coordinate
                for (unsigned j = 1; j < coordinates.Size(); ++j)
                {
                    if (coordinates[j][i] != str)
                    {
                        coordinateSameValue = false;
                        break;
                    }
                }
            }
//            log.Info("LoadAttributeEditor : Value=" + str);
            SetEditable(SetValue(parent->GetChild(i + 1), str, coordinateSameValue), editable && coordinateSameValue);
        }
    }
}

void LoadAttributeEditor(UIElement* list, const Vector<Serializable*>& serializables, const AttributeInfo& info, unsigned index)
{
    bool editable = (info.mode_ & AM_NOEDIT) == 0;

    UIElement* parent = GetAttributeEditorParent(list, index, 0);
    if (!parent)
        return;

    inspectorInLoadAttributeEditor = true;

    bool sameName = true;
    bool sameValue = true;
    Variant value = serializables.Front()->GetAttribute(index);
    Vector<Variant> values;
    for (unsigned i = 0; i < serializables.Size(); ++i)
    {
        if (index >= serializables[i]->GetNumAttributes() || serializables[i]->GetAttributes()->At(index).name_ != info.name_)
        {
            sameName = false;
            break;
        }

        Variant val = serializables[i]->GetAttribute(index);

//        URHO3D_LOGINFOF(" LoadAttributeEditor ... serializable[%d] attribute[%u] name=%s type=%s variant=%s ...", i, index, info.name_.CString(), val.GetTypeName().CString(), val.ToString().CString());

        if (val != value)
            sameValue = false;
        values.Push(val);
    }

    // Attribute with different values from multiple-select is loaded with default/empty value and non-editable
    if (sameName)
        LoadAttributeEditor(parent, value, info, editable, sameValue, values);
    else
        parent->SetVisible(false);

    inspectorInLoadAttributeEditor = false;
}

void Inspector_UpdateAttributes(const Vector<Serializable*>& serializables, ListView* list, bool& fullUpdate)
{
    // If attributes have changed structurally, do a full update
    unsigned count = GetAttributeEditorCount(serializables);
    if (!fullUpdate)
    {
        if (list->GetContentElement()->GetNumChildren() != count)
            fullUpdate = true;
    }

    // Remember the old scroll position so that a full update does not feel as jarring
    const IntVector2& oldViewPos = list->GetViewPosition();

    if (fullUpdate)
    {
        list->RemoveAllItems();
        const Vector<SharedPtr<UIElement> >& children = list->GetChildren();
        for (unsigned i = 0; i < children.Size(); ++i)
        {
            if (!children[i]->IsInternal())
                children[i]->Remove();
        }
    }

    if (!serializables.Size())
        return;

    unsigned numAttributes = serializables.Front()->GetNumAttributes();

//    URHO3D_LOGINFOF("MapEditorLibImpl() - Inspector_UpdateAttributes ... serializable NumAttributes=%u ...", numAttributes);

    // If there are many serializables, they must share same attribute structure (up to certain number if not all)
    for (unsigned i = 0; i < numAttributes; ++i)
    {
        AttributeInfo info = serializables.Front()->GetAttributes()->At(i);
        if (!inspectorShowNonEditableAttribute && (info.mode_ & AM_NOEDIT) != 0)
            continue;

//        URHO3D_LOGINFOF("MapEditorLibImpl() - Inspector_UpdateAttributes ... serializable fullUpdate=%s attribute[%d] ...", fullUpdate?"yes":"no", i);

        // Use the default value (could be instance's default value) of the first serializable as the default for all
        info.defaultValue_ = serializables.Front()->GetAttributeDefault(i);

        if (fullUpdate)
            CreateAttributeEditor(list, serializables, info, i, 0);

        LoadAttributeEditor(list, serializables, info, i);
    }

    if (fullUpdate)
        list->SetViewPosition(oldViewPos);
}

void Inspector_LayoutUpdated(UIElement* parent)
{
    // When window resize and so the list's width is changed, adjust the 'Is enabled' container width and icon panel width so that their children stay at the right most position
    for (unsigned i = 0; i < parent->GetNumChildren(); ++i)
    {
        UIElement* container = parent->GetChild(i);
        ListView* list = static_cast<ListView*>(container->GetChild(ATTRIBUTELIST));
        if (!list)
            continue;

        int width = list->GetWidth();

        UIElement* title = container->GetChild(TITLETEXT);

        // Adjust the icon panel's width
        UIElement* panel = container->GetChild(ICONSPANEL, true);
        if (panel)
            panel->SetWidth(width);

        // At the moment, only 'Is Enabled' container (place-holder + check box) is being created as child of the list view instead of as list item
        for (unsigned j = 0; j < list->GetNumChildren(); ++j)
        {
            UIElement* element = list->GetChild(j);

            if (!element->IsInternal())
            {
                element->SetFixedWidth(width);

                element->SetPosition(IntVector2(0, (title->GetScreenPosition() - list->GetScreenPosition()).y_));

                // Adjust icon panel's width one more time to cater for the space occupied by 'Is Enabled' check box
                if (panel)
                    panel->SetWidth(width - element->GetChild(1)->GetWidth() - panel->GetLayoutSpacing());

                break;
            }
        }
        for (unsigned j = 0; j < list->GetNumItems(); ++j)
        {
            UIElement* element = list->GetItem(j);
            element->SetFixedWidth(width - 6);
        }
    }
}


void MapEditorLibImpl::UpdateSpawnToolBar()
{
    UIElement* panel = uiPanels_[PANEL_MAINTOOLBAR];
    if (!panel)
        return;

    for (int i = 1; i < MAX_SPAWNCATEGORIES; i++)
    {
        CheckBox* checkbox = (CheckBox*)panel->GetChild(SpawnCategoryNameStr[i], true);
        checkbox->SetChecked(spawnCategory_ == i);
    }
}

void MapEditorLibImpl::UpdateInspector(bool fullUpdate)
{
    UIElement* panel = uiPanels_[PANEL_INSPECTORWINDOW];
    if (!panel)
        return;

    if (inspectorLocked_)
        return;

    UIElement* parentContainer = uiPanels_[PANEL_INSPECTORWINDOW]->GetChild(PARENTCONTAINER, true);

    bool attributesDirty = false;
    if (fullUpdate)
        attributesFullDirty = false;

    // If full update delete all containers and add them back as necessary
    if (fullUpdate)
    {
        parentContainer->RemoveAllChildren();
        inspectorNodeContainerIndex_ = M_MAX_UNSIGNED;
        inspectorComponentContainerStartIndex_ = 0;
//        elementContainerIndex = M_MAX_UNSIGNED;
    }

    // Update all ScriptInstances/LuaScriptInstances
    //UpdateScriptInstances();

    if (editNodes_.Size())
    {
        UIElement* container = Inspector_GetNodeContainer(parentContainer);
        Text* nodeTitle = static_cast<Text*>(container->GetChild(TITLETEXT));
        String nodeType;
        String idStr;

        if (editNode_)
        {
            if (editNode_->GetID() >= FIRST_LOCAL_ID)
                idStr.AppendWithFormat(" (Local ID %u)", editNode_->GetID());
            else
                idStr.AppendWithFormat(" (ID %u)", editNode_->GetID());
            nodeType = editNode_->GetTypeName();
            nodeTitle->SetText(nodeType + idStr);
            LineEdit* tagEdit = static_cast<LineEdit*>(parentContainer->GetChild(TAGSEDIT, true));
            const StringVector& tags = editNode_->GetTags();
            String tagsStr;
            for (unsigned i=0; i < tags.Size(); i++)
                tagsStr.Append(tags[i] + String(";"));
            tagEdit->SetText(tagsStr);
        }
        else
        {
            nodeType = editNodes_.Front()->GetTypeName();
            idStr.AppendWithFormat("%s (ID %s : %ux)", nodeType.CString(), STRIKED_OUT.CString(), editNodes_.Size());
            nodeTitle->SetText(idStr);
        }
        IconizeUIElement(nodeTitle, nodeType);

        ListView* list = static_cast<ListView*>(container->GetChild(ATTRIBUTELIST));

        Vector<Serializable*> serializables;
        for (unsigned i = 0; i < editNodes_.Size(); ++i)
            serializables.Push(static_cast<Serializable*>(editNodes_[i]));

        Inspector_UpdateAttributes(serializables, list, fullUpdate);

        if (fullUpdate)
        {
            /// todo Avoid hardcoding
            /// Resize the node editor according to the number of variables, up to a certain maximum
            unsigned maxAttrs = Clamp(list->GetContentElement()->GetNumChildren(), MIN_NODE_ATTRIBUTES, MAX_NODE_ATTRIBUTES);
            list->SetFixedHeight(maxAttrs * ATTR_HEIGHT + 2);
            container->SetFixedHeight(maxAttrs * ATTR_HEIGHT + 58);
        }

        // Set icon's target in the icon panel
        SetAttributeEditorID(container->GetChild(RESETTODEFAULT, true), serializables);
    }

    if (editComponents_.Size())
    {
        unsigned numEditableComponents = editComponents_.Size() / numEditableComponentsPerNode;
        String multiplierText;
        if (numEditableComponents > 1)
            multiplierText.AppendWithFormat(" (%ux)", numEditableComponents);

        for (unsigned j = 0; j < numEditableComponentsPerNode; ++j)
        {
            UIElement* container = Inspector_GetComponentContainer(parentContainer, j);
            Text* componentTitle = static_cast<Text*>(container->GetChild(TITLETEXT));
            Component* component = editComponents_[j * numEditableComponents];

            componentTitle->SetText(GetComponentTitle(component) + multiplierText);
            IconizeUIElement(componentTitle, component->GetTypeName());
            SetIconEnabledColor(componentTitle, component->IsEnabledEffective());

            URHO3D_LOGINFOF("MapEditorLibImpl() - UpdateInspector ... numEditableComponents=%u j=%u editComponent=%s title=%s ...", numEditableComponents, j, component->GetTypeName().CString(), componentTitle->GetText().CString());

            Vector<Serializable*> serializables;
            for (unsigned i = 0; i < numEditableComponents; ++i)
                serializables.Push(static_cast<Serializable*>(editComponents_[j * numEditableComponents + i]));

            Inspector_UpdateAttributes(serializables, static_cast<ListView*>(container->GetChild(ATTRIBUTELIST)), fullUpdate);

            SetAttributeEditorID(container->GetChild(RESETTODEFAULT, true), serializables);
        }
    }

    if (parentContainer->GetNumChildren() > 0)
    {
        if (!editNodes_.Empty())
        {
            Text* nodeTitle = static_cast<Text*>(Inspector_GetNodeContainer(parentContainer)->GetChild(TITLETEXT));
            if (editNode_)
            {
                SetIconEnabledColor(nodeTitle, editNode_->IsEnabled());
            }
            else if (editNodes_.Size() > 0)
            {
                bool hasSameEnabledState = true;

                for (unsigned i = 1; i < editNodes_.Size(); ++i)
                {
                    if (editNodes_[i]->IsEnabled() != editNodes_.Front()->IsEnabled())
                    {
                        hasSameEnabledState = false;
                        break;
                    }
                }

                SetIconEnabledColor(nodeTitle, editNodes_.Front()->IsEnabled(), !hasSameEnabledState);
            }
        }

        if (!editComponents_.Empty())
        {
            unsigned numEditableComponents = editComponents_.Size() / numEditableComponentsPerNode;

            for (unsigned j = 0; j < numEditableComponentsPerNode; ++j)
            {
                Text* componentTitle = static_cast<Text*>(Inspector_GetComponentContainer(parentContainer, 0)->GetChild(TITLETEXT));

                bool enabledEffective = editComponents_[j * numEditableComponents]->IsEnabledEffective();
                bool hasSameEnabledState = true;
                for (unsigned i = 1; i < numEditableComponents; ++i)
                {
                    if (editComponents_[j * numEditableComponents + i]->IsEnabledEffective() != enabledEffective)
                    {
                        hasSameEnabledState = false;
                        break;
                    }
                }

                SetIconEnabledColor(componentTitle, enabledEffective, !hasSameEnabledState);
            }
        }
    }
    else
    {
        // No editables, insert a dummy component container to show the information
        Text* titleText = static_cast<Text*>(Inspector_GetComponentContainer(parentContainer, 0)->GetChild(TITLETEXT));
        titleText->SetText("Select editable objects");
        titleText->SetAutoLocalizable(true);
        UIElement* panel = titleText->GetChild(ICONSPANEL);
        panel->SetVisible(false);
    }

    // Adjust size and position of manual-layout UI-elements, e.g. icons panel
    if (fullUpdate)
        Inspector_LayoutUpdated(parentContainer);
}

void MapEditorLibImpl::UpdateNodesAttributes()
{
    if (editNodes_.Size())
    {
        Vector<Serializable*> serializables;
        for (unsigned i = 0; i < editNodes_.Size(); ++i)
            serializables.Push(static_cast<Serializable*>(editNodes_[i]));

        bool fullUpdate = false;
        inspectorNodeContainerIndex_ = 0;
        Inspector_UpdateAttributes(serializables, static_cast<ListView*>(Inspector_GetNodeContainer(uiPanels_[PANEL_INSPECTORWINDOW]->GetChild(PARENTCONTAINER, true))->GetChild(ATTRIBUTELIST)), fullUpdate);
    }
}

void MapEditorLibImpl::UpdatePanel(int panel, bool force)
{
    if (!force && !uiPanelDirty_[panel])
        return;

    switch (panel)
    {
    case PANEL_MAINTOOLBAR :
        UpdateSpawnToolBar();
        break;
    case PANEL_INSPECTORWINDOW :
        bool fullupdate = true;
        UpdateInspector(fullupdate);
        break;
//    case PANEL_ANIMATOR2D_MAIN :
//        break;
    }

    uiPanelDirty_[panel] = false;
}

void MapEditorLibImpl::Update(float timeStep)
{
    if (!World2D::GetWorld())
        return;

    for (int i = 0; i < MAX_EDITORPANELS; i++)
        UpdatePanel(i);

    IntVector2 mouseposition;
    UIElement* uielt = 0;
    GameHelpers::GetInputPosition(mouseposition.x_, mouseposition.y_, &uielt);
    if (uielt)
        return;

    Input* input = GameContext::Get().input_;
    Camera* camera = GameContext::Get().camera_;

    float speedMultiplier = 1.f;
    if (input->GetKeyDown(KEY_LSHIFT))
        speedMultiplier = cameraShiftSpeedMultiplier;

    // Zoom camera
    if (input->GetMouseMoveWheel())
    {
        float zoom = camera->GetZoom() + input->GetMouseMoveWheel() * 0.1f * speedMultiplier;
        camera->SetZoom(Clamp(zoom, 0.1f, 30.f));
    }

    cameraViewMoving = false;

    // Pan camera
    if (input->GetMouseButtonDown(MOUSEB_MIDDLE))
    {
        const IntVector2& mouseMove = input->GetMouseMove();
        if (mouseMove.x_ != 0 || mouseMove.y_ != 0)
        {
            Vector3 translate(-mouseMove.x_, mouseMove.y_, 0.f);
            translate *= (timeStep * cameraBaseSpeed * 0.5f * speedMultiplier);
            camera->GetNode()->Translate(translate);
            cameraViewMoving = true;
        }

        return;
    }

    WorldMapPosition position;
    GameHelpers::GetWorldMapPosition(mouseposition, position);

    // Toggle PickMode
    if (input->GetKeyPress(KEY_TAB))
    {
        pickMode_++;
        if (pickMode_ >= MAX_PICK_MODES)
            pickMode_ = PICK_GEOMETRIES;

        URHO3D_LOGINFOF("PickMode = %s(%d) ", PickModeStr[pickMode_], pickMode_);    
    }
    // Delete Selected Objects
    if (input->GetKeyPress(KEY_DELETE))
    {
        DeleteSelectedObjects();
    }

    Drawable* drawableUnderMouse_ = 0;
    CollisionShape2D* shapeUnderMouse_ = 0;
    RenderCollider* colliderUnderMouse_ = 0;
    int colliderPolyShapeId_ = 0;
    int colliderContourId_ = 0;

    // Pick an object
    if (!disableViewRayCast_ && spawnCategory_ == SPAWN_NONE)
    {
        // ViewRayCast
        const IntRect& viewRect = GameContext::Get().renderer_->GetViewport(0)->GetRect();
        Ray ray = GameContext::Get().camera_->GetScreenRay(float(mouseposition.x_ - viewRect.left_) / viewRect.Width(), float(mouseposition.y_ - viewRect.top_) / viewRect.Height());

        if (pickMode_ == PICK_GEOMETRIES || pickMode_ == PICK_LIGHTS)
        {
            Drawable* drawable = 0;
            PODVector<RayQueryResult> results;
            RayOctreeQuery query(results, ray, RAY_TRIANGLE, GameContext::Get().camera_->GetFarClip(), pickModeDrawableFlags[pickMode_]);
            GameContext::Get().octree_->Raycast(query);

            if (results.Size() > 0)
            {
                // Find the smallest drawable
                float lastarea = M_INFINITY;
                for (unsigned i=0; i < results.Size(); i++)
                {
                    Drawable* d = results[i].drawable_;
                    if (d->IsInstanceOf<ObjectTiled>() || d->IsInstanceOf<RenderShape>() || d->IsInstanceOf<DrawableScroller>())
                        continue;

                    // Keep selected drawable if is in RayQuery Results.
                    if (selectedDrawable_ == d)
                    {
                        drawable = selectedDrawable_;
                        break;
                    }

                    float newarea = d->GetWorldArea2D();
                    if (IsNaN(newarea) || newarea == M_INFINITY || newarea == 0.f || newarea >= lastarea)
                        continue;

                    lastarea = newarea;
                    drawable = d;
                }
            }

            if (drawable)
                drawableUnderMouse_ = drawable;
        }
        else if (pickMode_ == PICK_RIGIDBODIES)
        {
            RigidBody2D* body = 0;
            GameContext::Get().physicsWorld_->GetPhysicElements(position.position_, body, shapeUnderMouse_);

            if (body)
                drawableUnderMouse_ = body->GetNode()->GetDerivedComponent<Drawable>();
        }
        else if (pickMode_ == PICK_COLLISIONSHAPES)
        {
            shapeUnderMouse_ = GameHelpers::GetMapCollisionShapeAt(position);
        }
        else if (pickMode_ == PICK_RENDERSHAPES)
        {
            selectRenderShapeClipped_ = false;
            drawableUnderMouse_ = GameHelpers::GetMapRenderShapeAt(position, colliderPolyShapeId_, colliderContourId_, colliderUnderMouse_, selectRenderShapeClipped_);
        }
        #if defined(ACTIVE_CLIPPING) && defined(ACTIVE_RENDERSHAPE_CLIPPING)
        else if (pickMode_ == PICK_RENDERSHAPES_CLIPPED)
        {
            selectRenderShapeClipped_ = true;
            drawableUnderMouse_ = GameHelpers::GetMapRenderShapeAt(position, colliderPolyShapeId_, colliderContourId_, colliderUnderMouse_, selectRenderShapeClipped_);
        }
        #endif
    }

    if (input->GetMouseButtonPress(MOUSEB_LEFT))
    {
        if (spawnCategory_ != SPAWN_NONE)
            clickMode_ = CLICK_SPAWN;
        else
            clickMode_ = CLICK_SELECT;
    }
    else if (input->GetMouseButtonDown(MOUSEB_RIGHT))
    {
        clickMode_ = CLICK_MOVE;
    }
    else if (input->GetMouseButtonDown(MOUSEB_LEFT) && clickMode_ == CLICK_SPAWN && spawnCategory_ == SPAWN_WATER)
    {
        waterQuantity_ = Clamp(waterQuantity_+0.002f, 0.01f, 2.f);                
    }
    else
    {
        clickMode_ = CLICK_NONE;
        waterQuantity_ = 0.01f;
    }

    // Selection Mode
    if (clickMode_ == CLICK_SELECT)
    {
        multiSelection_ = input->GetQualifierDown(QUAL_CTRL);

        if (drawableUnderMouse_)
        {
            if (drawableUnderMouse_->IsInstanceOf<RenderShape>())
            {
                RenderShape* rendershape  = static_cast<RenderShape*>(drawableUnderMouse_);

                selectRenderShapeColor_   = selectRenderShapeClipped_ ? Color::CYAN.ToUInt() : Color::MAGENTA.ToUInt();
                selectedPolyShapeId_      = colliderPolyShapeId_;
                selectedRenderContourId_  = colliderContourId_;

                SelectObject(rendershape, colliderUnderMouse_);

                // reclip debug
                if (selectRenderShapeClipped_)
                {
                    PolyShape& polyshape = rendershape->GetShapes()[selectedPolyShapeId_];
                    polyshape.Clip(polyshape.clippedRect_, true, rendershape->GetNode()->GetWorldTransform2D().Inverse(), true);
                }
            }
            else
            {
                if (drawableUnderMouse_ == AnimatorEditor::Get()->GetAnimatedSprite())
                    AnimatorEditor::Get()->Update(position.position_);
                else
                    AnimatorEditor::Get()->ResetSelection(RESETALLSELECT);

                SelectObject(drawableUnderMouse_);
            }
        }
        else if (shapeUnderMouse_)
        {
            SelectObject(shapeUnderMouse_);
        }
        else
        {
            UnSelectObject();
            AnimatorEditor::Get()->ResetSelection(RESETALLSELECT);
        }
    }
    else if (clickMode_ == CLICK_MOVE)
    {
        // Move Sprite
        if (AnimatorEditor::Get()->HasSelection())
        {
            int mode = input->GetQualifierDown(QUAL_SHIFT) ? GE_SCALE : (input->GetQualifierDown(QUAL_CTRL) ? GE_ROTATE : GE_TRANSLATE);
            AnimatorEditor::Get()->MoveSelection(position.position_, Vector2(input->GetMouseMoveX() * moveStep_, -input->GetMouseMoveY() * moveStep_), mode);
        }
        // Move Objects
        else if (!editNodes_.Empty())
        {
            if (MoveObjects(Vector2(input->GetMouseMoveX() * timeStep * moveStep_, -input->GetMouseMoveY() * timeStep * moveStep_)))
            {
                UpdateNodesAttributes();
                moveObjects_ = true;
            }
        }
    }
    else
    {
        multiSelection_ = false;

        // Spawn Mode
        if (clickMode_ == CLICK_SPAWN && spawnCategory_ != SPAWN_NONE)
        {
            SpawnObject(position);
        }

        else if (drawableUnderMouse_ || shapeUnderMouse_)
        {
            if (drawableUnderMouse_ && drawableUnderMouse_ == AnimatorEditor::Get()->GetAnimatedSprite())
            {
                AnimatorEditor::Get()->Update(position.position_);
            }
            else
            {
                AnimatorEditor::Get()->ResetSelection(RESETHOVERSELECT);

                // Draw Picked Object
                DebugRenderer* debugRenderer = scene_->GetComponent<DebugRenderer>();
                if (debugRenderer)
                {
                    if (drawableUnderMouse_)
                    {
                        debugRenderer->AddNode(drawableUnderMouse_->GetNode(), 1.f, false);
                        drawableUnderMouse_->DrawDebugGeometry(debugRenderer, false);
                    }
                    else
                    {
                        GameContext::Get().physicsWorld_->DrawDebug(shapeUnderMouse_, debugRenderer, false, Color::GRAY);
                    }
                }
            }

            // Update Move Objects Position
            if (!editNodes_.Empty() && moveObjects_)
            {
                for (unsigned i = 0; i < editNodes_.Size(); ++i)
                    World2D::GetWorld()->GetCurrentMap()->RefreshEntityPosition(editNodes_[i]);

                moveObjects_ = false;
            }
        }
    }
}


void MapEditorLibImpl::SpawnObject(const WorldMapPosition& position)
{
    Drawable2D* drawable = 0;

    if (spawnCategory_ == SPAWN_ACTOR)
    {
        Node* entity = World2D::GetWorld()->SpawnActor();
        if (entity)
        {
            drawable = entity->GetDerivedComponent<Drawable2D>();
            SelectObject(drawable);
            URHO3D_LOGINFOF("MapEditorLibImpl() - SpawnObject : Spawn Actor - entity=%s(%u)", entity->GetName().CString(), entity->GetID());
        }
    }
    else if (spawnCategory_ == SPAWN_MONSTER)
    {
        Node* entity = World2D::GetWorld()->SpawnEntity(spawnObject_);
        if (entity)
        {
            drawable = entity->GetDerivedComponent<Drawable2D>();
            SelectObject(drawable);

            URHO3D_LOGINFOF("MapEditorLibImpl() - SpawnObject : Spawn Monster - entity=%s(%u)", entity->GetName().CString(), entity->GetID());
        }
    }
    else if (spawnCategory_ == SPAWN_TILE)
    {
        if (GameContext::Get().input_->GetQualifierDown(QUAL_CTRL))
        {
            GameHelpers::RemoveTile(position, false, true, true);
            URHO3D_LOGINFOF("MapEditorLibImpl() - SpawnObject : Remove Tile !");
        }
        else
        {
            GameHelpers::AddTile(position);
            URHO3D_LOGINFOF("MapEditorLibImpl() - SpawnObject : Spawn Tile !");
        }
    }
    else if (spawnCategory_ == SPAWN_WATER)
    {
        GameHelpers::AddWater(position, waterQuantity_);
        URHO3D_LOGINFOF("MapEditorLibImpl() - SpawnObject : Spawn Water qty=%F !", waterQuantity_);
    }
    else if (spawnCategory_ == SPAWN_PLANT || spawnCategory_ == SPAWN_DOOR || spawnCategory_ == SPAWN_LIGHT || spawnCategory_ == SPAWN_DECORATION ||
             spawnCategory_ == SPAWN_CONTAINER || spawnCategory_ == SPAWN_COLLECTABLE || spawnCategory_ == SPAWN_TOOL)
    {
        Node* entity = 0;
        if (GOT::GetTypeProperties(spawnObject_) & GOT_Furniture)
        {
            int layerZ = ViewManager::Get()->GetCurrentViewZ();
            if (spawnCategory_ == SPAWN_DOOR)
                layerZ = THRESHOLDVIEW;
            else if (spawnCategory_ == SPAWN_PLANT)
            {
                if (layerZ == FRONTVIEW)
                    layerZ = editorLayerModifier_ > 1 ? FRONTBIOME : editorLayerModifier_ > 0 ? OUTERBIOME : BACKBIOME;
                else if (layerZ == INNERVIEW)
                    layerZ = editorLayerModifier_ > 1 ? FRONTINNERBIOME : BACKINNERBIOME;
            }
            entity = World2D::GetWorld()->SpawnFurniture(spawnObject_, layerZ, spawnCategory_ == SPAWN_PLANT);
        }
        else if (spawnCategory_ == SPAWN_COLLECTABLE)
            entity = World2D::GetWorld()->SpawnCollectable(spawnObject_);
        else
            entity = World2D::GetWorld()->SpawnEntity(spawnObject_);

        if (entity)
        {
            drawable = entity->GetDerivedComponent<Drawable2D>();
            SelectObject(drawable);
            URHO3D_LOGINFOF("MapEditorLibImpl() - SpawnObject : Spawn %s - entity=%s(%u)", SpawnCategoryNameStr[spawnCategory_], entity->GetName().CString(), entity->GetID());
        }
    }

    if (drawable)
    {
        // because the scene is not enable to update, we force by marking in the view and update manually the scene
        drawable->MarkInView(GameContext::Get().renderer2d_->GetCurrentFrameInfo());
        scene_->Update(GameContext::Get().engine_->GetNextTimeStep());
    }
}

bool MapEditorLibImpl::MoveObjects(const Vector2& adjust)
{
    bool moved = false;

    if (adjust.Length() > M_EPSILON)
    {
        for (unsigned i = 0; i < editNodes_.Size(); ++i)
        {
//            if (moveSnap)
//            {
//                float moveStepScaled = moveStep * snapScale;
//                adjust.x = Floor(adjust.x / moveStepScaled + 0.5) * moveStepScaled;
//                adjust.y = Floor(adjust.y / moveStepScaled + 0.5) * moveStepScaled;
//                adjust.z = Floor(adjust.z / moveStepScaled + 0.5) * moveStepScaled;
//            }

            Node* node = editNodes_[i];
            Vector2 nodeAdjust = adjust;
//            if (axisMode == AXIS_LOCAL && editNodes_.Size() == 1)
//                nodeAdjust = node->GetWorldRotation2D() * nodeAdjust;

            Vector2 worldPos = node->GetWorldPosition2D();
            Vector2 oldPos = node->GetPosition2D();

            worldPos += nodeAdjust;

            if (!node->GetParent())
                node->SetPosition2D(worldPos);
            else
                node->SetPosition2D(node->GetParent()->WorldToLocal2D(worldPos));

            if (node->GetPosition2D() != oldPos)
                moved = true;

            // allow Z move for furniture
            if (GOT::GetTypeProperties(StringHash(node->GetVar(GOA::GOT).GetUInt())) & GOT_Furniture)
            {
                Drawable2D* drawable = node->GetDerivedComponent<Drawable2D>();
                int layerZ = drawable->GetLayer();
                if (layerZ != THRESHOLDVIEW) // don't move doors
                {
                    int newlayerZ = layerZ;
                    if (ViewManager::Get()->GetCurrentViewZ() == FRONTVIEW)
                        newlayerZ = editorLayerModifier_ > 1 ? FRONTBIOME : editorLayerModifier_ > 0 ? OUTERBIOME : BACKBIOME;
                    else if (ViewManager::Get()->GetCurrentViewZ() == INNERVIEW)
                        newlayerZ = editorLayerModifier_ > 1 ? FRONTINNERBIOME : BACKINNERBIOME;

                    if (newlayerZ != layerZ)
                    {
                        node->SetVar(GOA::ONVIEWZ, ViewManager::Get()->GetCurrentViewZ());
                        drawable->SetViewMask(ViewManager::GetLayerMask(newlayerZ));
                        drawable->SetLayer(newlayerZ);
                    }
                }
            }
        }
    }

    return moved;
}

void MapEditorLibImpl::DeleteSelectedObjects()
{
    for (Node* node : editNodes_)
    {
        if (node->HasVar(GOA::GOT))
        {
            URHO3D_LOGINFOF("node %s(%u) deleted !", node->GetName().CString(), node->GetID());
            World2D::DestroyEntity(node->GetVar(GOA::ONMAP).GetUInt(), node);
        }
    }

    UnSelectObject();
}

void MapEditorLibImpl::UnSelectObject()
{
    editNodes_.Clear();
    editComponents_.Clear();

    selectedNodes_.Clear();
    selectedComponents_.Clear();

    selectedDrawable_ = 0;
    selectedShape_ = 0;
    selectedRenderShape_ = 0;

    lines_.Clear();
    points_.Clear();

    debugTextRootNode_->SetEnabledRecursive(false);

    SetVisible(PANEL_INSPECTORWINDOW, false);
}

void MapEditorLibImpl::SelectObject(Drawable* drawable)
{
    selectedDrawable_ = drawable;
    selectedShape_ = 0;
    selectedRenderShape_ = 0;

    lines_.Clear();
    points_.Clear();

    debugTextRootNode_->SetEnabledRecursive(false);

    if (selectedDrawable_)
    {
        if (!multiSelection_)
        {
            editNodes_.Clear();
            editComponents_.Clear();
        }

        editNode_ = selectedDrawable_->GetNode();
        editNodes_.Push(editNode_);
        editComponents_.Push(selectedDrawable_);

        URHO3D_LOGINFOF("MapEditor() : SelectObject : drawable=%u editNodes_=%u editComponents=%u", drawable, editNodes_.Size(), editComponents_.Size());

        UpdatePanel(PANEL_INSPECTORWINDOW, true);
        SetVisible(PANEL_INSPECTORWINDOW, true);
    }
}

void MapEditorLibImpl::SelectObject(CollisionShape2D* shape)
{
    selectedDrawable_ = 0;
    selectedShape_ = shape;
    selectedRenderShape_ = 0;

    lines_.Clear();
    points_.Clear();

    debugTextRootNode_->SetEnabledRecursive(false);

    if (selectedShape_)
    {
        URHO3D_LOGINFOF("MapEditor() : SelectObject : collisionshape=%u", shape);

        AddShapeToRender(selectedShape_, Color::GRAY.ToUInt(), Color::YELLOW.ToUInt());

        if (!multiSelection_)
        {
            editNodes_.Clear();
            editComponents_.Clear();
        }

//        editNodes_.Push();
        editComponents_.Push(selectedShape_);

        UpdatePanel(PANEL_INSPECTORWINDOW, true);
        SetVisible(PANEL_INSPECTORWINDOW, true);
    }
}

void MapEditorLibImpl::SelectObject(RenderShape* rendershape, RenderCollider* collider)
{
    selectedDrawable_ = 0;
    selectedShape_ = 0;
    selectedRenderShape_ = rendershape;
    selectedRenderCollider_ = collider;

    lines_.Clear();
    points_.Clear();

    debugTextRootNode_->SetEnabledRecursive(false);

    if (selectedRenderShape_)
    {
        URHO3D_LOGINFOF("MapEditor() : SelectObject : rendershape=%u viewzind=%d viewid=%d colliderz=%d layerz=%d",
                        rendershape, collider->params_->indz_, collider->params_->indv_, collider->params_->colliderz_, collider->params_->layerz_);

        const Vector<PolyShape >& shapes = rendershape->GetShapes();
        if (selectedPolyShapeId_ >= shapes.Size())
            return;
        const PolyShape& shape = shapes[selectedPolyShapeId_];
        const Vector<PODVector<Vector2 > >& contours = shape.contours_;
        if (selectedRenderContourId_ >= contours.Size())
            return;

        AddShapeToRender(selectedRenderShape_, selectRenderShapeColor_, Color::YELLOW.ToUInt(), true);
    }
}

void MapEditorLibImpl::UpdateSelectShape()
{
    lines_.Clear();
    points_.Clear();
    triangles_.Clear();
    contours_.Clear();

    debugTextRootNode_->SetEnabledRecursive(false);

    if (selectedDrawable_)
    {
        if (selectedDrawable_ != AnimatorEditor::Get()->GetAnimatedSprite())
        {
            AddShapeToRender(selectedDrawable_, Color::GRAY.ToUInt(), Color::YELLOW.ToUInt());
        }
    }

    if (AnimatorEditor::Get()->GetAnimatedSprite())
    {
        AddAnimatorBonesToRender();
    }

    if (selectedShape_)
    {
        AddShapeToRender(selectedShape_, Color::GRAY.ToUInt(), Color::YELLOW.ToUInt());
    }

    else if (selectedRenderShape_)
    {
        // reclip debug
        PolyShape& polyshape = selectedRenderShape_->GetShapes()[selectedPolyShapeId_];
        polyshape.Clip(polyshape.clippedRect_, true, selectedRenderShape_->GetNode()->GetWorldTransform2D().Inverse(), true);

        AddShapeToRender(selectedRenderShape_, selectRenderShapeColor_, Color::YELLOW.ToUInt(), true);
    }
}


void MapEditorLibImpl::SubscribeToUIEvent()
{
    SubscribeToEvent(E_SCREENMODE, URHO3D_HANDLER(MapEditorLibImpl, HandleResizeUI));

    // ToolBar
    {
        UIElement* panel = uiPanels_[PANEL_MAINTOOLBAR];
        for (int i = 1; i < MAX_SPAWNCATEGORIES; i++)
            SubscribeToEvent(panel->GetChild(SpawnCategoryNameStr[i], true), E_TOGGLED, URHO3D_HANDLER(MapEditorLibImpl, HandleToolBarCheckBoxToogle));

        DropDownList* layerAlignDdl = static_cast<DropDownList*>(panel->GetChild("LayerAlign", true));
        SubscribeToEvent(layerAlignDdl, E_ITEMSELECTED, URHO3D_HANDLER(MapEditorLibImpl, HandleToolBarLayerAlignChanged));
    }

    // Inspector Window
    {
        UIElement* panel = uiPanels_[PANEL_INSPECTORWINDOW];
        SubscribeToEvent(panel->GetChild("LockButton", true), E_PRESSED, URHO3D_HANDLER(MapEditorLibImpl, HandleToggleInspectorLock));
        SubscribeToEvent(panel, E_LAYOUTUPDATED, URHO3D_HANDLER(MapEditorLibImpl, HandleWindowLayoutUpdated));
    }

    // Animator2D Window

}

void MapEditorLibImpl::HandleToolBarCheckBoxToogle(StringHash eventType, VariantMap& eventData)
{
    CheckBox* checkbox = static_cast<CheckBox*>(eventData["Element"].GetPtr());
    if (!checkbox)
        return;

    int spawncategory = GetEnumValue(checkbox->GetName(), SpawnCategoryNameStr);

    if (checkbox->IsChecked())
    {
        spawnCategory_ = spawncategory;
        uiPanelDirty_[PANEL_MAINTOOLBAR] = true;
    }
    else if (spawncategory == spawnCategory_)
    {
        spawnCategory_ = SPAWN_NONE;
        uiPanelDirty_[PANEL_MAINTOOLBAR] = true;
    }

    if (spawncategory == SPAWN_MONSTER)
        GetPanel(PANEL_MONSTERPOPLIST)->SetVisible(checkbox->IsChecked());
    else if (spawncategory == SPAWN_PLANT)
        GetPanel(PANEL_PLANTPOPLIST)->SetVisible(checkbox->IsChecked());
    else if (spawncategory == SPAWN_DOOR)
        GetPanel(PANEL_DOORPOPLIST)->SetVisible(checkbox->IsChecked());
    else if (spawncategory == SPAWN_LIGHT)
        GetPanel(PANEL_LIGHTPOPLIST)->SetVisible(checkbox->IsChecked());
    else if (spawncategory == SPAWN_DECORATION)
        GetPanel(PANEL_DECORATIONPOPLIST)->SetVisible(checkbox->IsChecked());
    else if (spawncategory == SPAWN_CONTAINER)
        GetPanel(PANEL_CONTAINERPOPLIST)->SetVisible(checkbox->IsChecked());
    else if (spawncategory == SPAWN_COLLECTABLE)
        GetPanel(PANEL_COLLECTABLEPOPLIST)->SetVisible(checkbox->IsChecked());
    else if (spawncategory == SPAWN_TOOL)
        GetPanel(PANEL_TOOLPOPLIST)->SetVisible(checkbox->IsChecked());
}

void MapEditorLibImpl::HandleToolBarLayerAlignChanged(StringHash eventType, VariantMap& eventData)
{
    editorLayerModifier_ = eventData[ItemSelected::P_SELECTION].GetInt();

//    URHO3D_LOGINFOF("MapEditorLibImpl() - HandleToolBarLayerAlignChanged : editorLayerModifier_=%d", editorLayerModifier_);
}

void MapEditorLibImpl::HandleCategoryTypeSelected(StringHash eventType, VariantMap& eventData)
{
    Button* button = static_cast<Button*>(eventData["Element"].GetPtr());
    if (!button)
        return;

    spawnObject_ = button->GetVar(TYPE_VAR).GetStringHash();
}

void MapEditorLibImpl::HandleWindowLayoutUpdated(StringHash eventType, VariantMap& eventData)
{
    if (context_->GetEventSender() == uiPanels_[PANEL_INSPECTORWINDOW])
    {
        UIElement* parentContainer = uiPanels_[PANEL_INSPECTORWINDOW]->GetChild(PARENTCONTAINER, true);
        Inspector_LayoutUpdated(parentContainer);
    }
}

void MapEditorLibImpl::HandleResizeUI(StringHash eventType, VariantMap& eventData)
{
    ResizeUI();
}

void MapEditorLibImpl::HandleHidePanel(StringHash eventType, VariantMap& eventData)
{
    UIElement* elt = static_cast<UIElement*>(context_->GetEventSender());
    while (elt && elt->GetVar(PANEL_ID) == Variant::EMPTY)
        elt = elt->GetParent();

    if (elt)
        SetVisible(elt->GetVar(PANEL_ID).GetInt(), false);
}

void MapEditorLibImpl::HandleToggleInspectorLock(StringHash eventType, VariantMap& eventData)
{
    UIElement* inspector = uiPanels_[PANEL_INSPECTORWINDOW];
    if (!inspector)
        return;

    UIElement* inspectorLockButton = inspector->GetChild("LockButton", true);

    if (!inspectorLockButton)
    {
        inspectorLocked_ = false;
        return;
    }

    if (inspectorLocked_)
    {
        inspectorLocked_ = false;
        inspectorLockButton->SetStyle("Button");
        UpdateInspector(true);
    }
    else
    {
        inspectorLocked_ = true;
        inspectorLockButton->SetStyle("ToggledButton");
    }
}

void MapEditorLibImpl::HandleInspectorResetToDefault(StringHash eventType, VariantMap& eventData)
{
}

void MapEditorLibImpl::HandleInspectorTagsEdit(StringHash eventType, VariantMap& eventData)
{
}

void MapEditorLibImpl::HandleInspectorTagsSelect(StringHash eventType, VariantMap& eventData)
{
}

void MapEditorLibImpl::HandleInspectorEditAttribute(StringHash eventType, VariantMap& eventData)
{
    // TODO : this part doesn't work for the moment

    UIElement* attrEdit = static_cast<UIElement*>(eventData["Element"].GetPtr());
    UIElement* parent = attrEdit->GetParent();

    Vector<Serializable*> serializables;
    GetAttributeEditorTargets(attrEdit, serializables);
    if (serializables.Size())
    {
//        URHO3D_LOGINFOF("HandleInspectorEditAttribute : attrEdit=%s parent=%s num serializables = %u", attrEdit->GetName().CString(), parent->GetName().CString(), serializables.Size());

        for (unsigned i = 0; i < serializables.Size(); ++i)
            serializables[i]->ApplyAttributes();
    }
/*
    // Changing elements programmatically may cause events to be sent. Stop possible infinite loop in that case.
    if (inLoadAttributeEditor)
        return;

    UIElement@ attrEdit = eventData["Element"].GetPtr();
    UIElement@ parent = attrEdit.parent;
    Array<Serializable@>@ serializables = GetAttributeEditorTargets(attrEdit);
    if (serializables.empty)
        return;

    uint index = attrEdit.vars["Index"].GetUInt();
    uint subIndex = attrEdit.vars["SubIndex"].GetUInt();
    uint coordinate = attrEdit.vars["Coordinate"].GetUInt();
    bool intermediateEdit = eventType == TEXT_CHANGED_EVENT_TYPE;

    // Do the editor pre logic before attribute is being modified
    if (!PreEditAttribute(serializables, index))
        return;

    inEditAttribute = true;

    Array<Variant> oldValues;

    if (!dragEditAttribute)
    {
        // Store old values so that PostEditAttribute can create undo actions
        for (uint i = 0; i < serializables.length; ++i)
            oldValues.Push(serializables[i].attributes[index]);
    }

    StoreAttributeEditor(parent, serializables, index, subIndex, coordinate);
    for (uint i = 0; i < serializables.length; ++i)
        serializables[i].ApplyAttributes();

    if (!dragEditAttribute)
    {
        // Do the editor post logic after attribute has been modified.
        PostEditAttribute(serializables, index, oldValues);
    }

    inEditAttribute = false;

    // If not an intermediate edit, reload the editor fields with validated values
    // (attributes may have interactions; therefore we load everything, not just the value being edited)
    if (!intermediateEdit)
        attributesDirty = true;
*/
}

void MapEditorLibImpl::HandleInspectorPickResource(StringHash eventType, VariantMap& eventData)
{
}

void MapEditorLibImpl::HandleInspectorOpenResource(StringHash eventType, VariantMap& eventData)
{
}

void MapEditorLibImpl::HandleInspectorEditResource(StringHash eventType, VariantMap& eventData)
{
    UIElement* button = static_cast<UIElement*>(eventData["Element"].GetPtr());
    LineEdit* attrEdit = static_cast<LineEdit*>(button->GetParent()->GetChild(0));

    String fileName = attrEdit->GetText().Trimmed();
    if (fileName.Empty())
        return;

    StringHash resourceType(attrEdit->GetVar(TYPE_VAR).GetUInt());
    Resource* resource = GameContext::Get().resourceCache_->GetResource(resourceType, fileName);

    EditResource(resource);
}

void MapEditorLibImpl::HandleInspectorTestResource(StringHash eventType, VariantMap& eventData)
{
}


void MapEditorLibImpl::EditResource(Resource* resource)
{
    if (!resource)
        return;

    URHO3D_LOGINFOF("MapEditorLibImpl() - EditResource : type=%s name=%s ... Launch Editor ...", resource->GetTypeName().CString(), resource->GetName().CString());

    if (resource->IsInstanceOf<AnimationSet2D>())
    {
//        AnimatorEditor::Get()->Edit(static_cast<AnimationSet2D*>(resource));
        AnimatorEditor::Get()->Edit(editNode_);
    }
    else if (resource->IsInstanceOf<ParticleEffect2D>())
    {
//        ParticleEffect2DEditor::Get()->Open(static_cast<ParticleEffect2D*>(resource));
    }
    else if (resource->IsInstanceOf<Sprite2D>())
    {
//        Sprite2D* editResource = static_cast<Sprite2D*>(resource);
    }
    else if (resource->IsInstanceOf<SpriteSheet2D>())
    {
//        SpriteSheet2D* editResource = static_cast<SpriteSheet2D*>(resource);
    }
    else
        URHO3D_LOGWARNINGF("MapEditorLibImpl() - EditResource : type=%s name=%s ... Can't Find an Editor ...", resource->GetTypeName().CString(), resource->GetName().CString());
}


void MapEditorLibImpl::Render()
{
    UpdateSelectShape();

    if (selectedRenderShape_)
    {
        PolyShape& polyshape = selectedRenderShape_->GetShapes()[selectedPolyShapeId_];
        if (polyshape.clippedRect_ != lastClippedRect_)
            lastClippedRect_ = polyshape.clippedRect_;
    }

    if (lines_.Empty() && points_.Empty() && triangles_.Empty() && contours_.Empty())
        return;

    Graphics* graphics = scene_->GetContext()->GetSubsystem<Graphics>();

    // Engine does not render when window is closed or device is lost
    assert(graphics && graphics->IsInitialized() && !graphics->IsDeviceLost());

#ifdef URHO3D_VULKAN
    ShaderVariation* vs = graphics->GetShader(VS, "Basic");
    ShaderVariation* ps = graphics->GetShader(PS, "Basic");
#else
    ShaderVariation* vs = graphics->GetShader(VS, "Basic", "VERTEXCOLOR");
    ShaderVariation* ps = graphics->GetShader(PS, "Basic", "VERTEXCOLOR");
#endif

    const unsigned numVertices = points_.Size() + lines_.Size() * 2 + triangles_.Size() * 3 + contours_.Size() * 2;

    // Resize the vertex buffer if too small or much too large
    if (vertexBuffer_->GetVertexCount() < numVertices || vertexBuffer_->GetVertexCount() > numVertices * 2)
        vertexBuffer_->SetSize(numVertices, MASK_POSITION | MASK_COLOR, true);

    float* dest = (float*)vertexBuffer_->Lock(0, numVertices, true);
    if (!dest)
        return;

    for (unsigned i = 0; i < lines_.Size(); ++i)
    {
        const EditorLine& line = lines_[i];

        dest[0] = line.start_.x_;
        dest[1] = line.start_.y_;
        dest[2] = 0.f;
        ((unsigned&)dest[3]) = line.color_;
        dest[4] = line.end_.x_;
        dest[5] = line.end_.y_;
        dest[6] = 0.f;
        ((unsigned&)dest[7]) = line.color_;

        dest += 8;
    }

    for (unsigned i = 0; i < points_.Size(); ++i)
    {
        const EditorPoint& point = points_[i];

        dest[0] = point.point_.x_;
        dest[1] = point.point_.y_;
        dest[2] = 0.f;
        ((unsigned&)dest[3]) = point.color_;

        dest += 4;
    }

    for (unsigned i = 0; i < triangles_.Size(); ++i)
    {
        const EditorTriangle& triangle = triangles_[i];

        dest[0] = triangle.v0_.x_;
        dest[1] = triangle.v0_.y_;
        dest[2] = 0.f;
        ((unsigned&)dest[3]) = triangle.color_;
        dest[4] = triangle.v1_.x_;
        dest[5] = triangle.v1_.y_;
        dest[6] = 0.f;
        ((unsigned&)dest[7]) = triangle.color_;
        dest[8] = triangle.v2_.x_;
        dest[9] = triangle.v2_.y_;
        dest[10] = 0.f;
        ((unsigned&)dest[11]) = triangle.color_;

        dest += 12;
    }

    for (unsigned i = 0; i < contours_.Size(); ++i)
    {
        const EditorLine& line = contours_[i];

        dest[0] = line.start_.x_;
        dest[1] = line.start_.y_;
        dest[2] = 0.f;
        ((unsigned&)dest[3]) = line.color_;
        dest[4] = line.end_.x_;
        dest[5] = line.end_.y_;
        dest[6] = 0.f;
        ((unsigned&)dest[7]) = line.color_;

        dest += 8;
    }

    Camera* camera = scene_->GetContext()->GetSubsystem<Renderer>()->GetViewport(0)->GetCamera();
    Matrix3x4 view = camera->GetView();
    Matrix3x4 gpuProjection = camera->GetGPUProjection();

    vertexBuffer_->Unlock();

    graphics->SetBlendMode(BLEND_ALPHA);
    graphics->SetColorWrite(true);
    graphics->SetCullMode(CULL_NONE);
    graphics->SetDepthWrite(false);
    graphics->SetLineAntiAlias(false);
    graphics->SetScissorTest(false);
    graphics->SetStencilTest(false);
    graphics->SetVertexBuffer(vertexBuffer_);
    graphics->SetShaders(vs, ps);
    graphics->SetShaderParameter(VSP_MODEL, Matrix3x4::IDENTITY);
    graphics->SetShaderParameter(VSP_VIEW, view);
    graphics->SetShaderParameter(VSP_VIEWINV, view.Inverse());
    graphics->SetShaderParameter(VSP_VIEWPROJ, gpuProjection * view);
    graphics->SetShaderParameter(PSP_MATDIFFCOLOR, Color(1.0f, 1.0f, 1.0f, 1.0f));

    unsigned start = 0;
    unsigned count = 0;

    if (lines_.Size())
    {
#if defined(MAPEDITOR_OGL)
        graphics->SetLineWidth(linewidth);
#endif
        count = lines_.Size() * 2;
        graphics->Draw(LINE_LIST, start, count);
        start += count;
    }

    if (points_.Size())
    {
#if defined(MAPEDITOR_OGL)
        glPointSize(pointsize);
#endif

        count = points_.Size();
        graphics->Draw(POINT_LIST, start, count);
        start += count;
    }

    if (triangles_.Size())
    {
#if defined(MAPEDITOR_OGL)        
        graphics->SetLineWidth(1.f);
#endif
        count = triangles_.Size() * 3;
        graphics->Draw(TRIANGLE_LIST, start, count);
        start += count;
    }

    if (contours_.Size())
    {
#if defined(MAPEDITOR_OGL)        
        graphics->SetLineWidth(2.5f);
#endif        
        graphics->SetLineAntiAlias(true);

        count = contours_.Size() * 2;
        graphics->Draw(LINE_LIST, start, count);
        start += count;
    }

    graphics->SetDepthWrite(false);
    graphics->SetLineAntiAlias(false);
#if defined(MAPEDITOR_OGL)    
    graphics->SetLineWidth(1.f);
#endif
}

void MapEditorLibImpl::AddShapeToRender(Drawable* shape, unsigned linecolor, unsigned pointcolor)
{
    BoundingBox boundingRect = shape->GetWorldBoundingBox();

    PODVector<Vector2> boundingRectShape(4);
    boundingRectShape[0] = boundingRect.min_.ToVector2();
    boundingRectShape[1] = Vector2(boundingRect.min_.x_, boundingRect.max_.y_);
    boundingRectShape[2] = boundingRect.max_.ToVector2();
    boundingRectShape[3] = Vector2(boundingRect.max_.x_, boundingRect.min_.y_);
    AddVerticesToRender(boundingRectShape, Matrix2x3::IDENTITY, linecolor, pointcolor, false, false);
}

void MapEditorLibImpl::AddShapeToRender(CollisionShape2D* shape, unsigned linecolor, unsigned pointcolor)
{
    if (!shape->IsInstanceOf<CollisionChain2D>())
        return;

    CollisionChain2D* chain = static_cast<CollisionChain2D*>(shape);

    AddVerticesToRender(chain->GetVertices(), chain->GetNode()->GetWorldTransform2D(), linecolor, pointcolor);
}

void MapEditorLibImpl::AddShapeToRender(RenderShape* renderShape, unsigned linecolor, unsigned pointcolor, bool addBoundingRect)
{
    if (!renderShape)
        return;

    const Vector<PolyShape >& shapes = renderShape->GetShapes();

    if (selectedPolyShapeId_ >= shapes.Size())
        return;

    const PolyShape& shape = shapes[selectedPolyShapeId_];
    const Vector<PODVector<Vector2 > >& contours = selectRenderShapeClipped_ ? shape.clippedContours_ : shape.contours_;
    const Vector<Vector<PODVector<Vector2 > > >& gholes = selectRenderShapeClipped_ ? shape.clippedHoles_ : shape.holes_;

    if (selectedRenderContourId_ >= contours.Size())
        return;

    // show boundingRect
    if (addBoundingRect)
    {
        PODVector<Vector2> boundingRectShape(4);

        if (shape.IsClipped())
        {
            boundingRectShape[0] = shape.clippedRect_.min_;
            boundingRectShape[1] = Vector2(shape.clippedRect_.min_.x_, shape.clippedRect_.max_.y_);
            boundingRectShape[2] = shape.clippedRect_.max_;
            boundingRectShape[3] = Vector2(shape.clippedRect_.max_.x_, shape.clippedRect_.min_.y_);
            AddVerticesToRender(boundingRectShape, Matrix2x3::IDENTITY, Color::YELLOW.ToUInt(), pointcolor, false, false);
        }

        const Rect& boundingRect = shape.boundingRects_[Min(selectedRenderContourId_, shape.boundingRects_.Size()-1)];

        boundingRectShape[0] = boundingRect.min_;
        boundingRectShape[1] = Vector2(boundingRect.min_.x_, boundingRect.max_.y_);
        boundingRectShape[2] = boundingRect.max_;
        boundingRectShape[3] = Vector2(boundingRect.max_.x_, boundingRect.min_.y_);
        AddVerticesToRender(boundingRectShape, renderShape->GetNode()->GetWorldTransform2D(), Color::BLUE.ToUInt(), pointcolor, false, false);
    }

    AddVerticesToRender(contours[selectedRenderContourId_], renderShape->GetNode()->GetWorldTransform2D(), linecolor, pointcolor);
    if (selectedRenderContourId_ < gholes.Size())
    {
        const Vector<PODVector<Vector2 > >& holes = gholes[selectedRenderContourId_];
        for (unsigned i=0; i < holes.Size(); i++)
            AddVerticesToRender(holes[i], renderShape->GetNode()->GetWorldTransform2D(), Color(1.f, (float)i / holes.Size(), 0.f, 1.f).ToUInt(), pointcolor, false, true, "H"+String(i)+"_");
    }
}

void MapEditorLibImpl::AddAnimatorBonesToRender()
{
    const Vector<SharedPtr<BoneGizmo> >& gizmos = AnimatorEditor::Get()->GetBonesGizmos();
    const float scale = AnimatorEditor::Get()->GetAnimatedSprite()->GetNode()->GetWorldScale2D().x_;
    for (unsigned i=0; i < gizmos.Size(); i++)
        gizmos[i]->Render(triangles_, contours_, scale);
}

void MapEditorLibImpl::AddVerticesToRender(const PODVector<Vector2>& vertices, const Matrix2x3& transform, unsigned linecolor, unsigned pointcolor, bool addpoints, bool addpointtext, const String& tag)
{
    Vector2 point0, point1;
    unsigned i, j;
    for (i = 0, j = vertices.Size()-1; i < vertices.Size(); j = i++)
    {
        point0 = transform * vertices[j];
        point1 = transform * vertices[i];
        lines_.Push(EditorLine(point0, point1, linecolor));

        if (addpoints)
            points_.Push(EditorPoint(point1, pointcolor));

        if (addpointtext)
            AddDebugText(tag+String(i), point1 + GameHelpers::GetManhattanVector(i, vertices) * 0.2f);
    }

    if (addpointtext)
        debugTextRootNode_->SetEnabled(true);
}

void MapEditorLibImpl::AddDebugText(const String& text, const Vector2& position)
{
    Text3D* text3D;
    Node* node = debugTextRootNode_->GetChild(text);
    if (!node)
    {
        Font* font = debugTextRootNode_->GetContext()->GetSubsystem<ResourceCache>()->GetResource<Font>("Fonts/Anonymous Pro.ttf");

        node = debugTextRootNode_->CreateChild(text, LOCAL);

        text3D = node->CreateComponent<Text3D>(LOCAL);
        text3D->SetColor(Color::YELLOW);
        text3D->SetFont(font, 32);
        text3D->SetOccludee(false);
        text3D->SetAlignment(HA_CENTER, VA_CENTER);
    }
    else
    {
        text3D = node->GetComponent<Text3D>(LOCAL);
    }

    node->SetWorldPosition(Vector3(position.x_, position.y_, 0.f));
    text3D->SetText(text);

    node->SetEnabled(true);
}


void MapEditorLibImpl::Clean()
{
    // When the amount of debug geometry is reduced, release memory
    unsigned linesSize = lines_.Size();

    lines_.Clear();

    if (lines_.Capacity() > linesSize * 2)
        lines_.Reserve(linesSize);
}


//void AnimatorEditor::CreateUI()
//{
//    // UI
//    //  view3d with toolbar : buttons Show bones, Show sprites, Show triggers (point,box)
//    //  timeline panel
//    //  animation list panel
//    //  bones graph panel
//    //  sprites Z-order list panel
//    //  object attributes (x,y,angle,scale,COLOR(new),alpha,etc...)
//    //  charactermap panel
//    //  contextual sprites list : spritename filter, tag filter
//}

AnimatorEditor::AnimatorEditor(Context* context)  : Object(context)
{
    URHO3D_LOGDEBUGF("AnimatorEditor()");
    mainScene_ = GameContext::Get().rootScene_;
    lastSelectedTimelineId_ = lastHoverTimelineId_ = -1;
    lastSelectedBoneIndex_ = -1;
}

AnimatorEditor::~AnimatorEditor()
{
    URHO3D_LOGDEBUGF("~AnimatorEditor()");

    FinishEdit();

    GameContext::Get().Pause(false);

    animatorEditor_ = 0;
}

static Vector2 nodeoriginscale_;
static Vector2 nodeeditscale_;

void AnimatorEditor::Edit(Node* node)
{
    // if a node is currently in edition, Finish To Edit
    FinishEdit();

    URHO3D_LOGINFOF("AnimatorEditor() - Edit : node=%s(%u) ... ", node->GetName().CString(), node->GetID());

    // Add the EditNode in the main scene
    AnimatedSprite2D* animatedSprite = node->GetComponent<AnimatedSprite2D>();
    if (!animatedSprite)
    {
        URHO3D_LOGERRORF("AnimatorEditor() - Edit : node=%s(%u) ... Has no AnimatedSprite2D ! ", node->GetName().CString(), node->GetID());
        return;
    }

    // Edit Node and Rescale
    editedAnimatedSprite_ = animatedSprite;
    editNode_ = node;
    nodeoriginscale_ = editNode_->GetScale2D();
    nodeeditscale_ = nodeoriginscale_ * 2;
    editNode_->SetScale2D(nodeeditscale_);
    entityid_ = animatedSprite->GetSpriterEntityIndex();
    SetCharacterMappingPanel();

    isPlaying_ = false;
}

void AnimatorEditor::EditInPreview(AnimationSet2D* animationSet)
{
    URHO3D_LOGINFO("AnimatorEditor() - Edit : animationset2d = " + animationSet->GetName());

    // Edit an AnimationSet2D inside scmleditor

    panel_ = MapEditorLibImpl::GetPanel(PANEL_ANIMATOR2D_MAIN);
    preview_ = static_cast<View3D*>(panel_->GetChild(String("View2D"), true));

    if (!previewScene_)
    {
        previewScene_ = new Scene(context_);
        previewScene_->CreateComponent<Octree>();
        previewScene_->CreateComponent<Renderer2D>();
        Node* cameraNode = previewScene_->CreateChild("Camera", LOCAL);
        Camera* camera = cameraNode->CreateComponent<Camera>();
        camera->SetNearClip(-2.f);
        camera->SetFarClip(2.f);
        camera->SetFov(60.f);
        camera->SetOrthographic(true);
        cameraNode->SetPosition(Vector3(0, 0, -1.f));
        cameraNode->LookAt(Vector3(0, 0, 0));
    }

    Node* editNode = previewScene_->GetChild("EditNode", LOCAL);
    if (!editNode)
        editNode = previewScene_->CreateChild("EditNode", LOCAL);

//    GameHelpers::AddStaticSprite2D("sprite", editNode, "Textures/Actors/particules.png", 0, Vector3::ZERO, Vector2::ONE / PIXEL_SIZE, LOCAL);
    AnimatedSprite2D* animatedSprite = editNode->GetOrCreateComponent<AnimatedSprite2D>();

    // spine test
    #if (0)
    {
        editNode->SetPosition2D(Vector2(0.f, -8.f));
        editNode->SetScale2D(Vector2(1.5f, 1.5f));
        animationSet = GameContext::Get().resourceCache_->GetResource<AnimationSet2D>("2D/spine/raptor-pro.json");
        animatedSprite->enableDebugLog_ = true;
        animatedSprite->SetAnimationSet(animationSet);
        animatedSprite->SetEntity("default");
        animatedSprite->SetAnimation("walk");
        editNode->SetEnabled(false);
        editNode->SetEnabled(true);
    }
    #else
    // spriter
    {
        editNode->SetPosition2D(Vector2(0.f, -8.f));
        editNode->SetScale2D(Vector2(8.f, 8.f));
//        animatedSprite->SetCustomMaterial(GameContext::Get().layerMaterials_[LAYERACTORS]);
        animatedSprite->SetAnimationSet(animationSet);
        animatedSprite->SetSpriterEntity(0);
        animatedSprite->SetSpriterAnimation(0);
        animatedSprite->ResetCharacterMapping();
        animatedSprite->ApplyCharacterMap(CMAP_NAKED);
        animatedSprite->ApplyCharacterMap(CMAP_HEAD1);
        animatedSprite->ApplyCharacterMap(CMAP_NOHELMET);
        animatedSprite->ApplyCharacterMap(CMAP_NOARMOR);
        animatedSprite->ApplyCharacterMap(CMAP_NOWEAPON1);
        animatedSprite->ApplyCharacterMap(CMAP_NOWEAPON2);
    }
    #endif

    preview_->SetView(previewScene_, previewScene_->GetChild("Camera")->GetComponent<Camera>(), true);
    preview_->GetViewport()->SetRenderPath(GameContext::Get().renderer_->GetViewport(0)->GetRenderPath());

    SubscribeToEvent(preview_, E_RESIZED, URHO3D_HANDLER(AnimatorEditor, HandleResize));
    SubscribeToEvent(panel_, E_VISIBLECHANGED, URHO3D_HANDLER(AnimatorEditor, HandleVisible));

    MapEditorLibImpl::Get()->SetVisible(PANEL_ANIMATOR2D_MAIN, true);
}

void AnimatorEditor::FinishEdit()
{
    if (editNode_)
    {
        URHO3D_LOGINFOF("AnimatorEditor() - FinishEdit : node=%s(%u) ... ", editNode_->GetName().CString(), editNode_->GetID());

        ResetSelection(RESETALLSELECT);

        lastSelectedBoneIndex_ = -1;

        editNode_->SetScale2D(nodeoriginscale_);
        editNode_.Reset();
        editedAnimatedSprite_.Reset();
    }
}

static SpriteDebugInfo lastSelectInfo_, lastHoverInfo_;

void AnimatorEditor::ResetSelection(int mode)
{
    if (!editedAnimatedSprite_)
        return;

    bool cleared = false;

    if (lastHoverTimelineRefs_.Size() && (mode == RESETALLSELECT || mode == RESETHOVERSELECT))
    {
        for (unsigned i=0; i < lastHoverTimelineRefs_.Size(); i++)
            lastHoverTimelineRefs_[i]->color_ = lastHoverTimelineRefsColors_[i];

        lastHoverTimelineRefs_.Clear();
        lastHoverTimelineRefsColors_.Clear();
        lastHoverTimelineId_ = -1;
        cleared = true;
    }
    if (lastSelectedTimelineRefs_.Size() && (mode == RESETALLSELECT || mode == RESETCLICKSELECT))
    {
        for (unsigned i=0; i < lastSelectedTimelineRefs_.Size(); i++)
            lastSelectedTimelineRefs_[i]->color_ = lastSelectedTimelineRefsColors_[i];

        lastSelectedTimelineRefs_.Clear();
        lastSelectedTimelineRefsColors_.Clear();
        lastSelectedTimelineId_ = -1;
        cleared = true;
    }
    if (cleared && editedAnimatedSprite_->GetSpriterInstance()->GetCurrentMainKey())
    {
        editedAnimatedSprite_->GetSpriterInstance()->UpdateTimelineKeys();
        editedAnimatedSprite_->SetColorDirty();
    }
}

static int addingSpriterObjectTimelineId_ = -1;
static int addingSpriterObjectTimelineParentId_ = -1;
static Vector2 addingSpriterObjectTimelinePosition_;

void AnimatorEditor::Update(const Vector2& worldposition)
{
    if (GameContext::Get().input_->GetMouseButtonDown(MOUSEB_LEFT))
    {
        if (GameContext::Get().input_->GetQualifierDown(QUAL_ALT))
            AddSpriterObject(Spriter::BONE, worldposition, false);
        else
            Select(worldposition, GameContext::Get().input_->GetQualifierDown(QUAL_SHIFT), false);
    }
    else
    {
        if (addingSpriterObjectTimelineId_ != -1)
            AddSpriterObject(Spriter::BONE, worldposition, true);
        else
            Select(worldposition, GameContext::Get().input_->GetQualifierDown(QUAL_SHIFT), true);
    }
}

void AnimatorEditor::Select(const Vector2& worldposition, bool findbottomsprite, bool hover)
{
    // search a bone to select at worldposition
    if (!hover)
    {
        const unsigned numbones = editedAnimatedSprite_->GetSpriterInstance()->GetNumBoneKeys();
        for (unsigned iSelectedBone=0; iSelectedBone < numbones; iSelectedBone++)
        {
            BoneGizmo* bone = bonesGizmos_[iSelectedBone].Get();
            int triangleid = bone->IsInside(worldposition);
            if (triangleid != -1)
            {
                bonesGizmos_[iSelectedBone]->SetSelected(true);
                if (lastSelectedBoneIndex_ != -1 && iSelectedBone != lastSelectedBoneIndex_)
                    bonesGizmos_[lastSelectedBoneIndex_]->SetSelected(false);

                lastSelectedBoneIndex_ = iSelectedBone;
//                URHO3D_LOGINFOF("AnimatorEditor() - Select : hovering=%s boneindex=%d ", hover ? "true":"false", lastSelectedBoneIndex_);

                lastSelectBoneMoveMode_ = triangleid == 1 ? GE_ROTATE : GE_TRANSLATE;

                UpdateTimeKeysIcons();
                UpdateTimelineInfos();
                // found one
                return;
            }
        }

        // unselect lastbone if clic outside bones
        if (!hover && lastSelectedBoneIndex_ != -1)
        {
            bonesGizmos_[lastSelectedBoneIndex_]->SetSelected(false);
            lastSelectedBoneIndex_ = -1;
        }
    }

    // search a sprite to select
    {
        const Vector2 position = editedAnimatedSprite_->GetNode()->GetWorldTransform2D().Inverse() * worldposition;

        const float minalpha = 0.9f;
        SpriteDebugInfo& info = !hover ? lastSelectInfo_ : lastHoverInfo_;
        if (editedAnimatedSprite_->GetSpriteAt(position, findbottomsprite, minalpha, info))
        {
            // add debug drawrect
            if (hover)
            {
                DebugRenderer* debugRenderer = mainScene_->GetComponent<DebugRenderer>();
                const unsigned uintColor = Color::YELLOW.ToUInt();
                debugRenderer->AddLine(info.vertices_[0], info.vertices_[1], uintColor, false);
                debugRenderer->AddLine(info.vertices_[1], info.vertices_[2], uintColor, false);
                debugRenderer->AddLine(info.vertices_[2], info.vertices_[3], uintColor, false);
                debugRenderer->AddLine(info.vertices_[3], info.vertices_[0], uintColor, false);
            }

            int& lastTimelineId = !hover ? lastSelectedTimelineId_ : lastHoverTimelineId_;
            PODVector<Spriter::Ref*>& lastTimelineRefs = !hover ? lastSelectedTimelineRefs_ : lastHoverTimelineRefs_;
            PODVector<Color>& lastTimelineRefsColors =  !hover ? lastSelectedTimelineRefsColors_ : lastHoverTimelineRefsColors_;

            // get the Timeline Id
            Spriter::SpriteTimelineKey* spriteKey = editedAnimatedSprite_->GetSpriteKeys()[info.spriteindex_];
            if (lastTimelineId != spriteKey->timeline_->id_ && spriteKey->timeline_->objectType_ == Urho3D::Spriter::SPRITE)
            {
                ResetSelection(hover ? RESETHOVERSELECT : RESETCLICKSELECT);
                if (spriteKey->timeline_->id_ == lastHoverTimelineId_)
                    ResetSelection(RESETHOVERSELECT);

                if (!hover || spriteKey->timeline_->id_ != lastSelectedTimelineId_)
                {
                    lastTimelineId = spriteKey->timeline_->id_;

                    // change the color of the refs to green
                    lastTimelineRefs.Clear();
                    editedAnimatedSprite_->GetSpriterInstance()->GetAnimation()->GetObjectRefs(lastTimelineId, lastTimelineRefs);
                    for (unsigned i=0; i < lastTimelineRefs.Size(); i++)
                    {
                        lastTimelineRefsColors.Push(lastTimelineRefs[i]->color_);
                        lastTimelineRefs[i]->color_ = hover ? Color::GRAY : Color::GREEN;
                    }
                    editedAnimatedSprite_->GetSpriterInstance()->UpdateTimelineKeys();
                    editedAnimatedSprite_->SetColorDirty();
                    URHO3D_LOGINFOF("AnimatorEditor() - Select : hovering=%s index=%u sprite=%s timeline=%d numrefs=%u", hover ? "true":"false", info.spriteindex_, info.sprite_->GetName().CString(), lastTimelineId, lastTimelineRefs.Size());
                }
            }
        }
        else
        {
            ResetSelection(hover ? RESETHOVERSELECT : RESETCLICKSELECT);
        }
    }

    UpdateTimeKeysIcons();
    UpdateTimelineInfos();
}

void AnimatorEditor::AdjustSpinAt(Spriter::Timeline* timeline, unsigned key)
{
    // force the spin to 1 : if the behavior is not the required by the user, he can change the spin manually later.
    if (timeline->keys_.Size() == 1)
    {
        timeline->keys_.Front()->info_.spin = 1;
        return;
    }

    // when establishing the spin value, we try to have the smallest delta angle (< 180.f)
    Spriter::SpatialTimelineKey* tKey = timeline->keys_[key];
    Spriter::SpatialTimelineKey* nextKey = timeline->keys_[key+1 < timeline->keys_.Size() ? key+1 : 0];

    float angle = tKey->info_.angle_;
    float nextAngle = nextKey->info_.angle_;
    if (nextAngle > angle)
        tKey->info_.spin = (nextAngle - angle < 180.f) ? 1 : -1;
    else if (nextAngle < angle)
        tKey->info_.spin = (angle - nextAngle < 180.f) ? -1 : 1;
}

void AnimatorEditor::MoveSelection(const Vector2& worldposition, const Vector2& delta, int mode)
{
    if (lastSelectedBoneIndex_ != -1)
    {
        int selectedBoneTimelineId = editedAnimatedSprite_->GetSpriterInstance()->GetBoneKeys()[lastSelectedBoneIndex_]->timeline_->id_;
        Spriter::Timeline* timeline = editedAnimatedSprite_->GetSpriterInstance()->GetAnimation()->timelines_[selectedBoneTimelineId];

        // Update the move mode
        if (GameContext::Get().input_->GetMouseButtonPress(MOUSEB_RIGHT))
            lastSelectBoneMoveMode_ = bonesGizmos_[lastSelectedBoneIndex_]->IsInside(worldposition) == 0 ? GE_TRANSLATE : GE_ROTATE;

        // Get the current TimelineKey
        float time = editedAnimatedSprite_->GetCurrentAnimationTime();
        Spriter::BoneTimelineKey* tKey = static_cast<Spriter::BoneTimelineKey*>(timeline->GetTimeKey(time));
        if (tKey)
        {
            // Add new Key ?
            if (tKey->time_ != editedAnimatedSprite_->GetCurrentAnimationTime())
            {
                ;
            }

            // Flip delta in accordance with animatedsprite
            Vector2 ldelta = delta;
            if (editedAnimatedSprite_->GetFlipX())
                ldelta.x_ = -ldelta.x_;
            if (editedAnimatedSprite_->GetFlipY())
                ldelta.y_ = -ldelta.y_;

            // Scale the delta
            ldelta *= editedAnimatedSprite_->GetNode()->GetWorldScale2D();

            // Get the parent"s transform for the timeline
            Spriter::SpatialInfo sinfo;
            editedAnimatedSprite_->GetSpriterInstance()->GetAnimation()->UnMapToRoot(tKey, time, false, sinfo);
            if (sinfo.angle_ >= 360.f)
                sinfo.angle_ -= 360.f;
            else if (sinfo.angle_ < 0.f)
                sinfo.angle_ += 360.f;
            Matrix2x3 transform(Vector2::ZERO, -sinfo.angle_, Vector2(sinfo.scaleX_, sinfo.scaleY_));

            // The Delta is now in the parent referential
            ldelta = transform * ldelta;

            Spriter::SpatialInfo& spatialinfo = tKey->info_;

            if (mode == GE_SCALE)
            {
                ;
            }
            else if (lastSelectBoneMoveMode_ == GE_ROTATE)
            {
                // set new angle
                spatialinfo.angle_ += Sign(Abs(delta.y_) > Abs(delta.x_) ? delta.y_ : delta.x_) * ldelta.Length();

                // adjust angle
                if (spatialinfo.angle_ >= 360.f)
                    spatialinfo.angle_ -= 360.f;
                else if (spatialinfo.angle_ < 0.f)
                    spatialinfo.angle_ += 360.f;

                // adjust the spins
                AdjustSpinAt(timeline, tKey->id_);
                AdjustSpinAt(timeline, tKey->id_ > 0 ? tKey->id_-1 : timeline->keys_.Size()-1);
            }
            else
            {
                // set new position
                spatialinfo.x_ += ldelta.x_ * 0.5f;
                spatialinfo.y_ += ldelta.y_ * 0.5f;
            }

            if (!isPlaying_)
            {
                editedAnimatedSprite_->ForceUpdateBatches();
                editedAnimatedSprite_->GetSpriterInstance()->ResetCurrentTime();
                editedAnimatedSprite_->UpdateAnimation(time);

                UpdateTimelineInfos();
                UpdateBonesGizmos();
            }
        }
    }
    else
    {
        Spriter::Timeline* timeline = editedAnimatedSprite_->GetSpriterInstance()->GetAnimation()->timelines_[lastSelectedTimelineId_];
        if (timeline->objectType_ == Urho3D::Spriter::SPRITE)
        {
            // Get the current edited cmap and retrieve the slot to be edited
            UIElement* panel = MapEditorLibImpl::GetPanel(PANEL_ANIMATOR2D_SWSPRITES);
            LineEdit* lineEdit = static_cast<LineEdit*>(panel->GetChild("CMapName", true));
            if (lineEdit->GetText().Empty())
                return;

            Spriter::CharacterMap* spritemap = editedAnimatedSprite_->GetCharacterMap(StringHash(lineEdit->GetText()));
            if (!spritemap)
                return;

            // Get the current TimelineKey
            Spriter::SpriteTimelineKey* tKey = static_cast<Spriter::SpriteTimelineKey*>(timeline->GetTimeKey(editedAnimatedSprite_->GetCurrentAnimationTime()));

            // Find the sprite in the charactermap
            Spriter::MapInstruction* instruct = spritemap->GetInstruction(tKey->folderId_, tKey->fileId_);
            if (!instruct)
                return;

            // Set the targetdx, targetdy
            if (mode == GE_SCALE)
            {
                instruct->targetscalex_ += delta.x_ / editedAnimatedSprite_->GetNode()->GetWorldScale2D().x_ * 0.1f;
                instruct->targetscaley_ += delta.y_ / editedAnimatedSprite_->GetNode()->GetWorldScale2D().y_ * 0.1f;
            }
            else if (mode == GE_ROTATE)
            {
                instruct->targetdangle_ += delta.y_;
            }
            else
            {
                instruct->targetdx_ += delta.x_ / editedAnimatedSprite_->GetNode()->GetWorldScale2D().x_;
                instruct->targetdy_ += delta.y_ / editedAnimatedSprite_->GetNode()->GetWorldScale2D().y_;
            }

            UpdateSpriteMappingListItem(GetSpriteMappingListItem(Spriter::GetKey(tKey->folderId_, tKey->fileId_)), instruct);

            if (!isPlaying_)
            {
                UpdateTimelineInfos();
                editedAnimatedSprite_->UpdateAnimation(0.f);
            }
        }
    }
}

void AnimatorEditor::UpdateMapping(bool updatesprites, bool updatecolors)
{
    if (!editedAnimatedSprite_)
        return;

    UIElement* panel = MapEditorLibImpl::GetPanel(PANEL_ANIMATOR2D_CMAPS);
    ListView* appliedMaps = static_cast<ListView*>(panel->GetChild("AppliedMaps", true));

    editedAnimatedSprite_->ResetCharacterMapping(false);
    for (unsigned i=0; i < appliedMaps->GetNumItems(); i++)
    {
        Text* item = static_cast<Text*>(appliedMaps->GetItem(i));
        if (item->HasTag("Color"))
        {
            if (updatecolors)
            {
                URHO3D_LOGINFOF("AnimatorEditor() - UpdateMapping : Apply Color Map = %s", item->GetText().CString());
                editedAnimatedSprite_->ApplyColorMap(item->GetText());
            }
        }
        else if (updatesprites)
            editedAnimatedSprite_->ApplyCharacterMap(item->GetText());
    }
}


/// Main Character Mapping Panel

UIElement* AnimatorEditor::AddMap(const String& mapname)
{
    ListView* availableMaps = static_cast<ListView*>(MapEditorLibImpl::GetPanel(PANEL_ANIMATOR2D_CMAPS)->GetChild("AvailableMaps", true));
    Text* item = new Text(context_);
    item->SetStyle("FileSelectorListText");
    item->SetText(mapname);
    availableMaps->AddItem(item);
    SubscribeToEvent(item, E_DOUBLECLICK, URHO3D_HANDLER(AnimatorEditor, HandleEditCMap));
    return item;
}

UIElement* AnimatorEditor::ApplyMap(const String& mapname, bool isAColorMap)
{
    ListView* appliedMaps = static_cast<ListView*>(MapEditorLibImpl::GetPanel(PANEL_ANIMATOR2D_CMAPS)->GetChild("AppliedMaps", true));

    Text* item = new Text(context_);
    item->SetDefaultStyle(MapEditorLibImpl::GetUIStyle(UISTYLE_EDITORDEFAULT));
    item->SetStyle("FileSelectorListText");
    item->SetText(mapname);
    if (isAColorMap)
        item->AddTag("Color");
    appliedMaps->AddItem(item);
    SubscribeToEvent(item, E_DOUBLECLICK, URHO3D_HANDLER(AnimatorEditor, HandleEditCMap));
    return item;
}

UIElement* AnimatorEditor::PurgeAndApplyMap(UIElement* elt)
{
    String mapname = static_cast<Text*>(elt)->GetText();
    bool isAColorMap = elt->HasTag("Color");

    ListView* appliedMaps = static_cast<ListView*>(MapEditorLibImpl::GetPanel(PANEL_ANIMATOR2D_CMAPS)->GetChild("AppliedMaps", true));

    // skip if already applied at the end
    if (appliedMaps->GetNumItems())
    {
        if (static_cast<Text*>(appliedMaps->GetItem(appliedMaps->GetNumItems()-1))->GetText() == mapname)
            return appliedMaps->GetItem(appliedMaps->GetNumItems()-1);
    }

    // Purge Map if is already applied before the end
    if (appliedMaps->GetNumItems() > 1)
    {
        int i = appliedMaps->GetNumItems()-2;
        while (i >= 0)
        {
            if (static_cast<Text*>(appliedMaps->GetItem(i))->GetText() == mapname)
                appliedMaps->RemoveItem(i);
            i--;
        }
    }

    // Push the map at the end of the list
    Text* item = new Text(context_);
    item->SetDefaultStyle(MapEditorLibImpl::GetUIStyle(UISTYLE_EDITORDEFAULT));
    item->SetStyle("FileSelectorListText");
    item->SetText(mapname);
    if (isAColorMap)
        item->AddTag("Color");
    appliedMaps->AddItem(item);
    SubscribeToEvent(item, E_DOUBLECLICK, URHO3D_HANDLER(AnimatorEditor, HandleEditCMap));
    return item;
}

void AnimatorEditor::SetCharacterMappingPanel()
{
    AnimationSet2D* animationSet = editedAnimatedSprite_->GetAnimationSet();

    MapEditorLibImpl::Get()->SetVisible(PANEL_ANIMATOR2D_CMAPS, true);
    UIElement* panel = MapEditorLibImpl::GetPanel(PANEL_ANIMATOR2D_CMAPS);

    // Set SCML name
    static_cast<LineEdit*>(panel->GetChild("SetName", true))->SetText(animationSet->GetName());

    // Set Entity List
    const PODVector<Spriter::Entity*>& entities = animationSet->GetSpriterData()->entities_;
    DropDownList* entitylist = static_cast<DropDownList*>(panel->GetChild("EntityList", true));
    entitylist->RemoveAllItems();
    for (unsigned i=0; i < entities.Size(); i++)
    {
        Text* item = new Text(context_);
        item->SetStyle("FileSelectorListText");
        item->SetText(entities[i]->name_);
        entitylist->AddItem(item);
    }

    if (entities.Size() < entityid_)
        entityid_ = 0;

    entitylist->SetSelection(entityid_);
    Spriter::Entity* entity = entities[entityid_];

    //const PODVector<Spriter::Animation*>& animations = animationSet->GetSpriterData()->entities_[entityid]->animations_;

    // Set Available Character Maps
    const PODVector<Spriter::CharacterMap*>& charactermaps = entity->characterMaps_;
    ListView* availableMaps = static_cast<ListView*>(panel->GetChild("AvailableMaps", true));
    availableMaps->SetMultiselect(true);
    availableMaps->RemoveAllItems();
    for (unsigned i=0; i < charactermaps.Size(); i++)
    {
        UIElement* item = AddMap(charactermaps[i]->name_);
        URHO3D_LOGINFOF("AnimatorEditor() - SetCharacterMappingPanel : node=%s(%u) ... availableMaps maptype=sprite additem[%u]=%s elt=%u ", editNode_->GetName().CString(), editNode_->GetID(), i, charactermaps[i]->name_.CString(), item);
    }

    // Set Available Color Maps
    const PODVector<Spriter::ColorMap*>& colormaps = entity->colorMaps_;
    for (unsigned i=0; i < colormaps.Size(); i++)
    {
        UIElement* item = AddMap(colormaps[i]->name_);
        item->AddTag("Color");
        URHO3D_LOGINFOF("AnimatorEditor() - SetCharacterMappingPanel : node=%s(%u) ... availableMaps maptype=color additem[%u]=%s ", editNode_->GetName().CString(), editNode_->GetID(), i, colormaps[i]->name_.CString());
    }

    // Set Applied Maps
    GameHelpers::PurgeRedundantAppliedCharacterMaps(editedAnimatedSprite_);
    const PODVector<Spriter::CharacterMap*>& appliedcharactermaps = editedAnimatedSprite_->GetAppliedCharacterMaps();

    ListView* appliedMaps = static_cast<ListView*>(panel->GetChild("AppliedMaps", true));
    appliedMaps->RemoveAllItems();
    appliedMaps->SetMultiselect(true);
    for (unsigned i=0; i < appliedcharactermaps.Size(); i++)
    {
        UIElement* item = ApplyMap(appliedcharactermaps[i]->name_, false);
        URHO3D_LOGINFOF("AnimatorEditor() - SetCharacterMappingPanel : node=%s(%u) ... appliedcharactermaps additem[%u]=%s ", editNode_->GetName().CString(), editNode_->GetID(), i, appliedcharactermaps[i]->name_.CString());
    }

    SubscribeToEvent(panel->GetChild("SetSave", true), E_PRESSED, URHO3D_HANDLER(AnimatorEditor, HandleAnimationSetSave));

    SubscribeToEvent(panel->GetChild("EntityList", true), E_ITEMSELECTED, URHO3D_HANDLER(AnimatorEditor, HandleEntitySelected));
    SubscribeToEvent(panel->GetChild("AvailableMaps", true), E_ITEMSELECTED, URHO3D_HANDLER(AnimatorEditor, HandleCMapSelected));

    SubscribeToEvent(panel->GetChild("EditCMap", true), E_PRESSED, URHO3D_HANDLER(AnimatorEditor, HandleEditCMap));
    SubscribeToEvent(panel->GetChild("ApplyCMap", true), E_PRESSED, URHO3D_HANDLER(AnimatorEditor, HandleApplyCMap));
    SubscribeToEvent(panel->GetChild("NewCMap", true), E_PRESSED, URHO3D_HANDLER(AnimatorEditor, HandleNewCMap));
    SubscribeToEvent(panel->GetChild("DelCMap", true), E_PRESSED, URHO3D_HANDLER(AnimatorEditor, HandleDelCMap));
    SubscribeToEvent(panel->GetChild("MoveUpCMap", true), E_PRESSED, URHO3D_HANDLER(AnimatorEditor, HandleMoveUpAppliedCMap));
    SubscribeToEvent(panel->GetChild("MoveDownCMap", true), E_PRESSED, URHO3D_HANDLER(AnimatorEditor, HandleMoveDownAppliedCMap));
    SubscribeToEvent(panel->GetChild("RemoveCMap", true), E_PRESSED, URHO3D_HANDLER(AnimatorEditor, HandleRemoveAppliedCMap));

    panel_ = panel;

    SetColorSpritePanel();
    SetSpriteMappingPanel();
    SetSpriteSelectionList(GameContext::Get().resourceCache_->GetResource<SpriteSheet2D>("2D/spritesheet1.xml"));

    SetAnimationPanel();

    SubscribeToEvent(panel, E_VISIBLECHANGED, URHO3D_HANDLER(AnimatorEditor, HandleVisible));
    HandleVisible(StringHash::ZERO, GameContext::Get().context_->GetEventDataMap());
}

void AnimatorEditor::HandleVisible(StringHash eventType, VariantMap& eventData)
{
    bool visible = panel_->IsVisible();

    URHO3D_LOGINFOF("AnimatorEditor() - HandleVisible : visible=%s", visible ? "true":"false");

    if (!visible)
        FinishEdit();

    MapEditorLibImpl::Get()->SetVisible(PANEL_ANIMATOR2D_SWSPRITES, false);
    MapEditorLibImpl::Get()->SetVisible(PANEL_ANIMATOR2D_COLORMAPPING, false);
    if (!visible)
        MapEditorLibImpl::Get()->SetVisible(PANEL_ANIMATOR2D_ANIMS, false);

    if (previewScene_)
        previewScene_->SetUpdateEnabled(visible);

    GameContext::Get().Pause(visible);
}

void AnimatorEditor::HandleResize(StringHash eventType, VariantMap& eventData)
{
    URHO3D_LOGINFOF("AnimatorEditor() - HandleResize");

    panel_->BringToFront();
    if (preview_)
        preview_->QueueUpdate();
}

void AnimatorEditor::HandleAnimationSetSave(StringHash eventType, VariantMap& eventData)
{
    ResetSelection(RESETALLSELECT);

    AnimationSet2D* animationSet = editedAnimatedSprite_->GetAnimationSet();
    UIElement* panel = MapEditorLibImpl::GetPanel(PANEL_ANIMATOR2D_CMAPS);
    String savefilename = static_cast<LineEdit*>(panel->GetChild("SetName", true))->GetText();

    URHO3D_LOGINFOF("AnimatorEditor() - HandleAnimationSetSave : %s to %s ...", animationSet->GetName().CString(), savefilename.CString());
    animationSet->Save("Data/" + savefilename);
}

void AnimatorEditor::HandleEntitySelected(StringHash eventType, VariantMap& eventData)
{
    URHO3D_LOGINFOF("AnimatorEditor() - HandleEntitySelected");
    entityid_ = static_cast<DropDownList*>(MapEditorLibImpl::GetPanel(PANEL_ANIMATOR2D_CMAPS)->GetChild("EntityList", true))->GetSelection();
    SetCharacterMappingPanel();
}

void AnimatorEditor::HandleCMapSelected(StringHash eventType, VariantMap& eventData)
{
    URHO3D_LOGINFOF("AnimatorEditor() - HandleCMapSelected");
}

void AnimatorEditor::HandleEditCMap(StringHash eventType, VariantMap& eventData)
{
    URHO3D_LOGINFOF("AnimatorEditor() - HandleEditCMap");

    UIElement* elt = 0;

    if (eventType == E_DOUBLECLICK)
    {
        elt = static_cast<UIElement*>(context_->GetEventSender());
    }
    else if (eventType == E_PRESSED)
    {
        ListView* availableMaps = static_cast<ListView*>(MapEditorLibImpl::GetPanel(PANEL_ANIMATOR2D_CMAPS)->GetChild("AvailableMaps", true));
        if (availableMaps)
            elt = availableMaps->GetSelectedItem();
    }

    if (!elt)
        return;

    // Always Apply before edit
    Text* item = static_cast<Text*>(PurgeAndApplyMap(elt));

    UpdateMapping(true, true);

    if (item->HasTag("Color"))
    {
        // Show the color Mapping panel
        MapEditorLibImpl::Get()->SetVisible(PANEL_ANIMATOR2D_COLORMAPPING, true);

        // Set the colormap name in the color mapping panel
        static_cast<LineEdit*>(MapEditorLibImpl::GetPanel(PANEL_ANIMATOR2D_COLORMAPPING)->GetChild("CMapName", true))->SetText(item->GetText());

        UpdateColorSpritePanel();
    }
    else
    {
        // Show the Sprites Mapping panel
        MapEditorLibImpl::Get()->SetVisible(PANEL_ANIMATOR2D_SWSPRITES, true);

        // Set the colormap name in the color mapping panel
        static_cast<LineEdit*>(MapEditorLibImpl::GetPanel(PANEL_ANIMATOR2D_SWSPRITES)->GetChild("CMapName", true))->SetText(item->GetText());

        UpdateSpriteMappingPanel();
    }
}

void AnimatorEditor::HandleApplyCMap(StringHash eventType, VariantMap& eventData)
{
    URHO3D_LOGINFOF("AnimatorEditor() - HandleApplyCMap");

    UIElement* panel = MapEditorLibImpl::GetPanel(PANEL_ANIMATOR2D_CMAPS);
    ListView* availableMaps = static_cast<ListView*>(panel->GetChild("AvailableMaps", true));

    PODVector<UIElement*> selectedItems = availableMaps->GetSelectedItems();
    for (unsigned i=0; i < selectedItems.Size(); i++)
        UIElement* item = PurgeAndApplyMap(selectedItems[i]);

    UpdateMapping(true, true);
//    UpdateColorSpritePanel();
//    UpdateSpriteMappingPanel();
}

void AnimatorEditor::HandleNewCMap(StringHash eventType, VariantMap& eventData)
{
    URHO3D_LOGINFOF("AnimatorEditor() - HandleNewCMap");

    MapEditorLibImpl::Get()->SetVisible(PANEL_ANIMATOR2D_NEWCMAPS, true);
    UIElement* panel = MapEditorLibImpl::GetPanel(PANEL_ANIMATOR2D_NEWCMAPS);

    SubscribeToEvent(panel->GetChild("TypeList", true), E_ITEMSELECTED, URHO3D_HANDLER(AnimatorEditor, HandleNewCMapSelectType));
    SubscribeToEvent(panel->GetChild("OkButton", true), E_PRESSED, URHO3D_HANDLER(AnimatorEditor, HandleNewCMapCreate));
}

void AnimatorEditor::HandleNewCMapSelectType(StringHash eventType, VariantMap& eventData)
{
    URHO3D_LOGINFOF("AnimatorEditor() - HandleNewCMapSelectType");
}

void AnimatorEditor::HandleNewCMapCreate(StringHash eventType, VariantMap& eventData)
{
    UIElement* panel = MapEditorLibImpl::GetPanel(PANEL_ANIMATOR2D_NEWCMAPS);
    LineEdit* edit = static_cast<LineEdit*>(panel->GetChild("CMapName", true));

    URHO3D_LOGINFOF("AnimatorEditor() - HandleNewCMapCreate : name=%s", edit->GetText().CString());

    if (!edit->GetText().Empty())
    {
        bool isColorType = static_cast<DropDownList*>(panel->GetChild("TypeList", true))->GetSelection() == 1;

        // Add New Map in the available Maps
        MapEditorLibImpl::Get()->SetVisible(PANEL_ANIMATOR2D_NEWCMAPS, false);

        UIElement* item = AddMap(edit->GetText());
        if (isColorType)
            item->AddTag("Color");

        // Add New Map in the AnimationSet SpriterData
        Spriter::Entity* entity = editedAnimatedSprite_->GetSpriterInstance()->GetEntity();
        StringHash hashname(edit->GetText());
        if (isColorType)
        {
            if (!editedAnimatedSprite_->GetColorMap(hashname))
            {
                entity->colorMaps_.Push(new Spriter::ColorMap());
                Spriter::ColorMap* colormap = entity->colorMaps_.Back();
                colormap->id_ = entity->colorMaps_.Size()-1;
                colormap->name_ = edit->GetText();
                colormap->hashname_ = hashname;
            }
        }
        else
        {
            if (!editedAnimatedSprite_->GetCharacterMap(hashname))
            {
                entity->characterMaps_.Push(new Spriter::CharacterMap());
                Spriter::CharacterMap* cmap = entity->characterMaps_.Back();
                cmap->id_ = entity->characterMaps_.Size()-1;
                cmap->name_ = edit->GetText();
                cmap->hashname_ = hashname;
            }
        }
    }
}

void AnimatorEditor::HandleDelCMap(StringHash eventType, VariantMap& eventData)
{
    URHO3D_LOGINFOF("AnimatorEditor() - HandleDelCMap");
}

void AnimatorEditor::HandleMoveUpAppliedCMap(StringHash eventType, VariantMap& eventData)
{
//    URHO3D_LOGINFOF("AnimatorEditor() - HandleMoveUpAppliedCMap");

    UIElement* panel = MapEditorLibImpl::GetPanel(PANEL_ANIMATOR2D_CMAPS);
    ListView* appliedMaps = static_cast<ListView*>(panel->GetChild("AppliedMaps", true));
    PODVector<UIElement*> selectedItems = appliedMaps->GetSelectedItems();
    for (unsigned i=0; i < selectedItems.Size(); i++)
    {
        unsigned index = appliedMaps->FindItem(selectedItems[i]);
        if (index == 0)
            continue;

        SharedPtr<UIElement> element(selectedItems[i]);
        index--;
        appliedMaps->RemoveItem(selectedItems[i]);
        appliedMaps->InsertItem(index, element.Get());
        appliedMaps->SetSelection(index);
        element->SetSelected(true);
    }

    UpdateMapping(true, true);
    UpdateColorSpritePanel();
//    UpdateSpriteMappingPanel();
}

void AnimatorEditor::HandleMoveDownAppliedCMap(StringHash eventType, VariantMap& eventData)
{
//    URHO3D_LOGINFOF("AnimatorEditor() - HandleMoveDownAppliedCMap");

    UIElement* panel = MapEditorLibImpl::GetPanel(PANEL_ANIMATOR2D_CMAPS);
    ListView* appliedMaps = static_cast<ListView*>(panel->GetChild("AppliedMaps", true));
    PODVector<UIElement*> selectedItems = appliedMaps->GetSelectedItems();
    for (unsigned i=0; i < selectedItems.Size(); i++)
    {
        unsigned index = appliedMaps->FindItem(selectedItems[i]);
        if (index >= appliedMaps->GetNumItems()-1)
            continue;

        SharedPtr<UIElement> element(selectedItems[i]);
        index++;
        appliedMaps->RemoveItem(selectedItems[i]);
        appliedMaps->InsertItem(index, element.Get());
        appliedMaps->SetSelection(index);
        element->SetSelected(true);
    }

    UpdateMapping(true, true);
    UpdateColorSpritePanel();
//    UpdateSpriteMappingPanel();
}

void AnimatorEditor::HandleRemoveAppliedCMap(StringHash eventType, VariantMap& eventData)
{
    URHO3D_LOGINFOF("AnimatorEditor() - HandleRemoveAppliedCMap");

    UIElement* panel = MapEditorLibImpl::GetPanel(PANEL_ANIMATOR2D_CMAPS);
    ListView* appliedMaps = static_cast<ListView*>(panel->GetChild("AppliedMaps", true));
    PODVector<UIElement*> selectedItems = appliedMaps->GetSelectedItems();
    for (unsigned i=0; i < selectedItems.Size(); i++)
        appliedMaps->RemoveItem(selectedItems[i]);

    UpdateMapping(true, true);
    UpdateColorSpritePanel();
//    UpdateSpriteMappingPanel();
}


/// Animation Panel

void AnimatorEditor::SetAnimationPanel()
{
    URHO3D_LOGINFOF("AnimatorEditor() - SetAnimationPanel");

    AnimationSet2D* animationSet = editedAnimatedSprite_->GetAnimationSet();

    MapEditorLibImpl::Get()->SetVisible(PANEL_ANIMATOR2D_ANIMS, true);
    UIElement* panel = MapEditorLibImpl::GetPanel(PANEL_ANIMATOR2D_ANIMS);

    SubscribeToEvent(panel->GetChild("ShowBones", true), E_TOGGLED, URHO3D_HANDLER(AnimatorEditor, HandleAnimationToggleShowBones));

    const PODVector<Spriter::Entity*>& entities = animationSet->GetSpriterData()->entities_;
    const PODVector<Spriter::Animation*>& animations = entities[entityid_]->animations_;

    ListView* availableAnimations = static_cast<ListView*>(panel->GetChild("AvailableAnims", true));
    availableAnimations->SetMultiselect(false);
    availableAnimations->RemoveAllItems();
    for (unsigned i=0; i < animations.Size(); i++)
    {
        Text* item = new Text(context_);
        item->SetStyle("FileSelectorListText");
        item->SetText(animations[i]->name_);
        availableAnimations->AddItem(item);
    }

    String animation = animations.Front()->name_;
    static_cast<LineEdit*>(panel->GetChild("SetName", true))->SetText(animation);
    editedAnimatedSprite_->SetAnimation(animation, LM_FORCE_LOOPED);
    editedAnimatedSprite_->ResetAnimation();
    editedAnimatedSprite_->ForceUpdateBatches();

    UIElement* buttons = panel->GetChild("Buttons", true);
    if (!buttons->GetNumChildren())
    {
        UIElement* playbutton = CreateToolBarButton("AnimPlay", 30, UISTYLE_ICONS, false);
        static_cast<Text*>(static_cast<ToolTip*>(playbutton->GetChild("AnimPlayToolTip", true))->GetChild(0)->GetChild(0))->SetText("Play");

        buttons->AddChild(CreateToolBarButton("AnimPrevKey", 30, UISTYLE_ICONS, false));
        buttons->AddChild(playbutton);
        buttons->AddChild(CreateToolBarButton("AnimNextKey", 30, UISTYLE_ICONS, false));
        buttons->UpdateLayout();

        SubscribeToEvent(availableAnimations, E_ITEMSELECTED, URHO3D_HANDLER(AnimatorEditor, HandleAnimationSelect));
        SubscribeToEvent(playbutton, E_PRESSED, URHO3D_HANDLER(AnimatorEditor, HandleAnimationToggleTime));
        SubscribeToEvent(buttons->GetChild("AnimPrevKey", true), E_PRESSED, URHO3D_HANDLER(AnimatorEditor, HandleAnimationToggleTime));
        SubscribeToEvent(buttons->GetChild("AnimNextKey", true), E_PRESSED, URHO3D_HANDLER(AnimatorEditor, HandleAnimationToggleTime));
    }

    // Subscribe To Resize for TimeKeys repositionning
    UIElement* timeKeysContainer = MapEditorLibImpl::GetPanel(PANEL_ANIMATOR2D_ANIMS)->GetChild("TimeKeys", true);
    SubscribeToEvent(timeKeysContainer, E_RESIZED, URHO3D_HANDLER(AnimatorEditor, HandleTimeKeyContainerResized));

    // Set AnimationTime slider
    timeSlider_ = static_cast<Slider*>(panel->GetChild("AnimationTime", true));
    timeSlider_->SetRepeatRate(10.f);
    timeSlider_->SetRange(editedAnimatedSprite_->GetSpriterInstance()->GetAnimation()->length_ * 1000.f);
    timeSlider_->SetValue(editedAnimatedSprite_->GetCurrentAnimationTime() * 1000.f);
    SubscribeToEvent(timeSlider_, E_SLIDERPAGED, URHO3D_HANDLER(AnimatorEditor, HandleAnimationChangeTime));
    SubscribeToEvent(timeSlider_, E_SLIDERCHANGED, URHO3D_HANDLER(AnimatorEditor, HandleAnimationChangeTime));
    SubscribeToEvent(timeSlider_, E_DOUBLECLICK, URHO3D_HANDLER(AnimatorEditor, HandleAnimationModifyTimeKey));

    // Set Timeline Infos
    UIElement* timelineUi = MapEditorLibImpl::GetPanel(PANEL_ANIMATOR2D_ANIMS)->GetChild("TimelineContainer", true)->GetChild(0);
    // spin dropdownlist
    SubscribeToEvent(timelineUi->GetChild(6)->GetChild(1), E_ITEMSELECTED, URHO3D_HANDLER(AnimatorEditor, HandleAnimationKeyValueChanged));

    // Hide All bonesGizmos
    for (unsigned i=0; i < bonesGizmos_.Size(); i++)
        bonesGizmos_[i]->SetVisible(false);

    bonesVisible_ = static_cast<CheckBox*>(panel->GetChild("ShowBones", true))->IsChecked();

    SetTimeKeys();

    UpdateTimelineInfos();
    UpdateBonesGizmos();
}

void AnimatorEditor::PlayAnimation(bool enable)
{
    UIElement* button = MapEditorLibImpl::GetPanel(PANEL_ANIMATOR2D_ANIMS)->GetChild("AnimPlay", true);

    button->RemoveAllTags();

    // the tooltip is normally on root ui when hovering
    ToolTip* toolTip = static_cast<ToolTip*>(GameContext::Get().ui_->GetRoot()->GetChild("AnimPlayToolTip", false));
    if (!toolTip)
        toolTip = static_cast<ToolTip*>(button->GetChild("AnimPlayToolTip", true));

    isPlaying_ = enable;

    if (isPlaying_)
    {
        button->AddTag("Play");
        SubscribeToEvent(E_POSTUPDATE, URHO3D_HANDLER(AnimatorEditor, HandleAnimationPlay));
        static_cast<BorderImage*>(button->GetChild("Icon", true))->SetStyle("AnimPause");
        static_cast<Text*>(toolTip->GetChild(0)->GetChild(0))->SetText("Pause");
    }
    else
    {
        UnsubscribeFromEvent(E_POSTUPDATE);
        static_cast<BorderImage*>(button->GetChild("Icon", true))->SetStyle("AnimPlay");
        static_cast<Text*>(toolTip->GetChild(0)->GetChild(0))->SetText("Play");
    }
}

Spriter::Timeline* AnimatorEditor::AddTimeline(Spriter::SpriterObjectType type, float time)
{
    // TODO Check if the name exist already
    String name = "BoneTest1";

    // add timeline to animation
    Spriter::Animation* animation = editedAnimatedSprite_->GetSpriterInstance()->GetAnimation();
    Spriter::Timeline* timeline = new Spriter::Timeline();
    timeline->id_         = animation->timelines_.Size();
    timeline->name_       = name;
    timeline->hashname_   = StringHash(name);
    timeline->objectType_ = type;
    animation->timelines_.Push(timeline);

    Spriter::SpatialTimelineKey* spatialtimekey = 0;

    // add a first timekey at time
    if (type == Spriter::BONE)
    {
        // Set Time Key
        Spriter::BoneTimelineKey* timekey = new Spriter::BoneTimelineKey();

        // Set Object Info
        StringHash timelinehash(name);
        Spriter::ObjInfo& objinfo = editedAnimatedSprite_->GetSpriterInstance()->GetEntity()->objInfos_[timelinehash];
        objinfo.name_   = name;
        objinfo.type_   = type;
        objinfo.width_  = 16;
        objinfo.height_ = 4;

        spatialtimekey = timekey;
    }
    else if (type == Spriter::SPRITE)
    {
        // Set Time Key
        Spriter::SpriteTimelineKey* timekey = new Spriter::SpriteTimelineKey();
//        timekey->useDefaultPivot_ = ;
//        timekey->pivotX_          = ;
//        timekey->pivotY_          = ;
//        timekey->folderId_        = ;
//        timekey->fileId_          = ;
//        timekey->fx_              = ;

        spatialtimekey = timekey;
    }
    else if (type == Spriter::POINT)
    {
        // Set Time Key
        Spriter::PointTimelineKey* timekey = new Spriter::PointTimelineKey();

        spatialtimekey = timekey;
    }
    else if (type == Spriter::BOX)
    {
        // Set Time Key
        Spriter::BoxTimelineKey* timekey = new Spriter::BoxTimelineKey();
//        timekey->useDefaultPivot_ = ;
//        timekey->pivotX_          = ;
//        timekey->pivotY_          = ;
//        timekey->width_           = ;
//        timekey->height_          = ;

        // Set Object Info
        StringHash timelinehash(name);
        Spriter::ObjInfo& objinfo = editedAnimatedSprite_->GetSpriterInstance()->GetEntity()->objInfos_[timelinehash];
        objinfo.name_   = name;
        objinfo.type_   = type;
        objinfo.width_  = 16;
        objinfo.height_ = 16;
        objinfo.pivotX_ = 0.f;
        objinfo.pivotY_ = 0.f;

        spatialtimekey = timekey;
    }

    if (spatialtimekey)
    {
        spatialtimekey->id_        = timeline->keys_.Size();
        spatialtimekey->time_      = time;
        spatialtimekey->curveType_ = Spriter::LINEAR;
        spatialtimekey->c1_        = 0.f;
        spatialtimekey->c2_        = 0.f;
        spatialtimekey->c3_        = 0.f;
        spatialtimekey->c4_        = 0.f;
        spatialtimekey->timeline_   = timeline;
        spatialtimekey->info_.x_      = 0.f;
        spatialtimekey->info_.y_      = 0.f;
        spatialtimekey->info_.angle_  = 0.f;
        spatialtimekey->info_.scaleX_ = 1;
        spatialtimekey->info_.scaleY_ = 1;
        spatialtimekey->info_.spin    = 1;

        timeline->keys_.Push(spatialtimekey);
    }

    return timeline;
}

Spriter::MainlineKey* AnimatorEditor::AddMainlineKey(unsigned newmainkeyid, float time)
{
    Spriter::Animation* animation = editedAnimatedSprite_->GetSpriterInstance()->GetAnimation();

    Spriter::MainlineKey* mainkey = animation->mainlineKeys_[newmainkeyid-1];
    Spriter::MainlineKey* newmainkey = new Spriter::MainlineKey();
    animation->mainlineKeys_.Insert(newmainkeyid, newmainkey);
    newmainkey->time_ = time;
    newmainkey->curveType_ = Spriter::LINEAR;
    newmainkey->c1_ = 0.f;
    newmainkey->c2_ = 0.f;
    newmainkey->c3_ = 0.f;
    newmainkey->c4_ = 0.f;
    // recreate all the refs because the refs are uniques for each timekey
    for (unsigned i=0; i < mainkey->boneRefs_.Size(); i++)
    {
        Spriter::Ref* ref = new Spriter::Ref();
        mainkey->boneRefs_[i]->Copy(*ref);
        newmainkey->boneRefs_.Push(ref);
    }
    for (unsigned i=0; i < mainkey->objectRefs_.Size(); i++)
    {
        Spriter::Ref* ref = new Spriter::Ref();
        mainkey->objectRefs_[i]->Copy(*ref);
        newmainkey->objectRefs_.Push(ref);
    }

    // update mainline keys id
    for (unsigned i=newmainkeyid; i < animation->mainlineKeys_.Size(); i++)
        animation->mainlineKeys_[i]->id_ = i;

    return newmainkey;
}

void AnimatorEditor::UpdateRefs(float time, unsigned timelineid, int timelineparentid)
{
    Spriter::Animation* animation = editedAnimatedSprite_->GetSpriterInstance()->GetAnimation();
    Spriter::MainlineKey* mainkey = animation->GetMainlineKey(time);
    if (!mainkey)
    {
        URHO3D_LOGERRORF("AnimatorEditor() - AddSpriterObject : ... no mainkey for time=%F !", time);
        return;
    }

    unsigned mainkeyid = mainkey->id_;

    if (mainkey->time_ != time)
    {
        // add mainkey at time
        mainkey = AddMainlineKey(mainkeyid, time);
        mainkeyid = mainkey->id_;
    }

    Spriter::Timeline* timeline = animation->timelines_[timelineid];
    if (timeline->objectType_ == Spriter::BONE)
    {
        URHO3D_LOGINFOF("AnimatorEditor() - UpdateRefs : ... update time=%F timelineid=%u timelineparentid=%u ", time, timelineid, timelineparentid);

        // update the ref to the timeline in each mainlinekey
        for (unsigned i=mainkeyid; i < animation->mainlineKeys_.Size(); i++)
        {
            mainkey = animation->mainlineKeys_[i];

            // check if the ref for the timeline exists already in the mainlinekey
            Spriter::Ref* ref = mainkey->GetBoneRef(timelineid);

            // add new ref
            if (!ref)
            {
                ref = new Spriter::Ref();
                mainkey->boneRefs_.Back()->Copy(*ref);
                ref->id_ = mainkey->boneRefs_.Size();
                ref->timeline_ = timelineid;
                ref->key_ = 0;
                mainkey->boneRefs_.Push(ref);
                URHO3D_LOGINFOF("AnimatorEditor() - UpdateRefs : ... update time=%F timelineid=%u timelineparentid=%u ... Add Ref ... ", mainkey->time_, timelineid, timelineparentid);
            }

            // find the parent bonerefid
            ref->parent_ = timelineparentid != -1 ? mainkey->GetBoneRef(timelineparentid)->id_ : -1;

            URHO3D_LOGINFOF("AnimatorEditor() - UpdateRefs : ... update time=%F timelineid=%u timelineparentid=%u ... Add Ref ... set Parent=%u ", mainkey->time_, timelineid, timelineparentid, ref->parent_);
        }
    }
    else
    {
        for (unsigned i=mainkeyid; i < animation->mainlineKeys_.Size(); i++)
        {
            mainkey = animation->mainlineKeys_[i];

            // TODO ; z_index
//            unsigned z_index;
//            mainkey->objectRefs_.Insert(z_index, ref);
//            for (unsigned z = z_index; z < mainkey->objectRefs_.Size(); z++)
//            {
//                mainkey->objectRefs_[z]->id_ = z;
//                mainkey->objectRefs_[z]->zIndex_ = z;
//            }
        }
    }

    editedAnimatedSprite_->GetSpriterInstance()->UpdateTimelineKeys();
}

void AnimatorEditor::AddSpriterObject(Spriter::SpriterObjectType type, const Vector2& worldposition, bool finish)
{
    if (!finish)
    {
        if (addingSpriterObjectTimelineId_ == -1)
        {
            URHO3D_LOGINFOF("AnimatorEditor() - AddSpriterObject : ... start");

            // start position
            addingSpriterObjectTimelinePosition_ = worldposition;
            // add new timeline
            addingSpriterObjectTimelineId_ = AddTimeline(type, editedAnimatedSprite_->GetSpriterInstance()->GetCurrentTime())->id_;

            addingSpriterObjectTimelineParentId_ = -1;
            // get the timelineid for the selected bone
            if (lastSelectedBoneIndex_ != -1)
            {
                addingSpriterObjectTimelineParentId_ = editedAnimatedSprite_->GetSpriterInstance()->GetBoneKeys()[lastSelectedBoneIndex_]->timeline_->id_;
            }
            // if no bone selected but sprite selected, get the id of the parent (the bone that holds the sprite) for the selected sprite
            else if (lastSelectedTimelineId_ != -1)
            {
                Spriter::Animation* animation = editedAnimatedSprite_->GetSpriterInstance()->GetAnimation();
                Spriter::MainlineKey* mainkey = animation->GetMainlineKey(editedAnimatedSprite_->GetSpriterInstance()->GetCurrentTime());
                Spriter::Ref* ref = mainkey->GetObjectRef(lastSelectedTimelineId_);
                if (ref->parent_ != -1)
                    addingSpriterObjectTimelineParentId_ = mainkey->boneRefs_[ref->parent_]->timeline_;
            }

            // Update the mainlinekeys Refs with no parent for
            UpdateRefs(editedAnimatedSprite_->GetSpriterInstance()->GetCurrentTime(), addingSpriterObjectTimelineId_, -1);

            // Set Bone Position
            {
                Spriter::Timeline* timeline = editedAnimatedSprite_->GetSpriterInstance()->GetAnimation()->timelines_[addingSpriterObjectTimelineId_];
                Spriter::SpatialTimelineKey* bonekey = timeline->keys_.Back();
                bonekey->info_.x_ = worldposition.x_;
                bonekey->info_.y_ = worldposition.y_;
            }
        }
//        URHO3D_LOGINFOF("AnimatorEditor() - AddSpriterObject : ... ");

        // TODO : extend the bone : modif Spriter::ObjInfo, update bonegizmo
        Spriter::Timeline* timeline = editedAnimatedSprite_->GetSpriterInstance()->GetAnimation()->timelines_[addingSpriterObjectTimelineId_];

        // Set Bone Angle
//        {
//            Spriter::SpatialTimelineKey* bonekey = timeline->keys_.Back();
//            bonekey->info_.angle_ = ;
//        }
        // Set Bone Length
        {
            Spriter::ObjInfo& objinfo = editedAnimatedSprite_->GetSpriterInstance()->GetEntity()->objInfos_[timeline->hashname_];
            objinfo.width_  = 16 + (addingSpriterObjectTimelinePosition_ - worldposition).Length() / PIXEL_SIZE;
        }
        UpdateBonesGizmos(true);
    }
    else if (finish && addingSpriterObjectTimelineId_ != -1)
    {
        URHO3D_LOGINFOF("AnimatorEditor() - AddSpriterObject : ... finish : selectedTimelineId=%d !", addingSpriterObjectTimelineParentId_);

        // Update the mainlinekeys Refs
        UpdateRefs(editedAnimatedSprite_->GetSpriterInstance()->GetCurrentTime(), addingSpriterObjectTimelineId_, addingSpriterObjectTimelineParentId_);
        addingSpriterObjectTimelineId_ = addingSpriterObjectTimelineParentId_ = -1;
    }
}

void AnimatorEditor::SetTimeKeys()
{
    const int sliderKnobWidth = 10;
    const int keysize = 16;
    const float pivotx = (float)(keysize-sliderKnobWidth)/keysize * 0.5f;

    UIElement* timeKeysContainer = MapEditorLibImpl::GetPanel(PANEL_ANIMATOR2D_ANIMS)->GetChild("TimeKeys", true);
    timeKeysContainer->SetSize(timeSlider_->GetWidth()-sliderKnobWidth, 16);
    timeKeysContainer->RemoveAllChildren();

    Spriter::Animation* animation = editedAnimatedSprite_->GetSpriterInstance()->GetAnimation();
    const float animationtime = editedAnimatedSprite_->GetSpriterInstance()->GetAnimation()->length_;
    for (PODVector<Spriter::MainlineKey*>::ConstIterator it = animation->mainlineKeys_.Begin(); it != animation->mainlineKeys_.End(); ++it)
    {
        Button* keyui = new Button(GameContext::Get().context_);
        keyui->SetName("AnimTimeKey");
        keyui->SetDefaultStyle(MapEditorLibImpl::GetUIStyle(UISTYLE_ICONS));
        keyui->SetFixedSize(keysize, keysize);
        keyui->SetPivot(pivotx, -pivotx);
        keyui->SetBlendMode(BLEND_ALPHA);
        keyui->SetHoverOffset(0, 1);
        keyui->SetVar(UIELEMENT_CHILDINDEX, (int)(it-animation->mainlineKeys_.Begin()));
        keyui->SetVar(KEYTIME, (*it)->time_);
        timeKeysContainer->AddChild(keyui);

        int x = floor((*it)->time_ / animationtime * ((float)(timeSlider_->GetWidth()-sliderKnobWidth)));
        keyui->SetPosition(IntVector2(x, 0));

        SubscribeToEvent(keyui, E_PRESSED, URHO3D_HANDLER(AnimatorEditor, HandleAnimationToggleTime));
        SubscribeToEvent(keyui, E_DOUBLECLICK, URHO3D_HANDLER(AnimatorEditor, HandleAnimationModifyTimeKey));
    }

    UpdateTimeKeysIcons();
}

void AnimatorEditor::UpdateTimeKeys()
{
    UIElement* timeKeysContainer = MapEditorLibImpl::GetPanel(PANEL_ANIMATOR2D_ANIMS)->GetChild("TimeKeys", true);
    Spriter::Animation* animation = editedAnimatedSprite_->GetSpriterInstance()->GetAnimation();
    if (animation->mainlineKeys_.Size() != timeKeysContainer->GetNumChildren())
    {
        SetTimeKeys();
        return;
    }

    const int sliderKnobWidth = 10;
    timeKeysContainer->SetSize(timeSlider_->GetWidth()-sliderKnobWidth, 16);
    const float animationtime = editedAnimatedSprite_->GetSpriterInstance()->GetAnimation()->length_;
    for (unsigned i=0; i < animation->mainlineKeys_.Size(); i++)
    {
        UIElement* keyui = timeKeysContainer->GetChild(i);
        int x = floor(animation->mainlineKeys_[i]->time_ / animationtime * ((float)(timeSlider_->GetWidth()-sliderKnobWidth)));
        keyui->SetPosition(x, 0);
    }
}

void AnimatorEditor::UpdateTimeKeysIcons()
{
    UIElement* timeKeysContainer = MapEditorLibImpl::GetPanel(PANEL_ANIMATOR2D_ANIMS)->GetChild("TimeKeys", true);
    Spriter::Animation* animation = editedAnimatedSprite_->GetSpriterInstance()->GetAnimation();

    const int timelineid = lastSelectedBoneIndex_ != -1 ? editedAnimatedSprite_->GetSpriterInstance()->GetBoneKeys()[lastSelectedBoneIndex_]->timeline_->id_ : lastSelectedTimelineId_;
    if (timelineid != -1)
    {
        // use timeline
        // find if a timekey is owned by the timeline
        Spriter::Timeline* timeline = editedAnimatedSprite_->GetSpriterInstance()->GetAnimation()->timelines_[timelineid];
        for (unsigned i=0; i < animation->mainlineKeys_.Size(); i++)
        {
            bool owned = timeline->GetTimeKey(animation->mainlineKeys_[i]->time_)->time_ == animation->mainlineKeys_[i]->time_;
            timeKeysContainer->GetChild(i)->SetStyle(owned ? "AnimTimeKeyOwn" : "AnimTimeKey");
        }
    }
    else
    {
        // reset to mainline icon
        for (unsigned i=0; i < animation->mainlineKeys_.Size(); i++)
            timeKeysContainer->GetChild(i)->SetStyle("AnimTimeKey");
    }
}

void AnimatorEditor::UpdateTimelineInfos()
{
    UIElement* timelineInfoContainer = MapEditorLibImpl::GetPanel(PANEL_ANIMATOR2D_ANIMS)->GetChild("TimelineContainer", true)->GetChild(0);

    // Selected Bone Timeline
    if (lastSelectedBoneIndex_ != -1)
    {
        int selectedBoneTimelineId = editedAnimatedSprite_->GetSpriterInstance()->GetBoneKeys()[lastSelectedBoneIndex_]->timeline_->id_;
        Spriter::Timeline* timeline = editedAnimatedSprite_->GetSpriterInstance()->GetAnimation()->timelines_[selectedBoneTimelineId];
        static_cast<Text*>(timelineInfoContainer->GetChild(0)->GetChild(0))->SetText(String("bone   :"));
        static_cast<Text*>(timelineInfoContainer->GetChild(0)->GetChild(1))->SetText(timeline->name_);
        Spriter::BoneTimelineKey* tKey = static_cast<Spriter::BoneTimelineKey*>(timeline->GetTimeKey(editedAnimatedSprite_->GetCurrentAnimationTime()));
        static_cast<Text*>(timelineInfoContainer->GetChild(1)->GetChild(1))->SetText(String(tKey->id_) + String(" - time : ") + String(tKey->time_));
        static_cast<Text*>(timelineInfoContainer->GetChild(2)->GetChild(1))->SetText(String(Urho3D::Spriter::SpriterData::GetCurveTypeStr(tKey->curveType_)));
        static_cast<Text*>(timelineInfoContainer->GetChild(3)->GetChild(1))->SetText(String(tKey->info_.x_) + " " + String(tKey->info_.y_));
        static_cast<Text*>(timelineInfoContainer->GetChild(4)->GetChild(1))->SetText(String(tKey->info_.angle_));
        static_cast<Text*>(timelineInfoContainer->GetChild(5)->GetChild(1))->SetText(String(tKey->info_.scaleX_) + " " + String(tKey->info_.scaleY_));
        static_cast<DropDownList*>(timelineInfoContainer->GetChild(6)->GetChild(1))->SetSelection(tKey->info_.spin > 0 ? 0 : (tKey->info_.spin == 0 ? 1 : 2));

        for (unsigned i=3; i < 7; i++)
            timelineInfoContainer->GetChild(i)->SetVisible(true);
    }
    // Selected Sprite Timeline
    else if (lastSelectedTimelineId_ != -1)
    {
        Spriter::Timeline* timeline = editedAnimatedSprite_->GetSpriterInstance()->GetAnimation()->timelines_[lastSelectedTimelineId_];
        static_cast<Text*>(timelineInfoContainer->GetChild(0)->GetChild(0))->SetText(String("sprite :"));
        static_cast<Text*>(timelineInfoContainer->GetChild(0)->GetChild(1))->SetText(timeline->name_);
        Spriter::SpriteTimelineKey* tKey = static_cast<Spriter::SpriteTimelineKey*>(timeline->GetTimeKey(editedAnimatedSprite_->GetCurrentAnimationTime()));
        static_cast<Text*>(timelineInfoContainer->GetChild(1)->GetChild(1))->SetText(String(tKey->id_) + String(" - time : ") + String(tKey->time_));
        static_cast<Text*>(timelineInfoContainer->GetChild(2)->GetChild(1))->SetText(String(Urho3D::Spriter::SpriterData::GetCurveTypeStr(tKey->curveType_)));
        static_cast<Text*>(timelineInfoContainer->GetChild(3)->GetChild(1))->SetText(String(tKey->info_.x_) + " " + String(tKey->info_.y_));
        static_cast<Text*>(timelineInfoContainer->GetChild(4)->GetChild(1))->SetText(String(tKey->info_.angle_));
        static_cast<Text*>(timelineInfoContainer->GetChild(5)->GetChild(1))->SetText(String(tKey->info_.scaleX_) + " " + String(tKey->info_.scaleY_));
        static_cast<DropDownList*>(timelineInfoContainer->GetChild(6)->GetChild(1))->SetSelection(tKey->info_.spin > 0 ? 0 : (tKey->info_.spin == 0 ? 1 : 2));
        for (unsigned i=3; i < 7; i++)
            timelineInfoContainer->GetChild(i)->SetVisible(true);
    }
    // MainLine
    else
    {
        static_cast<Text*>(timelineInfoContainer->GetChild(0)->GetChild(0))->SetText("mainline");
        static_cast<Text*>(timelineInfoContainer->GetChild(0)->GetChild(1))->SetText(String::EMPTY);
        Spriter::MainlineKey* mainkey = editedAnimatedSprite_->GetSpriterInstance()->GetAnimation()->GetMainlineKey(editedAnimatedSprite_->GetCurrentAnimationTime());
        static_cast<Text*>(timelineInfoContainer->GetChild(1)->GetChild(1))->SetText(String(mainkey->id_) + String(" - time : ") + String(mainkey->time_));
        static_cast<Text*>(timelineInfoContainer->GetChild(2)->GetChild(1))->SetText(String(Urho3D::Spriter::SpriterData::GetCurveTypeStr(mainkey->curveType_)));
        for (unsigned i=3; i < 7; i++)
            timelineInfoContainer->GetChild(i)->SetVisible(false);
    }
}

void AnimatorEditor::UpdateBonesGizmos(bool force)
{
    const unsigned numbones = editedAnimatedSprite_->GetSpriterInstance()->GetNumBoneKeys();
    const PODVector<Spriter::BoneTimelineKey* >& bonekeys = editedAnimatedSprite_->GetSpriterInstance()->GetBoneKeys();

//    URHO3D_LOGINFOF("AnimatorEditor() - UpdateBonesGizmos : visibibility=%s numbones=%u ", bonesVisible_?"on":"off", numbones);

    if (numbones > bonesGizmos_.Size())
    {
        for (unsigned i=bonesGizmos_.Size(); i < numbones; i++)
            bonesGizmos_.Push(SharedPtr<BoneGizmo>(new BoneGizmo(context_)));
    }
    for (unsigned i=0; i < numbones; i++)
    {
        BoneGizmo* bone = bonesGizmos_[i].Get();
        bone->Update(editedAnimatedSprite_->GetNode(), bonekeys[i], force);
        bone->SetVisible(bonesVisible_);
    }
    if (bonesGizmos_.Size() > numbones)
    {
        for (unsigned i=numbones; i < bonesGizmos_.Size(); i++)
            bonesGizmos_[i]->SetVisible(false);
    }
}

void AnimatorEditor::HandleAnimationKeyValueChanged(StringHash eventType, VariantMap& eventData)
{
    DropDownList* list = static_cast<DropDownList*>(eventData[ItemSelected::P_ELEMENT].GetPtr());
    if (list)
    {
        int selection = eventData[ItemSelected::P_SELECTION].GetInt();
        int spinvalue = selection == 0 ? 1 : (selection == 1 ? 0 : -1);
//        URHO3D_LOGINFOF("AnimatorEditor() - HandleAnimationKeyValueChanged : selectionindex=%d spinvalue=%d", selection, spinvalue);
        if (lastSelectedBoneIndex_ != -1)
        {
            int selectedBoneTimelineId = editedAnimatedSprite_->GetSpriterInstance()->GetBoneKeys()[lastSelectedBoneIndex_]->timeline_->id_;
            Spriter::Timeline* timeline = editedAnimatedSprite_->GetSpriterInstance()->GetAnimation()->timelines_[selectedBoneTimelineId];

            // Get the current TimelineKey
            Spriter::BoneTimelineKey* tKey = static_cast<Spriter::BoneTimelineKey*>(timeline->GetTimeKey(editedAnimatedSprite_->GetCurrentAnimationTime()));
            if (tKey)
                tKey->info_.spin = spinvalue;
        }
        else if (lastSelectedTimelineId_ != -1)
        {
            Spriter::Timeline* timeline = editedAnimatedSprite_->GetSpriterInstance()->GetAnimation()->timelines_[lastSelectedTimelineId_];
            // Get the current TimelineKey
            Spriter::SpriteTimelineKey* tKey = static_cast<Spriter::SpriteTimelineKey*>(timeline->GetTimeKey(editedAnimatedSprite_->GetCurrentAnimationTime()));
            if (tKey)
                tKey->info_.spin = spinvalue;
        }
    }
}

void AnimatorEditor::HandleAnimationModifyTimeKey(StringHash eventType, VariantMap& eventData)
{
    UIElement* uielt = static_cast<UIElement*>(eventData[ItemSelected::P_ELEMENT].GetPtr());

    const Spriter::SpriterObjectType type = lastSelectedBoneIndex_ != -1 ? Spriter::BONE : Spriter::SPRITE;
    const int timelineid = type == Spriter::BONE ? editedAnimatedSprite_->GetSpriterInstance()->GetBoneKeys()[lastSelectedBoneIndex_]->timeline_->id_ : lastSelectedTimelineId_;
    if (timelineid == -1)
        return;

    const bool remove = GameContext::Get().input_->GetQualifierDown(QUAL_CTRL);
    const int keyinc = remove ? -1 : 1;

    // get time from the slider or from the pressed timekeyui
    float time = (!remove && uielt == timeSlider_) ? timeSlider_->GetValue() / 1000.f : uielt->GetVar(KEYTIME).GetFloat();

    Spriter::Animation* animation = editedAnimatedSprite_->GetSpriterInstance()->GetAnimation();
    Spriter::Timeline* timeline = animation->timelines_[timelineid];
    Spriter::MainlineKey* mainkey = animation->GetMainlineKey(time);
    unsigned mainkeyid = mainkey->id_;

    Spriter::SpatialTimelineKey* timelinekey = timeline->GetTimeKey(time);
    const unsigned timelinekeyid = timelinekey->id_;

    if (!remove)
    {
        if (timelinekey->time_ == time)
        {
            URHO3D_LOGINFOF("AnimatorEditor() - HandleAnimationModifyTimeKey : time = %f key already exist !", time);
            return;
        }

        // insert timelinekey

        // duplicate the timelinekey
        Spriter::SpatialTimelineKey* newTimelineKey;
        if (type == Spriter::BONE)
            newTimelineKey = new Spriter::BoneTimelineKey();
        else
            newTimelineKey = new Spriter::SpriteTimelineKey();
        timelinekey->Copy(newTimelineKey);
        newTimelineKey->time_ = time;
        newTimelineKey->timeline_ = timeline;
        // interpolate and insert
        const unsigned newtimelinekeyid = timelinekeyid + 1;
        Spriter::SpatialTimelineKey* nextTimelineKey = timeline->keys_[newtimelinekeyid < timeline->keys_.Size() ? newtimelinekeyid : 0];
        newTimelineKey->Interpolate(*nextTimelineKey, timelinekey->GetFactor(timelinekey->time_, nextTimelineKey->time_, animation->length_, time));
        timeline->keys_.Insert(newtimelinekeyid, newTimelineKey);
        // update timeline keys id
        for (unsigned i = 0; i < timeline->keys_.Size(); i++)
            timeline->keys_[i]->id_ = i;

//        URHO3D_LOGINFOF("AnimatorEditor() - HandleAnimationModifyTimeKey : time=%f insert a key id=%u (new=%u t=%F, prev=%u t=%F, next=%u t=%F) ...",
//                         time, newtimelinekeyid, newTimelineKey->id_, newTimelineKey->time_, timelinekey->id_, timelinekey->time_, nextTimelineKey->id_, nextTimelineKey->time_);

        // update mainline
        if (mainkey->time_ != time)
        {
            Spriter::MainlineKey* newmainkey = AddMainlineKey(mainkeyid+1, time);
            mainkeyid = newmainkey->id_;

            // replace the timelinekey by the new
            Spriter::Ref* ref = newmainkey->GetRef(timelineid);
            if (ref)
                ref->key_ = newtimelinekeyid;
            else
                URHO3D_LOGERRORF("AnimatorEditor() - HandleAnimationModifyTimeKey : time = %f timelineid=%d insert a mainkey id=%u no ref !!! ", time, timelineid, mainkeyid);

//            URHO3D_LOGINFOF("AnimatorEditor() - HandleAnimationModifyTimeKey : time = %f timelineid=%d insert a mainkey id=%u ... ", time, timelineid, mainkeyid);
            // skip this mainkey already modified in the next code part
            mainkeyid++;
        }
//        URHO3D_LOGINFOF("AnimatorEditor() - HandleAnimationModifyTimeKey : time=%f timelineid=%d insert a key id=%u (new=%u t=%F, prev=%u t=%F, next=%u t=%F) !!!",
//                         time, timelineid, newtimelinekeyid, newTimelineKey->id_, newTimelineKey->time_, timelinekey->id_, timelinekey->time_, nextTimelineKey->id_, nextTimelineKey->time_);
    }
    else
    {
        if (timelinekeyid >= timeline->keys_.Size())
        {
            URHO3D_LOGERRORF("AnimatorEditor() - HandleAnimationModifyTimeKey : time = %f timelineid=%d nokey id=%u !!! ", time, timelineid, timelinekeyid);
            return;
        }
        else if (timelinekeyid == 0)
        {
            URHO3D_LOGERRORF("AnimatorEditor() - HandleAnimationModifyTimeKey : time = %f timelineid=%d can't remove firstkey !!! ", time, timelineid);
            return;
        }

        URHO3D_LOGINFOF("AnimatorEditor() - HandleAnimationModifyTimeKey : time=%f timelineid=%d remove a key ...", time);

        // remove the timekey in the timeline
        timeline->keys_.Erase(timelinekeyid);
        delete timelinekey;

        // update timeline keys id
        for (unsigned i = timelinekeyid; i < timeline->keys_.Size(); i++)
            timeline->keys_[i]->id_ = i;

        // update the prevtimelinekey spin
        AdjustSpinAt(timeline, timelinekeyid > 0 ? timelinekeyid-1 : timeline->keys_.Size()-1);
    }

    // update the timekeys id for this timeline in the mainline refs
    {
        PODVector<Spriter::Ref*> refs;
        if (type == Spriter::BONE)
            animation->GetBoneRefs(timelineid, refs, mainkeyid);
        else
            animation->GetObjectRefs(timelineid, refs, mainkeyid);
        for (unsigned i=0; i < refs.Size(); i++)
        {
            Spriter::Ref* ref = refs[i];
            if (ref->key_ >= timelinekeyid)
                ref->key_ += keyinc;
        }
    }

    if (remove)
    {
        // check if the mainkey is need
        // if all timelinekey in the refs have differents times than the keytime, remove the mainkey
        bool mainkeyneeded = false;
        const PODVector<Spriter::Ref*>& refs = type == Spriter::BONE ? mainkey->boneRefs_ : mainkey->objectRefs_;
        for (unsigned i=0; i < refs.Size(); i++)
        {
            Spriter::Ref* ref = refs[i];
            if (animation->timelines_[ref->timeline_]->keys_[ref->key_]->time_ == time)
            {
                mainkeyneeded = true;
                break;
            }
        }
        if (!mainkeyneeded)
        {
            // remove the mainkey
            animation->mainlineKeys_.Erase(mainkeyid);
            delete mainkey;

            // update mainline keys id
            for (unsigned i=mainkeyid; i < animation->mainlineKeys_.Size(); i++)
                animation->mainlineKeys_[i]->id_ = i;
        }
    }

    // update the timekeys uielements
    // TODO : just insert new uielement is better !
    SetTimeKeys();

    timeSlider_->SetValue(time * 1000.f);
}

void AnimatorEditor::HandleTimeKeyContainerResized(StringHash eventType, VariantMap& eventData)
{
    UpdateTimeKeys();
}

void AnimatorEditor::HandleAnimationSelect(StringHash eventType, VariantMap& eventData)
{
//    URHO3D_LOGINFOF("AnimatorEditor() - HandleAnimationSelect");

    UIElement* panel = MapEditorLibImpl::GetPanel(PANEL_ANIMATOR2D_ANIMS);
    ListView* availableAnimations = static_cast<ListView*>(panel->GetChild("AvailableAnims", true));

    String animation = static_cast<Text*>(availableAnimations->GetSelectedItem())->GetText();
    static_cast<LineEdit*>(panel->GetChild("SetName", true))->SetText(animation);

    editedAnimatedSprite_->SetAnimation(animation, LM_FORCE_LOOPED);
    editedAnimatedSprite_->ResetAnimation();
    editedAnimatedSprite_->ForceUpdateBatches();

    SetTimeKeys();

    timeSlider_->SetRange(editedAnimatedSprite_->GetSpriterInstance()->GetAnimation()->length_ * 1000.f);
    timeSlider_->SetValue(editedAnimatedSprite_->GetCurrentAnimationTime() * 1000.f);

    if (bonesVisible_)
        UpdateBonesGizmos();
}

void AnimatorEditor::HandleAnimationToggleShowBones(StringHash eventType, VariantMap& eventData)
{
    bonesVisible_ = eventData[Toggled::P_STATE].GetBool();

    UpdateBonesGizmos();
}

void AnimatorEditor::HandleAnimationToggleTime(StringHash eventType, VariantMap& eventData)
{
    Button* button = static_cast<Button*>(eventData[Pressed::P_ELEMENT].GetPtr());

    if (button->GetName() == "AnimPlay")
    {
        PlayAnimation(!button->HasTag("Play"));
    }
    else
    {
        PlayAnimation(false);

        Spriter::Animation* animation = editedAnimatedSprite_->GetSpriterInstance()->GetAnimation();

        if (lastSelectedBoneIndex_ != -1 || lastSelectedTimelineId_ != -1)
        {
            const unsigned timelineId = lastSelectedBoneIndex_ != -1 ? editedAnimatedSprite_->GetSpriterInstance()->GetBoneKeys()[lastSelectedBoneIndex_]->timeline_->id_ : lastSelectedTimelineId_;
            Spriter::Timeline* timeline = editedAnimatedSprite_->GetSpriterInstance()->GetAnimation()->timelines_[timelineId];
            const int numTimeKeys = timeline->keys_.Size();
            int keyid = 0;
            if (button->GetName() == "AnimPrevKey")
            {
                keyid = timeline->GetTimeKey(editedAnimatedSprite_->GetCurrentAnimationTime())->id_ - 1;
                if (keyid < 0)
                    keyid = numTimeKeys-1;
            }
            else if (button->GetName() == "AnimNextKey")
            {
                keyid = timeline->GetTimeKey(editedAnimatedSprite_->GetCurrentAnimationTime())->id_ + 1;
                if (keyid > numTimeKeys-1)
                    keyid = 0;
            }
            else
            {
                // get the time of the mainlinekey and get the timelinekey for this time
                float time = animation->mainlineKeys_[button->GetVar(UIELEMENT_CHILDINDEX).GetInt()]->time_;
                keyid = timeline->GetTimeKey(time)->id_;
            }

            keyid = Clamp(keyid, 0, numTimeKeys-1);
            timeSlider_->SetValue(timeline->keys_[keyid]->time_* 1000.f);
        }
        else
        {
            const int numMainKeys = animation->mainlineKeys_.Size();

            int mainKeyId = editedAnimatedSprite_->GetSpriterInstance()->GetCurrentMainKey()->id_;
            if (button->GetName() == "AnimPrevKey")
            {
                mainKeyId--;
                if (mainKeyId < 0)
                    mainKeyId = numMainKeys-1;
            }
            else if (button->GetName() == "AnimNextKey")
            {
                mainKeyId++;
                if (mainKeyId > numMainKeys-1)
                    mainKeyId = 0;
            }
            else if (button->GetName() == "AnimTimeKey")
            {
                mainKeyId = button->GetVar(UIELEMENT_CHILDINDEX).GetInt();
            }

            mainKeyId = Clamp(mainKeyId, 0, numMainKeys-1);

            timeSlider_->SetValue(animation->mainlineKeys_[mainKeyId]->time_* 1000.f);
        }
    }
}

void AnimatorEditor::HandleAnimationChangeTime(StringHash eventType, VariantMap& eventData)
{
    if (eventType == E_SLIDERPAGED)
    {
        if (eventData[SliderPaged::P_PRESSED].GetBool())
        {
            float offset = (float)eventData[SliderPaged::P_OFFSET].GetInt() / timeSlider_->GetWidth() * editedAnimatedSprite_->GetSpriterInstance()->GetAnimation()->length_ * 1000.f;
            if (offset != 0.f)
            {
                if (isPlaying_)
                {
//                    URHO3D_LOGINFOF("AnimatorEditor() - HandleAnimationChangeTime : E_SLIDERPAGED ... timeoffset=%F", offset);
                    PlayAnimation(false);
                }

                timeSlider_->ChangeValue(offset);
            }
        }
    }
    else if (eventType == E_SLIDERCHANGED)
    {
        if (!isPlaying_)
        {
            float time = timeSlider_->GetValue() / 1000.f;

//            URHO3D_LOGINFOF("AnimatorEditor() - HandleAnimationChangeTime : E_SLIDERCHANGED ... time=%F", time);

            editedAnimatedSprite_->GetSpriterInstance()->ResetCurrentTime();
            editedAnimatedSprite_->UpdateAnimation(time);
            UpdateBonesGizmos();
            UpdateTimelineInfos();
        }
    }
}

void AnimatorEditor::HandleAnimationPlay(StringHash eventType, VariantMap& eventData)
{
    if (!editedAnimatedSprite_)
    {
        UnsubscribeFromEvent(E_POSTUPDATE);
        isPlaying_ = false;
        return;
    }

    editedAnimatedSprite_->UpdateAnimation(eventData[PostUpdate::P_TIMESTEP].GetFloat());

    timeSlider_->SetRange(editedAnimatedSprite_->GetSpriterInstance()->GetAnimation()->length_ * 1000.f);
    timeSlider_->SetValue(editedAnimatedSprite_->GetCurrentAnimationTime() * 1000.f);

    UpdateBonesGizmos();
}


/// Sprite Mapping Panel

void AnimatorEditor::SetSpriteMappingPanel()
{
    URHO3D_LOGINFOF("AnimatorEditor() - SetSpriteMappingPanel");
    UIElement* panel = MapEditorLibImpl::GetPanel(PANEL_ANIMATOR2D_SWSPRITES);

    SubscribeToEvent(panel->GetChild("ShowSwap", true), E_TOGGLED, URHO3D_HANDLER(AnimatorEditor, HandleSpriteMappingPanelToggleShow));
    SubscribeToEvent(panel->GetChild("ShowOffset", true), E_TOGGLED, URHO3D_HANDLER(AnimatorEditor, HandleSpriteMappingPanelToggleShow));

    SubscribeToEvent(panel->GetChild("UnMap", true), E_PRESSED, URHO3D_HANDLER(AnimatorEditor, HandleSpriteMappingPanelSelectionUnMap));
    SubscribeToEvent(panel->GetChild("Hide", true), E_PRESSED, URHO3D_HANDLER(AnimatorEditor, HandleSpriteMappingPanelSelectionHide));
    SubscribeToEvent(panel->GetChild("UnSwap", true), E_PRESSED, URHO3D_HANDLER(AnimatorEditor, HandleSpriteMappingPanelSelectionUnSwap));

    UpdateSpriteMappingPanel();
}

UIElement* AnimatorEditor::GetSpriteMappingListItem(unsigned key) const
{
    const unsigned numitems = spriteMappingList_->GetNumItems();
    for (unsigned i = 0; i < numitems; i++)
    {
        UIElement* item = spriteMappingList_->GetItem(i);
        if (item->GetVar(MAPPINGSPRITE_KEY).GetUInt() == key)
            return item;
    }
    return 0;
}

void AnimatorEditor::UpdateSpriteMappingListItem(UIElement* item, Spriter::MapInstruction* instruct)
{
    if (!item)
        return;

    const bool ismapped = instruct && instruct->targetFolder_ != -1;
    Sprite2D* mappedSprite = ismapped ? editedAnimatedSprite_->GetAnimationSet()->GetSpriterFileSprite(instruct->targetFolder_, instruct->targetFile_) : 0;
    Sprite2D* swappedSprite = mappedSprite ? editedAnimatedSprite_->GetSwappedSprite(mappedSprite) : 0;

    UIElement* mappedSpriteElt = item->GetChild("MappedSprite", true);
    if (mappedSprite)
        static_cast<BorderImage*>(mappedSpriteElt->GetChild(0))->SetSprite(mappedSprite);
    else
        static_cast<BorderImage*>(mappedSpriteElt->GetChild(0))->SetTexture(0);
    static_cast<Text*>(mappedSpriteElt->GetChild(1)->GetChild(0))->SetText(mappedSprite ? mappedSprite->GetName() : (instruct ? "hidden" : "none"));

    UIElement* swappedSpriteElt = item->GetChild("SwappedSprite", true);
    if (swappedSprite)
        static_cast<BorderImage*>(swappedSpriteElt->GetChild(0))->SetSprite(swappedSprite);
    else
        static_cast<BorderImage*>(swappedSpriteElt->GetChild(0))->SetTexture(0);
    static_cast<Text*>(swappedSpriteElt->GetChild(1)->GetChild(0))->SetText(swappedSprite ? swappedSprite->GetName(): "none");

    UIElement* offsetElt = item->GetChild("Offset", true);
    static_cast<Text*>(offsetElt->GetChild(0))->SetText(ismapped ? String().AppendWithFormat("p:%F,%F s:%F,%F r:%F", instruct->targetdx_, instruct->targetdy_, instruct->targetscalex_, instruct->targetscaley_, instruct->targetdangle_) : String::EMPTY);
}

void AnimatorEditor::UpdateSpriteMappingPanel()
{
    UIElement* panel = MapEditorLibImpl::GetPanel(PANEL_ANIMATOR2D_SWSPRITES);

    URHO3D_LOGINFOF("AnimatorEditor() - UpdateSpriteMappingPanel");

    spriteMappingList_ = static_cast<ListView*>(panel->GetChild("MappedSwappedSprites", true));
    spriteMappingList_->SetMultiselect(true);
    spriteMappingList_->RemoveAllItems();
    spriteMappingList_->SetDefaultStyle(MapEditorLibImpl::GetUIStyle(UISTYLE_EDITORDEFAULT));

    bool showswap = static_cast<CheckBox*>(panel->GetChild("ShowSwap", true))->IsChecked();
    UIElement* swapColumn = static_cast<ListView*>(panel->GetChild("SwapColumn", true));
    swapColumn->SetVisible(showswap);

    bool showoffset = static_cast<CheckBox*>(panel->GetChild("ShowOffset", true))->IsChecked();
    UIElement* offsetColumn = static_cast<ListView*>(panel->GetChild("OffsetColumn", true));
    offsetColumn->SetVisible(showoffset);

    LineEdit* lineEdit = static_cast<LineEdit*>(panel->GetChild("CMapName", true));
    if (lineEdit->GetText().Empty())
        return;

    Spriter::CharacterMap* spritemap = editedAnimatedSprite_->GetCharacterMap(StringHash(lineEdit->GetText()));
    if (!spritemap)
    {
        lineEdit->SetText(String::EMPTY);
        return;
    }

    AnimationSet2D* animationSet = editedAnimatedSprite_->GetAnimationSet();

    int slotid = 0;
    const HashMap<unsigned, SharedPtr<Sprite2D> >& spritemapping = editedAnimatedSprite_->GetAnimationSet()->GetSpriteMapping();
//    const HashMap<unsigned, SharedPtr<Sprite2D> >& spritemapping = editedAnimatedSprite_->GetSpriteMapping();
    for (HashMap<unsigned, SharedPtr<Sprite2D> >::ConstIterator it = spritemapping.Begin(); it != spritemapping.End(); ++it)
    {
        unsigned spritekey = it->first_;

        Text* item = new Text(context_);
        item->SetDefaultStyle(MapEditorLibImpl::GetUIStyle(UISTYLE_EDITORDEFAULT));
        item->SetStyle("FileSelectorListText");
        item->SetMinSize(0, 32);
        item->SetMaxSize(2147483647, 32);
        item->SetLayoutMode(LM_HORIZONTAL);
        item->LoadChildXML(MapEditorLibImpl::GetXmlElement(EDITORELT_ANIMATORMAPSWAPSPRITE), MapEditorLibImpl::GetUIStyle(UISTYLE_EDITORDEFAULT));

        item->SetVar(MAPPINGSPRITE_KEY, spritekey);
        item->SetVar(UIELEMENT_CHILDINDEX, slotid);
        static_cast<Text*>(item->GetChild("OriginSprite", true)->GetChild(0))->SetText(animationSet->GetSpriterFileSprite(spritekey)->GetName());

        UIElement* mappedSpriteElt = item->GetChild("MappedSprite", true);
        mappedSpriteElt->SetVar(UIELEMENT_CHILDINDEX, slotid);
        SubscribeToEvent(mappedSpriteElt->GetChild(1), E_PRESSED, URHO3D_HANDLER(AnimatorEditor, HandleSpriteMappingPanelSelectMappedSprite));

        UIElement* swappedSpriteElt = item->GetChild("SwappedSprite", true);
        swappedSpriteElt->SetVisible(showswap);
        if (showswap)
        {
            swappedSpriteElt->SetVar(UIELEMENT_CHILDINDEX, slotid);
            SubscribeToEvent(swappedSpriteElt->GetChild(1), E_PRESSED, URHO3D_HANDLER(AnimatorEditor, HandleSpriteMappingPanelSelectSwappedSprite));
        }

        UIElement* offsetElt = item->GetChild("Offset", true);
        offsetElt->SetVisible(showoffset);

        Text* offsettextelt = static_cast<Text*>(offsetElt->GetChild(0));
        offsettextelt->SetFontSize(7);
        offsettextelt->SetWordwrap(true);
        offsettextelt->SetHorizontalAlignment(HA_CENTER);

        UpdateSpriteMappingListItem(item, spritemap->GetInstruction(spritekey));

        spriteMappingList_->AddItem(item);
        slotid++;
    }

    editedSlotIndex_ = -1;
}

void AnimatorEditor::HandleSpriteMappingPanelToggleShow(StringHash eventType, VariantMap& eventData)
{
    bool enable = eventData[Toggled::P_STATE].GetBool();
    URHO3D_LOGINFOF("AnimatorEditor() - HandleSpriteMappingPanelToggleShow : enable=%s", enable?"true":"false");

    UpdateSpriteMappingPanel();
}

static bool sAnimatorEditorForceSwapping_ = false;

void AnimatorEditor::HandleSpriteMappingPanelSelectMappedSprite(StringHash eventType, VariantMap& eventData)
{
    MapEditorLibImpl::Get()->SetVisible(PANEL_ANIMATOR2D_SSELECTLIST, true);
    Button* mappedbutton = static_cast<Button*>(eventData[Pressed::P_ELEMENT].GetPtr());
    editedSlotIndex_ = mappedbutton->GetParent()->GetVar(UIELEMENT_CHILDINDEX).GetInt();
    sAnimatorEditorForceSwapping_ = false;
}

void AnimatorEditor::HandleSpriteMappingPanelSelectSwappedSprite(StringHash eventType, VariantMap& eventData)
{
    MapEditorLibImpl::Get()->SetVisible(PANEL_ANIMATOR2D_SSELECTLIST, true);
    Button* swappedbutton = static_cast<Button*>(eventData[Pressed::P_ELEMENT].GetPtr());
    editedSlotIndex_ = swappedbutton->GetParent()->GetVar(UIELEMENT_CHILDINDEX).GetInt();
    sAnimatorEditorForceSwapping_ = true;
}

void AnimatorEditor::HandleSpriteMappingPanelSelectionUnMap(StringHash eventType, VariantMap& eventData)
{
    UIElement* panel = MapEditorLibImpl::GetPanel(PANEL_ANIMATOR2D_SWSPRITES);
    LineEdit* lineEdit = static_cast<LineEdit*>(panel->GetChild("CMapName", true));
    if (lineEdit->GetText().Empty())
        return;

    Spriter::CharacterMap* spritemap = editedAnimatedSprite_->GetCharacterMap(StringHash(lineEdit->GetText()));
    if (!spritemap)
    {
        lineEdit->SetText(String::EMPTY);
        return;
    }

    ListView* mappedSwappedSprites = static_cast<ListView*>(panel->GetChild("MappedSwappedSprites", true));
    PODVector<UIElement*> selectedSlots = mappedSwappedSprites->GetSelectedItems();
    if (selectedSlots.Size())
    {
        const HashMap<unsigned, SharedPtr<Sprite2D> >& spritemapping = editedAnimatedSprite_->GetAnimationSet()->GetSpriteMapping();
        for (unsigned i=0; i < selectedSlots.Size(); i++)
        {
            UIElement* slot = selectedSlots[i];
            Spriter::MapInstruction* instruction = spritemap->GetInstruction(slot->GetVar(MAPPINGSPRITE_KEY).GetUInt());
            if (instruction)
                spritemap->maps_.Remove(instruction);
            UpdateSpriteMappingListItem(slot, 0);
        }

        UpdateMapping(true, true);
    }
}

void AnimatorEditor::HandleSpriteMappingPanelSelectionHide(StringHash eventType, VariantMap& eventData)
{
    UIElement* panel = MapEditorLibImpl::GetPanel(PANEL_ANIMATOR2D_SWSPRITES);
    LineEdit* lineEdit = static_cast<LineEdit*>(panel->GetChild("CMapName", true));
    if (lineEdit->GetText().Empty())
        return;

    Spriter::CharacterMap* spritemap = editedAnimatedSprite_->GetCharacterMap(StringHash(lineEdit->GetText()));
    if (!spritemap)
    {
        lineEdit->SetText(String::EMPTY);
        return;
    }

    ListView* mappedSwappedSprites = static_cast<ListView*>(panel->GetChild("MappedSwappedSprites", true));
    PODVector<UIElement*> selectedSlots = mappedSwappedSprites->GetSelectedItems();
    if (selectedSlots.Size())
    {
        const HashMap<unsigned, SharedPtr<Sprite2D> >& spritemapping = editedAnimatedSprite_->GetAnimationSet()->GetSpriteMapping();
        for (unsigned i=0; i < selectedSlots.Size(); i++)
        {
            UIElement* slot = selectedSlots[i];
            Spriter::MapInstruction* instruction = spritemap->GetInstruction(slot->GetVar(MAPPINGSPRITE_KEY).GetUInt(), true);
            instruction->RemoveTarget();
            UpdateSpriteMappingListItem(slot, instruction);
        }

        UpdateMapping(true, true);
    }
}

void AnimatorEditor::HandleSpriteMappingPanelSelectionUnSwap(StringHash eventType, VariantMap& eventData)
{
    UIElement* panel = MapEditorLibImpl::GetPanel(PANEL_ANIMATOR2D_SWSPRITES);
    LineEdit* lineEdit = static_cast<LineEdit*>(panel->GetChild("CMapName", true));
    if (lineEdit->GetText().Empty())
        return;

    Spriter::CharacterMap* spritemap = editedAnimatedSprite_->GetCharacterMap(StringHash(lineEdit->GetText()));
    if (!spritemap)
    {
        lineEdit->SetText(String::EMPTY);
        return;
    }

    ListView* mappedSwappedSprites = static_cast<ListView*>(panel->GetChild("MappedSwappedSprites", true));
    PODVector<UIElement*> selectedSlots = mappedSwappedSprites->GetSelectedItems();
    if (selectedSlots.Size())
    {
        const HashMap<unsigned, SharedPtr<Sprite2D> >& spritemapping = editedAnimatedSprite_->GetAnimationSet()->GetSpriteMapping();
        for (unsigned i=0; i < selectedSlots.Size(); i++)
        {
            UIElement* slot = selectedSlots[i];
            Spriter::MapInstruction* instruction = spritemap->GetInstruction(slot->GetVar(MAPPINGSPRITE_KEY).GetUInt(), true);
            editedAnimatedSprite_->UnSwapSprite(editedAnimatedSprite_->GetAnimationSet()->GetSpriterFileSprite(instruction->targetFolder_, instruction->targetFile_));

            UpdateSpriteMappingListItem(slot, instruction);
        }

        UpdateMapping(true, true);
    }
}


///  Atlas Panel

void AnimatorEditor::SetSpriteSelectionList(SpriteSheet2D* spritesheet)
{
    URHO3D_LOGINFOF("AnimatorEditor() - SetSpriteSelectionList");

    if (!spritesheet)
        spritesheet = editedAnimatedSprite_->GetAnimationSet()->GetSpriteSheet();

    UIElement* panel = MapEditorLibImpl::GetPanel(PANEL_ANIMATOR2D_SSELECTLIST);

    SubscribeToEvent(panel->GetChild("ShowPivots", true), E_TOGGLED, URHO3D_HANDLER(AnimatorEditor, HandleSpriteSelectionShowPivots));

    ListView* spritesList = static_cast<ListView*>(panel->GetChild("SpritesList", true));
    spritesList->RemoveAllItems();

    PODVector<Sprite2D*> externalSprites;
    const HashMap<String, SharedPtr<Sprite2D> >& sheetsprites = spritesheet->GetSpriteMapping();
    const HashMap<unsigned, SharedPtr<Sprite2D> >& setSprites = editedAnimatedSprite_->GetAnimationSet()->GetSpriteMapping();
    // find sheet sprites that are not inside AnimationSet
    for (HashMap<String, SharedPtr<Sprite2D> >::ConstIterator it = sheetsprites.Begin(); it != sheetsprites.End(); ++it)
    {
        Sprite2D* sprite = it->second_.Get();
        bool hasSprite = false;
        for (HashMap<unsigned, SharedPtr<Sprite2D> >::ConstIterator jt = setSprites.Begin(); jt != setSprites.End(); ++jt)
        {
            if (jt->second_->GetName() == sprite->GetName() && jt->second_->GetTexture() == sprite->GetTexture())
            {
                hasSprite = true;
                break;
            }
        }
        if (!hasSprite)
            externalSprites.Push(sprite);
    }

    const unsigned numSpritesByRow = 4;
    const unsigned numSprites = setSprites.Size() + externalSprites.Size();
    const unsigned numRows = numSprites / numSpritesByRow;
    const int numSpritesLastRow = numSprites - numRows * numSpritesByRow;
    // set sprites row
    for (unsigned i = 0; i < numRows; i++)
    {
        UIElement* row = new UIElement(context_);
        row->SetLayoutMode(LM_HORIZONTAL);
        row->SetLayoutSpacing(4);
        row->SetHorizontalAlignment(HA_CENTER);
        for (unsigned j = 0; j < numSpritesByRow; j++)
            row->AddChild(AddListButton(String::EMPTY, 0, true));
        spritesList->AddItem(row);
    }
    // set sprites last row
    if (numSpritesLastRow)
    {
        UIElement* row = new UIElement(context_);
        row->SetLayoutMode(LM_HORIZONTAL);
        row->SetLayoutSpacing(4);
        row->SetHorizontalAlignment(HA_CENTER);
        for (unsigned i = 0; i < numSpritesLastRow; i++)
            row->AddChild(AddListButton(String::EMPTY, 0, true));
        spritesList->AddItem(row);
    }
    // set Sprites
    unsigned i = 0;
    for (HashMap<unsigned, SharedPtr<Sprite2D> >::ConstIterator it = setSprites.Begin(); it != setSprites.End(); ++it, i++)
    {
        Sprite2D* sprite = it->second_.Get();
        int row = i / numSpritesByRow;
        int isprite = i - row * numSpritesByRow;

        UIElement* button = spritesList->GetItem(row)->GetChild(isprite);
        button->SetVar(MAPPINGSPRITE_KEY, it->first_);
        button->SetVar(SWAPPINGSPRITE_PTR, sprite);

        SubscribeToEvent(button, E_PRESSED, URHO3D_HANDLER(AnimatorEditor, HandleSpriteSelectionSpriteSelected));
        SubscribeToEvent(button, E_CLICK, URHO3D_HANDLER(AnimatorEditor, HandleSpriteSelectionStartEndMovePivot));

        BorderImage* spritebtn = static_cast<Button*>(button->GetChild(0));
        spritebtn->SetTexture(sprite->GetTexture());
        spritebtn->SetImageRect(sprite->GetRectangle());
        button->SetColor(Color::WHITE);
        spritebtn->SetColor(Color::WHITE);
        Text* textbtn = static_cast<Text*>(button->GetChild(1));
        textbtn->SetText(sprite->GetName());

        // set the default pivot
        unsigned folderid, fileid;
        Spriter::GetFolderFile(it->first_, folderid, fileid);
        Spriter::File* file = editedAnimatedSprite_->GetAnimationSet()->GetSpriterData()->folders_[folderid]->files_[fileid];
        button->GetChild("Pivot", true)->SetPosition(Min(file->pivotX_ * button->GetChild(0)->GetWidth(), button->GetChild(0)->GetWidth()-1), Min((1.f - file->pivotY_) * button->GetChild(0)->GetHeight(), button->GetChild(0)->GetHeight()-1));
    }
    // add external Sprites
    for (unsigned j=0; j < externalSprites.Size(); j++)
    {
        Sprite2D* sprite = externalSprites[j];
        int row = (i+j) / numSpritesByRow;
        int isprite = (i+j) - row * numSpritesByRow;

        UIElement* button = spritesList->GetItem(row)->GetChild(isprite);
        button->SetVar(MAPPINGSPRITE_KEY, SPRITEKEY_SWAP);
        button->SetVar(SWAPPINGSPRITE_PTR, sprite);

        SubscribeToEvent(button, E_PRESSED, URHO3D_HANDLER(AnimatorEditor, HandleSpriteSelectionSpriteSelected));
        SubscribeToEvent(button, E_CLICK, URHO3D_HANDLER(AnimatorEditor, HandleSpriteSelectionStartEndMovePivot));

        BorderImage* spritebtn = static_cast<Button*>(button->GetChild(0));
        spritebtn->SetTexture(sprite->GetTexture());
        spritebtn->SetImageRect(sprite->GetRectangle());
        button->SetColor(Color(0.25,0.25,0.25,1));
        spritebtn->SetColor(Color::WHITE);
        Text* textbtn = static_cast<Text*>(button->GetChild(1));
        textbtn->SetText(sprite->GetName());

        // set the default pivot from Sprite HotSpot
        button->GetChild("Pivot", true)->SetPosition(Min(sprite->GetHotSpot().x_ * button->GetChild(0)->GetWidth(), button->GetChild(0)->GetWidth()-1), Min(sprite->GetHotSpot().y_ * button->GetChild(0)->GetHeight(), button->GetChild(0)->GetHeight()-1));
    }
}

void AnimatorEditor::HandleSpriteSelectionSpriteSelected(StringHash eventType, VariantMap& eventData)
{
//    URHO3D_LOGINFOF("AnimatorEditor() - HandleSpriteSelectionSpriteSelected slotindex=%d", editedSlotIndex_);

    UIElement* panel = MapEditorLibImpl::GetPanel(PANEL_ANIMATOR2D_SWSPRITES);
    UIElement* spritebutton = static_cast<UIElement*>(eventData[Pressed::P_ELEMENT].GetPtr());
    StringHash cmap = StringHash(static_cast<LineEdit*>(panel->GetChild("CMapName", true))->GetText());

    const unsigned spritekey = spriteMappingList_->GetItem(editedSlotIndex_)->GetVar(MAPPINGSPRITE_KEY).GetUInt();
    const unsigned targetkey = sAnimatorEditorForceSwapping_ ? SPRITEKEY_SWAP : spritebutton->GetVar(MAPPINGSPRITE_KEY).GetUInt();

    Spriter::CharacterMap* spritemap = editedAnimatedSprite_->GetCharacterMap(cmap);
    Spriter::MapInstruction* instruction = spritemap->GetInstruction(spritekey);

    // add new mapinstruction
    if (!instruction && targetkey != SPRITEKEY_IGNORE)
    {
        instruction = spritemap->GetInstruction(spritekey, true);
    }

    if (instruction)
    {
        if (targetkey == SPRITEKEY_IGNORE)
        {
            instruction = spritemap->RemoveInstruction(spritekey);
        }
        else if (targetkey == SPRITEKEY_REMOVE)
        {
            instruction->RemoveTarget();
        }
        else if (targetkey == SPRITEKEY_SWAP)
        {
            instruction->SetTarget(spritekey);
            Sprite2D* mappedSprite = editedAnimatedSprite_->GetAnimationSet()->GetSpriterFileSprite(instruction->targetFolder_, instruction->targetFile_);
            editedAnimatedSprite_->SwapSprite(mappedSprite, static_cast<Sprite2D*>(spritebutton->GetVar(SWAPPINGSPRITE_PTR).GetPtr()), false);
        }
        else
        {
            instruction->SetTarget(targetkey);
        }

        UpdateSpriteMappingListItem(spriteMappingList_->GetItem(editedSlotIndex_), instruction);
        UpdateMapping(true, true);
    }
}

static WeakPtr<Button> pivotMoverSelectedSpriteBtn_;
static bool pivotMoverSelectedSpriteBtnAutoHide_;
static Vector2 pivotMoverValue_;

void AnimatorEditor::HandleSpriteSelectionStartEndMovePivot(StringHash eventType, VariantMap& eventData)
{
    if (eventData[Click::P_BUTTON].GetInt() == MOUSEB_MIDDLE)
    {
        pivotMoverSelectedSpriteBtn_ = static_cast<Button*>(eventData[Click::P_ELEMENT].GetPtr());
        UIElement* pivotelt = pivotMoverSelectedSpriteBtn_->GetChild("Pivot", true);
        pivotMoverSelectedSpriteBtnAutoHide_ = !pivotelt->IsVisible();
        pivotelt->SetVisible(true);
        SubscribeToEvent(E_POSTRENDERUPDATE, URHO3D_HANDLER(AnimatorEditor, HandleSpriteSelectionMovePivot));
    }
}

void AnimatorEditor::HandleSpriteSelectionMovePivot(StringHash eventType, VariantMap& eventData)
{
    BorderImage* spriteImage = static_cast<BorderImage*>(pivotMoverSelectedSpriteBtn_->GetChild(0));
    IntVector2 mousepositionInButton = spriteImage->ScreenToElement(GameContext::Get().input_->GetMousePosition());

    pivotMoverValue_.x_ = Clamp((float)mousepositionInButton.x_ / spriteImage->GetWidth(), 0.f, 1.f);
    pivotMoverValue_.y_ = Clamp((float)mousepositionInButton.y_ / spriteImage->GetHeight(), 0.f, 1.f);

    pivotMoverSelectedSpriteBtn_->GetChild("Pivot", true)->SetPosition(Min(pivotMoverValue_.x_ * spriteImage->GetWidth(), spriteImage->GetWidth()-1), Min(pivotMoverValue_.y_ * spriteImage->GetHeight(), spriteImage->GetHeight()-1));

    if (!GameContext::Get().input_->GetMouseButtonDown(MOUSEB_MIDDLE))
    {
//        URHO3D_LOGINFOF("AnimatorEditor() - HandleSpriteSelectionStartEndMovePivot : End Pivot Modifying ...");
        UnsubscribeFromEvent(E_POSTRENDERUPDATE);
        if (pivotMoverSelectedSpriteBtnAutoHide_)
            TimerRemover::Get()->Start(pivotMoverSelectedSpriteBtn_->GetChild("Pivot", true), 2.f, NOVISIBLE);

        // Update Sprite Pivot
        unsigned spritekey = pivotMoverSelectedSpriteBtn_->GetVar(MAPPINGSPRITE_KEY).GetUInt();
        // Update the Default Pivot in the spriterdata and the sprite2d in AnimationSet2D
        if (spritekey != SPRITEKEY_SWAP)
        {
            unsigned folderid, fileid;
            Spriter::GetFolderFile(spritekey, folderid, fileid);
            Spriter::File* file = editedAnimatedSprite_->GetAnimationSet()->GetSpriterData()->folders_[folderid]->files_[fileid];
            file->pivotX_ = pivotMoverValue_.x_;
            file->pivotY_ = 1.f - pivotMoverValue_.y_;
            // Need Update Key Infos
            editedAnimatedSprite_->GetAnimationSet()->GetSpriterData()->UpdateKeyInfos();

            // Sprite from AnimationSet Mapping
            Sprite2D* sprite = static_cast<Sprite2D*>(pivotMoverSelectedSpriteBtn_->GetVar(SWAPPINGSPRITE_PTR).GetPtr());
            sprite->SetHotSpot(Vector2(file->pivotX_, file->pivotY_));
            editedAnimatedSprite_->ForceUpdateBatches();

            URHO3D_LOGINFOF("AnimatorEditor() - HandleSpriteSelectionStartEndMovePivot : End Update Sprite=%s Pivot=%F %F", sprite->GetName().CString(), file->pivotX_, file->pivotY_);
        }
        // Update the sprite hotspot in the spritesheet or add the sprite in the spriterdata/AnimationSet2D ?
        else
        {

        }

        pivotMoverSelectedSpriteBtn_.Reset();
    }
}

void AnimatorEditor::HandleSpriteSelectionShowPivots(StringHash eventType, VariantMap& eventData)
{
    bool enable = eventData[Toggled::P_STATE].GetBool();

    UIElement* panel = MapEditorLibImpl::GetPanel(PANEL_ANIMATOR2D_SSELECTLIST);
    ListView* spritesList = static_cast<ListView*>(panel->GetChild("SpritesList", true));
    const unsigned numRows = spritesList->GetNumItems();
    for (unsigned i=0; i < numRows; i++)
    {
        UIElement* row = spritesList->GetItem(i);
        const unsigned numSpritesInTheRow = row->GetNumChildren();
        for (unsigned j=0; j < numSpritesInTheRow; j++)
            row->GetChild(j)->GetChild("Pivot", true)->SetVisible(enable);
    }
}


/// Color Panel

void AnimatorEditor::SetColorSpritePanel()
{
    URHO3D_LOGINFOF("AnimatorEditor() - SetSpriteColorMappingPanel");

    AnimationSet2D* animationSet = editedAnimatedSprite_->GetAnimationSet();

    UIElement* panel = MapEditorLibImpl::GetPanel(PANEL_ANIMATOR2D_COLORMAPPING);

    // Set Color List
    ListView* colors = static_cast<ListView*>(panel->GetChild("Colors", true));
    for (unsigned i=0; i < colors->GetNumItems(); i++)
    {
        UIElement* item = colors->GetItem(i);
        item->GetChild(0)->SetColor(EditorSpriteColors_[i]);
        SubscribeToEvent(item, E_PRESSED, URHO3D_HANDLER(AnimatorEditor, HandleColorPressed));
    }
}

void AnimatorEditor::UpdateColorSpritePanel()
{
    URHO3D_LOGINFOF("AnimatorEditor() - UpdateColorSpritePanel");

    AnimationSet2D* animationSet = editedAnimatedSprite_->GetAnimationSet();

    UIElement* panel = MapEditorLibImpl::GetPanel(PANEL_ANIMATOR2D_COLORMAPPING);

    UIElement* colorpanel = MapEditorLibImpl::GetPanel(PANEL_ANIMATOR2D_COLORMAPPING);
    String colormapname = static_cast<LineEdit*>(colorpanel->GetChild("CMapName", true))->GetText();
    Spriter::ColorMap* colormap = editedAnimatedSprite_->GetColorMap(StringHash(colormapname));

    // Set Sprite List
    ListView* colorSprites = static_cast<ListView*>(panel->GetChild("SlotSprites", true));
    colorSprites->RemoveAllItems();
    colorSprites->SetDefaultStyle(MapEditorLibImpl::GetUIStyle(UISTYLE_EDITORDEFAULT));
    colorSprites->SetMultiselect(true);
    colorSprites->SetHighlightMode(HM_ALWAYS);
    const HashMap<unsigned, SharedPtr<Sprite2D> >& spritemapping = animationSet->GetSpriteMapping();
    for (HashMap<unsigned, SharedPtr<Sprite2D> >::ConstIterator it = spritemapping.Begin(); it != spritemapping.End(); ++it)
    {
        Text* item = new Text(context_);
        item->SetDefaultStyle(MapEditorLibImpl::GetUIStyle(UISTYLE_EDITORDEFAULT));
        item->SetStyle("FileSelectorListText");
        item->SetMinSize(0, 32);
        item->SetMaxSize(2147483647, 32);
        item->SetLayoutMode(LM_HORIZONTAL);
        item->LoadChildXML(MapEditorLibImpl::GetXmlElement(EDITORELT_ANIMATORSPRITECOLOR), MapEditorLibImpl::GetUIStyle(UISTYLE_EDITORDEFAULT));
        item->SetVar(MAPPINGSPRITE_KEY, it->first_);
        UIElement* spriteElt = item->GetChild("Sprite", true);
        static_cast<BorderImage*>(spriteElt->GetChild(0))->SetSprite(it->second_.Get());
        static_cast<Text*>(spriteElt->GetChild(1))->SetText(it->second_->GetName());
        colorSprites->AddItem(item);

        if (colormap)
            SetSpriteSlotColor(item, colormap->GetColor(it->first_));
        else
            SetSpriteSlotColor(item, Color::WHITE);
    }
}

void AnimatorEditor::SetSpriteSlotColor(UIElement* slot, const Color& color)
{
    slot->GetChild("Sprite", true)->GetChild(0)->SetColor(color);
    BorderImage* slotcolor = static_cast<BorderImage*>(slot->GetChild("SpriteColor", true)->GetChild(0));
    if (color != Color::WHITE)
    {
        slotcolor->SetStyle("none");
        slotcolor->SetMinSize(24, 24);
        slotcolor->SetMaxSize(24, 24);
        slotcolor->SetTexture(0);
//        slotcolor->SetImageRect(IntRect(0, 0, 256, 128));
        slotcolor->SetColor(color);
    }
    else
    {
        slotcolor->SetStyle("BorderImageEmpty");
    }
}

void AnimatorEditor::HandleColorPressed(StringHash eventType, VariantMap& eventData)
{
    Color color = static_cast<UIElement*>(context_->GetEventSender())->GetChild(0)->GetColor(C_TOPLEFT);

    UIElement* panel = MapEditorLibImpl::GetPanel(PANEL_ANIMATOR2D_COLORMAPPING);
    ListView* colorSprites = static_cast<ListView*>(panel->GetChild("SlotSprites", true));

    PODVector<UIElement*> selectedSlots = colorSprites->GetSelectedItems();
    if (selectedSlots.Size())
    {
//        URHO3D_LOGINFOF("AnimatorEditor() - HandleColorPressed color=%s numselecteditems=%u", color.ToString().CString(), selectedSlots.Size());
        AnimatedSprite2D* animatedSprite = editNode_->GetComponent<AnimatedSprite2D>();

        String colormapname = static_cast<LineEdit*>(panel->GetChild("CMapName", true))->GetText();
        Spriter::ColorMap* colorMap = animatedSprite->GetColorMap(colormapname);

        if (colorMap)
        {
            for (unsigned i=0; i < selectedSlots.Size(); i++)
            {
                UIElement* slot = selectedSlots[i];
                SetSpriteSlotColor(slot, color);

                // Set Color in the colormap (spriter data)
                colorMap->SetColor(slot->GetVar(MAPPINGSPRITE_KEY).GetUInt(), color);

                // Set Color Instantly in animatedSprite
                animatedSprite->SetSpriteColor(slot->GetVar(MAPPINGSPRITE_KEY).GetUInt(), color);
            }
        }
    }
}


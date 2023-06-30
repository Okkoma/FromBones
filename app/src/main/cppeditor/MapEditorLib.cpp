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

#include "GameOptions.h"
#include "GameContext.h"
#include "GameHelpers.h"

#include "MapWorld.h"
#include "Map.h"

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
    "None", "Actor", "Monster", "Plant", "Tile", "Door", "Light", "Decoration", "Container", "Collectable", "Tool", 0
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

const String STRIKED_OUT = "——";   // Two unicode EM DASH (U+2014)
const String TITLETEXT("TitleText");
const String ATTRIBUTELIST("AttributeList");
const String ICONSPANEL("IconsPanel");
const String PARENTCONTAINER("ParentContainer");
const String RESETTODEFAULT("ResetToDefault");
const String TAGSEDIT("TagsEdit");
const String ICON("Icon");
const String UNKNOWN("Unknown");

const StringHash UIELEMENT_CHILDINDEX("UIElementChildIndex");
const StringHash MAPPINGSPRITE_KEY("SpriteKey");
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

unsigned inspectorNodeContainerIndex_;
unsigned inspectorComponentContainerStartIndex_;
bool inspectorShowNonEditableAttribute = false;
bool inspectorInLoadAttributeEditor;
bool inspectorShowID = true;
bool inspectorLocked_ = false;

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
    void HandleMonsterTypeSelected(StringHash eventType, VariantMap& eventData);
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
    void SetMonsterPopList();
    void RemoveUI();
    void ResizeUI();

    void UpdatePanel(int panel, bool force=false);
    void UpdateSpawnToolBar();
    void UpdateInspector(bool fullupdate);
    void UpdateNodesAttributes();

    void SpawnObject(const WorldMapPosition& position);
    bool MoveObjects(const Vector2& adjust);

    void UnSelectObject();
    void SelectObject(Drawable* shape);
    void SelectObject(CollisionShape2D* shape);
    void SelectObject(RenderShape* shape, RenderCollider* collider);
    void UpdateSelectShape();

    void AddShapeToRender(Drawable* shape, unsigned linecolor, unsigned pointcolor);
    void AddShapeToRender(CollisionShape2D* shape, unsigned linecolor, unsigned pointcolor);
    void AddShapeToRender(RenderShape* shape, unsigned linecolor, unsigned pointcolor, bool addBoundingRect=false);
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

    /// Points rendered
    PODVector<EditorPoint> points_;
    /// Lines rendered
    PODVector<EditorLine> lines_;
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
    StringHash spawnMonster_;
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

    static AnimatorEditor* Get()
    {
        if (!animatorEditor_)
            animatorEditor_ = new AnimatorEditor(GameContext::Get().context_);
        return animatorEditor_;
    }

private :
    void SetCharacterMappingPanel();
    void SetSpriteSlotColor(UIElement* slot, const Color& color);
    void SetColorSpritePanel();
    void SetSpriteMappingPanel();
    void SetSpriteSelectionList();
    void SetAnimationPanel();

    UIElement* AddMap(const String& mapname);
    UIElement* ApplyMap(const String& mapname);

    void UpdateColorSpritePanel();
    void UpdateSpriteMappingPanel();
    void UpdateMapping(bool updatesprites, bool updatecolors);
    void FinishEdit();

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
    void HandleColorPressed(StringHash eventType, VariantMap& eventData);
    void HandleSpriteMappingPanelShowSwap(StringHash eventType, VariantMap& eventData);
    void HandleSpriteMappingPanelSelectMappedSprite(StringHash eventType, VariantMap& eventData);
    void HandleSpriteMappingPanelSelectSwappedSprite(StringHash eventType, VariantMap& eventData);
    void HandleSpriteMappingPanelModifySprite(StringHash eventType, VariantMap& eventData);
    void HandleAnimationSelect(StringHash eventType, VariantMap& eventData);
    void HandleAnimationTooglePlay(StringHash eventType, VariantMap& eventData);
    void HandleAnimationPlay(StringHash eventType, VariantMap& eventData);

    SharedPtr<Scene> mainScene_, previewScene_;
    UIElement* panel_;
    View3D* preview_;

    WeakPtr<Node> editNode_;
    WeakPtr<AnimatedSprite2D> editedAnimatedSprite_;
    WeakPtr<Node> cloneNode_;
    unsigned entityid_;
    int editedSlotIndex_;

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
    spawnCategory_(SPAWN_NONE),
    spawnMonster_(StringHash("GOT_Skeleton"))
{
    editor_ = this;

    ResourcePicker::Init();

    vertexBuffer_ = new VertexBuffer(context);

    scene_ = GameContext::Get().rootScene_.Get();

    scene_->SetUpdateEnabled(false);

    debugTextRootNode_ = scene_->GetChild("EditorDebugText", LOCAL);
    if (!debugTextRootNode_)
        debugTextRootNode_ = scene_->CreateChild("EditorDebugText", LOCAL);

    URHO3D_LOGINFOF("MapEditorLibImpl()");

    CreateUI();

    SetVisible(PANEL_MAINTOOLBAR, true);

    SubscribeToUIEvent();
}

MapEditorLibImpl::~MapEditorLibImpl()
{
    if (debugTextRootNode_)
        debugTextRootNode_->Remove();

    RemoveUI();

    if (AnimatorEditor::Get())
        delete AnimatorEditor::Get();

    scene_->SetUpdateEnabled(true);

    URHO3D_LOGINFOF("~MapEditorLibImpl()");
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

    SetMonsterPopList();

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

void CreateToolBarIcon(UIElement* element)
{
    BorderImage* icon = new BorderImage(GameContext::Get().context_);
    icon->SetName("Icon");
    icon->SetDefaultStyle(MapEditorLibImpl::GetUIStyle(UISTYLE_TOOLBARICONS));
    icon->SetStyle(element->GetName());
    icon->SetFixedSize(60, 60);
    icon->SetBlendMode(BLEND_ALPHA);
    element->AddChild(icon);
}

UIElement* CreateToolTip(UIElement* parent, const String& title, const IntVector2& offset)
{
    ToolTip* toolTip = parent->CreateChild<ToolTip>("ToolTip");
    toolTip->SetPosition(offset);

    BorderImage* textHolder = toolTip->CreateChild<BorderImage>("BorderImage");
    textHolder->SetStyle("ToolTipBorderImage");
    textHolder->SetHorizontalAlignment(HA_CENTER);

    Text* toolTipText = textHolder->CreateChild<Text>("Text");
    toolTipText->SetStyle("ToolTipText");
    toolTipText->SetAutoLocalizable(true);
    toolTipText->SetText(title);

    return toolTip;
}

Button* CreateToolBarButton(const String title)
{
    Button* button = new Button(GameContext::Get().context_);
    button->SetName(title);
    button->SetDefaultStyle(MapEditorLibImpl::GetUIStyle(UISTYLE_EDITORDEFAULT));
    button->SetStyle("ToolBarButton");

    CreateToolBarIcon(button);
    CreateToolTip(button, title, IntVector2(button->GetWidth()/2, -20));

    return button;
}

CheckBox* CreateToolBarToggle(const String& title)
{
    CheckBox* toggle = new CheckBox(GameContext::Get().context_);
    toggle->SetName(title);
    toggle->SetDefaultStyle(MapEditorLibImpl::GetUIStyle(UISTYLE_EDITORDEFAULT));
    toggle->SetStyle("ToolBarToggle");

    CreateToolBarIcon(toggle);
    CreateToolTip(toggle, title, IntVector2(toggle->GetWidth()/2, -20));

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
        spawngroup->AddChild(CreateToolBarToggle(SpawnCategoryNameStr[i]));

    FinalizeGroupHorizontal(spawngroup, "ToolBarToggle");
    toolbar->AddChild(spawngroup);
}

UIElement* AddListButton(const String& name, Sprite2D* sprite)
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
    button->AddChild(spritebtn);

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

void MapEditorLibImpl::SetMonsterPopList()
{
    UIElement* panel = GetPanel(PANEL_MONSTERPOPLIST);
    IntVector2 posbutton = GetPanel(PANEL_MAINTOOLBAR)->GetChild(0)->GetChild(SPAWN_MONSTER)->GetPosition();
    panel->SetPosition(posbutton.x_ - 60, GameContext::Get().targetheight_ - (panel->GetSize().y_ + 60));

    ListView* spritesList = static_cast<ListView*>(panel->GetChild("SpritesList", true));
    spritesList->RemoveAllItems();

    const Vector<StringHash>& monsterTypes = COT::GetTypesInCategory(COT::MONSTERS);

    const unsigned numSpritesByRow = 4;
    const unsigned numSprites = monsterTypes.Size();
    const unsigned numRows = numSprites / numSpritesByRow;
    const int numSpritesLastRow = numSprites - numRows * numSpritesByRow;

    PODVector<UIElement*> buttons;

    unsigned monsterindex = 0;

    // set sprites row
    for (unsigned i = 0; i < numRows; i++)
    {
        UIElement* row = new UIElement(context_);
        row->SetLayoutMode(LM_HORIZONTAL);
        row->SetLayoutSpacing(4);
        row->SetHorizontalAlignment(HA_CENTER);
        for (unsigned j = 0; j < numSpritesByRow; j++)
        {
            StringHash hash = monsterTypes[monsterindex];
            const String& monsterName = GOT::GetType(hash);
            Sprite2D* sprite = GameContext::Get().spriteSheets_[UIEQUIPMENT]->GetSprite(monsterName);
            if (!sprite)
                sprite = GOT::GetObject(hash)->GetDerivedComponent<StaticSprite2D>()->GetSprite();

            UIElement* elt = AddListButton(monsterName, sprite);
            buttons.Push(elt);
            row->AddChild(elt);
            monsterindex++;
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
            StringHash hash = monsterTypes[monsterindex];
            const String& monsterName = GOT::GetType(hash);
            Sprite2D* sprite = GameContext::Get().spriteSheets_[UIEQUIPMENT]->GetSprite(monsterName);
            if (!sprite)
                sprite = GOT::GetObject(hash)->GetDerivedComponent<StaticSprite2D>()->GetSprite();

            UIElement* elt = AddListButton(monsterName, sprite);
            buttons.Push(elt);
            row->AddChild(elt);
            monsterindex++;
        }
        spritesList->AddItem(row);
    }

    // subscribe To Event
    for (unsigned i=0; i < buttons.Size(); i++)
    {
        buttons[i]->SetVar(TYPE_VAR, monsterTypes[i]);
        SubscribeToEvent(buttons[i], E_PRESSED, URHO3D_HANDLER(MapEditorLibImpl, HandleMonsterTypeSelected));
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
    bool editable = info.mode_ & AM_NOEDIT == 0;

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

            componentTitle->SetText(GetComponentTitle(editComponents_[j * numEditableComponents]) + multiplierText);
            IconizeUIElement(componentTitle, editComponents_[j * numEditableComponents]->GetTypeName());
            SetIconEnabledColor(componentTitle, editComponents_[j * numEditableComponents]->IsEnabledEffective());

            URHO3D_LOGINFOF("MapEditorLibImpl() - UpdateInspector ... j=%u editComponent=%s title=%s ...", j, editComponents_[j * numEditableComponents]->GetTypeName().CString(), componentTitle->GetText().CString());

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
            // Find the smallest drawable
            Drawable* drawable = 0;
            PODVector<RayQueryResult> results;
            RayOctreeQuery query(results, ray, RAY_TRIANGLE, GameContext::Get().camera_->GetFarClip(), pickModeDrawableFlags[pickMode_]);
            GameContext::Get().octree_->Raycast(query);

            if (results.Size() > 0)
            {
                float area = M_INFINITY;
                for (unsigned i=0; i < results.Size(); i++)
                {
                    Drawable* d = results[i].drawable_;
                    if (d->IsInstanceOf<ObjectTiled>() || d->IsInstanceOf<RenderShape>() || d->IsInstanceOf<DrawableScroller>())
                        continue;

                    float newarea = d->GetWorldArea2D();
                    if (newarea >= area)
                        continue;

                    area = newarea;
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
    else
    {
        clickMode_ = CLICK_NONE;
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
        }
    }
    else if (clickMode_ == CLICK_MOVE)
    {
        // Move Objects
        if (!editNodes_.Empty())
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
        if (clickMode_ == CLICK_SPAWN)
        {
            SpawnObject(position);
        }

        else if (drawableUnderMouse_ || shapeUnderMouse_)
        {
            // Draw Picked Object
            if (drawableUnderMouse_ != AnimatorEditor::Get()->GetAnimatedSprite())
            {
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
        Node* entity = World2D::GetWorld()->SpawnEntity(spawnMonster_);
        if (entity)
        {
            drawable = entity->GetDerivedComponent<Drawable2D>();
            SelectObject(drawable);

            URHO3D_LOGINFOF("MapEditorLibImpl() - SpawnObject : Spawn Monster - entity=%s(%u)", entity->GetName().CString(), entity->GetID());
        }
    }
    else if (spawnCategory_ == SPAWN_PLANT)
    {
        Node* entity = World2D::GetWorld()->SpawnFurniture(StringHash("FUR_LTreeGrow"));
        if (entity)
        {
            drawable = entity->GetDerivedComponent<Drawable2D>();
            SelectObject(drawable);
            URHO3D_LOGINFOF("MapEditorLibImpl() - SpawnObject : Spawn Plant !");
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

    if (drawable)
    {
        // because the scene is not enable to update, we force by marking in the view and update manually the scene
        drawable->MarkInView(GameContext::Get().renderer2d_->GetFrameInfo());
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
        }
    }

    return moved;
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
    debugTextRootNode_->SetEnabledRecursive(false);

    if (selectedDrawable_ && selectedDrawable_ != AnimatorEditor::Get()->GetAnimatedSprite())
    {
        AddShapeToRender(selectedDrawable_, Color::GRAY.ToUInt(), Color::YELLOW.ToUInt());
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
    {
        GetPanel(PANEL_MONSTERPOPLIST)->SetVisible(checkbox->IsChecked());
    }
}

void MapEditorLibImpl::HandleMonsterTypeSelected(StringHash eventType, VariantMap& eventData)
{
    Button* button = static_cast<Button*>(eventData["Element"].GetPtr());
    if (!button)
        return;

    spawnMonster_ = button->GetVar(TYPE_VAR).GetStringHash();
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

    if (lines_.Empty() && points_.Empty())
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

    unsigned numVertices = points_.Size() + lines_.Size() * 2;

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

    Camera* camera = scene_->GetContext()->GetSubsystem<Renderer>()->GetViewport(0)->GetCamera();
    Matrix3x4 view = camera->GetView();
    Matrix3x4 gpuProjection = camera->GetGPUProjection();

    vertexBuffer_->Unlock();

    graphics->SetBlendMode(BLEND_ALPHA);
    graphics->SetColorWrite(true);
    graphics->SetCullMode(CULL_NONE);
    graphics->SetDepthWrite(true);
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
        graphics->SetLineWidth(linewidth);

        count = lines_.Size() * 2;
        graphics->SetDepthTest(CMP_LESSEQUAL);
        graphics->Draw(LINE_LIST, start, count);
        start += count;
    }

    if (points_.Size())
    {
#if defined(MAPEDITOR_OGL)
        glPointSize(pointsize);
#endif
        count = points_.Size();
        graphics->SetDepthTest(CMP_LESSEQUAL);
        graphics->Draw(POINT_LIST, start, count);
        start += count;
    }

    graphics->SetDepthWrite(false);
    graphics->SetLineAntiAlias(false);
    graphics->SetLineWidth(1.f);
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
    URHO3D_LOGINFOF("AnimatorEditor()");
    mainScene_ = GameContext::Get().rootScene_;
}

AnimatorEditor::~AnimatorEditor()
{
    URHO3D_LOGINFOF("~AnimatorEditor()");
    animatorEditor_ = 0;

    FinishEdit();
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
}

void AnimatorEditor::EditInPreview(AnimationSet2D* animationSet)
{
    URHO3D_LOGINFO("AnimatorEditor() - Edit : animationset2d = " + animationSet->GetName());

    // Edit an AnimationSet2D inside scmleditor

    MapEditorLibImpl::Get()->SetVisible(PANEL_ANIMATOR2D_MAIN, true);

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
}

void AnimatorEditor::FinishEdit()
{
    if (editNode_)
    {
        URHO3D_LOGINFOF("AnimatorEditor() - FinishEdit : node=%s(%u) ... ", editNode_->GetName().CString(), editNode_->GetID());

        editNode_->SetScale2D(nodeoriginscale_);
        editNode_.Reset();
        editedAnimatedSprite_.Reset();
    }
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
    const PODVector<Spriter::CharacterMap*>& appliedcharactermaps = editedAnimatedSprite_->GetAppliedCharacterMaps();
    ListView* appliedMaps = static_cast<ListView*>(panel->GetChild("AppliedMaps", true));
    appliedMaps->RemoveAllItems();
    appliedMaps->SetMultiselect(true);
    for (unsigned i=0; i < appliedcharactermaps.Size(); i++)
    {
        Text* item = new Text(context_);
//        item->SetDefaultStyle(MapEditorLibImpl::GetUIStyle(UISTYLE_EDITORDEFAULT));
        item->SetStyle("FileSelectorListText");
        item->SetText(appliedcharactermaps[i]->name_);
        appliedMaps->AddItem(item);
        URHO3D_LOGINFOF("AnimatorEditor() - SetCharacterMappingPanel : node=%s(%u) ... appliedcharactermaps additem[%u]=%s ", editNode_->GetName().CString(), editNode_->GetID(), i, appliedcharactermaps[i]->name_.CString());
    }

    SubscribeToEvent(panel, E_VISIBLECHANGED, URHO3D_HANDLER(AnimatorEditor, HandleVisible));

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
    SetSpriteSelectionList();
    SetAnimationPanel();
}

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

void AnimatorEditor::SetSpriteMappingPanel()
{
    URHO3D_LOGINFOF("AnimatorEditor() - SetSpriteMappingPanel");
    UIElement* panel = MapEditorLibImpl::GetPanel(PANEL_ANIMATOR2D_SWSPRITES);

    CheckBox* showswap = static_cast<CheckBox*>(panel->GetChild("ShowSwap", true));
    SubscribeToEvent(showswap, E_TOGGLED, URHO3D_HANDLER(AnimatorEditor, HandleSpriteMappingPanelShowSwap));

    UpdateSpriteMappingPanel();
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

void AnimatorEditor::SetSpriteSelectionList()
{
    URHO3D_LOGINFOF("AnimatorEditor() - SetSpriteSelectionList");
    UIElement* panel = MapEditorLibImpl::GetPanel(PANEL_ANIMATOR2D_SSELECTLIST);
    ListView* spritesList = static_cast<ListView*>(panel->GetChild("SpritesList", true));
    spritesList->RemoveAllItems();

    const HashMap<unsigned, SharedPtr<Sprite2D> >& spritemapping = editedAnimatedSprite_->GetAnimationSet()->GetSpriteMapping();
    const unsigned numSpritesByRow = 4;
    const unsigned numSprites = spritemapping.Size() + 2;
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
            row->AddChild(AddListButton(String::EMPTY, 0));
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
            row->AddChild(AddListButton(String::EMPTY, 0));
        spritesList->AddItem(row);
    }
    // add remove button
    {
        UIElement* button = spritesList->GetItem(0)->GetChild(0);
        SubscribeToEvent(button, E_PRESSED, URHO3D_HANDLER(AnimatorEditor, HandleSpriteMappingPanelModifySprite));
        button->SetVar(MAPPINGSPRITE_KEY, SPRITEKEY_REMOVE);

        BorderImage* spritebtn = static_cast<Button*>(button->GetChild(0));
        Text* textbtn = static_cast<Text*>(button->GetChild(1));
        textbtn->SetFontSize(12);
        textbtn->SetAlignment(HA_CENTER, VA_CENTER);
        textbtn->SetColor(Color::RED);
        textbtn->SetText("Remove");
    }
    // add ignore button
    {
        UIElement* button = spritesList->GetItem(0)->GetChild(1);
        SubscribeToEvent(button, E_PRESSED, URHO3D_HANDLER(AnimatorEditor, HandleSpriteMappingPanelModifySprite));
        button->SetVar(MAPPINGSPRITE_KEY, SPRITEKEY_IGNORE);

        BorderImage* spritebtn = static_cast<Button*>(button->GetChild(0));
        Text* textbtn = static_cast<Text*>(button->GetChild(1));
        textbtn->SetFontSize(12);
        textbtn->SetAlignment(HA_CENTER, VA_CENTER);
        textbtn->SetColor(Color::GRAY);
        textbtn->SetText("none");
    }
    // set Sprites
    unsigned i = 2;
    for (HashMap<unsigned, SharedPtr<Sprite2D> >::ConstIterator it = spritemapping.Begin(); it != spritemapping.End(); ++it, i++)
    {
        Sprite2D* sprite = it->second_.Get();
        int row = i / numSpritesByRow;
        int isprite = i - row * numSpritesByRow;

        UIElement* button = spritesList->GetItem(row)->GetChild(isprite);
        SubscribeToEvent(button, E_PRESSED, URHO3D_HANDLER(AnimatorEditor, HandleSpriteMappingPanelModifySprite));

        button->SetVar(MAPPINGSPRITE_KEY, it->first_);

        BorderImage* spritebtn = static_cast<Button*>(button->GetChild(0));
        spritebtn->SetTexture(sprite->GetTexture());
        spritebtn->SetImageRect(sprite->GetRectangle());
        Text* textbtn = static_cast<Text*>(button->GetChild(1));
        textbtn->SetText(sprite->GetName());
    }
}

void AnimatorEditor::SetAnimationPanel()
{
    AnimationSet2D* animationSet = editedAnimatedSprite_->GetAnimationSet();

    MapEditorLibImpl::Get()->SetVisible(PANEL_ANIMATOR2D_ANIMS, true);
    UIElement* panel = MapEditorLibImpl::GetPanel(PANEL_ANIMATOR2D_ANIMS);

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

    UIElement* toggleplaybutton = panel->GetChild("TogglePlay", true);
    Text* text = static_cast<Text*>(toggleplaybutton->GetChild(0));
    text->SetText("Play");

    SubscribeToEvent(availableAnimations, E_ITEMSELECTED, URHO3D_HANDLER(AnimatorEditor, HandleAnimationSelect));
    SubscribeToEvent(toggleplaybutton, E_PRESSED, URHO3D_HANDLER(AnimatorEditor, HandleAnimationTooglePlay));
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
}

void AnimatorEditor::HandleAnimationTooglePlay(StringHash eventType, VariantMap& eventData)
{
    Text* text = static_cast<Text*>(static_cast<Button*>(eventData[Pressed::P_ELEMENT].GetPtr())->GetChild(0));
    if (text->GetText() == "Play")
    {
//        URHO3D_LOGINFOF("AnimatorEditor() - HandleAnimationTooglePlay : Play");
        text->SetText("Stop");
        SubscribeToEvent(E_POSTUPDATE, URHO3D_HANDLER(AnimatorEditor, HandleAnimationPlay));
    }
    else
    {
//        URHO3D_LOGINFOF("AnimatorEditor() - HandleAnimationTooglePlay : Pause");
        text->SetText("Play");
        UnsubscribeFromEvent(E_POSTUPDATE);
    }
}

void AnimatorEditor::HandleAnimationPlay(StringHash eventType, VariantMap& eventData)
{
    if (!editedAnimatedSprite_)
    {
        UnsubscribeFromEvent(E_POSTUPDATE);
        return;
    }

    editedAnimatedSprite_->UpdateAnimation(eventData[PostUpdate::P_TIMESTEP].GetFloat());
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

void AnimatorEditor::UpdateSpriteMappingPanel()
{
    UIElement* panel = MapEditorLibImpl::GetPanel(PANEL_ANIMATOR2D_SWSPRITES);

    URHO3D_LOGINFOF("AnimatorEditor() - UpdateSpriteMappingPanel");

    ListView* mappedSwappedSprites = static_cast<ListView*>(panel->GetChild("MappedSwappedSprites", true));

    mappedSwappedSprites->RemoveAllItems();
    mappedSwappedSprites->SetDefaultStyle(MapEditorLibImpl::GetUIStyle(UISTYLE_EDITORDEFAULT));

    bool showswap = static_cast<CheckBox*>(panel->GetChild("ShowSwap", true))->IsChecked();
    UIElement* swapColumn = static_cast<ListView*>(panel->GetChild("SwapColumn", true));
    swapColumn->SetVisible(showswap);

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
        int targetfolder, targetfile;

        Text* item = new Text(context_);
        item->SetDefaultStyle(MapEditorLibImpl::GetUIStyle(UISTYLE_EDITORDEFAULT));
        item->SetStyle("FileSelectorListText");
        item->SetMinSize(0, 32);
        item->SetMaxSize(2147483647, 32);
        item->SetLayoutMode(LM_HORIZONTAL);
        item->LoadChildXML(MapEditorLibImpl::GetXmlElement(EDITORELT_ANIMATORMAPSWAPSPRITE), MapEditorLibImpl::GetUIStyle(UISTYLE_EDITORDEFAULT));

        item->SetVar(MAPPINGSPRITE_KEY, spritekey);
        item->SetVar(UIELEMENT_CHILDINDEX, slotid);

        UIElement* originSpriteElt = item->GetChild("OriginSprite", true);
        UIElement* mappedSpriteElt = item->GetChild("MappedSprite", true);
        mappedSpriteElt->SetVar(UIELEMENT_CHILDINDEX, slotid);

        Sprite2D* originSprite = animationSet->GetSpriterFileSprite(spritekey);
        //Sprite2D* mappedSprite = it->second_;
        Sprite2D* mappedSprite = spritemap->GetTargetKey(spritekey, targetfolder, targetfile) ? animationSet->GetSpriterFileSprite(targetfolder, targetfile) : 0;

        static_cast<Text*>(originSpriteElt->GetChild(0))->SetText(originSprite->GetName());
        static_cast<BorderImage*>(mappedSpriteElt->GetChild(0))->SetSprite(mappedSprite);
        UIElement* mappedbutton = mappedSpriteElt->GetChild(1);
        static_cast<Text*>(mappedbutton->GetChild(0))->SetText(mappedSprite ? mappedSprite->GetName() : (targetfolder == -1 ? "removed" : "none"));
        SubscribeToEvent(mappedbutton, E_PRESSED, URHO3D_HANDLER(AnimatorEditor, HandleSpriteMappingPanelSelectMappedSprite));

        UIElement* swappedSpriteElt = item->GetChild("SwappedSprite", true);
        swappedSpriteElt->SetVisible(showswap);
        if (showswap)
        {
            Sprite2D* swappedSprite = editedAnimatedSprite_->GetSwappedSprite(mappedSprite);
            static_cast<BorderImage*>(swappedSpriteElt->GetChild(0))->SetSprite(swappedSprite);
            UIElement* swappedbutton = swappedSpriteElt->GetChild(1);
            static_cast<Text*>(swappedbutton->GetChild(0))->SetText(swappedSprite ? swappedSprite->GetName(): "none");
            SubscribeToEvent(swappedbutton, E_PRESSED, URHO3D_HANDLER(AnimatorEditor, HandleSpriteMappingPanelSelectSwappedSprite));
        }

        mappedSwappedSprites->AddItem(item);
        slotid++;
    }

    editedSlotIndex_ = -1;
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

void AnimatorEditor::UpdateMapping(bool updatesprites, bool updatecolors)
{
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

void AnimatorEditor::HandleResize(StringHash eventType, VariantMap& eventData)
{
    URHO3D_LOGINFOF("AnimatorEditor() - HandleResize");

    panel_->BringToFront();
    if (preview_)
        preview_->QueueUpdate();
}

void AnimatorEditor::HandleAnimationSetSave(StringHash eventType, VariantMap& eventData)
{
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

    if (eventType == E_DOUBLECLICK)
    {
        UIElement* elt = static_cast<UIElement*>(context_->GetEventSender());
        URHO3D_LOGINFOF("AnimatorEditor() - HandleEditCMap : E_DOUBLECLICK on elt=(%u)%s parent=%s", elt, elt->GetName().CString(), elt->GetParent()->GetName().CString());
    }

    UIElement* panel = MapEditorLibImpl::GetPanel(PANEL_ANIMATOR2D_CMAPS);
    ListView* availableMaps = static_cast<ListView*>(panel->GetChild("AvailableMaps", true));

    Text* selecteditem = static_cast<Text*>(availableMaps->GetSelectedItem());
    if (!selecteditem)
        return;

    if (selecteditem->HasTag("Color"))
    {
        // Show the color Mapping panel
        MapEditorLibImpl::Get()->SetVisible(PANEL_ANIMATOR2D_COLORMAPPING, true);

        // Set the colormap name in the color mapping panel
        static_cast<LineEdit*>(MapEditorLibImpl::GetPanel(PANEL_ANIMATOR2D_COLORMAPPING)->GetChild("CMapName", true))->SetText(selecteditem->GetText());

        // Set the color mapping of the animatedSprite
        AnimatedSprite2D* animatedSprite = editNode_->GetComponent<AnimatedSprite2D>();

        UpdateMapping(true, false);
//        ApplyMap(selecteditem->GetText())->AddTag("Color");
        animatedSprite->ApplyColorMap(selecteditem->GetText());
        UpdateColorSpritePanel();
    }
    else
    {
        // Show the Sprites Mapping panel
        MapEditorLibImpl::Get()->SetVisible(PANEL_ANIMATOR2D_SWSPRITES, true);

        // Set the colormap name in the color mapping panel
        static_cast<LineEdit*>(MapEditorLibImpl::GetPanel(PANEL_ANIMATOR2D_SWSPRITES)->GetChild("CMapName", true))->SetText(selecteditem->GetText());

        UpdateMapping(true, true);

        UpdateSpriteMappingPanel();
    }
}

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

UIElement* AnimatorEditor::ApplyMap(const String& mapname)
{
    ListView* appliedMaps = static_cast<ListView*>(MapEditorLibImpl::GetPanel(PANEL_ANIMATOR2D_CMAPS)->GetChild("AppliedMaps", true));

    Text* item = new Text(context_);
    item->SetDefaultStyle(MapEditorLibImpl::GetUIStyle(UISTYLE_EDITORDEFAULT));
    item->SetStyle("FileSelectorListText");
    item->SetText(mapname);
    appliedMaps->AddItem(item);

    return item;
}


void AnimatorEditor::HandleApplyCMap(StringHash eventType, VariantMap& eventData)
{
    URHO3D_LOGINFOF("AnimatorEditor() - HandleApplyCMap");

    UIElement* panel = MapEditorLibImpl::GetPanel(PANEL_ANIMATOR2D_CMAPS);
    ListView* availableMaps = static_cast<ListView*>(panel->GetChild("AvailableMaps", true));

    PODVector<UIElement*> selectedItems = availableMaps->GetSelectedItems();
    for (unsigned i=0; i < selectedItems.Size(); i++)
    {
        UIElement* item = ApplyMap(static_cast<Text*>(selectedItems[i])->GetText());
        if (selectedItems[i]->HasTag("Color"))
            item->AddTag("Color");
    }

    UpdateMapping(true, true);
    UpdateColorSpritePanel();
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


void AnimatorEditor::HandleVisible(StringHash eventType, VariantMap& eventData)
{
    URHO3D_LOGINFOF("AnimatorEditor() - HandleVisible : visible=%s", panel_->IsVisible()?"true":"false");

    if (!panel_->IsVisible())
        FinishEdit();

    MapEditorLibImpl::Get()->SetVisible(PANEL_ANIMATOR2D_SWSPRITES, false);
    MapEditorLibImpl::Get()->SetVisible(PANEL_ANIMATOR2D_COLORMAPPING, false);

    if (previewScene_)
    {
        previewScene_->SetUpdateEnabled(panel_->IsVisible());
    }
}

void AnimatorEditor::HandleSpriteMappingPanelShowSwap(StringHash eventType, VariantMap& eventData)
{
    bool enable = eventData[Toggled::P_STATE].GetBool();
    URHO3D_LOGINFOF("AnimatorEditor() - HandleSpriteMappingPanelShowSwap : enable=%s", enable?"true":"false");

    UpdateSpriteMappingPanel();
}

void AnimatorEditor::HandleSpriteMappingPanelModifySprite(StringHash eventType, VariantMap& eventData)
{
//    URHO3D_LOGINFOF("AnimatorEditor() - HandleSpriteMappingPanelModifySprite slotindex=%d", editedSlotIndex_);

    UIElement* panel = MapEditorLibImpl::GetPanel(PANEL_ANIMATOR2D_SWSPRITES);
    Spriter::CharacterMap* spritemap = editedAnimatedSprite_->GetCharacterMap(StringHash(static_cast<LineEdit*>(panel->GetChild("CMapName", true))->GetText()));
    ListView* mappedSwappedSprites = static_cast<ListView*>(panel->GetChild("MappedSwappedSprites", true));

    const unsigned spritekey = mappedSwappedSprites->GetItem(editedSlotIndex_)->GetVar(MAPPINGSPRITE_KEY).GetUInt();
    const unsigned targetkey = static_cast<UIElement*>(eventData[Pressed::P_ELEMENT].GetPtr())->GetVar(MAPPINGSPRITE_KEY).GetUInt();

    Spriter::MapInstruction* instruction = spritemap->GetInstruction(spritekey);

    // add new mapinstruction
    if (!instruction && targetkey != SPRITEKEY_IGNORE)
    {
        instruction = new Spriter::MapInstruction();
        spritemap->maps_.Push(instruction);

        instruction->SetOrigin(spritekey);
    }

    if (instruction)
    {
        Sprite2D* mappedSprite = 0;
        if (targetkey == SPRITEKEY_IGNORE)
        {
            spritemap->maps_.Remove(instruction);
        }
        else if (targetkey == SPRITEKEY_REMOVE)
        {
            instruction->RemoveTarget();
        }
        else
        {
            instruction->SetTarget(targetkey);
            mappedSprite = editedAnimatedSprite_->GetAnimationSet()->GetSpriterFileSprite(targetkey);
        }

        UIElement* mappedSpriteElt = mappedSwappedSprites->GetItem(editedSlotIndex_)->GetChild("MappedSprite", true);
        static_cast<BorderImage*>(mappedSpriteElt->GetChild(0))->SetSprite(mappedSprite);
        UIElement* mappedbutton = mappedSpriteElt->GetChild(1);
        static_cast<Text*>(mappedbutton->GetChild(0))->SetText(mappedSprite ? mappedSprite->GetName() : (targetkey == SPRITEKEY_REMOVE ? "removed" : "none"));

        UpdateMapping(true, true);
    }
}

void AnimatorEditor::HandleSpriteMappingPanelSelectMappedSprite(StringHash eventType, VariantMap& eventData)
{
    MapEditorLibImpl::Get()->SetVisible(PANEL_ANIMATOR2D_SSELECTLIST, true);

    Button* mappedbutton = static_cast<Button*>(eventData[Pressed::P_ELEMENT].GetPtr());
    editedSlotIndex_ = mappedbutton->GetParent()->GetVar(UIELEMENT_CHILDINDEX).GetInt();
//    URHO3D_LOGINFOF("AnimatorEditor() - HandleSpriteMappingPanelSelectMappedSprite slotindex=%d (parent=%s)", editedSlotIndex_, mappedbutton->GetParent()->GetName().CString());
}

void AnimatorEditor::HandleSpriteMappingPanelSelectSwappedSprite(StringHash eventType, VariantMap& eventData)
{
    URHO3D_LOGINFOF("AnimatorEditor() - HandleSpriteMappingPanelSelectSwappedSprite slotindex=%d", editedSlotIndex_);

}




//void AnimatorEditor::HandleSpriteMappingPanelHoveringSprite(StringHash eventType, VariantMap& eventData)
//{
//
//}

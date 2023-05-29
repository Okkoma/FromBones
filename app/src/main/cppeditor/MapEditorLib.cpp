#include <Urho3D/Urho3D.h>

#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Graphics/GraphicsEvents.h>

#include <Urho3D/Graphics/ShaderVariation.h>
#include <Urho3D/Graphics/VertexBuffer.h>
#include <Urho3D/Graphics/Texture2D.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/Graphics/Viewport.h>
#include <Urho3D/Graphics/Camera.h>

#include <Urho3D/Input/Input.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/IO/FileSystem.h>

#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Resource/XMLFile.h>

#include <Urho3D/Scene/Scene.h>

#include <Urho3D/Urho2D/CollisionShape2D.h>
#include <Urho3D/Urho2D/CollisionChain2D.h>

#include <Urho3D/UI/BorderImage.h>
#include <Urho3D/UI/Button.h>
#include <Urho3D/UI/CheckBox.h>
#include <Urho3D/UI/Font.h>
#include <Urho3D/UI/Text3D.h>
#include <Urho3D/UI/ToolTip.h>
#include <Urho3D/UI/UI.h>
#include <Urho3D/UI/UIElement.h>
#include <Urho3D/UI/UIEvents.h>

#include "DefsCore.h"
#include "DefsEntityInfo.h"
#include "DefsColliders.h"

#include "GameOptions.h"
#include "GameContext.h"
#include "GameHelpers.h"

#include "MapWorld.h"

#include "RenderShape.h"

#define linewidth 5.f
#define pointsize 10.f

#if defined(DESKTOP_GRAPHICS) && defined(URHO3D_OPENGL)
#include <GLEW/glew.h>
#define MAPEDITOR_OGL
#endif

#include "MapEditorLib.h"


class FROMBONES_API MapEditorLibImpl : public MapEditorLib
{
public :
    MapEditorLibImpl(Context* context);
    virtual ~MapEditorLibImpl();

    virtual void Update(float timestep);
    virtual void Clean();
    virtual void Render();

private:
    void CreateUI();
    void RemoveUI();
    void ResizeUI(StringHash eventType, VariantMap& eventData);
    void CreateToolBar();
    void ToolBarSpawnActor(StringHash eventType, VariantMap& eventData);
    void ToolBarSpawnMonster(StringHash eventType, VariantMap& eventData);
    void ToolBarSpawnPlant(StringHash eventType, VariantMap& eventData);
    void UpdateDirtyToolBar();

    void SpawnObject();

    void SelectShape(CollisionShape2D* shape);
    void AddShapeToRender(CollisionShape2D* shape, unsigned linecolor, unsigned pointcolor);
    void SelectShape(RenderShape* shape, RenderCollider* collider);
    void AddShapeToRender(RenderShape* shape, unsigned linecolor, unsigned pointcolor, bool addBoundingRect=false);
    void UpdateSelectShape();

    void AddVerticesToRender(const PODVector<Vector2>& vertices, const Matrix2x3& transform, unsigned linecolor, unsigned pointcolor, bool addpoints=true, bool addpointtext=true, const String& tag=String::EMPTY);
    void AddDebugText(const String& text, const Vector2& position);

    CollisionShape2D* selectedShape_;
    RenderShape* selectedRenderShape_;
    RenderCollider* selectedRenderCollider_;
    int selectedPolyShapeId_;
    int selectedRenderContourId_;
    unsigned selectRenderShapeColor_;
    bool selectRenderShapeClipped_;

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

    Scene* scene_;
    WeakPtr<Node> debugTextRootNode_;
    Rect lastClippedRect_;

    float cameraBaseSpeed;
    float cameraBaseRotationSpeed;
    float cameraShiftSpeedMultiplier;
    bool cameraViewMoving;

    float uiMaxOpacity_;
    UIElement* editorUI_;
    UIElement* toolBar_;
    bool toolBarDirty_;
    bool subscribedToEditorToolBar_;

    int spawnCategory_;
};

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

XMLFile* gameStyle_ = 0;
XMLFile* uiStyle_ = 0;
XMLFile* iconStyle_ = 0;

enum
{
    SPAWN_NONE = 0,
    SPAWN_ACTOR,
    SPAWN_MONSTER,
    SPAWN_PLANT,
    SPAWN_TILE
};

MapEditorLibImpl::MapEditorLibImpl(Context* context) :
    MapEditorLib(context),
    selectedShape_(0),
    selectedRenderShape_(0),
    selectedRenderCollider_(0),
    selectedPolyShapeId_(0),
    selectedRenderContourId_(0),
    selectRenderShapeColor_(0),
    selectRenderShapeClipped_(false),
    cameraBaseSpeed(5.f),
    cameraBaseRotationSpeed(0.2f),
    cameraShiftSpeedMultiplier(5.f),
    cameraViewMoving(false),
    uiMaxOpacity_(0.9f),
    editorUI_(0),
    toolBar_(0),
    toolBarDirty_(true),
    subscribedToEditorToolBar_(false),
    spawnCategory_(SPAWN_NONE)
{
    vertexBuffer_ = new VertexBuffer(context);

    scene_ = GameContext::Get().rootScene_.Get();
    debugTextRootNode_ = scene_->GetChild("EditorDebugText", LOCAL);
    if (!debugTextRootNode_)
        debugTextRootNode_ = scene_->CreateChild("EditorDebugText", LOCAL);

    URHO3D_LOGINFOF("MapEditorLibImpl()");

    CreateUI();
}

MapEditorLibImpl::~MapEditorLibImpl()
{
    if (debugTextRootNode_)
        debugTextRootNode_->Remove();

    RemoveUI();

    URHO3D_LOGINFOF("~MapEditorLibImpl()");
}

XMLFile* GetEditorUIXMLFile(const String& fileName)
{
    String fullFileName = GameContext::Get().fs_->GetProgramDir() + "Data/" + fileName;
    if (GameContext::Get().fs_->FileExists(fullFileName))
    {
        File file(GameContext::Get().context_, fullFileName, FILE_READ);
        XMLFile* xml = new XMLFile(GameContext::Get().context_);
        xml->SetName(fileName);
        if (xml->Load(file))
            return xml;
    }

    return 0;
}

void MapEditorLibImpl::CreateUI()
{
    gameStyle_ = GameContext::Get().ui_->GetRoot()->GetDefaultStyle();
    uiStyle_ = GetEditorUIXMLFile("Editor/DefaultStyle.xml");
    iconStyle_ = GetEditorUIXMLFile("Editor/EditorIcons.xml");

    GameContext::Get().ui_->GetRoot()->SetDefaultStyle(uiStyle_);

    editorUI_ = GameContext::Get().ui_->GetRoot()->CreateChild<UIElement>("EditorUI");
    editorUI_->SetFixedSize(GameContext::Get().ui_->GetRoot()->GetWidth(), GameContext::Get().ui_->GetRoot()->GetHeight());

    CreateToolBar();

//    CreateAttributeInspectorWindow();
//    CreateHierarchyWindow();
//
//    HideAttributeInspectorWindow();
//    HideHierarchyWindow();

    SubscribeToEvent(E_SCREENMODE, URHO3D_HANDLER(MapEditorLibImpl, ResizeUI));
}

void MapEditorLibImpl::RemoveUI()
{
    editorUI_->RemoveChild(toolBar_);
//    editorUI_->RemoveChild(hierarchyWindow_);
//    editorUI_->RemoveChild(attributeInspectorWindow_);
    GameContext::Get().ui_->GetRoot()->RemoveChild(editorUI_);

    GameContext::Get().ui_->GetRoot()->SetDefaultStyle(gameStyle_);

    editorUI_ = 0;
    toolBar_ = 0;
//    hierarchyWindow_ = 0;
//    attributeInspectorWindow_ = 0;
}

void MapEditorLibImpl::ResizeUI(StringHash eventType, VariantMap& eventData)
{
    editorUI_->SetFixedSize(GameContext::Get().ui_->GetRoot()->GetWidth(), GameContext::Get().ui_->GetRoot()->GetHeight());

    // Resize tool bar
    toolBar_->SetFixedWidth(GameContext::Get().ui_->GetRoot()->GetWidth());
    toolBar_->SetPosition(0, GameContext::Get().ui_->GetRoot()->GetHeight() - 68);

//    ResizeAttributeInspectorWindow();
//    ResizeHierarchyWindow();
}

void CreateToolBarIcon(UIElement* element)
{
    BorderImage* icon = new BorderImage(GameContext::Get().context_);
    icon->SetName("Icon");
    icon->SetDefaultStyle(iconStyle_);
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
    button->SetDefaultStyle(uiStyle_);
    button->SetStyle("ToolBarButton");

    CreateToolBarIcon(button);
    CreateToolTip(button, title, IntVector2(button->GetWidth()/2, -20));

    return button;
}

CheckBox* CreateToolBarToggle(const String& title)
{
    CheckBox* toggle = new CheckBox(GameContext::Get().context_);
    toggle->SetName(title);
    toggle->SetDefaultStyle(uiStyle_);
    toggle->SetStyle("ToolBarToggle");

    CreateToolBarIcon(toggle);
    CreateToolTip(toggle, title, IntVector2(toggle->GetWidth()/2, -20));

    return toggle;
}

UIElement* CreateGroup(const String& title, LayoutMode layoutMode)
{
    UIElement* group = new UIElement(GameContext::Get().context_);
    group->SetName(title);
    group->SetDefaultStyle(uiStyle_);
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

void MapEditorLibImpl::CreateToolBar()
{
    toolBarDirty_ = true;
    subscribedToEditorToolBar_ = false;

    toolBar_ = new BorderImage(GameContext::Get().context_);
    toolBar_->SetName("ToolBar");
    toolBar_->SetStyle("EditorToolBar");
    toolBar_->SetLayout(LM_HORIZONTAL);
    toolBar_->SetLayoutSpacing(4);
    toolBar_->SetLayoutBorder(IntRect(8, 4, 4, 8));
    toolBar_->SetOpacity(uiMaxOpacity_);

    toolBar_->SetFixedSize(GameContext::Get().ui_->GetRoot()->GetWidth(), 68);
    toolBar_->SetPosition(0, 0);

    editorUI_->AddChild(toolBar_);

    UIElement* spawngroup = CreateGroup("SpawnGroup", LM_HORIZONTAL);
    spawngroup->AddChild(CreateToolBarToggle("Actor"));
    spawngroup->AddChild(CreateToolBarToggle("Monster"));
    spawngroup->AddChild(CreateToolBarToggle("Plant"));
    spawngroup->AddChild(CreateToolBarToggle("Tile"));
    spawngroup->AddChild(CreateToolBarToggle("Door"));
    spawngroup->AddChild(CreateToolBarToggle("Light"));
    spawngroup->AddChild(CreateToolBarToggle("Decoration"));
    spawngroup->AddChild(CreateToolBarToggle("Container"));
    spawngroup->AddChild(CreateToolBarToggle("Collectable"));
    spawngroup->AddChild(CreateToolBarToggle("Tool"));
    FinalizeGroupHorizontal(spawngroup, "ToolBarToggle");
    toolBar_->AddChild(spawngroup);
}

void MapEditorLibImpl::ToolBarSpawnActor(StringHash eventType, VariantMap& eventData)
{
    CheckBox* edit = (CheckBox*)eventData["Element"].GetPtr();
    if (edit->IsChecked())
        spawnCategory_ = SPAWN_ACTOR;
    else if (spawnCategory_ == SPAWN_ACTOR)
        spawnCategory_ = SPAWN_NONE;
    toolBarDirty_ = true;
}

void MapEditorLibImpl::ToolBarSpawnMonster(StringHash eventType, VariantMap& eventData)
{
    CheckBox* edit = (CheckBox*)eventData["Element"].GetPtr();
    if (edit->IsChecked())
        spawnCategory_ = SPAWN_MONSTER;
    else if (spawnCategory_ == SPAWN_MONSTER)
        spawnCategory_ = SPAWN_NONE;
    toolBarDirty_ = true;
}

void MapEditorLibImpl::ToolBarSpawnPlant(StringHash eventType, VariantMap& eventData)
{
    CheckBox* edit = (CheckBox*)eventData["Element"].GetPtr();
    if (edit->IsChecked())
        spawnCategory_ = SPAWN_PLANT;
    else if (spawnCategory_ == SPAWN_PLANT)
        spawnCategory_ = SPAWN_NONE;
    toolBarDirty_ = true;
}

void MapEditorLibImpl::UpdateDirtyToolBar()
{
    if (!toolBar_ || !toolBarDirty_)
        return;

    CheckBox* spawnActorToogle = (CheckBox*)toolBar_->GetChild("Actor", true);
    spawnActorToogle->SetChecked(spawnCategory_ == SPAWN_ACTOR);

    CheckBox* spawnMonsterToogle = (CheckBox*)toolBar_->GetChild("Monster", true);
    spawnMonsterToogle->SetChecked(spawnCategory_ == SPAWN_MONSTER);

    CheckBox* spawnPlantToogle = (CheckBox*)toolBar_->GetChild("Plant", true);
    spawnPlantToogle->SetChecked(spawnCategory_ == SPAWN_PLANT);

    if (!subscribedToEditorToolBar_)
    {
        SubscribeToEvent(spawnActorToogle, E_TOGGLED, URHO3D_HANDLER(MapEditorLibImpl, ToolBarSpawnActor));
        SubscribeToEvent(spawnMonsterToogle, E_TOGGLED, URHO3D_HANDLER(MapEditorLibImpl, ToolBarSpawnMonster));
        SubscribeToEvent(spawnPlantToogle, E_TOGGLED, URHO3D_HANDLER(MapEditorLibImpl, ToolBarSpawnPlant));

        subscribedToEditorToolBar_ = true;
    }

    toolBarDirty_ = false;
}

void MapEditorLibImpl::SpawnObject()
{
    bool spawned = false;

    if (spawnCategory_ == SPAWN_ACTOR)
    {
        spawned = World2D::GetWorld()->SpawnActor() !=0;
        if (spawned)
            URHO3D_LOGINFOF("MapEditorLibImpl() - SpawnObject : Spawn Actor !");
    }
    else if (spawnCategory_ == SPAWN_MONSTER)
    {
        spawned = World2D::GetWorld()->SpawnEntity(StringHash("GOT_Skeleton")) != 0;
        if (spawned)
            URHO3D_LOGINFOF("MapEditorLibImpl() - SpawnObject : Spawn Monster !");
    }
    else if (spawnCategory_ == SPAWN_PLANT)
    {
//        spawned = World2D::GetWorld()->SpawnFurniture(StringHash("FUR_Plant04")) != 0;
        spawned = World2D::GetWorld()->SpawnFurniture(StringHash("FUR_LTreeGrow")) != 0;
        if (spawned)
            URHO3D_LOGINFOF("MapEditorLibImpl() - SpawnObject : Spawn Plant !");

    }
    else if (spawnCategory_ == SPAWN_TILE)
    {
        if (spawned)
            URHO3D_LOGINFOF("MapEditorLibImpl() - SpawnObject : Spawn Tile !");
    }

//    if (spawned == true)
//        UpdateHierarchyItem(editorScene.GetChild("Entities", true));
}

void MapEditorLibImpl::Update(float timeStep)
{
    if (!World2D::GetWorld())
        return;

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

    if (input->GetMouseButtonPress(MOUSEB_LEFT))
    {
        if (spawnCategory_ == SPAWN_NONE)
        {
            if (input->GetQualifierDown(QUAL_SHIFT))
            {
                RenderCollider* collider = 0;
                RenderShape* shape = GameHelpers::GetMapRenderShapeAt(position, selectedPolyShapeId_, selectedRenderContourId_, collider);

                URHO3D_LOGINFOF("MapEditorLibImpl() - Update : Select a rendershape at %s ... rendershape=%u polyshapeid=%d contourid=%d",
                                 mouseposition.ToString().CString(), shape, selectedPolyShapeId_, selectedRenderContourId_);

                selectRenderShapeClipped_ = false;
                selectRenderShapeColor_ = Color::MAGENTA.ToUInt();
                SelectShape(shape, collider);
            }
            else if (input->GetQualifierDown(QUAL_CTRL))
            {
                RenderCollider* collider = 0;
                RenderShape* shape = GameHelpers::GetMapRenderShapeAt(position, selectedPolyShapeId_, selectedRenderContourId_, collider, true);

                URHO3D_LOGINFOF("MapEditorLibImpl() - Update : Select a clipped rendershape at %s ... rendershape=%u polyshapeid=%d contourid=%d",
                                mouseposition.ToString().CString(), shape, selectedPolyShapeId_, selectedRenderContourId_);

                selectRenderShapeClipped_ = true;
                selectRenderShapeColor_ = Color::CYAN.ToUInt();
                SelectShape(shape, collider);

                // reclip debug
                if (shape && selectRenderShapeClipped_)
                {
                    PolyShape& polyshape = shape->GetShapes()[selectedPolyShapeId_];
                    polyshape.Clip(polyshape.clippedRect_, true, shape->GetNode()->GetWorldTransform2D().Inverse(), true);
                }
            }
            else
            {
                URHO3D_LOGINFOF("MapEditorLibImpl() - Update : Select a shape at %s ...", mouseposition.ToString().CString());
                SelectShape(GameHelpers::GetMapCollisionShapeAt(position));
            }
        }
        else
        {
            SpawnObject();
        }
    }
    else if (input->GetMouseButtonPress(MOUSEB_RIGHT))
    {
        if (input->GetQualifierDown(QUAL_SHIFT))
            GameHelpers::AddTile(position);
        else if (input->GetQualifierDown(QUAL_CTRL))
            GameHelpers::RemoveTile(position, false, true, true);
    }

    UpdateDirtyToolBar();
}

void MapEditorLibImpl::Render()
{
    if (selectedRenderShape_)
    {
        PolyShape& polyshape = selectedRenderShape_->GetShapes()[selectedPolyShapeId_];
        if (polyshape.clippedRect_ != lastClippedRect_)
        {
            UpdateSelectShape();
            lastClippedRect_ = polyshape.clippedRect_;
        }
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


void MapEditorLibImpl::AddShapeToRender(CollisionShape2D* shape, unsigned linecolor, unsigned pointcolor)
{
    if (!shape->IsInstanceOf<CollisionChain2D>())
        return;

    CollisionChain2D* chain = (CollisionChain2D*)shape;

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

void MapEditorLibImpl::SelectShape(CollisionShape2D* shape)
{
    lines_.Clear();
    points_.Clear();
    selectedShape_ = shape;
    selectedRenderShape_ = 0;

    debugTextRootNode_->SetEnabledRecursive(false);

    if (selectedShape_)
    {
        URHO3D_LOGINFOF("MapEditor() : SelectShape : shape=%u", shape);
        AddShapeToRender(selectedShape_, Color::GRAY.ToUInt(), Color::YELLOW.ToUInt());
    }
}

void MapEditorLibImpl::SelectShape(RenderShape* rendershape, RenderCollider* collider)
{
    lines_.Clear();
    points_.Clear();
    selectedShape_ = 0;
    selectedRenderShape_ = rendershape;
    selectedRenderCollider_ = collider;
    debugTextRootNode_->SetEnabledRecursive(false);

    if (selectedRenderShape_)
    {
       // URHO3D_LOGINFOF("MapEditor() : SelectShape : rendershape=%u viewzind=%d viewid=%d colliderz=%d layerz=%d",
       //                 rendershape, collider->params_->indz_, collider->params_->indv_, collider->params_->colliderz_, collider->params_->layerz_);

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

void MapEditorLibImpl::Clean()
{
    // When the amount of debug geometry is reduced, release memory
    unsigned linesSize = lines_.Size();

    lines_.Clear();

    if (lines_.Capacity() > linesSize * 2)
        lines_.Reserve(linesSize);
}


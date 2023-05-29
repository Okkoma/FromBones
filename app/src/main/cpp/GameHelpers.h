#pragma once

#include <cstdarg>

#include <Urho3D/IO/Log.h>
#include <Urho3D/Graphics/Light.h>
#include <Urho3D/Urho2D/Drawable2D.h>
#include <Urho3D/UI/UI.h>

#include "GameOptions.h"

#include "DefsCore.h"
#include "DefsEntityInfo.h"
#include "DefsEffects.h"
#include "DefsColliders.h"
#include "DefsMove.h"
#include "DefsShapes.h"

namespace Urho3D
{
class Camera;
class UIElement;
class Window;
class Sprite2D;
class SpriteSheet2D;
class AnimatedSprite2D;
class RigidBody2D;
class CollisionShape2D;
class DebugRenderer;
class Viewport;
}

using namespace Urho3D;

class GOC_Destroyer;
class GOC_Collectable;
class Player;
class Map;
class TextMessage;

int FROMBONES_API GetEnumValue(const String& value, const char** enumPtr);
bool FROMBONES_API IsDigital(const String& value);

inline bool AreEquals(const Vector2& point0, const Vector2& point1, const float epsilon)
{
    if (Abs(point0.x_ - point1.x_) > epsilon)
        return false;

    if (Abs(point0.y_ - point1.y_) > epsilon)
        return false;

    return true;
}

inline float Cross(const Vector2& a, const Vector2& b)
{
    return a.x_ * b.y_ - a.y_ * b.x_;
}

// tells if vec c lies on the left side of directed edge a->b
// 1 if left, -1 if right, 0 if colinear

void GetDirection(const Vector2& a, const Vector2& b, const Vector2& c, int& dir);

bool Intersect(const Vector2& x0, const Vector2& x1, const Vector2& y0, const Vector2& y1, Vector2& res);


class Tile;
struct ObjectMapedInfo;

class FROMBONES_API GameHelpers
{

public:

    static unsigned ToUIntAddr(void* value)
    {
        return (unsigned)(size_t)value;
    }

    static void SetGameLogFilter(unsigned logfilter);
    static unsigned GetGameLogFilter();
    static void SetGameLogLevel(int level=0);
    static int GetGameLogLevel();
    static void SetGameLogEnable(unsigned filterbits, bool state);

    /// Serialize Data Helpers
    static void LoadData(Context* context, const char* fileName, void* data, bool allocate = false);
    static void SaveData(Context* context, void* data, unsigned size, const char* fileName);
    static char* GetTextFileToBuffer(const char* fileName, size_t& s);
    static void SaveOffsetSpriteSheet(SpriteSheet2D* spriteSheet, const IntVector2& offset, const String& outputfilename);

    /// Serialize Scene/Node Helpers
    static bool LoadSceneXML(Context* context, Node* node, CreateMode mode);
    static bool SaveSceneXML(Context* context, Scene* scene, const char* fileName);
    static bool LoadNodeXML(Context* context, Node* node, const String& fileName, CreateMode mode=LOCAL, bool applyAttr=true);
    static bool SaveNodeXML(Context* context, Node* node, const char* fileName);

    /// Node Attributes Helpers
    static void LoadNodeAttributes(Node* node, const NodeAttributes& nodeAttr, bool applyAttr=true);
    static void SaveNodeAttributes(Node* node, NodeAttributes& nodeAttr);
    static const Variant& GetNodeAttributeValue(const StringHash& attr, const NodeAttributes& nodeAttr, unsigned index=0);
    static void CopyAttributes(Node* source, Node* dest, bool apply=true, bool removeUnusedComponents=true);
    static Vector<unsigned> sRemovableComponentsOnCopyAttributes_;

    /// Scene/Node Remover Helpers
    static bool CleanScene(Scene* scene, const String& stateToClean, int step);
    static void RemoveNodeSafe(Node* node, bool dumpnodes=false);

    /// Node Helpers
    static void SpawnGOtoNode(Context* context, StringHash type, Node* node);
    static void SpawnCollectableFromSlot(Context* context, Slot& slot, Node* node, unsigned int qty = M_MAX_UNSIGNED);
    static Node* SpawnParticleEffectInNode(Context* context, Node* node, const String& effectName, int layer, int viewmask, const Vector2& position, float angle, float fscale=1.f, bool removeTimer=true, float duration=-1.f, const Color& color=Color::WHITE, CreateMode mode=REPLICATED);
    static void SpawnParticleEffect(Context* context, const String& effectName, int layer, int viewmask, const Vector2& position, float worldAngle=0.f, float worldScale=1.f, bool removeTimer=true, float duration=-1.f, const Color& color=Color::WHITE, CreateMode mode=REPLICATED, Material* material=0, const float zf=0.f);
    static void TransferPlayersToMap(const ShortIntVector2& mPoint);
    static bool SpawnScraps(const StringHash& type, unsigned numscraps, const Color& color, bool iscollider, int layer, int viewZ, int viewMask, const Vector2& position, const Vector2& scale, float maxsize, float lifetime, float impulse, float density);
    static void SetControllabledNode(Node* node, int controlID=0, int movetype=MOVE2D_WALK, int numjumps=1, int viewZ=-1);
    static void SetDrawableLayerView(Node* node, int viewz=0);
    static void UpdateLayering(Node* node, int viewz=0);

    static void GetInputPosition(int& x, int& y, UIElement** uielt = 0);
    static RigidBody2D* GetBodyAtCursor();

    static bool IsNodeImmuneToEffect(Node* node, const EffectType& effect);

    /// Physics Helpers
    static bool SetPhysicProperties(Node* node, const PhysicEntityInfo& physic, bool init=true, bool setvelocity=false);
    static bool SetPhysicProperties(Node* node, const void* physicData, bool init=true, bool setvelocity=false);
    static void SetPhysicVelocity(Node* node, float velx, float vely);
    static void SetPhysicFlipX(Node* node);
    static Vector2 GetWorldMassCenter(Node* node, CollisionShape2D* shape);
    static void GetDropPoint(Node* holder, float& x, float& y);

    /// Component Helpers
    static void ShakeNode(Node* node, int numshakes, float duration, const Vector2& amplitude);
    static void MoveCameraTo(const Vector2& position, int viewport, float duration);
    static void ZoomCameraTo(float zoom, int viewport, float duration);
    static float GetZoomToFitTo(int numtilesx, int numtilesy, int viewport);
    static void AddStaticSprite2D(const char *nodename, Context* context, Node* node, const char *texturename, const char *materialname, const Vector3& position, const Vector2& scale, CreateMode mode = REPLICATED);
    static void AddPlane(const char *nodename, Context* context, Node* node, const char *materialname,const Vector3& pos, const Vector2& scale, CreateMode mode = REPLICATED);
    static void AddLight(Context* context, Node *node, LightType type, const Vector3& relativePosition=Vector3::ZERO,const Vector3& direction=Vector3::FORWARD, const float& fov=60.0f, const float& range=10.0f,
                         float brightness=1.0f, bool pervertex=false, bool colorAnimated=false);
    static bool SetLightActivation(Node* node, int viewport=0);
    static bool SetLightActivation(Player* player);
    static Sprite2D* GetSpriteForType(const StringHash& type, const StringHash& buildtype, const String& partname);
    static Sprite2D* GetSpriteForType(const StringHash& type, const StringHash& buildtype = StringHash::ZERO, int partIndex=-1);
    static void SetCollectableProperties(GOC_Collectable* collectable, const StringHash& type, VariantMap* slotData=0);
    static void GetRandomizedEquipment(AnimatedSprite2D* animatedSprite, EquipmentList* equipmentlist);
    static void SetEquipmentList(AnimatedSprite2D* animatedSprite, EquipmentList* equipmentlist);
    static void SetEntityVariation(AnimatedSprite2D* animatedsprite, int& entityid, const GOTInfo* gotinfo=0, bool forceRandomEntity=false, bool forceRandomMapping=false);
    static void SetRenderedAnimation(AnimatedSprite2D* animatedSprite, const String& cmap, const StringHash& type);
    static String GetMoveStateString(unsigned movestate);

    /// Map / Tile Helpers
    static void GetWorldMapPosition(const IntVector2& screenposition, WorldMapPosition& position);
    static CollisionShape2D* GetMapCollisionShapeAt(const WorldMapPosition& position);
    static RenderShape* GetMapRenderShapeAt(const WorldMapPosition& position, int& shapeid, int& contourid, RenderCollider*& rendercollider, bool clipped=false);
    static bool AddTile(const WorldMapPosition& position, bool backgroundAdding=false);
    static int RemoveTile(const WorldMapPosition& position, bool addeffects, bool adddoor, bool permutesametiles=false, bool onsameview=false);
    static bool SetTile(MapBase* map, FeatureType feat, unsigned tileindex, int layerZ, bool permutesametiles=false, Tile** removedtile=0);
    static bool SetTiles(MapBase* map, FeatureType feat, const Vector<unsigned>& tileindexes, int layerZ, bool entitymode);
    static bool CheckForDoorPlacement(MapBase* map, int viewid, const WorldMapPosition& wpos, bool& dooratleft);
    static void DumpTile(const WorldMapPosition& position);
    static Node* CreateObjectMapedFrom(const ObjectMapedInfo& info);
    static const StringHash& GetRandomMonsters(Vector<StringHash>* authorizedCategories=0);
    static bool CheckFreeTilesAtViewZ(const GOC_Destroyer* destroyer, MapBase* map, unsigned tileindex, int viewZ);
    static void AdjustPositionInTile(GOC_Destroyer* destroyer, MapBase* map, const WorldMapPosition& initialposition);
    // Pixel Shape Generators
    static PixelShape* GetPixelShape(int geometry, int width, int height, FeatureType wallType=1, FeatureType innerType=0, int r=0);
    static PixelShape* GetPixelShape(int id);

    /// Audio Helpers
    static void SpawnSound(Node* node, const char* fileName, float gain = 1.f);
    static void SpawnSound3D(Node* node, const char* fileName, float gain = 1.f);
    static void CreateMusicNode(Scene* scene);
    static void PlayMusic(Scene* scene, const char* fileName, bool loop, float gain = 0.7f);
    static void StopMusic(Scene* scene);
    static void SetSoundVolume(float volume);
    static void SetMusicVolume(float volume);

    /// Math / Converter Helpers
    static void ClampSizeTo(IntVector2& size, int dimension);
    static Vector2 ScreenToWorld2D(int screenx, int screeny);
    static Vector2 ScreenToWorld2D(const IntVector2& screenpoint);
    static void OrthoWorldToScreen(IntVector2& point, Node* node, int viewport);
    static void OrthoWorldToScreen(IntRect& rect, const BoundingBox& box, int viewport=0);
    static void SpriterCorrectorScaleFactor(Context* context, const char *filename, const float& scale_factor);
    static void EqualizeValues(PODVector<float>& values, int coeff=1, bool incbleft=true, bool incbright=true);
    template< typename T > static bool FloodFill(Matrix2D<T>& buffer, Stack<unsigned>& stack, const T& patternToFill, const T& fillPattern, const int& xo, const int& yo, const IntRect& bordercheck=IntRect::ZERO);
    template< typename T > static bool FloodFill(Matrix2D<T>& buffer, Stack<unsigned>& stack, const T& patternToFillFrom, const T& patternToFillTo, const T& fillPattern, const int& xo, const int& yo);
    template< typename T > static bool FloodFillInBorder(const Matrix2D<T>& bufferin, Matrix2D<T>& bufferout, Stack<unsigned>& stack, const T& borderPattern, const T& fillPattern, const int& xo, const int& yo, const IntRect& bordercheck);
    template< typename T > static IntVector2 BorderContains(Matrix2D<T>& buffer, const T& value, IntVector2& point, const IntRect& borderToCheck = IntRect::ZERO, int expand = 0);

    /// Graphics Helpers
    static bool IsInsidePolygon(const Vector2& point, const PODVector<Vector2>& polygon);
    static void ScaleManhattanContour(float value, PODVector<Vector2>& contour);
    static Vector2 GetManhattanVector(unsigned index, const PODVector<Vector2>& vertices);
    static void OffsetEqualVertices(PODVector<Vector2>& contour, const float epsilon, bool shrink=false);
    static void OffsetEqualVertices(const PODVector<Vector2>& contour1, PODVector<Vector2>& contour2, const float epsilon, bool shrink=false);
    static void GetCircleShape(const Vector2& center, float radius, int numpoints, PODVector<Vector2>& shape);
    static void GetEllipseShape(const Vector2& center, const Vector2& radius, int numpoints, PODVector<Vector2>& shape);
    static void TriangulateShape(const PODVector<Vector2>& contour, const Vector<PODVector<Vector2> >& holes, PODVector<Vector2>& vertices, PODVector<unsigned char>& memory);
    static void ClipShape(PolyShape& shape, const Rect& cliprect, const Matrix2x3& transform=Matrix2x3::IDENTITY);

    static void DrawDebugVertices(DebugRenderer* debug, const PODVector<Vector2>& vertices, const Color& color = Color::WHITE, const Matrix3x4& worldTransform = Matrix3x4::IDENTITY);
    static void DrawDebugShape(DebugRenderer* debug, const PODVector<Vector2>& shape, const Color& color = Color::WHITE, const Matrix3x4& worldTransform = Matrix3x4::IDENTITY);
    static void DrawDebugMesh(DebugRenderer* debug, const PODVector<Vector2>& vertices, const Color& color = Color::WHITE, const Matrix3x4& worldTransform = Matrix3x4::IDENTITY);
    static void DrawDebugMesh(DebugRenderer* debug, const Vector<Vertex2D>& vertices, const Color& color, bool quad=false);

    /// UI Helpers
    static void ApplySizeRatio(int w, int h, IntVector2& size);
    static UIElement* AddUIElement(const String& layout, const IntVector2& position, HorizontalAlignment hAlign, VerticalAlignment vAlign, float alpha=0.f, UIElement* root=0);
    template< typename T > static void SetUIElementFrom(T* uielement, int uitextureid, const String& spritesheetname, int clampsize=0);
    template< typename T > static void SetUIElementFrom(T* uielement, int uitextureid, const ResourceRef& ref, int clampsize=0);
    static void FindParentWithAttribute(const StringHash& attribute, UIElement* element, UIElement*& parent, Variant& var);
    static void FindParentWhoContainsName(const String& name, UIElement* element, UIElement*& parent);
    static void ClampPositionToScreen(IntVector2& position);
    static void ClampPositionUIElementToScreen(UIElement* element);
    static IntVector2 GetUIExitPoint(UIElement* element, const IntVector2& currentposition);
    static void SetMoveAnimationUI(UIElement* elt, const IntVector2& from, const IntVector2& to, float start, float delay);
    static void SetUIScale(UIElement* root, const IntVector2& defaultsize, float scale=0.f);
    static void SetScaleChildRecursive(UIElement* elt, float scale);
private:
    static void UpdateScrollBars(UIElement* elt);
public:
    static void ToggleModalWindow(Object* owner=0, UIElement* elt=0, bool force=false);
    static Window* GetModalWindow();
    static void SetEnableScissor(UIElement* elt, bool enable);
    static TextMessage* ShowUIMessage(const String& maintext, const String& endtext, bool localize, int fontsize, const IntVector2& position, float fadescale=1.f, float duration=2.f, float delaystart=0.f);
    static TextMessage* ShowUIMessage3D(const String& maintext, const String& endtext, bool localize, int fontsize, const IntVector2& position, float fadescale=1.f, float duration=2.f, float delaystart=0.f);
    static void ParseText(const String& text, Vector<String>& lines, int nummaxchar);

    /// Dump Helpers
    static void AppendBufferToString(String& string, const char* format, ...);
    static void DumpText(const char* buffer);
    static void DumpMap(int width, int height, const int* data);
    static void DumpObject(Object* ref);
    static void DumpNode(Node* node, int maxrecursivedeepth=1, bool withcomponent=true);
    static void DumpNodeRecursive(Node* node, unsigned currentDeepth, bool withcomponent=true, int deepth=0);
    static void DumpNode(unsigned id, bool withcomponentattr, bool rtt=false);
    static void DumpComponentTemplates();
    static void DumpVariantMap(const VariantMap& varmap);
    template< typename T > static void DumpData(const T* data, int markpattern, int n,  ...);
    static void DumpVertices(const PODVector<Vector2>& vertice);

    static void DrawDebugRect(const Rect& rect, DebugRenderer* debugRenderer, bool depthTest=false, const Color& color=Color::WHITE);
    static void DrawDebugUIRect(const IntRect& rect, const Color& color=Color::WHITE, bool filled=false);

    static WeakPtr<Window> backupModal_;
};

#ifdef ACTIVE_GAMELOGLEVEL
#define GAME_SETGAMELOGENABLE(filter, state) GameHelpers::SetGameLogEnable(filter, state)
#else
#define GAME_SETGAMELOGENABLE(filter, state) ((void)0)
#endif

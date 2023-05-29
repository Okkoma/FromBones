
enum MouseOrbitMode
{
    ORBIT_RELATIVE = 0,
    ORBIT_WRAP
}

enum NewNodeMode
{
    NEW_NODE_CAMERA_DIST = 0,
    NEW_NODE_IN_CENTER,
    NEW_NODE_RAYCAST
}

enum HotKeysMode
{
    HOTKEYS_MODE_STANDARD = 0,
    HOTKEYS_MODE_BLENDER
}

enum EditMode
{
    EDIT_MOVE = 0,
    EDIT_ROTATE,
    EDIT_SCALE,
    EDIT_SELECT,
    EDIT_SPAWN
}

enum SpawnCategory
{
    SPAWN_NONE = -1,
    SPAWN_ACTOR = 0,
    SPAWN_MONSTER,
    SPAWN_PLANT,
    SPAWN_TILE
}

enum AxisMode
{
    AXIS_WORLD = 0,
    AXIS_LOCAL
}

enum SnapScaleMode
{
    SNAP_SCALE_FULL = 0,
    SNAP_SCALE_HALF,
    SNAP_SCALE_QUARTER
}

Array<int> pickModeDrawableFlags = {
    DRAWABLE_GEOMETRY,
    DRAWABLE_LIGHT,
    DRAWABLE_ZONE
};

Array<String> editModeText = {
    "Move",
    "Rotate",
    "Scale",
    "Select",
    "Spawn"
};

Array<String> axisModeText = {
    "World",
    "Local"
};

Array<String> pickModeText = {
    "Geometries",
    "Lights",
    "Zones",
    "Rigidbodies",
    "UI-elements"
};

Array<String> fillModeText = {
    "Solid",
    "Wire",
    "Point"
};

const int PICK_GEOMETRIES = 0;
const int PICK_LIGHTS = 1;
const int PICK_ZONES = 2;
const int PICK_RIGIDBODIES = 3;
const int PICK_UI_ELEMENTS = 4;
const int MAX_PICK_MODES = 5;
const int MAX_UNDOSTACK_SIZE = 256;

const StringHash FILENAME_VAR("FileName");
const StringHash MODIFIED_VAR("Modified");

Scene@ editorScene;

String instantiateFileName;
CreateMode instantiateMode = REPLICATED;
bool sceneModified = false;
bool runUpdate = false;
bool movedNodes = false;

Array<Node@> selectedNodes;
Array<Component@> selectedComponents;
Node@ editNode;
Array<Node@> editNodes;
Array<Component@> editComponents;
uint numEditableComponentsPerNode = 1;

Array<XMLFile@> sceneCopyBuffer;

bool suppressSceneChanges = false;
bool inSelectionModify = false;
bool skipMruScene = false;

Array<EditActionGroup> undoStack;
uint undoStackPos = 0;

bool revertOnPause = false;
XMLFile@ revertData;

Vector3 lastOffsetForSmartDuplicate;


WeakHandle previewCamera;
Node@ cameraNode;
Camera@ camera;

Node@ gridNode;
CustomGeometry@ grid;

UIElement@ editorUI;
UIElement@ viewportUI; // holds the viewport ui, convienent for clearing and hiding

uint setViewportCursor = 0; // used to set cursor in post update
uint resizingBorder = 0; // current border that is dragging
uint viewportMode;
int  viewportBorderOffset = 2; // used to center borders over viewport seams,  should be half of width
int  viewportBorderWidth = 4; // width of a viewport resize border
IntRect viewportArea; // the area where the editor viewport is. if we ever want to have the viewport not take up the whole screen this abstracts that
IntRect viewportUIClipBorder = IntRect(27, 60, 0, 0); // used to clip viewport borders, the borders are ugly when going behind the transparent toolbars
RenderPath@ renderPath; // Renderpath to use on all views
String renderPathName;
bool mouseWheelCameraPosition = false;
bool contextMenuActionWaitFrame = false;
bool cameraViewMoving = false;
int hotKeyMode = 0; // used for checking that kind of style manipulation user are prefer (see HotKeysMode)
Vector3 lastSelectedNodesCenterPoint = Vector3(0,0,0); // for Blender mode to avoid own origin rotation when no nodes are selected. preserve last center for this
WeakHandle lastSelectedNode = null;
WeakHandle lastSelectedDrawable = null;
WeakHandle lastSelectedComponent = null;
bool viewCloser = false;
Component@ coloringComponent = null;
String coloringTypeName;
String coloringPropertyName;
Color coloringOldColor;
float coloringOldScalar;
bool debugRenderDisabled = false;

Text@ editorModeText;
Text@ renderStatsText;

SpawnCategory spawnCategory = SPAWN_NONE;
EditMode editMode = EDIT_MOVE;
AxisMode axisMode = AXIS_WORLD;
FillMode fillMode = FILL_SOLID;
SnapScaleMode snapScaleMode = SNAP_SCALE_FULL;

float viewNearClip = 0.1;
float viewFarClip = 1000.0;
float viewFov = 45.0;

float cameraBaseSpeed = 5;
float cameraBaseRotationSpeed = 0.2;
float cameraShiftSpeedMultiplier = 5;
float moveStep = 0.5;
float rotateStep = 5;
float scaleStep = 0.1;
float snapScale = 1.0;
bool limitRotation = false;
bool moveSnap = false;
bool rotateSnap = false;
bool scaleSnap = false;
bool renderingDebug = false;
bool physicsDebug = false;
bool octreeDebug = false;
int pickMode = PICK_GEOMETRIES;
bool orbiting = false;

bool toggledMouseLock = false;
int mouseOrbitMode = ORBIT_RELATIVE;

float newNodeDistance = 20;
int newNodeMode = NEW_NODE_CAMERA_DIST;

bool showGrid = true;
bool grid2DMode = false;
uint gridSize = 16;
uint gridSubdivisions = 3;
float gridScale = 8.0;
Color gridColor(0.1, 0.1, 0.1);
Color gridSubdivisionColor(0.05, 0.05, 0.05);
Color gridXColor(0.5, 0.1, 0.1);
Color gridYColor(0.1, 0.5, 0.1);
Color gridZColor(0.1, 0.1, 0.5);

String defaultTags;

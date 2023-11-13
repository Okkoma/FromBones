#include <iostream>
#include <cstdio>
#include <Urho3D/ThirdParty/PugiXml/pugixml.hpp>

#include <Urho3D/Urho3D.h>

#include <Urho3D/Core/CoreEvents.h>

#include <Urho3D/Engine/Console.h>
#include <Urho3D/Engine/DebugHud.h>
#include <Urho3D/Engine/Engine.h>
#include <Urho3D/Engine/EngineEvents.h>

#include <Urho3D/IO/Log.h>
#include <Urho3D/IO/FileSystem.h>

#include <Urho3D/Audio/Audio.h>

#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Graphics/GraphicsImpl.h>
#include <Urho3D/Graphics/GraphicsEvents.h>
#include <Urho3D/Graphics/Renderer.h>

#include <Urho3D/Resource/Localization.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Resource/XMLFile.h>

#include <Urho3D/Input/Input.h>
#include <Urho3D/Input/InputEvents.h>

#include <Urho3D/Scene/SceneEvents.h>

#include <Urho3D/Urho2D/AnimationSet2D.h>

#include "DefsGame.h"

#include "GameOptions.h"
#include "GameEvents.h"
#include "GameNetwork.h"
#include "GameContext.h"
#include "GameHelpers.h"
#include "GameCommands.h"
#include "GameUI.h"
#include "GameData.h"

#include "MapStorage.h"
#include "sOptions.h"

#include "SDL/SDL.h"
#ifdef __ANDROID__
#include <jni.h>
#endif

#include "Game.h"

#ifdef __ANDROID__
void CallAndroidActivityMethod(const char* method, const char* arg)
{
    // retrieve the JNI environment.
    JNIEnv* env = (JNIEnv*)SDL_AndroidGetJNIEnv();
    // retrieve the Java instance of the SDLActivity
    jobject activity = (jobject)SDL_AndroidGetActivity();
    // find the Java class of the activity. It should be SDLActivity or a subclass of it.
    jclass clazz(env->GetObjectClass(activity));
    // find the identifier of the method to call
    jmethodID method_id = env->GetMethodID(clazz, "finish", "()V");
    // effectively call the Java method
    env->CallVoidMethod(activity, method_id);
    // clean up the local references.
    env->DeleteLocalRef(activity);
    env->DeleteLocalRef(clazz);
}
#endif

URHO3D_DEFINE_APPLICATION_MAIN(Game);


static Input* input_ = 0;
static String engineConfigApplied_;


enum DeviceDPIEnum
{
    DPI_AUTO = 0,
    DPI_L,
    DPI_M,
    DPI_H,
    DPI_XH,
    DPI_XXH,
    DPI_XXXH,
};

float DeviceDPIValues_[] =
{
    0.f,
    120.f,
    160.f,
    240.f,
    320.f,
    480.f,
    640.f
};

const char* DeviceDPIs_[] =
{
    "auto",
    "ldpi",
    "mdpi",
    "hdpi",
    "xhdpi",
    "xxhdpi",
    "xxxhdpi"
};

const float DeviceDPIScales_[] =
{
    0.f,
    0.75f,
    1.f,
    1.5f,
    2.f,
    3.f,
    4.f
};


//#define PERFORMTESTS

#ifdef PERFORMTESTS
void PerformTest_EllipseW()
{
    // Some Profiling Tests for EllipseW
    EllipseW ellipse(Vector2::ZERO, Vector2(10000.f, 1500.f));

    const unsigned numTimedTests = 10000;
    const unsigned numGeneratedPoints = 96;
    const unsigned numYValues = 24;

    ellipse.GenerateShape(numGeneratedPoints);

    unsigned i;
    Vector2 position1, position2, angle1, angle2;
    float x1, y1, y2;

    long long time0, time1, time2;
    HiresTimer timer;

    Time::Sleep(10);
    timer.Reset();

    for (i=0; i < numTimedTests; i++)
        y1 = ellipse.GetY(i);

    time0 = timer.GetUSec(false);
    Time::Sleep(10);
    float xstep = 2.f * ellipse.radius_.x_ / numTimedTests;
    position1.x_ = ellipse.center_.x_ + ellipse.radius_.x_;

    timer.Reset();
    for (i=0; i < numTimedTests; i++)
    {
        position1.x_ -= i * xstep;
        ellipse.GetPositionOn(0.f, 1.f, position1, angle1);
    }

    time1 = timer.GetUSec(false);
    Time::Sleep(10);
    position1.x_ = ellipse.center_.x_ + ellipse.radius_.x_;

    timer.Reset();
    for (i=0; i < numTimedTests; i++)
    {
        position1.x_ -= i * xstep;
        ellipse.GetPositionOnShape(0.f, 1.f, position1, angle1);
    }

    time2 = timer.GetUSec(false);
    Time::Sleep(10);
    timer.Reset();

    URHO3D_LOGINFOF(" Ellipse Profiling %u Tests : GetY=%dusec GetPositionOn=%dusec GetPositionOnShape(%upoints)=%dusec", numTimedTests, time0, time1, numGeneratedPoints, time2);
    URHO3D_LOGINFOF(" Ellipse Profiling ... 1 Test  : GetY=%Fusec GetPositionOn=%Fusec GetPositionOnShape(%upoints)=%Fusec", (float)time0/numTimedTests, (float)time1/numTimedTests, numGeneratedPoints, (float)time2/numTimedTests);

    xstep = 2.f * ellipse.radius_.x_ / numYValues;
    for (i=0; i <= numYValues; i++)
    {
        position1.x_ = position2.x_ = ellipse.center_.x_ + ellipse.radius_.x_ - i * xstep;
        ellipse.GetPositionOnShape(0.f, 1.f, position1, angle1);
        ellipse.GetPositionOn(0.f, 1.f, position2, angle2);
        URHO3D_LOGINFOF("  upper ellipse y value : x=%F GetPositionOnShape=%F GetPositionOn=%F delta=%F cos=%F cos2=%F sinf=%F sinf2=%F", position1.x_, position1.y_, position2.y_, position1.y_-position2.y_, angle1.x_, angle2.x_, angle1.y_, angle2.y_);
    }
    for (i=0; i <= numYValues; i++)
    {
        position1.x_ = position2.x_ = ellipse.center_.x_ + ellipse.radius_.x_ - i * xstep;
        ellipse.GetPositionOnShape(0.f, -1.f, position1, angle1);
        ellipse.GetPositionOn(0.f, -1.f, position2, angle2);
        URHO3D_LOGINFOF("  lower ellipse y value : x=%F GetPositionOnShape=%F GetPositionOn=%F delta=%F cos=%F cos2=%F sinf=%F sinf2=%F", position1.x_, position1.y_, position2.y_, position1.y_-position2.y_, angle1.x_, angle2.x_, angle1.y_, angle2.y_);
    }

    URHO3D_LOGINFOF("PerformTest_EllipseW passed !");
}

void PerformTest_EllipseW_GetIntersections(const EllipseW& ellipse, const Rect& rect)
{
    URHO3D_LOGINFOF("PerformTest_EllipseW_GetIntersections Rect=%s EllipseAABB=%s ...", rect.ToString().CString(), ellipse.aabb_.ToString().CString());

    // Get the intersections
    Vector<Vector2> intersections;
    ellipse.GetIntersections(rect, intersections, true);

    for (int i = 0; i < intersections.Size(); ++i)
        URHO3D_LOGINFOF("  Intersection[%d] = %s ", i, intersections[i].ToString().CString());
}

void PerformTest_SaveAnimationSet(Context* context)
{
    ResourceCache* cache = context->GetSubsystem<ResourceCache>();

    String filenameIn = "Data/2D/actor-medium.scml";
    String filenameOut = "actor-medium-testsave.scml";

    URHO3D_LOGINFOF("PerformTest_SaveAnimationSet filenameIn=%s filenameOut=%s ...", filenameIn.CString(), filenameOut.CString());

    // Load a AnimationSet
    AnimationSet2D* animationSet = cache->GetResource<AnimationSet2D>(filenameIn, false);

    URHO3D_LOGINFOF("PerformTest_SaveAnimationSet filenameIn=%s ... Loaded", filenameIn.CString());

    // Modify the skin color
    animationSet->SetEntityObjectRefAttr("actor", "medsk", "bone_", Color(0.5f,0.5f,1.f,1.f));

    // And save it.
    bool ok = animationSet->Save(filenameOut);

    URHO3D_LOGINFOF("PerformTest_SaveAnimationSet filenameOut=%s ... Saved", filenameOut.CString());

    URHO3D_LOGINFOF("PerformTest_SaveAnimationSet passed !");
}

void PerformTests(Context* context)
{
    PerformTest_EllipseW();
    PerformTest_EllipseW_GetIntersections(EllipseW(Vector2(0, 0), Vector2(2, 1)), Rect(Vector2(-1, -1), Vector2(1, 1)));
    PerformTest_EllipseW_GetIntersections(EllipseW(Vector2(0, 0), Vector2(2, 1)), Rect(Vector2(0, 0), Vector2(2, 2)));
    PerformTest_EllipseW_GetIntersections(EllipseW(Vector2(-1, -1), Vector2(2, 1)), Rect(Vector2(-2, -2), Vector2(0, 0)));
    PerformTest_EllipseW_GetIntersections(EllipseW(Vector2(0, 0), Vector2(100, 20)), Rect(Vector2(-60, 10), Vector2(-40, 30)));

    PerformTest_SaveAnimationSet(context);
//    exit(-1);
}
#endif


Game* Game::game_ = 0;

Game::Game(Context* context) :
    Application(context)
{
    game_ = this;

    GameContext::RegisterObject(context);

    URHO3D_LOGINFO("Game() - ----------------------------------------");
    URHO3D_LOGINFO("Game() - Game                                   -");
    URHO3D_LOGINFO("Game() - ----------------------------------------");
}

Game::~Game()
{
    GameContext::Destroy();

    if (game_ == this)
        game_ = 0;

    URHO3D_LOGINFO("Game() - ----------------------------------------");
    URHO3D_LOGINFO("Game() - ~Game                                  -");
    URHO3D_LOGINFO("Game() - ----------------------------------------");
}

/*
void CheckGraphicBackEnds(String& str)
{
    str += "Testing video drivers...\n";

    SDL_Init( 0 );

    Vector<bool> drivers( SDL_GetNumVideoDrivers() );
    for( int i = 0; i < drivers.Size(); ++i )
    {
        drivers[ i ] = ( 0 == SDL_VideoInit( SDL_GetVideoDriver( i ) ) );
        SDL_VideoQuit();
    }

    str += "SDL_VIDEODRIVER available:";
    for( int i = 0; i < drivers.Size(); ++i )
    {
        str += " ";
        str += SDL_GetVideoDriver( i );
    }
    str += "\n";

    str += "SDL_VIDEODRIVER usable   :";
    for( int i = 0; i < drivers.Size(); ++i )
    {
        if( !drivers[ i ] ) continue;
        str += " ";
        str += SDL_GetVideoDriver( i );
    }
    str += "\n";

    if( SDL_Init( SDL_INIT_EVERYTHING ) < 0 )
        return;

    URHO3D_LOGERRORF("%s", str.CString());
}
*/

void Game::Setup()
{
    GameContext::Get().engine_ = engine_;

    GameConfig& config = GameContext::Get().gameConfig_;

    FileSystem* fs = GetSubsystem<FileSystem>();

    String engineConfigFilename = fs->GetProgramDir() + "engine_config.xml";

    if (!fs->FileExists(engineConfigFilename))
    {
        String platform = GetPlatform().ToLower();
        platform.Replace(" ", "");
        if (platform == "?")
            platform = "default";

        engineConfigFilename = fs->GetProgramDir() + "Data/EngineConfig/" + platform + ".xml";
    }

    engineConfigApplied_ = LoadGameConfig(Engine::GetParameter(engineParameters_, "GameConfig", engineConfigFilename).GetString(), &config);

    if (engineConfigApplied_.Empty())
    {
        engineParameters_["WindowTitle"]   = GameContext::GAMENAME;
        engineParameters_["WindowIcon"]    = "Textures/UI/icone.png";
        engineParameters_["ResourcePaths"] = "CoreData;Data";

        engineParameters_["FlushGPU"]        = true;
        engineParameters_["WorkerThreads"]   = true;
        /*
        engineParameters_["WindowWidth"]     = 1920;
        engineParameters_["WindowHeight"]    = 1080;
        */
        engineParameters_["Headless"]        = false;
        engineParameters_["Shadows"]         = false;

        engineParameters_["TripleBuffer"]    = false;
        engineParameters_["FullScreen"]      = true;
        engineParameters_["VSync"]           = true;
        config.frameLimiter_                 = false;
        //config.frameLimiter_                 = 51;

        engineParameters_["LogQuiet"] = true;
        engineParameters_["LogLevel"] = 1;      // 0:LOGDEBUG 1: LOGINFO+LOGWARNING+LOGERROR 2: LOGWARNING+LOGERROR  3: LOGERROR ONLY

        engineParameters_["MaterialQuality"]   = 0;
        engineParameters_["TextureQuality"]    = 2;
        engineParameters_["TextureFilterMode"] = 0;
        engineParameters_["MultiSample"]       = 0;

        engineParameters_["SoundBuffer"]        = 50;
        engineParameters_["SoundMixRate"]       = 44100;
        engineParameters_["SoundStereo"]        = false;
        engineParameters_["SoundInterpolation"] = false;

        if (config.splashviewed_)
            config.initState_ = "MainMenu";
        else
            config.splashviewed_ = true;

        config.networkMode_  = "local";
        config.language_     = 1;
        config.deviceDPI_    = DPI_M;
        config.uiDeviceDPI_  = DPI_M;

        config.soundEnabled_ = true;
        config.musicEnabled_ = false;

        if (GetPlatform() == "Android" || GetPlatform() == "iOS")
        {
            config.touchEnabled_ = true;
            config.screenJoystick_ = true;
        }
        else
        {
            config.touchEnabled_ = false;
            config.screenJoystick_ = false;
        }

        config.HUDEnabled_ = false;
        config.ctrlCameraEnabled_ = false;

        config.debugRenderEnabled_ = false;
        config.physics3DEnabled_ = false;
        config.physics2DEnabled_ = true;
        config.enlightScene_ = false;
        config.renderShapes_ = true;
        config.multiviews_ = false;
        config.tileSpanning_ = 0.1f;
        config.debugPhysics_ = false;
        config.debugLights_ = false;
        config.debugUI_ = false;
        config.debugRenderShape_ = false;
    }

    if (engineParameters_["LogName"].GetString().Empty() && !GetExecutableName().Empty())
        engineParameters_["LogName"] = GetFileName(GetExecutableName()) + ".log";

    if (config.frameLimiter_)
    {
        engineParameters_["FrameLimiter"] = true;
        engineParameters_["MaxFPS"] = config.frameLimiter_;
    }

    SDL_GetDisplayDPI(0, &GameContext::Get().ddpi_, &GameContext::Get().hdpi_, &GameContext::Get().vdpi_);

    // Add Graphics Device Path for the dpi
    String DeviceResolutionPath("Data/Graphics/");
    // Detect DPI
    int deviceDpi = config.deviceDPI_;
    if (deviceDpi == DPI_AUTO)
    {
        int dpiindex = DPI_L;
        for (dpiindex=DPI_L; dpiindex <= DPI_XXXH; dpiindex++)
        {
            if (GameContext::Get().ddpi_ <= DeviceDPIValues_[dpiindex])
                break;
        }

        deviceDpi = dpiindex;
    }
    String pathName = fs->GetProgramDir() + DeviceResolutionPath + String(DeviceDPIs_[deviceDpi]);
    while (!fs->DirExists(pathName))
    {
        deviceDpi--;
        pathName = fs->GetProgramDir() + DeviceResolutionPath + String(DeviceDPIs_[deviceDpi]);
    }
    DeviceResolutionPath += String(DeviceDPIs_[deviceDpi]);
    GameContext::Get().dpiScale_ = DeviceDPIScales_[deviceDpi];
    config.deviceDPI_ = deviceDpi;

    // Add UI Graphics Device Path for the uidpi
    int uiDeviceDpi = config.uiDeviceDPI_;
    String uiDeviceResolutionPath("Data/UI/Graphics/");
    pathName = fs->GetProgramDir() + uiDeviceResolutionPath + String(DeviceDPIs_[uiDeviceDpi]);
    while (!fs->DirExists(pathName))
    {
        uiDeviceDpi--;
        pathName = fs->GetProgramDir() + uiDeviceResolutionPath + String(DeviceDPIs_[uiDeviceDpi]);
    }
    uiDeviceResolutionPath += String(DeviceDPIs_[uiDeviceDpi]);
    GameContext::Get().uiDpiScale_ = DeviceDPIScales_[uiDeviceDpi];

    // Set Engine ResourcePaths
    String ResourcePaths(engineParameters_["ResourcePaths"].GetString());
    ResourcePaths.Append(";");
    ResourcePaths += DeviceResolutionPath;
    ResourcePaths.Append(";");
    ResourcePaths += uiDeviceResolutionPath;
    engineParameters_["ResourcePaths"] = ResourcePaths;

    GameContext::Get().gameConfig_.logString += "ResourcePaths=" + ResourcePaths;
//    CheckGraphicBackEnds(config.logString);

    GameContext::Get().gameConfig_.logLevel_ = engineParameters_["LogLevel"].GetInt();
}

String Game::LoadGameConfig(const String& fileName, GameConfig* config)
{
    // Read File in buffer
    File f(context_, fileName, FILE_READ);
    unsigned size = f.GetSize();
    if (!size)
        return String::EMPTY;

    unsigned char* buffer = new unsigned char[size];
    if (f.Read(buffer, size) != size)
    {
        config->logString += ToString("Apply %s... read file ERROR ! \n", fileName.CString());
        return String::EMPTY;
    }

    // parse xml File
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_buffer(buffer, size);

    delete[] buffer;

    if (result.status != pugi::status_ok)
    {
        config->logString += ToString("Apply %s... ERROR ! \n", fileName.CString());
//        std::cout << result.description()  << std::endl;
        return String::EMPTY;
    }

    config->logString += ToString("Apply %s... \n", fileName.CString());
//    std::cout << config->logString.CString();
//    std::cout << "Apply engine_config.xml... "<< std::endl << std::endl;
    // Root Elem
    pugi::xml_node root = doc.first_child();

    for (pugi::xml_node paramElem = root.child("parameter"); paramElem; paramElem = paramElem.next_sibling("parameter"))
    {
        const String& name = paramElem.attribute("name").value();
        if (engineParameters_.Contains(name)) continue;

        if (name == "WindowTitle" || name == "WindowIcon" || name == "ResourcePaths" || name == "LogName")
        {
            engineParameters_[name] = paramElem.attribute("value").value();
            config->logString += ToString("  engineParameters_[%s] = %s \n", name.CString(),paramElem.attribute("value").value());
//            std::cout << config->logString.CString();
//            std::cout << "  engineParameters_[" << name.CString() << "] = " << paramElem.attribute("value").value() << std::endl;
        }
        else if (name == "WindowWidth" || name == "WindowHeight" || name == "LogLevel" || name == "MaterialQuality" || name == "TextureQuality"
                 || name == "MultiSample" || name == "TextureFilterMode" || name == "WindowPositionX" || name == "WindowPositionY" || name == "RefreshRate")
        {
            engineParameters_[name] = paramElem.attribute("value").as_int();
            config->logString += ToString("  engineParameters_[%s] = %d \n", name.CString(),paramElem.attribute("value").as_int());
//            std::cout << config->logString.CString();
//            std::cout << "  engineParameters_[" << name.CString() << "] = " << paramElem.attribute("value").as_int() << std::endl;
        }
        else
        {
            engineParameters_[name] = paramElem.attribute("value").as_bool();
            config->logString += ToString("  engineParameters_[%s] = %s \n", name.CString(),paramElem.attribute("value").as_bool()==1?"true":"false");
//            std::cout << config->logString.CString();
//            std::cout << "  engineParameters_[" << name.CString() << "] = " << (paramElem.attribute("value").as_bool()==1?"true":"false") << std::endl;
        }
    }

    // See Todo category "Game General" 09/11/2022
    engineParameters_["TextureQuality"] = 2;

//    std::cout << std::endl;
    for (pugi::xml_node varElem = root.child("variable"); varElem; varElem = varElem.next_sibling("variable"))
    {
        const String& name = varElem.attribute("name").value();
        if (name == "language_" || name == "frameLimiter_" || name == "networkServerPort_")
        {
            int value = varElem.attribute("value").as_int();
            if (name == "language_")
                config->language_ = value;
            else if (name == "frameLimiter_")
                config->frameLimiter_ = value;
            else if (name == "networkServerPort_")
                config->networkServerPort_ = value;

            config->logString += ToString("  (Int) %s = %d \n", name.CString(), value);
//            std::cout << config->logString.CString();
        }
        else if (name == "initState_" || name == "sceneToLoad_" || name == "networkServerIP_" ||
                 name == "networkMode_" || name == "deviceDPI_" || name == "uiDeviceDPI_")
        {
            const String& value = varElem.attribute("value").value();

            if (name == "sceneToLoad_")
                ;
            //GameContext::sceneToLoad_ = varElem.attribute("value").value();
            else if (name == "initState_")
                config->initState_ = value;
            else if (name == "networkServerIP_")
                config->networkServerIP_ = value;
            else if (name == "networkMode_")
                config->networkMode_ = value;
            else if (name == "deviceDPI_")
            {
                config->deviceDPI_ = GetEnumValue(value, DeviceDPIs_);
                if (config->deviceDPI_ == -1)
                    config->deviceDPI_ = DPI_AUTO;
            }
            else if (name == "uiDeviceDPI_")
            {
                config->uiDeviceDPI_ = GetEnumValue(value, DeviceDPIs_);
                if (config->uiDeviceDPI_ == -1)
                    config->uiDeviceDPI_ = DPI_AUTO;
            }
            config->logString += ToString("  (String) %s = %s \n", name.CString(), value.CString());
//            std::cout << config->logString.CString();
        }
        else if (name == "tileSpanning_")
        {
            float value = varElem.attribute("value").as_float();
            config->tileSpanning_ = value;
        }
        else
        {
            bool value = varElem.attribute("value").as_bool();
            if (name == "SoundEnabled_") config->soundEnabled_ = value;
            else if (name == "MusicEnabled_") config->musicEnabled_ = value;

            else if (name == "touchEnabled_") config->touchEnabled_ = value;
            else if (name == "forceTouch_") config->forceTouch_ = value;
            else if (name == "screenJoystick_") config->screenJoystick_ = value;

            else if (name == "ctrlCameraEnabled_") config->ctrlCameraEnabled_ = value;
            else if (name == "HUDEnabled_") config->HUDEnabled_ = value;
            else if (name == "physics3DEnabled_") config->physics3DEnabled_ = value;
            else if (name == "physics2DEnabled_") config->physics2DEnabled_ = value;
            else if (name == "asynLoadingWorldMap_") config->asynLoadingWorldMap_ = value;
            else if (name == "enlightScene_") config->enlightScene_ = value;
            else if (name == "fluidEnabled_") config->fluidEnabled_ = value;

            else if (name == "multiViews_") config->multiviews_ = value;
            else if (name == "renderShapes_") config->renderShapes_ = value;

            else if (name == "debugRenderEnabled_") config->debugRenderEnabled_ = value;
            else if (name == "debugPathFinder_") config->debugPathFinder_ = value;
            else if (name == "debugPhysics_") config->debugPhysics_ = value;
            else if (name == "debugLights_") config->debugLights_ = value;
            else if (name == "debugWorld2D_") config->debugWorld2D_ = value;
            else if (name == "debugWorld2DTagged_") config->debugWorld2DTagged_ = value;
            else if (name == "debugUI_") config->debugUI_ = value;
            else if (name == "debugFluid_") config->debugFluid_ = value;
            else if (name == "debugSprite2D_") config->debugSprite2D_ = value;
            else if (name == "debugBodyExploder_") config->debugBodyExploder_ = value;
            else if (name == "debugScrollingShape_") config->debugScrollingShape_ = value;
            else if (name == "debugObjectTiled_") config->debugObjectTiled_ = value;
            else if (name == "debugRenderShape_") config->debugRenderShape_ = value;

            config->logString += ToString("  (bool) %s = %s \n", name.CString(), value ? "true":"false");
//            std::cout << config->logString.CString();
//            std::cout << "  (bool) " << name.CString() << " = " << (value ? "true":"false") << std::endl;
        }
    }

    config->logString += ToString("\n");
//    std::cout << std::endl;

    return fileName;
}

#ifdef __LINUX__
#include <link.h>
int loadedSOcallback(struct dl_phdr_info *info, size_t size, void *data)
{
    URHO3D_LOGINFOF("Game() - %s used", info->dlpi_name);
    return 0;
}
#endif

void ShowLoadedSo()
{
#ifdef __LINUX__
    dl_iterate_phdr(loadedSOcallback, 0);
#endif
}

void Game::Start()
{
#if defined(PERFORMTESTS)
    PerformTests(context_);
#endif

    URHO3D_LOGINFO("Game() - ----------------------------------------");
    URHO3D_LOGINFO("Game() - Start                                  -");
    URHO3D_LOGINFO("Game() - ----------------------------------------");
    URHO3D_LOGINFO("Game() - executable = " + GetExecutableName());
#if defined(ACTIVE_CLIPPING)
    URHO3D_LOGINFO("Game() - clipping = true");
#endif
    ShowLoadedSo();

    Graphics* graphics = GetSubsystem<Graphics>();

    bool srgb = graphics->GetSRGBSupport();

    URHO3D_LOGINFOF("Game() - Display Device Name = %s using Graphics in %s(Dpi~%f) sRGB=%s",
                    SDL_GetDisplayName(0), DeviceDPIs_[GameContext::Get().gameConfig_.deviceDPI_],
                    DeviceDPIValues_[GameContext::Get().gameConfig_.deviceDPI_],
                    srgb ? "true":"false");

    graphics->SetSRGB(srgb);

#if defined(URHO3D_OPENGL)
    {
        if (graphics->GetApiName().StartsWith("GL"))
        {
            String glslver((char*) glGetString(GL_SHADING_LANGUAGE_VERSION));

            // keep numerical
            String sVersion;
            for (int i=0; i < glslver.Length(); i++)
                if (glslver[i] >= '0' && glslver[i] <= '9')
                    sVersion += glslver[i];

            int version = ToInt(sVersion);

            URHO3D_LOGINFOF("Vendor graphic card : %s", glGetString(GL_VENDOR));
            URHO3D_LOGINFOF("Renderer            : %s", glGetString(GL_RENDERER));
            URHO3D_LOGINFOF("Version GL          : %s", glGetString(GL_VERSION));
            URHO3D_LOGINFOF("Version GLSL        : %s (%d)", glGetString(GL_SHADING_LANGUAGE_VERSION), version);

            // TODO : to change when shaders GLES will be ready
            if (glslver.Contains("ES"))
            {
                version = 120;
            }

            // Get Shader Paths
            FileSystem* fs = context_->GetSubsystem<FileSystem>();
            if (fs)
            {
                String shaderRootPath = fs->GetProgramDir() + "Data/Shaders/";
                URHO3D_LOGINFOF("Shader root Path    : %s", shaderRootPath.CString());
                Vector<String> results;
                Vector<int> availableShaderVersions;
                fs->ScanDir(results, shaderRootPath, "*", SCAN_DIRS, false);
                for (int i = 0; i < results.Size(); i++)
                {
                    const String& shaderDir = results[i];
                    if (shaderDir.StartsWith("GLSL-"))
                        availableShaderVersions.Push(ToInt(shaderDir.Substring(5)));
                }
//                URHO3D_LOGINFOF("Available versions  :");
//                for (int i=0; i < availableShaderVersions.Size(); i++)
//                {
//                    URHO3D_LOGINFOF(" -> %d", availableShaderVersions[i]);
//                }
                int bestversion = 0;
                for (int i=0; i < availableShaderVersions.Size(); i++)
                {
                    if (bestversion < availableShaderVersions[i] && availableShaderVersions[i] <= version)
                        bestversion = availableShaderVersions[i];
                }
                URHO3D_LOGINFOF("Best Shader version : %d", bestversion);
                version = bestversion;
            }
            else
            {
                if (version >= 130)
                    version = 130;
                else if (version >= 120)
                    version = 120;
                else
                    version = 110;
            }

            URHO3D_LOGINFOF("Shader Path         : Shaders/GLSL-%s/", String(version).CString());
            graphics->SetShaderPath(String("Shaders/GLSL-")+String(version)+"/");
        }
    }
#endif

//    int numsdldrivers = SDL_GetNumVideoDrivers();
//    for (int i=0; i < numsdldrivers; i++)
//    {
//        URHO3D_LOGINFOF("SDL available video driver [%d] = %s", i, SDL_GetVideoDriver(i));
//    }

//	URHO3D_LOGINFOF("Game() - Generate StringHash(ActiveAbility) = %u", StringHash("ActiveAbility").Value());
//	URHO3D_LOGINFOF("Game() - Generate StringHash(Ability) = %u", StringHash("Ability").Value());
//	URHO3D_LOGINFOF("Game() - Generate StringHash(AppliedMapping) = %u", StringHash("AppliedMapping").Value());
//  URHO3D_LOGINFOF("Game() - Generate StringHash(Entity) = %u", StringHash("Entity").Value());
//  URHO3D_LOGINFOF("Game() - Generate StringHash(MaxTickDelay) = %u", StringHash("MaxTickDelay").Value());

//    URHO3D_LOGINFOF("Game() - Generate Matrix90_2D() = %s", Matrix2x3(Vector2::ZERO, 90.f, 1.f).ToString().CString());

    SetupDirectories();

    GameHelpers::SetGameLogFilter(GAMELOGFILTER);
    URHO3D_LOGINFOF("Game() - LogFilter = %u", GameHelpers::GetGameLogFilter());

    URHO3D_LOGINFOF("%s", GameContext::Get().gameConfig_.logString.CString());
    URHO3D_LOGINFOF("Game() - Engine Config Applied = %s", engineConfigApplied_.Empty() ? "Default" : engineConfigApplied_.CString());

#ifdef URHO3D_PROFILING
    URHO3D_LOGINFOF("Game() - Engine Profiling actived !");
#endif

    // Create All Statics : Camera, Scene
    GameContext::Get().Initialize();

    SubscribeToEvent(E_POSTUPDATE, URHO3D_HANDLER(Game, HandleAsynchronousUpdate));

    // Detect Controls (Touch, Joystick)
    SetupControllers();

    // Network, HUD, Localization
    SetupSubSystems();

    // Start Managers
    GameContext::Get().Start();

    // Preload Ressources
    SubscribeToEvent(E_SCENEUPDATE, URHO3D_HANDLER(Game, HandlePreloadResources));

//    SpriteSheet2D* spritesheet = context_->GetSubsystem<ResourceCache>()->GetResource<SpriteSheet2D>("2D/scraps.xml");
//    GameHelpers::SaveOffsetSpriteSheet(spritesheet, IntVector2(784, 512-180), "Data/2D/scrapsoffseted.xml");

    URHO3D_LOGINFO("Game() - ----------------------------------------");
    URHO3D_LOGINFO("Game() - Start .... OK !                        -");
    URHO3D_LOGINFO("Game() - ----------------------------------------");
}

void Game::Stop()
{
    URHO3D_LOGERROR("Game() - ----------------------------------------");
    URHO3D_LOGERROR("Game() - Stop ...                               -");
    URHO3D_LOGERROR("Game() - ----------------------------------------");

    UnsubscribeFromAllEvents();

    GameContext::Get().Stop();

#ifdef DUMP_UI
    PODVector<UIElement*> children;
    GetSubsystem<UI>()->GetRoot()->GetChildren(children, true);
    for (PODVector<UIElement*>::ConstIterator it=children.Begin(); it!=children.End(); ++it)
        URHO3D_LOGINFOF(" UI child = %s",(*it)->GetName().CString());
#endif

    URHO3D_LOGERROR("Game() - ----------------------------------------");
    URHO3D_LOGERROR("Game() - Stop ...       OK !                     -");
    URHO3D_LOGERROR("Game() - ----------------------------------------");

#ifdef DUMP_RESOURCES
    engine_->DumpResources(true);
    //engine_->DumpMemory();
#endif

#ifdef URHO3D_VULKAN
    GetSubsystem<Graphics>()->GetImpl()->DumpRegisteredPipelineInfo();
#endif

    engine_->DumpProfiler();

    ShowLoadedSo();

    /// GAME EXIT

//#ifdef __ANDROID__
//    CallAndroidActivityMethod("finish", "()V");
//#endif
}


void Game::SetupDirectories()
{
    GameConfig* config = &GameContext::Get().gameConfig_;

    URHO3D_LOGINFO("Game() - SetupDirectories ...");
    FileSystem* fs = context_->GetSubsystem<FileSystem>();
    if (!fs)
    {
        URHO3D_LOGERROR("Game() - SetupDirectories : No FileSystem Found");
        return;
    }

    String saveDir = fs->GetAppPreferencesDir("OkkoStudio", GameContext::GAMENAME);
    if (saveDir.Empty())
    {
        saveDir = fs->GetUserDocumentsDir();
        if (saveDir.Empty())
        {
            URHO3D_LOGERROR("RegisterWorldPath : Can not Set Directories !");
            context_->GetSubsystem<Engine>()->Exit();
            return;
        }
        else
        {
            saveDir += ".OkkoStudio/" + GameContext::GAMENAME + "/";
            if (!fs->DirExists(saveDir))
                fs->CreateDir(saveDir);
        }
    }

    if (!fs->DirExists(saveDir + GAMESAVEDIR))
        fs->CreateDir(saveDir + GAMESAVEDIR);

    if (!fs->DirExists(saveDir + SAVELEVELSDIR))
        fs->CreateDir(saveDir + SAVELEVELSDIR);

    if (!fs->DirExists(saveDir +GAMEDATADIR))
        fs->CreateDir(saveDir + GAMEDATADIR);

    if (!fs->DirExists(saveDir + DATALEVELSDIR))
        fs->CreateDir(saveDir + DATALEVELSDIR);

    config->saveDir_ = saveDir;

//    fs->SetCurrentDir(fs->GetProgramDir());

    URHO3D_LOGINFOF("Game() - SetupDirectories : saveDir_ = %s ... OK !", saveDir.CString());
}

void Game::SetupControllers()
{
    URHO3D_LOGINFO("Game() - ----------------------------------------");
    URHO3D_LOGINFO("Game() - SetupControllers ...                   -");
    URHO3D_LOGINFO("Game() - ----------------------------------------");

    input_ = GetSubsystem<Input>();
    GameConfig& config = GameContext::Get().gameConfig_;

    int numjoysticks = input_->GetNumJoysticks();
    URHO3D_LOGINFOF("Game() - SetupControllers : NumJoysticks = %d", numjoysticks);

    // Set Joysticks
    GameContext::Get().InitJoysticks();

    SubscribeToEvent(E_JOYSTICKCONNECTED, URHO3D_HANDLER(Game, HandleJoystickChange));
    SubscribeToEvent(E_JOYSTICKDISCONNECTED, URHO3D_HANDLER(Game, HandleJoystickChange));

    // Set Touch Control
    {
        if (GetPlatform() == "Android" || GetPlatform() == "iOS")
        {
            URHO3D_LOGINFO("Game() - SetupControllers : Android or IOS : InitTouchInput ");

            GameContext::Get().InitTouchInput();
        }
        else if (!input_->GetNumJoysticks() || config.forceTouch_)
        {
            if (!config.touchEnabled_ && !config.forceTouch_)
            {
                SubscribeToEvent(E_TOUCHBEGIN, URHO3D_HANDLER(Game, HandleTouchBegin));
            }
            else if (config.forceTouch_)
            {
                GameContext::Get().InitTouchInput();
                input_->SetTouchEmulation(true);
                URHO3D_LOGINFO("Game() - SetupControllers : Desktop force TouchEmulation !");
            }

            URHO3D_LOGINFOF("Game() - SetupControllers : touchEnabled_=%s forceTouch_=%s",
                            config.touchEnabled_ ? "true":"false", config.forceTouch_ ? "true":"false");
        }
    }

    // Subscribe Cursor Mouse Move
//	if (!config.touchEnabled_)
    GameContext::Get().InitMouse(MM_FREE);

    GameContext::Get().PreselectBestControllers();

    URHO3D_LOGINFO("Game() - ----------------------------------------");
    URHO3D_LOGINFO("Game() - SetupControllers ... OK!               -");
    URHO3D_LOGINFO("Game() - ----------------------------------------");
}

void Game::SetupSubSystems()
{
    URHO3D_LOGINFO("Game() - ----------------------------------------");
    URHO3D_LOGINFO("Game() - SetupSubSystems ...                    -");
    URHO3D_LOGINFO("Game() - ----------------------------------------");

    GameConfig* config = &GameContext::Get().gameConfig_;

    // Set Localization
    Localization* l10n = GetSubsystem<Localization>();
    l10n->LoadJSONFile("Texts/UI_messages.json");
    l10n->SetLanguage(config->language_);

    // Set Game Texts
    GameData* data = new GameData(context_);
    data->SetLanguage(l10n->GetLanguage());
//    data->GetDialogueData()->Dump();
//    data->GetJournalData()->Dump();

    UI* ui = GetSubsystem<UI>();
    if (ui)
    {
        ResourceCache* cache = GetSubsystem<ResourceCache>();

        if (config->HUDEnabled_)
        {
            // Get default style
            XMLFile* xmlFile = cache->GetResource<XMLFile>("UI/DefaultStyle.xml");
            if (xmlFile)
            {
                // Create console
                Console* console = engine_->CreateConsole();
                if (console)
                {
                    GameContext::Get().console_ = console;
                    console->SetDefaultStyle(xmlFile);
                    console->SetNumRows(10);
                    console->SetNumBufferedRows(200);
#ifdef ACTIVE_CONSOLECOMMAND
                    console->SetCommandInterpreter(GetTypeName());
                    SubscribeToEvent(E_CONSOLECOMMAND, URHO3D_HANDLER(Game, HandleConsoleCommand));
#endif
                }

                // Create debug HUD.
                DebugHud* debugHud = engine_->CreateDebugHud();
                if (debugHud)
                {
                    debugHud->SetDefaultStyle(xmlFile);
                    SubscribeToEvent(E_KEYDOWN, URHO3D_HANDLER(Game, HandleKeyDownHUD));
                }

                // Default style on root
                ui->GetRoot()->SetDefaultStyle(xmlFile);
            }
        }
    }

#ifdef ACTIVE_NETWORK
    // Network
    if (config->networkMode_ != "local")
    {
        GameNetwork* network = new GameNetwork(context_);
        network->SetServerInfo(config->networkServerIP_, config->networkServerPort_);
        network->SetMode(Engine::GetParameter(engineParameters_, "NetMode", config->networkMode_).GetString());
    }
#endif

    if (config->networkMode_ == "local")
    {
        GameNetwork::AddGraphicMessage(context_, "Local.Mode.", IntVector2(1, 10), WHITEGRAY30, 0.f);

        GetSubsystem<Network>()->Stop();
    }

    // Audio
    Audio* audio = GetSubsystem<Audio>();
    if (audio)
    {
        audio->SetMasterGain(SOUND_MASTER, 1.f);
        audio->SetMasterGain(SOUND_EFFECT, 0.1f);
        audio->SetMasterGain(SOUND_AMBIENT, 0.5f);
        audio->SetMasterGain(SOUND_VOICE, 0.75f);
        audio->SetMasterGain(SOUND_MUSIC, 0.5f);
    }

    // Handlers
    SubscribeToEvent(E_SCREENMODE, URHO3D_HANDLER(Game, HandleWindowResize));

    URHO3D_LOGINFO("Game() - ----------------------------------------");
    URHO3D_LOGINFO("Game() - SetupSubSystems ... OK!                -");
    URHO3D_LOGINFO("Game() - ----------------------------------------");
}


void Game::HandlePreloadResources(StringHash eventType, VariantMap& eventData)
{
    if (GameContext::Get().PreloadResources())
    {
        UnsubscribeFromEvent(E_SCENEUPDATE);
        SendEvent(GAME_PRELOADINGFINISHED);

        GameContext::Get().SubscribeToCursorVisiblityEvents();

        SubscribeToEvent(E_BEGINRENDERING, URHO3D_HANDLER(Game, HandleBeginRendering));
    }
}

void Game::HandleAsynchronousUpdate(StringHash eventType, VariantMap& eventData)
{
    {
        Vector<World2DInfo>& world2DInfos = MapStorage::GetAllWorld2DInfos();

        unsigned numWorldModels = 0;
        for (unsigned i=0; i < world2DInfos.Size(); i++)
        {
            World2DInfo& worldinfo = world2DInfos[i];
            if (worldinfo.worldModel_)
            {
                numWorldModels++;
                if (worldinfo.worldModel_->UpdateGeneratingSnapShots())
                    break;
            }
        }

//		URHO3D_LOGINFOF("Game() - HandleAsynchronousUpdate ... Num world2DInfos = %u  Num worldModel Checked = %u", world2DInfos.Size(), numWorldModels);
    }
}

void Game::HandleTouchBegin(StringHash eventType, VariantMap& eventData)
{
    // On some platforms like Windows the presence of touch input can only be detected dynamically
    GameContext::Get().InitTouchInput();
    UnsubscribeFromEvent("TouchBegin");
}

void Game::HandleKeyDownHUD(StringHash eventType, VariantMap& eventData)
{
    using namespace KeyDown;

    if (eventData[P_QUALIFIERS].GetInt())
        return;

    int scancode = eventData[P_SCANCODE].GetInt();

//	if (scancode == SCANCODE_ESCAPE)
//	{
//        HandleMenuButton(eventType, eventData);
//
//        // 20200516 - Important : to not be grab to an element at start (ensure a reactive ui)
////        context_->GetSubsystem<Input>()->SetMouseGrabbed(false, true);
//	}

    // Toggle console with F1
    if (scancode == SCANCODE_F1)
    {
        Console* console = GetSubsystem<Console>();
        if (console)
            console->Toggle();
    }
    // Toggle debug HUD with F2
    else if (scancode == SCANCODE_F2)
    {
        DebugHud* debugHud = GetSubsystem<DebugHud>();
        if (debugHud)
        {
            if (debugHud->GetMode() == DEBUGHUD_SHOW_NONE)
                debugHud->SetMode(DEBUGHUD_SHOW_FPS);
            else if (debugHud->GetMode() == DEBUGHUD_SHOW_FPS)
                debugHud->SetMode(DEBUGHUD_SHOW_ALL);
            else if (debugHud->GetMode() == DEBUGHUD_SHOW_ALL)
                debugHud->SetMode(DEBUGHUD_SHOW_EVENTPROFILER);
            else if (debugHud->GetMode() == DEBUGHUD_SHOW_EVENTPROFILER)
                debugHud->SetMode(DEBUGHUD_SHOW_NONE);
        }
    }
    // Toggle createmode with F3
    else if (scancode == SCANCODE_F3)
    {
        GameContext::Get().forceCreateMode_ = !GameContext::Get().forceCreateMode_;

        URHO3D_LOGINFOF("Game() - Switch ForceCreateMode = %s", GameContext::Get().forceCreateMode_ ? "on":"off");
    }
    else if (scancode == SCANCODE_F4)
    {
        // For Debug : toggle Dump UIBatch
        UIBatch::dumpBatchMerging_ = !UIBatch::dumpBatchMerging_;
    }
#ifdef URHO3D_VULKAN
    else if (scancode == SCANCODE_X)
        context_->GetSubsystem<Graphics>()->GetImpl()->DumpRegisteredPipelineInfo();
#endif
    // Common rendering quality controls, only when UI has no focused element
    else// if (!GetSubsystem<UI>()->GetFocusElement())
    {
        Renderer* renderer = GetSubsystem<Renderer>();
        if (!renderer) return;
        // Texture quality
        else if (scancode == SCANCODE_1)
        {
            int quality = renderer->GetTextureQuality();
            ++quality;
            if (quality > QUALITY_HIGH)
                quality = QUALITY_LOW;

            renderer->SetTextureQuality(quality);
            Sprite2D::SetTextureLevels(GetSubsystem<Renderer>()->GetTextureQuality());
            SendEvent(WORLD_DIRTY);
            URHO3D_LOGINFOF("SetTextureQuality = %d", quality);
        }

        // Material quality
        else if (scancode == SCANCODE_2)
        {
            int quality = renderer->GetMaterialQuality();
            ++quality;
            if (quality > QUALITY_HIGH)
                quality = QUALITY_LOW;

            GameContext::Get().gameConfig_.enlightScene_ = (quality > 0);

            renderer->SetMaterialQuality(quality);
            URHO3D_LOGINFOF("SetMaterialQuality = %d", quality);
        }

        // Specular lighting
        else if (scancode == SCANCODE_3)
            renderer->SetSpecularLighting(!renderer->GetSpecularLighting());

        // Shadow rendering
        else if (scancode == SCANCODE_4)
            renderer->SetDrawShadows(!renderer->GetDrawShadows());

        // Shadow map resolution
        else if (scancode == SCANCODE_5)
        {
            int shadowMapSize = renderer->GetShadowMapSize();
            shadowMapSize *= 2;
            if (shadowMapSize > 2048)
                shadowMapSize = 512;
            renderer->SetShadowMapSize(shadowMapSize);
        }

        // Shadow depth and filtering quality
        else if (scancode == SCANCODE_6)
        {
            ShadowQuality quality = renderer->GetShadowQuality();
            quality = (ShadowQuality)(quality + 1);
            if (quality > SHADOWQUALITY_BLUR_VSM)
                quality = SHADOWQUALITY_SIMPLE_16BIT;
            renderer->SetShadowQuality(quality);
        }

        // Occlusion culling
        else if (scancode == SCANCODE_7)
        {
            bool occlusion = renderer->GetMaxOccluderTriangles() > 0;
            occlusion = !occlusion;
            renderer->SetMaxOccluderTriangles(occlusion ? 5000 : 0);
        }

        // Instancing
        else if (scancode == SCANCODE_8)
            renderer->SetDynamicInstancing(!renderer->GetDynamicInstancing());

        // Take screenshot
        else if (scancode == SCANCODE_9)
        {
            Graphics* graphics = GetSubsystem<Graphics>();
            Image screenshot(context_);
            graphics->TakeScreenShot(screenshot);
            // Here we save in the Data folder with date and time appended
            screenshot.SavePNG(GetSubsystem<FileSystem>()->GetProgramDir() + "Data/Screenshot_" +
                               Time::GetTimeStamp().Replaced(':', '_').Replaced('.', '_').Replaced(' ', '_') + ".png");
        }
    }
}

void Game::HandleWindowResize(StringHash eventType, VariantMap& eventData)
{
    GameContext::Get().ResetScreen();

    SendEvent(GAME_SCREENRESIZED);
}

void Game::HandleBeginRendering(StringHash eventType, VariantMap& eventData)
{
    // reset the log filter : allow show log from engine
    GameHelpers::SetGameLogLevel(0);
}

void Game::HandleJoystickChange(StringHash eventType, VariantMap& eventData)
{
//    if (eventType == E_JOYSTICKCONNECTED)
//    {
//        int joyid = eventData[JoystickConnected::P_JOYSTICKID].GetInt();
//
//    }
//
//    if (eventType == E_JOYSTICKDISCONNECTED)
//    {
//        int joyid = eventData[JoystickDisconnected::P_JOYSTICKID].GetInt();
//
//    }

    GameContext::Get().ValidateJoysticks();
}

void Game::HandleConsoleCommand(StringHash eventType, VariantMap& eventData)
{
    using namespace ConsoleCommand;
    if (eventData[P_ID].GetString() != GetTypeName())
        return;

    GameCommands::Launch(context_, eventData[P_COMMAND].GetString());
}


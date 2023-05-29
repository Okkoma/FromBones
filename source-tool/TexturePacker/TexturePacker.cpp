
#include <Urho3D/Core/Context.h>
#include <Urho3D/Core/ProcessUtils.h>
#include <Urho3D/Core/StringUtils.h>
#include <Urho3D/IO/File.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Resource/Image.h>
#include <Urho3D/Resource/XMLElement.h>
#include <Urho3D/Resource/XMLFile.h>

#include <Urho3D/Urho2D/AnimationSet2D.h>
#include <Urho3D/Urho2D/AnimatedSprite2D.h>
#include <Urho3D/Urho2D/StaticSprite2D.h>
#include <Urho3D/Urho2D/Sprite2D.h>
#include <Urho3D/Urho2D/SpriteSheet2D.h>

#ifdef WIN32
#include <windows.h>
#endif

#include "MemoryObjects.h"

#define STBRP_LARGE_RECTS
#define STB_RECT_PACK_IMPLEMENTATION
#include <STB/stb_rect_pack.h>

#include <Urho3D/DebugNew.h>

using namespace Urho3D;

const int MIN_TEXTURE_SIZE = 256;
const int MAX_TEXTURE_SIZE = 2048;

IntVector2 sOffset(IntVector2::ONE);

struct PackerInfo
{
    String path;
    String name;
    IntVector2 position;
    IntVector2 size;
    IntVector2 frame;
    IntVector2 framesize;
    IntVector2 hotspot;
};

struct Info
{
    String basepngfolder;
    String inputentityfolder;
    String inputscmlfolder;
    String inputspritefolder;

    String outputgraphicfolder;
    String outputscmlfolder;
    String outputtexturefolder;
    String outputtexturefile;

    float dpiratio;

    Vector<String> entityfiles;
    Vector<String> spritefiles;
    Vector<String> scmlfiles;
    HashMap<String, String> defaultsprite;
    HashMap<String, Vector<String> > duplicatespritenames;
    HashMap<String, PackerInfo > emptysprites;
    HashMap<String, Vector2 > customhotspots;
    HashMap<String, Vector<String> > customspritesheets;

    Vector<Vector<String> > spritesbyscml;
    Vector<PackerInfo> packerinfos;
    Vector<PackerInfo*> sortedpackerinfos;

    bool addemptysprites;
    bool skipfirst;
};


/// Sorting Functions

static bool SortByWidth(const stbrp_rect& lhs, const stbrp_rect& rhs)
{
    return lhs.w > rhs.w;
}

static bool SortByHeight(const stbrp_rect& lhs, const stbrp_rect& rhs)
{
    return lhs.h > rhs.h;
}

static bool SortByName(PackerInfo* lhs, PackerInfo* rhs)
{
    return String::Compare(lhs->name.CString(), rhs->name.CString(), false) < 0;
}


/// Algorithms

bool PackRectsSTB(int width, int height, PODVector<stbrp_rect>& rects)
{
    const int PACKER_NUM_NODES = 4096;

    stbrp_context packerContext;
    stbrp_node packerMemory[PACKER_NUM_NODES];
    stbrp_init_target(&packerContext, width, height, packerMemory, PACKER_NUM_NODES);
    stbrp_setup_heuristic(&packerContext, STBRP_HEURISTIC_Skyline_BF_sortHeight);
    return stbrp_pack_rects(&packerContext, &rects[0], rects.Size());
}

bool PackRectsNaiveRows(int width, int height, PODVector<stbrp_rect>& rects)
{
    // Sort by a heuristic
    Sort(rects.Begin(), rects.End(), SortByHeight);

    int xPos = 0;
    int yPos = 0;
    int largestHThisRow = 0;

    bool result = true;

    // Loop over all the rectangles
    for (PODVector<stbrp_rect>::Iterator it = rects.Begin(); it != rects.End(); ++it)
    {
        stbrp_rect& rect = *it;
        rect.was_packed = 0;

        // If this rectangle will go past the width of the image
        // Then loop around to next row, using the largest height from the previous row
        if ((xPos + rect.w) > width)
        {
            yPos += largestHThisRow;
            xPos = 0;
            largestHThisRow = 0;
        }

        // If we go off the bottom edge of the image, then we've failed
        if ((yPos + rect.h) > height)
        {
            result = false;
            break;
        }

        // This is the position of the rectangle
        rect.x = xPos;
        rect.y = yPos;

        // Move along to the next spot in the row
        xPos += rect.w;

        // Just saving the largest height in the new row
        if (rect.h > largestHThisRow)
            largestHThisRow = rect.h;

        // Success!
        rect.was_packed = 1;
    }

    return result;
}

bool PackRectsNaiveCols(int width, int height, PODVector<stbrp_rect>& rects)
{
    // Sort by a heuristic
    Sort(rects.Begin(), rects.End(), SortByWidth);

    int xPos = 0;
    int yPos = 0;
    int largestHThisCol = 0;

    bool result = true;

    // Loop over all the rectangles
    for (PODVector<stbrp_rect>::Iterator it = rects.Begin(); it != rects.End(); ++it)
    {
        stbrp_rect& rect = *it;
        rect.was_packed = 0;

        // If this rectangle will go past the height of the image
        // Then loop around to next col, using the largest width from the previous col
        if ((yPos + rect.h) > height)
        {
            xPos += largestHThisCol;
            yPos = 0;
            largestHThisCol = 0;
        }

        // If we go off the bottom edge of the image, then we've failed
        if ((xPos + rect.w) > width)
        {
            result = false;
            break;
        }

        // This is the position of the rectangle
        rect.x = xPos;
        rect.y = yPos;

        // Move along to the next spot in the row
        yPos += rect.h;

        // Just saving the largest height in the new row
        if (rect.w > largestHThisCol)
            largestHThisCol = rect.w;

        // Success!
        rect.was_packed = 1;
    }

    return result;
}

bool PackRectsBLPixels(int width, int height, PODVector<stbrp_rect>& rects, bool skipfirst)
{
    int first = 0;

    if (skipfirst)
    {
        first = 1;
        stbrp_rect& rect = rects[0];
        if (rect.w > width || rect.h > height)
            return false;
    }

    // Sort by a heuristic
    Sort(rects.Begin()+first, rects.End(), SortByHeight);

    // Maintain a grid of bools, telling us whether each pixel has got a rect on it
    Matrix2D<bool> image(width, height);
    image.SetBufferValue(false);

    if (skipfirst)
    {
        stbrp_rect& rect = rects[0];

        rect.x = 0;
        rect.y = 0;

        // Set the used pixels to true so we don't overlap them
        for (int ix = rect.x; ix < rect.x + rect.w; ix++)
        {
            for (int iy = rect.y; iy < rect.y + rect.h; iy++)
            {
                image(ix, iy) = true;
            }
        }

        rect.was_packed = true;
    }

    for (PODVector<stbrp_rect>::Iterator it = rects.Begin()+first; it != rects.End(); ++it)
    {
        stbrp_rect& rect = *it;

        // Loop over X and Y pixels
        bool done = false;

        for (int y = 0; y < height && !done; y++)
        {
            // Make sure this rectangle doesn't go over the edge of the boundary
            if (y + rect.h >= height || y + rect.h < 0)
                continue;

            for (int x = 0; x < width && !done; x++)
            {
                // Make sure this rectangle doesn't go over the edge of the boundary
                if (x + rect.w >= width || x + rect.w < 0)
                    continue;

                // For every coordinate, check top left and bottom right
                if (!image(x, y) && !image(x + rect.w, y + rect.h))
                {
                    // Corners of image are free
                    // If valid, check all pixels inside that rect
                    bool valid = true;

                    for (int ix = x; ix < x + rect.w; ix++)
                    {
                        for (int iy = y; iy < y + rect.h; iy++)
                        {
                            if (image(ix, iy))
                            {
                                valid = false;
                                break;
                            }
                        }
                    }

                    // If all good, we've found a location
                    if (valid)
                    {
                        rect.x = x;
                        rect.y = y;
                        done = true;

                        // Set the used pixels to true so we don't overlap them
                        for (int ix = x; ix < x + rect.w; ix++)
                        {
                            for (int iy = y; iy < y + rect.h; iy++)
                            {
                                image(ix, iy) = true;
                            }
                        }

                        rect.was_packed = true;
                    }
                }
            }
        }

        if (!done)
            return false;
    }

    return true;
}


/// Main Functions

bool ExtractSpriteFilenames(Context* context, Info& info, bool trim)
{
    URHO3D_LOGINFO(" - Extracting files for texture " + info.outputtexturefile + " ...");

    Vector<String> scmlfiles = info.scmlfiles;
    Vector<String> spritefiles = info.spritefiles;
    info.scmlfiles.Clear();
    info.spritefiles.Clear();

    ResourceCache* cache = context->GetSubsystem<ResourceCache>();
    FileSystem* fileSystem = context->GetSubsystem<FileSystem>();

    // extract drawable resources from entityfile
    for (Vector<String>::ConstIterator it = info.entityfiles.Begin(); it != info.entityfiles.End(); ++it)
    {
        String entityfilename = info.inputentityfolder + (*it);
        URHO3D_LOGINFO("    Checking " + entityfilename + " ...");
        if (!fileSystem->FileExists(entityfilename))
        {
            URHO3D_LOGERROR("   EntityFile " + entityfilename + " does not exist.");
            return false;
        }

        SharedPtr<XMLFile> xmlfile = cache->GetTempResource<XMLFile>(entityfilename);
        XMLElement root = xmlfile->GetRoot("node");
        XMLElement component = root.GetChild("component");
        while (component)
        {
            String type = component.GetAttribute("type");
            if (type == "StaticSprite2D")
            {
                // get png filename
                XMLElement attribute = component.GetChild("attribute");
                while (attribute)
                {
                    if (attribute.GetAttribute("name") == "Sprite")
                    {
                        ResourceRef spriteref = attribute.GetResourceRef();
                        String filename;
                        if (spriteref.type_ == Sprite2D::GetTypeStatic())
                        {
                            filename = spriteref.name_;
                        }
                        else if (spriteref.type_ == SpriteSheet2D::GetTypeStatic())
                        {
                            // value.name_ include sprite sheet name and sprite name.
                            Vector<String> names = spriteref.name_.Split('@');
                            if (names.Size() != 2)
                            {
                                URHO3D_LOGERROR("   EntityFile " + filename + " error extracting sprite name !");
                                return false;
                            }

                            filename = names[1];
                        }

                        if (!filename.Empty())
                        {
                            String name = ReplaceExtension(filename, String::EMPTY);
                            if (!spritefiles.Contains(name))
                            {
//                                URHO3D_LOGINFO("    Extract  Png File " + filename + " from " + entityfilename);
                                spritefiles.Push(name);
                            }
                        }

                        break;
                    }

                    attribute = attribute.GetNext("attribute");
                }
            }
            else if (type == "AnimatedSprite2D")
            {
                // get scml filename
                XMLElement attribute = component.GetChild("attribute");

                while (attribute)
                {
                    if (attribute.GetAttribute("name") == "Animation Set")
                    {
                        ResourceRef animationsetref = attribute.GetResourceRef();
                        if (animationsetref.type_ == AnimationSet2D::GetTypeStatic())
                        {
                            String filename = GetFileNameAndExtension(animationsetref.name_);
                            if (!scmlfiles.Contains(filename))
                            {
//                                URHO3D_LOGINFO("    Extract Scml File " + filename + " from " + entityfilename);
                                scmlfiles.Push(filename);
                            }
                        }

                        break;
                    }

                    attribute = attribute.GetNext("attribute");
                }
            }

            component = component.GetNext("component");
        }
    }

    // extract png files from each scml
    for (unsigned i = 0; i < scmlfiles.Size(); i++)
    {
        String path = info.inputscmlfolder + scmlfiles[i];
        String scmlfilename = scmlfiles[i];

        URHO3D_LOGINFO("    Checking " + scmlfilename + " ...");
        if (!fileSystem->FileExists(path))
        {
            URHO3D_LOGERROR("   SCML File " + path + " does not exist.");
            return false;
        }

        info.scmlfiles.Push(scmlfiles[i]);
        info.spritesbyscml.Resize(info.spritesbyscml.Size()+1);

        URHO3D_LOGINFO("    Register spritesheet xml File for " + scmlfilename);

        SharedPtr<XMLFile> xmlfile = cache->GetTempResource<XMLFile>(path);
        XMLElement root = xmlfile->GetRoot("spriter_data");
        XMLElement folder = root.GetChild("folder");
        while (folder)
        {
            XMLElement file = folder.GetChild("file");
            while (file)
            {
                String name = ReplaceExtension(file.GetAttribute("name"), String::EMPTY);
                String path = info.inputspritefolder + name + ".png";

                if (fileSystem->FileExists(path))
                {
                    if (!spritefiles.Contains(name))
                        spritefiles.Push(name);
                }
                else if (info.addemptysprites)
                {
                    // non-existing sprite
                    PackerInfo& packerInfo = info.emptysprites[name];
                    packerInfo.name = name;
                    packerInfo.size.x_ = file.GetInt("width");
                    packerInfo.size.y_ = file.GetInt("height");
                    packerInfo.hotspot.x_ = file.GetFloat("pivot_x") * packerInfo.size.x_;
                    packerInfo.hotspot.y_ = (1.f-file.GetFloat("pivot_y")) * packerInfo.size.y_;
                }

                if (!info.spritesbyscml.Back().Contains(name))
                    info.spritesbyscml.Back().Push(name);

                file = file.GetNext("file");
            }
            folder = folder.GetNext("folder");
        }
    }

    // check all input files exist
    URHO3D_LOGINFO("    Checking PNG Files ...");

    for (unsigned i = 0; i < spritefiles.Size(); ++i)
    {
        String name = ReplaceExtension(spritefiles[i], String::EMPTY);
        String path = info.inputspritefolder + name + ".png";

        if (!fileSystem->FileExists(path))
        {
            URHO3D_LOGWARNING("    PNG File " + path + " does not exist.");
            continue;
        }

        File file(context, path);
        Image image(context);

        if (!image.Load(file))
        {
            URHO3D_LOGWARNING("   Could not load image " + path + ".");
            continue;
        }
        if (image.IsCompressed())
        {
            URHO3D_LOGWARNING("   " + path + " is compressed. Compressed images are not allowed.");
            continue;
        }

        URHO3D_LOGINFO("     PNG File added : " + path);
        info.spritefiles.Push(name);
    }

    URHO3D_LOGINFOF("    Numbers finding PNG Files = %u", info.spritefiles.Size());

    // create packer infos
    info.packerinfos.Clear();
    info.packerinfos.Resize(info.spritefiles.Size());

    for (unsigned i = 0; i < info.spritefiles.Size(); ++i)
    {
        PackerInfo& packerInfo = info.packerinfos[i];
        packerInfo.name = info.spritefiles[i];
        packerInfo.path = info.inputspritefolder + packerInfo.name + ".png";

        File file(context, packerInfo.path);
        Image image(context);
        image.Load(file);

        int imageWidth = image.GetWidth();
        int imageHeight = image.GetHeight();
        packerInfo.size.x_ = imageWidth;
        packerInfo.size.y_ = imageHeight;

        if (trim)
        {
            int trimOffsetX = 0;
            int trimOffsetY = 0;
            int adjustedWidth = imageWidth;
            int adjustedHeight = imageHeight;

            int minX = 0;
            int maxX = imageWidth;
            int minY = 0;
            int maxY = imageHeight;

            // find minX
            bool findok = false;
            Color color;
            for (int x = 0; x < imageWidth && !findok; ++x)
            for (int y = 0; y < imageHeight; ++y)
            {
                color = image.GetPixel(x, y);
                if (color.a_ != 0)
                {
                    minX = x;
                    findok = true;
                    break;
                }
            }

            // find maxX
            findok = false;
            for (int x = imageWidth-1; x >= 0 && !findok; --x)
            for (int y = 0; y < imageHeight; ++y)
            {
                color = image.GetPixel(x, y);
                if (color.a_ != 0)
                {
                    maxX = x;
                    findok = true;
                    break;
                }
            }

            // find minY
            findok = false;
            for (int y = 0; y < imageHeight && !findok; ++y)
            for (int x = 0; x < imageWidth; ++x)
            {
                color = image.GetPixel(x, y);
                if (color.a_ != 0)
                {
                    minY = y;
                    findok = true;
                    break;
                }
            }

            // find maxY
            findok = false;
            for (int y = imageHeight-1; y >= 0 && !findok; --y)
            for (int x = 0; x < imageWidth; ++x)
            {
                color = image.GetPixel(x, y);
                if (color.a_ != 0)
                {
                    maxY = y;
                    findok = true;
                    break;
                }
            }

            trimOffsetX = minX;
            trimOffsetY = minY;
            adjustedWidth = maxX - minX + 1;
            adjustedHeight = maxY - minY + 1;

            if (adjustedWidth != imageWidth || adjustedHeight != imageHeight)
            {
                packerInfo.size.x_ = adjustedWidth;
                packerInfo.size.y_ = adjustedHeight;
                packerInfo.frame.x_ -= trimOffsetX;
                packerInfo.frame.y_ -= trimOffsetY;
                packerInfo.framesize.x_ = imageWidth;
                packerInfo.framesize.y_ = imageHeight;
            }
        }
    }

    if (trim)
        URHO3D_LOGINFO("    Trim Sprites.");

    // copy custom hotspots
    for (unsigned i = 0; i < info.packerinfos.Size(); ++i)
    {
        PackerInfo& packerInfo = info.packerinfos[i];

        HashMap<String, Vector2>::Iterator it = info.customhotspots.Find(packerInfo.name);
        if (it != info.customhotspots.End())
        {
            Vector2 hotspot = it->second_;
            packerInfo.hotspot.x_ = Round(hotspot.x_ * (float)packerInfo.size.x_);
            packerInfo.hotspot.y_ = Round(hotspot.y_ * (float)packerInfo.size.y_);
        }
    }

    return true;
}

inline void AddXMLTextureLine(const PackerInfo& packerInfo, XMLElement& root)
{
    XMLElement subTexture = root.CreateChild("SubTexture");
    subTexture.SetString("name", packerInfo.name);
    subTexture.SetInt("x", packerInfo.position.x_ + sOffset.x_);
    subTexture.SetInt("y", packerInfo.position.y_ + sOffset.y_);
    subTexture.SetInt("width", packerInfo.size.x_);
    subTexture.SetInt("height", packerInfo.size.y_);

    if (packerInfo.framesize != IntVector2::ZERO)
    {
        subTexture.SetInt("frameX", packerInfo.frame.x_);
        subTexture.SetInt("frameY", packerInfo.frame.y_);
        subTexture.SetInt("frameWidth", packerInfo.framesize.x_);
        subTexture.SetInt("frameHeight", packerInfo.framesize.y_);
    }

    if (packerInfo.hotspot != IntVector2::ZERO)
    {
        subTexture.SetInt("hotspotx", packerInfo.hotspot.x_);
        subTexture.SetInt("hotspoty", packerInfo.hotspot.y_);
    }
}

void GenerateTextureFile(Context* context, Info& info, int preferedsize, const IntVector2& padding, bool debug)
{
    URHO3D_LOGINFO(" - Generating texture " + info.outputtexturefile + " ...");

    if (info.spritefiles.Size() < 2)
    {
        URHO3D_LOGERROR("   no sprite files to pack.");
        return;
    }

    bool success = false;
    float packedratio = 0.f;
    IntVector2 texturesize;

    // resolve the rects positions
    {
        unsigned neededarea = 0;

        // load rectangles
        PODVector<stbrp_rect> packerRects;
        packerRects.Resize(info.packerinfos.Size());

        for (unsigned i = 0; i < info.packerinfos.Size(); ++i)
        {
            packerRects[i].id = i;
            packerRects[i].x = 0;
            packerRects[i].y = 0;
            packerRects[i].w = info.packerinfos[i].size.x_ + padding.x_;
            packerRects[i].h = info.packerinfos[i].size.y_ + padding.y_;
            neededarea += packerRects[i].w * packerRects[i].h;
        }

        // calculate the minimal area needed (squared size)
        Vector<int> imagesizes;
        if (preferedsize)
            imagesizes.Push(preferedsize);

        for (unsigned w = MIN_TEXTURE_SIZE; w <= MAX_TEXTURE_SIZE; w = w<<1)
        {
            if (w * w > neededarea)
                imagesizes.Push(w);
        }

        // find the rects positions for each image size
        success = false;
        for (int d = 0; d < imagesizes.Size() && !success; d++)
        {
            texturesize.x_ = texturesize.y_ = imagesizes[d];

            // Reset packed infos.
            for (unsigned i = 0; i < packerRects.Size(); ++i)
                packerRects[i].was_packed = 0;

            bool fit = false;
/*
            // try algo stb_rect_pack (the good compromise)
            if (!fit)
                fit = PackRectsSTB(texturesize.x_, texturesize.y_, packerRects);

            // try algo ByRows (the fastest)
            if (!fit)
                fit = PackRectsNaiveRows(texturesize.x_, texturesize.y_, packerRects);
*/
            // try algo ByPixelScan (the best ratio)
            if (!fit)
                fit = PackRectsBLPixels(texturesize.x_, texturesize.y_, packerRects, info.skipfirst);

            if (fit)
            {
                packedratio = (float)neededarea/((float)texturesize.x_ * (float)texturesize.y_) * 100.f;
                success = true;

                // distribute values to packer info
                for (unsigned i = 0; i < info.packerinfos.Size(); ++i)
                {
                    PackerInfo& packerInfo = info.packerinfos[packerRects[i].id];
                    packerInfo.position.x_ = packerRects[i].x;
                    packerInfo.position.y_ = packerRects[i].y;
                }
            }
        }

        if (!success)
        {
            URHO3D_LOGERROR("   Could not allocate for all images.  The max sprite sheet texture size is " + String(MAX_TEXTURE_SIZE) + "x" + String(MAX_TEXTURE_SIZE) + ".");
            return;
        }
    }

    // create the spritesheet image
    {
        Image spriteSheetImage(context);
        spriteSheetImage.SetSize(texturesize.x_, texturesize.y_, 4);
        spriteSheetImage.SetData(0);

        for (unsigned i = 0; i < info.packerinfos.Size(); ++i)
        {
            PackerInfo& packerInfo = info.packerinfos[i];

            File file(context, packerInfo.path);
            Image image(context);
            if (!image.Load(file))
                ErrorExit("   Could not load image " + packerInfo.path + ".");

            for (int y = 0; y < packerInfo.size.y_; ++y)
            {
                for (int x = 0; x < packerInfo.size.x_; ++x)
                {
                    unsigned color = image.GetPixelInt(x - packerInfo.frame.x_, y - packerInfo.frame.y_);
                    spriteSheetImage.SetPixelInt(packerInfo.position.x_ + x + sOffset.x_, packerInfo.position.y_ + y + sOffset.y_, color);
                }
            }
        }

        if (debug)
        {
            URHO3D_LOGINFO("    Drawing debug information.");

            unsigned DEBUG_RECTCOLOR = Color::GREEN.ToUInt();
            for (unsigned i = 0; i < info.packerinfos.Size(); ++i)
            {
                PackerInfo& packerInfo = info.packerinfos[i];
                for (int x = 0; x < packerInfo.size.x_; ++x)
                {
                    spriteSheetImage.SetPixelInt(packerInfo.position.x_ + x + sOffset.x_, packerInfo.position.y_ + sOffset.y_, DEBUG_RECTCOLOR);
                    spriteSheetImage.SetPixelInt(packerInfo.position.x_ + x + sOffset.x_, packerInfo.position.y_ + packerInfo.size.y_ + sOffset.y_, DEBUG_RECTCOLOR);
                }
                for (int y = 0; y < packerInfo.size.y_; ++y)
                {
                    spriteSheetImage.SetPixelInt(packerInfo.position.x_ + sOffset.x_, packerInfo.position.y_ + y, DEBUG_RECTCOLOR);
                    spriteSheetImage.SetPixelInt(packerInfo.position.x_ + packerInfo.size.x_ + sOffset.x_, packerInfo.position.y_ + y + sOffset.y_, DEBUG_RECTCOLOR);
                }
            }

            unsigned DEBUG_FRAMECOLOR = Color::BLUE.ToUInt();
            for (unsigned i = 0; i < info.packerinfos.Size(); ++i)
            {
                PackerInfo& packerInfo = info.packerinfos[i];
                for (int x = 0; x < packerInfo.framesize.x_; ++x)
                {
                    spriteSheetImage.SetPixelInt(packerInfo.position.x_ + x, packerInfo.position.y_, DEBUG_FRAMECOLOR);
                    spriteSheetImage.SetPixelInt(packerInfo.position.x_ + x, packerInfo.position.y_ + packerInfo.framesize.y_, DEBUG_FRAMECOLOR);
                }
                for (int y = 0; y < packerInfo.framesize.y_; ++y)
                {
                    spriteSheetImage.SetPixelInt(packerInfo.position.x_, packerInfo.position.y_ + y, DEBUG_FRAMECOLOR);
                    spriteSheetImage.SetPixelInt(packerInfo.position.x_ + packerInfo.framesize.x_, packerInfo.position.y_ + y, DEBUG_FRAMECOLOR);
                }
            }

            unsigned DEBUG_HOTSPOTCOLOR = Color::RED.ToUInt();
            for (unsigned i = 0; i < info.packerinfos.Size(); ++i)
            {
                PackerInfo& packerInfo = info.packerinfos[i];
                IntVector2 hotspot = packerInfo.position + packerInfo.hotspot;
                for (int x = -4; x <= 4; ++x)
                    if (hotspot.x_ + x + sOffset.x_ >= 0 && hotspot.x_ + x + sOffset.x_ < texturesize.x_)
                        spriteSheetImage.SetPixelInt(hotspot.x_ + x + sOffset.x_, hotspot.y_ + sOffset.y_, DEBUG_HOTSPOTCOLOR);
                for (int y = -4; y <= 4; ++y)
                    if (hotspot.y_ + y + sOffset.y_ >= 0 && hotspot.y_ + y + sOffset.y_ < texturesize.y_)
                        spriteSheetImage.SetPixelInt(hotspot.x_ + sOffset.x_, hotspot.y_ + y + sOffset.y_, DEBUG_HOTSPOTCOLOR);
            }
        }

        URHO3D_LOGINFO("    Saving output image.");
        spriteSheetImage.SavePNG(info.outputgraphicfolder + info.outputtexturefolder + info.outputtexturefile);
    }

    // add empty sprites
    // copy custom non-existing sprites (like "casque" or "swordf" in petit.scml)
    // apply the dpi ratio in this case because the scml are in mdpi only
    if (info.addemptysprites)
    {
        for (HashMap<String, PackerInfo >::ConstIterator it = info.emptysprites.Begin(); it != info.emptysprites.End(); ++it)
        {
            info.packerinfos.Resize(info.packerinfos.Size()+1);
            PackerInfo& packerInfo = info.packerinfos.Back();
            const PackerInfo& p = it->second_;
            packerInfo.name = p.name;
            packerInfo.size.x_ = Round(p.size.x_ * info.dpiratio);
            packerInfo.size.y_ = Round(p.size.y_ * info.dpiratio);
            packerInfo.hotspot.x_ = Round(p.hotspot.x_ * info.dpiratio);
            packerInfo.hotspot.y_ = Round(p.hotspot.y_ * info.dpiratio);
        }
    }

    // add duplicate names like ABI_Grapin = ancre
    if (info.duplicatespritenames.Size())
    {
        for (HashMap<String, Vector<String> >::ConstIterator it = info.duplicatespritenames.Begin(); it != info.duplicatespritenames.End(); ++it)
        {
            String name = it->first_;

            const PackerInfo* ppackerinfo = 0;
            for (Vector<PackerInfo>::ConstIterator jt = info.packerinfos.Begin(); jt != info.packerinfos.End(); ++jt)
            {
                if (jt->name == name)
                {
                    ppackerinfo = &(*jt);
                    break;
                }
            }

            if (!ppackerinfo)
                continue;

            const PackerInfo& p = *ppackerinfo;

            const Vector<String>& duplicatenames = it->second_;
            for (Vector<String>::ConstIterator jt = duplicatenames.Begin(); jt != duplicatenames.End(); ++jt)
            {
                info.packerinfos.Resize(info.packerinfos.Size()+1);
                PackerInfo& packerInfo = info.packerinfos.Back();
                packerInfo.name = *jt;
                packerInfo.position = p.position;
                packerInfo.size = p.size;
            }
        }
    }

    // sort alphabetically for the spritesheet xml file
    info.sortedpackerinfos.Resize(info.packerinfos.Size());
    for (unsigned i=0; i < info.packerinfos.Size(); i++)
        info.sortedpackerinfos[i] = &info.packerinfos[i];
    Sort(info.sortedpackerinfos.Begin(), info.sortedpackerinfos.End(), SortByName);

    // create the main spritesheet xml file
    {
        XMLFile xml(context);
        XMLElement root = xml.CreateRoot("TextureAtlas");
        root.SetAttribute("imagePath", info.outputtexturefolder + info.outputtexturefile);

        for (unsigned i = 0; i < info.sortedpackerinfos.Size(); ++i)
            AddXMLTextureLine(*info.sortedpackerinfos[i], root);

        String spriteSheetFileName = info.outputgraphicfolder + info.outputscmlfolder + ReplaceExtension(info.outputtexturefile, ".xml");
        File spriteSheetFile(context);
        spriteSheetFile.Open(spriteSheetFileName, FILE_WRITE);
        xml.Save(spriteSheetFile);

        URHO3D_LOGINFO("    Saving SpriteSheet xml file.");
    }

    // create the spritesheet xml file for each scml
    for (unsigned j=0; j < info.scmlfiles.Size(); j++)
    {
        if (!info.spritesbyscml[j].Size())
            continue;

        String scmlname = ReplaceExtension(GetFileName(info.scmlfiles[j]), String::EMPTY);

        const Vector<String>& spritenames = info.spritesbyscml[j];
        String spriteSheetFileName = info.outputgraphicfolder + info.outputscmlfolder + ReplaceExtension(scmlname, ".xml");

        XMLFile xml(context);
        XMLElement root = xml.CreateRoot("TextureAtlas");
        root.SetAttribute("imagePath", info.outputtexturefolder + info.outputtexturefile);

        unsigned numsprites = 0;

        // add the default sprite line in first position
        int masterindex = -1;
        HashMap<String, String>::ConstIterator it = info.defaultsprite.Find(scmlname);
        if (it != info.defaultsprite.End())
        {
            String mastersprite = it->second_;
            String alias;
            Vector<String> splitStrs = mastersprite.Split('|');
            if (splitStrs.Size() > 1)
            {
                mastersprite = splitStrs[0];
                alias = splitStrs[1];
            }

            for (unsigned i = 0; i < info.sortedpackerinfos.Size(); ++i)
            {
                PackerInfo& packerInfo = *info.sortedpackerinfos[i];

                if (packerInfo.name == mastersprite)
                {
                    // add alias if needed
                    if (!alias.Empty())
                    {
                        packerInfo.name = alias;
                        AddXMLTextureLine(packerInfo, root);
                        packerInfo.name = mastersprite;
                    }
                    // add the default sprite
                    masterindex = i;
                    AddXMLTextureLine(packerInfo, root);
                    numsprites++;
                    break;
                }
            }
        }

        // Add the XML Sprite Lines
        for (unsigned i = 0; i < info.sortedpackerinfos.Size(); ++i)
        {
            // skip the master sprite already Added
            if (masterindex != -1 && masterindex == i)
                continue;

            PackerInfo& packerInfo = *info.sortedpackerinfos[i];

            // skip sprites that are not included in this scml
            if (!spritenames.Contains(packerInfo.name))
                continue;

            AddXMLTextureLine(packerInfo, root);

            numsprites++;
        }

        if (numsprites)
        {
            File spriteSheetFile(context);
            spriteSheetFile.Open(spriteSheetFileName, FILE_WRITE);
            xml.Save(spriteSheetFile);
        }
    }
    URHO3D_LOGINFO("    Saving All Scml SpriteSheet xml files.");

    // create the custom sritesheet xml files
    for (HashMap<String, Vector<String> >::ConstIterator it = info.customspritesheets.Begin(); it != info.customspritesheets.End(); ++it)
    {
        String spriteSheetFileName = info.outputgraphicfolder + info.outputscmlfolder + ReplaceExtension(it->first_, ".xml");
        const Vector<String>& spritenames = it->second_;

        XMLFile xml(context);
        XMLElement root = xml.CreateRoot("TextureAtlas");
        root.SetAttribute("imagePath", info.outputtexturefolder + info.outputtexturefile);

        unsigned numsprites = 0;

        // Add the XML Sprite Lines
        for (unsigned i = 0; i < info.sortedpackerinfos.Size(); ++i)
        {
            PackerInfo& packerInfo = *info.sortedpackerinfos[i];

            // skip sprites that are not included in this scml
            if (!spritenames.Contains(packerInfo.name))
                continue;

            AddXMLTextureLine(packerInfo, root);

            numsprites++;
        }

        if (numsprites)
        {
            File spriteSheetFile(context);
            spriteSheetFile.Open(spriteSheetFileName, FILE_WRITE);
            xml.Save(spriteSheetFile);
        }
    }
    URHO3D_LOGINFO("    Saving All Custom SpriteSheet xml files.");

    // metrics
    URHO3D_LOGINFOF("    Texture Size = %dx%d ... PackedRatio = %F%% ... Success !", texturesize.x_, texturesize.y_, packedratio);
}

void GenerateFiles(Context* context, Info& info, const IntVector2& padding, bool trim, bool debug, bool usedpiratio=true)
{
    Vector<String> dpi;
    Vector<int> texturesize;
    Vector<float> dpiratio;

    dpi.Push("ldpi/");
    dpi.Push("mdpi/");
    dpi.Push("hdpi/");
//    texturesize.Push(768);
//    texturesize.Push(1024);
//    texturesize.Push(1536);
    texturesize.Push(0);
    texturesize.Push(0);
    texturesize.Push(0);

    if (usedpiratio)
    {
        dpiratio.Push(0.75f);
        dpiratio.Push(1.f);
        dpiratio.Push(1.5f);
    }
    else
    {
        dpiratio.Push(1.f);
        dpiratio.Push(1.f);
        dpiratio.Push(1.f);
    }

    String outgraphicsfolder = "Graphics/";
    if (info.basepngfolder == "UI/")
        outgraphicsfolder = "UI/Graphics/";

    for (Vector<String>::ConstIterator it = dpi.Begin(); it != dpi.End(); ++it)
    {
        int index = it-dpi.Begin();

        info.inputspritefolder       = "./Input/" + info.basepngfolder + (*it);
        info.outputgraphicfolder     = "./Output/Data/" + outgraphicsfolder + (*it);

        info.dpiratio = dpiratio[index];

        URHO3D_LOGINFO("Generate " + info.outputtexturefile + " ...");

        if (ExtractSpriteFilenames(context, info, trim))
            GenerateTextureFile(context, info, texturesize[index], padding, debug);
    }
}

void Help()
{
    ErrorExit("Usage: SpritePacker -options <input file> <input file> <output png file>\n"
        "\n"
        "Options:\n"
        "-h Shows this help message.\n"
        "-px Adds x pixels of padding per image to width.\n"
        "-py Adds y pixels of padding per image to height.\n"
        "-ox Adds x pixels to the horizontal position per image.\n"
        "-oy Adds y pixels to the horizontal position per image.\n"
        "-trim Trims excess transparent space from individual images offsets by frame size.\n"
        "-debug Draws allocation boxes on sprite.\n");
}


int main(int argc, char** argv)
{
    SharedPtr<Context> context(new Context());
    context->RegisterSubsystem(new FileSystem(context));
    context->RegisterSubsystem(new ResourceCache(context));
    context->RegisterSubsystem(new Log(context));

    Vector<String> arguments;

#ifdef WIN32
    arguments = ParseArguments(GetCommandLineW());
#else
    arguments = ParseArguments(argc, argv);
#endif

    IntVector2 padding(IntVector2::ONE);

    bool help  = false;
    bool trim  = false;
    bool debug = false;

    while (arguments.Size() > 0)
    {
        String arg = arguments[0];
        arguments.Erase(0);

        if (arg.Empty())
            continue;

        if (arg.StartsWith("-"))
        {
            if      (arg == "-px")     { padding.x_ = ToUInt(arguments[0]); arguments.Erase(0); }
            else if (arg == "-py")     { padding.y_ = ToUInt(arguments[0]); arguments.Erase(0); }
            else if (arg == "-ox")     { sOffset.x_ = ToUInt(arguments[0]); arguments.Erase(0); }
            else if (arg == "-oy")     { sOffset.y_ = ToUInt(arguments[0]); arguments.Erase(0); }
            else if (arg == "-trim")   { trim = true; }
            else if (arg == "-h")      { help = true; break; }
            else if (arg == "-debug")  { debug = true; }
        }
    }

    if (help)
    {
        Help();
        return 0;
    }

    // Set the max offset equal to padding to prevent images from going out of bounds
    sOffset.x_ = Min(sOffset.x_, padding.x_);
    sOffset.y_ = Min(sOffset.y_, padding.y_);

    String inputentityfolder   = "./Input/Entity/";
    String inputscmlfolder     = "./Input/SCML/";
    String inputbasepngfolder  = "PNG/";
    String outputscmlfolder    = "2D/";
    String outputtexturefolder = "Textures/Actors/";

    {
        Info info;
        info.basepngfolder       = inputbasepngfolder;
        info.inputentityfolder   = inputentityfolder;
        info.inputscmlfolder     = inputscmlfolder;
        info.outputscmlfolder    = outputscmlfolder;
        info.outputtexturefolder = outputtexturefolder;
        info.addemptysprites     = true;
        info.skipfirst           = false;

        info.outputtexturefile   = "spritesheet1.png";

        info.entityfiles.Push("ailedark.xml");
        info.entityfiles.Push("armorlizard.xml");
        info.entityfiles.Push("armorpetit2.xml");
        info.entityfiles.Push("armorskeleton.xml");
        info.entityfiles.Push("bullet.xml");
        info.entityfiles.Push("helmet-black.xml");
        info.entityfiles.Push("helmet-brown.xml");
        info.entityfiles.Push("helmet-green.xml");
        info.entityfiles.Push("helmet-lizard1.xml");
        info.entityfiles.Push("helmet-lizard2.xml");
        info.entityfiles.Push("helmet-lizard3.xml");
        info.entityfiles.Push("helmet-white.xml");
        info.entityfiles.Push("weapons-axe01.xml");
        info.entityfiles.Push("weapons-bomb.xml");
        info.entityfiles.Push("weapons-epeearete.xml");
        info.entityfiles.Push("weapons-grapin.xml");
        info.entityfiles.Push("weapons-hammer01.xml");
        info.entityfiles.Push("weapons-hugesword.xml");
        info.entityfiles.Push("weapons-katanasword.xml");
        info.entityfiles.Push("weapons-lame.xml");
        info.entityfiles.Push("weapons-largesword.xml");
        info.entityfiles.Push("weapons-largeswordflame.xml");
        info.entityfiles.Push("weapons-longsword.xml");
        info.entityfiles.Push("weapons-pirategun.xml");
        info.entityfiles.Push("weapons-shortsword.xml");
        info.entityfiles.Push("torche.xml");
        info.entityfiles.Push("filet.xml");
        info.entityfiles.Push("arrosoir.xml");
        info.entityfiles.Push("pelle.xml");
        info.entityfiles.Push("avatar-petit.xml");
        info.entityfiles.Push("avatar-lizard.xml");
        info.entityfiles.Push("avatar-newskel.xml");

        info.scmlfiles.Push("effects_flame.scml");
        info.scmlfiles.Push("grapin.scml");

        info.spritefiles.Push("corde.png");
        info.spritefiles.Push("cordel.png");

        info.defaultsprite["armorlizard"] = "armure_01";
        info.defaultsprite["armorpetit2"] = "armure_01_b";
        info.defaultsprite["armorskeleton"] = "armureskel01";
        info.defaultsprite["epee-arete"] = "epee-arete";
        info.defaultsprite["epee-flame"] = "largesword";
        info.defaultsprite["bombe"] = "bombe_01";
        info.defaultsprite["filet"] = "manche_filet|filet";
        info.duplicatespritenames["manche_filet"].Push("filet");
//        info.defaultsprite["torche"] = "torche";

        info.customhotspots["bluehelmet"]      = Vector2((float)(20 + 59) / 119.f, (float)(18 + 55) / 110.f);
        info.customhotspots["brownhelmet"]     = Vector2((float)(20 + 59) / 119.f, (float)(18 + 55) / 110.f);
        info.customhotspots["greenhelmet"]     = Vector2((float)(20 + 59) / 119.f, (float)(18 + 55) / 110.f);
        info.customhotspots["whitehelmet"]     = Vector2((float)(20 + 59) / 119.f, (float)(18 + 55) / 110.f);
        info.customhotspots["lizardhelmet_01"] = Vector2((float)(39 + 72) / 145.f, (float)(2 + 49) / 99.f);
        info.customhotspots["lizardhelmet_02"] = Vector2((float)(39 + 72) / 145.f, (float)(2 + 49) / 99.f);
        info.customhotspots["lizardhelmet_03"] = Vector2((float)(39 + 72) / 145.f, (float)(2 + 49) / 99.f);

        info.customhotspots["smallsword"]      = Vector2((-70 + 120) / 240.f, (-2 + 50) / 100.f);
        info.customhotspots["longsword"]       = Vector2((-70 + 120) / 240.f, (-2 + 50) / 100.f);
        info.customhotspots["largesword"]      = Vector2((-70 + 120) / 240.f, (-2 + 50) / 100.f);
        info.customhotspots["hugesword"]       = Vector2((-70 + 120) / 240.f, (-2 + 50) / 100.f);
        info.customhotspots["katana01"]        = Vector2((-111 + 172) / 345.f, 22 / 45.f);
        info.customhotspots["hammer01"]        = Vector2((-100 + 156) / 312.f, 67 / 134.f);
        info.customhotspots["axe01"]           = Vector2((-80 + 136) / 273.f, 75 / 151.f);
        info.customhotspots["pistolet01"]      = Vector2((2 + 40) / 81.f, (-43 + 56) / 113.f);

        info.customhotspots["ancre"]      	   = Vector2(0.5f, 0.92f);
        info.customhotspots["pelle"]      	   = Vector2(0.2f, 0.5f);
        info.customhotspots["arrosoir"]    	   = Vector2(0.1f, 0.75f);

        GenerateFiles(context, info, padding, trim, debug);
    }
    {
        Info info;
        info.basepngfolder       = inputbasepngfolder;
        info.inputentityfolder   = inputentityfolder;
        info.inputscmlfolder     = inputscmlfolder;
        info.outputscmlfolder    = outputscmlfolder;
        info.outputtexturefolder = outputtexturefolder;
        info.addemptysprites     = true;
        info.skipfirst           = false;

        info.outputtexturefile   = "spritesheet2.png";

        info.entityfiles.Push("avatar-bat.xml");
        info.entityfiles.Push("avatar-bitroll.xml");
        info.entityfiles.Push("avatar-buros.xml");
        info.entityfiles.Push("avatar-kigrat.xml");
        info.entityfiles.Push("avatar-loup.xml");
        info.entityfiles.Push("avatar-ours.xml");
        info.entityfiles.Push("avatar-petite.xml");
        info.entityfiles.Push("avatar-raignee.xml");
        info.entityfiles.Push("avatar-shuktuk.xml");
        info.entityfiles.Push("avatar-sorceress.xml");
        info.entityfiles.Push("avatar-livache.xml");
        info.entityfiles.Push("avatar-vampire.xml");
        info.entityfiles.Push("bouledefeu.xml");
        info.entityfiles.Push("milkblast2.xml");

        info.scmlfiles.Push("lance.scml");
        info.scmlfiles.Push("rayon_hypno.scml");
        info.scmlfiles.Push("rayon_transfo.scml");

        info.defaultsprite["lance"] = "ki_lance";

        GenerateFiles(context, info, padding, trim, debug);
    }
    {
        Info info;
        info.basepngfolder       = inputbasepngfolder;
        info.inputentityfolder   = inputentityfolder;
        info.inputscmlfolder     = inputscmlfolder;
        info.outputscmlfolder    = outputscmlfolder;
        info.outputtexturefolder = outputtexturefolder;
        info.addemptysprites     = true;
        info.skipfirst           = false;

        info.outputtexturefile   = "spritesheet3.png";

        info.entityfiles.Push("avatar-chapanze.xml");
        info.entityfiles.Push("avatar-darkren.xml");
        info.entityfiles.Push("avatar-elsarion.xml");
        info.entityfiles.Push("avatar-eredot.xml");
        info.entityfiles.Push("avatar-junkelspil.xml");
        info.entityfiles.Push("karotos.xml");
        info.entityfiles.Push("goblin.xml");
        info.entityfiles.Push("avatar-mirubil.xml");
        info.entityfiles.Push("oeufmirubil.xml");
		info.entityfiles.Push("avatar-spectii.xml");
		info.entityfiles.Push("avatar-podomorphe.xml");
		info.entityfiles.Push("poisonviolet.xml");

        info.scmlfiles.Push("griffes.scml");

        GenerateFiles(context, info, padding, trim, debug);
    }
    {
        Info info;
        info.basepngfolder       = inputbasepngfolder;
        info.inputentityfolder   = inputentityfolder;
        info.inputscmlfolder     = inputscmlfolder;
        info.outputscmlfolder    = outputscmlfolder;
        info.outputtexturefolder = outputtexturefolder;
        info.addemptysprites     = true;
        info.skipfirst           = false;

        info.outputtexturefile   = "spritesheet4.png";

        info.entityfiles.Push("avatar-redlord.xml");
        info.entityfiles.Push("avatar-rockgolem.xml");
        info.entityfiles.Push("avatar-rockgolem-flore.xml");

        GenerateFiles(context, info, padding, trim, debug);
    }
    {
        Info info;
        info.basepngfolder       = inputbasepngfolder;
        info.inputentityfolder   = inputentityfolder;
        info.inputscmlfolder     = inputscmlfolder;
        info.outputscmlfolder    = outputscmlfolder;
        info.outputtexturefolder = outputtexturefolder;
        info.addemptysprites     = false;
        info.skipfirst           = false;

        info.outputtexturefile   = "collectable1.png";

        info.entityfiles.Push("coffre.xml");
        info.entityfiles.Push("money.xml");
        info.entityfiles.Push("potion.xml");
        info.entityfiles.Push("potionregen.xml");
        info.entityfiles.Push("eliegorseed.xml");

        info.spritefiles.Push("scrapbomb1.png");
        info.spritefiles.Push("scrapbomb2.png");
        info.spritefiles.Push("scrapbomb3.png");
        info.spritefiles.Push("scrapbomb4.png");
        info.customspritesheets["scrapsbomb"].Push("scrapbomb1");
        info.customspritesheets["scrapsbomb"].Push("scrapbomb2");
        info.customspritesheets["scrapsbomb"].Push("scrapbomb3");
        info.customspritesheets["scrapsbomb"].Push("scrapbomb4");

        info.spritefiles.Push("scrapbone1.png");
        info.spritefiles.Push("scrapbone2.png");
        info.spritefiles.Push("scrapbone3.png");
        info.spritefiles.Push("scrapbone4.png");
        info.spritefiles.Push("scrapbone5.png");
        info.customspritesheets["scrapsbone"].Push("scrapbone1");
        info.customspritesheets["scrapsbone"].Push("scrapbone2");
        info.customspritesheets["scrapsbone"].Push("scrapbone3");
        info.customspritesheets["scrapsbone"].Push("scrapbone4");
        info.customspritesheets["scrapsbone"].Push("scrapbone5");

        info.spritefiles.Push("scrapemeat1.png");
        info.spritefiles.Push("scrapemeat2.png");
        info.spritefiles.Push("scrapemeat3.png");
        info.spritefiles.Push("scrapemeat4.png");
        info.spritefiles.Push("scrapemeat5.png");
        info.customspritesheets["scrapselsarionmeat"].Push("scrapemeat1");
        info.customspritesheets["scrapselsarionmeat"].Push("scrapemeat2");
        info.customspritesheets["scrapselsarionmeat"].Push("scrapemeat3");
        info.customspritesheets["scrapselsarionmeat"].Push("scrapemeat4");
        info.customspritesheets["scrapselsarionmeat"].Push("scrapemeat5");

        info.spritefiles.Push("scrapwallwhite1.png");
        info.spritefiles.Push("scrapwallwhite2.png");
        info.spritefiles.Push("scrapwallwhite3.png");
        info.spritefiles.Push("scrapwallwhite4.png");
        info.spritefiles.Push("scrapwallwhite5.png");
        info.customspritesheets["scrapsterrain0"].Push("scrapwallwhite1");
        info.customspritesheets["scrapsterrain0"].Push("scrapwallwhite2");
        info.customspritesheets["scrapsterrain0"].Push("scrapwallwhite3");
        info.customspritesheets["scrapsterrain0"].Push("scrapwallwhite4");
        info.customspritesheets["scrapsterrain0"].Push("scrapwallwhite5");
        info.customspritesheets["scrapsterrain1"].Push("scrapwallwhite1");
        info.customspritesheets["scrapsterrain1"].Push("scrapwallwhite2");
        info.customspritesheets["scrapsterrain1"].Push("scrapwallwhite3");
        info.customspritesheets["scrapsterrain1"].Push("scrapwallwhite4");
        info.customspritesheets["scrapsterrain1"].Push("scrapwallwhite5");

        info.spritefiles.Push("scrapgrass1.png");
        info.spritefiles.Push("scrapgrass2.png");
        info.spritefiles.Push("scrapgrass3.png");
        info.spritefiles.Push("scrapgrass4.png");
        info.customspritesheets["scrapsterrain2"].Push("scrapgrass1");
        info.customspritesheets["scrapsterrain2"].Push("scrapgrass2");
        info.customspritesheets["scrapsterrain2"].Push("scrapgrass3");
        info.customspritesheets["scrapsterrain2"].Push("scrapgrass4");

        info.spritefiles.Push("scrapwallblack1.png");
        info.spritefiles.Push("scrapwallblack2.png");
        info.spritefiles.Push("scrapwallblack3.png");
        info.spritefiles.Push("scrapwallblack4.png");
        info.spritefiles.Push("scrapwallblack5.png");
        info.customspritesheets["scrapsterrain3"].Push("scrapwallblack1");
        info.customspritesheets["scrapsterrain3"].Push("scrapwallblack2");
        info.customspritesheets["scrapsterrain3"].Push("scrapwallblack3");
        info.customspritesheets["scrapsterrain3"].Push("scrapwallblack4");
        info.customspritesheets["scrapsterrain3"].Push("scrapwallblack5");

        info.spritefiles.Push("scrapbrick1.png");
        info.spritefiles.Push("scrapbrick2.png");
        info.spritefiles.Push("scrapbrick3.png");
        info.spritefiles.Push("scrapbrick4.png");
        info.spritefiles.Push("scrapbrick5.png");
        info.customspritesheets["scrapsterrain4"].Push("scrapbrick1");
        info.customspritesheets["scrapsterrain4"].Push("scrapbrick2");
        info.customspritesheets["scrapsterrain4"].Push("scrapbrick3");
        info.customspritesheets["scrapsterrain4"].Push("scrapbrick4");
        info.customspritesheets["scrapsterrain4"].Push("scrapbrick5");

        info.defaultsprite["money"] = "or_pepite04";
//        masterspritebyscml["potion"] = ;
//        masterspritebyscml["potionregen"] = ;

        GenerateFiles(context, info, padding, trim, debug);
    }

    inputentityfolder   = "./Input/Furniture/";
    outputtexturefolder = "Textures/Grounds/";

    // vegetation
    {
        Info info;
        info.basepngfolder       = inputbasepngfolder;
        info.inputentityfolder   = inputentityfolder;
        info.inputscmlfolder     = inputscmlfolder;
        info.outputscmlfolder    = outputscmlfolder;
        info.outputtexturefolder = outputtexturefolder;
        info.addemptysprites     = false;
        info.skipfirst           = false;

        info.outputtexturefile   = "plants.png";

        info.entityfiles.Push("plant01.xml");
        info.entityfiles.Push("plant02.xml");

        info.spritefiles.Push("branche.png");

        GenerateFiles(context, info, padding, trim, debug);
    }
    // vegetation static
    {
        Info info;
        info.basepngfolder       = inputbasepngfolder;
        info.inputentityfolder   = inputentityfolder;
        info.inputscmlfolder     = inputscmlfolder;
        info.outputscmlfolder    = outputscmlfolder;
        info.outputtexturefolder = outputtexturefolder;
        info.addemptysprites     = false;
        info.skipfirst           = false;

        info.outputtexturefile   = "plantsstaticnew.png";

        info.entityfiles.Push("eliegorflower_static.xml");
        info.entityfiles.Push("plant01_static.xml");
        info.entityfiles.Push("plant02_static.xml");
        info.entityfiles.Push("plant03_static.xml");
        info.entityfiles.Push("plant04_static.xml");
        info.entityfiles.Push("plant05_static.xml");
        info.entityfiles.Push("plant06_static.xml");
        info.entityfiles.Push("plant07_static.xml");
        info.entityfiles.Push("plant08_static.xml");
        info.entityfiles.Push("rockpic01_static.xml");
        info.entityfiles.Push("rockpic02_static.xml");
        info.entityfiles.Push("rockpic03_static.xml");
        info.entityfiles.Push("ronce01_static.xml");
        info.entityfiles.Push("tronc01_static.xml");
        info.entityfiles.Push("tronc02_static.xml");
        info.entityfiles.Push("woodpic01_static.xml");
        info.entityfiles.Push("woodpic02_static.xml");
        info.entityfiles.Push("woodpic03_static.xml");
        info.entityfiles.Push("escalier.xml");

		info.spritefiles.Push("planche1.png");
		info.spritefiles.Push("planche2.png");
        info.spritefiles.Push("cordage1.png");
        info.spritefiles.Push("cordage2.png");
        info.spritefiles.Push("cordage3.png");
		info.spritefiles.Push("cordage4.png");

        info.customhotspots["arbre01"]       = Vector2(0.5f, (float)(220 + 264) / 528.f);
        info.customhotspots["arbre02"]       = Vector2(0.5f, (float)(185 + 222) / 444.f);
        info.customhotspots["champi01"]      = Vector2(0.5f, (float)(50 + 74) / 148.f);
        info.customhotspots["champi02"]      = Vector2(0.5f, (float)(50 + 56) / 112.f);
        info.customhotspots["champi03"]      = Vector2(0.5f, (float)(50 + 65) / 131.f);
        info.customhotspots["champi04"]      = Vector2(0.5f, (float)(50 + 49) / 98.f);
        info.customhotspots["eliegorflower"] = Vector2(0.5f, (float)(110 + 109) / 219.f);
        info.customhotspots["herbe01"]       = Vector2(0.5f, (50 + 95) / 190.f);
        info.customhotspots["herbe02"]       = Vector2(0.5f, (50 + 58) / 116.f);
        info.customhotspots["rockpic1"]      = Vector2(0.5f, (60 + 65) / 130.f);
        info.customhotspots["rockpic2"]      = Vector2(0.5f, (65 + 109) / 219.f);
        info.customhotspots["rockpic3"]      = Vector2(0.5f, (70 + 72) / 144.f);
        info.customhotspots["ronce1"]        = Vector2(0.5f, (80 + 85) / 171.f);
        info.customhotspots["tronc1"]        = Vector2(0.5f, (120 + 119) / 239.f);
        info.customhotspots["tronc2"]        = Vector2(0.5f, (120 + 130) / 259.f);
        info.customhotspots["woodpic1"]      = Vector2(0.5f, (65 + 62) / 125.f);
        info.customhotspots["woodpic2"]      = Vector2(0.5f, (75 + 72) / 144.f);
        info.customhotspots["woodpic3"]      = Vector2(0.5f, (50 + 45) / 91.f);

        GenerateFiles(context, info, padding, trim, debug);
    }
    // furnitures
    {
        Info info;
        info.basepngfolder       = inputbasepngfolder;
        info.inputentityfolder   = inputentityfolder;
        info.inputscmlfolder     = inputscmlfolder;
        info.outputscmlfolder    = outputscmlfolder;
        info.outputtexturefolder = outputtexturefolder;
        info.addemptysprites     = false;
        info.skipfirst           = false;

        info.outputtexturefile   = "furnituresdung01.png";

        info.entityfiles.Push("armoire.xml");
        info.entityfiles.Push("bougie.xml");
        info.entityfiles.Push("cheminee.xml");
        info.entityfiles.Push("chemineeallumee.xml");
        info.entityfiles.Push("enclume.xml");
        info.entityfiles.Push("feudecamp.xml");
        info.entityfiles.Push("marmite.xml");
        info.entityfiles.Push("portal.xml");
        info.entityfiles.Push("porte.xml");
        info.entityfiles.Push("sanctuary.xml");
        info.entityfiles.Push("stelerip.xml");
        info.entityfiles.Push("table.xml");
        info.entityfiles.Push("tablegarnie.xml");
        info.entityfiles.Push("tenture.xml");

        info.spritefiles.Push("halo_portal.png");

        info.spritefiles.Push("lustrebase.png");
		info.spritefiles.Push("hook01.png");
        info.spritefiles.Push("maillon1.png");
        info.spritefiles.Push("bougie2.png");

        GenerateFiles(context, info, padding, trim, debug);
    }

    inputbasepngfolder  = "UI/";
    outputtexturefolder = "Textures/UI/";
    outputscmlfolder    = "UI/";
//    sOffset.x_ = 0;
//    sOffset.y_ = 0;
//    padding = IntVector2::ZERO;
    sOffset.x_ = 1;
    sOffset.y_ = 1;
    padding = IntVector2(1,1);

    // UI Game Equipment
    {
        Info info;
        info.basepngfolder       = inputbasepngfolder;
        info.inputentityfolder   = inputentityfolder;
        info.inputscmlfolder     = inputscmlfolder;
        info.outputscmlfolder    = outputscmlfolder;
        info.outputtexturefolder = outputtexturefolder;
        info.addemptysprites     = false;

        // force the first sprite to be at 0,0 : here the font !
        info.skipfirst           = true;

        info.outputtexturefile   = "game_equipment.png";

        info.spritefiles.Push("0_font.png");

        info.spritefiles.Push("ArmorLizard.png");
        info.spritefiles.Push("ArmorPetit.png");
        info.spritefiles.Push("ArmorSkeleton.png");

        info.spritefiles.Push("Elsarion.png");
        info.spritefiles.Push("GOT_Bat.png");
        info.spritefiles.Push("GOT_Bitroll.png");
        info.spritefiles.Push("GOT_Buros.png");
        info.spritefiles.Push("GOT_Chapanze.png");
        info.spritefiles.Push("GOT_Darkren.png");
        info.spritefiles.Push("GOT_Goblin.png");
        info.spritefiles.Push("GOT_JunkelSpil.png");
        info.spritefiles.Push("GOT_Karotos.png");
        info.spritefiles.Push("GOT_Kigrat.png");
        info.spritefiles.Push("GOT_Lizard.png");
        info.spritefiles.Push("GOT_Loup.png");
        info.spritefiles.Push("GOT_Mirubil.png");
        info.spritefiles.Push("GOT_Ours.png");
        info.spritefiles.Push("GOT_Petit.png");
        info.spritefiles.Push("GOT_Petite_1.png");
        info.spritefiles.Push("GOT_Petite_2.png");
        info.spritefiles.Push("GOT_Raignee.png");
        info.spritefiles.Push("GOT_RedLord.png");
        info.spritefiles.Push("GOT_RockGolem.png");
        info.spritefiles.Push("GOT_ShukTuk.png");
        info.spritefiles.Push("GOT_Skeleton.png");
        info.spritefiles.Push("GOT_Sorceress.png");
		info.spritefiles.Push("GOT_Vampire.png");
        info.spritefiles.Push("GOT_Podomorphe.png");
		info.spritefiles.Push("GOT_Spectii.png");
		info.spritefiles.Push("GOT_Fantomette.png");
        info.spritefiles.Push("GOT_Livache.png");

        info.spritefiles.Push("UISeparator.png");
        info.spritefiles.Push("UIWindowSquare.png");
        info.spritefiles.Push("UIWindowEmpty.png");
        info.spritefiles.Push("abilityframe.png");

        info.spritefiles.Push("cursor.png");
        info.spritefiles.Push("cursorB.png");
        info.spritefiles.Push("cursorBL.png");
        info.spritefiles.Push("cursorBR.png");
        info.spritefiles.Push("cursorL.png");
        info.spritefiles.Push("cursorR.png");
        info.spritefiles.Push("cursorT.png");
        info.spritefiles.Push("cursorTL.png");
        info.spritefiles.Push("cursorTR.png");
        info.spritefiles.Push("cursorZP.png");
        info.spritefiles.Push("cursorZM.png");
        info.spritefiles.Push("cursorTP.png");

        info.spritefiles.Push("ailed.png");
        info.spritefiles.Push("ancre.png");
        info.spritefiles.Push("axe01.png");
        info.spritefiles.Push("bag.png");
        info.spritefiles.Push("bagicon_off.png");
        info.spritefiles.Push("bagicon_on.png");
        info.spritefiles.Push("bandeauoeil01.png");
        info.spritefiles.Push("blackcape.png");
        info.spritefiles.Push("bluehelmet.png");
        info.spritefiles.Push("bomb.png");
        info.spritefiles.Push("brownhelmet.png");
        info.spritefiles.Push("ceinture01.png");
        info.spritefiles.Push("crafticon_off.png");
        info.spritefiles.Push("crafticon_on.png");
        info.spritefiles.Push("crochet01.png");

        info.spritefiles.Push("effect4.png");
        info.spritefiles.Push("effect5.png");
        info.spritefiles.Push("effectx.png");
        info.spritefiles.Push("eliegorseed.png");
        info.spritefiles.Push("els_elsachunk1.png");
        info.spritefiles.Push("els_elsachunk2.png");
        info.spritefiles.Push("els_elsachunk3.png");
        info.spritefiles.Push("els_elsachunk4.png");
        info.spritefiles.Push("els_elsachunk5.png");
        info.spritefiles.Push("enclume.png");
        info.spritefiles.Push("epee-arete.png");
        info.spritefiles.Push("equipicon_off.png");
        info.spritefiles.Push("equipicon_on.png");
        info.spritefiles.Push("greenhelmet.png");
        info.spritefiles.Push("hammer01.png");
        info.spritefiles.Push("filet.png");
        info.spritefiles.Push("pelle.png");
        info.spritefiles.Push("arrosoir.png");
        info.spritefiles.Push("healthbar.png");
        info.spritefiles.Push("hugesword.png");
        info.spritefiles.Push("icon_close_off.png");
        info.spritefiles.Push("icon_close_on.png");
        info.spritefiles.Push("jambebois01.png");
        info.spritefiles.Push("katana01.png");
        info.spritefiles.Push("lame.png");
        info.spritefiles.Push("lance.png");
        info.spritefiles.Push("largesword.png");
        info.spritefiles.Push("lifeicon.png");
        info.spritefiles.Push("lizardhelmet_01.png");
        info.spritefiles.Push("lizardhelmet_02.png");
        info.spritefiles.Push("lizardhelmet_03.png");
        info.spritefiles.Push("longsword.png");
        info.spritefiles.Push("manabar.png");
        info.spritefiles.Push("mapicon_off.png");
        info.spritefiles.Push("mapicon_on.png");
        info.spritefiles.Push("markcheck0.png");
        info.spritefiles.Push("markcheck1.png");
        info.spritefiles.Push("markcheck2.png");
        info.spritefiles.Push("marmite.png");
        info.spritefiles.Push("menuicon_off.png");
        info.spritefiles.Push("menuicon_on.png");
        info.spritefiles.Push("money.png");
        info.spritefiles.Push("oldsword.png");
        info.spritefiles.Push("pistolet01.png");
        info.spritefiles.Push("point.png");
        info.spritefiles.Push("potion_rose.png");
        info.spritefiles.Push("potion_verte.png");
        info.spritefiles.Push("questicon_off.png");
        info.spritefiles.Push("questicon_on.png");
        info.spritefiles.Push("redshield1.png");
        info.spritefiles.Push("skelavbrasd01.png");
        info.spritefiles.Push("skelavbrasg01.png");
        info.spritefiles.Push("skelbassin01.png");
        info.spritefiles.Push("skelbrasd01.png");
        info.spritefiles.Push("skelbrasg01.png");
        info.spritefiles.Push("skelcrane01.png");
        info.spritefiles.Push("skelcrane02.png");
        info.spritefiles.Push("skelcrane03.png");
        info.spritefiles.Push("skeljambed01.png");
        info.spritefiles.Push("skeljambeg01.png");
        info.spritefiles.Push("skelmachoire01.png");
        info.spritefiles.Push("skelmachoire02.png");
        info.spritefiles.Push("skelmaind01.png");
        info.spritefiles.Push("skelmaing01.png");
        info.spritefiles.Push("skelpiedd01.png");
        info.spritefiles.Push("skelpiedg01.png");
        info.spritefiles.Push("skeltibiad01.png");
        info.spritefiles.Push("skeltibiag01.png");
        info.spritefiles.Push("skeltorse01.png");
        info.spritefiles.Push("slot.png");
        info.spritefiles.Push("slot120.png");
        info.spritefiles.Push("slot40.png");
        info.spritefiles.Push("slotarrowl_off.png");
        info.spritefiles.Push("slotarrowl_on.png");
        info.spritefiles.Push("slotarrowr_off.png");
        info.spritefiles.Push("slotarrowr_on.png");
        info.spritefiles.Push("slothalo.png");
        info.spritefiles.Push("smallsword.png");
        info.spritefiles.Push("statebar.png");
        info.spritefiles.Push("torche.png");
        info.spritefiles.Push("tricorne01.png");
        info.spritefiles.Push("whitehelmet.png");
        info.spritefiles.Push("windowframe1.png");

        info.duplicatespritenames["ArmorLizard"].Push("armure_01");
        info.duplicatespritenames["ArmorPetit"].Push("armure_01_b");
        info.duplicatespritenames["ArmorSkeleton"].Push("armureskel01");
        info.duplicatespritenames["marmite"].Push("Marmite");

        info.duplicatespritenames["bag"].Push("or_pepite04");

        info.duplicatespritenames["ancre"].Push("ABI_Grapin");
        info.duplicatespritenames["effect4"].Push("ABI_CrossWalls");
        info.duplicatespritenames["effect4"].Push("ABI_WallBreaker");
        info.duplicatespritenames["effect4"].Push("ABI_WallBuilder");
        info.duplicatespritenames["lame"].Push("ABI_Shooter");
        info.duplicatespritenames["lame"].Push("Lame");
        info.duplicatespritenames["pistolet01"].Push("ABI_AnimShooter");
        info.duplicatespritenames["pistolet01"].Push("Bullet");
        info.duplicatespritenames["lance"].Push("Lance");
        info.duplicatespritenames["bomb"].Push("ABIBomb");
        info.duplicatespritenames["ailed"].Push("ABI_Fly");

        info.duplicatespritenames["blackcape"].Push("Cape1");
		info.duplicatespritenames["bandeauoeil01"].Push("BlindFold1");
		info.duplicatespritenames["ceinture01"].Push("Belt1");

        GenerateFiles(context, info, padding, trim, debug);
    }

    // Game Dialogue
    inputscmlfolder     = "./Input/SCML/";
    inputbasepngfolder  = "PNG/";
    outputscmlfolder    = "UI/";
    outputtexturefolder = "Textures/UI/";
    {
        Info info;
        info.basepngfolder       = inputbasepngfolder;
        info.inputentityfolder   = inputentityfolder;
        info.inputscmlfolder     = inputscmlfolder;
        info.outputscmlfolder    = outputscmlfolder;
        info.outputtexturefolder = outputtexturefolder;
        info.addemptysprites     = false;
        info.skipfirst           = false;

        info.outputtexturefile   = "game_dialog.png";

        info.scmlfiles.Push("dialogmark.scml");

        info.spritefiles.Push("actordialogframesmall.png");
        info.spritefiles.Push("actordialogframebig.png");
        info.spritefiles.Push("actordialogframemedium.png");
        info.spritefiles.Push("actordialogrequest.png");

        info.customspritesheets["dialogmark"].Push("actorinteract");

        GenerateFiles(context, info, padding, trim, debug, false);
    }

    // TODO : Ground Tiles
    // Pack And Generate TileSheets
//    basepngfolder       = "./Input/Tiles/";
//    outputtexturefolder = "Textures/Grounds/";

    return 0;
}


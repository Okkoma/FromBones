#pragma once

#ifdef EDITORMODE1

namespace Urho3D
{
class Script;
class ScriptFile;
}

class asIScriptEngine;

using namespace Urho3D;


void RegisterFromBonesAPI(asIScriptEngine* engine);


#endif

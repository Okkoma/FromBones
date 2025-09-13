#include <Urho3D/Urho3D.h>

#include <Urho3D/Core/Context.h>
#include <Urho3D/IO/Log.h>

#include <Urho3D/Resource/Resource.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Resource/JSONFile.h>
#include "CraftRecipes.h"


HashMap<StringHash, String > CraftRecipes::elementsToRecipeName_;
HashMap<StringHash, String > CraftRecipes::recipeToElements_;

void CraftRecipes::LoadJSONFile(Context* context, const String& name)
{
    JSONFile* jsonFile = context->GetSubsystem<ResourceCache>()->GetResource<JSONFile>(name);
    if (!jsonFile)
    {
        URHO3D_LOGWARNINGF("CraftRecipes() - LoadJSONFile : %s not exist !", name.CString());
        return;
    }

    const JSONValue& source = jsonFile->GetRoot();

    URHO3D_LOGINFO("CraftRecipes() - LoadJSONFile : " + name);

    Vector<String> materials;
    Vector<String> tools;

    for (JSONObject::ConstIterator i = source.Begin(); i != source.End(); ++i)
    {
        const String& name = i->first_;
        if (name.Empty())
        {
            URHO3D_LOGWARNING("CraftRecipes() - LoadJSONFile : name is empty !");
            continue;
        }

        materials.Clear();
        tools.Clear();

        const JSONValue& attributes = i->second_;
        for (JSONObject::ConstIterator j = attributes.Begin(); j != attributes.End(); ++j)
        {
            const String& attribute = j->first_;
            if (attribute.StartsWith("material"))
                materials += j->second_.GetString().Split(',');
            else if (attribute.StartsWith("tool"))
                tools += j->second_.GetString().Split(',');
            else
            {
                URHO3D_LOGWARNING("CraftRecipes() - LoadJSONFile : attribute is unknown, name=\"" + name + "\"");
                continue;
            }
        }

        for (unsigned i=0; i < materials.Size(); i++)
            materials[i] = materials[i].Trimmed();

        for (unsigned i=0; i < tools.Size(); i++)
            tools[i] = tools[i].Trimmed();

        RegisterRecipe(name, materials, tools);
    }
}
void CraftRecipes::RegisterRecipe(const String& name, const String& mat1, const String& mat2, const String& mat3, const String& mat4, const String& tool1, const String& tool2)
{
    Vector<String> materials;
    Vector<String> tools;

    materials.Push(mat1);
    if (!mat2.Empty())
        materials.Push(mat2);
    if (!mat3.Empty())
        materials.Push(mat3);
    if (!mat4.Empty())
        materials.Push(mat4);
    if (!tool1.Empty())
        tools.Push(tool1);
    if (!tool2.Empty())
        tools.Push(tool2);

    RegisterRecipe(name, materials, tools);
}

void CraftRecipes::RegisterRecipe(const String& name, Vector<String>& materials, Vector<String>& tools)
{
    StringHash recipe(name);

    if (GetElementsForRecipe(recipe).Empty())
    {
        String sortedelements = GetSortedElementsName(materials, tools);
        StringHash sortedHash(sortedelements);
        elementsToRecipeName_[StringHash(sortedelements)] = name;
        recipeToElements_[recipe] = sortedelements;

        URHO3D_LOGINFOF("CraftRecipes() - RegisterRecipe : name=%s(%u) elements=%s(%u) !", name.CString(), recipe.Value(), sortedelements.CString(), sortedHash.Value());
    }
}

String CraftRecipes::GetSortedElementsName(Vector<String>& materials, Vector<String>& tools)
{
    Sort(materials.Begin(), materials.End());
    Sort(tools.Begin(), tools.End());
    String sortedelements;
    for (unsigned i=0; i < 4; i++)
    {
        if (i < materials.Size())
            sortedelements += materials[i];
        else
            sortedelements += "0";

        sortedelements += "|";
    }
    for (unsigned i=0; i < 2; i++)
    {
        if (i < tools.Size())
            sortedelements += tools[i];
        else
            sortedelements += "0";

        if (i < 1)
            sortedelements += "|";
    }

    return sortedelements;
}

const String& CraftRecipes::GetRecipeForElements(Vector<String>& materials, Vector<String>& tools)
{
    return GetRecipeForElements(GetSortedElementsName(materials, tools));
}

const String& CraftRecipes::GetRecipeForElements(const String& sortedelements)
{
    HashMap<StringHash, String >::ConstIterator it = elementsToRecipeName_.Find(StringHash(sortedelements));
    return it != elementsToRecipeName_.End() ? it->second_ : String::EMPTY;
}

const String& CraftRecipes::GetElementsForRecipe(const StringHash& recipe)
{
    HashMap<StringHash, String >::ConstIterator it = recipeToElements_.Find(recipe);
    return it != recipeToElements_.End() ? it->second_ : String::EMPTY;
}

StringHash CraftRecipes::GetRecipe(const StringHash& hashname)
{
    HashMap<StringHash, String >::ConstIterator it = recipeToElements_.Find(hashname);
    return it != recipeToElements_.End() ? it->first_ : String::EMPTY;
}

void CraftRecipes::GetElementsForRecipe(const StringHash& recipe, Vector<String>& materials, Vector<String>& tools)
{
    materials.Clear();
    tools.Clear();

    Vector<String> elements = GetElementsForRecipe(recipe).Split('|');
    for (int i=0; i < 4; i++)
    {
        if (elements[i] != "0")
            materials.Push(elements[i]);
    }
    for (int i=4; i < 6; i++)
    {
        if (elements[i] != "0")
            tools.Push(elements[i]);
    }
}

void CraftRecipes::Dump()
{

}

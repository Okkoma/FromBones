#pragma once

using namespace Urho3D;

struct CraftRecipes
{

public :
    static void LoadJSONFile(Context* context, const String& name);
    static void RegisterRecipe(const String& name, const String& mat1, const String& mat2=String::EMPTY, const String& mat3=String::EMPTY, const String& mat4=String::EMPTY, const String& tool1=String::EMPTY, const String& tool2=String::EMPTY);
    static void RegisterRecipe(const String& name, Vector<String>& materials, Vector<String>& tools);
    static String GetSortedElementsName(Vector<String>& materials, Vector<String>& tools);
    static const String& GetRecipeForElements(const String& sortedelements);
    static const String& GetRecipeForElements(Vector<String>& materials, Vector<String>& tools);
    static const String& GetElementsForRecipe(const StringHash& recipe);
    static StringHash GetRecipe(const StringHash& hashname);
    static void GetElementsForRecipe(const StringHash& recipe, Vector<String>& materials, Vector<String>& tools);

    static void Dump();

private :
    static HashMap<StringHash, String > elementsToRecipeName_;
    static HashMap<StringHash, String > recipeToElements_;
};

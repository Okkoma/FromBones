#pragma once

class FROMBONES_API MapEditorLib : public Object
{
    URHO3D_OBJECT(MapEditorLib, Object);

public:
    MapEditorLib(Context* context) : Object(context) { }
    virtual ~MapEditorLib() { }

    virtual void Update(float timestep) = 0;
    virtual void Clean() = 0;
    virtual void Render() = 0;
};


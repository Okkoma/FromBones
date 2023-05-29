#pragma once

#include <Urho3D/Urho2D/Drawable2D.h>
#include <Urho3D/Urho2D/RigidBody2D.h>

#include "GOC_Controller.h"
#include "GOC_ControllerAI.h"
#include "GOC_Destroyer.h"
#include "GOC_Collide2D.h"
#include "GOC_Move2D.h"
#include "GOC_Animator2D.h"
#include "GOC_Life.h"
#include "GOC_Attack.h"
#include "GOC_Inventory.h"
#include "GOC_Detector.h"

struct CommonComponents
{
    Drawable2D* drawable_;
    RigidBody2D* body_;

    GOC_Controller* controller_;
    GOC_AIController* controllerai_;
    GOC_Destroyer* destroyer_;
    GOC_Collide2D* collider_;
    GOC_Move2D* mover_;
    GOC_Animator2D* animator_;

    GOC_Life* life_;
    GOC_Attack* attack_;
    GOC_Inventory* inventory_;
};

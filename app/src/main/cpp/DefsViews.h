#pragma once

#include "GameOptionsTest.h"

const unsigned MAX_VIEWPORTS = 4;

const int NOVIEW             = -1;

// Scroller Back Layers
const int BACKSCROLL_0       = 0;     // Sky
const int BACKSCROLL_1       = 1;     // First Mountains
const int BACKSCROLL_2       = 2;
const int BACKSCROLL_3       = 6;
const int BACKWATER_1        = 7;
const int BACKSCROLL_4       = 8;
const int BACKWATER_2        = 9;
const int BACKSCROLL_5       = 10;

// View Layers
//const int BACKBIOME          = 10;
//const int BACKGROUND         = 15;
//const int BACKACTORVIEW      = 16;
//const int INNERRAINLAYER     = 20;
//const int BACKVIEW           = 25;
//const int BACKINNERBIOME     = 30;
//const int THRESHOLDVIEW      = 35;
//const int INNERVIEW          = 40;
//const int FRONTINNERBIOME    = 45;
//const int OUTERVIEW          = 50;
//const int FRONTVIEW          = 80;
//const int FRONTBIOME         = 90;

const int BACKBIOME          = 19;
const int BACKGROUND         = 20;
const int BACKACTORVIEW      = 23;
const int INNERRAINLAYER     = 30;
const int BACKVIEW           = 40;
const int BACKINNERBIOME     = 50;
const int THRESHOLDVIEW      = 60;
const int INNERVIEW          = 70;
const int FRONTINNERBIOME    = 80;
const int OUTERVIEW          = 90;
const int OUTERBIOME         = 96;
const int FRONTVIEW          = 100;
const int FRONTBIOME         = 110;

// Layer Modifiers
const int LAYER_BOBJECTMAPED = -(INNERVIEW - 10 + 1);
const int LAYER_BACKROOFS    = -4;
const int LAYER_PLATEFORMS   = -7;
const int LAYER_PLATEFORM    = -6;
const int LAYER_FURNITURES   = -4;
const int LAYER_ACTOR        = -3;
const int LAYER_OBJECTMAPED  = -2;
const int LAYER_FLUID        = -1;
const int LAYER_FRONTROOFS   = 2;
#ifdef ACTIVE_RENDERSHAPE_EMBOSE
const int LAYER_DECALS          = 1;
const int LAYER_SEWINGS         = 1;
const int LAYER_RENDERSHAPE     = 3;
#ifndef GL_ES_VERSION_2_0
const float RS_ALPHA_COMMON     = 0.65f;
const float RS_ALPHA_INNERVIEW  = 0.85f;
const float RS_ALPHA_BACKVIEW   = 0.45f;
const float RS_ALPHA_BACKGROUND = 0.45f;
#else
const float RS_ALPHA_COMMON     = 0.35f;
const float RS_ALPHA_INNERVIEW  = 0.35f;
const float RS_ALPHA_BACKVIEW   = 0.25f;
const float RS_ALPHA_BACKGROUND = 0.25f;
#endif

#else
const int LAYER_RENDERSHAPE     = 1;
const int LAYER_DECALS          = 2;
const int LAYER_SEWINGS         = 3;
const float RS_ALPHA_COMMON     = 0.5f;
const float RS_ALPHA_INNERVIEW  = 0.35f;
const float RS_ALPHA_BACKVIEW   = 0.25f;
const float RS_ALPHA_BACKGROUND = 0.35f;
#endif
const int LAYER_WEATHER      = 3;
const int LAYERADDER_DIALOGS = 4;

const int DRAWORDER_EFFECT = 1000;
const int DRAWORDER_TILEENTITY = 2000;

// Scroller Front Layers
const int FRONTSCROLL_1      = FRONTVIEW+2;
const int FRONTSCROLL_2      = FRONTVIEW+4;
const int FRONTSCROLL_3      = FRONTVIEW+6;
const int FRONTSCROLL_4      = FRONTVIEW+8;
const int FRONTSCROLL_5      = FRONTVIEW+12;

const int LAYERMAXVALUE      = FRONTSCROLL_5;
const float PARALLAXSPEED    = 1.05f;

const float SCROLLERBACKGROUND_ALPHA = 0.4f;
const float SCROLLERBACKVIEW_ALPHA   = 0.3f;
const float SCROLLERINNERVIEW_ALPHA  = 0.5f;

// Layer Mask
const unsigned BACKGROUND_MASK          = 256U;//Urho3D::DRAWABLE_ANY+1;
const unsigned BACKVIEW_MASK            = BACKGROUND_MASK << 1;
const unsigned INNERVIEW_MASK           = BACKGROUND_MASK << 2;
const unsigned INNERBIOME_MASK          = BACKGROUND_MASK << 3;
const unsigned OUTERVIEW_MASK           = BACKGROUND_MASK << 4;
const unsigned FRONTVIEW_MASK           = BACKGROUND_MASK << 5;
const unsigned FRONTBIOME_MASK          = BACKGROUND_MASK << 6;

// View Mask
//const unsigned WEATHERINSIDEVIEW_MASK   = BACKGROUND_MASK << 7;
//const unsigned WEATHEROUTSIDEVIEW_MASK  = BACKGROUND_MASK << 8;
//const unsigned VIEWPORT_MASK            = BACKGROUND_MASK << 9;
//const unsigned ALLVIEWPORTS_MASK        = VIEWPORT_MASK | (VIEWPORT_MASK << 1) | (VIEWPORT_MASK << 2) | (VIEWPORT_MASK << 3);
const unsigned VIEWPORTSCROLLER_INSIDE_MASK      = BACKGROUND_MASK << 7;
const unsigned ALLVIEWPORTSCROLLERS_INSIDE_MASK  = VIEWPORTSCROLLER_INSIDE_MASK | (VIEWPORTSCROLLER_INSIDE_MASK << 1) | (VIEWPORTSCROLLER_INSIDE_MASK << 2) | (VIEWPORTSCROLLER_INSIDE_MASK << 3);
const unsigned VIEWPORTSCROLLER_OUTSIDE_MASK     = BACKGROUND_MASK << 11;
const unsigned ALLVIEWPORTSCROLLERS_OUTSIDE_MASK = VIEWPORTSCROLLER_OUTSIDE_MASK | (VIEWPORTSCROLLER_OUTSIDE_MASK << 1) | (VIEWPORTSCROLLER_OUTSIDE_MASK << 2) | (VIEWPORTSCROLLER_OUTSIDE_MASK << 3);
//const unsigned DEFAULT_INNERVIEW_MASK  = BACKGROUND_MASK | BACKVIEW_MASK | INNERVIEW_MASK | WEATHERINSIDEVIEW_MASK | INNERBIOME_MASK;
//const unsigned DEFAULT_FRONTVIEW_MASK  = BACKGROUND_MASK | OUTERVIEW_MASK | INNERVIEW_MASK | FRONTVIEW_MASK | WEATHERINSIDEVIEW_MASK | WEATHEROUTSIDEVIEW_MASK | FRONTBIOME_MASK | INNERBIOME_MASK;
const unsigned DEFAULT_INNERVIEW_MASK  = BACKGROUND_MASK | BACKVIEW_MASK | INNERVIEW_MASK | INNERBIOME_MASK;// | ALLVIEWPORTSCROLLERS_INSIDE_MASK;
const unsigned DEFAULT_FRONTVIEW_MASK  = BACKGROUND_MASK | INNERVIEW_MASK | INNERBIOME_MASK | OUTERVIEW_MASK | FRONTVIEW_MASK | FRONTBIOME_MASK;// | VIEWPORTSCROLLER_INSIDE_MASK | VIEWPORTSCROLLER_OUTSIDE_MASK;
const unsigned NOLIGHT_MASK = BACKGROUND_MASK << 15;

// View Ids
const unsigned FrontView_ViewId = 0;
const unsigned BackGround_ViewId = 1;
const unsigned InnerView_ViewId = 2;
const unsigned BackView_ViewId = 3;
const unsigned OuterView_ViewId = 4;

// Enlightment Threshold
const float ENLIGHTTHRESHOLD = 0.5f;

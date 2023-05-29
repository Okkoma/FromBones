
void SetRenderPath(const String&in newRenderPathName)
{
    renderPath = null;
    renderPathName = newRenderPathName.Trimmed();

    if (renderPathName.length > 0)
    {
        File@ file = cache.GetFile(renderPathName);
        if (file !is null)
        {
            XMLFile@ xml = XMLFile();
            if (xml.Load(file))
            {
                renderPath = RenderPath();
                if (!renderPath.Load(xml))
                    renderPath = null;
            }
        }
    }

    // If renderPath is null, the engine default will be used
    renderer.viewports[0].renderPath = renderPath;

//    if (materialPreview !is null && materialPreview.viewport !is null)
//        materialPreview.viewport.renderPath = renderPath;
}

void SetFillMode(FillMode fillMode_)
{
    fillMode = fillMode_;

    renderer.viewports[0].camera.fillMode = fillMode_;
}

void SetActiveViewport()
{
    // Sets the global variables to the current context
    @camera = renderer.viewports[0].camera;
    @cameraNode = camera.node;

//    @audio.listener = context.soundListener;

    // Camera is created before gizmo, this gets called again after UI is created
//    if (gizmo !is null)
//        gizmo.viewMask = camera.viewMask;

    // If a mode is changed while in a drag or hovering over a border these can get out of sync
    resizingBorder = 0;
    setViewportCursor = 0;
}

void SetMouseMode(bool enable)
{
    if (enable)
    {
        if (mouseOrbitMode == ORBIT_RELATIVE)
        {
            input.mouseMode = MM_RELATIVE;
            ui.cursor.visible = false;
        }
        else if (mouseOrbitMode == ORBIT_WRAP)
            input.mouseMode = MM_WRAP;
    }
    else
    {
        input.mouseMode = MM_ABSOLUTE;
        ui.cursor.visible = true;
    }
}

void SetMouseLock()
{
    toggledMouseLock = true;
    SetMouseMode(true);
    FadeUI();
}

void ReleaseMouseLock()
{
    if (toggledMouseLock)
    {
        toggledMouseLock = false;
        SetMouseMode(false);
    }
}

void UpdateView(float timeStep)
{
    if (ui.HasModalElement() || ui.focusElement !is null)
    {
        ReleaseMouseLock();
        return;
    }

    float speedMultiplier = 1.0;
    if (input.keyDown[KEY_LSHIFT])
        speedMultiplier = cameraShiftSpeedMultiplier;

    // Zoom camera
    if (input.mouseMoveWheel != 0 && ui.GetElementAt(ui.cursor.position) is null)
    {
        if (mouseWheelCameraPosition)
        {
            cameraNode.Translate(Vector3(0, 0, -cameraBaseSpeed) * -input.mouseMoveWheel*20 * timeStep * speedMultiplier);
        }
        else
        {
            float zoom = camera.zoom + input.mouseMoveWheel * 0.1 * speedMultiplier;
            camera.zoom = Clamp(zoom, 0.1, 30);
        }
    }

    cameraViewMoving = false;

    // Pan camera
    if (input.mouseButtonDown[MOUSEB_RIGHT] || input.mouseButtonDown[MOUSEB_MIDDLE])
    {
//        SetMouseLock();

        IntVector2 mouseMove = input.mouseMove;
        if (mouseMove.x != 0 || mouseMove.y != 0)
        {
            if (input.mouseButtonDown[MOUSEB_MIDDLE])
            {
                cameraNode.Translate(Vector3(-mouseMove.x, mouseMove.y, 0) * timeStep * cameraBaseSpeed * 0.5);
                cameraViewMoving = true;
            }
        }

        return;
    }
//    else
//    {
//        ReleaseMouseLock();
//    }


    if (!editNodes.empty)
    {
        // Move nodes
        if (input.mouseButtonDown[MOUSEB_LEFT])
        {
            Vector3 adjust(input.mouseMove.x, -input.mouseMove.y, 0);

            adjust *= timeStep * moveStep;

            bool moved = MoveNodes(adjust);

            if (moved)
            {
                UpdateNodeAttributes();
                movedNodes = true;
            }
        }
        // TODO : Update moved nodes
        else if (movedNodes)
        {
            log.Info("End Move Nodes !");
            for (uint i = 0; i < editNodes.length; ++i)
                UpdateEntityPosition(editNodes[i]);

            movedNodes = false;
        }
    }
}

bool MoveNodes(Vector3 adjust)
{
    bool moved = false;

    if (adjust.length > M_EPSILON)
    {
        for (uint i = 0; i < editNodes.length; ++i)
        {
//            if (moveSnap)
//            {
//                float moveStepScaled = moveStep * snapScale;
//                adjust.x = Floor(adjust.x / moveStepScaled + 0.5) * moveStepScaled;
//                adjust.y = Floor(adjust.y / moveStepScaled + 0.5) * moveStepScaled;
//                adjust.z = Floor(adjust.z / moveStepScaled + 0.5) * moveStepScaled;
//            }

            Node@ node = editNodes[i];
            Vector3 nodeAdjust = adjust;
            if (axisMode == AXIS_LOCAL && editNodes.length == 1)
                nodeAdjust = node.worldRotation * nodeAdjust;

            Vector3 worldPos = node.worldPosition;
            Vector3 oldPos = node.position;

            worldPos += nodeAdjust;

            if (node.parent is null)
                node.position = worldPos;
            else
                node.position = node.parent.WorldToLocal(worldPos);

            if (node.position != oldPos)
                moved = true;
        }
    }

    return moved;
}

void SteppedObjectManipulation(int key)
{
    if (editNodes.empty || editMode == EDIT_SELECT)
        return;

    // Do not react in non-snapped mode, because that is handled in frame update
    if (editMode == EDIT_MOVE && !moveSnap)
        return;
    if (editMode == EDIT_ROTATE && !rotateSnap)
        return;
    if (editMode == EDIT_SCALE && !scaleSnap)
        return;

    Vector3 adjust(0, 0, 0);
    if (key == KEY_UP)
        adjust.z = 1;
    if (key == KEY_DOWN)
        adjust.z = -1;
    if (key == KEY_LEFT)
        adjust.x = -1;
    if (key == KEY_RIGHT)
        adjust.x = 1;
    if (key == KEY_PAGEUP)
        adjust.y = 1;
    if (key == KEY_PAGEDOWN)
        adjust.y = -1;
    if (editMode == EDIT_SCALE)
    {
        if (key == KEY_KP_PLUS)
            adjust = Vector3(1, 1, 1);
        if (key == KEY_KP_MINUS)
            adjust = Vector3(-1, -1, -1);
    }

    if (adjust == Vector3(0, 0, 0))
        return;

    bool moved = false;

//    switch (editMode)
//    {
//    case EDIT_MOVE:
//        moved = MoveNodes(adjust);
//        break;
//
//    case EDIT_ROTATE:
//        {
//            float rotateStepScaled = rotateStep * snapScale;
//            moved = RotateNodes(adjust * rotateStepScaled);
//        }
//        break;
//
//    case EDIT_SCALE:
//        {
//            float scaleStepScaled = scaleStep * snapScale;
//            moved = ScaleNodes(adjust * scaleStepScaled);
//        }
//        break;
//    }

    if (moved)
        UpdateNodeAttributes();
}

void SpawnObject()
{
    bool spawned = false;

    if (spawnCategory == SPAWN_ACTOR)
    {
        spawned = world2D.SpawnActor() !is null;
        if (spawned)
            log.Info("Spawn Actor !");
    }
    else if (spawnCategory == SPAWN_MONSTER)
    {
        spawned = world2D.SpawnEntity(StringHash("GOT_Skeleton")) !is null;
        if (spawned)
            log.Info("Spawn Monster !");
    }
    else if (spawnCategory == SPAWN_PLANT)
    {
        spawned = world2D.SpawnFurniture(StringHash("FUR_Plant04")) !is null;
        if (spawned)
            log.Info("Spawn Plant !");

    }
    else if (spawnCategory == SPAWN_TILE)
    {
        if (spawned)
            log.Info("Spawn Tile !");
    }

    if (spawned == true)
        UpdateHierarchyItem(editorScene.GetChild("Entities", true));
}

void DrawNodeDebug(Node@ node, DebugRenderer@ debug, bool drawNode = true)
{
    if (node == null)
        return;

    if (drawNode)
        debug.AddNode(node, 1.0, false);

    if (node !is editorScene)
    {
        // drawdebug only drawable2d and shaped2d
        for (uint j = 0; j < node.numComponents; ++j)
        {
            Drawable@ drawable2d = cast<Drawable2D>(node.components[j]);
            if (drawable2d !is null)
            {
                drawable2d.DrawDebugGeometry(debug, false);
            }
            else
            {
                CollisionShape2D@ shape2d = cast<CollisionShape2D>(node.components[j]);
                if (shape2d !is null)
                    shape2d.DrawDebugGeometry(debug, false);
            }
        }

        // To avoid cluttering the view, do not draw the node axes for child nodes
        for (uint k = 0; k < node.numChildren; ++k)
            DrawNodeDebug(node.children[k], debug, false);
    }
}

void ViewMouseMove()
{
    // setting mouse position based on mouse position
    if (ui.IsDragging()) { }
    else if (ui.focusElement !is null || input.mouseButtonDown[MOUSEB_LEFT|MOUSEB_MIDDLE|MOUSEB_RIGHT])
        return;
}

void ViewMouseClick()
{
    ViewRaycast(true);
}

Ray GetActiveViewportCameraRay()
{
    IntRect view = renderer.viewports[0].rect;
    return camera.GetScreenRay(
        float(ui.cursorPosition.x - view.left) / view.width,
        float(ui.cursorPosition.y - view.top) / view.height
    );
}

void ViewMouseClickEnd()
{
    // checks to close open popup windows
    IntVector2 pos = ui.cursorPosition;
    if (contextMenu !is null && contextMenu.enabled)
    {
        if (contextMenuActionWaitFrame)
            contextMenuActionWaitFrame = false;
        else
        {
            if (!contextMenu.IsInside(pos, true))
                CloseContextMenu();
        }
    }
    if (quickMenu !is null && quickMenu.enabled)
    {
        bool enabled = quickMenu.IsInside(pos, true);
        quickMenu.enabled = enabled;
        quickMenu.visible = enabled;
    }
}

void ViewRaycast(bool mouseClick)
{
    // Ignore if UI has modal element
    if (ui.HasModalElement())
        return;

    // Ignore if mouse is grabbed by other operation
    if (input.mouseGrabbed)
        return;

    IntVector2 pos = ui.cursorPosition;
    UIElement@ elementAtPos = ui.GetElementAt(pos, pickMode != PICK_UI_ELEMENTS);
    if (spawnCategory > SPAWN_NONE)
    {
        if(mouseClick && input.mouseButtonPress[MOUSEB_LEFT] && elementAtPos is null)
        {
            SpawnObject();
        }
        return;
    }

    // Do not raycast / change selection if hovering over the gizmo
//    if (IsGizmoSelected())
//        return;

    DebugRenderer@ debug = editorScene.debugRenderer;

    // Do not raycast / change selection if hovering over a UI element when not in PICK_UI_ELEMENTS Mode
    if (elementAtPos !is null)
        return;

    Ray cameraRay = GetActiveViewportCameraRay();
    Component@ selectedComponent;

    if (pickMode < PICK_RIGIDBODIES)
    {
        if (editorScene.octree is null)
            return;

//        RayQueryResult result = editorScene.octree.RaycastSingle(cameraRay, RAY_TRIANGLE, camera.farClip, pickModeDrawableFlags[pickMode], 0x7fffffff);

        // Find the smallest drawable
        int ir = -1;
        Array<RayQueryResult>@ results = editorScene.octree.Raycast(cameraRay, RAY_TRIANGLE, camera.farClip, pickModeDrawableFlags[pickMode]);
        if (results.length > 0)
        {
            float area = M_INFINITY;

            for (uint i=0; i < results.length; i++)
            {
                Drawable@ drawable = results[i].drawable;
                if (cast<ObjectTiled>(drawable) !is null)
                    continue;
                if (cast<RenderShape>(drawable) !is null)
                    continue;
                if (cast<DrawableScroller>(drawable) !is null)
                    continue;
                float newarea =drawable.worldArea2D;
                if (newarea >= area)
                    continue;

                area = newarea;
                ir = i;
            }
        }

        if (ir != -1)
        {
            RayQueryResult result = results[ir];
            if (result.drawable !is null)
            {
                Drawable@ drawable = result.drawable;

                // for actual last selected node or component in both modes
                if (input.mouseButtonDown[MOUSEB_LEFT])
                {
                    lastSelectedNode = drawable.node;
                    lastSelectedDrawable = drawable;
                    lastSelectedComponent = drawable;
                }
                selectedComponent = drawable;

                if (debug !is null)
                {
                    debug.AddNode(drawable.node, 1.0, false);
                    drawable.DrawDebugGeometry(debug, false);
                }
            }
        }
        else
        {
            log.Info("ViewRaycast no drawable found with rayOrigin=" + cameraRay.origin.ToString());
        }
    }
    else
    {
        Component@ body = bodyAtCursor;
        if (body !is null)
        {
            log.Info("ViewRaycast body : body=" + body.node.name);
            selectedComponent = body;
        }
    }

    bool multiselect = input.qualifierDown[QUAL_CTRL];
    bool componentSelectQualifier = input.qualifierDown[QUAL_SHIFT];
    bool mouseButtonPressRL = input.mouseButtonPress[MOUSEB_LEFT];

    if (mouseClick && mouseButtonPressRL)
    {
        if (selectedComponent !is null)
        {
            if (componentSelectQualifier)
            {
                // If we are selecting components, but have nodes in existing selection, do not multiselect to prevent confusion
                if (!selectedNodes.empty)
                    multiselect = false;
                SelectComponent(selectedComponent, multiselect);
            }
            else
            {
                // If we are selecting nodes, but have components in existing selection, do not multiselect to prevent confusion
                if (!selectedComponents.empty)
                    multiselect = false;

                log.Info("select node = " + selectedComponent.node.name);
                SelectNode(selectedComponent.node, multiselect);
            }
            ShowAttributeInspectorWindow();
        }
        else
        {
            // If clicked on emptiness in non-multiselect mode, clear the selection
            if (!multiselect)
            {
                SelectComponent(null, false);
                HideAttributeInspectorWindow();
            }
        }
    }
}

Vector3 GetNewNodePosition()
{
    if (newNodeMode == NEW_NODE_IN_CENTER)
        return Vector3(0, 0, 0);
//    if (newNodeMode == NEW_NODE_RAYCAST)
//    {
//        Ray cameraRay = camera.GetScreenRay(0.5, 0.5);
//        Vector3 position, normal;
//        if (GetSpawnPosition(cameraRay, camera.farClip, position, normal, 0, false))
//            return position;
//    }
    return cameraNode.worldPosition + cameraNode.worldRotation * Vector3(0, 0, newNodeDistance);
}

void ToggleRenderingDebug()
{
    renderingDebug = !renderingDebug;
}

void TogglePhysicsDebug()
{
    physicsDebug = !physicsDebug;
}

void LocateNode(Node@ node)
{
    if (node is null || node is editorScene)
        return;

    Vector3 center = node.worldPosition;
    float distance = newNodeDistance;

    for (uint i = 0; i < node.numComponents; ++i)
    {
        // Determine view distance from drawable component's bounding box. Skip skybox, as its box is very large, as well as lights
        Drawable@ drawable = cast<Drawable>(node.components[i]);
        if (drawable !is null && cast<Skybox>(drawable) is null && cast<Light>(drawable) is null)
        {
            BoundingBox box = drawable.worldBoundingBox;
            center = box.center;
            // Ensure the object fits on the screen
            distance = Max(distance, newNodeDistance + box.size.length);
            break;
        }
    }

    if (distance > viewFarClip)
        distance = viewFarClip;

    cameraNode.worldPosition = center - cameraNode.worldDirection * distance;
}

Vector3 SelectedNodesCenterPoint()
{
    Vector3 centerPoint;
    uint count = selectedNodes.length;
    for (uint i = 0; i < selectedNodes.length; ++i)
        centerPoint += selectedNodes[i].worldPosition;

    for (uint i = 0; i < selectedComponents.length; ++i)
    {
        Drawable@ drawable = cast<Drawable>(selectedComponents[i]);
        count++;
        if (drawable !is null)
            centerPoint += drawable.node.LocalToWorld(drawable.boundingBox.center);
        else
            centerPoint += selectedComponents[i].node.worldPosition;
    }

    if (count > 0)
    {
        lastSelectedNodesCenterPoint = centerPoint / count;
        return centerPoint / count;
    }
    else
    {
        lastSelectedNodesCenterPoint = centerPoint;
        return centerPoint;
    }
}

Vector3 GetScreenCollision(IntVector2 pos)
{
    Ray cameraRay = camera.GetScreenRay(float(pos.x) / renderer.viewports[0].rect.width, float(pos.y) / renderer.viewports[0].rect.height);
    Vector3 res = cameraNode.position + cameraRay.direction * Vector3(0, 0, newNodeDistance);

    bool physicsFound = false;
//    if (editorScene.physicsWorld !is null)
//    {
//        if (!runUpdate)
//            editorScene.physicsWorld.UpdateCollisions();
//
//        PhysicsRaycastResult result = editorScene.physicsWorld.RaycastSingle(cameraRay, camera.farClip);
//
//        if (result.body !is null)
//        {
//            physicsFound = true;
//            result.position;
//        }
//    }

    if (editorScene.octree is null)
        return res;

    RayQueryResult result = editorScene.octree.RaycastSingle(cameraRay, RAY_TRIANGLE, camera.farClip, DRAWABLE_GEOMETRY, 0x7fffffff);

    if (result.drawable !is null)
    {
        // take the closer of the results
        if (physicsFound && (cameraNode.position - res).length < (cameraNode.position - result.position).length)
            return res;
        else
            return result.position;
    }

    return res;
}

Drawable@ GetDrawableAtMousePostion()
{
    IntVector2 pos = ui.cursorPosition;
    Ray cameraRay = camera.GetScreenRay(float(pos.x) / renderer.viewports[0].rect.width, float(pos.y) / renderer.viewports[0].rect.height);

    if (editorScene.octree is null)
        return null;

    RayQueryResult result = editorScene.octree.RaycastSingle(cameraRay, RAY_TRIANGLE, camera.farClip, DRAWABLE_GEOMETRY, 0x7fffffff);

    return result.drawable;
}

void ClearSceneSelection()
{
    selectedNodes.Clear();
    selectedComponents.Clear();
    editNode = null;
    editNodes.Clear();
    editComponents.Clear();
    numEditableComponentsPerNode = 1;

//    HideGizmo();
}

void HandlePostRenderUpdate()
{
    DebugRenderer@ debug = editorScene.debugRenderer;
    if (debug is null || orbiting || debugRenderDisabled)
        return;

    // Visualize the currently selected nodes
    for (uint i = 0; i < selectedNodes.length; ++i)
        DrawNodeDebug(selectedNodes[i], debug, false);

    // Visualize the currently selected components
    for (uint i = 0; i < selectedComponents.length; ++i)
    {
        if (cast<ObjectTiled>(selectedComponents[i]) !is null)
            continue;
        if (cast<RenderShape>(selectedComponents[i]) !is null)
            continue;
        if (cast<DrawableScroller>(selectedComponents[i]) !is null)
            continue;
        selectedComponents[i].DrawDebugGeometry(debug, false);
    }

    if (!cameraViewMoving)
        ViewRaycast(false);
}

void HandleBeginViewUpdate(StringHash eventType, VariantMap& eventData)
{
    // Hide gizmo, grid and debug icons from any camera other then active viewport
    if (eventData["Camera"].GetPtr() !is camera)
    {
//        if (gizmo !is null)
//            gizmo.viewMask = 0;
    }
    if (eventData["Camera"].GetPtr() is previewCamera.Get())
    {
        suppressSceneChanges = true;
        if (grid !is null)
            grid.viewMask = 0;
//        if (debugIconsNode !is null)
//            debugIconsNode.enabled = false;
        suppressSceneChanges = false;
    }
}

void HandleEndViewUpdate(StringHash eventType, VariantMap& eventData)
{
    // Restore gizmo and grid after camera view update
    if (eventData["Camera"].GetPtr() !is camera)
    {
//        if (gizmo !is null)
//            gizmo.viewMask = 0x80000000;
    }
    if (eventData["Camera"].GetPtr() is previewCamera.Get())
    {
        suppressSceneChanges = true;
        if (grid !is null)
            grid.viewMask = 0x80000000;
//        if (debugIconsNode !is null)
//            debugIconsNode.enabled = true;
        suppressSceneChanges = false;
    }
}

bool debugWasEnabled = true;

void HandleBeginViewRender(StringHash eventType, VariantMap& eventData)
{
    // Hide debug geometry from preview camera
    if (eventData["Camera"].GetPtr() is previewCamera.Get())
    {
        DebugRenderer@ debug = editorScene.GetComponent("DebugRenderer");
        if (debug !is null)
        {
            suppressSceneChanges = true; // Do not want UI update now
            debugWasEnabled = debug.enabled;
            debug.enabled = false;
            suppressSceneChanges = false;
        }
    }
}

void HandleEndViewRender(StringHash eventType, VariantMap& eventData)
{
    // Restore debug geometry after preview camera render
    if (eventData["Camera"].GetPtr() is previewCamera.Get())
    {
        DebugRenderer@ debug = editorScene.GetComponent("DebugRenderer");
        if (debug !is null)
        {
            suppressSceneChanges = true; // Do not want UI update now
            debug.enabled = debugWasEnabled;
            suppressSceneChanges = false;
        }
    }
}

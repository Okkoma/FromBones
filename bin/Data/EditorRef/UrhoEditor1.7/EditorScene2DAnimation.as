

bool ToggleScene2DAnimator()
{
    if (scene2DAnimator.GetWindow().visible == false)
        scene2DAnimator.Show();
    else
        scene2DAnimator.Hide();
    return true;
}

void SetScene2DAnimatorRunning(bool runstate)
{
    if (scene2DAnimator.sceneAnimation_ !is null)
    {
        if (!scene2DAnimator.sceneAnimation_.get_node().get_enabled())
            return;

        scene2DAnimator.Show();
        scene2DAnimator.sceneAnimation_.SetRunning(runstate);
    }
}

class Scene2DAnimator
{
    private Window@ window;
    private ListView@ timeLineGroups;
    private UIElement@ timelineContainer;
    private TimeLineModifier@ timelineModifier;
    private Array<uint> timeLinesIndexes;

    SceneAnimation2D@ sceneAnimation_;
    Node@ cameraNode_;

    void Create()
    {
        if (window !is null)
            return;

        window = LoadEditorUI("UI/EditorScene2DAnimatorWindow.xml");
        ui.root.AddChild(window);
        window.opacity = uiMaxOpacity;

        window.SetPosition(browserWindow.position.x + browserWindow.width + 10, browserWindow.position.y);

        timeLineGroups = window.GetChild("TimeLineGroups", true);
        timelineContainer = timeLineGroups.GetChild("TimeLineContainer", true);
        timelineModifier = window.GetChild("TimeLineModifier", true);
        sceneAnimation_ = null;
        cameraNode_ = null;

        // Subscribe to Events
        SubscribeToEvent(window.GetChild("CloseButton", true), "Released", "Hide");
        SubscribeToEvent(timeLineGroups, "ItemClicked", "HandleTimeLineGroupClick");
        SubscribeToEvent(timelineModifier, "ItemClicked", "HandleTimeLineModClicked");
        SubscribeToEvent(timelineModifier, "ItemSelected", "HandleTimeLineModSelected");

        Hide();
    }

    Window@ GetWindow() { return window; }

    Text@ InsertTimeGroup(uint index, const String&in name, UIElement@ parentUI = null)
    {
        Text@ group = Text();
        timeLineGroups.InsertItem(index, group, parentUI);
        group.style = "FileSelectorListText";
        group.text = name;
        group.name = name;
        return group;
    }

    void Update(float timestep)
    {
        if (runUpdate && sceneAnimation_ !is null)
        {
            if (!sceneAnimation_.get_node().get_enabled())
                return;

            if (!sceneAnimation_.IsFinished())
                sceneAnimation_.Update(timestep);
            else
                sceneAnimation_.SetTime(0.f);
        }
    }

    void UpdateTimeGroups()
    {
        if (sceneAnimation_ is null)
            return;

        timeLineGroups.RemoveAllItems();
        Array<String>@ timeLineNames = sceneAnimation_.GetTimeLineNames();
        uint numNames = timeLineNames.length;
        uint itemIndex = 0;
        for (uint i = 0; i < numNames; ++i)
        {
            Text@ group = InsertTimeGroup(itemIndex++, timeLineNames[i]);
            Array<String>@ timeLineObjectNames = sceneAnimation_.GetTimeLineObjectNames(i);
            uint numObjectNames = timeLineObjectNames.length;
            for (uint j = 0; j < numObjectNames; ++j)
                InsertTimeGroup(itemIndex++, timeLineObjectNames[j], group);
        }

        timelineModifier.ConnectTimeLineContainer(timeLineGroups);
    }

    void UpdateTimeLines()
    {
        timeLinesIndexes.Clear();
        uint numNames = sceneAnimation_.GetTimeLineNames().length;
        uint itemIndex = 0;
        for (uint i = 0; i < numNames; ++i)
        {
            timeLinesIndexes.Push(itemIndex);
            itemIndex += sceneAnimation_.GetTimeLineObjectNames(i).length + 1;
        }
    }

    Node@ GetObjectNode(const String&in name)
    {
        if (sceneAnimation_ is null)
            return null;

        Array<String>@ timeLineNames = sceneAnimation_.GetTimeLineNames();
        uint numNames = timeLineNames.length;
        uint itemIndex = 0;
        for (uint i = 0; i < numNames; ++i)
        {
            Array<String>@ timeLineObjectNames = sceneAnimation_.GetTimeLineObjectNames(i);
            uint numObjectNames = timeLineObjectNames.length;
            for (uint j = 0; j < numObjectNames; ++j)
            {
                if (timeLineObjectNames[j] == name)
                {
                    uint nodeid = sceneAnimation_.GetTimeLineObjectNodeID(i,j);
                    if (nodeid > 0)
                    {
                        Node@ node = editorScene.GetNode(nodeid);
                        if (node !is null)
                            return node;
                    }
                }
            }
        }

        return null;
    }

    void SetSceneAnimation2D(SceneAnimation2D@ animation)
    {
        if (animation is null || animation is sceneAnimation_)
            return;

        if (sceneAnimation_ !is null)
        {
            get_log().Info("Scene2DAnimator::SetSceneAnimation2D : stop last animation");

            UnsubscribeFromEvent(sceneAnimation_, "SceneAnimationFinished");
            UnsubscribeFromEvent(sceneAnimation_, "SceneAnimationTimeLinesUpdated");
            sceneAnimation_.Stop();
        }

        sceneAnimation_ = animation;

        get_log().Info("Scene2DAnimator::SetSceneAnimation2D : set new animation");

        if (sceneAnimation_.IsEmpty())
            sceneAnimation_.AddTimeLine(0, "TimeGroup");

        timelineModifier.SetTimeKeys(sceneAnimation_);

        UpdateTimeGroups();
        UpdateTimeLines();

        SubscribeToEvent(sceneAnimation_, "SceneAnimationFinished", "HandleSceneAnimationFinished");
        SubscribeToEvent(sceneAnimation_, "SceneAnimationTimeLinesUpdated", "HandleTimeLinesUpdated");

        if (sceneAnimation_.get_node().get_enabled())
            sceneAnimation_.SetTime(sceneAnimation_.GetTime());

        Show();
    }

    void UpdateSelection()
    {
        get_log().Info("Scene2DAnimator::UpdateSelection");

        if (!editComponents.empty)
        {
            uint numEditableComponents = editComponents.length;

            SceneAnimation2D@ animation = null;
            for (uint i = 0; i < numEditableComponents; ++i)
            {
                if (GetTypeName(editComponents[i].type) == "SceneAnimation2D")
                {
                    /* get_log().Info("Scene2DAnimator::UpdateSelection " + GetTypeName(editComponents[i].type)); */
                    animation = editComponents[i];
                    break;
                }
            }

            SetSceneAnimation2D(animation);
        }

        if (sceneAnimation_ is null)
            Hide();

        if (window.visible)
        {
            cameraNode_ = GetObjectNode("cam1");

            if (cameraNode_ !is null)
            {
                previewCamera = cameraNode_.GetComponent("Camera");
                InstantiateCameraPreview();
            }
        }
    }

    void ResetTime()
    {
        get_log().Info("Scene2DAnimator::ResetTime");

        Array<Component@>@ sceneanimations = editorScene.GetComponents("SceneAnimation2D", true);
        for (uint i = 0; i < sceneanimations.length; ++i)
        {
            SceneAnimation2D@ sceneAnimation = sceneanimations[i];
            sceneAnimation.SetTime(0.f);
        }
    }

    void Show()
    {
        window.visible = true;
        window.BringToFront();
    }

    void Hide()
    {
        window.visible = false;
    }

    Menu@ CreateTimelineActionMenu(String text, String handler, VariantMap& eventData)
    {
        Menu@ menu = Menu();
        menu.defaultStyle = uiStyle;
        menu.style = AUTO_STYLE;
        menu.SetLayout(LM_HORIZONTAL, 0, IntRect(8, 2, 8, 2));
        Text@ menuText = Text();
        menuText.style = "EditorMenuText";
        menu.AddChild(menuText);
        menuText.text = text;
        menuText.autoLocalizable = false;
        SubscribeToEvent(menu, "Released", handler);

        menu.get_vars() = eventData;

        return menu;
    }

    // return the indexes of the timeline and object in SceneAnimation2D Component
    int GetAnimationTimeLineIndex(int& objectindex)
    {
        int timelineindex = -1;
        for (uint i = 0; i < timeLinesIndexes.length; ++i)
        {
            if (timeLinesIndexes[i] > objectindex)
                break;
            timelineindex = i;
        }

        if (timelineindex != -1)
            objectindex -= (timeLinesIndexes[timelineindex]+1);

        return timelineindex;
    }

    void ShowSerializableInInspector(Array<Serializable@>@ serializables)
    {
        if (serializables.length == 0)
            return;

        ClearSceneSelection();
        DeleteAllContainers();

        bool fullUpdate = false;
        Array<Serializable@> serializable;
        serializable.Resize(1);
        for (uint i=0; i < serializables.length; ++i)
        {
            if (serializables[i] is null)
                continue;

            UIElement@ container = GetComponentContainer(i);
            Text@ serializableTitle = container.GetChild("TitleText");

            serializable[0] = serializables[i];
            Component@ component = cast<Component>(serializable[0]);
            if (component !is null)
                serializableTitle.text = GetComponentTitle(serializable[0]);
            else
                serializableTitle.text = serializable[0].typeName;

            IconizeUIElement(serializableTitle, serializable[0].typeName);
            SetIconEnabledColor(serializableTitle, true);
            UpdateAttributes(serializable, container.GetChild("AttributeList"), fullUpdate);
            SetAttributeEditorID(container.GetChild("ResetToDefault", true), serializable);
        }

        HandleWindowLayoutUpdated();
    }

    void ShowSceneObjectInInspector(UIElement@ element)
    {
        if (sceneAnimation_ is null)
            return;

        int objectindex = timeLineGroups.FindItem(element);
        int timelineindex = GetAnimationTimeLineIndex(objectindex);

        if (timelineindex != -1)
        {
            Array<Serializable@> serializables;
            if (objectindex != -1)
            {
                serializables.Push(sceneAnimation_.GetTimeLineObject(timelineindex, objectindex));
            }
            else
            {
                serializables.Push(sceneAnimation_.GetTimeLine(timelineindex));
            }

            ShowSerializableInInspector(serializables);
        }
    }

    void ShowSceneActionInInspector(VariantMap& eventData)
    {
        if (sceneAnimation_ is null)
            return;

        int timelineindex = eventData["Element"].GetInt();
        int actionindex = eventData["Selection"].GetInt();
        if (timelineindex != -1 && actionindex != -1)
        {
            Array<Serializable@> serializables;
            serializables.Push(sceneAnimation_.GetTimeLineAction(timelineindex, actionindex));
            serializables.Push(sceneAnimation_.GetTimeLineActionPath(timelineindex, actionindex));
            serializables.Push(sceneAnimation_.GetTimeLineActionSpeedCurve(timelineindex, actionindex));
            ShowSerializableInInspector(serializables);
        }
    }

    void HandleSceneAnimationFinished(StringHash eventType, VariantMap& eventData)
    {
        get_log().Info("Scene2DAnimator::HandleSceneAnimationFinished");

        StopSceneUpdate();

        toolBarDirty = true;
        editorScene.set_elapsedTime(0);
    }

    void HandleTimeLinesUpdated(StringHash eventType, VariantMap& eventData)
    {
        get_log().Info("Scene2DAnimator::HandleTimeLinesUpdated");

        UpdateTimeGroups();
    }

    void HandleTimeLineGroupClick(StringHash eventType, VariantMap& eventData)
    {
        if (eventData["Button"].GetInt() == MOUSEB_RIGHT)
        {
            // Opens a contextual menu for timeline
            UIElement@ uiElement = eventData["Item"].GetPtr();
            Array<UIElement@> actions;
            if (uiElement.get_indent() == 2)// timelineContainer.get_indent())
            {
                //get_log().Info("Scene2DAnimator::HandleTimeLineClick ident=" + uiElement.get_indent());
                actions.Push(CreateTimelineActionMenu("Insert New TimeLine", "HandleTimeLineInsert", eventData));
                actions.Push(CreateTimelineActionMenu("Remove", "HandleTimeLineRemove", eventData));
                actions.Push(CreateTimelineActionMenu("Add Node Objects", "HandleTimeLineAddObjects", eventData));
                ActivateContextMenu(actions);
            }
            else if (uiElement.get_indent() == 3)//timelineContainer.get_indent()+1)
            {
                actions.Push(CreateTimelineActionMenu("Remove Node Object", "HandleTimeLineRemoveObject", eventData));
                ActivateContextMenu(actions);
            }
        }
        else if (eventData["Button"].GetInt() == MOUSEB_LEFT)
        {
            UIElement@ uiElement = eventData["Item"].GetPtr();
            //if (uiElement.get_indent() == 3)
            {
                ShowSceneObjectInInspector(uiElement);
            }
        }
    }

    void HandleTimeLineModClicked(StringHash eventType, VariantMap& eventData)
    {
        /* get_log().Info("Scene2DAnimator::HandleTimeLineModClick !"); */

        if (eventData["Element"].GetInt() == -1)
            return;

        Array<UIElement@> actions;
        if (eventData["Selection"].GetInt() != -1)
        {
            ShowSceneActionInInspector(eventData);
            actions.Push(CreateTimelineActionMenu("Remove Action", "HandleTimeLineRemoveAction", eventData));
        }
        else
        {
            actions.Push(CreateTimelineActionMenu("Insert New Action", "HandleTimeLineAddAction", eventData));
        }

        ActivateContextMenu(actions);
    }

    void HandleTimeLineModSelected(StringHash eventType, VariantMap& eventData)
    {
        ShowSceneActionInInspector(eventData);
    }

    void HandleTimeLineInsert(StringHash eventType, VariantMap& eventData)
    {
        if (sceneAnimation_ is null)
            return;

        UIElement@ element = eventData["Element"].GetPtr();
        Text@ timelineui = element.vars["Item"].GetPtr();
        if (timelineui is null)
            return;

        // TODO
        // Get the index in the ListView
        // Get existing default names "TimelineXX"
        Array<String>@ timeLineNames = sceneAnimation_.GetTimeLineNames();
        uint numNames = timeLineNames.length;
        String newname("Timeline");
        Array<uint> existingindices;
        uint index=0;
        for (uint i = 0; i < numNames; ++i)
        {
            if (timeLineNames[i].StartsWith(newname, true))
                existingindices.Push(timeLineNames[i].Substring(newname.length).ToUInt());
            if (timeLineNames[i] == timelineui.name)
                index = i;
        }
        // Set the default name of the new Timeline with the proper indice
        uint nameindice = 0;
        if (existingindices.length > 0)
        {
            existingindices.Sort();
            for (uint i = 0; i < existingindices.length; ++i)
            {
                if (existingindices[i] > nameindice+1)
                    break;
                nameindice = existingindices[i];
            }
        }
        newname.opAddAssign(nameindice+1);

        sceneAnimation_.AddTimeLine(index, newname);
        InsertTimeGroup(timeLineGroups.FindItem(timelineui), newname);
        timelineModifier.AddTimeLine(index, newname);

        UpdateTimeLines();

        CloseContextMenu();
    }

    void HandleTimeLineRemove(StringHash eventType, VariantMap& eventData)
    {
        if (sceneAnimation_ is null)
            return;

        UIElement@ element = eventData["Element"].GetPtr();
        Text@ timelineui = element.vars["Item"].GetPtr();
        if (timelineui is null)
            return;

        sceneAnimation_.RemoveTimeLine(timelineui.name);
        timeLineGroups.RemoveItem(timelineui);
        timelineModifier.RemoveTimeLine(timelineui.name);

        UpdateTimeLines();
        CloseContextMenu();
    }

    void HandleTimeLineAddObjects(StringHash eventType, VariantMap& eventData)
    {
        if (sceneAnimation_ is null)
            return;

        UIElement@ element = eventData["Element"].GetPtr();
        Text@ timelineui = element.vars["Item"].GetPtr();
        if (timelineui is null)
            return;

        int eltindex = timeLineGroups.FindItem(timelineui);
        int objectindex = eltindex;
        int timelineindex = GetAnimationTimeLineIndex(objectindex);

        for (uint i=0; i < selectedNodes.length; ++i)
        {
            sceneAnimation_.AddNodeObject(selectedNodes[i], timelineindex, objectindex+i+1);
            InsertTimeGroup(eltindex+i+1, selectedNodes[i].name, timelineui);
        }
        timelineModifier.UpdateTimeLines();
        UpdateTimeLines();
        CloseContextMenu();
    }

    void HandleTimeLineRemoveObject(StringHash eventType, VariantMap& eventData)
    {
        if (sceneAnimation_ is null)
            return;

        UIElement@ element = eventData["Element"].GetPtr();
        Text@ objectui = element.vars["Item"].GetPtr();
        if (objectui is null)
            return;

        int objectindex = timeLineGroups.FindItem(objectui);
        int timelineindex = GetAnimationTimeLineIndex(objectindex);
        sceneAnimation_.RemoveNodeObject(timelineindex, objectindex);
        timeLineGroups.RemoveItem(objectui);
        timelineModifier.UpdateTimeLines();
        UpdateTimeLines();
        CloseContextMenu();
    }

    void HandleTimeLineAddAction(StringHash eventType, VariantMap& eventData)
    {
        UIElement@ element = eventData["Element"].GetPtr();
        int timelineindex = element.vars["Element"].GetInt();
        float time = element.vars["item"].GetFloat();
        get_log().Info("Scene2DAnimator::HandleTimeLineAddAction : timeline=" + timelineindex + " time=" + time);
        sceneAnimation_.AddAction(timelineindex, time);
        timelineModifier.AddTimeKey(timelineindex, time);
        CloseContextMenu();
    }

    void HandleTimeLineRemoveAction(StringHash eventType, VariantMap& eventData)
    {
        UIElement@ element = eventData["Element"].GetPtr();
        int timelineindex = element.vars["Element"].GetInt();
        int timekey = element.vars["Selection"].GetInt();
        get_log().Info("Scene2DAnimator::HandleTimeLineRemoveAction : timeline=" + timelineindex + " timekey=" + timekey);
        sceneAnimation_.RemoveAction(timelineindex, timekey);
        timelineModifier.RemoveTimeKey(timelineindex, timekey);
        CloseContextMenu();
    }
}

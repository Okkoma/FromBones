//
// Copyright (c) 2022 OkkoStudio.
//

package com.okkostudio.frombones;

import android.content.Intent;
import android.os.Bundle;

import java.util.List;

import io.urho3d.UrhoActivity;

public class FromBones extends UrhoActivity {

    private static final String TAG = "FromBones";

    @Override
    protected void onLoadLibrary(List<String> libraryNames) {
		libraryNames.add(TAG);
		super.onLoadLibrary(libraryNames);
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        Intent launchIntent = getIntent();
        if(launchIntent.getAction().equals("com.google.intent.action.TEST_LOOP")) {
            int scenario = launchIntent.getIntExtra("scenario", 0);

            // handle the test game loop here
        }
    }
}

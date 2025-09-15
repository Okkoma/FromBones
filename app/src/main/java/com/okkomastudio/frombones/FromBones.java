//
// Copyright (c) 2022-2025 Okkoma Studio.
//


package com.okkomastudio.frombones;

import java.util.List;
import android.content.Intent;
import android.annotation.SuppressLint;
import android.os.Bundle;

import io.urho3d.UrhoActivity;

@SuppressLint("SetTextI18n")

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

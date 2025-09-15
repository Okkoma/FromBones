# FromBones

![presentation](app/snapshots/snapshots.gif)

A 2D plateformer game with integrated editor.
Up to 4 local players.

[![build](../../actions/workflows/cmake-multi-platform.yml/badge.svg?branch=master)](../../actions/workflows/cmake-multi-platform.yml)


## Build Prerequisites

- **CMake** installed.
- Install the dependencies for **U3D**. Official documentation here: [Urho3D Build Instructions](https://u3d.io/docs/_building.html).


## Native Build Instructions

 Use CMake to generate the build files.

    cmake -S . -B build
    cmake --build build --config Release


## Android Build Instructions

 Use the gradle build script:
 
    ./gradlew bundle
    
    
## Native Installation

Once the build is complete, you can install the project using the appropriate installation command.

    cmake --install build

Default installation path: ./exe/bin


## Third-Party

- Fork of Urho3D1.7 engine : https://github.com/Okkoma/Urho3D-FromBones/ for origin project see https://urho3d.io/
- Accidental Noise : https://accidentalnoise.sourceforge.net/ by Joshua Tippetts.
- Wren : https://wren.io/
- FastPoly2tri : https://github.com/MetricPanda/fast-poly2tri/
- Clipper2 : https://angusj.com/clipper2/
- dlfcn : https://github.com/dlfcn-win32/dlfcn-win32/


## Platforms

**Tested:** Windows, Linux, Android


## License

For more details, see the LICENSE file.


## To Do

See notes folders.

Finalize vk port and network.

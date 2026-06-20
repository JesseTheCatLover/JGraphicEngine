# RedleafEngine

Redleaf is a Cross-Platform, Rendering-First, Source-Available Game Engine built on JEngine Technology.

The engine is currently in Alpha1 development, with its editor in EditorPrototype phase.

![Overview Photo1](https://i.postimg.cc/MHQV95Yx/Screenshot-2026-06-20-at-4-42-20-PM.png)
![Overview Photo2](https://i.postimg.cc/zGWd0jcB/11D33136-C354-4A37-8323-DCD745C36D5B-1-201-a.jpg)
<sub><a href="https://sketchfab.com/3d-models/the-armoury-adbe88dcf04d4a87a4c2f62ed6ca0880">3D Model Source</a></sub>

## Philosophy

Redleaf is built around a core principle:

> "Power without complexity"

Game development is an iterative process. The engine is structured to keep that loop direct: make a change, run the project, observe the result, and adjust.

To support different development styles, Redleaf exposes multiple levels of control.

At the highest level, developers use standard high-level engine APIs to perform common tasks quickly and consistently.

At a lower level, functionality can be extended through components and user-defined systems, allowing behavior to be composed rather than hardcoded.

At the lowest level, the engine remains fully open for direct extension of core systems when higher-level abstractions are not sufficient.

This structure is intended to let developers work at the level that best fits the task, without forcing a single workflow across all projects.

## Architecture

Redleaf is a module-based engine with clear separation between subsystems and ownership boundaries.

Functionality is split into independent modules responsible for specific areas of the engine, such as runtime systems, rendering, resources, and tooling. Modules communicate through explicit interfaces rather than shared state.

The engine is designed to be cross-platform. Platform and rendering functionality are abstracted behind backend interfaces, allowing implementations to be swapped without affecting core engine code.

Resource management is handle-based. Engine systems operate on lightweight references, while ownership and lifetime are handled internally by dedicated systems.

## Gallery

#### Editor Prototype Early Development:

![GalleryPhoto1](https://i.postimg.cc/yYCZVFT2/Screenshot-2026-06-20-at-4-39-56-PM.png)
![GalleryPhoto2](https://i.postimg.cc/15pfv1MG/Screenshot-2026-06-20-at-4-41-15-PM.png)

## Requirements

Redleaf requires the following tools to build from source:

* CMake 3.15 or newer
* A C++20 compatible compiler
* Rust (Cargo)
* Python 3
* Any CMake-compatible IDE or generator of your choice

Supported development environments include:

* Visual Studio 2022
* CLion
* Xcode
* Any IDE with CMake support

## Building

Clone the repository:

```bash
git clone --recurse-submodules https://github.com/JesseTheCatLover/RedleafEngine.git
cd RedleafEngine
```

If you already cloned without `--recurse-submodules`, initialize them with:

```bash
git submodule update --init --recursive
```

Generate project files:

```bash
cmake -S . -B Build
```

Build:

```bash
cmake --build Build
```


CMake automatically builds the required tools and runs reflection generation and other tools during the build process. Rust and Python are required dependencies and must be available in PATH.

## License

RedleafEngine is distributed under the RedleafEngine End User License Agreement (RE-EULA).

Read the [EULA](./EULA.md) file, version 1.0.

---

RedleafEngine — JEngine Technology
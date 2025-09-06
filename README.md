# JGraphicEngine  

A personal graphics engine built with **C++ and OpenGL**, created as a way to explore and learn modern rendering techniques.  
It’s a work-in-progress project that grows over time as I experiment with new features.  

## Features  
- Written in C++ with OpenGL  
- Basic rendering pipeline  
- Modern rendering practices (in progress)  
- Expandable and evolving with new experiments  

![Overview Photo](https://i.postimg.cc/8PR63pJ6/2025-08-30-12-03-41-2.jpg)
<sub><a href="https://sketchfab.com/3d-models/the-armoury-adbe88dcf04d4a87a4c2f62ed6ca0880">3D Model Source</a></sub>

## Getting Started  
Clone the repo and build with CMake:  

```bash
git clone https://github.com/JesseTheCatLover/JGraphicEngine.git
cd JGraphicEngine
mkdir build && cd build
cmake ..
make
```

## 🎮 Demos  

The `/Demos` folder contains standalone programs that **showcase the features of the engine** and reflect the **progress of the project over time**. Each demo focuses on a specific concept or milestone:

- **Demo01_FlyingCamera.cpp** *(First Steps)*  
  Introduced a free camera system with movement and look controls — the foundation for exploring 3D space.  

- **Demo02_ModelLoading.cpp** *(Expanding the World)*  
  Added support for importing and rendering external 3D models, moving beyond primitives.  

- **Demo03_PostProcessing.cpp** *(Visual Effects & Screen Shaders)*  
  Added a post-processing system with effects like grayscale, pixelation, and scanlines to extend the engine’s rendering capabilities.

---

### ▶️ Running a Demo  

After building with **CMake**, demos can be run directly from the build directory:  

```bash
./Demo01_FlyingCamera
./Demo02_ModelLoading
./Demo03_PostProcessing

# Barbü

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

![screenshot](https://i.imgur.com/3gtP0iJ.png)

Barbü is a prototyping framework for real-time 3D graphics aimed at simulating hair & stuffs.

It is written in **C++ 17** and **OpenGL 4.6**.

Check the gallery [here](https://imgur.com/a/MgJyFNG), or take a [sneak peek](https://imgur.com/ZyjYbiN).

## Some Features

* Entity-Components based system with live scene edit.
* Hot-reload of external assets (*textures, models, shaders*).
* PBR pipeline with Image Based Lighting.
* Postprocessing with tonemapping and horizontal based SSAO.
* Marschner's hair reflectance model GPU implementation.
* Dynamic GPU particles & hair simulation.
* GPU skinning animation via Dual Quaternion blending.
* GLTF 2.0 & OBJ drag-n-drop model import.
* Features rich camera controls.
* Colourful console logger with context tracking !

## Quickstart

We will be using the command-line on Unix and [Git Bash](https://git-for-windows.github.io/) on Windows.

### Dependencies

The following third parties are used :

* [GLFW (3.3.4)](https://github.com/glfw/glfw/tree/3.3.4) : window management,
* [GLM (0.9.9.8)](https://github.com/g-truc/glm/releases/tag/0.9.9.8) : linear algebra.
* [imgui (1.82)](https://github.com/ocornut/imgui/tree/v1.82) : user interface.
* [im3d (1.16)](https://github.com/john-chapman/im3d/) : gizmo controller.
* [stb_image (2.26)](https://github.com/nothings/stb) : image loader.
* [cgltf (1.10)](https://github.com/jkuhlmann/cgltf) : gltf 2.0 loader.
* [MikkTSpace](https://github.com/mmikk/MikkTSpace) : tangent space computation.

Some of them are shipped directly, others must be retrieved as submodule :
```bash
git submodule init
git submodule update
```

### Build

We will first create a build directory then generate the CMake cache depending on your system.

```bash
mkdir BUILDs && cd BUILDs
```

On **Unix**, using Makefile (*replace `$NUM_CPU` by the number of core you  want to use*) :
```bash
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -- -j$NUM_CPU
```

On **Windows**, using MSVC 15 for x64:
```bash
cmake .. -G "Visual Studio 15 2017 Win64"
cmake --build . --target ALL_BUILD --config Release
```

*Notes:*

 1. *Using CMake, the build configuration type (ie. Debug, Release) is set at Build Time with MSVC and at Cache Generation Time with Makefile.*

 2. *OpenGL extensions are generated automatically by a custom [Python](https://www.python.org/downloads/) script.  Alternatively [GLEW](http://glew.sourceforge.net/) can be used by specifying the option `-DOPT_USE_GLEW=ON` to CMake. __If something does not compile due to OpenGL functions, try to use GLEW instead.__*

### Run

The binary can be found in the project `./bin/` directory:
```bash
../bin/barbu
```

## References

* ["An Efficient Representation for Irradiance Environment Maps"](http://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.458.6377&rep=rep1&type=pdf), Ramamoorthi, Ravi, and Pat Hanrahan. _Proceedings of the 28th annual conference on Computer graphics and interactive techniques_. 2001.

* ["Fast Simulation of Inextensible Hair and Fur"](https://matthias-research.github.io/pages/publications/FTLHairFur.pdf). M. Müller, T.J. Kim, N. Chentanez. Virtual Reality Interactions and Physical Simulations (VRIPhys 2012).

* ["Geometric Skinning with Approximate Dual Quaternion Blending."](https://www.cs.utah.edu/~ladislav/kavan08geometric/kavan08geometric.pdf), Kavan, Ladislav, et al. ACM Transactions on Graphics (TOG) 27.4 (2008): 1-23.

* ["Hair Animation and Rendering in the Nalu Demo"](https://developer.nvidia.com/gpugems/gpugems2/part-iii-high-quality-rendering/chapter-23-hair-animation-and-rendering-nalu-demo). Hubert Nguyen, William Donnelly, GPU Gems 2, Chap 23

* ["Light Scattering from Human Hair Fibers"](https://graphics.stanford.edu/papers/hair/). Marschner, S.R., Jensen, H.W., Cammarano, M., Worley, S. and Hanrahan, P., 2003. ACM Transactions on Graphics (TOG), 22(3), pp.780-791.

* ["Practical Applications of Compute for Simulation in Agni's Philosophy"](http://www.jp.square-enix.com/tech/library/pdf/SiggraphAsia2014_simulation.pdf), Napaporn Metaaphanon, GPU Compute for Graphics, ACM SIGGRAPH ASIA 2014 Courses.

* ["Real time hair simulation and rendering on the GPU"](https://developer.download.nvidia.com/presentations/2008/SIGGRAPH/RealTimeHairRendering_SponsoredSession2.pdf). Sarah Tariq, Louis Bavoil. ACM SIGGRAPH 2008 talks.

* ["The Importance of Being Linear"](https://developer.nvidia.com/gpugems/gpugems3/part-iv-image-effects/chapter-24-importance-being-linear). Larry Gritz, Eugene d'Eon, GPU Gems 3, Chap 24.

* ["Uncharted 2: HDR Lighting"](https://gdcvault.com/play/1012351/Uncharted-2-HDR). John Hable, GDC 2012.

* ["Real Shading in Unreal Engine 4"](https://cdn2.unrealengine.com/Resources/files/2013SiggraphPresentationsNotes-26915738.pdf). Brian Karis, Epic Games. 2013.

## License

*Barbü* is released under the *MIT* license.

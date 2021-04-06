# Barbü

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

![screenshot](https://i.imgur.com/qOxWtyo.png)

Barbü is a *work in progress*, fairly GPU-based, prototyping playground for real-time hair simulation & fun.

It is written in **C++ 17** and **OpenGL 4.6**.

## Some Features

* Entity component based hierarchy with live scene edit.
* Hot-reload of externals assets (*textures, models, shaders*).
* Hair strand dynamics using the dynamic follow-the-leader (DFTL) algorithm.
* Marschner's reflectance model implementation on Compute Shaders.
* Tonemapping post-processing with HBAO.
* Irradiance environment map based lighting.

## Quickstart

We will be using the command-line on Unix and [Git Bash](https://git-for-windows.github.io/) on Windows.

### Dependencies

The following dependencies are pulled in as submodules.

- [GLFW XX](https://github.com/glfw/glfw) for window management,
- [GLM XX](https://github.com/g-truc/glm/releases/tag/0.9.8.1) for mathematics.
- [imgui XX](https://github.com/ocornut/imgui) for UI.

To retrieve them, type the following command line in the project directory :
```bash
git submodule init
git submodule update
```

Other dependencies are shipped directly :
* [im3d](https://github.com/john-chapman/im3d/)
* stb_image

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

 2. *OpenGL extensions are generated automatically by a custom [Python](https://www.python.org/downloads/) script.  Alternatively [GLEW](http://glew.sourceforge.net/) can be used by specifying the option `-DOPT_USE_GLEW=ON` to CMake. __If something does not compile due to OpenGL functions, try to use GLEW.__*

### Run

The binary can be found in the project `./bin/` directory:
```bash
../bin/barbu_demo
```

## References

* ["Hair Animation and Rendering in the Nalu Demo"](https://developer.nvidia.com/gpugems/gpugems2/part-iii-high-quality-rendering/chapter-23-hair-animation-and-rendering-nalu-demo). Hubert Nguyen, William Donnelly, GPU Gems 2, Chap 23

* ["Real time hair simulation and rendering on the GPU"](https://developer.download.nvidia.com/presentations/2008/SIGGRAPH/RealTimeHairRendering_SponsoredSession2.pdf). Sarah Tariq, Louis Bavoil. ACM SIGGRAPH 2008 talks.

* ["Practical Applications of Compute for Simulation in Agni's Philosophy"](http://www.jp.square-enix.com/tech/library/pdf/SiggraphAsia2014_simulation.pdf), Napaporn Metaaphanon, GPU Compute for Graphics, ACM SIGGRAPH ASIA 2014 Courses.

* ["Fast Simulation of Inextensible Hair and Fur"](https://matthias-research.github.io/pages/publications/FTLHairFur.pdf). M. Müller, T.J. Kim, N. Chentanez. Virtual Reality Interactions and Physical Simulations (VRIPhys 2012).

* ["Light Scattering from Human Hair Fibers"](https://graphics.stanford.edu/papers/hair/). Marschner, S.R., Jensen, H.W., Cammarano, M., Worley, S. and Hanrahan, P., 2003. ACM Transactions on Graphics (TOG), 22(3), pp.780-791.

* ["The Importance of Being Linear"](https://developer.nvidia.com/gpugems/gpugems3/part-iv-image-effects/chapter-24-importance-being-linear). Larry Gritz, Eugene d'Eon, GPU Gems 3, Chap 24.

* ["Uncharted 2: HDR Lighting"](https://gdcvault.com/play/1012351/Uncharted-2-HDR). John Hable, GDC 2012.

* ["An Efficient Representation for Irradiance Environment Maps"](http://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.458.6377&rep=rep1&type=pdf), Ramamoorthi, Ravi, and Pat Hanrahan. _Proceedings of the 28th annual conference on Computer graphics and interactive techniques_. 2001.

## License

*Barbü* is released under the *MIT* license.

# Atlas

![GitHub contributors](https://img.shields.io/github/contributors/maxvdec/atlas)
![GitHub last commit](https://img.shields.io/github/last-commit/maxvdec/atlas)
![Tests](https://github.com/maxvdec/atlas/actions/workflows/build.yaml/badge.svg)
![GitHub Issues or Pull Requests](https://img.shields.io/github/issues/maxvdec/atlas)
![GitHub Repo stars](https://img.shields.io/github/stars/maxvdec/atlas)
[![](https://dcbadge.limes.pink/api/server/WKrxKtr7kW)](https://discord.gg/WKrxKtr7kW)

Atlas is a Game Engine that uses the latest technologies to provide a fast, simple but powerful experience for developers.
It is built with C++ and uses OpenGL and Vulkan for rendering, with plans to support Metal in the future.

![Atlas Screenshot](example.png)

## Features

- Cross-platform support (Windows, macOS, Linux)
- Fast graphics rendering with OpenGL and Vulkan (via Opal)
- Simple and intuitive API
- Physics engine (Bezel)
- Audio engine (Finewave)
- Terrain system (Aurora)
- Environment system (Hydra)
- Debugging system (Tracer)
- Asset management
- Intuitive C++ scripting mode

## Engine Architecture

- The main renderer is Atlas. Atlas is the one in charge of the graphics rendering, input handling, window management and more.
- The physics engine is Bezel. Bezel is a separate library that handles all the physics calculations and simulations.
- The audio engine is Finewave. Finewave is a separate library that handles all the audio playback and processing.
- The terrain system is Aurora. Aurora is a separate library that handles terrain generation and rendering.
- The environment system is Hydra. Hydra is a separate library that handles sky, atmosphere, weather and fluid simulation.
- The rendering backend is Opal, an in-house rendering abstraction layer that allows for easy switching between different graphics APIs.
- The debugging system is Tracer. Tracer is a module in the engine that provides various debugging tools and utilities.

## Getting the Engine

Go to the releases page [here](https://github.com/maxvdec/atlas/releases) to download the latest version of Atlas. Then you can follow the instructions in the documentation to get started.

```bash
atlas create myProject
```

## Building from Source

To build the engine from source, you will need to have CMake and a C++ compiler installed on your system. Then you can clone the repository and build the engine using the following commands:

```bash
git clone https://github.com/maxvdec/atlas.git
cd atlas
mkdir build
cd build
cmake ..
make
```

You can then run the engine using the following command:

```bash
./Atlas
```

Be sure to check the documentation for more detailed instructions on building and running the engine.

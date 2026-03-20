# How does the scene packing format work?

Atlas divides the important parts of a scene in files. In this case we have two formats for this first version:

* Scene format files (`.ascene`). These are JSON files that basically pack in it the information of the scene, such as the entities, components and values for those components.

* Material files (`.amat`). These are JSON files that pack the information of the materials, such as the textures, colors and other properties.

* Graphite Theme files (`.gtheme`). These are JSON files that pack the information of the Graphite themes, such as the colors, fonts and other properties.

* Terrain Generator files (`.atgen`). These are JSON files that pack the information of the terrain generators, such as the heightmap, textures and other properties.

* Biome files (`.abiome`). These are JSON files that pack the information of the biomes, such as the terrain generator, materials and other properties.

The idea is that the scene format files will reference the material files, so that we can reuse materials across different scenes. This way, we can have a more modular and efficient way of managing our assets.

## How do I understand a scene format file?
To understand a scene format file, start looking through the documentation at this directory and `/docs`, and then look at the code in this directory and `/tests`.
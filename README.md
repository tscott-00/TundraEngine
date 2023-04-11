# TundraEngine

Tundra Engine is a game engine meant for very large coordinate scales, such as realistically sizes planets. Tundra Engine contains Physically Based Rendering (PBR) tools, such as a renderer for basic objects and a system for creating environment maps dynamically. The pipeline also readily supports screen space reflections and refractions. Bullet integration is built-in through the use of game components, but it is not hardcoded so other physics engines can be integrated. Object transforms are stored with double precision on the CPU, then transformation matrices are mapped to be camera-local prior to GPU upload to preserve fidelity on large coordinate scales.

Here are some screenshots from the project this engine was created for:
![plot](./SpaceHorses_Terrain.png)
![plot](./SpaceHorses_Atmosphere.png)
![plot](./SpaceHorses_Galaxy.png)

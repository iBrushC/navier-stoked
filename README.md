# **Navier Stoked**: *A Platform Fighter Based on Realtime Fluid*

![screenshot](https://raw.githubusercontent.com/iBrushC/navier-stoked/refs/heads/main/ss/icon.png)

### *Important Note*
This is a tech demo more than anything else. I threw it together in about two weeks for a university final project, so it's quite rough around the edges. Most of the code is fairly clean though, so if someone did have interest in modifying it further, they could.

## Dependencies
Building requires basically any C compliler, Raylib 5.5 (other versions not tested), Physac 2D physics header, and Raygui header if you want to recompile shaders through the debug GUI. Other than support for relatively modern OpenGL, the requirements to run it should be quite low. It can normally get 50-60 FPS on a 2021 Thinkpad with integrated graphics.

## Gameplay
Each player requires a controller with two joysticks, two triggers, and two bumpers, so any Xbox or Playstation controller should do. The mappings are

| Input | Function |
| -- | -- |
| **Left Stick** | movement, including jumping by tilting upward |
| **Right Stick** | attack direction, no connection to movement inputs |
| **Right Trigger** | flamethrower |
| **Left Trigger** | death-beam, charge and release attack |
| **Right Bumper** | dash in attack direction |
| **Left Bumper** | shield in attack direction |

## Technical Specs
Fluid is an grid-based shader implementation taken from the paper *Simple and Fast Fluids* by *Martin Guay, Fabrice Colin,* and *Richard Egli*. It's computed and rendered by two different shaders. Computation runs multiple times (currently 6) per frame to have a stable but fast result. Right now the fluid is a bit lacking because I didn't want to implement another double-buffering system to implement dyes, so the rendering is entirely based on velocity. 

The fluid buffer is sent back to the CPU every other frame because I found every frame to be a little too slow and unstable on my machine. For interaction with players, the velocity is sampled at the center and corners then averaged and added as a force (yes I know this doesn't make a lot of sense). To interact with the fluid, the fluid buffer is rendered to with the desired emitter data. Because raylib can only perform drawing routines with 8 bit RGBA integers, rendered data is sent with an alpha value of less than one. The shader takes any sub-one alpha value pixels and normalizes them (e.g. 0 to 255 becomes -1.0 to 1.0)

Additional notes include the use of 16 bit RGBA textures, the inclusion of some vorticity confinement, and tweaked non-physically-accurate values to make the fluid more exciting to fight with.

#include <stdio.h>
#include <stdlib.h>

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#include "gameobjects.h"
#include "fluid.h"

#define SCREEN_WIDTH (2560)
#define SCREEN_HEIGHT (1600)

#define GRAVITY (6)

#define DEBUG_MODE (0)

//----------------------------------------------------------------------------------
// Structs
//----------------------------------------------------------------------------------

// Scene struct should make it easy to add levels, features, multiple players, etc etc.
typedef struct Scene {
    long long int t;    // Time counter

    // Camera
    Camera2D* camera;

    // Players
    Player players[4];
    int player_count;

    // Environment
    FluidBody fluid;
    EnvironmentObj environment[50]; // Arbitrary limit because I don't want to deal with dynamic memory allocation
    int environment_obj_count;
} Scene;

//----------------------------------------------------------------------------------
// Local Functions Declaration
//----------------------------------------------------------------------------------
static void initScene(Scene* scene, int player_count);
static void unloadScene(Scene* scene);

static void update(Scene* scene);                   // Full Frame Update Loop
static void frameUpdateInputs(Scene* scene);        // Use input
// static void frameUpdateState(Scene* scene);     // Updates between menus and screens
static void frameUpdateCamera(Scene* scene);        // Update the camera position and rotation
static void frameUpdateFluid(Scene* scene);         // Update the fluid
static void frameUpdatePhysics(Scene* scene);   // Update the physics of all objects
static void frameDrawPhysicsBodies(Scene* scene);   // A debug mode to draw all hitboxes
static void frameDrawDebugGUI(Scene* scene);
static void frameDrawFrame(Scene* scene);           // Draw frame objects
// static void framePostProcess(Scene* scene);     // Draw post processing steps

int toggle = 1;

//----------------------------------------------------------------------------------
// Main entry point
//----------------------------------------------------------------------------------
int main()
{
    // Initialization
    //--------------------------------------------------------------------------------------

    // UPDATE THIS WITH EVERY SUCCESSFUL BUILD
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Navier Stoked 0.1.25");

    // Create scene
    Scene scene;
    initScene(&scene, 2);

    SetTargetFPS(60);
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!WindowShouldClose())    // Detect window close button or ESC key
    {
        update(&scene);
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    unloadScene(&scene);
    ClosePhysics();    // End physics 
    CloseWindow();     // Close window and OpenGL context

    return 0;
}

// Initialize camera, objects, players, etc
static void initScene(Scene* scene, int player_count) {
    // Camera
    Camera2D* camera = malloc(sizeof(Camera*));
    
    camera->target = (Vector2){};
    camera->offset = (Vector2){SCREEN_WIDTH / 2, 2*SCREEN_HEIGHT / 3};
    camera->rotation = 0;
    camera->zoom = 1;

    scene->camera = camera;

    // Physics
    InitPhysics();
    SetPhysicsGravity(0, GRAVITY);

    // Scene Objects
    EnvironmentObj mainplatform = createEnvironmentBox(
        (Vector2){0, 100},
        2000, 20, 0, 1, false, BLUE
    );
    scene->environment[0] = mainplatform;
    EnvironmentObj secondleft = createEnvironmentBox(
        (Vector2){500, -100},
        400, 20, 0, 1, false, BLUE
    );
    scene->environment[1] = secondleft;
    EnvironmentObj secondright = createEnvironmentBox(
        (Vector2){-500, -100},
        400, 20, 0, 1, false, BLUE
    );
    scene->environment[2] = secondright;
    EnvironmentObj thirdleft = createEnvironmentBox(
        (Vector2){1000, -500},
        300, 20, 0, 1, false, BLUE
    );
    scene->environment[3] = thirdleft;
    EnvironmentObj thirdright = createEnvironmentBox(
        (Vector2){-1000, -500},
        300, 20, 0, 1, false, BLUE
    );
    scene->environment[4] = thirdright;
    EnvironmentObj topplatform = createEnvironmentBox(
        (Vector2){0, -300},
        500, 20, 0, 1, false, BLUE
    );
    scene->environment[5] = topplatform;
    EnvironmentObj toptopplatform = createEnvironmentBox(
        (Vector2){0, -700},
        250, 20, 0, 1, false, BLUE
    );
    scene->environment[6] = toptopplatform;

    scene->environment_obj_count = 7;

    // Fluid
    scene->fluid = createFluidBody(
        1920, 1080, 0, -500, 
        SCREEN_WIDTH*3, SCREEN_HEIGHT*3
    );

    // Draw fluid boundaries
    BeginTextureMode(scene->fluid.boundary_tex);
        for (int i = 0; i < scene->environment_obj_count; i++) {
            drawEnvironmentObjToFluid(&scene->environment[i], &scene->fluid);
        }
    EndTextureMode();

    // Players
    scene->player_count = player_count;
    for (int i = 0; i < player_count; i++) {
        scene->players[i] = createPlayer((Vector2){4*PLAYER_WIDTH*i, 0}, i);
    }

    // Time
    scene->t = 0;
}

static void unloadScene(Scene* scene) {
    unloadFluidBody(&scene->fluid);
}

float ReverseFloat( const float inFloat )
{
   float retVal;
   char *floatToConvert = ( char* ) & inFloat;
   char *returnFloat = ( char* ) & retVal;

   // swap the bytes into a temporary buffer
   returnFloat[0] = floatToConvert[3];
   returnFloat[1] = floatToConvert[2];
   returnFloat[2] = floatToConvert[1];
   returnFloat[3] = floatToConvert[0];

   return retVal;
}

// Update and draw game frame
static void update(Scene* scene)
{   
    scene->t++;
    
    frameUpdateInputs(scene);
    // frameUpdateState(scene);    // Update the scene
    frameUpdateCamera(scene);   // Update the camera
    frameUpdateFluid(scene);
    if (scene->t > 10) {
        frameUpdatePhysics(scene);  // Update the physics
    }

    // Draw 
    //----------------------------------------------------------------------------------
    BeginDrawing();

    // DrawText(TextFormat("%i", scene->t), 100, 100, 25, BLUE);

    frameDrawFrame(scene);
    // framePostProcess(scene);
    
    // Debug
    if (DEBUG_MODE) {
        frameDrawPhysicsBodies(scene);
        frameDrawDebugGUI(scene);
    }

    EndDrawing();
    //----------------------------------------------------------------------------------

    for (int i = 0; i < scene->player_count; i++) {
        scene->players[i].p_colliding = scene->players[i].physics->isColliding;
    }
}

// Take in all user inputs and update the scene accordingly
static void frameUpdateInputs(Scene* scene) {

    // Update player movements
    for (int i = 0; i < scene->player_count; i++) {
        updatePlayerMovement(&scene->players[i]);
    }
}

// Have camera track players
static void frameUpdateCamera(Scene* scene) {
    Vector2 center = { 0 };
    float zoom_factor = 1;

    // Calculate center
    for (int i = 0; i < scene->player_count; i++) {
        center.x += scene->players[i].physics->position.x + PLAYER_WIDTH / 2;
        center.y += scene->players[i].physics->position.y + PLAYER_HEIGHT / 2;
    }
    center.x /= scene->player_count;
    center.y /= scene->player_count;

    scene->camera->target.x = center.x;
    scene->camera->target.y = center.y;

    // Calculate zoom
    // Ugly solution but I couldn't get standard deviation to work
    for (int i = 0; i < scene->player_count; i++) {
        for (int j = 0; j < scene->player_count; j++) {
            if (i == j) continue;

            float dx = abs(
                scene->players[i].physics->position.x -
                scene->players[j].physics->position.x +
                12*PLAYER_WIDTH
            ) + 1;
            zoom_factor = min(zoom_factor, (float)SCREEN_WIDTH / dx);

            float dy = abs(
                scene->players[i].physics->position.y -
                scene->players[j].physics->position.y +
                4*PLAYER_HEIGHT
            ) + 1;
            zoom_factor = min(zoom_factor, (float)SCREEN_HEIGHT / dy);
        }
    }

    scene->camera->zoom = 
        scene->camera->zoom * 0.6 +
        zoom_factor * 0.4;
}

static void frameUpdateFluid(Scene* scene) {
    float time = (float)scene->t / 60.0;
    setFluidUniforms(&scene->fluid, &time);
    // Need to set this each frame for unknown reasons
    SetShaderValueTexture(scene->fluid.shader, scene->fluid.boundary_uniform, scene->fluid.boundary_tex.texture);
}

// Clamping with sigmoid
float sclamp(float x, float r) {
    return (r / (1 + exp(-0.3*x))) - r/2;
}

static void frameUpdatePhysics(Scene* scene) {
    for (int i = 0; i < scene->player_count; i++) {
        // Add inverse flamethrower force
        if (scene->players[i].flamethower_force > 0.2) {
            PhysicsAddForce(
                scene->players[i].physics,
                (Vector2){
                    -30*scene->players[i].direction.x * scene->players[i].flamethower_force,
                    -30*scene->players[i].direction.y * scene->players[i].flamethower_force,
                }
            );
        }

        // LAUNCH
        if (scene->players[i].death_enabled) {
            PhysicsAddForce(
                scene->players[i].physics,
                (Vector2){
                    -100*scene->players[i].direction.x,
                    -100*scene->players[i].direction.y,
                }
            );
        }

        // Add buffer force
        Vector2 aspects = fluidAspect(&scene->fluid);
        int adj_width = PLAYER_WIDTH / aspects.x;
        int adj_height = PLAYER_HEIGHT / aspects.y;
        Vector2 player_pos = environmentToFluidCoords(scene->players[i].physics->position, &scene->fluid);
        
        int x = player_pos.x;
        int y = (scene->fluid.y_resolution-1) - player_pos.y;
        Vector4 center = getCPUImgValue(&scene->fluid, x, y);
        Vector4 top_left = getCPUImgValue(
            &scene->fluid, 
            x + adj_width, 
            y + adj_height
        );
        Vector4 top_right = getCPUImgValue(
            &scene->fluid, 
            x - adj_width, 
            y + adj_height
        );
        Vector4 bottom_left = getCPUImgValue(
            &scene->fluid, 
            x + adj_width,
            y - adj_height
        );
        Vector4 bottom_right = getCPUImgValue(
            &scene->fluid, 
            x - adj_width,
            y - adj_height
        );

        Vector2 calc_force = {
            (center.x + top_left.x + top_right.x + bottom_left.x + bottom_right.x) / 5.0,
            (center.y + top_left.y + top_right.y + bottom_left.y + bottom_right.y) / 5.0,
        };
        
        PhysicsAddForce(
            scene->players[i].physics,
            (Vector2){
                sclamp(calc_force.x, 750),
                sclamp(calc_force.y, 750),
            }
        );

        // Also do death checking for the players
        if (
            (player_pos.x < 0) || 
            (player_pos.y < 0) || 
            (player_pos.x > scene->fluid.x_resolution) || 
            (player_pos.y > scene->fluid.y_resolution)
        ) {
            scene->players[i].physics->position.x = 4*PLAYER_WIDTH*i;
            scene->players[i].physics->position.y = 0;
            scene->players[i].physics->velocity.x = 0;
            scene->players[i].physics->velocity.y = 0;
        }
    }
}

// An extra pass to draw physics objects
static void frameDrawPhysicsBodies(Scene* scene) {
    // Scene 2D objects
    BeginMode2D(*scene->camera);

    // Draw scene
    int bodiesCount = GetPhysicsBodiesCount();
    for (int i = 0; i < bodiesCount; i++)
    {
        PhysicsBody body = GetPhysicsBody(i);

        if (body != NULL)
        {
            int vertexCount = GetPhysicsShapeVerticesCount(i);
            for (int j = 0; j < vertexCount; j++)
            {
                // Get physics bodies shape vertices to draw lines
                // Note: GetPhysicsShapeVertex() already calculates rotation transformations
                Vector2 vertexA = GetPhysicsShapeVertex(body, j);

                int jj = (((j + 1) < vertexCount) ? (j + 1) : 0);   // Get next vertex or first to close the shape
                Vector2 vertexB = GetPhysicsShapeVertex(body, jj);

                DrawLineV(
                    vertexA, 
                    vertexB, 
                    body->enabled ? BLUE : RED
                );     // Draw a line between two vertex positions
            }
        }
    }

    EndMode2D();
}

// Maybe a return type
static void frameDrawDebugGUI(Scene* scene) {
    float slider_value = 0;

    // Adjust K value
    GuiSlider(
        (Rectangle){40, 60, 120, 20},
        "",
        "Fluid K",
        &slider_value,
        0, 1
    );

    // Adjust Viscosity value
    GuiSlider(
        (Rectangle){40, 100, 120, 20},
        "",
        "Fluid Viscosity",
        &slider_value,
        0, 1
    );

    if (GuiButton((Rectangle){40, 140, 120, 20}, "Recompile Shaders")) {
        scene->fluid.shader = LoadShader(0, "fluid_comp.glsl");
        scene->fluid.render_shader = LoadShader(0, "fluid_render.glsl");
        scene->t = 0;
    }

    return;
}

void playerHandleFlamethrower(Scene* scene, int player_id) {
    Vector2 new_pos = environmentToFluidCoords(
        scene->players[player_id].physics->position,
        &scene->fluid
    );
    Vector2 aspect = fluidAspect(&scene->fluid);

    int radius = PLAYER_WIDTH;
    float x_dir = scene->players[player_id].direction.x;
    float y_dir = scene->players[player_id].direction.y;
    float flame_force = scene->players[player_id].flamethower_force;

    // Handle the flamethrower drawing
    float player_rot = atan2(y_dir, x_dir) * 180 / PI;
    Vector2 dot_pos = {
        new_pos.x, 
        new_pos.y + PLAYER_HEIGHT / aspect.y / 5
    };
    dot_pos.x += x_dir * 70 / aspect.x;
    dot_pos.y -= y_dir * 70 / aspect.y;

    Color flame_direction = {
        x_dir * 127.5 * flame_force + 127,
        y_dir * 127.5 * flame_force + 127,
        0,
        254
    };

    if (flame_force > 0.05) {
        DrawRectanglePro(
            (Rectangle){dot_pos.x, dot_pos.y, radius, 4},
            (Vector2){0, 2},
            -player_rot,
            flame_direction
        );
        // Focus beam
        DrawRectanglePro(
            (Rectangle){
                dot_pos.x + 8*y_dir, 
                dot_pos.y + 8*x_dir, 
                radius, 
                4
            },
            (Vector2){0, 2},
            -player_rot - 15,
            flame_direction
        );
        DrawRectanglePro(
            (Rectangle){
                dot_pos.x - 8*y_dir, 
                dot_pos.y - 8*x_dir, 
                radius, 
                4
            },
            (Vector2){0, 2},
            -player_rot + 15,
            flame_direction
        );
    }
}

void playerHandleBlock(Scene* scene, int player_id) {
    Vector2 new_pos = environmentToFluidCoords(
        scene->players[player_id].physics->position,
        &scene->fluid
    );
    Vector2 aspect = fluidAspect(&scene->fluid);

    // Handle the block drawing
    float player_rot = atan2(scene->players[player_id].direction.y, scene->players[player_id].direction.x) * 180 / PI;
    Vector2 block_pos = {
        new_pos.x, 
        new_pos.y + PLAYER_HEIGHT / aspect.y / 5
    };
    block_pos.x += scene->players[player_id].direction.x * 65 / aspect.x;
    block_pos.y -= scene->players[player_id].direction.y * 65 / aspect.y;

    Color flame_direction = {
        (scene->players[player_id].direction.x * 0.001 * 127.5) + 127,
        (scene->players[player_id].direction.y * 0.001 * 127.5) + 127,
        0,
        255
    };

    if (scene->players[player_id].block_enabled) {
        DrawRectanglePro(
            (Rectangle){block_pos.x, block_pos.y, 3, 40},
            (Vector2){0, 20},
            -player_rot,
            flame_direction
        );
    }
}

void playerHandleDeathbeam(Scene* scene, int player_id) {
    if (scene->players[player_id].death_charge == 0) return;
    
    Vector2 new_pos = environmentToFluidCoords(
        scene->players[player_id].physics->position,
        &scene->fluid
    );
    Vector2 aspect = fluidAspect(&scene->fluid);

    // Draws the "charge dot"
    float x_dir = scene->players[player_id].direction.x;
    float y_dir = scene->players[player_id].direction.y;
    float player_rot = atan2(y_dir, x_dir) * 180 / PI;
    Vector2 beam_pos = {
        new_pos.x, 
        new_pos.y + PLAYER_HEIGHT / aspect.y / 5 + 3
    };
    beam_pos.x += x_dir * 50 / aspect.x;
    beam_pos.y -= y_dir * 50 / aspect.y;

    if ((scene->players[player_id].death_charge > 0.01) && !scene->players[player_id].death_enabled) {
        DrawCircle(
            beam_pos.x,
            beam_pos.y,
            PLAYER_WIDTH / aspect.x / 2,
            (Color){GetRandomValue(0, 255), GetRandomValue(0, 255), 0, 255}
        );
    }

    // If it's enabled, draw a fucking death beam
    if (scene->players[player_id].death_enabled) {
        Color flame_direction = {
            x_dir * 127.5 + 127,
            y_dir * 127.5 + 127,
            0,
            254
        };
        scene->players[player_id].death_charge = max(0, scene->players[player_id].death_charge - 1);

        // Main rectangle
        DrawRectanglePro(
            (Rectangle){beam_pos.x, beam_pos.y, 100, 4},
            (Vector2){0, 2},
            -player_rot,
            flame_direction
        );
        
        // Focus beam
        DrawRectanglePro(
            (Rectangle){
                beam_pos.x + 12*y_dir, 
                beam_pos.y + 12*x_dir, 
                50, 
                4
            },
            (Vector2){0, 2},
            -player_rot - 15,
            flame_direction
        );
        DrawRectanglePro(
            (Rectangle){
                beam_pos.x - 10*y_dir, 
                beam_pos.y - 10*x_dir, 
                50, 
                4
            },
            (Vector2){0, 2},
            -player_rot + 15,
            flame_direction
        );

        if (scene->players[player_id].death_charge == 0) {
            scene->players[player_id].death_enabled = 0;
        }
    }
}

// Draw the frame
static void frameDrawFrame(Scene* scene) {
    // Clear
    ClearBackground((Color){10, 12, 15, 255});
 
    // Screenspace (UI) objects
    DrawFPS(20, 20);
    // int tmp = scene->players[0].physics->isColliding;
    // DrawText(TextFormat("Is colliding? %i", tmp), 20, 140, 20, GREEN);

    // Update the fluid buffer
    for (int i = 0; i < 6; i++) {
        BeginTextureMode(scene->fluid.fluid_tex);
        
        for (int j = 0; j < scene->player_count; j++) {
            playerHandleFlamethrower(scene, j);
            playerHandleDeathbeam(scene, j);
            playerHandleBlock(scene, j);
        }

        if (IsKeyDown(KEY_E)) {
            Vector2 new_pos = environmentToFluidCoords(
                scene->players[1].physics->position,
                &scene->fluid
            );
            DrawRectangle(
                new_pos.x + 15,
                new_pos.y - 3, 
                30, 2, 
                (Color){255, 127, 0, 254}
            );
        }
        if (IsKeyDown(KEY_Q)) {
            Vector2 new_pos = environmentToFluidCoords(
                scene->players[1].physics->position,
                &scene->fluid
            );
            DrawRectangle(
                new_pos.x - 25,
                new_pos.y - 3, 
                30, 2, 
                (Color){0, 127, 0, 254}
            );
        }

        EndTextureMode();
        SetShaderValueTexture(
            scene->fluid.shader, 
            scene->fluid.fluid_uniform, 
            scene->fluid.fluid_tex.texture
        );


        updateFluidBuffer(&scene->fluid);
    }

    // Scene 2D objects
    BeginMode2D(*scene->camera);

    // Draw scene
    for (int i = 0; i < scene->environment_obj_count; i++) {
        drawEnvironmentObj(&scene->environment[i]);
    }

    // Draw players
    for (int i = 0; i < scene->player_count; i++) {
        drawPlayer(&scene->players[i], scene->t);
    }

    // Draw particles

    // Draw fluid
    drawFluidBody(&scene->fluid);

    EndMode2D();
}
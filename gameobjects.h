#ifndef NVST_GAMEOBJECTS
#define NVST_GAMEOBJECTS

#include "raylib.h"

#define PHYSAC_IMPLEMENTATION
#include "physac.h"

#include "fluid.h"

#define PLAYER_WIDTH (25)
#define PLAYER_HEIGHT (75)
#define HORIZ_VELOCITY (2.0)
#define DASH_VELOCITY (2.0)
#define JUMP_VELOCITY (2.0)

//----------------------------------------------------------------------------------
// Structs
//----------------------------------------------------------------------------------

// Inputs (per player)
typedef struct NV_InputCollection {
    int left_key;
    int right_key;
    int jump_key;
    int slam_key;
} InputCollection;

// Player
typedef struct NV_Player {
    int player_id;

    PhysicsBody physics;
    Vector2 direction;

    InputCollection controls; // Obselete

    int p_colliding;

    // Flame thrower
    float flamethower_force;

    // Death beam
    float death_charge;
    int death_enabled;

    // Block
    int block_enabled;

    // Dash
    int dash_enabled;
    int dash_timer;
} Player;

// Box object
typedef struct NV_BoxObj {
    float width;
    float height;
    PhysicsBody physics;
} BoxObj;

// Circle Objct
typedef struct NV_CircleObj {
    float radius;
    PhysicsBody physics;
} CircleObj;

// Polygon
typedef struct NV_PolygonObj {
    int sides;
    float radius;
    PhysicsBody physics;
} PolygonObj;


typedef struct NV_EnvironmentObj {
    // Enum to keep track of what type the object is
    enum NV_ObjType {
        BOX,
        CIRCLE,
        POLYGON
    } obj_type;

    // The object itself
    union NV_ObjUnion {
        BoxObj box;
        CircleObj circle;
        PolygonObj polygon;
    } obj;

    // Other data
    Color color;

} EnvironmentObj;

//----------------------------------------------------------------------------------
// Functions (I'm too lazy to make a c file)
//----------------------------------------------------------------------------------

// Create the player
Player createPlayer(
    Vector2 position,
    int index
) {
    Player player = { 0 };
    player.player_id = index;

    player.flamethower_force = 0;
    player.death_charge = 0;
    player.death_enabled = 0;
    player.block_enabled = 0;
    player.dash_enabled = 0;
    player.dash_timer = 0;

    // TODO: switch this to controllers
    switch (player.player_id) {
        case (0): {
            player.controls.left_key = KEY_LEFT;
            player.controls.right_key = KEY_RIGHT;
            player.controls.jump_key = KEY_UP;
            player.controls.slam_key = KEY_DOWN;
        } break;
        case (1): {
            player.controls.left_key = KEY_A;
            player.controls.right_key = KEY_D;
            player.controls.jump_key = KEY_W;
            player.controls.slam_key = KEY_S;
        } break;
        case (2): {
            // TODO
        } break;
        case (3): {
            // TODO
        } break;
        default: {} break;
    }

    // TODO: Choose a spawn location
    player.physics = CreatePhysicsBodyRectangle(
        position,
        PLAYER_WIDTH,
        PLAYER_HEIGHT,
        1
    );
    player.physics->useGravity = 1;
    player.physics->freezeOrient = 1;

    return player;
}

float lerp(float a, float b, float x) {
    return (a*x) + (b*(1-x));
}

float joystickDeadzone(float x) {
    if (x > 0.1 || x < -0.1) {
        return x;
    }
    return 0;
}

// TODO: Make more dashy and fun
void updatePlayerMovement(Player* player) {
    /*
    // Old system
    if (IsKeyDown(player->controls.left_key)) {
        player->physics->velocity.x = lerp(
            player->physics->velocity.x,
            -HORIZ_VELOCITY,
            0.95
        );
    }
    if (IsKeyDown(player->controls.right_key)) {
        player->physics->velocity.x = lerp(
            player->physics->velocity.x,
            HORIZ_VELOCITY,
            0.95
        );
    }
    if (IsKeyDown(player->controls.jump_key) && player->physics->isGrounded) {
        player->physics->velocity.y = -JUMP_VELOCITY;
    }
    // Might remove this
    if (IsKeyDown(player->controls.slam_key)) {
        player->physics->velocity.y = lerp(
            player->physics->velocity.y,
            HORIZ_VELOCITY,
            0.95
        );
    }
    
    */
    player->dash_timer = max(player->dash_timer - 1, 0);

    if (!IsGamepadAvailable(player->player_id)) return;

    // Basic movement
    float charge_inhibit = max(0.0, 1.0 / (0.008*player->death_charge + 1.0));
    float x_left = joystickDeadzone(GetGamepadAxisMovement(player->player_id, GAMEPAD_AXIS_LEFT_X));
    float y_left = joystickDeadzone(GetGamepadAxisMovement(player->player_id, GAMEPAD_AXIS_LEFT_Y));
    player->physics->velocity.x = lerp(
        player->physics->velocity.x, 
        x_left*HORIZ_VELOCITY*charge_inhibit, 
        0.95
    );
    if (player->physics->isGrounded && y_left < -0.7) {
        player->physics->velocity.y = -JUMP_VELOCITY;
    }

    // Direction setting
    float x_right = GetGamepadAxisMovement(player->player_id, GAMEPAD_AXIS_RIGHT_X);
    float y_right = GetGamepadAxisMovement(player->player_id, GAMEPAD_AXIS_RIGHT_Y);
    float norm = sqrt((x_right*x_right + y_right*y_right));
    
    if (norm > 0.8) {
        player->direction.x = x_right / norm;
        player->direction.y = y_right / norm;
    }

    // Dash
    if (IsGamepadButtonPressed(player->player_id, GAMEPAD_BUTTON_RIGHT_TRIGGER_1) && player->dash_timer <= 0) {
        
        PhysicsAddForce(
            player->physics,
            (Vector2){3000*player->direction.x, 3000*player->direction.y} 
        );
        
        player->dash_enabled = 1;
        player->dash_timer = 100;
    }

    // Flamethrower
    player->flamethower_force = GetGamepadAxisMovement(player->player_id, GAMEPAD_AXIS_RIGHT_TRIGGER);

    // Deathbeam
    float left_trigger = GetGamepadAxisMovement(player->player_id, GAMEPAD_AXIS_LEFT_TRIGGER);
    if (left_trigger > 0.1) {
        player->death_charge += left_trigger;
    }
    
    // When they release the trigger, send the death beam
    if (player->death_charge > 1 && left_trigger < 0.1) {
        player->death_enabled = 1;
    }

    // Block
    if (IsGamepadButtonDown(player->player_id, GAMEPAD_BUTTON_LEFT_TRIGGER_1)) {
        player->block_enabled = 1;
    } else {
        player->block_enabled = 0;
    }
}

// Create a box
EnvironmentObj createEnvironmentBox(
    Vector2 position, 
    int width, 
    int height, 
    float rotation,
    float density,
    bool enabled,
    Color color
) {
    // Create object and set parameters
    EnvironmentObj obj;
    obj.obj_type = BOX;
    obj.color = color;

    // Create the box
    BoxObj box = { 0 };
    box.width = width;
    box.height = height;
    box.physics = CreatePhysicsBodyRectangle(position, width, height, density);
    box.physics->orient = rotation;
    box.physics->enabled = enabled;

    // Set the box
    obj.obj.box = box;

    return obj;
}

// Create a circle
EnvironmentObj createEnvironmentCircle(
    Vector2 position, 
    float radius,
    float density,
    bool enabled,
    Color color
) {
    // Create object and set parameters
    EnvironmentObj obj;
    obj.obj_type = CIRCLE;
    obj.color = color;

    // Create the box
    CircleObj circle = { 0 };
    circle.radius = radius;
    circle.physics = CreatePhysicsBodyCircle(position, radius, density);
    circle.physics->enabled = false;

    // Set the box
    obj.obj.circle = circle;

    return obj;
}

// Create a polygon
EnvironmentObj createEnvironmentPolygon(
    Vector2 position, 
    int sides,
    float radius,
    float rotation,
    float density,
    bool enabled,
    Color color
) {
    // Create object and set parameters
    EnvironmentObj obj;
    obj.obj_type = POLYGON;
    obj.color = color;

    // Create the box
    PolygonObj polygon = { 0 };
    polygon.radius = radius;
    polygon.sides = sides;
    polygon.physics = CreatePhysicsBodyPolygon(position, radius, sides, density);
    polygon.physics->orient = rotation;
    polygon.physics->enabled = enabled;

    // Set the box
    obj.obj.polygon = polygon;

    return obj;
}

// Helper function to get the position
Vector2 getObjPosition(EnvironmentObj* obj) {
    switch(obj->obj_type) {
        case(BOX): {
            return obj->obj.box.physics->position;
        } break;
        case(CIRCLE): {
            return obj->obj.circle.physics->position;
        } break;
        case(POLYGON): {
            return obj->obj.polygon.physics->position;
        } break;
        default: {
            return (Vector2){-999999, -99999};
        }
    }
}

// Player drawing
static void drawPlayer(Player* player, long long int t) {
    float player_x = player->physics->position.x - PLAYER_WIDTH / 2;
    float player_y = player->physics->position.y - PLAYER_HEIGHT / 2;
    DrawRectangle(
        player_x - 2,
        player_y - 2,
        PLAYER_WIDTH + 4,
        PLAYER_HEIGHT + 4,
        WHITE
    );
    DrawRectangle(
        player_x,
        player_y,
        PLAYER_WIDTH,
        PLAYER_HEIGHT,
        (Color){
            (int)((player->player_id*7.5 + 1)*127.5) % 255, 
            (int)((player->player_id*1.8 + 1)*127.5) % 255, 
            (int)((player->player_id*2.2 + 1)*127.5) % 255, 
            255
        }
    );

    // Triangle to show orientation
    float player_rot = atan2(player->direction.y, player->direction.x) * 180 / PI;
    Vector2 triangle_pos = {player_x + PLAYER_WIDTH / 2, player_y + PLAYER_HEIGHT / 5};
    triangle_pos.x += player->direction.x * 30;
    triangle_pos.y += player->direction.y * 30;
    DrawPoly(
        triangle_pos,
        3,
        PLAYER_WIDTH / 2,
        player_rot,
        RED
    );

    // Dynamic player names, probably will remove
    const char* player_name = TextFormat("Player %i", player->player_id + 1);
    int font_size = 22;
    int text_size = MeasureText(player_name, font_size);
    DrawText(
        player_name,
        player_x - text_size/2 + PLAYER_WIDTH/2 - 1,
        player_y - 36 + 8.0*sin(player->player_id + (float)t / 17.0) - 2,
        font_size,
        WHITE
    );
    DrawText(
        player_name,
        player_x - text_size/2 + PLAYER_WIDTH/2 + 1,
        player_y - 34 + 8.0*sin(player->player_id + (float)t / 17.0) - 2,
        font_size,
        WHITE
    );
    DrawText(
        player_name,
        player_x - text_size/2 + PLAYER_WIDTH/2 - 1,
        player_y - 34 + 8.0*sin(player->player_id + (float)t / 17.0) - 2,
        font_size,
        WHITE
    );
    DrawText(
        player_name,
        player_x - text_size/2 + PLAYER_WIDTH/2 + 1,
        player_y - 36 + 8.0*sin(player->player_id + (float)t / 17.0) - 2,
        font_size,
        WHITE
    );
    DrawText(
        player_name,
        player_x - text_size/2 + PLAYER_WIDTH/2,
        player_y - 35 + 8.0*sin(player->player_id + (float)t / 17.0),
        font_size,
        BLACK
    );
}

// Environment drawing
static void drawEnvironmentObj(EnvironmentObj* obj) {
    Vector2 pos = getObjPosition(obj);

    switch (obj->obj_type) {
        default: return;

        // Box drawing
        case (BOX): {
            Rectangle rect = {pos.x, pos.y, obj->obj.box.width, obj->obj.box.height};
            DrawRectanglePro(
                rect,
                (Vector2) {obj->obj.box.width / 2, obj->obj.box.height / 2},
                obj->obj.box.physics->orient * 180/PI,
                obj->color
            );
        } break;

        case (CIRCLE): {
            DrawCircle(
                pos.x,
                pos.y,
                obj->obj.circle.radius,
                obj->color
            );
        } break;

        case (POLYGON): {
            DrawPoly(
                pos,
                obj->obj.polygon.sides,
                obj->obj.polygon.radius,
                obj->obj.polygon.physics->orient * 180/PI, 
                obj->color
            );
        } break;
    }
}

static Vector2 environmentToFluidCoords(Vector2 pos, FluidBody* fluid) {
    Vector2 new_pos;

    new_pos.x = (fluid->x_resolution) * (0.5 + (pos.x - fluid->bounds.x)/fluid->bounds.width);
    new_pos.y = (fluid->y_resolution) * (0.5 - (pos.y - fluid->bounds.y)/fluid->bounds.height);

    return new_pos;
}

// static Vector2 fluidToEnvironmentCoords(Vector2 pos, FluidBody* fluid) {
//     Vector2 new_pos;
//     new_pos.x = (((pos.x / fluid->x_resolution) - 0.5) * fluid->bounds.width) + fluid->bounds.x;
//     new_pos.y = (((pos.y / fluid->y_resolution) - 0.5) * -fluid->bounds.height) + fluid->bounds.y;
//     return new_pos;
// }

static Vector2 fluidAspect(FluidBody* fluid) {
    Vector2 aspects;
    aspects.x = fluid->bounds.width / fluid->x_resolution;
    aspects.y = fluid->bounds.height / fluid->y_resolution;

    return aspects;
}

static void drawEnvironmentObjToFluid(EnvironmentObj* obj, FluidBody* fluid) {
    Vector2 pos = getObjPosition(obj);
    Vector2 aspects = fluidAspect(fluid);

    pos = environmentToFluidCoords(pos, fluid);
    // TODO: This only works at full resolution, figure out why the scaling gets fucked at half or quarter

    switch (obj->obj_type) {
        default: return;
        
        // Box drawing
        case (BOX): {
            Rectangle rect = {
                pos.x, 
                pos.y, 
                obj->obj.box.width / aspects.x, 
                obj->obj.box.height / aspects.y
            };
            DrawRectanglePro(
                rect,
                (Vector2) 
                {rect.width / 2, rect.height / 2},
                obj->obj.box.physics->orient * 180/PI,
                RED
            );
        } break;

        case (CIRCLE): {
            DrawCircle(
                pos.x,
                pos.y,
                obj->obj.circle.radius / aspects.x,
                RED
            );
        } break;

        case (POLYGON): {
            DrawPoly(
                pos,
                obj->obj.polygon.sides,
                obj->obj.polygon.radius / aspects.x,
                obj->obj.polygon.physics->orient * 180/PI, 
                RED
            );
        } break;
    }
}

#endif
#ifndef NVST_FLUIDS
#define NVST_FLUIDS

#include "raylib.h"

#define GRAPHICS_API_OPENGL_33
#include "rlgl.h"

#define RENDER_FORMAT PIXELFORMAT_UNCOMPRESSED_R16G16B16A16

typedef struct NV_Fluid {
    Shader shader;
    Shader render_shader;
    RenderTexture2D boundary_tex;
    RenderTexture2D fluid_tex;
    RenderTexture2D fluid_tex_b;
    Image cpu_image;
    int sample_to_cpu;
    int active_buffer_i;
    int time_uniform;
    int boundary_uniform;
    int fluid_uniform;
    int final_render_uniform;
    Rectangle bounds;
    int x_resolution;
    int y_resolution;
} FluidBody;

FluidBody createFluidBody(
    int x_resolution,
    int y_resolution,
    int x_position,
    int y_position,
    int width,
    int height
) {
    FluidBody fluid = { 0 };

    // Sampler that yoinks the render texture from the GPU
    fluid.sample_to_cpu = 0;
    fluid.cpu_image = GenImageColor(x_resolution, y_resolution, BLANK);
    fluid.cpu_image.width = x_resolution;
    fluid.cpu_image.height = y_resolution;
    fluid.cpu_image.format = PIXELFORMAT_UNCOMPRESSED_R16G16B16A16;

    // Load params
    fluid.x_resolution = x_resolution;
    fluid.y_resolution = y_resolution;
    fluid.bounds = (Rectangle){x_position, y_position, width, height};

    // Create the textures
    fluid.fluid_tex = LoadRenderTexture(x_resolution, y_resolution);
    fluid.fluid_tex.id = rlLoadFramebuffer();
    rlEnableFramebuffer(fluid.fluid_tex.id);
    fluid.fluid_tex.texture.id = rlLoadTexture(0, x_resolution, y_resolution, RENDER_FORMAT, 1);
    SetTextureFilter(fluid.fluid_tex.texture, TEXTURE_FILTER_TRILINEAR);
    fluid.fluid_tex.texture.width = x_resolution;
    fluid.fluid_tex.texture.height = y_resolution;
    fluid.fluid_tex.texture.format = RENDER_FORMAT;
    fluid.fluid_tex.texture.mipmaps = 1;

    rlFramebufferAttach(
        fluid.fluid_tex.id, 
        fluid.fluid_tex.texture.id,
        RL_ATTACHMENT_COLOR_CHANNEL0, 
        RL_ATTACHMENT_TEXTURE2D, 
        0
    );

    rlDisableFramebuffer();

    fluid.fluid_tex_b = LoadRenderTexture(x_resolution, y_resolution);
    fluid.fluid_tex_b.id = rlLoadFramebuffer();
    rlEnableFramebuffer(fluid.fluid_tex_b.id);
    fluid.fluid_tex_b.texture.id = rlLoadTexture(0, x_resolution, y_resolution, RENDER_FORMAT, 1);
    SetTextureFilter(fluid.fluid_tex_b.texture, TEXTURE_FILTER_TRILINEAR);
    fluid.fluid_tex_b.texture.width = x_resolution;
    fluid.fluid_tex_b.texture.height = y_resolution;
    fluid.fluid_tex_b.texture.format = RENDER_FORMAT;
    fluid.fluid_tex_b.texture.mipmaps = 1;

    rlFramebufferAttach(
        fluid.fluid_tex_b.id, 
        fluid.fluid_tex_b.texture.id,
        RL_ATTACHMENT_COLOR_CHANNEL0, 
        RL_ATTACHMENT_TEXTURE2D, 
        0
    );
    rlDisableFramebuffer();

    fluid.fluid_uniform = GetShaderLocation(fluid.shader, "uFluid");
    SetShaderValueTexture(fluid.shader, fluid.fluid_uniform, fluid.fluid_tex.texture);

    // Set the double buffering
    fluid.active_buffer_i = 1;
    // 1 -> fluid_tex
    // 0 -> fluid_tex_b

    // Load shader
    fluid.shader = LoadShader(0, "fluid_comp.glsl");
    fluid.render_shader = LoadShader(0, "fluid_render.glsl");

    // Set time uniform
    float time = 0.0;
    fluid.time_uniform = GetShaderLocation(fluid.shader, "uTime");
    SetShaderValue(fluid.shader, fluid.time_uniform, &time, SHADER_UNIFORM_INT);

    // Set uniform for boundaries
    fluid.boundary_uniform = GetShaderLocation(fluid.shader, "uBoundaries");
    fluid.boundary_tex = LoadRenderTexture(x_resolution, y_resolution);

    // Get render uniform
    fluid.final_render_uniform = GetShaderLocation(fluid.render_shader, "uFluid");

    return fluid;
}

void unloadFluidBody (FluidBody* fluid) {
    UnloadShader(fluid->shader);
    UnloadShader(fluid->render_shader);

    UnloadImage(fluid->cpu_image);

    UnloadRenderTexture(fluid->fluid_tex);
    UnloadRenderTexture(fluid->fluid_tex_b);
    UnloadRenderTexture(fluid->boundary_tex);

    rlUnloadFramebuffer(fluid->fluid_tex.id);
    rlUnloadFramebuffer(fluid->fluid_tex_b.id);
    rlUnloadTexture(fluid->fluid_tex.texture.id);
    rlUnloadTexture(fluid->fluid_tex_b.texture.id);
}

// Draw the fluid
void drawFluidBody(FluidBody* fluid) {
    fluid->sample_to_cpu = (fluid->sample_to_cpu + 1) % 2;
    // Runs the rendering pass
    BeginShaderMode(fluid->render_shader);
    // Would be better to do this with two pointers (front and back buffer) but Im lazy
    if (fluid->active_buffer_i) {
        SetShaderValueTexture(fluid->render_shader, fluid->final_render_uniform, fluid->fluid_tex.texture);
        if (fluid->sample_to_cpu == 0) {
            UnloadImage(fluid->cpu_image);
            fluid->cpu_image = LoadImageFromTexture(
                fluid->fluid_tex.texture
            );
        }
    } else {
        SetShaderValueTexture(fluid->render_shader, fluid->final_render_uniform, fluid->fluid_tex_b.texture);
        if (fluid->sample_to_cpu == 0) {
            UnloadImage(fluid->cpu_image);
            fluid->cpu_image = LoadImageFromTexture(
                fluid->fluid_tex_b.texture
            );
        }
    }

    DrawTexturePro(
        fluid->fluid_tex.texture,
        (Rectangle){0, 0, fluid->fluid_tex.texture.width, fluid->fluid_tex.texture.height},
        fluid->bounds,
        (Vector2){fluid->bounds.width / 2, fluid->bounds.height / 2},
        0.0,
        WHITE
    );

    // Draw the fluid
    EndShaderMode();
}

// Draws the fluid back to it's own texture 
void updateFluidBuffer(FluidBody* fluid) {
    BeginShaderMode(fluid->shader);
    // fluid_tex
    if (fluid->active_buffer_i) {
        BeginTextureMode(fluid->fluid_tex_b);
        ClearBackground(BLANK);
        DrawTexturePro(
            fluid->fluid_tex.texture,
            (Rectangle){0, fluid->y_resolution, fluid->x_resolution, -fluid->y_resolution},
            (Rectangle){0, 0, fluid->x_resolution, fluid->y_resolution},
            (Vector2){0, 0},
            0.0,
            BLANK
        );
        EndTextureMode();
        fluid->active_buffer_i = 0;
    } 
    // fluid_tex_b
    else {
        BeginTextureMode(fluid->fluid_tex);
        ClearBackground(BLANK);
        DrawTexturePro(
            fluid->fluid_tex_b.texture,
            (Rectangle){0, fluid->y_resolution, fluid->x_resolution, -fluid->y_resolution},
            (Rectangle){0, 0, fluid->x_resolution, fluid->y_resolution},
            (Vector2){0, 0},
            0.0,
            BLANK
        );

        EndTextureMode();
        fluid->active_buffer_i = 1;
    }

    EndShaderMode();

    if (fluid->active_buffer_i) {
        SetShaderValueTexture(fluid->shader, fluid->fluid_uniform, fluid->fluid_tex.texture);
    } else {
        SetShaderValueTexture(fluid->shader, fluid->fluid_uniform, fluid->fluid_tex_b.texture);
    }
}

void setFluidUniforms(FluidBody* fluid, float* time) {
    SetShaderValue(fluid->shader, fluid->time_uniform, time, SHADER_UNIFORM_FLOAT);
}

// Conversion shit
union FP32
{
    uint32_t u;
    float f;
};

float convertFloat16ToNativeFloat(short int value)
{
    const union FP32 magic = { (254UL - 15UL) << 23 };
    const union FP32 was_inf_nan = { (127UL + 16UL) << 23 };
    union FP32 out;

    out.u = (value & 0x7FFFU) << 13;
    out.f *= magic.f;
    if (out.f >= was_inf_nan.f)
    {
        out.u |= 255UL << 23;
    }
    out.u |= (value & 0x8000UL) << 16;

    return out.f;
}

// Reads a pixel value
Vector4 getCPUImgValue(FluidBody* fluid, int x, int y) {
    if (x < 0 || x >= fluid->x_resolution || y < 0 || y >= fluid->y_resolution) {
        return (Vector4){0, 0, 0, 0};
    }

    int base_index = (y*fluid->x_resolution + x) * (4); // (y*xres + x) * (4 components)
    short int* f16_cpu_image = (short int*)fluid->cpu_image.data;
    short int r = f16_cpu_image[base_index];
    short int g = f16_cpu_image[base_index + 1];
    short int b = f16_cpu_image[base_index + 2];
    short int a = f16_cpu_image[base_index + 3];

    Vector4 output = {
        convertFloat16ToNativeFloat(r),
        convertFloat16ToNativeFloat(g),
        convertFloat16ToNativeFloat(b),
        convertFloat16ToNativeFloat(a)
    };
    return output;
}

#endif
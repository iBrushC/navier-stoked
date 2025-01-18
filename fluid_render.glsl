#version 450

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;

// Uniforms
uniform sampler2D uFluid;

// Output fragment color
out vec4 finalColor;

void main() {
    vec2 uv = vec2(fragTexCoord.x, 1 - fragTexCoord);
    vec4 fluid_data = texture(uFluid, fragTexCoord);
    float opacity = abs(fluid_data.x) + abs(fluid_data.y);
    opacity = (opacity) / 3;
    // vec3 color = mix(vec3(1.0, 0.4, 0.5), vec3(1.0, 0.6, 0.4), opacity);
    vec3 color = mix(vec3(0.4, 1.0, 0.5), vec3(0.6, 1.0, 0.4), opacity);
    finalColor = vec4(color, opacity*opacity);//vec4(color*fluid_data.z, opacity);
    // finalColor = fluid_data;//vec4(color*fluid_data.z, opacity);
}
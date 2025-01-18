#version 450

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;

// Uniforms
uniform float uTime = 0;
uniform sampler2D uBoundaries;
uniform sampler2D uFluid;

// Output fragment color
out vec4 finalColor;

float mag2(vec2 p) {
    return dot(p,p);
}

vec2 point1(float t) {
    t *= 0.62;
    return vec2(0.12,0.5 + sin(t)*0.2);
}
vec2 point2(float t) {
    t *= 0.62;
    return vec2(0.88,0.5 + cos(t + 1.5708)*0.2);
}

void main() {
    if (uTime < 0.1)
    {
        finalColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    vec2 uv = fragTexCoord - vec2(0, 1); // UV Sampling
    vec2 w = 1.0/vec2(1920.0, 1080.0);  // TODO: Replace with uniforms
    float dt = 0.1;              // TODO: Replace with uniforms
    const float K = 0.03;
	const float v = 0.19;
    
    vec4 data = textureLod(uFluid, uv, 0.0);
    vec4 tr = textureLod(uFluid, uv + vec2(w.x , 0), 0.0);
    vec4 tl = textureLod(uFluid, uv - vec2(w.x , 0), 0.0);
    vec4 tu = textureLod(uFluid, uv + vec2(0 , w.y), 0.0);
    vec4 td = textureLod(uFluid, uv - vec2(0 , w.y), 0.0);
    
    vec3 dx = (tr.xyz - tl.xyz)*0.5;
    vec3 dy = (tu.xyz - td.xyz)*0.5;
    vec2 densDif = vec2(dx.z, dy.z);
    
    data.z -= dt*dot(vec3(densDif, dx.x + dy.y), data.xyz); //density
    vec2 laplacian = tu.xy + td.xy + tr.xy + tl.xy - 4.0*data.xy;
    vec2 viscForce = vec2(v)*laplacian;

    vec4 advect = textureLod(uFluid, uv - dt*data.xy*w, 0.0);
    data.xy = advect.xy; //advection

    // float center_circle = smoothstep(0.006, 0.004, length(fragTexCoord + vec2(-0.5 + 0.3*cos(uTime*0.5), -1.5 + 0.3*sin(uTime*0.5))));
    // float center_circle = smoothstep(0.01, 0.004, length(fragTexCoord + vec2(-0.5, -1.5)));

    // Here's the problem: The texture cannot read negative values in any channel
    // vec2 external_forces = 10*vec2(sin(uTime*0.863)*sin(uTime*0.463 - 1.5), cos(uTime*1.73)*cos(uTime*1.93 + 0.3)) * vec2(center_circle);
    // vec2 external_forces = vec2(10*sin(uTime*0.45), 0.0) * center_circle;
    vec2 external_forces = vec2(0);
    // external_forces.xy += 0.75*vec2(.0003, 0.00015)/(mag2(uv-point1(uTime))+0.0001);
    // external_forces.xy -= 0.75*vec2(.0003, 0.00015)/(mag2(uv-point2(uTime))+0.0001);

    if (data.w < 1) {
        external_forces.xy += 100*(data.xy - 0.5 + vec2(0, 0.01));
        external_forces.xy += 10*cos(uTime*uv*-93.472*sin(uv*10983.29) + 239132);
        data.z = 0;
        data.w = 0;
    }

    // data.z += center_circle;
    
    data.xy += dt*(viscForce.xy - K/dt*densDif + 8*external_forces); //update velocity
    data.xy = max(vec2(0), abs(data.xy)-0.0008)*sign(data.xy); //linear velocity decay

    float curl = (tr.y - tl.y - tu.x + td.x);
    vec2 vort = vec2(abs(tu.w) - abs(td.w), abs(tl.w) - abs(tr.w));
    vort *= -0.2/length(vort + 1e-9)*curl;
    data.xy += vort;
    
    // data.y *= smoothstep(.5,.48,abs(uv.y-0.5)); // Boundaries
    
    data = clamp(data, vec4(vec2(-100000), 0.5 , -10.), vec4(vec2(100000), 15.0 , 10.));

    // Horizontal boundary conditions
    if (
        (textureLod(uBoundaries, uv + vec2(w.x, 0), 0.0).x > 0) || // Right blocked
        (textureLod(uBoundaries, uv - vec2(w.x, 0), 0.0).x > 0)    // Left blocked
    ) {
        data.x = 0;
    }

    // Vertical boundary conditions
    if (
        (textureLod(uBoundaries, uv + vec2(0, w.y), 0.0).x > 0) || // Right blocked
        (textureLod(uBoundaries, uv - vec2(0, w.y), 0.0).x > 0)    // Left blocked
    ) {
        data.y = 0;
    }

    // Clear the pressure of blocked areas
    data *= (1 - texture(uBoundaries, uv).y);

    finalColor = vec4(data.xyz, 1);
    // finalColor = texture(uBoundaries, uv);
    // finalColor = vec4(uv - data.xy*w, 0, 1);
}
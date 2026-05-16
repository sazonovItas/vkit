#version 450

layout(location = 0) in vec2 inUV;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec4 outColor;

void main() {
    float d = dot(inUV, inUV);
    if (d > 1.0) discard;
    // Hard inner disc with a soft edge ring
    float alpha = 1.0 - smoothstep(0.6, 1.0, d);
    outColor = vec4(inColor, alpha);
}

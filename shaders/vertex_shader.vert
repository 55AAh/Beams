#version 430 core
layout (location = 0) in vec2 aPos;

uniform float zoom;

struct Element {
    float u;
    float w;
    float t;
    float m;
    float q;
    float n;
};

layout(std430, binding = 0) restrict readonly buffer ElementsBuffer {
    Element elements[];
};

out vec3 vertexColor;

#define s aPos.x
#define element int(aPos.y)

void main()
{
    gl_Position = vec4(elements[element].u + s / 3, elements[element].w + s*s / 3, 0.0, 1.0);
    vertexColor = vec3(element == 0, element == 1, element == 2);
}
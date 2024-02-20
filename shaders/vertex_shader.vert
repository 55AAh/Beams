#version 430 core


struct ElementUWT {
    float u, w, t;
};
ElementUWT construct_uwt(float u, float w, float t);

struct ElementParams {
    ElementUWT uwt;
    float m;
    float q, n;
};

struct UniformParams {
    float EI, GJ;
    float total_weight;
    float total_length;
};

ElementUWT construct_uwt(float u, float w, float t) {
    return ElementUWT(u, w, t);
}

layout (location = 0) in vec2 aPos;
layout(std430, binding = 0) restrict readonly buffer ElementsBuffer {
    ElementParams elements[];
};
uniform float up_array[4];
uniform float zoom;

out vec3 vertexColor;

// This function will be implemented in included formulae.h
ElementUWT calc_uwt(ElementParams ep, UniformParams up, float s);

void main() {
    UniformParams up = UniformParams(up_array[0], up_array[1], up_array[2], up_array[3]);

    float s = aPos.x * up.total_length / float(elements.length());
    int element = int(aPos.y);

    ElementUWT uwt = calc_uwt(elements[element], up, s);

    gl_Position = vec4(uwt.u, uwt.w, 0.0, 1.0);
//    gl_Position = vec4(elements[element].uwt.u + s / 3, elements[element].uwt.w + s*s / 3, 0.0, 1.0);
    int _color = element % 3;
    vertexColor = vec3(_color == 0, _color == 1, _color == 2);
}

// Do not remove or change the comment below
// It will be replaced by the contents of formulae.h

#define CURRENTLY_IN_GLSL_SHADER
// FORMULAE //
#undef CURRENTLY_IN_GLSL_SHADER
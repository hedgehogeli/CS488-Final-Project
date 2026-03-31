#pragma once

#include "Material.hpp"

#include <glm/glm.hpp>

static double eps = 1e-4;
static double shadowBias = 1e-3;

struct Ray {
    glm::vec4 origin;
    glm::vec4 direction;
    double indexOfRefraction = 1.0;
};

struct Intersection {
    double t = std::numeric_limits<double>::infinity();
    glm::vec4 point;
    glm::vec4 normal;
    Material* material;
    bool hit = false;
    glm::vec2 uv {0.0f}; // uv for texture mapping
    glm::vec4 tangent{0.0f}; // tangent for normal mapping
    // for now we only set and use these 2 ^ with boxes
};

#pragma once

#include <string>

#include <glm/glm.hpp>

#include "PhongMaterial.hpp"
#include "Texture.hpp"

/**
 * Phong material with optional diffuse and OpenGL-style normal-map PNGs.
 * Paths may be empty to skip loading that map (see Texture::loadDiffuse / loadNormal).
 */
class TexturedMaterial : public PhongMaterial {
public:
    TexturedMaterial(
        const glm::vec3& kd,
        const glm::vec3& ks,
        double shininess,
        const std::string& diffusePath,
        const std::string& normalPath,
        const glm::vec2& uvScale = glm::vec2(1.0f),
        double transparency = 0.0,
        double refractiveIndex = 0.0);

    ~TexturedMaterial() override = default;

    const Texture& texture() const { return m_texture; }
    Texture& texture() { return m_texture; }

private:
    Texture m_texture;
};

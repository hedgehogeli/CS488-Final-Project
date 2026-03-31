#include "TexturedMaterial.hpp"

TexturedMaterial::TexturedMaterial(
    const glm::vec3& kd,
    const glm::vec3& ks,
    double shininess,
    const std::string& diffusePath,
    const std::string& normalPath,
    const glm::vec2& uvScale,
    double transparency,
    double refractiveIndex)
    : PhongMaterial(kd, ks, shininess, transparency, refractiveIndex)
{
    if (!diffusePath.empty()) {
      m_texture.loadDiffuse(diffusePath);
    }
    if (!normalPath.empty()) {
      m_texture.loadNormal(normalPath);
    }
    m_texture.setUvScale(uvScale);
}

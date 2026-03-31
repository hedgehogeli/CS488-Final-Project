#pragma once

#include "Image.hpp"
#include <glm/glm.hpp>
#include <string>
#include <vector>

class Texture {
public:
    Texture() = default;
    
    // Load textures from files (empty string = don't load)
    bool loadDiffuse(const std::string& filename);
    bool loadNormal(const std::string& filename);
    
    // Check what's loaded
    bool hasDiffuse() const { return m_diffuse.width() > 0; }
    bool hasNormal() const { return m_normal.width() > 0; }
    
    // Sample diffuse color at UV (returns [0,1] RGB)
    glm::vec3 sampleDiffuse(glm::vec2 uv) const;
    
    // Sample normal map at UV (returns [-1,1] tangent-space normal)
    glm::vec3 sampleNormal(glm::vec2 uv) const;
    
    // UV transform
    void setUvScale(glm::vec2 scale) { m_uvScale = scale; }
    void setUvOffset(glm::vec2 offset) { m_uvOffset = offset; }
    glm::vec2 uvScale() const { return m_uvScale; }
    glm::vec2 uvOffset() const { return m_uvOffset; }
    
private:
    Image m_diffuse;
    Image m_normal;
    std::vector<Image> m_diffuseMips;
    std::vector<Image> m_normalMips;
    
    glm::vec2 m_uvScale = {1.0f, 1.0f};
    glm::vec2 m_uvOffset = {0.0f, 0.0f};
    
    // Internal: sample an image at UV with bilinear filtering
    static glm::vec3 sampleImageBilinear(const Image& img, glm::vec2 uv);
    static std::vector<Image> buildMipChain(const Image& base);
    static float computeMipLevel(const std::vector<Image>& mips, const glm::vec2& uvScale);
    static glm::vec3 sampleImageMipmapped(
        const std::vector<Image>& mips,
        const Image& fallback,
        glm::vec2 uv,
        const glm::vec2& uvScale
    );
    
    // Apply scale and offset, then tile
    glm::vec2 transformUV(glm::vec2 uv) const;
};
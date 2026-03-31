#include "Texture.hpp"
#include <cmath>
#include <iostream>
#include <algorithm>

bool Texture::loadDiffuse(const std::string& filename) {
    if (filename.empty()) return false;
    if (!m_diffuse.loadPng(filename)) {
        std::cerr << "Failed to load diffuse texture: " << filename << std::endl;
        return false;
    }
    m_diffuseMips = buildMipChain(m_diffuse);
    return true;
}

bool Texture::loadNormal(const std::string& filename) {
    if (filename.empty()) return false;
    if (!m_normal.loadPng(filename)) {
        std::cerr << "Failed to load normal texture: " << filename << std::endl;
        return false;
    }
    m_normalMips = buildMipChain(m_normal);
    return true;
}

glm::vec2 Texture::transformUV(glm::vec2 uv) const {
    glm::vec2 scaled = uv * m_uvScale + m_uvOffset;
    
    // Tile/wrap to [0,1]
    scaled.x = scaled.x - std::floor(scaled.x);
    scaled.y = scaled.y - std::floor(scaled.y);
    
    return scaled;
}

glm::vec3 Texture::sampleImageBilinear(const Image& img, glm::vec2 uv) {
    if (img.width() == 0 || img.height() == 0) {
        return glm::vec3(0.0f);
    }

    // Flip V: UV has V=0 at bottom, image has Y=0 at top
    float u = uv.x;
    float v = 1.0f - uv.y;

    const int w = static_cast<int>(img.width());
    const int h = static_cast<int>(img.height());

    const float x = u * (static_cast<float>(w) - 1.0f);
    const float y = v * (static_cast<float>(h) - 1.0f);

    const int x0 = std::max(0, std::min(static_cast<int>(std::floor(x)), w - 1));
    const int y0 = std::max(0, std::min(static_cast<int>(std::floor(y)), h - 1));
    const int x1 = std::max(0, std::min(x0 + 1, w - 1));
    const int y1 = std::max(0, std::min(y0 + 1, h - 1));

    const float tx = x - static_cast<float>(x0);
    const float ty = y - static_cast<float>(y0);

    const glm::vec3 c00(
        static_cast<float>(img(static_cast<uint>(x0), static_cast<uint>(y0), 0)),
        static_cast<float>(img(static_cast<uint>(x0), static_cast<uint>(y0), 1)),
        static_cast<float>(img(static_cast<uint>(x0), static_cast<uint>(y0), 2))
    );
    const glm::vec3 c10(
        static_cast<float>(img(static_cast<uint>(x1), static_cast<uint>(y0), 0)),
        static_cast<float>(img(static_cast<uint>(x1), static_cast<uint>(y0), 1)),
        static_cast<float>(img(static_cast<uint>(x1), static_cast<uint>(y0), 2))
    );
    const glm::vec3 c01(
        static_cast<float>(img(static_cast<uint>(x0), static_cast<uint>(y1), 0)),
        static_cast<float>(img(static_cast<uint>(x0), static_cast<uint>(y1), 1)),
        static_cast<float>(img(static_cast<uint>(x0), static_cast<uint>(y1), 2))
    );
    const glm::vec3 c11(
        static_cast<float>(img(static_cast<uint>(x1), static_cast<uint>(y1), 0)),
        static_cast<float>(img(static_cast<uint>(x1), static_cast<uint>(y1), 1)),
        static_cast<float>(img(static_cast<uint>(x1), static_cast<uint>(y1), 2))
    );

    const glm::vec3 c0 = glm::mix(c00, c10, tx);
    const glm::vec3 c1 = glm::mix(c01, c11, tx);
    return glm::mix(c0, c1, ty);
}

std::vector<Image> Texture::buildMipChain(const Image& base) {
    std::vector<Image> mips;
    if (base.width() == 0 || base.height() == 0) {
        return mips;
    }

    mips.push_back(base);
    while (mips.back().width() > 1 || mips.back().height() > 1) {
        const Image& src = mips.back();
        const uint srcW = src.width();
        const uint srcH = src.height();
        const uint dstW = std::max(1u, srcW / 2);
        const uint dstH = std::max(1u, srcH / 2);

        Image dst(dstW, dstH);
        for (uint y = 0; y < dstH; ++y) {
            for (uint x = 0; x < dstW; ++x) {
                const uint sx0 = std::min(srcW - 1, 2u * x);
                const uint sy0 = std::min(srcH - 1, 2u * y);
                const uint sx1 = std::min(srcW - 1, sx0 + 1);
                const uint sy1 = std::min(srcH - 1, sy0 + 1);

                for (uint c = 0; c < 3; ++c) {
                    const double avg =
                        0.25 * (src(sx0, sy0, c) + src(sx1, sy0, c) +
                                src(sx0, sy1, c) + src(sx1, sy1, c));
                    dst(x, y, c) = avg;
                }
            }
        }
        mips.push_back(dst);
    }

    return mips;
}

float Texture::computeMipLevel(const std::vector<Image>& mips, const glm::vec2& uvScale) {
    if (mips.empty()) {
        return 0.0f;
    }

    const float maxScale = std::max(std::abs(uvScale.x), std::abs(uvScale.y));
    if (maxScale <= 1.0f) {
        return 0.0f;
    }

    const float unclampedLevel = std::log2(maxScale);
    const float maxLevel = static_cast<float>(mips.size() - 1);
    return std::max(0.0f, std::min(unclampedLevel, maxLevel));
}

glm::vec3 Texture::sampleImageMipmapped(
    const std::vector<Image>& mips,
    const Image& fallback,
    glm::vec2 uv,
    const glm::vec2& uvScale
) {
    if (mips.empty()) {
        return sampleImageBilinear(fallback, uv);
    }

    const float lod = computeMipLevel(mips, uvScale);
    const int l0 = std::max(0, std::min(static_cast<int>(std::floor(lod)), static_cast<int>(mips.size() - 1)));
    const int l1 = std::max(0, std::min(l0 + 1, static_cast<int>(mips.size() - 1)));
    const float t = lod - static_cast<float>(l0);

    const glm::vec3 c0 = sampleImageBilinear(mips[static_cast<size_t>(l0)], uv);
    const glm::vec3 c1 = sampleImageBilinear(mips[static_cast<size_t>(l1)], uv);
    return glm::mix(c0, c1, t);
}

glm::vec3 Texture::sampleDiffuse(glm::vec2 uv) const {
    if (!hasDiffuse()) {
        return glm::vec3(1.0f);  // white fallback
    }
    return sampleImageMipmapped(m_diffuseMips, m_diffuse, transformUV(uv), m_uvScale);
}

glm::vec3 Texture::sampleNormal(glm::vec2 uv) const {
    if (!hasNormal()) {
        return glm::vec3(0.0f, 0.0f, 1.0f);  // unperturbed tangent-space normal
    }
    glm::vec3 rgb = sampleImageMipmapped(m_normalMips, m_normal, transformUV(uv), m_uvScale);
    // Convert from [0,1] to [-1,1]
    glm::vec3 n = rgb * 2.0f - 1.0f;
    if (glm::dot(n, n) < 1e-12f) {
        return glm::vec3(0.0f, 0.0f, 1.0f);
    }
    return glm::normalize(n);
}
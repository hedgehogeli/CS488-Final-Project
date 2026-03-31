#pragma once

#include <array>
#include <cstdint>

#include <glm/glm.hpp>

class Perlin {
public:
	explicit Perlin(uint32_t seed = 0u);

	// Returns improved Perlin noise in [-1, 1].
	float noise(const glm::vec3& p) const;

private:
	std::array<int, 512> m_perm;

	static float fade(float t);
	static float lerp(float a, float b, float t);
	static float grad(int hash, float x, float y, float z);
};

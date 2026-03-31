#pragma once

#include <cstdint>

#include <glm/glm.hpp>

#include "perlin.hpp"

class FBM {
public:
	FBM(
		uint32_t seed = 0u,
		int octaves = 5,
		float lacunarity = 2.0f,
		float gain = 0.5f,
		float baseFrequency = 1.0f
	);

	// Returns normalised fBm in [0, 1].
	float sample(const glm::vec3& p) const;

private:
	Perlin m_perlin;
	int m_octaves;
	float m_lacunarity;
	float m_gain;
	float m_baseFrequency;
};

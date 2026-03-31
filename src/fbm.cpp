#include "fbm.hpp"

#include <algorithm>

FBM::FBM(
	uint32_t seed,
	int octaves,
	float lacunarity,
	float gain,
	float baseFrequency
)
	: m_perlin(seed)
	, m_octaves(std::max(1, octaves))
	, m_lacunarity(lacunarity > 0.0f ? lacunarity : 2.0f)
	, m_gain(gain)
	, m_baseFrequency(baseFrequency > 0.0f ? baseFrequency : 1.0f)
{}

float FBM::sample(const glm::vec3& p) const {
	float sum = 0.0f;
	float amp = 1.0f;
	float freq = m_baseFrequency;
	float ampNorm = 0.0f;

	for (int i = 0; i < m_octaves; ++i) {
		sum += amp * m_perlin.noise(p * freq);
		ampNorm += amp;
		freq *= m_lacunarity;
		amp *= m_gain;
	}

	if (ampNorm <= 0.0f) {
		return 0.5f;
	}

	const float n = sum / ampNorm;            // Approximately in [-1, 1].
	const float remapped = 0.5f * (n + 1.0f); // [0, 1].
	return std::max(0.0f, std::min(1.0f, remapped));
}

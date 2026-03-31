#include "perlin.hpp"

#include <algorithm>
#include <numeric>
#include <random>

namespace {
inline int fastFloor(float x) {
	const int i = static_cast<int>(x);
	return (x < static_cast<float>(i)) ? (i - 1) : i;
}
} // namespace

Perlin::Perlin(uint32_t seed) {
	std::array<int, 256> p;
	std::iota(p.begin(), p.end(), 0);

	std::mt19937 rng(seed);
	std::shuffle(p.begin(), p.end(), rng);

	for (size_t i = 0; i < 256; ++i) {
		m_perm[i] = p[i];
		m_perm[i + 256] = p[i];
	}
}

float Perlin::noise(const glm::vec3& p) const {
	const int xi = fastFloor(p.x) & 255;
	const int yi = fastFloor(p.y) & 255;
	const int zi = fastFloor(p.z) & 255;

	const float xf = p.x - static_cast<float>(fastFloor(p.x));
	const float yf = p.y - static_cast<float>(fastFloor(p.y));
	const float zf = p.z - static_cast<float>(fastFloor(p.z));

	const float u = fade(xf);
	const float v = fade(yf);
	const float w = fade(zf);

	const int aaa = m_perm[m_perm[m_perm[xi] + yi] + zi];
	const int aba = m_perm[m_perm[m_perm[xi] + yi + 1] + zi];
	const int aab = m_perm[m_perm[m_perm[xi] + yi] + zi + 1];
	const int abb = m_perm[m_perm[m_perm[xi] + yi + 1] + zi + 1];
	const int baa = m_perm[m_perm[m_perm[xi + 1] + yi] + zi];
	const int bba = m_perm[m_perm[m_perm[xi + 1] + yi + 1] + zi];
	const int bab = m_perm[m_perm[m_perm[xi + 1] + yi] + zi + 1];
	const int bbb = m_perm[m_perm[m_perm[xi + 1] + yi + 1] + zi + 1];

	const float x1 = lerp(
		grad(aaa, xf, yf, zf),
		grad(baa, xf - 1.0f, yf, zf),
		u
	);
	const float x2 = lerp(
		grad(aba, xf, yf - 1.0f, zf),
		grad(bba, xf - 1.0f, yf - 1.0f, zf),
		u
	);
	const float y1 = lerp(x1, x2, v);

	const float x3 = lerp(
		grad(aab, xf, yf, zf - 1.0f),
		grad(bab, xf - 1.0f, yf, zf - 1.0f),
		u
	);
	const float x4 = lerp(
		grad(abb, xf, yf - 1.0f, zf - 1.0f),
		grad(bbb, xf - 1.0f, yf - 1.0f, zf - 1.0f),
		u
	);
	const float y2 = lerp(x3, x4, v);

	return lerp(y1, y2, w);
}

float Perlin::fade(float t) {
	return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}

float Perlin::lerp(float a, float b, float t) {
	return a + t * (b - a);
}

float Perlin::grad(int hash, float x, float y, float z) {
	const int h = hash & 15;
	const float u = (h < 8) ? x : y;
	const float v = (h < 4) ? y : ((h == 12 || h == 14) ? x : z);
	return ((h & 1) ? -u : u) + ((h & 2) ? -v : v);
}

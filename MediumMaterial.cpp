#include "MediumMaterial.hpp"

MediumMaterial::MediumMaterial(
	const glm::vec3& scatteringColour,
	const glm::vec3& sigmaA,
	const glm::vec3& sigmaS,
	float densityScale,
	float stepSize
)
	: m_scatteringColour(scatteringColour)
	, m_sigmaA(sigmaA)
	, m_sigmaS(sigmaS)
	, m_densityScale(densityScale)
	, m_stepSize(stepSize)
	, m_fbm(1337u, 5, 2.0f, 0.5f, 1.0f)
	, m_noiseScale(0.25f)
	, m_noiseThreshold(0.45f)
	, m_noiseSoftness(0.15f)
{}

MediumMaterial::~MediumMaterial() = default;

const glm::vec3& MediumMaterial::scatteringColour() const {
	return m_scatteringColour;
}

const glm::vec3& MediumMaterial::sigmaA() const {
	return m_sigmaA;
}

const glm::vec3& MediumMaterial::sigmaS() const {
	return m_sigmaS;
}

glm::vec3 MediumMaterial::sigmaT() const {
	return m_sigmaA + m_sigmaS;
}

float MediumMaterial::densityScale() const {
	return m_densityScale;
}

float MediumMaterial::stepSize() const {
	return m_stepSize;
}

void MediumMaterial::setBounds(const glm::vec3& boundsMin, const glm::vec3& boundsMax) {
	m_boundsMin = boundsMin;
	m_boundsMax = boundsMax;
}



// float MediumMaterial::densityAt(const glm::vec3& point, float offset) const {
// 	const glm::vec3 noisePos = point * m_noiseScale + glm::vec3(offset, 0.0f, 0.0f);
// 	const float n = m_fbm.sample(noisePos);

// 	const float lo = m_noiseThreshold - m_noiseSoftness;
// 	const float hi = m_noiseThreshold + m_noiseSoftness;
// 	if (n <= lo) return 0.0f;
// 	if (n >= hi) return 1.0f;

// 	const float t = (n - lo) / (hi - lo);
// 	return t * t * (3.0f - 2.0f * t);
// }

// Smooth fade to zero near edges of [0,1]^3 local space
static float edgeFalloff(const glm::vec3& localPos) {
	const float margin = 0.25f; // MAGIC: edge fade band width
	auto fade = [&](float x) {
		float lo = glm::smoothstep(0.0f, margin, x);
		float hi = glm::smoothstep(0.0f, margin, 1.0f - x);
		return lo * hi;
	};
	return fade(localPos.x) * fade(localPos.y) * fade(localPos.z);
}

// Vertical density profile: flat bottom, rounded top
static float heightGradient(float h) {
	// h is normalised height [0,1] within the bounding box
	float base = glm::smoothstep(0.0f, 0.12f, h);   // MAGIC: thin hard floor
	float top  = glm::smoothstep(1.0f, 0.6f, h);     // MAGIC: dome falloff starts at 60% height
	return base * top;
}

float MediumMaterial::densityAt(const glm::vec3& point, float offset) const {
	// --- Local coordinates in [0,1]^3 within bounding box ---
	const glm::vec3 localPos = (point - m_boundsMin) / (m_boundsMax - m_boundsMin);

	// --- Shape envelope: edges + vertical profile ---
	const float edge = edgeFalloff(localPos);
	const float hgrad = heightGradient(localPos.y);
	const float envelope = edge * hgrad;

	if (envelope <= 0.001f) return 0.0f; // early out

	// --- Primary shape noise (low freq) ---
	const glm::vec3 noisePos = point * m_noiseScale + glm::vec3(offset, 0.0f, 0.0f);
	const float n = m_fbm.sample(noisePos);

	const float lo = m_noiseThreshold - m_noiseSoftness;
	const float hi = m_noiseThreshold + m_noiseSoftness;
	if (n <= lo) return 0.0f;

	float shape;
	if (n >= hi) shape = 1.0f;
	else {
		const float t = (n - lo) / (hi - lo);
		shape = t * t * (3.0f - 2.0f * t);
	}

	// Combine shape with envelope
	shape *= envelope;

	if (shape <= 0.001f) return 0.0f; // early out before detail pass

	// --- Detail erosion (high freq, subtractive) ---
	const float detailScale = 3.0f;  // MAGIC: detail frequency multiplier relative to shape
	const glm::vec3 detailPos = noisePos * detailScale;
	const float detailNoise = m_fbm.sample(detailPos);

	const float detailStrength = 0.35f; // MAGIC: how much detail erodes the shape
	// Erode more at the edges where shape is already thin
	const float erosion = detailNoise * detailStrength * (1.0f - shape * 0.5f);
	const float density = glm::clamp(shape - erosion, 0.0f, 1.0f);

	return density;
}
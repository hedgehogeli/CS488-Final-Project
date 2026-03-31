#pragma once

#include <glm/glm.hpp>

#include "Material.hpp"
#include "fbm.hpp"

class MediumMaterial : public Material {
public:
	MediumMaterial(
		const glm::vec3& scatteringColour = glm::vec3(1.0f),
		const glm::vec3& sigmaA = glm::vec3(0.0f),
		const glm::vec3& sigmaS = glm::vec3(0.0f),
		float densityScale = 1.0f,
		float stepSize = 0.1f
	);

	~MediumMaterial() override;

	const glm::vec3& scatteringColour() const;
	const glm::vec3& sigmaA() const;
	const glm::vec3& sigmaS() const;
	glm::vec3 sigmaT() const;
	float densityScale() const;
	float stepSize() const;

	float densityAt(const glm::vec3& point, float offset = 0.0f) const;

	void setBounds(const glm::vec3& boundsMin, const glm::vec3& boundsMax);
	const glm::vec3& boundsMin() const { return m_boundsMin; }
	const glm::vec3& boundsMax() const { return m_boundsMax; }

private:
	glm::vec3 m_scatteringColour;
	glm::vec3 m_sigmaA;
	glm::vec3 m_sigmaS;
	float m_densityScale;
	float m_stepSize;
	FBM m_fbm;
	float m_noiseScale;
	float m_noiseThreshold;
	float m_noiseSoftness;

	glm::vec3 m_boundsMin{0.0f};
	glm::vec3 m_boundsMax{1.0f};
};

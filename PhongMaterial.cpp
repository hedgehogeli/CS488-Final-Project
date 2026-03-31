// Termm--Fall 2020

#include "PhongMaterial.hpp"

PhongMaterial::PhongMaterial(
	const glm::vec3& kd, const glm::vec3& ks, double shininess, double transparency, double refractiveIndex)
	: m_kd(kd)
	, m_ks(ks)
	, m_shininess(shininess)
	, m_transparency(transparency)
	, m_refractiveIndex(refractiveIndex)
{}

PhongMaterial::~PhongMaterial()
{}

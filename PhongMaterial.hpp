// Termm--Fall 2020

#pragma once

#include <glm/glm.hpp>

#include "Material.hpp"

class PhongMaterial : public Material {
public:
  PhongMaterial(const glm::vec3& kd, const glm::vec3& ks, double shininess, double transparency=0.0, double refractiveIndex = 0.0);
  virtual ~PhongMaterial();

  const glm::vec3& kd() const { return m_kd; }
  const glm::vec3& ks() const { return m_ks; }
  double shininess() const { return m_shininess; }
  double transparency() const { return m_transparency; }
  double refractiveIndex() const { return m_refractiveIndex; }

private:
  glm::vec3 m_kd;
  glm::vec3 m_ks;

  double m_shininess;
  double m_transparency; // 0.0 fully opaque. this is used to deal with refraction picking up local color.
  double m_refractiveIndex; 
  // 0.0 default means we don't handle refraction and reflection
  // 1.0 = air, 1.33 = water, 1.5 = glass, 2.42 = diamond
};

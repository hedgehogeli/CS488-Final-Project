// Termm--Fall 2020

#pragma once

#include "RayTracing.hpp"

#include <glm/glm.hpp>

class Primitive {
public:
  virtual ~Primitive();
  virtual bool intersect(const Ray&, Intersection&);
};

class Sphere : public Primitive {
public:
  virtual ~Sphere();
  bool intersect(const Ray&, Intersection&) override;
};

class Cube : public Primitive {
public:
  virtual ~Cube();
  bool intersect(const Ray&, Intersection&) override;
};

// Canonical capped cylinder: axis +Y, radius 1, from y = 0 to y = 1 (local space).
class Cylinder : public Primitive {
public:
  virtual ~Cylinder();
  bool intersect(const Ray&, Intersection&) override;
};

class NonhierSphere : public Primitive {
public:
  NonhierSphere(const glm::vec3& pos, double radius)
    : m_pos(pos), m_radius(radius)
  {
  }
  virtual ~NonhierSphere();
  bool intersect(const Ray&, Intersection&) override;

private:
  glm::vec3 m_pos;
  double m_radius;
};

class NonhierBox : public Primitive {
public:
  NonhierBox(const glm::vec3& pos, double size)
    : m_pos(pos), m_size(size)
  {
  }
  
  virtual ~NonhierBox();
  bool intersect(const Ray&, Intersection&) override;

private:
  glm::vec3 m_pos;
  double m_size;
};

class BoundingBox : public Primitive {
public:
  BoundingBox(const glm::vec3& min, const glm::vec3& max)
    : m_min(min), m_max(max)
  {
  }
  
  virtual ~BoundingBox();
  bool intersect(const Ray&, Intersection&) override;

private:
  glm::vec3 m_min;
  glm::vec3 m_max;
};
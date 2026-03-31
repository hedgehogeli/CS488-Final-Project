// Termm--Fall 2020

#include <algorithm>
#include <cmath>

#include <glm/gtc/constants.hpp>

#include "Primitive.hpp"
#include "polyroots.hpp"

Primitive::~Primitive()
{
}

Sphere::~Sphere()
{
}

Cube::~Cube()
{
}

Cylinder::~Cylinder()
{
}

NonhierSphere::~NonhierSphere()
{
}

NonhierBox::~NonhierBox()
{
}

BoundingBox::~BoundingBox()
{
}

bool Primitive::intersect(const Ray& ray, Intersection& isect) {
    return false; // placeholder. we should make this pure virtual later.
}

// Ray: origin + t * direction
    // Box from m_pos to m_pos + (m_size, m_size, m_size)

    /*
    working for X Y aligned faces
    (0) a + bt = P // parametric line
    (1) m_pos             + A(x) + B(y)
    (2) m_pos + m_size(z) + B(x) + B(y)
    intersects if 0 <= A, B <= m_size

    (0) intersect (1) 
    t = (pos.z - a.z) / b.z
    (a.x + b.x * t - pos.x) = A  
    (a.y + b.y * t - pos.y) = B  

    (0) intersect (2) 
    t = (pos.z + size - a.z) / b.z
    A, B same

    need t >= 0 or eps,
    and also check that b.z >= eps before computing, since say parallel to plane => no intersection
    */

// returns true and sets isect if ray hits box [bmin, bmax].
static bool intersect_slab(const Ray& ray, const glm::vec3& bmin, const glm::vec3& bmax,
                           Intersection& isect) {
    glm::vec3 o(ray.origin);
    glm::vec3 d(ray.direction);
    glm::vec3 inv_d(1.0 / d.x, 1.0 / d.y, 1.0 / d.z);

    double t0x = (bmin.x - o.x) * inv_d.x;
    double t1x = (bmax.x - o.x) * inv_d.x;
    if (t0x > t1x) std::swap(t0x, t1x);
    double t0y = (bmin.y - o.y) * inv_d.y;
    double t1y = (bmax.y - o.y) * inv_d.y;
    if (t0y > t1y) std::swap(t0y, t1y);
    double t0z = (bmin.z - o.z) * inv_d.z;
    double t1z = (bmax.z - o.z) * inv_d.z;
    if (t0z > t1z) std::swap(t0z, t1z);

    double t_enter = std::max({ t0x, t0y, t0z });
    double t_exit  = std::min({ t1x, t1y, t1z });
    if (t_enter > t_exit || t_exit < eps) {
        isect.hit = false;
        return false;
    }
    double t_hit = (t_enter >= eps) ? t_enter : t_exit;
    if (t_hit < eps) {
        isect.hit = false;
        return false;
    }

    isect.hit = true;
    isect.t = t_hit;
    isect.point = ray.origin + ray.direction * (float)t_hit;

    // compute N
    glm::vec3 normal(0.0f);
    if (t_enter >= eps) {
        if (t_enter == t0x) normal.x = (d.x > 0) ? -1.0f : 1.0f;
        else if (t_enter == t0y) normal.y = (d.y > 0) ? -1.0f : 1.0f;
        else normal.z = (d.z > 0) ? -1.0f : 1.0f;
    } else {
        if (t_exit == t1x) normal.x = (d.x > 0) ? 1.0f : -1.0f;
        else if (t_exit == t1y) normal.y = (d.y > 0) ? 1.0f : -1.0f;
        else normal.z = (d.z > 0) ? 1.0f : -1.0f;
    }
    isect.normal = glm::vec4(normal, 0.0f);

    // Compute UV from the local hit position on the face
    // get tangent as well
    glm::vec3 p = glm::vec3(isect.point);
    glm::vec3 size = bmax - bmin;
    glm::vec3 tangent;
    if (normal.x != 0.0f) {          // hit X face, project onto YZ
        isect.uv = glm::vec2((p.z - bmin.z) / size.z,(p.y - bmin.y) / size.y);
        tangent = glm::vec3(1, 0, 0);
    } else if (normal.y != 0.0f) {   // hit Y face, project onto XZ
        isect.uv = glm::vec2((p.x - bmin.x) / size.x,(p.z - bmin.z) / size.z);
        tangent = glm::vec3(0, 0, 1);
    } else {                          // hit Z face, project onto XY
        isect.uv = glm::vec2((p.x - bmin.x) / size.x,(p.y - bmin.y) / size.y);
        tangent = glm::vec3(1, 0, 0);
    }
    isect.tangent = glm::vec4(tangent, 0.0f);

    return true;
}


bool Sphere::intersect(const Ray& ray, Intersection& isect) {
    glm::vec3 d = glm::vec3(ray.origin); // d = a - C, origin to center
    glm::vec3 dir = glm::vec3(ray.direction);
    // (b.b)t^2 + 2(b.d)t + (d.d - R^2) = 0
    double A = glm::dot(dir, dir); 
    double B = glm::dot(dir, d) * 2;
    double C = glm::dot(d, d) - 1.0;
    double roots[2];
    int rootCount = quadraticRoots(A, B, C, roots);

    double t = -1.0;
    for (int i = 0; i < rootCount; ++i) {
        if (roots[i] > eps && (t < 0 || roots[i] < t))
            t = roots[i];
    }

    if (t < 0) {
        isect.hit = false;
        return false;
    }

    isect.t = t;
    isect.hit = true;
    isect.point = ray.origin + ray.direction * float(t);
    isect.normal =  isect.point;
    // function caller will set material
    return true;
    
}

bool Cube::intersect(const Ray& ray, Intersection& isect) {
    return intersect_slab(ray, glm::vec3(0, 0, 0), glm::vec3(1, 1, 1), isect);
}

bool Cylinder::intersect(const Ray& ray, Intersection& isect) {
    glm::vec3 o(ray.origin);
    glm::vec3 d(ray.direction);

    enum class HitKind { None, Side, Bottom, Top };
    double best_t = -1.0;
    HitKind kind = HitKind::None;

    auto consider = [&](double t, HitKind k) {
        if (t > eps && (best_t < 0 || t < best_t)) {
            best_t = t;
            kind = k;
        }
    };

    // Infinite cylinder x^2 + z^2 = 1, clipped to 0 <= y <= 1
    const double A = d.x * d.x + d.z * d.z;
    if (A > 1e-18) {
        const double B = 2.0 * (o.x * d.x + o.z * d.z);
        const double C = o.x * o.x + o.z * o.z - 1.0;
        double roots[2];
        const size_t n = quadraticRoots(A, B, C, roots);
        for (size_t i = 0; i < n; ++i) {
            const double t = roots[i];
            if (t <= eps) continue;
            const double y = o.y + t * d.y;
            if (y >= -eps && y <= 1.0 + eps) {
                consider(t, HitKind::Side);
            }
        }
    }

    // Caps at y = 0 and y = 1
    if (std::abs(d.y) > 1e-18) {
        const double t_bot = -o.y / d.y;
        if (t_bot > eps) {
            const double x = o.x + t_bot * d.x;
            const double z = o.z + t_bot * d.z;
            if (x * x + z * z <= 1.0 + 1e-9) {
                consider(t_bot, HitKind::Bottom);
            }
        }
        const double t_top = (1.0 - o.y) / d.y;
        if (t_top > eps) {
            const double x = o.x + t_top * d.x;
            const double z = o.z + t_top * d.z;
            if (x * x + z * z <= 1.0 + 1e-9) {
                consider(t_top, HitKind::Top);
            }
        }
    }

    if (best_t < 0.0) {
        isect.hit = false;
        return false;
    }

    isect.t = best_t;
    isect.hit = true;
    isect.point = ray.origin + ray.direction * static_cast<float>(best_t);
    glm::vec3 p = glm::vec3(isect.point);

    if (kind == HitKind::Side) {
        glm::vec3 nxz(p.x, 0.0f, p.z);
        isect.normal = glm::vec4(glm::normalize(nxz), 0.0f);
        const double pi = glm::pi<double>();
        const float u = static_cast<float>((std::atan2(p.z, p.x) + pi) / (2.0 * pi));
        isect.uv = glm::vec2(u, p.y);
        glm::vec3 tang(-p.z, 0.0f, p.x);
        isect.tangent = glm::vec4(glm::length(tang) > eps ? glm::normalize(tang) : glm::vec3(1, 0, 0), 0.0f);
    } else if (kind == HitKind::Bottom) {
        isect.normal = glm::vec4(0.0f, -1.0f, 0.0f, 0.0f);
        isect.uv = glm::vec2((p.x + 1.0f) * 0.5f, (p.z + 1.0f) * 0.5f);
        isect.tangent = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
    } else {
        isect.normal = glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
        isect.uv = glm::vec2((p.x + 1.0f) * 0.5f, (p.z + 1.0f) * 0.5f);
        isect.tangent = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
    }

    return true;
}

bool NonhierBox::intersect(const Ray& ray, Intersection& isect) {
    glm::vec3 bmax(m_pos.x + m_size, m_pos.y + m_size, m_pos.z + m_size);
    return intersect_slab(ray, m_pos, bmax, isect);
}

bool NonhierSphere::intersect(const Ray& ray, Intersection& isect) {
    glm::vec4 d(ray.origin - glm::vec4(m_pos, 1.0)); // d = a - C, origin to center
    // (b.b)t^2 + 2(b.d)t + (d.d - R^2) = 0
    double A = glm::dot(ray.direction, ray.direction); 
    double B = glm::dot(ray.direction, d) * 2;
    double C = glm::dot(d, d) - m_radius*m_radius;
    double roots[2];
    int rootCount = quadraticRoots(A, B, C, roots);

    // double eps = 1e-6;
    double t = -1.0;
    for (int i = 0; i < rootCount; ++i) {
        if (roots[i] > eps) {
            if (t < 0 || roots[i] < t) {
                t = roots[i];
            }
        }
    }

    if (t < 0) {
        isect.hit = false;
        return false;
    }

    isect.t = t;
    isect.hit = true;
    isect.point = ray.origin + ray.direction * float(t);
    isect.normal = (isect.point - glm::vec4(m_pos, 1.0)) / float(m_radius);
    // function caller will set material
    return true;
}



bool BoundingBox::intersect(const Ray& ray, Intersection& isect) {
    return intersect_slab(ray, m_min, m_max, isect);
}
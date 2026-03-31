// Termm--Fall 2020

#include <iostream>
#include <fstream>

#include <glm/ext.hpp>

// #include "cs488-framework/ObjFileDecoder.hpp"
#include "Mesh.hpp"

Mesh::Mesh( const std::string& fname )
	: m_vertices()
	, m_faces()
{
	std::string code;
	double vx, vy, vz;
	size_t s1, s2, s3;

	glm::vec3 mins(std::numeric_limits<float>::max());
	glm::vec3 maxs(std::numeric_limits<float>::min());

	std::ifstream ifs( fname.c_str() );
	while( ifs >> code ) {
		if( code == "v" ) {
			ifs >> vx >> vy >> vz;
			glm::vec3 v( vx, vy, vz );
			m_vertices.push_back( v );
			mins = glm::min(mins, v);
			maxs = glm::max(maxs, v);
		} else if( code == "f" ) {
			ifs >> s1 >> s2 >> s3;
			m_faces.push_back( Triangle( s1 - 1, s2 - 1, s3 - 1 ) );
		}
	}

	aabb = std::make_unique<BoundingBox>(mins, maxs);
}

   
bool Mesh::intersect(const Ray& ray, Intersection& isect) {
	double mesh_eps = 1e-6;
	#ifdef RENDER_BOUNDING_VOLUMES
		return aabb->intersect(ray, isect);
	#else
		Intersection i;
		if (!aabb->intersect(ray, i)) {
			isect.hit = false;
			return false;
		}

		// we've hit bounding box, compute actual mesh intersection

		double best_t = -1.0;
		glm::vec4 best_point;
		glm::vec4 best_normal;

		auto det = [](const glm::vec3& a, const glm::vec3& b, const glm::vec3& c) {
			return glm::dot(a, glm::cross(b, c));
		};

		for (size_t i = 0; i < m_faces.size(); ++i) {
			const Triangle& tri = m_faces[i];
			glm::vec3 p1 = m_vertices[tri.v1];
			glm::vec3 p2 = m_vertices[tri.v2];
			glm::vec3 p3 = m_vertices[tri.v3];

			// ray–triangle intersection; if hit, get t, point, face normal
			// line := a + bt = p
			// triangle := p1 + B*v1 + C*v2
			glm::vec3 v1 = p2 - p1;
			glm::vec3 v2 = p3 - p1;
			glm::vec3 O = glm::vec3(ray.origin);
			glm::vec3 D = glm::vec3(-ray.direction);
			glm::vec3 R = O - p1;

			double detM = det(D, v1, v2);
			if (std::abs(detM) < mesh_eps) continue;  // ray parallel to triangle 
			double t = det(R, v1, v2) / detM;
			double beta = det(D, R, v2) / detM;
			double gamma = det(D, v1, R) / detM;
			if (beta < 0 || gamma < 0 || beta + gamma > 1) continue;

			if (t > mesh_eps && (best_t < 0 || t < best_t)) {
				best_t = t;
				best_point = ray.origin + ray.direction * (float)t;
				best_normal = glm::vec4(glm::normalize(glm::cross(v1, v2)), 0.0f);
			}
		}

		if (best_t < 0) {
			isect.hit = false;
			return false;
		}
		isect.hit = true;
		isect.t = best_t;
		isect.point = best_point;
		isect.normal = best_normal;
		// caller sets material
		return true;
	#endif 
}

std::ostream& operator<<(std::ostream& out, const Mesh& mesh)
{
  out << "mesh {";
  /*
  
  for( size_t idx = 0; idx < mesh.m_verts.size(); ++idx ) {
  	const MeshVertex& v = mesh.m_verts[idx];
  	out << glm::to_string( v.m_position );
	if( mesh.m_have_norm ) {
  	  out << " / " << glm::to_string( v.m_normal );
	}
	if( mesh.m_have_uv ) {
  	  out << " / " << glm::to_string( v.m_uv );
	}
  }

*/
  out << "}";
  return out;
}

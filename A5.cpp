// Termm--Fall 2020

#include <vector>
#include <random>
#include <algorithm>
#include <atomic>
#include <thread>
#include <limits>
#include <cstdint>
#include <cstdlib>

#include <glm/ext.hpp>

#include "A5.hpp"
#include "RayTracing.hpp"
#include "GeometryNode.hpp"
#include "PhongMaterial.hpp"
#include "MediumMaterial.hpp"
#include "TexturedMaterial.hpp"


Ray makeCameraRay(const Camera& camera, double sampleX, double sampleY) {
	double px = (sampleX / camera.pixelWidth - 0.5) * camera.imageWidth;
	double py = (0.5 - sampleY / camera.pixelHeight) * camera.imageHeight;

	glm::vec3 pixel = camera.planeCenter + px * camera.u + py * camera.v;
	glm::vec3 rayDir = glm::normalize(pixel - camera.eye);

	return Ray{glm::vec4(camera.eye, 1.0), glm::vec4(rayDir, 0.0)};
}

float luminance(const glm::vec3& c) {
	return 0.2126f * c.r + 0.7152f * c.g + 0.0722f * c.b;
}

float sampleVariance(const std::vector<glm::vec3>& samples) {
	if (samples.size() < 2) return 0.0f;
	float mean = 0.0f;
	for (const auto& s : samples) mean += luminance(s);
	mean /= static_cast<float>(samples.size());
	float var = 0.0f;
	for (const auto& s : samples) {
		float d = luminance(s) - mean;
		var += d * d;
	}
	return var / static_cast<float>(samples.size());
}

static uint64_t mixSeed(uint64_t x) {
	x += 0x9e3779b97f4a7c15ull;
	x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ull;
	x = (x ^ (x >> 27)) * 0x94d049bb133111ebull;
	return x ^ (x >> 31);
}

static uint32_t currentFrameSeed() {
	const char* frameEnv = std::getenv("FRAME");
	const uint64_t frame = frameEnv ? static_cast<uint64_t>(std::strtoull(frameEnv, nullptr, 10)) : 0ull;
	return static_cast<uint32_t>(mixSeed(frame));
}

glm::vec3 samplePixelAdaptive(
	const Camera& camera,
	const TraceContext& ctx,
	int px, int py,
	const AdaptiveSampling& params
) {
	static const uint32_t frameSeed = currentFrameSeed();
	const uint64_t pixelKey =
		(static_cast<uint64_t>(static_cast<uint32_t>(px)) << 32) ^
		static_cast<uint64_t>(static_cast<uint32_t>(py));
	std::mt19937 rng(static_cast<uint32_t>(mixSeed(pixelKey ^ frameSeed)));
	std::uniform_real_distribution<double> dist(0.0, 1.0);

	std::vector<glm::vec3> samples;
	samples.reserve(params.coarseGrid * params.coarseGrid + params.refineGrid * params.refineGrid);

	const int n = params.coarseGrid;
	for (int i = 0; i < n; ++i) {
		for (int j = 0; j < n; ++j) {
			double sx = px + (i + dist(rng)) / n;
			double sy = py + (j + dist(rng)) / n;
			Ray ray = makeCameraRay(camera, sx, sy);
			samples.push_back(traceRay(ctx, ray, ctx.maxDepth, ctx.minContribution));
		}
	}

	float var = sampleVariance(samples);
	if (var <= params.varianceThreshold) {
		glm::vec3 sum(0.0f);
		for (const auto& s : samples) sum += s;
		return sum / static_cast<float>(samples.size());
	}

	const int m = params.refineGrid;
	for (int i = 0; i < m; ++i) {
		for (int j = 0; j < m; ++j) {
			double sx = px + (i + dist(rng)) / m;
			double sy = py + (j + dist(rng)) / m;
			Ray ray = makeCameraRay(camera, sx, sy);
			samples.push_back(traceRay(ctx, ray, ctx.maxDepth, ctx.minContribution));
		}
	}
	glm::vec3 sum(0.0f);
	for (const auto& s : samples) sum += s;
	return sum / static_cast<float>(samples.size());
} 

bool isEnteringMedium(
	const Intersection& hit,
	const Ray& ray
) {
	// For a bounded volume, front-face hits mean we are crossing into the medium.
	return glm::dot(glm::vec3(ray.direction), glm::vec3(hit.normal)) < 0.0f;
}

bool findMediumSegment(
	const TraceContext& ctx,
	const Ray& ray,
	const Intersection& entryHit,
	const MediumMaterial& medium,
	MediumSegment& segment
) {
	segment = MediumSegment{};
	segment.tEnter = 0.0;

	// Start just inside the boundary and ask the existing scene traversal for the
	// next event along this same ray. Under the current simplifying assumptions,
	// that event is either:
	// 1. the exit boundary of this same medium, or
	// 2. a solid object embedded inside the medium.
	const glm::vec4 insideOrigin = entryHit.point + eps * ray.direction;
	const Ray insideRay{insideOrigin, ray.direction, ray.indexOfRefraction};

	Intersection nextHit;
	if (!intersectScene(ctx.root, insideRay, nextHit)) {
		return false;
	}

	segment.tExit = nextHit.t;
	segment.nextHit = nextHit;
	// Under the current first-pass design, a hit on the same material means we
	// reached the far boundary of this bounded medium. Distinct media, overlapping
	// media, and starting inside a medium are deferred.
	segment.exitsMedium = (nextHit.material == const_cast<MediumMaterial*>(&medium));
	segment.hitsSolidBeforeExit = !segment.exitsMedium;

	return true;
}

glm::vec3 integrateMediumTransmittance(
	const MediumMaterial& medium,
	const glm::vec4& origin,
	const glm::vec4& direction,
	float distance
) {
	glm::vec3 T(1.0f);
	const float stepSize = std::max(medium.stepSize(), static_cast<float>(eps));
	const glm::vec3 sigmaT = medium.sigmaT();

	if (distance <= eps) {
		return T;
	}

	for (float traveled = 0.0f; traveled < distance; traveled += stepSize) {
		const float ds = std::min(stepSize, distance - traveled);
		if (ds <= 0.0f) break;

		const float sampleT = traveled + 0.5f * ds;
		const glm::vec3 samplePos = glm::vec3(origin + direction * sampleT);
		const float density = std::max(0.0f, medium.densityAt(samplePos) * medium.densityScale());
		if (density <= 0.0f) {
			continue;
		}

		T *= glm::exp(-sigmaT * density * ds);
		if (glm::compMax(T) < 1e-3f) {
			return glm::vec3(0.0f);
		}
	}

	return T;
}

SegmentTransmittance transmittanceAlongSegment(
	SceneNode* root,
	const SegmentQuery& query
) {
	SegmentTransmittance result;
	if (!root || query.maxDistance <= eps) {
		return result;
	}

	glm::vec4 origin = query.origin;
	glm::vec4 direction = query.direction;
	float traveled = 0.0f;

	const MediumMaterial* activeMedium = query.startingMedium;
	const MediumMaterial* suspendedMedium = nullptr;
	const Material* insideSurfaceMaterial = nullptr;
	float surfaceEntryDistance = 0.0f;

	while (traveled < query.maxDistance) {
		const float remaining = query.maxDistance - traveled;
		Intersection hit;
		const bool foundHit = intersectScene(root, Ray{origin, direction}, hit);

		if (!foundHit || hit.t <= eps || hit.t >= remaining - eps) {
			if (activeMedium && !insideSurfaceMaterial) {
				result.transmittance *= integrateMediumTransmittance(
					*activeMedium, origin, direction, remaining);
			}
			if (glm::compMax(result.transmittance) < 1e-3f) {
				result.transmittance = glm::vec3(0.0f);
				result.blocked = true;
			}
			return result;
		}

		if (activeMedium && !insideSurfaceMaterial) {
			result.transmittance *= integrateMediumTransmittance(
				*activeMedium, origin, direction, static_cast<float>(hit.t));
			if (glm::compMax(result.transmittance) < 1e-3f) {
				result.transmittance = glm::vec3(0.0f);
				result.blocked = true;
				return result;
			}
		}

		traveled += static_cast<float>(hit.t);
		origin = hit.point + eps * direction;

		if (const auto* medium = dynamic_cast<const MediumMaterial*>(hit.material)) {
			if (activeMedium == medium) {
				activeMedium = nullptr;
				continue;
			}

			if (!activeMedium && isEnteringMedium(hit, Ray{origin, direction})) {
				activeMedium = medium;
			}
			continue;
		}

		const auto* material = dynamic_cast<const PhongMaterial*>(hit.material);
		if (!material) {
			result.transmittance = glm::vec3(0.0f);
			result.blocked = true;
			return result;
		}

		const bool entering = glm::dot(glm::vec3(direction), glm::vec3(hit.normal)) < 0.0f;
		if (material->transparency() <= 0.0) {
			result.transmittance = glm::vec3(0.0f);
			result.blocked = true;
			return result;
		}

		if (entering && insideSurfaceMaterial == nullptr) {
			insideSurfaceMaterial = hit.material;
			surfaceEntryDistance = traveled;
			suspendedMedium = activeMedium;
			activeMedium = nullptr;
			continue;
		}

		if (!entering && hit.material == insideSurfaceMaterial) {
			const float thickness = traveled - surfaceEntryDistance;
			const float transparency = std::max(static_cast<float>(material->transparency()), 1e-6f);
			const glm::vec3 sigmaA = -glm::log(glm::vec3(transparency));
			result.transmittance *= glm::exp(-sigmaA * thickness);
			insideSurfaceMaterial = nullptr;
			activeMedium = suspendedMedium;
			suspendedMedium = nullptr;

			if (glm::compMax(result.transmittance) < 1e-3f) {
				result.transmittance = glm::vec3(0.0f);
				result.blocked = true;
				return result;
			}
		}
	}

	return result;
}

MediumIntegrationResult integrateMedium(
	const TraceContext& ctx,
	const Ray& ray,
	const Intersection& entryHit,
	const MediumMaterial& medium,
	const MediumSegment& segment
) {
	MediumIntegrationResult result;
	const float stepSize = std::max(medium.stepSize(), static_cast<float>(eps));
	const glm::vec4 marchOrigin = entryHit.point + eps * ray.direction;
	const glm::vec3 sigmaT = medium.sigmaT();
	const glm::vec3 sigmaS = medium.sigmaS();
	const glm::vec3 scatterTint = medium.scatteringColour();

	if (segment.tExit <= eps) {
		return result;
	}

	// March from just inside the entry boundary toward the next event. This
	// integrates single scattering and Beer-Lambert attenuation along the camera ray.
	for (float traveled = 0.0f; traveled < segment.tExit; traveled += stepSize) {
		const float ds = std::min(stepSize, static_cast<float>(segment.tExit) - traveled);
		if (ds <= 0.0f) break;

		const float sampleT = traveled + 0.5f * ds;
		const glm::vec3 samplePos = glm::vec3(marchOrigin + ray.direction * sampleT);

		// Density currently returns a dummy value. Later this becomes fBm/noise
		// evaluated inside the medium's bounded region.
		const float density = std::max(0.0f, medium.densityAt(samplePos) * medium.densityScale());
		if (density <= 0.0f) {
			continue;
		}

		const glm::vec3 opticalDepth = sigmaT * density * ds;
		const glm::vec3 stepTransmittance = glm::exp(-opticalDepth);

		glm::vec3 inscattered(0.0f);
		for (const auto* light : ctx.lights) {
			const glm::vec3 toLight = light->position - samplePos;
			const float lightDist = glm::length(toLight);
			if (lightDist <= eps) continue;

			const float atten = 1.0f /
				static_cast<float>(light->falloff[0] +
					light->falloff[1] * lightDist +
					light->falloff[2] * lightDist * lightDist);

			const glm::vec3 lightDir = toLight / lightDist;
			const SegmentTransmittance lightSegment = transmittanceAlongSegment(
				ctx.root,
				SegmentQuery{
					glm::vec4(samplePos + lightDir * static_cast<float>(shadowBias), 1.0f),
					glm::vec4(lightDir, 0.0f),
					lightDist,
					&medium
				}
			);
			if (glm::compMax(lightSegment.transmittance) <= 1e-6f) {
				continue;
			}

			// Isotropic phase function for a simple cloud/fog baseline.
			// const float phase = 1.0f / (4.0f * glm::pi<float>());
			const float phase = 1.0f; // MAGIC VALUE. keeping it high else clouds look black
			inscattered += lightSegment.transmittance * light->colour * atten * phase;
		}

		result.radiance += result.transmittance *
			(scatterTint * sigmaS * density * ds) *
			inscattered;
		result.transmittance *= stepTransmittance;

		if (glm::compMax(result.transmittance) < 1e-3f) {
			result.transmittance = glm::vec3(0.0f);
			break;
		}
	}

	return result;
}

glm::vec3 traceMedium(
	const TraceContext& ctx,
	const Ray& ray,
	const Intersection& entryHit,
	const MediumMaterial& medium,
	int depthRemaining,
	float contribution
) {
	MediumSegment segment;
	if (!findMediumSegment(ctx, ray, entryHit, medium, segment)) {
		// If we fail to find a continuation event, fall back to whatever lies beyond
		// the volume. This keeps the skeleton safe until marching is implemented.
		return backgroundColour(ray);
	}

	const MediumIntegrationResult mediumResult =
		integrateMedium(ctx, ray, entryHit, medium, segment);

	if (segment.hitsSolidBeforeExit) {
		// The solid's contribution should be filtered by the camera-to-solid
		// transmittance accumulated through the medium.
		const float downstreamContribution =
			contribution * std::max(0.0f, glm::compMax(mediumResult.transmittance));
		return mediumResult.radiance +
			mediumResult.transmittance *
			shade(segment.nextHit, ray, ctx, depthRemaining, downstreamContribution);
	}

	// Otherwise the next event is the exit boundary. Continue tracing from just
	// beyond that boundary and composite it behind the integrated medium.
	const glm::vec4 exitOrigin = segment.nextHit.point + eps * ray.direction;
	const Ray exitRay{exitOrigin, ray.direction, ray.indexOfRefraction};
	const float downstreamContribution =
		contribution * std::max(0.0f, glm::compMax(mediumResult.transmittance));

	return mediumResult.radiance +
		mediumResult.transmittance *
		traceRay(ctx, exitRay, depthRemaining, downstreamContribution);
}


glm::vec3 traceRay(
	const TraceContext& ctx,
	const Ray& ray,
	int depthRemaining,
	float contribution
) {
	Intersection i;
	intersectScene(ctx.root, ray, i);
	if (i.hit) {
		if (const auto* medium = dynamic_cast<const MediumMaterial*>(i.material)) {
			// The first medium pass only supports the common case of entering from
			// outside. Exit hits are passed through so we do not treat the boundary
			// as an opaque surface.
			if (isEnteringMedium(i, ray)) {
				return traceMedium(ctx, ray, i, *medium, depthRemaining, contribution);
			}

			const glm::vec4 exitOrigin = i.point + eps * ray.direction;
			return traceRay(ctx, Ray{exitOrigin, ray.direction, ray.indexOfRefraction}, depthRemaining, contribution);
		}

		return shade(i, ray, ctx, depthRemaining, contribution);
	}
	return backgroundColour(ray);
}

bool intersectScene(
    SceneNode* node,
    const Ray& ray,
    Intersection& closestHit
) {
	if (!node) return false;

	// Transform ray into this node's local space
    glm::vec4 localOrigin = node->invtrans * ray.origin;
    glm::vec4 localDir = node->invtrans * ray.direction; // don't normalise
	Ray localRay{localOrigin, localDir};

	bool hitAnything = false;

	// test ourselves
	if (node->m_nodeType == NodeType::GeometryNode) {
        GeometryNode* gn = static_cast<GeometryNode*>(node);
        Intersection hit;
		gn->m_primitive->intersect(localRay, hit);
        if (hit.hit && hit.t < closestHit.t) { 
			closestHit = hit; 
			closestHit.material = gn->m_material; 
			hitAnything = true; 
		}
    }

	// recurse
	for (SceneNode* child : node->children) {
        if (intersectScene(child, localRay, closestHit)) {
            hitAnything = true;
        }
    }

	// If we hit anything, transform the result back to CALLER coordinate space 
    if (hitAnything) {
        closestHit.point  = node->trans * closestHit.point;
        closestHit.normal = glm::vec4(glm::normalize(glm::mat3(glm::transpose(node->invtrans)) * glm::vec3(closestHit.normal)), 0.0f);
		closestHit.tangent = glm::vec4(glm::normalize(glm::mat3(node->trans) * glm::vec3(closestHit.tangent)), 0.0f);
        closestHit.t = glm::length(glm::vec3(closestHit.point - ray.origin));
    }

    return hitAnything;
}

glm::vec3 shade(
	const Intersection& hit,
	const Ray& ray,
	const TraceContext& ctx,
	int depthRemaining,
	float contribution
) {
	glm::vec3 local = lighting(hit, glm::vec3(ray.direction), ctx.root, ctx.lights, ctx.ambient);

	const PhongMaterial* material = static_cast<const PhongMaterial*>(hit.material);

	if (depthRemaining <= 0 || contribution < eps || !material->refractiveIndex()) {
		return local;
	}

	glm::vec3 refractionResult(0.0);
	glm::vec3 reflectionResult(0.0);
	double reflectance = 0.0;

	double n1 = ray.indexOfRefraction;
	double n2 = material->refractiveIndex();

	float cos_theta1 = glm::dot(-ray.direction, hit.normal);
	glm::vec4 N = hit.normal;
	if (cos_theta1 < 0) {
		N = -N;
		cos_theta1 = -cos_theta1;
		std::swap(n1, n2);
	}

	float cos2_theta2 = 1.0f - (n1/n2)*(n1/n2) * (1.0f - cos_theta1*cos_theta1);
	if (cos2_theta2 >= 0) {
		float cos_theta2 = glm::sqrt(cos2_theta2);
		double R = glm::pow((n1 - n2) / (n1 + n2), 2);
		reflectance = R + (1.0 - R) * glm::pow(1.0 - cos_theta1, 5);

		auto refractDir = n1 / n2 * ray.direction + (n1 / n2 * cos_theta1 - cos_theta2) * N;
		glm::vec4 newOrigin = hit.point - N * eps;
		Ray refractedRay{newOrigin, refractDir, material->refractiveIndex()};
		refractionResult = traceRay(ctx, refractedRay, depthRemaining - 1, (1.0f - static_cast<float>(reflectance)) * contribution);
	} else {
		reflectance = 1.0;
	}

	glm::vec4 newOrigin = hit.point + N * eps;
	glm::vec4 reflectDir = glm::reflect(ray.direction, N);
	Ray reflectedRay{newOrigin, reflectDir, ray.indexOfRefraction};
	reflectionResult = traceRay(ctx, reflectedRay, depthRemaining - 1, static_cast<float>(reflectance) * contribution);

	return static_cast<float>(reflectance) * reflectionResult +
		(1.0f - static_cast<float>(reflectance)) * (
			material->transparency() * refractionResult +
			(1.0f - material->transparency()) * local
		);
}

glm::vec3 lighting(
    const Intersection& hit,
    const glm::vec3& viewDir,
    SceneNode* root,
    const std::list<Light*>& lights,
    const glm::vec3& ambient
) {
	// we assuming everything a PhongMaterial today

	glm::vec3 ret(0,0,0);

	auto material = static_cast<const PhongMaterial*>(hit.material);

	// use texture kd if exists. 
	glm::vec3 kd = material->kd();
	const TexturedMaterial* texMat = dynamic_cast<const TexturedMaterial*>(material);
	if (texMat) {
		if (texMat->texture().hasDiffuse()) {
			glm::vec3 texColor = texMat->texture().sampleDiffuse(hit.uv);
			kd = kd * texColor;  // modulate kd by texture
		}
	}
	ret += ambient * kd; // ambient portion. blinnPhong() or phong() handles the diffuse + specular

	for (auto light : lights) {
		// if (!isShadowed( hit, *light, root)) {
		// 	ret += blinnPhong(hit, static_cast<const PhongMaterial*>(hit.material), *light, viewDir);
		// }
		glm::vec3 transmittance = shadowTransmittance(hit, *light, root);
		if (glm::compMax(transmittance) > 1e-6f) {
			glm::vec3 colour = blinnPhong(hit, static_cast<const PhongMaterial*>(hit.material), *light, viewDir);
			ret += transmittance * colour;
		}
	}

	return ret;
}

/*
glm::vec3 phong(
    const Intersection& hit,
    const PhongMaterial* material,
    const Light& light,
    const glm::vec3& viewDir
) {
    glm::vec3 N = glm::vec3(hit.normal);
    glm::vec3 toLight = light.position - glm::vec3(hit.point);
    glm::vec3 L = glm::normalize(toLight);
    glm::vec3 V = glm::normalize(-viewDir);          // surface to viewer
    glm::vec3 R = glm::normalize(-L + 2.0f * glm::dot(L, N) * N); // reflect L about N

    float d = glm::length(toLight);
    float atten = 1.0f / (light.falloff[0] + light.falloff[1] * d + light.falloff[2] * d * d);

    float NdotL = std::max(0.0f, glm::dot(N, L));
    float RdotV = std::max(0.0f, glm::dot(R, V));

    glm::vec3 diffuse  = material->kd() * light.colour * NdotL * atten;
    glm::vec3 specular = material->ks() * light.colour * std::pow(RdotV, (float)material->shininess()) * atten;

    return diffuse + specular;
}
*/


glm::vec3 blinnPhong(
    const Intersection& hit,
    const PhongMaterial* material,
    const Light& light, // assumes light in line of sight 
    const glm::vec3& viewDir // viewer to surface, aka ray.direction
) {
	// L_out(v) = [k_a*I_a +] k_d(l.n)I_d + k_s(h.n)^p * I_s
	// this function is not responsible for ambient  

	glm::vec3 N = glm::vec3(hit.normal);
	glm::vec3 toLight = light.position - glm::vec3(hit.point);
	glm::vec3 L = glm::normalize(toLight);
	glm::vec3 H = glm::normalize(L - viewDir); // H = L + V

	float d = glm::length(toLight);
	float atten = 1.0f / (light.falloff[0] + light.falloff[1] * d + light.falloff[2] * d * d);

	// compute kd: check if we have a texture
	glm::vec3 kd = material->kd();
	const TexturedMaterial* texMat = dynamic_cast<const TexturedMaterial*>(material);
	if (texMat) {
		if (texMat->texture().hasDiffuse()) {
			glm::vec3 texColor = texMat->texture().sampleDiffuse(hit.uv);
			kd = kd * texColor;  // modulate kd by texture
		}
		if (texMat->texture().hasNormal()) { // normal GL format, in [-1, 1] 
			glm::vec3 T = glm::normalize(glm::vec3(hit.tangent));
			glm::vec3 B = glm::normalize(glm::cross(N, T));
			T = glm::cross(B, N);  // re-orthogonalize
			
			glm::mat3 TBN(T, B, N);
			glm::vec3 texN = texMat->texture().sampleNormal(hit.uv);
			N = glm::normalize(TBN * texN);
		}
	}

	glm::vec3 diffuse = kd * light.colour * std::max(0.0f, glm::dot(N, L)) * atten; // k_d(l.n)I_d
	glm::vec3 specular = material->ks() * light.colour * std::pow(std::max(0.0f, glm::dot(N, H)), float(material->shininess())) * atten; // k_s(h.n)^p * I_s

	return diffuse + specular;
}

bool isShadowed(
    const Intersection& hit,
    const Light& light,
    SceneNode* root
) {
	Intersection blocker;
	glm::vec3 toLight = light.position - glm::vec3(hit.point);
	const float toLightLen = glm::length(toLight);
	if (toLightLen <= eps) return false;
	const glm::vec3 L = toLight / toLightLen;
	const glm::vec3 N = glm::normalize(glm::vec3(hit.normal));
	const float signNL = (glm::dot(N, L) >= 0.0f) ? 1.0f : -1.0f;
	glm::vec4 shadowOrigin = hit.point + static_cast<float>(shadowBias) * signNL * glm::vec4(N, 0.0f);
	toLight = light.position - glm::vec3(shadowOrigin);

	if (!intersectScene(root, Ray{ shadowOrigin , glm::vec4(glm::normalize(toLight), 0.0) }, blocker)) return false;  

	if (blocker.t > shadowBias && blocker.t < glm::length(toLight) - shadowBias) {
		return true;
	}

	return false;
}

glm::vec3 shadowTransmittance(
    const Intersection& hit,
    const Light& light,
    SceneNode* root
) {
    glm::vec3 toLight = light.position - glm::vec3(hit.point);
    const float toLightLen = glm::length(toLight);
    if (toLightLen <= eps) {
        return glm::vec3(1.0f);
    }

    const glm::vec3 dir = toLight / toLightLen;
    const glm::vec3 N = glm::normalize(glm::vec3(hit.normal));
    const float signNL = (glm::dot(N, dir) >= 0.0f) ? 1.0f : -1.0f;
    const glm::vec4 shadowOrigin = hit.point + static_cast<float>(shadowBias) * signNL * glm::vec4(N, 0.0f);

    toLight = light.position - glm::vec3(shadowOrigin);
    glm::vec3 shadowDir = glm::normalize(toLight);
    const float maxDist = glm::length(toLight);
    if (maxDist <= eps) {
        return glm::vec3(1.0f);
    }

	const SegmentTransmittance segment = transmittanceAlongSegment(
		root,
		SegmentQuery{
			shadowOrigin,
			glm::vec4(shadowDir, 0.0f),
			maxDist,
			nullptr
		}
	);

    return segment.transmittance;
}

glm::vec3 backgroundColour(const Ray& ray) {
	// ASSUME: ray should already be normalised 

	float up = ray.direction.y;  // +1 = zenith, 0 = horizon, -1 = nadir 

    const glm::vec3 skyZenith  = glm::vec3(0.05f, 0.05f, 0.25f);   // dark blue
    const glm::vec3 skyHorizon = glm::vec3(0.5f,  0.6f,  0.85f);   // light blue / white near horiz
    const glm::vec3 groundHorizon = glm::vec3(0.40f, 0.3f,  0.2f);  // brown near horizon
    const glm::vec3 groundNadir = glm::vec3(0.15f, 0.1f,  0.05f);   // dark brown

    // if (up >= 0.0f) {
        // Sky: blend from horizon (up=0) to zenith (up=1)
        return glm::mix(skyHorizon, skyZenith, up);
    // } else {
        // Ground: blend from horizon (up=0) to nadir (up=-1)
        // return glm::mix(groundHorizon, groundNadir, -up);
    // }
}


static void initMediumBounds(SceneNode* node, const glm::mat4& parentTransform) {
	if (!node) return;

	const glm::mat4 worldTransform = parentTransform * node->trans;

	if (node->m_nodeType == NodeType::GeometryNode) {
		auto* gn = static_cast<GeometryNode*>(node);
		if (auto* medium = dynamic_cast<MediumMaterial*>(gn->m_material)) {
			// Canonical Cube primitive occupies [0,1]^3 in local space.
			// Transform all 8 corners to world space and take the AABB.
			glm::vec3 wMin(std::numeric_limits<float>::max());
			glm::vec3 wMax(std::numeric_limits<float>::lowest());
			for (int i = 0; i < 8; ++i) {
				glm::vec3 corner(
					(i & 1) ? 1.0f : 0.0f,
					(i & 2) ? 1.0f : 0.0f,
					(i & 4) ? 1.0f : 0.0f
				);
				glm::vec3 world = glm::vec3(worldTransform * glm::vec4(corner, 1.0f));
				wMin = glm::min(wMin, world);
				wMax = glm::max(wMax, world);
			}
			medium->setBounds(wMin, wMax);
		}
	}

	for (SceneNode* child : node->children) {
		initMediumBounds(child, worldTransform);
	}
}

void A5_Render(
		// What to render  
		SceneNode * root,

		// Image to write to, set to a given width and height  
		Image & image,

		// Viewing parameters  
		const glm::vec3 & eye,
		const glm::vec3 & view,
		const glm::vec3 & up,
		double fovy,

		// Lighting parameters  
		const glm::vec3 & ambient,
		const std::list<Light *> & lights,

		const RenderPreset& preset
) {

  	std::cout << "F20: Calling A5_Render(\n" <<
		  "\t" << *root <<
          "\t" << "Image(width:" << image.width() << ", height:" << image.height() << ")\n"
          "\t" << "eye:  " << glm::to_string(eye) << std::endl <<
		  "\t" << "view: " << glm::to_string(view) << std::endl <<
		  "\t" << "up:   " << glm::to_string(up) << std::endl <<
		  "\t" << "fovy: " << fovy << std::endl <<
          "\t" << "ambient: " << glm::to_string(ambient) << std::endl <<
		  "\t" << "lights{" << std::endl;

	for(const Light * light : lights) {
		std::cout << "\t\t" <<  *light << std::endl;
	}
	std::cout << "\t}" << std::endl;
	std::cout << "\t" << "preset: coarse=" << preset.adaptive.coarseGrid
		  << " refine=" << preset.adaptive.refineGrid
		  << " varThr=" << preset.adaptive.varianceThreshold
		  << " maxDepth=" << preset.maxDepth << std::endl;
	std::cout <<")" << std::endl;

	size_t h = image.height();
	size_t w = image.width();
	double aspect = (double)w / (double)h;

	glm::vec3 w_vec = normalize(view); // view direction
	glm::vec3 u_vec = normalize(cross(w_vec, up)); // camera right 
	glm::vec3 v_vec = normalize(cross(u_vec, w_vec)); // camera up

	// double imagey = tan(glm::radians(fovy)) * 2;
	double imagey = 2.0 * tan(glm::radians(fovy / 2.0));
	double imagex = aspect * imagey;

	glm::vec3 planeCenter = eye + w_vec;

	Camera camera{
		eye,
		planeCenter,
		u_vec,
		v_vec,
		imagex,
		imagey,
		w,
		h
	};
	initMediumBounds(root, glm::mat4(1.0f));

	TraceContext ctx{root, lights, ambient, preset.maxDepth, 1.0f};

	const unsigned numThreads = std::max(1u, std::thread::hardware_concurrency());
	auto renderTile = [&](size_t xBegin, size_t xEnd, size_t yBegin, size_t yEnd) {
		for (size_t y = yBegin; y < yEnd; ++y) {
			for (size_t x = xBegin; x < xEnd; ++x) {
				glm::vec3 colour = samplePixelAdaptive(
					camera, ctx, static_cast<int>(x), static_cast<int>(y), preset.adaptive);
				image(static_cast<uint>(x), static_cast<uint>(y), 0) = static_cast<double>(colour.x);
				image(static_cast<uint>(x), static_cast<uint>(y), 1) = static_cast<double>(colour.y);
				image(static_cast<uint>(x), static_cast<uint>(y), 2) = static_cast<double>(colour.z);
			}
		}
	};

	if (numThreads <= 1) {
		renderTile(0, w, 0, h);
	} else {
		const size_t tileWidth = 16;
		const size_t tileHeight = 16;
		const size_t tilesX = (w + tileWidth - 1) / tileWidth;
		const size_t tilesY = (h + tileHeight - 1) / tileHeight;
		const size_t totalTiles = tilesX * tilesY;
		std::atomic<size_t> nextTile{0};

		std::vector<std::thread> workers;
		workers.reserve(numThreads);
		for (unsigned t = 0; t < numThreads; ++t) {
			workers.emplace_back([&]() {
				while (true) {
					const size_t tileIndex = nextTile.fetch_add(1, std::memory_order_relaxed);
					if (tileIndex >= totalTiles) {
						break;
					}
					const size_t tileY = tileIndex / tilesX;
					const size_t tileX = tileIndex % tilesX;
					const size_t xBegin = tileX * tileWidth;
					const size_t yBegin = tileY * tileHeight;
					const size_t xEnd = std::min(xBegin + tileWidth, w);
					const size_t yEnd = std::min(yBegin + tileHeight, h);
					renderTile(xBegin, xEnd, yBegin, yEnd);
				}
			});
		}
		for (auto& worker : workers) {
			worker.join();
		}
	}

}

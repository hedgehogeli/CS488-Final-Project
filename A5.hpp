// Termm--Fall 2020

#pragma once

#include <cstddef>
#include <list>

#include <glm/glm.hpp>

#include "SceneNode.hpp"
#include "Light.hpp"
#include "Image.hpp"
#include "RayTracing.hpp"
#include "MediumMaterial.hpp"
#include "PhongMaterial.hpp"

struct Camera {
	glm::vec3 eye;
	glm::vec3 planeCenter;
	glm::vec3 u;
	glm::vec3 v;
	double imageWidth;
	double imageHeight;
	size_t pixelWidth;
	size_t pixelHeight;
};

struct TraceContext {
	SceneNode* root;
	const std::list<Light*>& lights;
	glm::vec3 ambient;
	int maxDepth;
	float minContribution;
};

struct MediumSegment {
	double tEnter = 0.0;
	double tExit = 0.0;
	bool exitsMedium = false;
	bool hitsSolidBeforeExit = false;
	Intersection nextHit;
};

struct MediumIntegrationResult {
	glm::vec3 radiance{0.0f};
	glm::vec3 transmittance{1.0f};
};

struct SegmentQuery {
	glm::vec4 origin;
	glm::vec4 direction;
	float maxDistance = 0.0f;
	const MediumMaterial* startingMedium = nullptr;
};

struct SegmentTransmittance {
	glm::vec3 transmittance{1.0f};
	bool blocked = false;
};

struct AdaptiveSampling {
	int coarseGrid;           // coarseGrid x coarseGrid initial samples
	int refineGrid;           // refineGrid x refineGrid extra samples when if var > varianceThreshold
	float varianceThreshold;
};

/**
 * Render quality presets for animation / preview vs final renders.
 * - low: fast (single primary sample per pixel, shallow ray depth)
 * - high: previous A4 defaults (adaptive supersampling, deeper rays)
 */
struct RenderPreset {
	AdaptiveSampling adaptive;
	int maxDepth;

	static RenderPreset lowQuality();
	static RenderPreset highQuality();
};

inline RenderPreset RenderPreset::lowQuality() {
	return RenderPreset{
		AdaptiveSampling{1, 0, 0.0f},
		10
	};
}

inline RenderPreset RenderPreset::highQuality() {
	return RenderPreset{
		AdaptiveSampling{3, 4, 0.02f},
		10
	};
}

Ray makeCameraRay(const Camera& camera, double sampleX, double sampleY);

glm::vec3 traceRay(
	const TraceContext& ctx,
	const Ray& ray,
	int depthRemaining,
	float contribution
);

bool isEnteringMedium(
	const Intersection& hit,
	const Ray& ray
);

bool findMediumSegment(
	const TraceContext& ctx,
	const Ray& ray,
	const Intersection& entryHit,
	const MediumMaterial& medium,
	MediumSegment& segment
);

MediumIntegrationResult integrateMedium(
	const TraceContext& ctx,
	const Ray& ray,
	const Intersection& entryHit,
	const MediumMaterial& medium,
	const MediumSegment& segment
);

glm::vec3 integrateMediumTransmittance(
	const MediumMaterial& medium,
	const glm::vec4& origin,
	const glm::vec4& direction,
	float distance
);

SegmentTransmittance transmittanceAlongSegment(
	SceneNode* root,
	const SegmentQuery& query
);

glm::vec3 traceMedium(
	const TraceContext& ctx,
	const Ray& ray,
	const Intersection& entryHit,
	const MediumMaterial& medium,
	int depthRemaining,
	float contribution
);

glm::vec3 samplePixelAdaptive(
	const Camera& camera,
	const TraceContext& ctx,
	int px, int py,
	const AdaptiveSampling& params
);

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

		// Sampling / depth ( default = highQuality )
		// const RenderPreset& preset = RenderPreset::lowQuality()
		const RenderPreset& preset = RenderPreset::highQuality()
);

bool intersectScene(
	SceneNode* node,
	const Ray& ray,
	Intersection& closestHit
);

glm::vec3 shade(
	const Intersection& hit,
	const Ray& ray,
	const TraceContext& ctx,
	int depthRemaining,
	float contribution
);

glm::vec3 lighting(
    const Intersection& hit,
    const glm::vec3& viewDir,
    SceneNode* root,
    const std::list<Light*>& lights,
    const glm::vec3& ambient
);


glm::vec3 phong(
	const Intersection& hit,
	const PhongMaterial* material,
	const Light& light,
	const glm::vec3& viewDir
);

glm::vec3 blinnPhong(
	const Intersection& hit,
	const PhongMaterial* material,
	const Light& light,
	const glm::vec3& viewDir
);

bool isShadowed(
	const Intersection& hit,
	const Light& light,
	SceneNode* root
);

glm::vec3 shadowTransmittance(
	const Intersection& hit,
	const Light& light,
	SceneNode* root
);

glm::vec3 backgroundColour(const Ray& ray);
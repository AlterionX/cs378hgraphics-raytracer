#include <cmath>
#include <iostream>

#include "../ui/TraceUI.h"
extern TraceUI* traceUI;

#include "light.h"
#include <glm/glm.hpp>
#include <glm/gtx/io.hpp>

#define EPS_BACKUP 0.0000000001
#define SHADOW_INF 1e18

using namespace std;

double DirectionalLight::distanceAttenuation(const glm::dvec3& P) const
{
	// distance to light is infinite, so f(di) goes to 0.  Return 1.
	return 1.0;
}


glm::dvec3 DirectionalLight::shadowAttenuation(const ray& r, const glm::dvec3& p) const
{
	// back-up by eps
	glm::dvec3 pb = p - r.getDirection() * EPS_BACKUP;

	// trace shadow
	glm::dvec3 sattn(1.0, 1.0, 1.0);
	ray r2l(pb, getDirection(pb), glm::dvec3(1.0, 1.0, 1.0), ray::SHADOW);
	vector<isect> iv = scene->intersectList(r2l);
	std::sort(iv.begin(), iv.end(), [](const isect& a, const isect& b) { return a.getT() < b.getT(); });
	// for this ray, do intersect and move forward until nothing happens
	for (auto iv_it : iv) {
		const Material& m_in = iv_it.getMaterial();
		const bool is_inside = glm::dot(iv_it.getN(), r2l.getDirection()) > 0;

		r2l.setPosition(r2l.at(iv_it.getT()));
		const Material& m_out = traceUI->overlappingObjects() ? scene->discoverMat(r2l) : air;

		auto curr_m = is_inside ? m_in : m_out;
		auto next_m = is_inside ? m_out : m_in;

		if (!next_m.Trans()) return glm::dvec3(0.0);
		if (traceUI->aTermSwitch() && glm::dot(sattn, sattn) < traceUI->getATermThresh()*traceUI->getATermThresh())  return glm::dvec3(0.0);
		sattn *= glm::pow(curr_m.kt(iv_it), glm::dvec3(iv_it.getT()));
	}

	return sattn;
}

glm::dvec3 DirectionalLight::getColor() const
{
	return color;
}

glm::dvec3 DirectionalLight::getDirection(const glm::dvec3& P) const
{
	return -orientation;
}

double PointLight::distanceAttenuation(const glm::dvec3& P) const
{

	// YOUR CODE HERE

	// You'll need to modify this method to attenuate the intensity
	// of the light based on the distance between the source and the
	// point P.  For now, we assume no attenuation and just return 1.0
	double d = glm::distance(position, P);
	return max(min(1.0 / (constantTerm + linearTerm * d + quadraticTerm * d*d), 1.0), 0.0);
}

glm::dvec3 PointLight::getColor() const
{
	return color;
}

glm::dvec3 PointLight::getDirection(const glm::dvec3& P) const
{
	return glm::normalize(position - P);
}

// assumption: call from out of material
glm::dvec3 PointLight::shadowAttenuation(const ray& r, const glm::dvec3& p) const {
	// back-up by eps
	glm::dvec3 pb = p - r.getDirection() * EPS_BACKUP;

	// trace shadow
	glm::dvec3 sattn(1.0, 1.0, 1.0);
	ray r2l(pb, getDirection(pb), glm::dvec3(1.0, 1.0, 1.0), ray::SHADOW);
	vector<isect> iv = scene->intersectList(r2l);
	std::sort(iv.begin(), iv.end(), [](const isect& a, const isect& b) { return a.getT() < b.getT(); });
	// for this ray, do intersect and move forward until nothing happens
	for (auto iv_it : iv) {
		const Material& m_in = iv_it.getMaterial();
		const bool is_inside = glm::dot(iv_it.getN(), r2l.getDirection()) > 0;

		r2l.setPosition(r2l.at(iv_it.getT()));
		const Material& m_out = traceUI->overlappingObjects() ? scene->discoverMat(r2l) : air;

		auto curr_m = is_inside ? m_in : m_out;
		auto next_m = is_inside ? m_out : m_in;

		// check point light
		if (glm::dot(
			glm::normalize(position - r2l.at(iv_it.getT())),
			r2l.getDirection()) <= 0
			) {  // went beyond the point light
			if (is_inside) {
				sattn *= glm::pow(curr_m.kt(iv_it), glm::dvec3(glm::distance(position, r2l.getPosition())));
			}
			break;
		}

		if (!next_m.Trans()) return glm::dvec3(0.0);
		if (traceUI->aTermSwitch() && glm::dot(sattn, sattn) < traceUI->getATermThresh()*traceUI->getATermThresh())  return glm::dvec3(0.0);
		sattn *= glm::pow(curr_m.kt(iv_it), glm::dvec3(iv_it.getT()));
	}

	return sattn;
}

#define VERBOSE 0

#include <cmath>
#include <iostream>

#include "../ui/TraceUI.h"
extern TraceUI* traceUI;

#include "light.h"
#include <glm/glm.hpp>
#include <glm/gtx/io.hpp>

#define EPS_BACKUP 0.0000000001
#define SHADOW_INF 1e18

glm::dvec3 Light::shadowAttenuation(const ray& r, const glm::dvec3& pos) const {
	// back-up by eps
	glm::dvec3 pb = pos - r.getDirection() * EPS_BACKUP;
	return srsAttenuation(pb, getDirection(pb));
}
glm::dvec3 Light::srsAttenuation(const glm::dvec3& pos, const glm::dvec3& dir) const {
	// trace shadow
	glm::dvec3 sattn(1.0, 1.0, 1.0);
	ray r2l(pos, dir, glm::dvec3(1.0, 1.0, 1.0), ray::SHADOW);
	std::vector<isect> iv = scene->intersectList(r2l);
	std::sort(iv.begin(), iv.end(), [](const isect& a, const isect& b) { return a.getT() < b.getT(); });

	// for this ray, handle intersect and move forward until nothing happens
	double last_t = 0.0;
	for (auto iv_it : iv) {
		double t = iv_it.getT() - last_t;
		last_t = iv_it.getT();
		iv_it.setT(t);

		const Material& m_in = iv_it.getMaterial();
		const bool is_inside = glm::dot(iv_it.getN(), r2l.getDirection()) > 0;

		r2l.setPosition(r2l.at(iv_it.getT()));
		const Material& m_out = traceUI->overlappingObjects() ? scene->discoverMat(ray(r2l)) : air;

		auto curr_m = is_inside ? m_in : m_out;
		auto next_m = is_inside ? m_out : m_in;

		// went beyond a non-infinite light source
		if (sattnLimitCheck(r2l, iv_it, sattn, next_m, curr_m)) return sattn;

		if (!next_m.Trans()) return glm::dvec3(0.0);
		if (traceUI->aTermSwitch() && glm::dot(sattn, sattn) < traceUI->getATermThresh()*traceUI->getATermThresh()) return glm::dvec3(0.0);
		sattn *= glm::pow(curr_m.kt(iv_it), glm::dvec3(iv_it.getT()));
	}

	return sattn;
}

// distance to light is infinite, so f(di) goes to 0.  Return 1.
double DirectionalLight::distanceAttenuation(const glm::dvec3& P) const { return 1.0; }
bool DirectionalLight::sattnLimitCheck(const ray& r, const isect& i, glm::dvec3& sattn, const Material& n, const Material& c) const { return false; }
glm::dvec3 DirectionalLight::getColor() const { return color; }
glm::dvec3 DirectionalLight::getDirection(const glm::dvec3& P) const { return -orientation; }

double PointLight::distanceAttenuation(const glm::dvec3& P) const {
	double d = glm::distance(position, P);
	return glm::clamp(1.0 / (constantTerm + linearTerm * d + quadraticTerm * d * d), 0.0, 1.0);
}
bool PointLight::sattnLimitCheck(const ray& r, const isect& i, glm::dvec3& sattn, const Material& n, const Material& c) const {
	if (glm::dot(position - r.at(i.getT()), r.getDirection()) <= 0) {  
		// if (glm::dot(i.getN(), r.getDirection()) > 0) sattn *= glm::pow(c.kt(i), glm::dvec3(glm::distance(position, r.getPosition())));
		return true;
	}
	return false;
}
glm::dvec3 PointLight::getColor() const { return color; }
glm::dvec3 PointLight::getDirection(const glm::dvec3& P) const { return glm::normalize(position - P); }

// assumption: call from out of material
glm::dvec3 AreaLight::shadowAttenuation(const ray& r, const glm::dvec3& p) const {
	if (!validImpact(r, p)) {
		return glm::dvec3(0.0);
	}
	auto sattn = glm::dvec3(1.0);
	for (int i = 0; i < traceUI->softShadowRes(); i++) {
		//pick point on area light
		glm::dvec3 lpos = pick(i);
		auto pb = p - r.getDirection() * EPS_BACKUP;
		sattn += srsAttenuation(pb, glm::normalize(lpos - pb));
	}
	sattn *= (1.0/traceUI->softShadowRes());
	return sattn;
}
bool AreaLight::sattnLimitCheck(const ray& r, const isect& i, glm::dvec3& sattn, const Material& n, const Material& c) const {
	glm::dvec3 imp = impact(r);
	// Note, duplicate code
	if (glm::dot(position - r.at(i.getT()), r.getDirection()) <= 0) {
		if (glm::dot(i.getN(), r.getDirection()) > 0) sattn *= glm::pow(c.kt(i), glm::dvec3(glm::distance(position, r.getPosition())));
		return true;
	}
	return false;
}

bool AreaLightRect::validImpact(const ray& r, const glm::dvec3& p) const {

}
glm::dvec3 AreaLightRect::pick(const int i) const {

}
glm::dvec3 AreaLightRect::impact(const ray& r) const {

}

bool AreaLightCirc::validImpact(const ray& r, const glm::dvec3& p) const {

}
glm::dvec3 AreaLightCirc::pick(const int i) const {

}
glm::dvec3 AreaLightCirc::impact(const ray& r) const {

}

bool SpotLight::validImpact(const ray& r, const glm::dvec3& p) const {

}
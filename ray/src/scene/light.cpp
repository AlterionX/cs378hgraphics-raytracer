#include <cmath>
#include <iostream>

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
	// YOUR CODE HERE:
	// You should implement shadow-handling code here.
	// std::cout << "DirLight Shadow" << std::endl;

	// back-up by eps
	glm::dvec3 pb = p - r.getDirection()*EPS_BACKUP;

	// trace shadow
	glm::dvec3 sattn(1.0, 1.0, 1.0);
	isect cur;
	for (
		ray r2l(pb, getDirection(pb), glm::dvec3(1.0, 1.0, 1.0), ray::SHADOW);
		scene->intersect(r2l, cur);
		r2l.setPosition(r2l.at(cur.getT() + EPS_BACKUP))
	) {
		const Material& m = cur.getMaterial();
		const glm::dvec3 kt_val = m.kt(cur);
		bool is_inside = glm::dot(cur.getN(), r2l.getDirection()) > 0;

		// check material
		if(!m.Trans()) return glm::dvec3(0.0, 0.0, 0.0);
		// jump to next intersection
		if(is_inside) sattn *= glm::pow(kt_val, glm::dvec3(cur.getT()));
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
	return max(min(1.0 / (constantTerm + linearTerm*d + quadraticTerm*d*d), 1.0), 0.0);
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
glm::dvec3 PointLight::shadowAttenuation(const ray& r, const glm::dvec3& p) const
{
	// YOUR CODE HERE:
	// You should implement shadow-handling code here.
	// std::cout << "P'Light Shadow" << std::endl;

	// back-up by eps
	glm::dvec3 pb = p - r.getDirection() * EPS_BACKUP;

	// trace shadow
	glm::dvec3 sattn(1.0, 1.0, 1.0);
	isect cur;
	// for this ray, do intersect and move forward until nothing happens
	for (
		ray r2l(pb, getDirection(pb), glm::dvec3(1.0, 1.0, 1.0), ray::SHADOW);
		scene->intersect(r2l, cur);
		r2l.setPosition(r2l.at(cur.getT() + EPS_BACKUP))
	) {
		const Material& m = cur.getMaterial();
		const glm::dvec3 kt_val = m.kt(cur);
		const bool is_inside = glm::dot(cur.getN(), r2l.getDirection()) > 0;

		// check point light
		if(glm::dot(
			glm::normalize(position - r2l.at(cur.getT())),
			r2l.getDirection()) <= 0
		) {  // went beyond the point light
			if(is_inside) {
					sattn *= glm::pow(kt_val, glm::dvec3(glm::distance(position, r2l.getPosition())));
			}
			break;
		}

		// check material
		if(!m.Trans()) return glm::dvec3(0.0, 0.0, 0.0);

		// jump to next intersection
		if (is_inside) sattn *= glm::pow(kt_val, glm::dvec3(cur.getT()));
	}

	return sattn;
}

#define VERBOSE 0

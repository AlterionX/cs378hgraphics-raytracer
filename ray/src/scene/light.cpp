#include <cmath>
#include <iostream>

#include "light.h"
#include <glm/glm.hpp>
#include <glm/gtx/io.hpp>

#define EPS_BACKUP 0.0000001
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
	ray r2l(pb, getDirection(pb), glm::dvec3(1.0, 1.0, 1.0), ray::SHADOW);
	glm::dvec3 sattn(1.0, 1.0, 1.0);
	isect cur;
	while(scene->intersect(r2l, cur)) {
		// std::cout << "shadow hit: " << r2l.getPosition() << " -> " << r2l.at(cur.getT()) << std::endl;
		const Material& m = cur.getMaterial();
		const glm::dvec3 kt_val = m.kt(cur);
		bool is_inside = glm::dot(cur.getN(), r2l.getDirection()) >= 0;

		// check material
		if(!m.Trans()) {
			// std::cout << "OPAQUE!!!" << std::endl;
			return glm::dvec3(0.0, 0.0, 0.0);
		}

		// jump to next intersection
		if(is_inside) {
			// std::cout << "transmitting with " << kt_val << " " << cur.getT() << std::endl;
			sattn = sattn * glm::pow(kt_val, glm::dvec3(cur.getT()));
		}
		is_inside = !is_inside;
		r2l.setPosition(r2l.at(cur.getT()));
		// std::cout << "clean jump: " << sattn << std::endl;
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
	glm::dvec3 pb = p - r.getDirection()*EPS_BACKUP;

	// trace shadow
	ray r2l(pb, getDirection(pb), glm::dvec3(1.0, 1.0, 1.0), ray::SHADOW);
	glm::dvec3 sattn(1.0, 1.0, 1.0);
	isect cur;
	while(scene->intersect(r2l, cur)) {
	// std::cout << "shadow hit: " << r2l.getPosition() << " -> " << r2l.at(cur.getT()) << std::endl;
		const Material& m = cur.getMaterial();
		const glm::dvec3 kt_val = m.kt(cur);
		const bool is_inside = glm::dot(cur.getN(), r2l.getDirection()) > 0;

		// check point light
		if(glm::dot(
			glm::normalize(position - r2l.at(cur.getT())),
			r2l.getDirection()) <= 0
		) {  // went beyond the point light
		// std::cout << "too far..." << std::endl;
		if(is_inside) {
			// std::cout << "Inside transparent surface." << std::endl;
				sattn = sattn * glm::pow(kt_val, glm::dvec3(glm::distance(position, r2l.getPosition())));
			}
			break;
		}

		// check material
		if(!m.Trans()) {
			// std::cout << "OPAQUE!!!" << std::endl;
			return glm::dvec3(0.0, 0.0, 0.0);
		}

		// jump to next intersection
		if(is_inside) {
			// std::cout << "Passing through transparent surface." << std::endl;
			// std::cout << "transmitting with " << kt_val << " " << cur.getT() << std::endl;
			// std::cout << glm::pow(kt_val, glm::dvec3(cur.getT())) << std::endl;
			sattn = sattn * glm::pow(kt_val, glm::dvec3(cur.getT()));
		}
		r2l.setPosition(r2l.at(cur.getT() + EPS_BACKUP));
		// std::cout << "clean jump: " << sattn << std::endl;
	}

// std::cout << "P'Light Shadow End" << std::endl;

	return sattn;
}

#define VERBOSE 0

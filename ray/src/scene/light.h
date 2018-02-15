#ifndef __LIGHT_H__
#define __LIGHT_H__

#ifndef _WIN32
#include <algorithm>
using std::min;
using std::max;
#endif

#include "../util.h"

#include "scene.h"
#include "../ui/TraceUI.h"
#include <FL/gl.h>

class Light : public SceneElement {
public:
	virtual double distanceAttenuation(const glm::dvec3& P) const = 0;

	virtual glm::dvec3 shadowAttenuation(const ray& r, const glm::dvec3& pos) const;
	virtual glm::dvec3 srsAttenuation(const glm::dvec3& pos, const glm::dvec3& dir) const;
	virtual bool sattnLimitCheck(const ray& r, const isect& i, glm::dvec3& sattn, const Material& n, const Material& c) const = 0;

	virtual glm::dvec3 getColor() const = 0;
	virtual glm::dvec3 getDirection(const glm::dvec3& P) const = 0;

protected:
	Light(Scene *scene, const glm::dvec3& col) : SceneElement(scene), color(col) {}

	glm::dvec3 color;

public:
	virtual void glDraw(GLenum lightID) const {}
	virtual void glDraw() const {}
};

class DirectionalLight : public Light {
public:
	DirectionalLight(Scene *scene, const glm::dvec3& orien, const glm::dvec3& color)
		: Light(scene, color), orientation(glm::normalize(orien)) {}

	virtual bool sattnLimitCheck(const ray& r, const isect& i, glm::dvec3& sattn, const Material& n, const Material& c) const;

	virtual double distanceAttenuation(const glm::dvec3& P) const;

	virtual glm::dvec3 getColor() const;
	virtual glm::dvec3 getDirection(const glm::dvec3& P) const;

protected:
	glm::dvec3 orientation;

public:
	void glDraw(GLenum lightID) const;
	void glDraw() const;
};

class PointLight : public Light {
public:
	PointLight(Scene *scene, const glm::dvec3& pos, const glm::dvec3& color,
		float constantAttenuationTerm, float linearAttenuationTerm, float quadraticAttenuationTerm)
		: Light(scene, color), position(pos),
		constantTerm(constantAttenuationTerm),
		linearTerm(linearAttenuationTerm),
		quadraticTerm(quadraticAttenuationTerm) {}

	virtual double distanceAttenuation(const glm::dvec3& P) const;

	virtual bool sattnLimitCheck(const ray& r, const isect& i, glm::dvec3& sattn, const Material& n, const Material& c) const;

	virtual glm::dvec3 getColor() const;
	virtual glm::dvec3 getDirection(const glm::dvec3& P) const;

	void setAttenuationConstants(float a, float b, float c) {
		constantTerm = a;
		linearTerm = b;
		quadraticTerm = c;
	}

protected:
	glm::dvec3 position;

	// These three values are the a, b, and c in the distance
	// attenuation function (from the slide labelled
	// "Intensity drop-off with distance"):
	//    f(d) = min( 1, 1/( a + b d + c d^2 ) )
	float constantTerm;		// a
	float linearTerm;		// b
	float quadraticTerm;	// c

public:
	void glDraw(GLenum lightID) const;
	void glDraw() const;

protected:

};


//TODO stochastic lighting
class AreaLight : public PointLight {
public:
	AreaLight(Scene *sc, const glm::dvec3& p, const glm::dvec3& col,
		float cat, float lat, float qat, const glm::dvec3& ori) :
		PointLight(sc, p, col, cat, lat, qat), ori(glm::normalize(ori)) {}

	virtual glm::dvec3 shadowAttenuation(const ray& r, const glm::dvec3& pos) const;
	virtual bool sattnLimitCheck(const ray& r, const isect& i, glm::dvec3& sattn, const Material& n, const Material& c) const;

	virtual bool validImpact(const ray& r, const glm::dvec3& p) const;
	virtual bool validImpact(const ray& r, const glm::dvec3& p, glm::dvec3& lp) const;
	virtual glm::dvec3 pick(const int i) const = 0;
	virtual glm::dvec3 impact(const ray& r) const = 0;

protected:
	glm::dvec3 ori;
};

class AreaLightRect : public AreaLight {
public:
	AreaLightRect(
		Scene *sc, const glm::dvec3& p, const glm::dvec3& col,
		float cat, float lat, float qat,
		const glm::dvec3& ori,
		double w, double h,
		const glm::dvec3& u
	) : AreaLight(sc, p, col, cat, lat, qat, ori),
		w(w), h(h), u(glm::normalize(u)), v(glm::cross(ori, u)) {}

	virtual glm::dvec3 pick(const int i) const;
	virtual glm::dvec3 impact(const ray& r) const;

protected:
	double w;
	double h;
	glm::dvec3 u; // local x vec
	glm::dvec3 v; // local y vec
};

class AreaLightCirc : public AreaLight {
public:
	AreaLightCirc(Scene *sc, const glm::dvec3& p, const glm::dvec3& col,
		float cat, float lat, float qat, const glm::dvec3& ori, double r) : AreaLight(sc, p, col, cat, lat, qat, ori), r(r) {}

	virtual glm::dvec3 pick(const int i) const;
	virtual glm::dvec3 impact(const ray& r) const;

protected:
	double r;
};

class SpotLight : public AreaLightCirc {
public:
	SpotLight(Scene *sc, const glm::dvec3& p, const glm::dvec3& col,
		float cat, float lat, float qat, const glm::dvec3& ori, double r, double ang) :
		AreaLightCirc(sc, p, col, cat, lat, qat, ori, r), ang(ang), ang_tan(glm::tan(ang / 360 * PI)), offset(ang_tan * r) {}

	virtual bool validImpact(const ray& r, const glm::dvec3& p) const;
	virtual bool validImpact(const ray& r, const glm::dvec3& p, glm::dvec3& lp) const;

protected:
	double ang;
	double ang_tan;
	double offset;
};

#endif // __LIGHT_H__

#include "material.h"
#include "../ui/TraceUI.h"
#include "light.h"
#include "ray.h"
#include "scene.h"
extern TraceUI* traceUI;

#include <glm/gtx/io.hpp>
#include <iostream>
// #include <algorithm>
#include "../fileio/images.h"

using namespace std;
extern bool debugMode;

// Some normal air material
const Material air = Material(
	glm::dvec3(0.0), glm::dvec3(0.0), glm::dvec3(0.0),
	glm::dvec3(0.0), glm::dvec3(0.0), glm::dvec3(1.0),
	0.0, 1.0, 0.0
);
const Material vantablack_mat = Material(
	glm::dvec3(0.0), glm::dvec3(0.0), glm::dvec3(0.0),
	glm::dvec3(0.0), glm::dvec3(0.0), glm::dvec3(0.0),
	0.0, 0.0, 0.0
);

Material::~Material()
{
}

// Apply the phong model to this point on the surface of the object, returning
// the color of that point.
glm::dvec3 Material::shade(Scene* scene, const ray& r, const isect& i) const
{
	// YOUR CODE HERE

	// For now, this method just returns the diffuse color of the object.
	// This gives a single matte color for every distinct surface in the
	// scene, and that's it.  Simple, but enough to get you started.
	// (It's also inconsistent with the phong model...)

	// Your mission is to fill in this method with the rest of the phong
	// shading model, including the contributions of all the light sources.
	// You will need to call both distanceAttenuation() and
	// shadowAttenuation()
	// somewhere in your code in order to compute shadows and light falloff.
	//	if( debugMode )
	//	std::cout << "Debugging Phong code..." << std::endl;

	// When you're iterating through the lights,
	// you'll want to use code that looks something
	// like this:
	//
	// for ( const auto& pLight : scene->getAllLights() )
	// {
	//              // pLight has type unique_ptr<Light>
	// 		.
	// 		.
	// 		.
	// }

	// intersection
	const glm::dvec3 isect_p = r.at(i.getT());
	const glm::dvec3 surf_n = i.getN();
	const glm::dvec3 v = r.getDirection();

	const auto sh = this->shininess(i);
	const auto kd = this->kd(i);
	const auto ks = this->ks(i);
	const auto trans = this->Trans();

	const auto& lights = scene->getAllLights();

	// local illumination: ambient, diffuse, specular
	glm::dvec3 i_out = ke(i) + ka(i) * scene->ambient();
	for(auto l_idx = 0; l_idx < lights.size(); ++l_idx) {
		const glm::dvec3 l_i = lights[l_idx]->getDirection(isect_p);
		const glm::dvec3 l_r = (l_i - 2 * (glm::dot(l_i, surf_n)) * surf_n);
		const glm::dvec3 l_color = lights[l_idx]->getColor();

		double dot = glm::dot(l_i, surf_n);
		if (trans) dot = glm::abs(dot);
		const glm::dvec3 d_comp = kd * glm::max(0.0, dot);
		const glm::dvec3 s_comp = ks * glm::pow(
			glm::dvec3(glm::max(0.0, glm::dot(l_r, v))),
			glm::dvec3(sh)
		);

		const double dattn = lights[l_idx]->distanceAttenuation(isect_p);
		const glm::dvec3 sattn = lights[l_idx]->shadowAttenuation(r, isect_p);
		i_out += dattn * sattn * lights[l_idx]->getColor() * (d_comp + s_comp);
	}

	return i_out;
}

TextureMap::TextureMap(string filename)
{
	data = readImage(filename.c_str(), width, height);
	if (data.empty()) {
		width = 0;
		height = 0;
		string error("Unable to load texture map '");
		error.append(filename);
		error.append("'.");
		throw TextureMapException(error);
	}
}

glm::dvec3 TextureMap::getMappedValue(const glm::dvec2& coord) const
{
	// YOUR CODE HERE
	//
	// In order to add texture mapping support to the
	// raytracer, you need to implement this function.
	// What this function should do is convert from
	// parametric space which is the unit square
	// [0, 1] x [0, 1] in 2-space to bitmap coordinates,
	// and use these to perform bilinear interpolation
	// of the values.

	double x = coord[0];
	double y = coord[1];

	if(0.0 <= x && x <= 1.0 && 0.0 <= y && y <= 1.0) {
		// convert & find grid
		x *= width - 1; 			y *= height - 1;
		int ix = (int) x, 		iy = (int) y;
		x -= ix; 				y -= iy;

		// std::cout << "map -> " << x << " " << y << std::endl;

		// fetch vertex values
		glm::dvec3 prows[2];
		for(int i = 0; i < 2; i++) {
			auto pl = getPixelAt(i + ix, 0 + iy);
			auto pr = getPixelAt(i + ix, 1 + iy);
			prows[i] = y * (pr - pl) + pl;
		}

		// std::cout << x << " " << y << ": return " << ((x * (prows[1] - prows[0]) + prows[0]) / 255.0) << std::endl;
		return (x * (prows[1] - prows[0]) + prows[0]) / 255.0;
	}
	return glm::dvec3(0.0, 1.0, 0.0);
}

glm::dvec3 TextureMap::getPixelAt(int x, int y) const
{
	// YOUR CODE HERE
	//
	// In order to add texture mapping support to the
	// raytracer, you need to implement this function.

	if(0 <= x && x < width && 0 <= y && y < height) {
		// copied from RayTracer.cpp
		int idx = ( x + y * width ) * 3;

		glm::dvec3 vpix(data[idx + 0], data[idx + 1], data[idx + 2]);
		// std::cout << x << " " << y << ": return " << vpix << std::endl;
		// return glm::normalize(vpix);
		return vpix;
	}
	return glm::dvec3(0.0, 0.0, 0.0);
}

glm::dvec3 MaterialParameter::value(const isect& is) const
{
	if (0 != _textureMap)
		return _textureMap->getMappedValue(is.getUVCoordinates());
	else
		return _value;
}

double MaterialParameter::intensityValue(const isect& is) const
{
	if (0 != _textureMap) {
		glm::dvec3 value(
		        _textureMap->getMappedValue(is.getUVCoordinates()));
		return (0.299 * value[0]) + (0.587 * value[1]) +
		       (0.114 * value[2]);
	} else
		return (0.299 * _value[0]) + (0.587 * _value[1]) +
		       (0.114 * _value[2]);
}

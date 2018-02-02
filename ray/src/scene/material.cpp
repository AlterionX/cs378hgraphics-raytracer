#include "material.h"
#include "../ui/TraceUI.h"
#include "light.h"
#include "ray.h"
extern TraceUI* traceUI;

#include <glm/gtx/io.hpp>
#include <iostream>
// #include <algorithm>
#include "../fileio/images.h"

using namespace std;
extern bool debugMode;

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
	//		std::cout << "Debugging Phong code..." << std::endl;

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
	const glm::dvec3 isect_n = i.getN();
	const glm::dvec3 ray_d = r.getDirection();

	// material params
	const glm::dvec3 ka_val = ka(i);
	const glm::dvec3 kd_val = kd(i);
	const glm::dvec3 ks_val = ks(i);
	const double sh_val = shininess(i);

	// local illumination: ambient, diffuse, specular
	glm::dvec3 i_out = ke(i) + ka(i) * scene->ambient();
	for(const auto& pLight: scene->getAllLights()) {
		const glm::dvec3 l_ld = pLight->getDirection(isect_p);
		const glm::dvec3 l_color = pLight->getColor();
		const double l_dattn_c = pLight->distanceAttenuation(isect_p);
		const glm::dvec3 l_sattn_c = pLight->shadowAttenuation(r, isect_p);

		// if(glm::dot(l_ld, isect_n) > 0) {
			const glm::dvec3 l_rd = l_ld - 2*(glm::dot(l_ld, isect_n))*isect_n;

			i_out += l_dattn_c * l_sattn_c* l_color* (
						kd_val* glm::abs(glm::dot(l_ld, isect_n))
						+ ks_val* glm::pow(glm::dvec3(glm::max(0.0, glm::dot(l_rd, ray_d))), 
												glm::dvec3(sh_val)));
		// }
	}

	// for(int k=0; k<3; k++)
	// 	i_out[k] = min(1.0, i_out[k]);

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

	return glm::dvec3(1, 1, 1);
}

glm::dvec3 TextureMap::getPixelAt(int x, int y) const
{
	// YOUR CODE HERE
	//
	// In order to add texture mapping support to the
	// raytracer, you need to implement this function.

	return glm::dvec3(1, 1, 1);
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

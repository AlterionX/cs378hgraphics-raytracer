#include "cubeMap.h"
#include "ray.h"
#include "../ui/TraceUI.h"
#include "../scene/material.h"
#include <glm/gtx/io.hpp>

#include <iostream>
#include <fstream>
extern TraceUI* traceUI;

#define SIN45 0.70710678118
glm::dvec3 CubeMap::getColor(ray r) const
{
	// YOUR CODE HERE
	// FIXME: Implement Cube Map here

	auto absRD = glm::abs(r.getDirection());
	auto rd = r.getDirection();
	auto xy = absRD[0] >= absRD[1];
	auto yz = absRD[1] >= absRD[2];
	auto zx = absRD[2] >= absRD[0];
	int map;
	double scale = 0.5;// / SIN45;
	glm::dvec2 d;
	// Ignore case in which rd component of interest is equivalent to 0
	if (xy && !zx) { // x direction
		scale /= absRD[0];
		d = glm::dvec2(rd[0] > 0 ? rd[2] : -rd[2], rd[1]);
		map = rd[0] > 0 ? 0 : 1;
	} else if (yz && !xy) { // y direction
		scale /= absRD[1];
		d = glm::dvec2(rd[0], rd[1] > 0 ? rd[2] : -rd[2]);
		map = rd[1] > 0 ? 2 : 3;
	} else if (zx && !yz) { // z direction
		scale /= absRD[2];
		d = glm::dvec2(rd[2] > 0 ? rd[0] : -rd[0], rd[1]);
		map = rd[2] > 0 ? 4 : 5;
	}

	d = d * scale + 0.5;
	//d = (d / SIN45 + 1.0) * 0.5;

	return this->tMap[map]->getMappedValue(d);
}

CubeMap::CubeMap()
{
}

CubeMap::~CubeMap()
{
}

void CubeMap::setNthMap(int n, TextureMap* m)
{
	if (m != tMap[n].get())
		tMap[n].reset(m);
}

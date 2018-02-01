#include "trimesh.h"
#include <assert.h>
#include <float.h>
#include <string.h>
#include <algorithm>
#include <cmath>
#include "../ui/TraceUI.h"
#include "../util.h"

#include <iostream>

extern TraceUI* traceUI;

using namespace std;

Trimesh::~Trimesh()
{
	for (auto m : materials)
		delete m;
	for (auto f : faces)
		delete f;
}

// must add vertices, normals, and materials IN ORDER
void Trimesh::addVertex(const glm::dvec3& v)
{
	vertices.emplace_back(v);
}

void Trimesh::addMaterial(Material* m)
{
	materials.emplace_back(m);
}

void Trimesh::addNormal(const glm::dvec3& n)
{
	normals.emplace_back(n);
}

// Returns false if the vertices a,b,c don't all exist
bool Trimesh::addFace(int a, int b, int c)
{
	int vcnt = vertices.size();

	if (a >= vcnt || b >= vcnt || c >= vcnt)
		return false;

	TrimeshFace* newFace = new TrimeshFace(
	        scene, new Material(*this->material), this, a, b, c);
	newFace->setTransform(this->transform);
	if (!newFace->degen)
		faces.push_back(newFace);
	else
		delete newFace;

	// Don't add faces to the scene's object list so we can cull by bounding
	// box
	return true;
}

// Check to make sure that if we have per-vertex materials or normals
// they are the right number.
const char* Trimesh::doubleCheck()
{
	if (!materials.empty() && materials.size() != vertices.size())
		return "Bad Trimesh: Wrong number of materials.";
	if (!normals.empty() && normals.size() != vertices.size())
		return "Bad Trimesh: Wrong number of normals.";

	return 0;
}

bool Trimesh::intersectLocal(ray& r, isect& i) const
{
	bool have_one = false;
	for (auto face : faces) {
		isect cur;
		if (face->intersectLocal(r, cur)) {
			if (!have_one || (cur.getT() < i.getT())) {
				i = cur;
				have_one = true;
			}
		}
	}
	if (!have_one)
		i.setT(1000.0);
	return have_one;
}

bool TrimeshFace::intersect(ray& r, isect& i) const
{
	return intersectLocal(r, i);
}

// Intersect ray r with the triangle abc.  If it hits returns true,
// and put the parameter in t and the barycentric coordinates of the
// intersection in u (alpha) and v (beta).
bool TrimeshFace::intersectLocal(ray& r, isect& i) const
{
	glm::dvec3 a = this->parent->vertices[(*this)[0]];
	glm::dvec3 b = this->parent->vertices[(*this)[1]];
	glm::dvec3 c = this->parent->vertices[(*this)[2]];

	//assume counter clockwise
	double t = glm::dot(this->normal, r.getDirection());
	if (ZCHK(t)) return false;
	t = glm::dot(r.getPosition() - a, this->normal) / t;
	if (BTTC(t)) return false;
	glm::dvec3 p_isect = r.at(t);
	for (int i = 0; i < 3; i++) {
		//On the edge means intersection
		glm::dvec3 prime = this->parent->vertices[(*this)[i]];
		glm::dvec3 edgev = this->parent->vertices[(*this)[(i+1)%3]];
		if (glm::dot(glm::cross(edgev - prime, p_isect - prime), this->normal) < 0) {
			return false;
		}
	}

	//Fill isect
	i.setObject(this);
	i.setT(t);
	i.setN(this->normal);
	glm::dvec3 faceAreaV = glm::cross(b-a, c-a);
	double faceArea = glm::sqrt(glm::dot(faceAreaV, faceAreaV));
	glm::dvec3 uV = glm::cross(b-a, p_isect-a);
	double u = glm::sqrt(glm::dot(uV, uV));
	glm::dvec3 vV = glm::cross(p_isect-a, c-a);
	double v = glm::sqrt(glm::dot(vV, vV));
	if (ZCHK(faceArea)) return false;
	i.setUVCoordinates(glm::dvec2(uV/faceArea, vV/faceArea));
	i.setBary(a);

	return true;
}

// Once all the verts and faces are loaded, per vertex normals can be
// generated by averaging the normals of the neighboring faces.
void Trimesh::generateNormals()
{
	int cnt = vertices.size();
	normals.resize(cnt);
	std::vector<int> numFaces(cnt, 0);

	for (auto face : faces) {
		glm::dvec3 faceNormal = face->getNormal();

		for (int i = 0; i < 3; ++i) {
			normals[(*face)[i]] += faceNormal;
			++numFaces[(*face)[i]];
		}
	}

	for (int i = 0; i < cnt; ++i) {
		if (numFaces[i])
			normals[i] /= numFaces[i];
	}

	vertNorms = true;
}

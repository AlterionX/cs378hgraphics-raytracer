#include "trimesh.h"
#include <assert.h>
#include <float.h>
#include <string.h>
#include <algorithm>
#include <cmath>
#include "../ui/TraceUI.h"
#include "../util.h"
#include "../scene/kdTree.h"

#include <iostream>

extern TraceUI* traceUI;

using namespace std;

Trimesh::~Trimesh() {
	for (auto m : materials)
		delete m;
	for (auto f : faces)
		delete f;
}

// must add vertices, normals, and materials IN ORDER
void Trimesh::addVertex(const glm::dvec3& v) {
	vertices.emplace_back(v);
}

void Trimesh::addMaterial(Material* m) {
	materials.emplace_back(m);
}

void Trimesh::addNormal(const glm::dvec3& n) {
	normals.emplace_back(n);
}

// Returns false if the vertices a,b,c don't all exist
bool Trimesh::addFace(int a, int b, int c) {
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

void Trimesh::conclude() {
	if (dirty) {
		// if (kdtree != NULL) {
		// 	delete kdtree;
		// }
		kdtree = new KdTree<TrimeshFace*>(&this->faces);
		dirty = true;
	}
}

bool Trimesh::intersectLocal(ray& r, isect& i) const {
	bool have_one = false;
	std::vector<int> potenlist;
	kdtree->intersectList(r, potenlist);
	for (auto face_i : potenlist) {
		auto &face = this->faces[face_i];
		isect cur;
		if (face->intersectLocal(r, cur)) {
			if (!have_one || (cur.getT() < i.getT())) {
				i = cur;
				have_one = true;
			}
		}
	}
	if (!have_one) i.setT(1000.0);
	return have_one;
}

void Trimesh::intersectLocalList(ray& r, std::vector<isect>& iv) const {
	std::vector<int> potenlist;
	kdtree->intersectList(r, potenlist);
	for (auto face_i : potenlist) {
		auto &face = this->faces[face_i];
		isect cur;
		if (face->intersectLocal(r, cur)) iv.push_back(cur);
	}
}

bool TrimeshFace::intersect(ray& r, isect& i) const {
	return intersectLocal(r, i);
}
void TrimeshFace::intersectLocalList(ray& r, std::vector<isect>& iv) const {
    isect i;
	intersectLocal(r, i);
    iv.push_back(i);
}

// Intersect ray r with the triangle abc.  If it hits returns true,
// and put the parameter in t and the barycentric coordinates of the
// intersection in u (alpha) and v (beta).
bool TrimeshFace::intersectLocal(ray& r, isect& i) const
{
	glm::dvec3 verts[3] = {
		this->parent->vertices[(*this)[0]],
		this->parent->vertices[(*this)[1]],
		this->parent->vertices[(*this)[2]]
	};

	//assume counter clockwise
	//plane intersection
	double t = glm::dot(this->normal, r.getDirection());
	//if (r.type() == ray::VISIBILITY && BTTC(t)) return false;
	if (ZCHK(t)) return false;
	t = glm::dot(verts[0] - r.getPosition(), this->normal) / t;
	if (BTTC(t)) return false;
	glm::dvec3 p_isect = r.at(t);

	//Within tri
	for (int i = 0; i < 3; i++) {
		//On the edge means intersection
		glm::dvec3 prime = verts[i];
		glm::dvec3 edgev = verts[(i+1)%3];
		if (BTTC(glm::dot(glm::cross(edgev - prime, p_isect - prime), this->normal))) {
			return false;
		}
	}

	//calc bary, see if can merge with upper section
	double faceArea = glm::dot(glm::cross(verts[1] - verts[0], verts[2] - verts[0]), this->normal);
	double baryU = glm::dot(glm::cross(verts[1] - p_isect, verts[2] - p_isect), this->normal);
	double baryV = glm::dot(glm::cross(verts[2] - p_isect, verts[0] - p_isect), this->normal);
	if (ZCHK(faceArea)) return false;
	glm::dvec3 bary = glm::dvec3(baryU/faceArea, baryV/faceArea, 0);
	bary[2] = 1 - bary[0] - bary[1];
	//Fill isect with known
	i.setObject(this);
	i.setT(t);
	i.setBary(bary);
	if (this->parent->materials.size()) {
		Material i_material = Material();
		i_material += bary[0] * (*(this->parent->materials[(*this)[0]]));
		i_material += bary[1] * (*(this->parent->materials[(*this)[1]]));
		i_material += bary[2] * (*(this->parent->materials[(*this)[2]]));
		i.setMaterial(i_material);
	}
	if (this->parent->normals.size() != 0) { //!this->parent->vertNorms) {
		i.setN(glm::normalize(
			glm::dmat3(
				this->parent->normals[(*this)[0]],
				this->parent->normals[(*this)[1]],
				this->parent->normals[(*this)[2]]
			) * bary
		));
	} else {
		i.setN(this->normal);
	}
	// Theoretically working uv code
	/*if (this->uvCoords.size()) {
		i.setUVCoordinates(
			glm::dmat3(
				this->parent->uvCoords[(*this)[0]],
				this->parent->uvCoords[(*this)[1]],
				this->parent->uvCoords[(*this)[2]]
			) * bary
		);
	}*/


	return true;
}

// Once all the verts and faces are loaded, per vertex normals can be
// generated by averaging the normals of the neighboring faces.
void Trimesh::generateNormals()
{
	int cnt = vertices.size();
	normals.resize(cnt);
	std::vector<int> numFaces(cnt, 0);

	// std::cout << "generate normal" << std::endl;

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
	// std::cout << "generate ended: " << std::endl;

	vertNorms = true;
	// std::cout << (vertNorms ? "bool set" : "hmm?") << std::endl;
}

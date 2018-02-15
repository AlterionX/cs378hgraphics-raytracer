#include <cmath>

#include "scene.h"
#include "light.h"
#include "kdTree.h"
#include "../ui/TraceUI.h"
#include <glm/gtx/extended_min_max.hpp>
#include <iostream>
#include <glm/gtx/io.hpp>

using namespace std;

bool Geometry::intersect(ray& r, isect& i) const {
	double tmin, tmax;
	if (hasBoundingBoxCapability() && !(bounds.intersect(r, tmin, tmax))) return false;
	// Transform the ray into the object's local coordinate space
	glm::dvec3 pos = transform->globalToLocalCoords(r.getPosition());
	glm::dvec3 dir = transform->globalToLocalCoords(r.getPosition() + r.getDirection()) - pos;
	double length = glm::length(dir);
	dir = glm::normalize(dir);
	// Backup World pos/dir, and switch to local pos/dir
	glm::dvec3 Wpos = r.getPosition();
	glm::dvec3 Wdir = r.getDirection();
	r.setPosition(pos);
	r.setDirection(dir);
	bool rtrn = false;
	if (intersectLocal(r, i))
	{
		// Transform the intersection point & normal returned back into global space.
		i.setN(transform->localToGlobalCoordsNormal(i.getN()));
		i.setT(i.getT()/length);
		rtrn = true;
	}
	// Restore World pos/dir
	r.setPosition(Wpos);
	r.setDirection(Wdir);
	return rtrn;
}

std::vector<isect> Geometry::intersectList(ray& r) const {
    std::vector<isect> buf;
	double tmin, tmax;
	if (hasBoundingBoxCapability() && !(bounds.intersect(r, tmin, tmax))) return buf;
	// Transform the ray into the object's local coordinate space
	glm::dvec3 pos = transform->globalToLocalCoords(r.getPosition());
	glm::dvec3 dir = transform->globalToLocalCoords(r.getPosition() + r.getDirection()) - pos;
	double length = glm::length(dir);
	dir = glm::normalize(dir);
	// Backup World pos/dir, and switch to local pos/dir
	glm::dvec3 Wpos = r.getPosition();
	glm::dvec3 Wdir = r.getDirection();
	r.setPosition(pos);
	r.setDirection(dir);
	intersectLocalList(r, buf);
    for (auto& i : buf) {
        // Transform the intersection point & normal returned back into global space.
        i.setN(transform->localToGlobalCoordsNormal(i.getN()));
        i.setT(i.getT()/length);
    }
	// Restore World pos/dir
	r.setPosition(Wpos);
	r.setDirection(Wdir);
	return buf;
}

bool Geometry::hasBoundingBoxCapability() const {
	// by default, primitives do not have to specify a bounding box.
	// If this method returns true for a primitive, then either the ComputeBoundingBox() or
    // the ComputeLocalBoundingBox() method must be implemented.

	// If no bounding box capability is supported for an object, that object will
	// be checked against every single ray drawn.  This should be avoided whenever possible,
	// but this possibility exists so that new primitives will not have to have bounding
	// boxes implemented for them.
	return false;
}

void Geometry::ComputeBoundingBox() {
    // take the object's local bounding box, transform all 8 points on it,
    // and use those to find a new bounding box.

    BoundingBox localBounds = ComputeLocalBoundingBox();

    glm::dvec3 min = localBounds.getMin();
    glm::dvec3 max = localBounds.getMax();

    glm::dvec4 v, newMax, newMin;

    v = transform->localToGlobalCoords( glm::dvec4(min[0], min[1], min[2], 1) );
    newMax = v;
    newMin = v;
    v = transform->localToGlobalCoords( glm::dvec4(max[0], min[1], min[2], 1) );
    newMax = glm::max(newMax, v);
    newMin = glm::min(newMin, v);
    v = transform->localToGlobalCoords( glm::dvec4(min[0], max[1], min[2], 1) );
    newMax = glm::max(newMax, v);
    newMin = glm::min(newMin, v);
    v = transform->localToGlobalCoords( glm::dvec4(max[0], max[1], min[2], 1) );
    newMax = glm::max(newMax, v);
    newMin = glm::min(newMin, v);
    v = transform->localToGlobalCoords( glm::dvec4(min[0], min[1], max[2], 1) );
    newMax = glm::max(newMax, v);
    newMin = glm::min(newMin, v);
    v = transform->localToGlobalCoords( glm::dvec4(max[0], min[1], max[2], 1) );
    newMax = glm::max(newMax, v);
    newMin = glm::min(newMin, v);
    v = transform->localToGlobalCoords( glm::dvec4(min[0], max[1], max[2], 1) );
    newMax = glm::max(newMax, v);
    newMin = glm::min(newMin, v);
    v = transform->localToGlobalCoords( glm::dvec4(max[0], max[1], max[2], 1) );
    newMax = glm::max(newMax, v);
    newMin = glm::min(newMin, v);

    bounds.setMax(glm::dvec3(newMax));
    bounds.setMin(glm::dvec3(newMin));
}

bool Geometry::check(const Geometry *ptr) const {
    return this->basisObj() == ptr->basisObj();
};
const Geometry* Geometry::basisObj() const {
    return this;
}

Scene::Scene(): kdtree(NULL), dirty(true)
{
}

Scene::~Scene()
{
}

void Scene::add(Geometry* obj) {
	obj->ComputeBoundingBox();
	sceneBounds.merge(obj->getBoundingBox());
	objects.emplace_back(obj);
	dirty = true;
}

void Scene::add(Light* light)
{
	lights.emplace_back(light);
}

void Scene::conclude() {
	if (dirty) {
		// if (kdtree != NULL) {
		// 	delete kdtree;
		// }
		kdtree = new KdTree<std::unique_ptr<Geometry> >(&this->objects);
		dirty = true;
	}
}

// Get any intersection with an object.  Return information about the
// intersection through the reference parameter.
bool Scene::intersect(ray& r, isect& i) const {
	double tmin = 0.0;
	double tmax = 0.0;
	bool have_one = false;

	std::vector<int> potenlist;
	kdtree->intersectList(r, potenlist);
	for(const auto& obj_i : potenlist) {
		auto &obj = this->objects[obj_i];
		isect cur;
		if( obj->intersect(r, cur) ) {
			if(!have_one || (cur.getT() < i.getT())) {
				i = cur;
				have_one = true;
			}
		}
	}
	if(!have_one)
		i.setT(1000.0);
	// if debugging,
	if (TraceUI::m_debug)
		intersectCache.push_back(std::make_pair(new ray(r), new isect(i)));
	return have_one;
}

std::vector<isect> Scene::intersectList(ray& r) const {
    std::vector<isect> iv;

	std::vector<int> potenlist;
	kdtree->intersectList(r, potenlist);
	for(const auto obj_i : potenlist) {
		auto &obj = this->objects[obj_i];
        auto niv = obj->intersectList(r);
    	// if debugging,
    	if (TraceUI::m_debug) for (const auto i : niv) {
        	intersectCache.push_back(std::make_pair(new ray(r), new isect(i)));
        }
		iv.insert(iv.end(), niv.begin(), niv.end());
	}
	return iv;
}

TextureMap* Scene::getTexture(string name) {
	auto itr = textureCache.find(name);
	if (itr == textureCache.end()) {
		textureCache[name].reset(new TextureMap(name));
		return textureCache[name].get();
	}
	return itr->second.get();
}

// Assumptions:
//	All objects intersected are closed/manifold geom
//	All objects are homogenous in composition (possible to do a reverse trace and interpolate, but complicates logic)
//	Material index of refraction and kt are a simple average
Material Scene::discoverMat(ray&& r) {
	auto iv = intersectList(r);
	std::sort(iv.begin(), iv.end(), [](const isect& a, const isect& b) { return a.getT() > b.getT(); });
	std::vector<isect> obj_stk = std::vector<isect>();
	for (isect iv_it : iv) {
		const bool leaving = glm::dot(iv_it.getN(), r.getDirection()) > 0;
		if (leaving) {
			obj_stk.push_back(iv_it);
		} else {
			for (auto it = obj_stk.begin(); it != obj_stk.end(); ++it) {
				if (isect::checkObj(iv_it, obj_stk.back())) {
					obj_stk.erase(it);
                    break;
				}
			}
		}
	}
	//TODO how does one combine all these materials?
	auto blank = Material(air);
	for (isect os_it : obj_stk) {
		if (!os_it.getMaterial().Trans()) return vantablack_mat; // Not sure if this is even possible
		blank += os_it.getMaterial();
	}
	blank = (1.0 / (double)obj_stk.size()) * blank;
    std::cout << obj_stk.size() << std::endl;
	return blank;
}

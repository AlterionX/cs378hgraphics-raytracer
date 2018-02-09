#include <algorithm>

#include "scene.h"
#include "material.h"
#include <glm/vec3.hpp>

#define LEAF_NUM 3

/*************************************/
/** KdTree ***************************/

KdTree::KdTree(const std::vector<T> &its) : item(NULL) {
	child[0] = child[1] = NULL;

	// get global bounding box
	BoundingBox b = its[0];
	for(const auto &ix: its)
		b.merge(ix->getBoundingBox());

	// create node + is_leave?
	_bound = b;
	if(its.size() <= LEAF_NUM) {
		_item = &its;
	}
	else { // its.size >= 2
		// determine spliting direction
		glm::dvec3 bmax = b.getMax();
		glm::dvec3 bmin = b.getMin();
		int mx_idx = 0;
		for(int i=1; i<3; i++)
			if(bmax[mx_idx] - bmin[mx_idx] < bmax[i] - bmin[i])
				mx_idx = i;

		// sort the list with middle point of bounding box
		std::sort(its.begin(), its.end(),
			[=](const T a, const T b) { 
				const BoundingBox box_a = a.getBoundingBox();
				const BoundingBox box_b = b.getBoundingBox();

				return (box_a.getMax()[mx_idx] + box_a.getMin()[mx_idx])
						< (box_b.getMax()[mx_idx] + box_b.getMin()[mx_idx])
	        });

		// split the list in half
		int mid = its.size()/2;
		std::vector<T> its_r;
		for(int i=its.size()-1; i>=mid; i--) 
			its_r.push_back(its.pop_back());

		// get children
		_child[0] = new KdTree(its);
		_child[1] = new KdTree(its_r);
	}
}

// for visualization
bool KdTree::intersect(ray& r, isect& i) {
	// bool have_one = false;
	// double tmin, tmax;
	// if(_bound.intersect(r, tmin, tmax)) {
	// 	for(int i=0; i<2; i++) {
	// 		isect cur;
	// 		if(_child[i].intersect(r, i)) {
	// 			if(!have_one || (cur.getT() < i.getT())) {
	// 				i = cur;
	// 				have_one = true;
	// 			}
	// 		}
	// 	}
	// }
	return false;
}

// for acceleration
void KdTree::intersectList(ray& r, std::vector<T> &hits) {
	bool have_one = false;
	double tmin, tmax;
	if(_bound.intersect(r, tmin, tmax)) {
		if(_item != NULL) {
			for(auto &it: _item)
				hits.push_back(it);
		}
		else {
			_child[0].intersectBBox(r, hits);
			_child[1].intersectBBox(r, hits);
		}
	}
}
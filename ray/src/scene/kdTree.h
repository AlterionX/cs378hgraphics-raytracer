#ifndef KDTREE_H__
#define KDTREE_H__

// ... really BVH ???

#define LEAF_NUM 3

#include <algorithm>
#include <vector>

#include "bbox.h"
#include "ray.h"
#include "scene.h"
#include "material.h"
#include <glm/vec3.hpp>

template <typename T> 
class KdTree {
public:
	// load and build 
	KdTree(const std::vector<T> &its) {
		child[0] = child[1] = NULL;

		// get global bounding box
		BoundingBox b = its[0]->getBoundingBox();
		for(const auto &ix: its)
			b.merge(ix->getBoundingBox());

		// create node + is_leave?
		bound = b;
		if(its.size() <= LEAF_NUM) {
			item = &its;
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
					const BoundingBox box_a = a->getBoundingBox();
					const BoundingBox box_b = b->getBoundingBox();

					return (box_a.getMax()[mx_idx] + box_a.getMin()[mx_idx])
							< (box_b.getMax()[mx_idx] + box_b.getMin()[mx_idx]);
		        });

			// split the list in half
			int mid = its.size()/2;
			std::vector<T> it_split[2];
			for(int i=0; i<its.size(); i++) 
				it_split[i < mid ? 0 : 1].push_back(its[i]);

			// get children
			for(int i=0; i<2; i++) 
				child[i] = new KdTree(it_split[i]);
		}
	}

	// for visualization
	bool intersect(ray& r, isect& i) {
		// bool have_one = false;
		// double tmin, tmax;
		// if(bound.intersect(r, tmin, tmax)) {
		// 	for(int i=0; i<2; i++) {
		// 		isect cur;
		// 		if(child[i].intersect(r, i)) {
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
	bool intersectList(ray& r, std::vector<T> &hits) {
		bool have_one = false;
		double tmin, tmax;
		if(bound.intersect(r, tmin, tmax)) {
			if(item != NULL) {
				for(auto &it: *item)
					hits.push_back(it);
			}
			else {
				child[0]->intersectList(r, hits);
				child[1]->intersectList(r, hits);
			}
		}
	}

private:
	KdTree* child[2];
	BoundingBox bound;
	const std::vector<T> *item;
};

#endif // KDTREE_H__

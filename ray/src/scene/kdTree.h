#ifndef KDTREE_H__
#define KDTREE_H__

// ... really BVH ???

#define LEAF_NUM 3

#include <algorithm>
#include <iostream>
#include <vector>
#include <glm/vec3.hpp>

#include "bbox.h"
#include "ray.h"
// #include "scene.h"
// #include "material.h"

template <typename T>
class KdTree {
public:

	// interface constructor
	KdTree(const std::vector<T> *its) : KdTree(its, getRange(0, its->size())) {
	}

	// build
	KdTree(const std::vector<T> *its, const std::vector<int> indexes) {
		child[0] = child[1] = NULL;
		// items = its;
		it_idxs.clear();

		if(indexes.size() == 0) return; // no item ?

		// get global bounding box
		BoundingBox b = (*its)[indexes[0]]->getBoundingBox();
		for(const auto &ix: indexes)
			b.merge((*its)[ix]->getBoundingBox());

		// create node + is_leave?
		bound = b;
		if(indexes.size() <= LEAF_NUM) {
			// std::cout << "reach leaves with #nodes: " << indexes.size() << std::endl;
			for(int x: indexes)
				it_idxs.push_back(x);
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
			auto idxs = getRange(0, indexes.size());
			std::sort(idxs.begin(), idxs.end(),
				[=](const int a, const int b) {
					const BoundingBox box_a = (*its)[indexes[a]]->getBoundingBox();
					const BoundingBox box_b = (*its)[indexes[b]]->getBoundingBox();

					return (box_a.getMax()[mx_idx] + box_a.getMin()[mx_idx])
							< (box_b.getMax()[mx_idx] + box_b.getMin()[mx_idx]);
		        });

			// split the list in half
			int mid = indexes.size()/2;
			std::vector<int> it_split[2];
			for(int i=0; i<indexes.size(); i++)
				it_split[i < mid ? 0 : 1].push_back(indexes[idxs[i]]);

			// get children
			// std::cout << "split: "; for(int i=0; i<2; i++) {for(auto x: it_split[i]) std::cout << x << " "; std::cout << " / ";} std::cout << std::endl;
			for(int i=0; i<2; i++)
				child[i] = new KdTree<T>(its, it_split[i]);
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
	// bool intersectList(ray& r, std::vector<T> &hits) {
	bool intersectList(ray& r, std::vector<int> &hits) {
		bool have_one = false;
		double tmin, tmax;
		if(bound.intersect(r, tmin, tmax)) {
			if(it_idxs.size() > 0) {
				for(int it: it_idxs) {
					hits.push_back(it);
					have_one = true;
				}
			}
			else {
				have_one |= child[0]->intersectList(r, hits);
				have_one |= child[1]->intersectList(r, hits);
			}
		}
		// std::cout << "return #hits: " << hits.size() << std::endl;
		return have_one;
	}

private:
	std::vector<int> getRange(int l, int r) {
		std::vector<int> idxs(r-l);
		std::iota(idxs.begin(), idxs.end(), l);
		return idxs;
	}

	KdTree* child[2];
	BoundingBox bound;
	// const std::vector<T> *items;
	std::vector<int> it_idxs;
};

#endif // KDTREE_H__

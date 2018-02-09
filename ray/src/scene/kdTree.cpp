#include <algorithm>

#include "scene.h"
#include "material.h"

KdTree::KdTree(std::vector<T*> &its) : item(&its) {

}

// for visualization
bool KdTree::intersect(ray& r, isect& i) {
	return false;
}

// for acceleration
bool KdTree::intersectBBox(ray& r, std::vector<T*> &hits) {
	return false;
}

KdNode* KdTree::build(std::vector<T*> &its) {
	return NULL;
	// if(its.size() <= 0) return NULL;
	// std::sort(its.begin(), its.end(), bboxComp);

	// int mid = its.size()/2;
	// for(int i=its.size()-1)
}
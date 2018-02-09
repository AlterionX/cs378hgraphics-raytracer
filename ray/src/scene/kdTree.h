#ifndef KDTREE_H__
#define KDTREE_H__

// ... really BVH ???

#include <vector>

#include "bbox.h"
#include "kdTree.h"
#include "ray.h"

class KdNode {
public:
	KdNode(BoundingBox b) : bound(b) {child[0] = NULL; child[1] = NULL;}

	KdNode* getNode(int i) {return child[i];}
	KdNode* setNode(int i, KdNode* node) {return child[i] = node;}

private:
	KdNode* child[2];
	BoundingBox bound;
};

template <typename T> 
class KdTree {
public:
	// load and build 
	KdTree(std::vector<T*> &its);

	// for visualization
	bool intersect(ray& r, isect& i);

	// for acceleration
	bool intersectList(ray& r, std::vector<T*> &hits);
private:
	KdNode* build(std::vector<T*> &its);

	KdNode root;
	std::vector<T*> *items; 
};

#endif // KDTREE_H__

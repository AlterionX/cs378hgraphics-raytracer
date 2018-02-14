#ifndef __BOX_H__
#define __BOX_H__

#include "../scene/scene.h"
#include <glm/vec3.hpp>
#include <glm/glm.hpp>

class Box : public MaterialSceneObject {
public:
	Box(Scene *scene, Material *mat)
		: MaterialSceneObject(scene, mat)
	{
	}

	virtual bool intersectLocal(ray& r, isect& i) const;
	virtual void intersectLocalList(ray& r, std::vector<isect>& iv) const;
	virtual bool hasBoundingBoxCapability() const { return true; }

	virtual glm::dvec3 computeNormal(int bestIndex, isect& i) const;

	virtual BoundingBox ComputeLocalBoundingBox()
	{
		BoundingBox localbounds;
		localbounds.setMax(glm::dvec3(0.5, 0.5, 0.5));
		localbounds.setMin(glm::dvec3(-0.5, -0.5, -0.5));
		return localbounds;
	}

protected:
	void glDrawLocal(int quality, bool actualMaterials, bool actualTextures) const;
};

#endif // __BOX_H__

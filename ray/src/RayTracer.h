#ifndef __RAYTRACER_H__
#define __RAYTRACER_H__

#define MAX_THREADS 32

// The main ray tracer.

#include <time.h>
#include <glm/vec3.hpp>
#include <queue>
#include <thread>
#include "scene/cubeMap.h"
#include "scene/ray.h"
#include "ui/TraceUI.h"

class Scene;
class Pixel {
public:
	Pixel(int i, int j, unsigned char* ptr) : ix(i), jy(j), value(ptr) {}

	int ix;
	int jy;
	unsigned char* value;
};

class RayTracer {
public:
	RayTracer();
	~RayTracer();

	glm::dvec3 tracePixel(int i, int j);
	glm::dvec3 traceRay(ray& r, double thresh, int depth,
        double& length, glm::dvec3* curr_m_kt = nullptr);

	glm::dvec3 getPixel(int i, int j);
	void setPixel(int i, int j, glm::dvec3 color);
	void getBuffer(unsigned char*& buf, int& w, int& h);
	double aspectRatio();

	void traceImage(int w, int h);
	int adaptaa(double x1, double x2, double y1, double y2, glm::dvec3 &val);
	int aaImage();

	bool checkRender();
	void waitRender();

	void traceSetup(int w, int h);

	bool loadScene(const char* fn);
	bool sceneLoaded() { return scene != 0; }

	void setReady(bool ready) { m_bBufferReady = ready; }
	bool isReady() const { return m_bBufferReady; }

	const Scene& getScene() { return *scene; }

	bool stopTrace;

private:
	glm::dvec3 trace(double x, double y);

	std::vector<unsigned char> buffer;
	int buffer_width, buffer_height;
	int bufferSize;
	unsigned int threads;
	int block_size;
	double thresh;
    TraceUI::AAMode aaMode;
	int samples;
	double aaThresh;
	std::unique_ptr<Scene> scene;

	bool m_bBufferReady;

};

#endif // __RAYTRACER_H__

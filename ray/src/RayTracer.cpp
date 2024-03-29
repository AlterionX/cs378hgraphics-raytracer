// The main ray tracer.

#pragma warning (disable: 4786)

#include "util.h"
#include "RayTracer.h"
#include "scene/light.h"
#include "scene/material.h"
#include "scene/ray.h"

#include "parser/Tokenizer.h"
#include "parser/Parser.h"

#include <cmath>
#include <algorithm>
#include <glm/glm.hpp>
#include <glm/gtx/io.hpp>
#include <string.h> // for memset

#include <iostream>
#include <fstream>

#define ANAGLYPH_DELTA 0.25

using namespace std;
extern TraceUI* traceUI;

// Use this variable to decide if you want to print out
// debugging messages.  Gets set in the "trace single ray" mode
// in TraceGLWindow, for example.
bool debugMode = false;

// Trace a top-level ray through pixel(i,j), i.e. normalized window coordinates (x,y),
// through the projection plane, and out into the scene.
glm::dvec3 RayTracer::trace(double x, double y) {
	// Clear out the ray cache in the scene for debugging purposes,
	if (TraceUI::m_debug)
		scene->intersectCache.clear();

	ray r(glm::dvec3(0, 0, 0), glm::dvec3(0, 0, 0), glm::dvec3(1, 1, 1), ray::VISIBILITY);
	scene->getCamera().rayThrough(x, y, r);

	double dummy;
	glm::dvec3 ret = traceRay(r, traceUI->getATermThresh(), traceUI->getDepth(), dummy);

    //Depth of field, avg
    if (traceUI->dofSwitch()) {
        double fd = glm::max(traceUI->getDofFD(), 1.0);
        ray fr = ray(r);
        scene->getCamera().rayThrough(0.5, 0.5, fr);
        glm::dvec3 fp_n = -r.getDirection();
        glm::dvec3 fp_pt = r.at(fd);

        //plane intersection
    	double t = glm::dot(fp_n, r.getDirection());
    	t = glm::dot(fp_pt - r.getPosition(), fp_n) / t;
    	glm::dvec3 dest = r.at(t);

        double sz = traceUI->getDofApSz() / 2; // diam / 2 = radius
        int divs = traceUI->getDofSubDiv();
        // sample divs + 1 from before
        double baseAngle = PI / divs;
        for (int i = 0; i < divs; i++) {
            double offsetAngle = PI / 2;

            offsetAngle = offsetAngle / divs + (i - 1) * baseAngle;
            glm::dvec3 offVec = (glm::cos(offsetAngle) * scene->getCamera().getV() + glm::sin(offsetAngle) * scene->getCamera().getU()) * sz;

            r.setPosition(scene->getCamera().getEye() + offVec);
            r.setDirection(glm::normalize(dest - r.getPosition()));

            ret += traceRay(r, traceUI->getATermThresh(), traceUI->getDepth(), dummy);
        }
        ret *= (1.0 / (divs + 1.0));
    }

	ret = glm::clamp(ret, 0.0, 1.0);
	return ret;
}

glm::dvec3 RayTracer::tracePixel(int i, int j)
{
	glm::dvec3 col(0, 0, 0);

	if (!sceneLoaded()) return col;

	double x = double(i) / (double(buffer_width) * ((traceUI->aaSwitch() && traceUI->getAAMode() != TraceUI::AAMode::ADAPTIVE) ? samples : 1));
	double y = double(j) / (double(buffer_height) * ((traceUI->aaSwitch() && traceUI->getAAMode() != TraceUI::AAMode::ADAPTIVE) ? samples : 1));

    col = trace(x, y);

    if(traceUI->anaglyph()) {
        glm::dvec3 cam_eye(scene->getCamera().getEye());
        scene->getCamera().setEye(cam_eye + glm::dvec3(ANAGLYPH_DELTA, 0.0, 0.0));
        glm::dvec3 col_red = trace(x, y);
        scene->getCamera().setEye(cam_eye);

        col[0] = col_red[0];
    }

	return col;
}

#define VERBOSE 0

// Do recursive ray tracing!  You'll want to insert a lot of code here
// (or places called from here) to handle reflection, refraction, etc etc.
glm::dvec3 RayTracer::traceRay(ray& r, double thresh, int depth, double& t, glm::dvec3* curr_m_kt)
{
	auto colorC = glm::dvec3(0.0, 0.0, 0.0);
#if VERBOSE
	std::cerr << "== current depth: " << depth << std::endl;
#endif

	isect i;
	if (depth >= 0 && scene->intersect(r, i)) {
		depth -= 1;

		// material in the collision surface negative n material
		const Material& m_in = i.getMaterial();
		// material in the collision surface n direction
		t = i.getT();

		colorC = m_in.shade(scene.get(), r, i);
		if (traceUI->aTermSwitch() && glm::dot(colorC, colorC) < thresh) return colorC;

		if (m_in.Recur() && depth > 0) {
			const Material& m_out = traceUI->overlappingObjects() ? scene->discoverMat(ray(r.at(i.getT() - RAY_EPSILON), r.getDirection(), glm::dvec3(1.0), ray::SHADOW)) : air;

			//Assuming hitting a face from the back is leaving and from the front is entering
			auto leaving = glm::dot(i.getN(), r.getDirection()) >= 0;
			auto curr_m = leaving ? m_in : m_out;
			auto next_m = leaving ? m_out : m_in;

			auto normal = (leaving ? -1.0 : 1.0) * i.getN();
			auto c = -1 * glm::dot(normal, r.getDirection());

			auto eta = next_m.Trans() ? curr_m.index(i) / next_m.index(i) : 0; //refractiveIndex
			auto radicand = 1 - eta * eta * (1 - c * c);
			auto tir = next_m.Trans() && radicand < 0;

			// reflection or total "internal" reflection
			if (m_in.Refl() || tir) {
				double reflT;
				auto reflDir = r.getDirection() + 2 * c * normal;
				auto reflStart = r.at(i.getT() - RAY_EPSILON);
				ray reflRay(reflStart, reflDir, glm::dvec3(1.0), ray::REFLECTION);
				auto reflCol = traceRay(reflRay, thresh, depth, reflT) * m_in.kr(i);
				reflCol *= glm::max(glm::min(glm::pow(curr_m.kt(i), glm::dvec3(reflT)), 1.0), 0.0);
				colorC += reflCol;
			}
			//refraction and is not total "internal" reflection
			if (next_m.Trans() && !tir) {
				double transT;
				ray transRay(r.at(i.getT() + RAY_EPSILON),
					eta * r.getDirection() + (eta * c - glm::sqrt(radicand)) * normal,
					glm::dvec3(1.0), ray::REFRACTION
				);

				auto transCol = traceRay(transRay, thresh, depth, transT);

				transCol *= glm::pow(next_m.kt(i), glm::dvec3(transT));
				colorC += transCol;
			}
		}
	}
	else { // No intersection
		if (traceUI->cubeMap()) colorC = traceUI->getCubeMap()->getColor(r);
	}
#if VERBOSE
	std::cerr << "== depth: " << depth + 1 << " done, returning: " << colorC << std::endl;
#endif
	return colorC;
}

RayTracer::RayTracer()
	: scene(nullptr), buffer(0), thresh(0), buffer_width(256), buffer_height(256), m_bBufferReady(false)
{}

RayTracer::~RayTracer()
{
}

void RayTracer::getBuffer(unsigned char *&buf, int &w, int &h)
{
	buf = buffer.data();
	h = buffer_height;
    w = buffer_width;
}

double RayTracer::aspectRatio()
{
	return sceneLoaded() ? scene->getCamera().getAspectRatio() : 1;
}

bool RayTracer::loadScene(const char* fn)
{
	ifstream ifs(fn);
	if (!ifs) {
		string msg("Error: couldn't read scene file ");
		msg.append(fn);
		traceUI->alert(msg);
		return false;
	}

	// Strip off filename, leaving only the path:
	string path(fn);
	if (path.find_last_of("\\/") == string::npos)
		path = ".";
	else
		path = path.substr(0, path.find_last_of("\\/"));

	// Call this with 'true' for debug output from the tokenizer
	Tokenizer tokenizer(ifs, false);
	Parser parser(tokenizer, path);
	try {
		scene.reset(parser.parseScene());
	}
	catch (SyntaxErrorException& pe) {
		traceUI->alert(pe.formattedMessage());
		return false;
	}
	catch (ParserException& pe) {
		string msg("Parser: fatal exception ");
		msg.append(pe.message());
		traceUI->alert(msg);
		return false;
	}
	catch (TextureMapException e) {
		string msg("Texture mapping exception: ");
		msg.append(e.message());
		traceUI->alert(msg);
		return false;
	}

	if (!sceneLoaded())
		return false;

	return true;
}

void RayTracer::traceSetup(int w, int h)
{
	if (buffer_width != w || buffer_height != h)
	{
		buffer_width = w;
		buffer_height = h;
		bufferSize = buffer_width * buffer_height * 3;
		buffer.resize(bufferSize);
	}
	std::fill(buffer.begin(), buffer.end(), 0);
	m_bBufferReady = true;

	/*
	 * Sync with TraceUI
	 */

	threads = traceUI->getThreads();
	block_size = traceUI->getBlockSize();
	thresh = traceUI->getATermThresh();
	aaMode = traceUI->getAAMode();
	samples = traceUI->getAASamples();
	aaThresh = traceUI->getAAThresh();

	// YOUR CODE HERE
	// FIXME: Additional initializations
}

/*
 * RayTracer::traceImage
 *
 *	Trace the image and store the pixel data in RayTracer::buffer.
 *
 *	Arguments:
 *		w:	width of the image buffer
 *		h:	height of the image buffer
 *
 */
void RayTracer::traceImage(int w, int h) {
	// Always call traceSetup before rendering anything.
	traceSetup(w, h);

    #pragma omp parallel num_threads(this->threads)
    #pragma omp for collapse(2)
	for (int i = 0; i < w; i++) {
		for (int j = 0; j < h; j++) {
            if (traceUI->aaSwitch()) {
                if (traceUI->getAAMode() != TraceUI::AAMode::ADAPTIVE) {
                    int modi = i * samples;
                    int modj = j * samples;
                    glm::dvec3 col = glm::dvec3(0.0);
                    for (double subi = 0; subi < samples; subi += 1) {
                        for (double subj = 0; subj < samples; subj += 1) {
                        	col += tracePixel(modi + subi, modj + subj);
                        }
                    }
                    col /= double(samples * samples);
                    setPixel(i, j, col);
                } else {
                    double x1 = double(i)/double(buffer_width);
    				double x2 = double(i+1)/double(buffer_width);
    				double y1 = double(j)/double(buffer_height);
    				double y2 = double(j+1)/double(buffer_height);

    				glm::dvec3 val;
    				adaptaa(x1, x2, y1, y2, val);
    				setPixel(i, j, val);
                }
            } else {
    			setPixel(i, j, tracePixel(i, j));
            }
		}
	}
}

#define AA_DIV 8 // uncomment line 352 to use
double adaa_eps = 0.0001;
int RayTracer::adaptaa(double x1, double x2, double y1, double y2, glm::dvec3 &val) {
	if (x1 + adaa_eps >= x2 || y1 + adaa_eps >= y2) return 0;

	// std::cout << "adaa: " << x1 << " " << x2 << " - " << y1 << " " << y2 << std::endl;

	double xs[] = { x1, (x1 + x2) / 2.0, x2 };
	double ys[] = { y1, (y1 + y2) / 2.0, y2 };
	double w = x2 - x1, h = y2 - y1;
	std::vector<glm::dvec3> s; // samples list
	glm::dvec3 mu(0.0, 0.0, 0.0), sd(0.0, 0.0, 0.0);

	// sample + calculate mean
    #pragma omp parallel num_threads(this->threads)
    #pragma omp for
	for (int i = 0; i < samples*samples; i++) {
		glm::dvec2 s_xy = hammersley(i, samples*samples);
		glm::dvec3 s_c = trace(s_xy[0] * w + x1, s_xy[1] * h + y1);
		s.push_back(s_c);
		mu += s_c;
	}
	mu *= (1.0 / (samples*samples));

	// calculate standard deviation
	for (const auto& s_c : s) {
		sd += glm::pow(glm::abs(s_c - mu), glm::dvec3(2.0));
	}
	sd *= (1.0 / (samples*samples - 1));

	// divide?
	if (glm::length(sd) > aaThresh) {
		// std::cout << "divide!" << std::endl;
		mu = glm::dvec3(0.0, 0.0, 0.0);
        #pragma omp parallel num_threads(this->threads)
        #pragma omp for collapse(2)
		for (int i = 0; i < 2; i++)
			for (int j = 0; j < 2; j++) {
				glm::dvec3 subval;
				adaptaa(xs[i], xs[i + 1], ys[j], ys[j + 1], subval);
				mu += subval;
			}
		mu *= (1.0 / 4.0);
	}

	// std::cout << "adaa: " << x1 << " " << x2 << " - " << y1 << " " << y2 << " " << val << std::endl;

	val = mu;
	return 0;
}

int RayTracer::aaImage() {
	int sampleCnt = 0;
	return sampleCnt;
}

bool RayTracer::checkRender() {
	// FIXME: Return true if tracing is done.
	return true;
}

void RayTracer::waitRender() {
	// YOUR CODE HERE
	// FIXME: Wait until the rendering process is done.
}


glm::dvec3 RayTracer::getPixel(int i, int j) {
	unsigned char *pixel = buffer.data() + (i + j * buffer_width) * 3;
	return glm::dvec3((double)pixel[0] / 255.0, (double)pixel[1] / 255.0, (double)pixel[2] / 255.0);
}

void RayTracer::setPixel(int i, int j, glm::dvec3 color) {
	unsigned char *pixel = buffer.data() + (i + j * buffer_width) * 3;

	pixel[0] = (int)(255.0 * color[0]);
	pixel[1] = (int)(255.0 * color[1]);
	pixel[2] = (int)(255.0 * color[2]);
}

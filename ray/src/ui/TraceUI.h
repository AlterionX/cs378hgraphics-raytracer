//
// rayUI.h
//
// The header file for the UI part
//

#ifndef __TraceUI_h__
#define __TraceUI_h__

#include <string>
#include <memory>
#define MAX_THREADS 32

using std::string;

class RayTracer;
class CubeMap;

class TraceUI {
public:
	TraceUI();
	virtual ~TraceUI();

	virtual int run() = 0;

	// Send an alert to the user in some manner
	virtual void alert(const string& msg) = 0;

	// setters
	virtual void setRayTracer(RayTracer* r) { raytracer = r; }
	void useCubeMap(bool b) { m_usingCubeMap = b; }

	// accessors and their variables:
public: // size
	int getSize() const { return m_nSize; }
protected:
	int m_nSize = 512;        // Size of the traced image
public:
	int getDepth() const { return m_nDepth; }
protected:
	int m_nDepth = 0;         // Max depth of recursion
public:
	int getBlockSize() const { return m_nBlockSize; }
protected:
	int m_nBlockSize = 4;     // Blocksize (square, even, power of 2 preferred)

public: // Adaptive termination (1 small bell)
	bool aTermSwitch() const { return m_aterm_thresh > 0.0; }
	double getATermThresh() const { return m_aterm_thresh; }
protected:
	double m_aterm_thresh = 0;

public: // Antialiasing modes and info (adaptive = 1 bell, jittered = 1 small bell)
	enum class AAMode { NONE = 0, SUPERSAMPLE = 1, ADAPTIVE = 2, JITTERED = 3 };
	bool aaSwitch() const { return (bool)m_aa_mode; }
	AAMode getAAMode() const { return m_aa_mode; }
	int getAASamples() const { return m_aa_samples; }
	double getAAThresh() const { return m_aa_thresh; }
protected:
	AAMode m_aa_mode = AAMode::NONE;
	int m_aa_samples = 3;
	double m_aa_thresh = 1.0;

public: // Acceleration structure and info
	enum class AccelStruct { NONE = 0, BVH = 1, KD_TREE = 2 };
	bool accelStructSwitch() const { return (bool)m_accel_struct; }
	int getMaxDepth() const { return m_nTreeDepth; }
	int getLeafSize() const { return m_nLeafSize; }
protected:
	AccelStruct m_accel_struct = AccelStruct::NONE;
	int m_nTreeDepth = 15;    // maximum kdTree depth
	int m_nLeafSize = 10;     // target number of objects per leaf

public:
	int getFilterWidth() const { return m_nFilterWidth; }
protected:
	int m_nFilterWidth = 1;   // width of cubemap filter

public:
	int getThreads() const { return m_threads; }

public:
	bool shadowSw() const { return m_shadows; }
	bool smShadSw() const { return m_smoothshade; }
	bool bkFaceSw() const { return m_backface; }
protected:
	bool m_shadows = true;       // compute shadows?
	bool m_smoothshade = true;   // turn on/off smoothshading?
	bool m_backface = true;      // cull backfaces?

public: // render with cubemap
	bool cubeMap() const { return m_usingCubeMap && cubemap; }
	CubeMap* getCubeMap() const { return cubemap.get(); }
	void setCubeMap(CubeMap* cm);
protected:
	bool m_usingCubeMap = false;
	std::unique_ptr<CubeMap> cubemap;

public: // overlapping objects (1 bell)
	bool overlappingObjects() { return m_overlappingObjects; }
protected:
	bool m_overlappingObjects = false;

public: // dof (2 bells)
	bool dofSwitch() { return m_dof_apsz == 0; }
	double getDofApSz() { return m_dof_apsz; }
	double getDofFD() { return m_dof_fd; }
	bool getDofJitter() { return m_dof_jitter; }
protected:
	double m_dof_apsz = 0;
	double m_dof_fd = 0;
	bool m_dof_jitter = false;

public:
	bool glossSwitch() { return m_gloss; }
protected:
	bool m_gloss = false;

public:
	// ray counter
	static void addRays(int number, int ctr)
	{
		if (ctr >= 0)
			rayCount[ctr] += number;
	}
	static void addRay(int ctr)
	{
		if (ctr >= 0)
			rayCount[ctr]++;
	}
	static int getCount(int ctr) { return ctr < 0 ? -1 : rayCount[ctr]; }
	static int getCount()
	{
		int total = 0;
		for (int i = 0; i < m_threads; i++)
			total += rayCount[i];
		return total;
	}
	static int resetCount(int ctr)
	{
		if (ctr < 0)
			return -1;
		int temp = rayCount[ctr];
		rayCount[ctr] = 0;
		return temp;
	}
	static int resetCount()
	{
		int total = 0;
		for (int i = 0; i < m_threads; i++) {
			total += rayCount[i];
			rayCount[i] = 0;
		}
		return total;
	}

	static int m_threads; // number of threads to run
	static bool m_debug;

	static bool matchCubemapFiles(const string& one_cubemap_file,
		string matched_fn[6],
		string& pdir);
protected:
	RayTracer * raytracer = nullptr;

	static int rayCount[MAX_THREADS]; // Ray counter

	// Determines whether or not to show debugging information
	// for individual rays.  Disabled by default for efficiency
	// reasons.
	bool m_displayDebuggingInfo = false;

	void loadFromJson(const char* file);
	void smartLoadCubemap(const string& file);
};

#endif

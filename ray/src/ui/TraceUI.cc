#include "TraceUI.h"
#include <sys/types.h>
#include <dirent.h>
#include "../scene/cubeMap.h"
#include "../scene/material.h"

/*
 * JSON for Modern C++
 * version 3.0.1
 * https://github.com/nlohmann/json
 */
#include "json.hpp"
using Json = nlohmann::json;
#include <iostream>
#include <fstream>

namespace {
template <typename T>
void load(Json& j, const string& field, T& target)
{
	// Do not change its value by default
	target = j.value(field, target);
}

} // anonymous namespace

TraceUI::TraceUI()
{
	for (unsigned int i = 0; i < MAX_THREADS; i++)
		rayCount[i] = 0;
}

TraceUI::~TraceUI()
{
}

void TraceUI::setCubeMap(CubeMap* cm)
{
	cubemap.reset(cm);
}

void TraceUI::loadFromJson(const char* file)
{
	std::ifstream fin(file);
	Json json;
	fin >> json;

	load(json, "threads", m_threads);
	load(json, "size", m_nSize);
	load(json, "recursion_depth", m_nDepth);

    int aterm_thresh = m_aterm_thresh * 1000;
	load(json, "threshold", aterm_thresh);
    m_aterm_thresh = aterm_thresh / 1000.0;

	load(json, "blocksize", m_nBlockSize);
	load(json, "tree_depth", m_nTreeDepth);
	load(json, "leaf_size", m_nLeafSize);
	load(json, "filter_width", m_nFilterWidth);

    bool aa;
    try {
        aa = json.at("anti_alias");
        m_aa_mode = (AAMode) aa;
    } catch (Json::type_error& e) {
    } catch (Json::out_of_range& e) {}
	load(json, "supersamples", m_aa_thresh);
    int aa_thresh = m_aa_thresh * 100;
	load(json, "aa_threshold", aa_thresh);
    m_aa_thresh = aa_thresh / 100.0;

    try {
        bool accel_struct = accelStructSwitch();
        accel_struct = json.at("kdtree");
        m_aa_mode = (AAMode) aa;
    } catch (Json::type_error& e) {
    } catch (Json::out_of_range& e) {}

	load(json, "shadows", m_shadows);
	load(json, "smoothshade", m_smoothshade);
	load(json, "backface_culling", m_backface);

	//TODO make settings json capable
}

namespace {
const char *matcher[][2] = {
	{"pos", "x"},
	{"neg", "x"},
	{"pos", "y"},
	{"neg", "y"},
	{"pos", "z"},
	{"neg", "z"},
};
}

bool TraceUI::matchCubemapFiles(const string& one_cubemap_file,
				string matched_fn[6],
				string& pdir)
{
	DIR *dp;
	struct dirent *ep;
	std::string fN = one_cubemap_file;
	pdir = fN.substr(0, fN.find_last_of("/"));
	dp = opendir(pdir.data());

	if (dp == NULL) {
		std::cerr << "Couldn't open the directory " << pdir << std::endl;
		return false;
	}
	for (int i = 0; i < 6; i++)
		matched_fn[i].clear();
	int matched = 0;
	while ((ep = readdir(dp))) {
		std::string fn(ep->d_name);
		for (int i = 0; i < 6; i++) {
			auto pos0 = fn.find_first_of(matcher[i][0]);
			if (pos0 == std::string::npos)
				continue;
			auto pos1 =  fn.find_first_of(matcher[i][1], pos0);
			if (pos1 == std::string::npos)
				continue;
			if (!matched_fn[i].empty()) {
				(void)closedir(dp);
				std::cerr << matcher[i][0] << matcher[i][1]
					  << " matches " << matched_fn[i]
					  << " and " << fn
					  << ", stop smartload to avoid confliction"
					  << std::endl;
				return false;
			}
			matched_fn[i] = fn;
			matched++;
			break;
		}
		if (matched == 6)
			break;
	}
	(void)closedir(dp);
	if (matched != 6) {
		std::cerr << "Cannot locate all six cubemap files"
			  << std::endl;
		return false;
	}
	return true;
}

void TraceUI::smartLoadCubemap(const string& file)
{
	string matched_fn[6];
	string pdir;
	bool matched = matchCubemapFiles(file, matched_fn, pdir);
	if (matched) {
		if (!getCubeMap()) {
			setCubeMap(new CubeMap());
		}
		try {
			for (int i = 0; i < 6; i++)
				cubemap->setNthMap(i, new TextureMap(pdir + "/" + matched_fn[i]));
		} catch (TextureMapException &xcpt) {
			cubemap.reset();
			std::cerr << xcpt.message() << std::endl;
			return ;
		}
		useCubeMap(true);
	}
}

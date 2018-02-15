#include <stdarg.h>
#include <time.h>
#include <iostream>
#ifndef __WIN32
#include <unistd.h>
#else
extern char* optarg = NULL;
extern int optind, opterr, optopt;
extern int getopt(int argc, char* const* argv, const char* optstring);
#endif

#include <assert.h>

#include "../fileio/images.h"
#include "CommandLineUI.h"

#include "../RayTracer.h"

using namespace std;

// The command line UI simply parses out all the arguments off
// the command line and stores them locally.
CommandLineUI::CommandLineUI(int argc, char** argv) : TraceUI()
{
	int i;
	progName = argv[0];
	const char* jsonfile = nullptr;
	string cubemap_file;
	char prev = 0;
	while ((i = getopt(argc, argv, "tr:w:hj:c:O:A:B:C:D")) != EOF) {
		switch (i) {
			case 'r':
				m_nDepth = atoi(optarg);
				break;
			case 'w':
				m_nSize = atoi(optarg);
				break;
			case 'j':
				jsonfile = optarg;
				break;
			case 'c':
				cubemap_file = optarg;
				break;
			case 'O':
				prev = *optarg;
				switch(prev) {
					case 'a':
						m_aa_mode = AAMode::ADAPTIVE;
						break;
					case 'j':
						m_aa_mode = AAMode::JITTERED;
						break;
					case 'r':
						m_aa_mode = AAMode::SUPERSAMPLE;
						break;
					case 'o':
						m_overlappingObjects = true;
						break;
                    case 'd':
                        m_dof = true;
                        break;
					case 'g':
						m_anaglyph = true;
						break;
					case 'c':
						break;
                    case 's':
                        break;
					default:
						std::cerr << "Invalid argument for O: '" << i << "'."
						          << std::endl;
						usage();
						exit(1);
				}
				break;
			case 'A':
				switch(prev) {
					case 'a':
					case 'j':
					case 'r':
						m_aa_samples = atoi(optarg);
						break;
					case 'c':
						m_aterm_thresh = atof(optarg);
						break;
                    case 'd':
                        m_dof_fd = atof(optarg);
                        break;
                    case 's':
                        m_ss_res = atoi(optarg);
                        break;
					default:
						std::cerr << "Invalid argument for A, with prequel " << prev << ": '" << i << "'." << std::endl;
						usage();
						exit(1);
				}
				break;
			case 'B':
				switch(prev) {
					case 'a':
						m_aa_thresh = atof(optarg);
						break;
                    case 'd':
                        m_dof_div = atoi(optarg);
                        break;
					default:
						std::cerr << "Invalid argument for B, with prequel " << ((unsigned int) prev) << ": '" << i << "'." << std::endl;
						usage();
						exit(1);
				}
				break;
			case 'C':
				switch(prev) {
                    case 'd':
                        m_dof_apsz = atof(optarg);
                        break;
					default:
						std::cerr << "Invalid argument for C, with prequel " << ((unsigned int) prev) << ": '" << i << "'." << std::endl;
						usage();
						exit(1);
				}
				break;
			case 'h':
				usage();
				exit(1);
			default:
				// Oops; unknown argument
				std::cerr << "Invalid argument: '" << i << "'." << std::endl;
				usage();
				exit(1);
		}
	}
	if (jsonfile) {
		loadFromJson(jsonfile);
	}
	if (!cubemap_file.empty()) {
		smartLoadCubemap(cubemap_file);
	}

	if (optind >= argc - 1) {
		std::cerr << "no input and/or output name." << std::endl;
		exit(1);
	}

	rayName = argv[optind];
	imgName = argv[optind + 1];
}

int CommandLineUI::run()
{
	assert(raytracer != 0);
	raytracer->loadScene(rayName);

	if (raytracer->sceneLoaded()) {
		int width = m_nSize;
		int height = (int)(width / raytracer->aspectRatio() + 0.5);

		raytracer->traceSetup(width, height);

		clock_t start, end;
		start = clock();

		raytracer->traceImage(width, height);
		raytracer->waitRender();
		if (aaSwitch()) {
			raytracer->aaImage();
			raytracer->waitRender();
		}

		end = clock();

		// save image
		unsigned char* buf;

		raytracer->getBuffer(buf, width, height);

		if (buf)
			writeImage(imgName, width, height, buf);

		double t = (double)(end - start) / CLOCKS_PER_SEC;
		// int totalRays = TraceUI::resetCount();
		// std::cout << "total time = " << t << " seconds,
		// rays traced = " << totalRays << std::endl;
		return 0;
	} else {
		std::cerr << "Unable to load ray file '" << rayName << "'"
		          << std::endl;
		return (1);
	}
}

void CommandLineUI::alert(const string& msg)
{
	std::cerr << msg << std::endl;
}

void CommandLineUI::usage()
{
	using namespace std;
	cerr << "usage: " << progName
	     << " [options] [input.ray output.png]" << endl
	     << "  -r <#>      set recursion level (default " << m_nDepth << ")" << endl
	     << "  -w <#>      set output image width (default " << m_nSize << ")" << endl
	     << "  -j <FILE>   set parameters from JSON file" << endl
	     << "  -c <FILE>   one Cubemap file, the remaining files will be detected automatically" << endl
		 << "  -O <char>   additional options as follows:" << endl
		 << "                  a     turns on adaptive AA" << endl
		 << "                  j     turns on jittered AA" << endl
		 << "                  r     turns on regular AA" << endl
		 << "                  o     turns on overlapping objects" << endl
         << "                  d     turns on dof" << endl
         << "                  g     turns on anaglyph" << endl
		 << "                  c     turns on adaptive termination, must pass -A for this to have meaning." << endl
         << "                  s     modifies stochastic lighting raycount, must pass -A for this to have meaning." << endl
		 << "  -A <#>      extra information for the most recent option passed into -O, as listed:" << endl
		 << "                  ajr   sets antialiasing limit (supersampling value for jittered and regular, max depth for adaptive)." << endl
		 << "                  c     sets the threshold for adaptive termination." << endl
         << "                  s     set the number of rays to sample." << endl
         << "                  d     set the focal distance (double)." << endl
		 << "  -B <?>      even more extra information for the most recent option passed into -O, as listed:" << endl
		 << "                  a     sets the threshold value for adaptive anitaliazing (double)." << endl
         << "                  d     sets the number of samples to take for dof. (double)." << endl
		 << "  -C <?>      even more extra information for the most recent option passed into -O, as listed:" << endl
		 << "                  d     sets the aperture size (double)." << endl;
}

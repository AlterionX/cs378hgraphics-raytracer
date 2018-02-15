#ifndef __UTIL_H__
#define __UTIL_H__

#include <glm/glm.hpp>

//Zero check
#define ZCHK(fl) fl < RAY_EPSILON/32 && fl > -RAY_EPSILON/32
//Backwards time travel check
#define BTTC(fl) fl < RAY_EPSILON/32

#define PI 3.1415926535897932384626433832795028841971

glm::dvec2 hammersley(int n, int N);

#endif

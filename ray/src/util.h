//TODO add in RAY_EPSILON

#ifndef __UTIL_H__
#define __UTIL_H__

//Zero check
#define ZCHK(fl) fl < RAY_EPSILON/32 && fl > -RAY_EPSILON/32
//Backwards time travel check
#define BTTC(fl) fl < RAY_EPSILON/32

#define PI 3.1415926535897932384626433832795028841971

glm::dvec2 hammersley(int n, int N) {
	double mul = 0.5, result = 0.0;
	while (n > 0) {
		result += (n % 2) ? mul : 0;
		n /= 2;
		mul /= 2.0;
	}
	return glm::dvec2(result, ((double)n) / N);
}

#endif

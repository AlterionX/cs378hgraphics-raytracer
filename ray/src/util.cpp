#include "util.h"

glm::dvec2 hammersley(int n, int N) {
   double mul = 0.5, result = 0.0;
   while (n > 0) {
       result += (n % 2) ? mul : 0;
       n /= 2;
       mul /= 2.0;
   }
   return glm::dvec2(result, ((double)n) / N);
}

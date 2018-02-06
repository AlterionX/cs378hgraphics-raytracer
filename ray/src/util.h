//TODO add in RAY_EPSILON

//Zero check
#define ZCHK(fl) fl < RAY_EPSILON && fl > -RAY_EPSILON
//Backwards time travel check
#define BTTC(fl) fl < -RAY_EPSILON

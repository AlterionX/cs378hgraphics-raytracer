//TODO add in RAY_EPSILON

//Zero check
#define ZCHK(fl) fl < RAY_EPSILON/32 && fl > -RAY_EPSILON/32
//Backwards time travel check
#define BTTC(fl) fl < RAY_EPSILON/32

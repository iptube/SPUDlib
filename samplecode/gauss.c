#include "gauss.h"
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>

// http://en.wikipedia.org/wiki/Box%E2%80%93Muller_transform
double gauss(double mean, double stddev)
{
    const double two_pi = 2.0 * M_PI;
    static double z0, z1;
	static bool generate = false;
	generate = !generate;

	if (!generate)
	   return z1 * stddev + mean;

	double u1, u2;
	do {
	   u1 = rand() * (1.0 / RAND_MAX);
	   u2 = rand() * (1.0 / RAND_MAX);
	} while ( u1 <= DBL_MIN );

	z0 = sqrt(-2.0 * log(u1)) * cos(two_pi * u2);
	z1 = sqrt(-2.0 * log(u1)) * sin(two_pi * u2);
	return z0 * stddev + mean;
}

#ifndef GAUSSIAN_FUNC
#define GAUSSIAN_FUNC

#include <cmath>

float gaussianFunction(float x, float y, float sigma)
{
	return exp(-1*(pow(x, 2)+pow(y, 2)) / (2*pow(sigma, 2))) / (2*M_PI*pow(sigma, 2));
}

#endif	// GAUSSIAN_FUNC

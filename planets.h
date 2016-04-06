#ifndef _PLANETS_H
#define _PLANETS_H

enum Planets {
	SUN = 0,
	EARTH,
	MARS,
	NUM_OF_PLANETS
};

struct PlanetInfo {
	float revRadius_ratio;    // The revolution radius ratio to the earth.
	float revPeriod_ratio;    // The revolution period ratio to the earth.
	float rotPeriod_ratio;    // The rotation period ratio to the earth.
	float planetRadius_ratio; // The planet radius ratio to the earth.
};

#endif // _PLANETS_H

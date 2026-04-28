#include "Math.hpp"

#undef INFINITY
#include <math.h>

float32 SquareRoot(float32 x)
{
	return sqrtf(x);
}

float32 Sine(float32 x)
{
	return sinf(x);
}

float32 Cosine(float32 x)
{
	return cosf(x);
}

float32 Tangent(float32 x)
{
	return tanf(x);
}

#pragma once

#include "Base.hpp"
#include "NoCopy.hpp"

struct PCGRandomContext
{
	uint32 State;
};

inline uint32 RandomUInt32PCG(PCGRandomContext* context)
{
	const uint32 state = context->State;
	context->State = context->State * 747796405U + 2891336453U;
	const uint32 word = ((state >> ((state >> 28U) + 4U)) ^ state) * 277803737U;
	return (word >> 22U) ^ word;
}

inline PCGRandomContext SeedRandomPCG(uint32 initialState)
{
	PCGRandomContext context =
	{
		.State = initialState,
	};
	RandomUInt32PCG(&context);
	return context;
}

class RandomContext : public NoCopy
{
public:
	explicit RandomContext(uint32 seed)
		: Context(SeedRandomPCG(seed))
	{
	}

	uint32 UInt32()
	{
		return RandomUInt32PCG(&Context);
	}

	float Float01()
	{
		return static_cast<float>(UInt32()) / static_cast<float>(UINT32_MAX);
	}

private:
	PCGRandomContext Context;
};

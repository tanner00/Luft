#pragma once

#include "Base.hpp"

struct PcgRandomContext
{
	uint32 State;
};

inline uint32 RandomPcg(PcgRandomContext* context)
{
	const uint32 state = context->State;
	context->State = context->State * 747796405U + 2891336453U;
	const uint32 word = ((state >> ((state >> 28U) + 4U)) ^ state) * 277803737U;
	return (word >> 22U) ^ word;
}

inline PcgRandomContext SeedRandomPcg(uint32 initialState)
{
	PcgRandomContext context =
	{
		.State = initialState,
	};
	RandomPcg(&context);
	return context;
}

class RandomContext
{
public:
	explicit RandomContext(uint32 seed)
		: Context(SeedRandomPcg(seed))
	{
	}

	uint32 Uint32()
	{
		return RandomPcg(&Context);
	}

	float Float01()
	{
		return static_cast<float>(Uint32()) / static_cast<float>(0xFFFFFFFF);
	}

private:
	PcgRandomContext Context;
};

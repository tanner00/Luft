#pragma once

#include "Base.hpp"
#include "NoCopy.hpp"

struct PcgRandomContext
{
	uint32 State;
};

inline uint32 RandomUint32Pcg(PcgRandomContext* context)
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
	RandomUint32Pcg(&context);
	return context;
}

class RandomContext : public NoCopy
{
public:
	explicit RandomContext(uint32 seed)
		: Context(SeedRandomPcg(seed))
	{
	}

	uint32 Uint32()
	{
		return RandomUint32Pcg(&Context);
	}

	float Float01()
	{
		return static_cast<float>(Uint32()) / static_cast<float>(UINT32_MAX);
	}

private:
	PcgRandomContext Context;
};

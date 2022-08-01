#include "general_common.h"

// Uses primes
uint64_t GetChunkedSeed(uint64_t baseSeed, int64_t chunkPosX, int64_t chunkPosY, int64_t chunkPosZ)
{
	return baseSeed + chunkPosX * UINT64_C(2521008887) + chunkPosY * UINT64_C(20996011) + chunkPosZ * UINT64_C(3010349);
}

// Tempering used in Mersenne Twister
uint32_t TemperU32(uint32_t x)
{
	x ^= x >> 11;
	x ^= x << 7 & 0x9D2C5680;
	x ^= x << 15 & 0xEFC60000;
	x ^= x >> 18;

	return x;
}

uint32_t GenerateU32(uint64_t* seed)
{
	*seed = 6364136223846793005ULL * *seed + 1;

	return TemperU32(*seed >> 32);
}

uint64_t GenerateU64(uint64_t* seed)
{
	return static_cast<uint64_t>(GenerateU32(seed)) | (static_cast<uint64_t>(GenerateU32(seed)) << 32);
}

float GenerateF32(float x0, float x1, uint32_t possibilities, uint64_t* seed)
{
	uint32_t i = GenerateU32(seed) % possibilities;

	return x0 + i * (x1 - x0) / (possibilities - UINT32_C(1));
}

double GenerateF64(double x0, double x1, uint32_t possibilities, uint64_t* seed)
{
	uint32_t i = GenerateU32(seed) % possibilities;

	return x0 + i * (x1 - x0) / (possibilities - UINT32_C(1));
}

#include <iostream>
void GenerateF32Normal(float* output, size_t outputLength, uint64_t* seed)
{
	for (size_t i = 0; i < (outputLength - 1) / 2 + 1; ++i)
	{
		// Add a little to min avoid negative infinity and stay at 1.0 max to avoid nan.
		float U1 = GenerateF32(0.0001F, 1.0F, UINT32_C(1000000), seed);
		float U2 = GenerateF32(0.0F, 1.0F, UINT32_C(1000000), seed);

		float c = sqrtf(-2.0F * logf(U1));
		output[i * 2] = c * cosf(2.0F * PI * U2);
		if (i * 2 + 1 < outputLength)
			output[i * 2 + 1] = c * sinf(2.0F * PI * U2);
	}
}

// For some reason, this gives an average of ~0.055
void GenerateF64Normal(double* output, size_t outputLength, uint64_t* seed)
{
	for (size_t i = 0; i < (outputLength - 1) / 2 + 1; ++i)
	{
		// Add a little to avoid negative infinity
		double U1 = GenerateF64(0.0000001, 1.0, UINT32_C(1000000000), seed);
		double U2 = GenerateF64(0.0, 1.0, UINT32_C(1000000000), seed);

		double c = sqrt(-2.0 * log(U1));
		output[i * 2] = c * cos(2.0 * D_PI * U2);
		if (i * 2 + 1 < outputLength)
			output[i * 2 + 1] = c * sin(2.0 * D_PI * U2);
	}
}

/*std::list<Suspendable*> Suspendable::allSuspendable;

SuspendedState::~SuspendedState()
{
	for (auto& p : suspendedData)
		p.first->Deallocate(p.second);
}

Suspendable::Suspendable()
{
	allSuspendable.push_back(this);
}

Suspendable::~Suspendable()
{
	allSuspendable.remove(this);
}

void Suspendable::SuspendAll(SuspendedState& state)
{
	for (Suspendable* suspendable : allSuspendable)
	{
		if (state.suspendedData.find(suspendable) != state.suspendedData.end())
			continue;
		state.suspendedData[suspendable] = suspendable->Suspend();
	}
}

void Suspendable::Resume(SuspendedState& state)
{
	for (auto& p : state.suspendedData)
		p.first->Resume(p.second);
	state.suspendedData.clear();
}

void Suspendable::Swap(SuspendedState& state)
{
	for (auto& p : state.suspendedData)
		p.first->Swap(p.second);
}*/
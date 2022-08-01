#pragma once

// For GLU etc
#define _WIN32

#include <list>
#include <vector>
#include <unordered_map>
#include <memory>
#include <stdint.h>
#include <math.h>

#include "debug.h"

const static float PI = 3.14159265358979323846F;
const static double D_PI = 3.14159265358979323846;

#define TUNDRA_USE_DOUBLE_PRECISION

#ifdef TUNDRA_USE_DOUBLE_PRECISION
typedef double real;
#define REAL_ZERO 0.0
#define REAL_ONE 1.0
#else
typedef float real;
#define REAL_ZERO 0.0F
#define REAL_ONE 1.0F
#endif

#define SMALL_NUM 0.001
#define SMALL_NUMF 0.001F

#define no_copy_constructor(name) public: name(name&) = delete;
#define no_assignment_operator(name) public: void operator=(name& other) = delete;
#define no_transfer_functions(name) no_copy_constructor(name)\
                                    no_assignment_operator(name)
#define singleton(name) private:\
							name();\
						public:\
							static name& GetInstance()\
							{\
								static name instance;\
								\
								return instance;\
							}\
							\
							no_transfer_functions(name)

#define can_copy(name) name* rtc_GetCopy() const override\
                       {\
					       return new name(*this);\
                       }

#define cant_copy(name) no_copy_constructor(name)\
						name* rtc_GetCopy() const override\
						{\
							return nullptr;\
						}
						
#define make_vec(type, ...) std::move(std::vector<type>({ __VA_ARGS__ }))

// Uses primes
uint64_t GetChunkedSeed(uint64_t baseSeed, int64_t chunkPosX, int64_t chunkPosY, int64_t chunkPosZ);

// Tempering used in Mersenne Twister
uint32_t TemperU32(uint32_t x);

uint32_t GenerateU32(uint64_t* seed);

uint64_t GenerateU64(uint64_t* seed);

/*
Returns an number between x0 and x1 (inclusive) with the number of possibilities specified (may not be realistic if small due to float limits).
possibilities must be larger or equal to 2;
*/
float GenerateF32(float x0, float x1, uint32_t possibilities, uint64_t* seed);

double GenerateF64(double x0, double x1, uint32_t possibilities, uint64_t* seed);

/*
Fills an array with unit normally distritubted values (32 uses 1e6 possibilities and 64 uses 1e9).
Multiply the output by the standard deviation and add the mean to get values for a specific distribution.
Uses the Box-Muller Transform.
*/
void GenerateF32Normal(float* output, size_t outputLength, uint64_t* seed);
void GenerateF64Normal(double* output, size_t outputLength, uint64_t* seed);

template<class T>
void RemoveVectorComponent(std::vector<T>& list, const T& component)
{
	for (size_t i = 0; i < list.size(); ++i)
		if (list[i] == component)
		{
			list[i] = list[list.size() - 1];
			list.pop_back();
			break;
		}
}

template<class T>
class RequestList
{
private:
	struct Request
	{
		T val;
		bool isAdd;

		Request(const T& val, bool isAdd)
		{
			this->val = val;
			this->isAdd = isAdd;
		}
	};

	std::list<T> list;
	std::list<Request> requests;
public:
	void Add(const T& val)
	{
		requests.push_back(Request(val, true));
	}

	void Remove(const T& val)
	{
		requests.push_back(Request(val, false));
	}

	inline auto begin()
	{
		return list.begin();
	}

	inline const auto begin() const
	{
		return list.begin();
	}

	inline auto end()
	{
		return list.end();
	}

	inline const auto end() const
	{
		return list.end();
	}

	void Clear()
	{
		list.clear();
		requests.clear();
	}

	void ProcessRequests()
	{
		for (const Request& r : requests)
		{
			if (r.isAdd)
				list.push_back(r.val);
			else
				list.remove(r.val);
		}
		requests.clear();
	}
};

// TODO: Currently never deallocates any real stored memory before the entire object is destroyed. Is this a good idea? Completely empty non-head sections should definitely be removed.
// TODO: Currently does nothing when it runs out of chain space

// WARNING: Because of the nature of this class, the destructors will never be called for objects that were not destructed with free()
// TODO: Isn't very good for non-virtual objects. Having the nextObj* likely doubles the memory (with padding.) Is nextObj* even needed for this? This is meant for storage, and not constant looping so it is fine if it is slightly slow when clearing (can just use tail map for next obj.) This would make allocation and deletion faster, and make the memory more efficent.
// TODO: If sizeof(Section) == alignof(Section) (or even maybe sizeof(Slice) * 64 ==  alignof(Section)) then reinterpret_cast<Section*>(obj) = section
// TODO B = 8 or alig

namespace CT
{
	constexpr size_t CoverageLog2(size_t n)
	{
		float current = static_cast<float>(n);
		size_t result = 1;
		while (current > 2.0F)
		{
			current *= 0.5F;
			++result;
		}

		return result;
	}
}

template<class T>
class MemoryPool
{
	static_assert(UINT64_C(0b1) << CT::CoverageLog2(sizeof(T) * UINT64_C(64)) <= 8192, "T is too large to be poooled");
private:
	struct alignas(UINT64_C(0b1) << CT::CoverageLog2(sizeof(T) * UINT64_C(64))) Section
	{
		struct Slice
		{
			alignas(alignof(T)) uint8_t memory[sizeof(T)];

			Slice() :
				memory()
			{ }

			no_transfer_functions(Slice)
		};

		Slice slices[64];
		uint64_t availabilityMap;

		size_t chainIndex;
		std::unique_ptr<Section> nextSection;

		Section(size_t chainIndex) :
			chainIndex(chainIndex),
			availabilityMap(0b1111111111111111111111111111111111111111111111111111111111111111),
			nextSection(nullptr)
		{ }

		T* LocateNextObject(uint8_t startIndex)
		{
			for (uint8_t i = startIndex; i < UINT8_C(64); ++i)
				if (((availabilityMap >> i) & 0b1) == 0)
				{
					return reinterpret_cast<T*>(&slices[i]);
				}

			if (nextSection != nullptr)
				return nextSection->LocateNextObject(0);
			else
				return nullptr;
		}

		void* operator new(size_t i)
		{
			return aligned_alloc(i, UINT64_C(0b1) << CT::CoverageLog2(sizeof(T) * UINT64_C(64)));
			//return _aligned_malloc(i, alignof(Section));
		}

		void operator delete(void* p)
		{
			std::free(p);
			//_aligned_free(p);
		}

		no_transfer_functions(Section)
	};

	std::unique_ptr<Section> first;
	Section* nextAvailable;
public:
	class Iterator
	{
	private:
		T* obj;
	public:
		Iterator(T* obj) :
			obj(obj)
		{ }

		Iterator& operator++()
		{
			if (obj)
			{
				Section* section = reinterpret_cast<Section*>(reinterpret_cast<intptr_t>(obj) / alignof(Section) * alignof(Section));
				obj = section->LocateNextObject(static_cast<uint8_t>(obj - reinterpret_cast<T*>(&section->slices[0]) + 1));
			}

			return *this;
		}

		Iterator operator++(int)
		{
			Iterator iterator = *this;
			++*this;

			return *this;
		}
		
		bool operator!=(const Iterator& iterator)
		{
			return obj != iterator.obj;
		}

		T* operator*()
		{
			return obj;
		}
	};

	MemoryPool() :
		first(new Section(0)),
		nextAvailable(first.get())
	{ }

	~MemoryPool()
	{
		for (T* obj : *this)
			obj->~T();
	}

	template<typename... Args>
	T* alloc(Args... args)
	{
		for (uint8_t i = 0; i < UINT8_C(64); ++i)
		{
			if (((nextAvailable->availabilityMap >> i) & 0b1) == 1)
			{
				T* obj = new (&nextAvailable->slices[i].memory[0]) T(std::forward<Args>(args)...);

				nextAvailable->availabilityMap = ~(~nextAvailable->availabilityMap | (UINT64_C(0b1) << i));
				if (nextAvailable->availabilityMap == 0)
				{
					Section* search = nextAvailable;
					while (true)
					{
						if (search->nextSection == nullptr)
						{
							search->nextSection = std::unique_ptr<Section>(new Section(search->chainIndex + 1));
							nextAvailable = search->nextSection.get();

							return obj;
						}

						search = search->nextSection.get();

						if (search->availabilityMap != 0)
						{
							nextAvailable = search;

							return obj;
						}
					}
				}

				return obj;
			}
		}

		std::terminate();

		return nullptr;
	}

	void free(T* obj)
	{
		obj->~T();

		Section* section = reinterpret_cast<Section*>(reinterpret_cast<intptr_t>(reinterpret_cast<unsigned char*>(obj)) / alignof(Section) * alignof(Section));

		section->availabilityMap = section->availabilityMap | (UINT64_C(0b1) << (obj - reinterpret_cast<T*>(&section->slices[0])));

		if (section->chainIndex < nextAvailable->chainIndex)
			nextAvailable = section;
	}

	void clear()
	{
		for (T* obj : *this)
			obj->~T();

		first = std::unique_ptr<Section>(new Section(0));
		nextAvailable = first.get();
	}

	Iterator begin()
	{
		return Iterator(first->LocateNextObject(0));
	}

	Iterator end()
	{
		return Iterator(nullptr);
	}

	no_transfer_functions(MemoryPool)
};

//#include <bitset>

// B is base type (if child classes are used, and there are not all deallocated with free(), then this destructor must be virtual) M is memory slice size, and A is maximum alignment allowed for stored data types.
//pow(2.0, ceil(log(S * 64.0) / log(2.0)))
/*template<class B, size_t S = sizeof(B), size_t A = (8 >= alignof(B) ? 8 : alignof(B))>
class VirtualMemoryPool
{
	static_assert(S > 0, "S must be larger than 0");
	static_assert(0b1ui64 << CT::CoverageLog2(S * 64ui64) <= 8192, "S is too large to be poooled");
private:
	struct alignas(0b1ui64 << CT::CoverageLog2(S * 64ui64)) Section
	{
		struct Slice
		{
			alignas(A) unsigned __int8 memory[S];

			Slice() { }

			no_transfer_functions(Slice)
		};

		static_assert(S == sizeof(Slice), "sizeof(Slice) != S");

		Slice slices[64];
		// TODO: chainIndex should probably be size_t
		size_t chainIndex;
		uint64_t availabilityMap;
		uint64_t tailMap;
		std::unique_ptr<Section> nextSection;
		Section* previousSection;

		Section(size_t chainIndex) :
			chainIndex(chainIndex),
			availabilityMap(0b1111111111111111111111111111111111111111111111111111111111111111),
			tailMap(0),
			nextSection(nullptr),
			previousSection(nullptr)
		{ }

		Slice* LocateNextObject(unsigned __int8 startIndex)
		{
			for (unsigned __int8 i = startIndex; i < 64ui8; ++i)
				if (((tailMap >> i) & 0b1) == 1)
					return &slices[i];

			if (nextSection != nullptr)
				return nextSection->LocateNextObject(0);
			else
				return nullptr;
		}

		// TODO: Can use for reverse iterator
		/*Slice* LocatePreviousObject(unsigned __int8 startIndex)
		{
		for (unsigned __int8 i1 = startIndex + 1; i1 > 0; --i1)
		{
		unsigned __int8 = i - 1ui8;
		if ((tailMap >> i) & 0b1 == 1)
		return slices[i];
		}

		if (previousSection != nullptr)
		return previousSection->LocatePreviousObject(63);
		else
		return nullptr;
		}*//*

		Slice* AllocateAvailableSlice(unsigned __int8 requiredUnits)
		{
			unsigned __int8 unitCount = 0;
			unsigned __int8 startBit = 0;
			Slice* currentSlice = nullptr;

			for (unsigned __int8 i = 0; i < 64ui8; ++i)
			{
				if (((availabilityMap >> i) & 0b1) == 1)
				{
					if (unitCount == 0)
					{
						currentSlice = &slices[i];
						startBit = i;
					}
					++unitCount;
					if (unitCount == requiredUnits)
					{
						// TODO: Could probably do this without the loop
						for (unsigned __int8 i = 0; i < requiredUnits; ++i)
							availabilityMap = ~((~availabilityMap) | (0b1ui64 << (i + startBit)));
						tailMap = tailMap | (0b1ui64 << startBit);

						return currentSlice;
					}
				}
				else
					unitCount = 0;
			}

			return nullptr;
		}

		void FreeSlice(unsigned __int8 startUnit, unsigned __int8 requiredUnits)
		{
			for (unsigned __int8 i = 0; i < requiredUnits; ++i)
				availabilityMap = availabilityMap | (0b1ui64 << (i + startUnit));
			tailMap = ~(~tailMap | (0b1ui64 << startUnit));
		}

		Section& GetNextSection()
		{
			if (nextSection == nullptr)
			{
				nextSection = std::unique_ptr<Section>(new Section(chainIndex + 1));
				nextSection->previousSection = this;
			}

			return *nextSection;
		}

		void* operator new(size_t i)
		{
			return _aligned_malloc(i, alignof(Section));
		}

		void operator delete(void* p)
		{
			_aligned_free(p);
		}

		no_transfer_functions(Section)
	};

	std::unique_ptr<Section> first;
	Section* nextAvailable;
public:
	class Iterator
	{
	private:
		typename Section::Slice* objTail;
	public:
		Iterator(typename Section::Slice* objTail) :
			objTail(objTail)
		{ }

		Iterator& operator++()
		{
			if (objTail)
			{
				Section* section = reinterpret_cast<Section*>(reinterpret_cast<intptr_t>(objTail) / alignof(Section) * alignof(Section));
				objTail = section->LocateNextObject(static_cast<unsigned __int8>(objTail - &section->slices[0] + 1));
			}

			return *this;
		}

		Iterator operator++(int)
		{
			Iterator iterator = *this;
			++*this;

			return *this;
		}

		bool operator!=(const Iterator& iterator)
		{
			return objTail != iterator.objTail;
		}

		B* operator*()
		{
			return reinterpret_cast<B*>(&objTail->memory[0]);
		}
	};

	VirtualMemoryPool() :
		first(new Section(0)),
		nextAvailable(first.get())
	{ }

	~VirtualMemoryPool()
	{
		for (B* obj : *this)
			obj->~B();
	}

	template<class T, typename... Args>
	T* alloc(Args... args)
	{
		static_assert(sizeof(T) >= S, "sizeof(T) must be larger than 0");
		static_assert(alignof(T) <= A, "alignof(T) must be less than or equal to A");
		static_assert(sizeof(T) <= S * 64, "sizeof(T) must be less than or equal to sizeof(Slice) * 64 - sizeof(Slice*)");

		unsigned __int8 requiredUnits = static_cast<unsigned __int8>(ceil(static_cast<double>(sizeof(T)) / S));

		Section* section = nextAvailable;
		while (true)
		{
			Section::Slice* test = nullptr;
			if (test == nullptr)
				test = nullptr;
			Section::Slice* mem = section->AllocateAvailableSlice(requiredUnits);
			if (mem != nullptr)
			{
				T* obj = new (&mem->memory[0]) T(std::forward<Args>(args)...);

				if (section->availabilityMap == 0 && section == nextAvailable)
				{
					while (true)
					{
						section = &section->GetNextSection();
						if (section->availabilityMap != 0)
						{
							nextAvailable = section;

							return obj;
						}
					}
				}

				return obj;
			}

			section = &section->GetNextSection();
		}

		std::terminate();

		return nullptr;
	}

	template<class T>
	void free(T* obj)
	{
		obj->~T();

		unsigned __int8 usedUnits = static_cast<unsigned __int8>(ceil(static_cast<double>(sizeof(T)) / S));

		Section* section = reinterpret_cast<Section*>(reinterpret_cast<intptr_t>(obj) / alignof(Section) * alignof(Section));
		section->FreeSlice(static_cast<unsigned __int8>(reinterpret_cast<typename Section::Slice*>(obj) - &section->slices[0]), usedUnits);

		if (section->chainIndex < nextAvailable->chainIndex)
			nextAvailable = section;
	}

	void clear()
	{
		for (B* obj : *this)
		{
			obj->~B();
		}

		first = std::unique_ptr<Section>(new Section(0));
		nextAvailable = first.get();
	}

	Iterator begin()
	{
		return Iterator(first->LocateNextObject(0));
	}

	Iterator end()
	{
		return Iterator(nullptr);
	}

	no_transfer_functions(VirtualMemoryPool);
};*/

/*class Suspendable;

class SuspendedState
{
	friend class Suspendable;
private:
	std::unordered_map<Suspendable*, void*> suspendedData;
public:
	SuspendedState() { }
	~SuspendedState();
};

class Suspendable
{
private:
	static std::list<Suspendable*> allSuspendable;
public:
	Suspendable();
	virtual ~Suspendable();

	virtual void* Suspend() = 0;
	virtual void Resume(void* data) = 0;
	virtual void Swap(void* data) = 0;
	virtual void Deallocate(void* data) = 0;
	
	// TODO: Isn't safe if the same object is suspended twice
	static void SuspendAll(SuspendedState& state);

	template<class T>
	static void SuspendSpecific(SuspendedState& state)
	{
		for (Suspendable* suspendable : allSuspendable)
		{
			if (dynamic_cast<T*>(suspendable))
			{
				if (state.suspendedData.find(suspendable) != state.suspendedData.end())
					continue;
				state.suspendedData[suspendable] = suspendable->Suspend();
			}
		}
	}

	static void Resume(SuspendedState& state);
	static void Swap(SuspendedState& state);
};*/
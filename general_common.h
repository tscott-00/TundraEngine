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
void RemoveVectorComponent(std::vector<T>& list, const T& component) {
	for (size_t i = 0; i < list.size(); ++i)
		if (list[i] == component) {
			list[i] = list[list.size() - 1];
			list.pop_back();
			break;
		}
}

template<class T>
class RequestList {
private:
	struct Request {
		T val;
		bool isAdd;

		Request(const T& val, bool isAdd) {
			this->val = val;
			this->isAdd = isAdd;
		}
	};

	std::list<T> list;
	std::list<Request> requests;
public:
	void Add(const T& val) {
		requests.push_back(Request(val, true));
	}

	void Remove(const T& val) {
		requests.push_back(Request(val, false));
	}

	inline auto begin() {
		return list.begin();
	}

	inline const auto begin() const {
		return list.begin();
	}

	inline auto end() {
		return list.end();
	}

	inline const auto end() const {
		return list.end();
	}

	void Clear(){
		list.clear();
		requests.clear();
	}

	void ProcessRequests() {
		for (const Request& r : requests) {
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

namespace CT {
	constexpr size_t CoverageLog2(size_t n) {
		float current = static_cast<float>(n);
		size_t result = 1;
		while (current > 2.0F) {
			current *= 0.5F;
			++result;
		}

		return result;
	}
}

template<class T>
class MemoryPool {
	static_assert(UINT64_C(0b1) << CT::CoverageLog2(sizeof(T) * UINT64_C(64)) <= 8192, "T is too large to be poooled");
private:
	struct alignas(UINT64_C(0b1) << CT::CoverageLog2(sizeof(T) * UINT64_C(64))) Section {
		struct Slice {
			alignas(alignof(T)) uint8_t memory[sizeof(T)];

			Slice() :
				memory() { }

			no_transfer_functions(Slice)
		};

		Slice slices[64];
		uint64_t availabilityMap;

		size_t chainIndex;
		std::unique_ptr<Section> nextSection;

		Section(size_t chainIndex) :
			chainIndex(chainIndex),
			availabilityMap(0b1111111111111111111111111111111111111111111111111111111111111111),
			nextSection(nullptr) { }

		T* LocateNextObject(uint8_t startIndex) {
			for (uint8_t i = startIndex; i < UINT8_C(64); ++i)
				if (((availabilityMap >> i) & 0b1) == 0)
					return reinterpret_cast<T*>(&slices[i]);

			return nextSection != nullptr ? nextSection->LocateNextObject(0) : nullptr;
		}

		void* operator new(size_t i) {
			return aligned_alloc(i, UINT64_C(0b1) << CT::CoverageLog2(sizeof(T) * UINT64_C(64)));
			//return _aligned_malloc(i, alignof(Section));
		}

		void operator delete(void* p) {
			std::free(p);
			//_aligned_free(p);
		}

		no_transfer_functions(Section)
	};

	std::unique_ptr<Section> first;
	Section* nextAvailable;
public:
	class Iterator {
	private:
		T* obj;
	public:
		Iterator(T* obj) :
			obj(obj)
		{ }

		Iterator& operator++() {
			if (obj) {
				Section* section = reinterpret_cast<Section*>(reinterpret_cast<intptr_t>(obj) / alignof(Section) * alignof(Section));
				obj = section->LocateNextObject(static_cast<uint8_t>(obj - reinterpret_cast<T*>(&section->slices[0]) + 1));
			}

			return *this;
		}

		Iterator operator++(int) {
			Iterator iterator = *this;
			++*this;

			return *this;
		}
		
		bool operator!=(const Iterator& iterator) {
			return obj != iterator.obj;
		}

		T* operator*() {
			return obj;
		}
	};

	MemoryPool() :
		first(new Section(0)),
		nextAvailable(first.get())
	{ }

	~MemoryPool() {
		for (T* obj : *this)
			obj->~T();
	}

	template<typename... Args>
	T* alloc(Args... args) {
		for (uint8_t i = 0; i < UINT8_C(64); ++i) {
			if (((nextAvailable->availabilityMap >> i) & 0b1) == 1) {
				T* obj = new (&nextAvailable->slices[i].memory[0]) T(std::forward<Args>(args)...);

				nextAvailable->availabilityMap = ~(~nextAvailable->availabilityMap | (UINT64_C(0b1) << i));
				if (nextAvailable->availabilityMap == 0) {
					Section* search = nextAvailable;
					while (true) {
						if (search->nextSection == nullptr) {
							search->nextSection = std::unique_ptr<Section>(new Section(search->chainIndex + 1));
							nextAvailable = search->nextSection.get();

							return obj;
						}

						search = search->nextSection.get();

						if (search->availabilityMap != 0) {
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

	void free(T* obj) {
		obj->~T();

		Section* section = reinterpret_cast<Section*>(reinterpret_cast<intptr_t>(reinterpret_cast<unsigned char*>(obj)) / alignof(Section) * alignof(Section));

		section->availabilityMap = section->availabilityMap | (UINT64_C(0b1) << (obj - reinterpret_cast<T*>(&section->slices[0])));

		if (section->chainIndex < nextAvailable->chainIndex)
			nextAvailable = section;
	}

	void clear() {
		for (T* obj : *this)
			obj->~T();

		first = std::unique_ptr<Section>(new Section(0));
		nextAvailable = first.get();
	}

	Iterator begin() {
		return Iterator(first->LocateNextObject(0));
	}

	Iterator end() {
		return Iterator(nullptr);
	}

	no_transfer_functions(MemoryPool)
};

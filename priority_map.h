#pragma once
#include <vector>

// WARNING: Completely WIP, .../Java/General(Foundation)/ACSL/.../PriorityQueue.java

template <class K, class V>
struct MapEntry
{
	K key;
	V value;

	MapEntry(K key, V value)
	{
		this->key = key;
		this->value = value;
	}
};

template <class K, class V>
class MinPriorityMap
{
public:
	MinPriorityMap();

	//bool IsEmpty();
	//bool Peek();
	//bool Pop();
	//bool Push();
private:
	std::vector<MapEntry<K, V>> heap;
};
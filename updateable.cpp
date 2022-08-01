#include "updateable.h"

#include <list>
#include <unordered_map>

namespace
{
	// TODO: Use general_common RequestList
	struct ListRequest
	{
		Updateable* object;
		bool isAdd;

		ListRequest(Updateable* object, bool isAdd)
		{
			this->object = object;
			this->isAdd = isAdd;
		}
	};

	std::list<Updateable*> objects;
	//std::list<Updateable*>::iterator* priorityLocations[256];
	std::list<ListRequest> listRequests;
}

void Updateable::EnableUpdates() const
{
	listRequests.push_back(ListRequest(const_cast<Updateable*>(this), true));
}

void Updateable::DisableUpdates() const
{
	listRequests.push_back(ListRequest(const_cast<Updateable*>(this), false));
}

namespace
{
	inline void ProcessRequests()
	{
		// Adding and removing new/old objects
		for (auto request : listRequests)
		{
			if (request.isAdd)
			{
				// TODO: add thing correctly
				objects.push_back(request.object);
			}
			else
			{
				// TODO: find starting location of area, then iterate over that to find object (may go over if it can't be found, but that shouldn't be possible)
				objects.remove(request.object);
			}
		}
		listRequests.clear();
	}
}

namespace Internal
{
	/*namespace Storage
	{
		void* UpdateableManager::Suspender::Suspend()
		{
			ProcessRequests();

			std::list<Updateable*> test(std::move(objects));
			return new std::list<Updateable*>(std::move(objects));
		}

		void UpdateableManager::Suspender::Resume(void* data)
		{
			listRequests.clear();

			objects = std::move(*reinterpret_cast<std::list<Updateable*>*>(data));
			delete reinterpret_cast<std::list<Updateable*>*>(data);
		}

		void UpdateableManager::Suspender::Swap(void* data)
		{
			ProcessRequests();

			std::list<Updateable*>& vec = *reinterpret_cast<std::list<Updateable*>*>(data);
			std::swap(objects, vec);
		}

		void UpdateableManager::Suspender::Deallocate(void* data)
		{
			delete reinterpret_cast<std::list<Updateable*>*>(data);
		}
	}*/

	void Update(const GameTime& time)
	{
		ProcessRequests();

		// Updating objects
		for (auto object : objects)
			object->Update(time);
	}
}

/*
// Left over objects need to be deleted
// (Only objects and add list are deleted and not remove list.
// If removed list was deleted, then no matter what all objects would be deleted twice,
// because the object has either been added and is in objects or hasn't and is in the add list.
// Add list and object list are deleted seperately because they never overlap.
// An object can only be added once, and after being added the list is cleared.)
for (auto object : objects)
delete object;
for (auto object : objectAddList)
delete object;
*/
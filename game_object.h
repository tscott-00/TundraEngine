#pragma once

#include <unordered_map>

#include "general_common.h"
#include "transform.h"

template<class T>
class TComponent {
public:
	T* gameObject;

	TComponent() :
		gameObject(nullptr)
	{ }

	virtual ~TComponent() {}

	// Defined in the order they are called
	virtual void OnCreate() { }
	virtual void OnEnable() { }
	virtual void OnDisable() { }

	virtual void OnLMBClick() { }
	
	virtual TComponent* rtc_GetCopy() const = 0;

	no_assignment_operator(TComponent);
};

template<class T>
class TGameObject;

namespace Internal {
	namespace Storage {
		template<class T>
		struct GameObjects {
			static MemoryPool<TGameObject<T>> objects;
			static std::vector<TGameObject<T>*> destructionQueueA;
			static std::vector<TGameObject<T>*> destructionQueueB;
			static std::vector<TGameObject<T>*> destructionQueueC;
		};
	}
}

template<class T>
MemoryPool<TGameObject<T>> Internal::Storage::GameObjects<T>::objects;
template<class T>
std::vector<TGameObject<T>*> Internal::Storage::GameObjects<T>::destructionQueueA;
template<class T>
std::vector<TGameObject<T>*> Internal::Storage::GameObjects<T>::destructionQueueB;
template<class T>
std::vector<TGameObject<T>*> Internal::Storage::GameObjects<T>::destructionQueueC;

template<class T>
class TGameObject {
public:
	T transform;
	// make sure it is transfered when instantiating
	//unsigned char layer;

	TGameObject() {
		components = new std::vector<TComponent<TGameObject<T>>*>();
		transform.SetOwner(this);
	}

	TGameObject(const TGameObject<T>& other) :
		transform(other.transform) {
		components = new std::vector<TComponent<TGameObject<T>>*>();

		transform.SetOwner(this);
		SetComponents(other);
	}

	~TGameObject() {
		if (transform.isEnabled) {
			for (auto component : *components) {
				component->OnDisable();
				delete component;
			}
		} else {
			for (auto component : *components)
				delete component;
		}

		delete components;
	}

	template<class U, typename... Args>
	U* AddComponent(Args... args) {
		U* component = new U(std::forward<Args>(args)...);
		components->push_back(component);
		
		component->gameObject = this;
		component->OnCreate();

		if (transform.isEnabled)
			component->OnEnable();

		return component;
	}

	void SetComponents(const TGameObject<T>& other) {
		for (TComponent<TGameObject<T>>* component : *components) {
			if (transform.isEnabled)
				component->OnDisable();
			delete component;
		}
		components->clear();

		for (TComponent<TGameObject<T>>* component : *other.components) {
			TComponent<TGameObject<T>>* newComponent = component->rtc_GetCopy();
			if (newComponent == nullptr)
				continue;
			// Finish if the component was copyable
			components->push_back(newComponent);
			newComponent->gameObject = this;
			newComponent->OnCreate();

			if (transform.isEnabled)
				newComponent->OnEnable();
		}
	}

	template<class C>
	C* GetComponent() {
		for (TComponent<TGameObject<T>>* t : *components)
			if (C* c = dynamic_cast<C*>(t))
				return c;
		return nullptr;
	}

	template<class C>
	std::vector<C*> GetAllComponents() {
		std::vector<C*> result;
		for (TComponent<TGameObject<T>>* t : *components)
			if (C* c = dynamic_cast<C*>(t))
				result.push_back(c);

		return result;
	}

	template<class C>
	void RemoveComponent() {
		for (size_t i = 0; i < components->size(); ++i)
			if (C* c = dynamic_cast<C*>((*components)[i])) {
				if (transform.isEnabled)
					(*components)[i]->OnDisable();

				(*components)[i] = (*components)[components->size() - 1];
				components->pop_back();

				return;
			}
	}

	template<class C>
	void RemoveAllComponentsOfType() {
	remove_component:
		for (size_t i = 0; i < components->size(); ++i)
			if (C* c = dynamic_cast<C*>((*components)[i])) {
				if (transform.isEnabled)
					(*components)[i]->OnDisable();

				(*components)[i] = (*components)[components->size() - 1];
				components->pop_back();

				goto remove_component;
			}
	}

	inline void SetParent(TGameObject<T>& gameObject) {
		transform.SetParent(gameObject.transform);
		if (gameObject.IsEnabled())
			Enable();
		else
			Disable();
	}

	// TODO: Support null!
	inline void SetParent(TGameObject<T>* gameObject) {
		if (gameObject != nullptr)
			SetParent(*gameObject);
	}

	inline bool IsEnabled() const {
		return transform.isEnabled;
	}

	inline void SetEnabled(bool enabled) {
		if (transform.isEnabled != enabled) {
			if (enabled)
				Enable();
			else
				Disable();
		}
	}

	inline void SwitchEnabled() {
		if (transform.isEnabled)
			Disable();
		else
			Enable();
	}

	inline void Enable() {
		if (transform.isEnabled) return;
		transform.isEnabled = true;
		for (auto component : *components)
			component->OnEnable();
		for (T* t : transform.GetChildren())
			if (t->GetOwner() != nullptr)
				t->GetOwner()->Enable();
	}

	inline void Disable() {
		if (!transform.isEnabled) return;
		transform.isEnabled = false;
		for (auto component : *components)
			component->OnDisable();
		for (T* t : transform.GetChildren())
			if (t->GetOwner() != nullptr)
				t->GetOwner()->Disable();
	}

	no_assignment_operator(TGameObject);
private:
	std::vector<TComponent<TGameObject<T>>*>* components;
};

namespace Scene {
	template<class T>
	void Clear() {
		// WARNING: could cause problems if something tried to instantiate or destroy while going through
		for (TGameObject<T>* object : Internal::Storage::GameObjects<T>::objects)
			object->Disable();
		Internal::Storage::GameObjects<T>::objects.clear();
	}

	template<class T>
	void rtc_DeleteObject(TGameObject<T>* object) {
		for (T* t : object->transform.GetChildren())
			if (t->GetOwner() != nullptr)
				rtc_DeleteObject(t->GetOwner());

		Internal::Storage::GameObjects<T>::objects.free(object);
	}

	template<class T>
	void Update() {
		while (!Internal::Storage::GameObjects<T>::destructionQueueA.empty()) {
			std::swap(Internal::Storage::GameObjects<T>::destructionQueueA, Internal::Storage::GameObjects<T>::destructionQueueB);

			for (TGameObject<T>* object : Internal::Storage::GameObjects<T>::destructionQueueB) {
				object->Disable();
				Internal::Storage::GameObjects<T>::destructionQueueC.push_back(object);
			}
			Internal::Storage::GameObjects<T>::destructionQueueB.clear();
		}

		for (TGameObject<T>* object : Internal::Storage::GameObjects<T>::destructionQueueC)
			rtc_DeleteObject(object);
		Internal::Storage::GameObjects<T>::destructionQueueC.clear();
	}
}

template<class G, class T>
inline G* Instantiate(const T& transform) {
	G* newObject = Internal::Storage::GameObjects<T>::objects.alloc();
	newObject->transform = transform;

	return newObject;
}

template<class G, class T>
inline G* Instantiate(G* other, const T& transform) {
	G* newObject = Internal::Storage::GameObjects<T>::objects.alloc(*other);
	//G* newObject = Internal::Storage::GameObjects<T>::objects.alloc<const G&>(*other);
	newObject->transform = transform;

	for (T* t : other->transform.GetChildren()) {
		if (t->GetOwner() == nullptr) continue;
		TGameObject<T>* n = Instantiate<G, T>(t->GetOwner(), *t);
		n->transform.SetParent(newObject->transform);
	}

	return newObject;
}

template<class T>
void Destroy(TGameObject<T>* object) {
	Internal::Storage::GameObjects<T>::destructionQueueA.push_back(object);
}

typedef TGameObject<Transform3> GameObject3;
typedef TGameObject<Transform2> GameObject2;

typedef TComponent<GameObject3> Component3;
typedef TComponent<GameObject2> Component2;

#pragma once

#include "game_time.h"
#include "game_object.h"

class Updateable {
public:
	Updateable();

	virtual void Update(const GameTime& time) = 0;
protected:
	void EnableUpdates() const;
	void DisableUpdates() const;
};

template <class T>
class TUpdateableComponent : public TComponent<T>, public Updateable {
public:
	TUpdateableComponent() { }

	void OnEnable() override {
		EnableUpdates();
	}

	void OnDisable() override {
		DisableUpdates();
	}
};

namespace Internal {
	void Update(const GameTime& time);
}

typedef TUpdateableComponent<GameObject2> UpdateableComponent2;
typedef TUpdateableComponent<GameObject3> UpdateableComponent3;

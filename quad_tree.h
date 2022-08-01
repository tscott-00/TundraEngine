#pragma once

/*class EmptyQuadTree
{
public:
	static const unsigned int CHILDREN_COUNT = 4;
private:
	EmptyQuadTree* children[CHILDREN_COUNT];
public:
	EmptyQuadTree()
	{
		children[0] = nullptr;
		children[1] = nullptr;
		children[2] = nullptr;
		children[3] = nullptr;
	}

	void Subdivide()
	{
		children[0] = new EmptyQuadTree();
		children[1] = new EmptyQuadTree();
		children[2] = new EmptyQuadTree();
		children[3] = new EmptyQuadTree();
	}

	virtual ~EmptyQuadTree()
	{
		if (children[0] != nullptr)
		{
			delete children[0];
			delete children[1];
			delete children[2];
			delete children[3];
		}
	}

	EmptyQuadTree* operator[](unsigned int i) const
	{
		return children[i];
	}

	EmptyQuadTree(const EmptyQuadTree&) = delete;
	EmptyQuadTree& operator=(const EmptyQuadTree&) = delete;
};*/

class StateQuadTree
{
public:
	static const unsigned int CHILDREN_COUNT = 4;
private:
	StateQuadTree* parent;
	StateQuadTree* children[CHILDREN_COUNT];
	bool state;

	void ClearChildren()
	{
		delete children[0];
		delete children[1];
		delete children[2];
		delete children[3];

		children[0] = nullptr;
		children[1] = nullptr;
		children[2] = nullptr;
		children[3] = nullptr;
	}
public:
	StateQuadTree()
	{
		parent = nullptr;
		children[0] = nullptr;
		children[1] = nullptr;
		children[2] = nullptr;
		children[3] = nullptr;
		state = true;
	}

	StateQuadTree& Subdivide()
	{
		children[0] = new StateQuadTree();
		children[1] = new StateQuadTree();
		children[2] = new StateQuadTree();
		children[3] = new StateQuadTree();
		children[0]->parent = this;
		children[1]->parent = this;
		children[2]->parent = this;
		children[3]->parent = this;

		return *this;
	}

	const bool& GetState() const
	{
		return state;
	}

	void FalsifyState()
	{
		state = false;
		if (parent != nullptr && parent->children[0]->state == false && parent->children[1]->state == false && parent->children[2]->state == false && parent->children[3]->state == false)
		{
			parent->state = false;
			parent->ClearChildren();
		}
	}

	virtual ~StateQuadTree()
	{
		if (children[0] != nullptr)
		{
			delete children[0];
			delete children[1];
			delete children[2];
			delete children[3];
		}
	}

	StateQuadTree* operator[](unsigned int i) const
	{
		return children[i];
	}

	StateQuadTree(const StateQuadTree&) = delete;
	StateQuadTree& operator=(const StateQuadTree&) = delete;
};

// TODO: Implement
template<class T>
class TQuadTree
{
};

typedef TQuadTree<bool> BoolQuadTree;
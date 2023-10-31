#ifndef __PROGTEST__
#include <array>
#include <cassert>
#include <cstdarg>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <optional>
#include <random>
#include <type_traits>

// We use std::set as a reference to check our implementation.
// It is not available in progtest :)
#include <set>

template <typename T>
struct Ref {
	size_t size() const { return _data.size(); }
	const T *find(const T &value) const {
		auto it = _data.find(value);
		if (it == _data.end())
			return nullptr;
		return &*it;
	}
	bool insert(const T &value) { return _data.insert(value).second; }
	bool erase(const T &value) { return _data.erase(value); }

	auto begin() const { return _data.begin(); }
	auto end() const { return _data.end(); }

private:
	std::set<T> _data;
};

#endif

namespace config {
	// Enable to check that the tree is AVL balanced.
	inline constexpr bool CHECK_DEPTH = false;

	// Disable if your implementation does not have parent pointers
	inline constexpr bool PARENT_POINTERS = true;
}

// TODO implement
template <typename T>
struct Tree {
	struct Node {
		Node *m_parent;
		Node *m_leftChild;
		Node *m_rightChild;
		T m_value;
		ssize_t m_sign;
		size_t m_height;
		Node(T value) : m_value(value) {
			// sanity check
			m_parent = nullptr;
			m_leftChild = nullptr;
			m_rightChild = nullptr;
			m_sign = 0;
			m_height = 0;
		}

		bool operator<(const Node &other) const {
			return std::less<T>{}(m_value, other.m_value);
		}

		bool operator>(const Node &other) const {
			return std::less<T>{}(other.m_value, m_value);
		}

		bool operator==(const Node &other) const {
			return !(*this < other) && !(other < *this);
		}

		bool operator!=(const Node &other) const {
			return (*this < other) || (other < *this);
		}

		void calculateNewSign() {
			m_sign = 0;
			if (m_leftChild) {
				m_sign -= m_leftChild->m_height + 1;
			}
			if (m_rightChild) {
				m_sign += m_rightChild->m_height + 1;
			}
		}

		void calculateNewHeight() {
			m_height = 0;
			if (m_leftChild) {
				m_height = m_leftChild->m_height + 1;
			}
			if (m_rightChild && m_height < m_rightChild->m_height + 1) {
				m_height = m_rightChild->m_height + 1;
			}

			calculateNewSign();
		}

		void swapValues(Node &other) {
			T temp = m_value;
			m_value = other.m_value;
			other.m_value = temp;
		}
	};

	Node *m_root;
	size_t m_size;

	void leftRotate(Node *x) {
		Node *parent = x->m_parent;
		Node *y = x->m_rightChild;

		Node *subtreeB = y->m_leftChild;

		x->m_parent = y;
		y->m_leftChild = x;
		if (subtreeB) {
			subtreeB->m_parent = x;
		}
		x->m_rightChild = subtreeB;
		y->m_parent = parent;

		x->calculateNewHeight();
		y->calculateNewHeight();

		if (!parent) {
			m_root = y;
			return;
		}
		if (parent->m_leftChild == x) {
			parent->m_leftChild = y;
			return;
		}
		parent->m_rightChild = y;
	}

	void rightRotate(Node *x) {
		Node *parent = x->m_parent;
		Node *y = x->m_leftChild;

		Node *subtreeB = y->m_rightChild;

		x->m_parent = y;
		y->m_rightChild = x;
		if (subtreeB) {
			subtreeB->m_parent = x;
		}
		x->m_leftChild = subtreeB;
		y->m_parent = parent;

		x->calculateNewHeight();
		y->calculateNewHeight();

		if (!parent) {
			m_root = y;
			return;
		}
		if (parent->m_leftChild == x) {
			parent->m_leftChild = y;
			return;
		}
		parent->m_rightChild = y;
	}

	void leftRightRotate(Node *x) {
		leftRotate(x->m_leftChild);
		rightRotate(x);
	}

	void rightLeftRotate(Node *x) {
		rightRotate(x->m_rightChild);
		leftRotate(x);
	}

	size_t size() const {
		return m_size;
	}

	Node *findByValue(const T &value) const {
		Node toFind(value);
		Node *visiting = m_root;
		while (visiting != nullptr && *visiting != toFind) {
			visiting = *visiting > toFind ? visiting->m_leftChild : visiting->m_rightChild;
		}
		return visiting;
	}

	void balance(Node *visiting) {
		while (visiting) {
			// calculate new height
			visiting->calculateNewHeight();
			if (visiting->m_sign < -1) {
				if (visiting->m_leftChild && visiting->m_leftChild->m_sign == 1) {
					leftRightRotate(visiting);
				} else {
					rightRotate(visiting);
				}
			} else if (visiting->m_sign > 1) {
				if (visiting->m_rightChild && visiting->m_rightChild->m_sign == -1) {
					rightLeftRotate(visiting);
				} else {
					leftRotate(visiting);
				}
			}
			visiting = visiting->m_parent;
		}
	}

	const T *find(const T &value) const {
		Node *found = findByValue(value);
		return found ? &(found->m_value) : nullptr;
	}
	bool insert(T value) {
		Node *toInsert = new Node(value);
		// if tree is empty
		if (!m_root) {
			m_root = toInsert;
			++m_size;
			return true;
		}

		Node *visiting = m_root;
		bool left = true;
		while (visiting) {
			toInsert->m_parent = visiting;
			if (*toInsert == *visiting) {
				delete toInsert;
				return false;
			}
			left = *toInsert < *visiting;
			visiting = left ? visiting->m_leftChild : visiting->m_rightChild;
		}
		if (left) {
			toInsert->m_parent->m_leftChild = toInsert;
		} else {
			toInsert->m_parent->m_rightChild = toInsert;
		}
		++m_size;

		balance(toInsert->m_parent);

		return true;
	}

	Node *findMin(Node *n) const {
		if (!n) {
			return nullptr;
		}
		while (n->m_leftChild) {
			n = n->m_leftChild;
		}
		return n;
	}

	bool erase(const T &value) {
		Node *toDelete = findByValue(value);
		// case 1 - this node is not in tree
		if (!toDelete) {
			return false;
		}

		while (true) {
			// case 2 - this node is a leaf
			if (!toDelete->m_leftChild && !toDelete->m_rightChild) {
				// if parent exists, remove this node as it's child
				if (toDelete->m_parent) {
					if (toDelete->m_parent->m_leftChild == toDelete) {
						toDelete->m_parent->m_leftChild = nullptr;
					} else {
						toDelete->m_parent->m_rightChild = nullptr;
					}
				}
				break;
			}
			// case 3 - this node has one child
			// case 3.1 - the child is left
			if (toDelete->m_leftChild && !toDelete->m_rightChild) {
				// set toDelete's parent as parent of toDelete's only child
				toDelete->m_leftChild->m_parent = toDelete->m_parent;
				// if parent exists, set it's child as this child
				if (toDelete->m_parent) {
					if (toDelete->m_parent->m_leftChild == toDelete) {
						toDelete->m_parent->m_leftChild = toDelete->m_leftChild;
					} else {
						toDelete->m_parent->m_rightChild = toDelete->m_leftChild;
					}
				} else { // parent doesn't exists - toDelete is root - set this child as root
					m_root = toDelete->m_leftChild;
				}
				break;
			}
			// case 3.2 - the child is right
			if (!toDelete->m_leftChild && toDelete->m_rightChild) {
				// set toDelete's parent as parent of toDelete's only child
				toDelete->m_rightChild->m_parent = toDelete->m_parent;
				// if parent exists, set it's child as this child
				if (toDelete->m_parent) {
					if (toDelete->m_parent->m_leftChild == toDelete) {
						toDelete->m_parent->m_leftChild = toDelete->m_rightChild;
					} else {
						toDelete->m_parent->m_rightChild = toDelete->m_rightChild;
					}
				} else { // parent doesn't exists - toDelete is root - set this child as root
					m_root = toDelete->m_rightChild;
				}
				break;
			}
			// case 4 - this node has two children
			Node *min = findMin(toDelete->m_rightChild);
			toDelete->swapValues(*min);
			toDelete = min;
		}

		Node *balanceFrom = toDelete->m_parent;
		// delete the node itself
		delete toDelete;
		--m_size;
		balance(balanceFrom);
		return true;
	}

	Tree() {
		m_root = nullptr;
		m_size = 0;
	}
	void recursiveDestruct(Node *toDelete) {
		if (toDelete) {
			recursiveDestruct(toDelete->m_leftChild);
			recursiveDestruct(toDelete->m_rightChild);
			delete toDelete;
		}
	}
	~Tree() {
		recursiveDestruct(m_root);
	}

	// Needed to test the structure of the tree.
	struct TesterInterface {
		static const Node *root(const Tree *t) { return t->m_root; }
		// Parent of root must be nullptr
		static const Node *parent(const Node *n) { return n->m_parent; }
		static const Node *right(const Node *n) { return n->m_rightChild; }
		static const Node *left(const Node *n) { return n->m_leftChild; }
		static const T &value(const Node *n) { return n->m_value; }
	};
};

#ifndef __PROGTEST__

struct TestFailed : std::runtime_error {
	using std::runtime_error::runtime_error;
};

std::string fmt(const char *f, ...) {
	va_list args1;
	va_list args2;
	va_start(args1, f);
	va_copy(args2, args1);

	std::string buf(vsnprintf(nullptr, 0, f, args1), '\0');
	va_end(args1);

	vsnprintf(buf.data(), buf.size() + 1, f, args2);
	va_end(args2);

	return buf;
}

template <typename T>
struct Tester {
	Tester() = default;

	void size() const {
		size_t r = ref.size();
		size_t t = tested.size();
		if (r != t)
			throw TestFailed(fmt("Size: got %zu but expected %zu.", t, r));
	}

	void find(const T &x) const {
		auto r = ref.find(x);
		auto t = tested.find(x);
		bool found_r = r != nullptr;
		bool found_t = t != nullptr;

		if (found_r != found_t)
			_throw("Find mismatch", found_r);
		if (found_r && *t != x)
			throw TestFailed("Find: found different value");
	}

	void insert(const T &x, bool check_tree_ = false) {
		auto succ_r = ref.insert(x);
		auto succ_t = tested.insert(x);
		if (succ_r != succ_t)
			_throw("Insert mismatch", succ_r);
		size();
		if (check_tree_)
			check_tree();
	}

	void erase(const T &x, bool check_tree_ = false) {
		bool succ_r = ref.erase(x);
		auto succ_t = tested.erase(x);
		if (succ_r != succ_t)
			_throw("Erase mismatch", succ_r);
		size();
		if (check_tree_)
			check_tree();
	}

	struct NodeCheckResult {
		const T *min = nullptr;
		const T *max = nullptr;
		int depth = -1;
		size_t size = 0;
	};

	void check_tree() const {
		using TI = typename Tree<T>::TesterInterface;
		auto ref_it = ref.begin();
		bool check_value_failed = false;
		auto check_value = [&](const T &v) {
			if (check_value_failed)
				return;
			check_value_failed = (ref_it == ref.end() || *ref_it != v);
			if (!check_value_failed)
				++ref_it;
		};

		auto r = check_node(TI::root(&tested), decltype(TI::root(&tested))(nullptr), check_value);
		size_t t_size = tested.size();

		if (t_size != r.size)
			throw TestFailed(
				fmt("Check tree: size() reports %zu but expected %zu.", t_size, r.size));

		if (check_value_failed)
			throw TestFailed(
				"Check tree: element mismatch");

		size();
	}

	template <typename Node, typename F>
	NodeCheckResult check_node(const Node *n, const Node *p, F &check_value) const {
		if (!n)
			return {};

		using TI = typename Tree<T>::TesterInterface;
		if constexpr (config::PARENT_POINTERS) {
			if (TI::parent(n) != p)
				throw TestFailed("Parent mismatch.");
		}

		auto l = check_node(TI::left(n), n, check_value);
		check_value(TI::value(n));
		auto r = check_node(TI::right(n), n, check_value);

		if (l.max && !(*l.max < TI::value(n)))
			throw TestFailed("Max of left subtree is too big.");
		if (r.min && !(TI::value(n) < *r.min))
			throw TestFailed("Min of right subtree is too small.");

		if (config::CHECK_DEPTH && abs(l.depth - r.depth) > 1)
			throw TestFailed(fmt(
				"Tree is not avl balanced: left depth %i and right depth %i.",
				l.depth, r.depth));

		return {
			l.min ? l.min : &TI::value(n),
			r.max ? r.max : &TI::value(n),
			std::max(l.depth, r.depth) + 1, 1 + l.size + r.size};
	}

	static void _throw(const char *msg, bool s) {
		throw TestFailed(fmt("%s: ref %s.", msg, s ? "succeeded" : "failed"));
	}

	Tree<T> tested;
	Ref<T> ref;
};

void test_insert() {
	Tester<int> t;

	for (int i = 0; i < 10; i++)
		t.insert(i, true);
	for (int i = -10; i < 20; i++)
		t.find(i);

	for (int i = 0; i < 10; i++)
		t.insert((1 + i * 7) % 17, true);
	for (int i = -10; i < 20; i++)
		t.find(i);
}

void test_erase() {
	Tester<int> t;

	for (int i = 0; i < 10; i++)
		t.insert((1 + i * 7) % 17, true);
	for (int i = -10; i < 20; i++)
		t.find(i);

	for (int i = 3; i < 22; i += 2)
		t.erase(i, true);
	for (int i = -10; i < 20; i++)
		t.find(i);

	for (int i = 0; i < 10; i++)
		t.insert((1 + i * 13) % 17 - 8, true);
	for (int i = -10; i < 20; i++)
		t.find(i);

	for (int i = -4; i < 10; i++)
		t.erase(i, true);
	for (int i = -10; i < 20; i++)
		t.find(i);
}

enum RandomTestFlags : unsigned {
	SEQ = 1,
	NO_ERASE = 2,
	CHECK_TREE = 4
};

void test_random(size_t size, unsigned flags = 0) {
	Tester<size_t> t;
	std::mt19937 my_rand(24707 + size);

	bool seq = flags & SEQ;
	bool erase = !(flags & NO_ERASE);
	bool check_tree = flags & CHECK_TREE;

	for (size_t i = 0; i < size; i++)
		t.insert(seq ? 2 * i : my_rand() % (3 * size), check_tree);

	t.check_tree();

	for (size_t i = 0; i < 3 * size + 1; i++)
		t.find(i);

	for (size_t i = 0; i < 30 * size; i++)
		switch (my_rand() % 5) {
			case 1:
				t.insert(my_rand() % (3 * size), check_tree);
				break;
			case 2:
				if (erase)
					t.erase(my_rand() % (3 * size), check_tree);
				break;
			default:
				t.find(my_rand() % (3 * size));
		}

	t.check_tree();
}

int main() {
	try {
		std::cout << "Insert test..." << std::endl;
		test_insert();

		std::cout << "Erase test..." << std::endl;
		test_erase();

		std::cout << "Tiny random test..." << std::endl;
		test_random(20, CHECK_TREE);

		std::cout << "Small random test..." << std::endl;
		test_random(200, CHECK_TREE);

		std::cout << "Big random test..." << std::endl;
		test_random(50'000);

		std::cout << "Big sequential test..." << std::endl;
		test_random(50'000, SEQ);

		std::cout << "All tests passed." << std::endl;
	} catch (const TestFailed &e) {
		std::cout << "Test failed: " << e.what() << std::endl;
	}
}

#endif

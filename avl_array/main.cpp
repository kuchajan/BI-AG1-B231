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

// We use std::vector as a reference to check our implementation.
// It is not available in progtest :)
#include <vector>

template <typename T>
struct Ref {
	bool empty() const { return _data.empty(); }
	size_t size() const { return _data.size(); }

	const T &operator[](size_t index) const { return _data.at(index); }
	T &operator[](size_t index) { return _data.at(index); }

	void insert(size_t index, T value) {
		if (index > _data.size())
			throw std::out_of_range("oops");
		_data.insert(_data.begin() + index, std::move(value));
	}

	T erase(size_t index) {
		T ret = std::move(_data.at(index));
		_data.erase(_data.begin() + index);
		return ret;
	}

	auto begin() const { return _data.begin(); }
	auto end() const { return _data.end(); }

private:
	std::vector<T> _data;
};

#endif

namespace config {
	inline constexpr bool PARENT_POINTERS = true;
	inline constexpr bool CHECK_DEPTH = true;
}

template <typename T>
struct Array {
	struct Node {
		// * tree structure variables
		// neighbours
		Node *m_parent;
		Node *m_leftChild;
		Node *m_rightChild;
		// avl variables
		size_t m_height;
		// * node value variables
		T m_value;
		size_t m_index;

		Node(T value, size_t index) : m_value(value), m_index(index) {
			// sanity check
			m_parent = nullptr;
			m_leftChild = nullptr;
			m_rightChild = nullptr;
			m_height = 0;
		}

		ssize_t getSign() const {
			return (m_rightChild != nullptr ? (ssize_t)(m_rightChild->m_height + 1) : (ssize_t)0) - (m_leftChild != nullptr ? (ssize_t)(m_leftChild->m_height + 1) : (ssize_t)0);
		}

		void calculateNewHeight() {
			m_height = 0;
			// it is expected that children's height is already calculated
			if (m_leftChild) {
				m_height = m_leftChild->m_height + 1;
			}
			if (m_rightChild && m_height < m_rightChild->m_height + 1) {
				m_height = m_rightChild->m_height + 1;
			}
		}

		void swapNodes(Node &other) {
			std::swap(m_value, other.m_value);
			std::swap(m_index, other.m_index);
		}

		bool operator<(const Node &other) const {
			return m_index < other.m_index;
		}
		bool operator>(const Node &other) const {
			return m_index > other.m_index;
		}
		bool operator<=(const Node &other) const {
			return m_index <= other.m_index;
		}
		bool operator>=(const Node &other) const {
			return m_index >= other.m_index;
		}
		bool operator==(const Node &other) const {
			return m_index == other.m_index;
		}
		bool operator!=(const Node &other) const {
			return m_index != other.m_index;
		}
	};

	Node *find(size_t index) const {
		if (index >= size()) {
			throw std::out_of_range("Index is outside of tree size");
		}
		Node *visiting = m_root;
		while (visiting && visiting->m_index != index) {
			visiting = visiting->m_index > index ? visiting->m_leftChild : visiting->m_rightChild;
		}
		if (visiting == nullptr) {
			throw std::logic_error("Index does not exist");
		}
		return visiting;
	}

	Node *m_root;
	size_t m_size;

	Array() {
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
	~Array() {
		recursiveDestruct(m_root);
	}

	bool empty() const {
		return m_root == nullptr;
	}
	size_t size() const {
		return m_size;
	}

	const T &operator[](size_t index) const {
		return (find(index)->m_value);
	}
	T &operator[](size_t index) {
		return (find(index)->m_value);
	}

	void insert(size_t index, T value);
	T erase(size_t index);

	// Needed to test the structure of the tree.
	// Replace Node with the real type of your nodes
	// and implementations with the ones matching
	// your attributes.
	struct TesterInterface {
		// using Node = ...
		static const Node *root(const Array *t) { return t->root; }
		// Parent of root must be nullptr, ignore if config::PARENT_POINTERS == false
		static const Node *parent(const Node *n) { return n->parent; }
		static const Node *right(const Node *n) { return n->right; }
		static const Node *left(const Node *n) { return n->left; }
		static const T &value(const Node *n) { return n->value; }
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

	size_t size() const {
		bool te = tested.empty();
		size_t r = ref.size();
		size_t t = tested.size();
		if (te != !t)
			throw TestFailed(fmt("Size: size %zu but empty is %s.",
								 t, te ? "true" : "false"));
		if (r != t)
			throw TestFailed(fmt("Size: got %zu but expected %zu.", t, r));
		return r;
	}

	const T &operator[](size_t index) const {
		const T &r = ref[index];
		const T &t = tested[index];
		if (r != t)
			throw TestFailed("Op [] const mismatch.");
		return t;
	}

	void assign(size_t index, T x) {
		ref[index] = x;
		tested[index] = std::move(x);
		operator[](index);
	}

	void insert(size_t i, T x, bool check_tree_ = false) {
		ref.insert(i, x);
		tested.insert(i, std::move(x));
		size();
		if (check_tree_)
			check_tree();
	}

	T erase(size_t i, bool check_tree_ = false) {
		T r = ref.erase(i);
		T t = tested.erase(i);
		if (r != t)
			TestFailed(fmt("Erase mismatch at %zu.", i));
		size();
		if (check_tree_)
			check_tree();
		return t;
	}

	void check_tree() const {
		using TI = typename Array<T>::TesterInterface;
		auto ref_it = ref.begin();
		bool check_value_failed = false;
		auto check_value = [&](const T &v) {
			if (check_value_failed)
				return;
			check_value_failed = (ref_it == ref.end() || *ref_it != v);
			if (!check_value_failed)
				++ref_it;
		};

		size();

		check_node(TI::root(&tested), decltype(TI::root(&tested))(nullptr), check_value);

		if (check_value_failed)
			throw TestFailed(
				"Check tree: element mismatch");
	}

	template <typename Node, typename F>
	int check_node(const Node *n, const Node *p, F &check_value) const {
		if (!n)
			return -1;

		using TI = typename Array<T>::TesterInterface;
		if constexpr (config::PARENT_POINTERS) {
			if (TI::parent(n) != p)
				throw TestFailed("Parent mismatch.");
		}

		auto l_depth = check_node(TI::left(n), n, check_value);
		check_value(TI::value(n));
		auto r_depth = check_node(TI::right(n), n, check_value);

		if (config::CHECK_DEPTH && abs(l_depth - r_depth) > 1)
			throw TestFailed(fmt(
				"Tree is not avl balanced: left depth %i and right depth %i.",
				l_depth, r_depth));

		return std::max(l_depth, r_depth) + 1;
	}

	static void _throw(const char *msg, bool s) {
		throw TestFailed(fmt("%s: ref %s.", msg, s ? "succeeded" : "failed"));
	}

	Array<T> tested;
	Ref<T> ref;
};

void test_insert() {
	Tester<int> t;

	for (int i = 0; i < 10; i++)
		t.insert(i, i, true);
	for (int i = 0; i < 10; i++)
		t.insert(i, -i, true);
	for (size_t i = 0; i < t.size(); i++)
		t[i];

	for (int i = 0; i < 5; i++)
		t.insert(15, (1 + i * 7) % 17, true);
	for (int i = 0; i < 10; i++)
		t.assign(2 * i, 3 * t[2 * i]);
	for (size_t i = 0; i < t.size(); i++)
		t[i];
}

void test_erase() {
	Tester<int> t;

	for (int i = 0; i < 10; i++)
		t.insert(i, i, true);
	for (int i = 0; i < 10; i++)
		t.insert(i, -i, true);

	for (size_t i = 3; i < t.size(); i += 2)
		t.erase(i, true);
	for (size_t i = 0; i < t.size(); i++)
		t[i];

	for (int i = 0; i < 5; i++)
		t.insert(3, (1 + i * 7) % 17, true);
	for (size_t i = 1; i < t.size(); i += 3)
		t.erase(i, true);

	for (int i = 0; i < 20; i++)
		t.insert(3, 100 + i, true);

	for (int i = 0; i < 5; i++)
		t.erase(t.size() - 1, true);
	for (int i = 0; i < 5; i++)
		t.erase(0, true);

	for (int i = 0; i < 4; i++)
		t.insert(i, i, true);
	for (size_t i = 0; i < t.size(); i++)
		t[i];
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

	for (size_t i = 0; i < size; i++) {
		size_t pos = seq ? 0 : my_rand() % (i + 1);
		t.insert(pos, my_rand() % (3 * size), check_tree);
	}

	t.check_tree();

	for (size_t i = 0; i < t.size(); i++)
		t[i];

	for (size_t i = 0; i < 30 * size; i++)
		switch (my_rand() % 7) {
			case 1: {
				if (!erase && i % 3 == 0)
					break;
				size_t pos = seq ? 0 : my_rand() % (t.size() + 1);
				t.insert(pos, my_rand() % 1'000'000, check_tree);
				break;
			}
			case 2:
				if (erase)
					t.erase(my_rand() % t.size(), check_tree);
				break;
			case 3:
				t.assign(my_rand() % t.size(), 155 + i);
				break;
			default:
				t[my_rand() % t.size()];
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

		std::cout << "Bigger random test..." << std::endl;
		test_random(5'000);

		std::cout << "Bigger sequential test..." << std::endl;
		test_random(5'000, SEQ);

		std::cout << "All tests passed." << std::endl;
	} catch (const TestFailed &e) {
		std::cout << "Test failed: " << e.what() << std::endl;
	}
}

#endif

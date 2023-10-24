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

// Note that std::pop_heap and std::push_heap are disabled
#include <algorithm>

#include <deque>
#include <random>
#include <type_traits>
#include <vector>

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

// We use std::multiset as a reference to check our implementation.
// It is not available in progtest :)
#include <set>

// On the other hand, Link (as seen in the harder version of this task)
// is still available in progtest.

#endif

size_t getChildIndex(size_t parent, size_t ith) {
	if (ith > 1) {
		throw std::invalid_argument("Child cannot be anything but 0th or 1st");
	}
	return 2 * parent + 1 + ith;
}

size_t getParentIndex(size_t child) {
	if (child == 0) {
		throw std::invalid_argument("0 is root and thus has no parent");
	}
	return (child - 1) / 2;
}

template <typename T, typename Comp = std::less<T>>
struct BinaryHeap {
private:
	std::vector<T> m_data;
	Comp m_comp;

public:
	BinaryHeap() {}
	explicit BinaryHeap(Comp comp) : m_comp(comp) {}

	bool empty() const {
		return m_data.size() == 0;
	}
	size_t size() const {
		return m_data.size();
	}

	const T &min() const {
		return m_data.at(0); // will throw if empty
	}

	T extract_min() {
		// remove min
		T min = m_data.at(0); // will throw if empty
		std::swap(m_data[0], m_data.back());
		m_data.pop_back();
		// bubble down
		size_t visiting = 0;
		while (getChildIndex(visiting, 0) < m_data.size()) {
			size_t leftChild = getChildIndex(visiting, 0);
			size_t rightChild = getChildIndex(visiting, 1);

			size_t smallerChild = leftChild;
			if (rightChild < m_data.size() && m_comp(m_data[rightChild], m_data[leftChild])) {
				smallerChild = rightChild;
			}

			if (!m_comp(m_data[smallerChild], m_data[visiting])) {
				// parent is smaller or equal than child
				break;
			}
			std::swap(m_data[visiting], m_data[smallerChild]);
			visiting = smallerChild;
		}

		return min;
	}

	void push(const T &&val) {
		size_t visiting = m_data.size();
		m_data.emplace_back(std::move(val));

		// bubble up
		while (visiting != 0) {
			size_t parent = getParentIndex(visiting);
			if (m_comp(m_data[parent], m_data[visiting]) || !m_comp(m_data[visiting], m_data[parent])) {
				// parent is smaller
				break;
			}
			std::swap(m_data[visiting], m_data[parent]);
			visiting = parent;
		}
	}

	// Helpers to enable testing.
	struct TestHelper {
		static const T &index_to_value(const BinaryHeap &H, size_t index) {
			return H.m_data[index];
		}

		static size_t root_index() { return 0; }
		static size_t parent(size_t index) {
			return getParentIndex(index);
		}
		static size_t child(size_t parent, size_t ith) {
			return getChildIndex(parent, ith);
		}
	};
};

#ifndef __PROGTEST__

#define CHECK(cond, ...)                   \
	do {                                   \
		if (!(cond))                       \
			throw Error(fmt(__VA_ARGS__)); \
	} while (0)

template <
	typename T, typename Cmp,
	template <typename, typename> typename Tested_>
class HeapTester {
	struct Node;
	using TElem = const Node *;

	struct CmpInternal {
		const HeapTester *t;

		bool operator()(const Node &a, const Node &b) const { return t->_cmp(a.value, b.value); }
		bool operator()(const TElem &a, const TElem &b) const {
			return t->_cmp(a->value, b->value);
		}
	};

	using It = typename std::multiset<Node, CmpInternal>::const_iterator;

	struct Node {
		T value;
		mutable It rref;

		explicit Node(T value) : value(std::move(value)) {}
		friend class HeapTester;
	};

public:
	struct Error : std::runtime_error {
		using std::runtime_error::runtime_error;
	};

	explicit HeapTester(Cmp cmp = {})
		: _cmp(std::move(cmp)), _ref(CmpInternal{this}), _t(CmpInternal{this}) {}

	bool empty() const {
		CHECK(_t.empty() == _ref.empty(),
			  "_t.empty() == %d, _ref.empty() == %d", _t.empty(), _ref.empty());
		return _t.empty();
	}

	size_t size() const {
		CHECK(_t.size() == _ref.size(),
			  "_t.size() == %zu, _ref.size() == %zu", _t.size(), _ref.size());
		return _t.size();
	}

	void push(T val) {
		It it = _ref.insert(Node(std::move(val)));
		it->rref = it;
		_t.push(&*it);
	}

	const T &min() const {
		const Node *n = _min();
		_check_node(n);
		return n->value;
	}

	T extract_min() {
		const Node *r = _min();
		_check_node(r);
		const Node *t = _t.extract_min();
		CHECK(r == t, "_t.min() != _t.extract_min()");
		return std::move(_ref.extract(r->rref).value().value);
	}

	void check_structure() {
		using Helper = typename decltype(_t)::TestHelper;

		auto node_at_index = [&](size_t i) {
			return Helper::index_to_value(_t, i);
		};

		std::vector<const Node *> seen;
		const size_t root = Helper::root_index();
		const size_t max = root + size();
		for (size_t i = root; i < max; i++) {
			const Node *n = node_at_index(i);
			_check_node(n);
			seen.push_back(n);

			if (i != root)
				CHECK(!_cmp(n->value, node_at_index(Helper::parent(i))->value), "parent > son");

			CHECK(Helper::parent(Helper::child(i, 0)) == i, "n->left->parent != n");
			CHECK(Helper::parent(Helper::child(i, 1)) == i, "n->right->parent != n");
		}

		std::sort(seen.begin(), seen.end());
		CHECK(std::unique(seen.begin(), seen.end()) == seen.end(),
			  "Some node was seen multiple times");
	}

private:
	void _check_node(const Node *n) const {
		CHECK(&*n->rref == n, "Node mismatch (rref)");
	}

	const Node *_min() const {
		const Node &rmin = *_ref.begin();
		const Node *tmin = _t.min();
		CHECK(!_cmp(rmin.value, tmin->value), "_ref.min() < _t.min()");
		CHECK(!_cmp(tmin->value, rmin.value), "_t.min() < _ref.min()");
		return tmin;
	}

	Cmp _cmp;
	std::multiset<Node, CmpInternal> _ref;
	Tested_<TElem, CmpInternal> _t;
};

#undef CHECK

class testTypeName {
private:
	int m_data;

public:
	testTypeName(int data) : m_data(data) {}
	testTypeName(const testTypeName &) = delete;
	testTypeName(testTypeName &&t) { std::swap(m_data, t.m_data); }
	~testTypeName() {}

	bool operator<(const testTypeName &right) const {
		return this->m_data < right.m_data;
	}
};

template <typename Heap>
void run_test(int max, bool check_structure = false) {
	Heap H;

	auto check = [&]() {
		if (check_structure)
			H.check_structure();
	};

	for (int i = 0; i < max; i++) {
		H.push((i * 991) % (5 * max));
		check();
	}

	for (int i = 0; i < 2 * max; i++) {
		H.empty();
		H.extract_min();
		check();
		H.push((i * 991) % (7 * max));
		check();
	}
}

template <typename T, typename Cmp = std::less<T>>
using Tester = HeapTester<T, Cmp, BinaryHeap>;

int main() {
	{
		BinaryHeap<int> H;

		try {
			H.min();
			throw TestFailed("min on empty heap did not throw");
		} catch (const std::out_of_range &) {
		}

		try {
			H.extract_min();
			throw TestFailed("extract_min on empty heap did not throw");
		} catch (const std::out_of_range &) {
		}
	}

	try {
		std::cout << "My test..." << std::endl;
		run_test<Tester<testTypeName>>(20);
		std::cout << "Small test..." << std::endl;
		run_test<Tester<int>>(20);
		run_test<Tester<int>>(20, true);
		run_test<Tester<int>>(1'000, true);

		std::cout << "Big test..." << std::endl;
		run_test<Tester<int>>(500'000);

		std::cout << "All tests passed." << std::endl;
	} catch (const TestFailed &) {
	}
}

#endif

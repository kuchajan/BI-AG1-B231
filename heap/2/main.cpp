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

struct NoCopy {
	NoCopy() = default;
	NoCopy(const NoCopy &) = delete;
	NoCopy &operator=(const NoCopy &) = delete;

	NoCopy(NoCopy &&) = default;
	NoCopy &operator=(NoCopy &&) = default;
};

struct Link : private NoCopy {
	struct Error : std::runtime_error {
		using std::runtime_error::runtime_error;
	};

	Link() = default;
	~Link() { unlink(); }

	Link(Link &&o) { swap(o); }
	Link &operator=(Link &&l) {
		if (other)
			throw Error("Already linked");
		swap(l);
		return *this;
	}

	explicit operator bool() const { return other; }

	void link(Link &l) {
		if (l || *this)
			throw Error("this or l already linked");
		other = &l;
		l.other = this;
	}

	void swap(Link &l) {
		std::swap(other, l.other);
		if (other)
			other->other = this;
		if (l.other)
			l.other->other = &l;
	}

	void unlink() {
		if (other)
			other->other = nullptr;
		other = nullptr;
	}

	Link *get() const { return other; }

private:
	Link *other = nullptr;
};

template <typename T, typename X = T>
struct LinkTo : private Link {
	LinkTo() = default;

	explicit LinkTo(std::remove_const_t<T> &t) {
		link(static_cast<std::remove_const_t<X> &>(t));
	}

	T *get() const {
		if (!*this)
			throw Link::Error("Link does not exist");
		return static_cast<T *>(static_cast<X *>(Link::get()));
	}

	using Link::operator bool;
};

// We use std::multiset as a reference to check our implementation.
// It is not available in progtest :)
#include <set>

template <typename T, typename Cmp>
struct RefHeap {
	RefHeap() = default;
	explicit RefHeap(Cmp cmp = {}) : _data(std::move(cmp)) {}

	bool empty() const { return _data.empty(); }
	size_t size() const { return _data.size(); }

	const T &min() const { return *_data.begin(); }

	struct Ref : private NoCopy {
		using It = typename std::multiset<T, Cmp>::const_iterator;

		Ref() = default;
		Ref(It it) : it(it) {}

		const T &operator*() const { return *it; }
		const T *operator->() const { return &operator*(); }

	private:
		friend struct RefHeap;
		mutable It it;
	};

	Ref push(T val) { return {_data.insert(std::move(val))}; }
	T extract_min() { return erase(_data.begin()); }

	template <typename Fun>
	void change(const Ref &ref, Fun f) {
		auto n = _data.extract(ref.it);
		f(n.value());
		ref.it = _data.insert(std::move(n));
	}

	T erase(const Ref &ref) {
		auto n = _data.extract(ref.it);
		return std::move(n.value());
	}

private:
	std::multiset<T, Cmp> _data;
};

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
class BinaryHeap {
private:
	struct Node : Link {
		Node(T value, size_t index) : value(std::move(value)), index(index) {}
		T value;
		size_t index;
	};
	std::vector<Node> m_data;
	Comp m_comp;

	void mySwap(size_t index1, size_t index2) {
		m_data[index1].index = index2;
		m_data[index2].index = index1;
		std::swap(m_data[index1], m_data[index2]);
	}

public:
	BinaryHeap() {}
	explicit BinaryHeap(Comp comp) : m_comp(comp) {}

	bool empty() const;
	size_t size() const;

	const T &min() const;
	T extract_min();

	struct Ref : private LinkTo<const Node> {
		using LinkTo<const Node>::LinkTo;
		const T &operator*() const { return this->get()->value; }
		const T *operator->() const { return &operator*(); }
		friend struct BinaryHeap;
	};

	Ref push(T val);

	// Joined increase / decrease method.
	// Changes key represented by ref to f(key)
	//
	// Functor f should be called with a *non-const* reference
	// to the value of ref as argument. This reference must not
	// escape f. After f returns, the heap should check how
	// the value changed and fix its structure accordingly.
	template <typename Fun>
	void change(const Ref &ref, Fun f);

	// Remove the value referenced by ref from the heap.
	T erase(const Ref &ref);

	// Helpers to enable testing.
	struct TestHelper {
		static const T &index_to_value(const BinaryHeap &H, size_t index);

		static size_t root_index() { return 0; }
		static size_t parent(size_t index);
		static size_t child(size_t parent, size_t ith);
	};

private:
	// Convert ref value to index in heap (not mandatory for tests
	// but it might help with implementation of other methods).
	size_t ref_to_index(const Ref &ref) const;
};

#ifndef __PROGTEST__

#define CHECK(cond, ...)                   \
	do {                                   \
		if (!(cond))                       \
			throw Error(fmt(__VA_ARGS__)); \
	} while (0)

template <
	typename T, typename Cmp,
	template <typename, typename> typename Tested_,
	template <typename, typename> typename Ref_>
class HeapTester {
	struct Node;
	struct RefLink : Link {};
	struct TLink : Link {};
	using TElem = LinkTo<Node, TLink>;

	struct CmpInternal {
		const HeapTester *t;

		bool operator()(const Node &a, const Node &b) const { return t->_cmp(a.value, b.value); }
		bool operator()(const TElem &a, const TElem &b) const {
			return t->_cmp(a.get()->value, b.get()->value);
		}
	};

	using THeap = Tested_<TElem, CmpInternal>;
	using TRef = typename THeap::Ref;

	using RHeap = Ref_<Node, CmpInternal>;
	using RRef = typename RHeap::Ref;

	struct Node : RefLink, TLink {
		T value;
		TRef tref;
		RRef rref;

		explicit Node(T value) : value(std::move(value)) {}
		friend class HeapTester;
	};

public:
	struct Ref : private LinkTo<const Node, const RefLink> {
		using LinkTo<const Node, const RefLink>::LinkTo;
		using LinkTo<const Node, const RefLink>::operator bool;
		const T &operator*() const { return this->get()->value; }
		const T *operator->() const { return &operator*(); }
		friend class HeapTester;
	};

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

	Ref push(T val) {
		Node node{val};
		Ref ref{node};
		node.tref = _t.push(TElem(node));
		const_cast<RRef &>(ref.get()->rref) = _ref.push(std::move(node));
		return ref;
	}

	const T &min() const {
		const Node *n = _min().get();
		_check_node(n);
		return n->value;
	}

	T extract_min() {
		const Node *r = _min();
		_check_node(r);
		TElem t = _t.extract_min();
		CHECK(r == t.get(), "_t.min() != _t.extract_min()");
		return _ref_erase(r);
	}

	template <typename Fun>
	void change(const Ref &ref, Fun f) {
		_check_node(ref.get());
		size_t cnt = 0;
		_ref.change(ref.get()->rref, [&](Node &rt) {
			_t.change(ref.get()->tref, [&](TElem &tt) {
				_check_node(ref.get());
				CHECK(&rt == ref.get(), "change: rt != ref");
				CHECK(tt.get() == ref.get(), "change: tt != ref");
				cnt++;
				f(rt.value);
			});
		});

		CHECK(cnt == 1, "change: f was not called once but %zu times", cnt);
	}

	T erase(const Ref &ref) {
		_check_node(ref.get());
		TElem t = _t.erase(ref.get()->tref);
		CHECK(t.get() == ref.get(), "erase: t != ref");
		return _ref_erase(t.get());
	}

	void check_structure() {
		using Helper = typename decltype(_t)::TestHelper;

		auto node_at_index = [&](size_t i) {
			return Helper::index_to_value(_t, i).get();
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
		CHECK((*n->tref).get() == n, "Node mismatch (tref)");
	}

	const Node *_min() const {
		const Node &rmin = _ref.min();
		const TElem &tmin = _t.min();
		CHECK(!_cmp(rmin.value, tmin.get()->value), "_ref.min() < _t.min()");
		CHECK(!_cmp(tmin.get()->value, rmin.value), "_t.min() < _ref.min()");
		return tmin.get();
	}

	T _ref_erase(const Node *n) {
		return _ref.erase(n->rref).value;
	}

	Cmp _cmp;
	RHeap _ref;
	THeap _t;
};

#undef CHECK

namespace test_flags {
	enum TestFlags : unsigned {
		CHANGE = 1,
		ERASE = 2,
		CHECK_STRUCTURE = 4
	};
}

template <typename Heap>
void run_test(int max, unsigned flags = 0) {
	using namespace test_flags;

	Heap H;
	std::vector<typename Heap::Ref> refs;

	auto check = [&]() {
		if (flags & CHECK_STRUCTURE)
			H.check_structure();
	};

	for (int i = 0; i < max; i++) {
		refs.emplace_back(H.push((i * 991) % (5 * max)));
		check();
	}

	if (flags & CHANGE) {
		for (int i = 0; i < max; i++) {
			const auto &ref = refs[(i * 17) % refs.size()];

			if (ref)
				H.change(ref, [&](int &v) {
					v += (i * 31) % 100 - 50;
				});

			if (i % 30 == 0)
				H.extract_min();
			if (i % 13 == 0)
				refs.emplace_back(H.push(i / 13 + 11));
			check();
		}

		for (int i = 0; i < (max / 2); i++) {
			const auto &ref = refs[(i * 17 * 3) % refs.size()];

			if (ref)
				H.change(ref, [&](int &v) {
					v += (i * 31) % 90 - 40;
				});

			if (i % 66 == 0)
				H.extract_min();
			check();
		}
	}

	if (flags & ERASE)
		for (int i = 0; i < (max / 4); i++) {
			H.size();
			if (refs.back())
				H.erase(refs.back());
			check();
			refs.pop_back();
		}

	for (int i = 0; i < (max / 2); i++) {
		H.empty();
		H.extract_min();
		check();
	}
}

template <typename T, typename Cmp = std::less<T>>
using Tester = HeapTester<T, Cmp, BinaryHeap, RefHeap>;

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
		using namespace test_flags;
		std::cout << "No change or erase..." << std::endl;
		run_test<Tester<int>>(20);
		run_test<Tester<int>>(20, CHECK_STRUCTURE);
		run_test<Tester<int>>(10'000);

		std::cout << "With change but no erase..." << std::endl;
		run_test<Tester<int>>(20, CHANGE);
		run_test<Tester<int>>(20, CHANGE | CHECK_STRUCTURE);
		run_test<Tester<int>>(10'000, CHANGE);

		std::cout << "With change and erase..." << std::endl;
		run_test<Tester<int>>(20, CHANGE | ERASE);
		run_test<Tester<int>>(20, CHANGE | ERASE | CHECK_STRUCTURE);
		run_test<Tester<int>>(200, CHANGE | ERASE | CHECK_STRUCTURE);
		run_test<Tester<int>>(10'000, CHANGE | ERASE);

		std::cout << "Big test..." << std::endl;
		run_test<Tester<int>>(500'000, CHANGE | ERASE);

		std::cout << "All tests passed." << std::endl;
	} catch (const TestFailed &) {
	}
}

#endif

#ifndef __PROGTEST__
#include <algorithm>
#include <array>
#include <bitset>
#include <cassert>
#include <cstdint>
#include <deque>
#include <iomanip>
#include <iostream>
#include <limits>
#include <list>
#include <memory>
#include <optional>
#include <queue>
#include <random>
#include <stack>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#endif

struct TextEditorBackend {
	struct Node {
		// * tree structure variables
		// neighbours
		Node *m_parent;
		Node *m_leftChild;
		Node *m_rightChild;
		// avl variables
		size_t m_height;
		size_t m_size;
		size_t m_lineCount;
		// * node value variables
		char m_value;

		Node(char value) : m_value(value) {
			// sanity check
			m_parent = nullptr;
			m_leftChild = nullptr;
			m_rightChild = nullptr;
			m_height = 0;
			m_size = 1;
			m_lineCount = m_value == '\n' ? 1 : 0;
		}

		ssize_t getSign() const {
			return (m_rightChild != nullptr ? (ssize_t)(m_rightChild->m_height + 1) : (ssize_t)0) - (m_leftChild != nullptr ? (ssize_t)(m_leftChild->m_height + 1) : (ssize_t)0);
		}

		void calculateNewHeight() {
			m_height = 0;
			m_size = 1;
			m_lineCount = m_value == '\n' ? 1 : 0;
			// it is expected that children's height is already calculated
			if (m_leftChild) {
				m_height = m_leftChild->m_height + 1;
				m_size += m_leftChild->m_size;
				m_lineCount += m_leftChild->m_lineCount;
			}
			if (m_rightChild) {
				m_height = m_height < m_rightChild->m_height + 1 ? m_rightChild->m_height + 1 : m_height;
				m_size += m_rightChild->m_size;
				m_lineCount += m_rightChild->m_lineCount;
			}
		}

		void swapNodes(Node &other) {
			std::swap(m_value, other.m_value);
		}
	};
	Node *m_root;

	TextEditorBackend(const std::string &text) {
		// naive implementation
		m_root = nullptr;
		for (char ch : text)
			insert(size(), ch);
	}

	size_t getSize(Node *n) const {
		return n ? n->m_size : 0;
	}
	size_t getLineCount(Node *n) const {
		return n ? n->m_lineCount : 0;
	}
	size_t size() const {
		return getSize(m_root);
	}
	size_t lines() const {
		return getLineCount(m_root) + 1;
	}

	Node *findMin(Node *n) const {
		while (n && n->m_leftChild) {
			n = n->m_leftChild;
		}
		return n;
	}

	Node *findMax(Node *n) const {
		while (n && n->m_rightChild) {
			n = n->m_rightChild;
		}
		return n;
	}

	Node *find(size_t index) const {
		if (index >= size()) {
			throw std::out_of_range("Index does not exist");
		}
		Node *visiting = m_root;
		while (visiting) {
			if (index == 0) {
				return findMin(visiting);
			}
			if ((index + 1) >= visiting->m_size) {
				return findMax(visiting);
			}
			if (index == getSize(visiting->m_leftChild)) {
				return visiting;
			}
			if (index < getSize(visiting->m_leftChild)) {
				visiting = visiting->m_leftChild;
			} else {
				index -= getSize(visiting->m_leftChild) + 1;
				visiting = visiting->m_rightChild;
			}
		}
		throw std::logic_error("Loop exited without finding the index");
	}

	char at(size_t i) const {
		return find(i)->m_value;
	}
	void edit(size_t i, char c) {
		Node *toEdit = find(i);
		bool newLineEdit = (toEdit->m_value == '\n' || c == '\n') && toEdit->m_value != c;
		toEdit->m_value = c;
		if (newLineEdit) {
			Node *visiting = toEdit;
			while (visiting) {
				visiting->m_lineCount += c == '\n' ? 1 : -1;
				visiting = visiting->m_parent;
			}
		}
	}

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

	void balance(Node *visiting) {
		while (visiting) {
			// calculate new height
			visiting->calculateNewHeight();
			if (visiting->getSign() < -1) {
				if (visiting->m_leftChild && visiting->m_leftChild->getSign() == 1) {
					leftRotate(visiting->m_leftChild);
				}
				rightRotate(visiting);
			} else if (visiting->getSign() > 1) {
				if (visiting->m_rightChild && visiting->m_rightChild->getSign() == -1) {
					rightRotate(visiting->m_rightChild);
				}
				leftRotate(visiting);
			}
			visiting = visiting->m_parent;
		}
	}

	void insert(size_t index, char value) {
		// setup insertion and sub-methods
		if (index > size())
			throw std::out_of_range("Index is not inside [0, size()]");
		Node *toInsert = new Node(value);
		if (!m_root) {
			m_root = toInsert;
			return;
		}
		Node *visiting = m_root;
		bool isLeft = true;
		// insertion recursive loop
		while (visiting) {
			toInsert->m_parent = visiting;
			isLeft = index <= getSize(visiting->m_leftChild);
			index = isLeft ? index : index - (getSize(visiting->m_leftChild) + 1);
			visiting = isLeft ? visiting->m_leftChild : visiting->m_rightChild;
		}
		if (isLeft) {
			toInsert->m_parent->m_leftChild = toInsert;
		} else {
			toInsert->m_parent->m_rightChild = toInsert;
		}

		balance(toInsert->m_parent);
	}

	void eraseSubMethod(Node *toDelete, Node *subChild) {
		// set toDelete's parent as parent of toDelete's only child (if it exists)
		if (subChild) {
			subChild->m_parent = toDelete->m_parent;
		}
		// if parent exists, set it's child as this child
		if (toDelete->m_parent) {
			if (toDelete->m_parent->m_leftChild == toDelete) {
				toDelete->m_parent->m_leftChild = subChild;
			} else {
				toDelete->m_parent->m_rightChild = subChild;
			}
		} else { // parent doesn't exist - toDelete is root - set this child as root
			m_root = subChild;
		}
	}
	void erase(size_t index) {
		// find the node
		Node *toDelete = find(index);
		// case 1 - this node is not in tree - doesn't happen

		if (toDelete->m_leftChild && toDelete->m_rightChild) {
			// case 4 - this node has two children
			Node *min = findMin(toDelete->m_rightChild);
			toDelete->swapNodes(*min);
			toDelete = min;
		}

		if (!toDelete->m_leftChild && !toDelete->m_rightChild) {
			// case 2 - this node is a leaf
			eraseSubMethod(toDelete, nullptr);
		} else if (toDelete->m_leftChild && !toDelete->m_rightChild) {
			// case 3 - this node has one child
			// case 3.1 - the child is left
			eraseSubMethod(toDelete, toDelete->m_leftChild);
		} else {
			// case 3.2 - the child is right
			eraseSubMethod(toDelete, toDelete->m_rightChild);
		}

		Node *balanceFrom = toDelete->m_parent;
		delete toDelete;
		balance(balanceFrom);
	}

	// Returns the index of the start of the i-th line
	size_t line_start(size_t lineIndex) const {
		if (lineIndex >= lines())
			throw std::out_of_range("Line index is outside [0, lines())");
		// same as find, but with lines instead of size
		Node *visiting = m_root;
		size_t index = getSize(visiting->m_leftChild);
		while (visiting) {
			if (lineIndex == 0) {
				while (visiting->m_leftChild) {
					visiting = visiting->m_leftChild;
					index -= getSize(visiting ? visiting->m_rightChild : nullptr) + 1;
				}
				break;
			}
			if (lineIndex <= getLineCount(visiting->m_leftChild)) { // line is to the left
				// line index is unchanged
				visiting = visiting->m_leftChild;
				index -= getSize(visiting ? visiting->m_rightChild : nullptr) + 1;
			} else { // the line is to the right
				lineIndex -= getLineCount(visiting->m_leftChild) + (visiting->m_value == '\n' ? 1 : 0);
				visiting = visiting->m_rightChild;
				index += getSize(visiting ? visiting->m_leftChild : nullptr) + 1;
			}
		}
		return index;
	}
	// Returns the length of the i-th line, including the newline
	size_t line_length(size_t lineIndex) const {
		if (lineIndex >= lines())
			throw std::out_of_range("Line index is outside [0, lines())");
		return (lineIndex + 1 >= lines() ? size() : line_start(lineIndex + 1)) - line_start(lineIndex);
	}
	// Returns the index of the line that contains the i-th character
	size_t char_to_line(size_t index) const {
		// same as find, but more complicated
		if (index >= size()) {
			throw std::out_of_range("Index does not exist");
		}
		size_t lineCount = 0;
		Node *visiting = m_root;

		while (visiting) {
			if (index == getSize(visiting->m_leftChild)) {
				return lineCount + getLineCount(visiting->m_leftChild); //
			}
			if (index < getSize(visiting->m_leftChild)) {
				visiting = visiting->m_leftChild;
			} else {
				index -= getSize(visiting->m_leftChild) + 1;
				lineCount += getLineCount(visiting->m_leftChild) + (visiting->m_value == '\n' ? 1 : 0);
				visiting = visiting->m_rightChild;
			}
		}
		throw std::logic_error("Loop exited without finding the index");
	}
};

#ifndef __PROGTEST__

////////////////// Dark magic, ignore ////////////////////////

template <typename T>
auto quote(const T &t) { return t; }

std::string quote(const std::string &s) {
	std::string ret = "\"";
	for (char c : s)
		if (c != '\n')
			ret += c;
		else
			ret += "\\n";
	return ret + "\"";
}

#define STR_(a) #a
#define STR(a) STR_(a)

#define CHECK_(a, b, a_str, b_str)                                   \
	do {                                                             \
		auto _a = (a);                                               \
		decltype(a) _b = (b);                                        \
		if (_a != _b) {                                              \
			std::cout << "Line " << __LINE__ << ": Assertion "       \
					  << a_str << " == " << b_str << " failed!"      \
					  << " (lhs: " << quote(_a) << ")" << std::endl; \
			fail++;                                                  \
		} else                                                       \
			ok++;                                                    \
	} while (0)

#define CHECK(a, b) CHECK_(a, b, #a, #b)

#define CHECK_ALL(expr, ...)                                                    \
	do {                                                                        \
		std::array _arr = {__VA_ARGS__};                                        \
		for (size_t _i = 0; _i < _arr.size(); _i++)                             \
			CHECK_((expr)(_i), _arr[_i], STR(expr) "(" << _i << ")", _arr[_i]); \
	} while (0)

#define CHECK_EX(expr, ex)                                                                                                             \
	do {                                                                                                                               \
		try {                                                                                                                          \
			(expr);                                                                                                                    \
			fail++;                                                                                                                    \
			std::cout << "Line " << __LINE__ << ": Expected " STR(expr) " to throw " #ex " but no exception was raised." << std::endl; \
		} catch (const ex &) {                                                                                                         \
			ok++;                                                                                                                      \
		} catch (...) {                                                                                                                \
			fail++;                                                                                                                    \
			std::cout << "Line " << __LINE__ << ": Expected " STR(expr) " to throw " #ex " but got different exception." << std::endl; \
		}                                                                                                                              \
	} while (0)

////////////////// End of dark magic ////////////////////////

std::string text(const TextEditorBackend &t) {
	std::string ret;
	for (size_t i = 0; i < t.size(); i++)
		ret.push_back(t.at(i));
	return ret;
}

void test1(int &ok, int &fail) {
	TextEditorBackend s("123\n456\n789");
	CHECK(s.size(), 11);
	CHECK(text(s), "123\n456\n789");
	CHECK(s.lines(), 3);
	CHECK_ALL(s.char_to_line, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2);
	CHECK_ALL(s.line_start, 0, 4, 8);
	CHECK_ALL(s.line_length, 4, 4, 3);
}

void test2(int &ok, int &fail) {
	TextEditorBackend t("123\n456\n789\n");
	CHECK(t.size(), 12);
	CHECK(text(t), "123\n456\n789\n");
	CHECK(t.lines(), 4);
	CHECK_ALL(t.char_to_line, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2);
	CHECK_ALL(t.line_start, 0, 4, 8, 12);
	CHECK_ALL(t.line_length, 4, 4, 4, 0);
}

void test3(int &ok, int &fail) {
	TextEditorBackend t("asdfasdfasdf");

	CHECK(t.size(), 12);
	CHECK(text(t), "asdfasdfasdf");
	CHECK(t.lines(), 1);
	CHECK_ALL(t.char_to_line, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
	CHECK(t.line_start(0), 0);
	CHECK(t.line_length(0), 12);

	t.insert(0, '\n');
	CHECK(t.size(), 13);
	CHECK(text(t), "\nasdfasdfasdf");
	CHECK(t.lines(), 2);
	CHECK_ALL(t.line_start, 0, 1);

	t.insert(4, '\n');
	CHECK(t.size(), 14);
	CHECK(text(t), "\nasd\nfasdfasdf");
	CHECK(t.lines(), 3);
	CHECK_ALL(t.line_start, 0, 1, 5);

	t.insert(t.size(), '\n');
	CHECK(t.size(), 15);
	CHECK(text(t), "\nasd\nfasdfasdf\n");
	CHECK(t.lines(), 4);
	CHECK_ALL(t.line_start, 0, 1, 5, 15);

	t.edit(t.size() - 1, 'H');
	CHECK(t.size(), 15);
	CHECK(text(t), "\nasd\nfasdfasdfH");
	CHECK(t.lines(), 3);
	CHECK_ALL(t.line_start, 0, 1, 5);

	t.erase(8);
	CHECK(t.size(), 14);
	CHECK(text(t), "\nasd\nfasfasdfH");
	CHECK(t.lines(), 3);
	CHECK_ALL(t.line_start, 0, 1, 5);

	t.erase(4);
	CHECK(t.size(), 13);
	CHECK(text(t), "\nasdfasfasdfH");
	CHECK(t.lines(), 2);
	CHECK_ALL(t.line_start, 0, 1);
}

void test_ex(int &ok, int &fail) {
	TextEditorBackend t("123\n456\n789\n");
	CHECK_EX(t.at(12), std::out_of_range);

	CHECK_EX(t.insert(13, 'a'), std::out_of_range);
	CHECK_EX(t.edit(12, 'x'), std::out_of_range);
	CHECK_EX(t.erase(12), std::out_of_range);

	CHECK_EX(t.line_start(4), std::out_of_range);
	CHECK_EX(t.line_start(40), std::out_of_range);
	CHECK_EX(t.line_length(4), std::out_of_range);
	CHECK_EX(t.line_length(6), std::out_of_range);
	CHECK_EX(t.char_to_line(12), std::out_of_range);
	CHECK_EX(t.char_to_line(25), std::out_of_range);
}

char randChar(std::mt19937 &rand) {
	const char chars[11] = "123456789\n";
	return chars[rand() % 10];
}

void myTest(size_t size = 1'000'000) {
	std::mt19937 my_rand(24707 + size);
	std::string ref;
	TextEditorBackend t(ref);
	for (size_t i = 0; i < size; ++i) {
		switch (my_rand() % 5) {
			case 0: {
				// erase
				if (!ref.empty()) {
					size_t where = my_rand() % (ref.length());
					ref.erase(where, 1);
					t.erase(where);
				}
				break;
			}
			case 1: {
				// edit
				if (!ref.empty()) {
					size_t where = my_rand() % (ref.length());
					char what = randChar(my_rand);
					ref[where] = what;
					t.edit(where, what);
				}
				break;
			}
			default: {
				size_t where = my_rand() % (ref.length() + 1);
				char what = randChar(my_rand);
				ref.insert(ref.begin() + where, what);
				t.insert(where, what);
				break;
			}
		}
		if ((std::count(ref.begin(), ref.end(), '\n') + 1) != t.lines()) {
			throw std::runtime_error("found mismatch in lines");
		}
		if (ref.size() != t.size()) {
			throw std::runtime_error("found mismatch in size");
		}
	}
}

int main() {
	int ok = 0, fail = 0;
	if (!fail)
		test1(ok, fail);
	if (!fail)
		test2(ok, fail);
	if (!fail)
		test3(ok, fail);
	if (!fail)
		test_ex(ok, fail);

	if (!fail)
		std::cout << "Passed all " << ok << " tests!" << std::endl;
	else
		std::cout << "Failed " << fail << " of " << (ok + fail) << " tests." << std::endl;

	myTest();
}

#endif

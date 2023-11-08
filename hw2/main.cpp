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
		return getLineCount(m_root);
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
		find(i)->m_value = c;
	}
	void insert(size_t i, char c);
	void erase(size_t i);

	size_t line_start(size_t r) const;
	size_t line_length(size_t r) const;
	size_t char_to_line(size_t i) const;
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
}

#endif

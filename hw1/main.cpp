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
#include <map>
#include <memory>
#include <optional>
#include <queue>
#include <random>
#include <set>
#include <stack>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <vector>

enum Point : size_t {};

struct Path {
	Point from, to;
	unsigned length;

	Path(size_t f, size_t t, unsigned l) : from{f}, to{t}, length{l} {}

	friend bool operator==(const Path &a, const Path &b) {
		return std::tie(a.from, a.to, a.length) == std::tie(b.from, b.to, b.length);
	}

	friend bool operator!=(const Path &a, const Path &b) { return !(a == b); }
};

#endif

struct MyPath {
	Point m_to;
	unsigned m_length;
	MyPath(size_t to, unsigned length) : m_to{to}, m_length(length) {}
};

class Graph {
private:
	size_t m_size;
	std::map<Point, std::vector<MyPath>> m_G;
	std::map<Point, std::vector<MyPath>> m_GInverse;

public:
	Graph(size_t size, const std::vector<Path> &paths) : m_size(size) {
		for (Path p : paths) {
			if (m_G.count(p.from) == 0) {
				m_G.insert(p.from, {});
				m_GInverse.insert(p.from, {});
			}
			if (m_G.count(p.to) == 0) {
				m_G.insert(p.to, {});
				m_GInverse.insert(p.to, {});
			}

			m_G.find(p.from)->second.push_back({p.to, p.length});
			m_GInverse.find(p.to)->second.push_back({p.from, p.length});
		}
	}

	const size_t getSize() {
		return m_size;
	}
};

std::vector<Path> longest_track(size_t points, const std::vector<Path> &all_paths) {
	// construct graph
	Graph g(points, all_paths);
}

#ifndef __PROGTEST__

struct Test {
	unsigned longest_track;
	size_t points;
	std::vector<Path> all_paths;
};

inline const Test TESTS[] = {
	{13, 5, {{3, 2, 10}, {3, 0, 9}, {0, 2, 3}, {2, 4, 1}}},
	{11, 5, {{3, 2, 10}, {3, 1, 4}, {1, 2, 3}, {2, 4, 1}}},
	{16, 8, {{3, 2, 10}, {3, 1, 1}, {1, 2, 3}, {1, 4, 15}}},
};

#define CHECK(cond, ...)              \
	do {                              \
		if (cond)                     \
			break;                    \
		printf("Fail: " __VA_ARGS__); \
		printf("\n");                 \
		return false;                 \
	} while (0)

bool run_test(const Test &t) {
	auto sol = longest_track(t.points, t.all_paths);

	unsigned length = 0;
	for (auto [_, __, l] : sol)
		length += l;

	CHECK(t.longest_track == length,
		  "Wrong length: got %u but expected %u", length, t.longest_track);

	for (size_t i = 0; i < sol.size(); i++) {
		CHECK(std::count(t.all_paths.begin(), t.all_paths.end(), sol[i]),
			  "Solution contains non-existent path: %zu -> %zu (%u)",
			  sol[i].from, sol[i].to, sol[i].length);

		if (i > 0)
			CHECK(sol[i].from == sol[i - 1].to,
				  "Paths are not consecutive: %zu != %zu", sol[i - 1].to, sol[i].from);
	}

	return true;
}
#undef CHECK

int main() {
	int ok = 0, fail = 0;

	for (auto &&t : TESTS)
		(run_test(t) ? ok : fail)++;

	if (!fail)
		printf("Passed all %i tests!\n", ok);
	else
		printf("Failed %u of %u tests.\n", fail, fail + ok);
}

#endif

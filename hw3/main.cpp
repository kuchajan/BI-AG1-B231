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

using Price = unsigned long long;
using Employee = size_t;
inline constexpr Employee NO_EMPLOYEE = -1;
using Gift = size_t;

#endif

// I will call the employee with no boss a supreme boss
// Each employee has only one or no boss
// 1. If the employee has no boss, he is the supreme boss
// 2. If the employee has one boss, he also has only one supreme boss, and that is either his boss, or his boss's boss (since his boss can only have one boss), or his boss's boss's boss, and so on
// 3. An employee cannot have more than one boss
// No employee is their own boss, not even inderectly, and each employee has only one boss <=> graph of employees has no cycles <=> graph is a forest
// I do not know if every graph of employees given is connected, so I will assume the worst; it is not, and thus graph is not a tree, but only a forest
// However, each connected component is by definition connected, and since the original graph has no cycles, it's connected components also have no cycles
// So, each connected component is a tree
// My algorithm will first divide the graph into trees, and then will compute the minimal sum coloring for each tree.
// The minimal sum coloring for a tree is correct
// * source: Kubicka, Ewa; Schwenk, Allen J. (1989), "An introduction to chromatic sums", Proceedings of the 17th ACM Computer Science Conference (CSC '89), New York, NY, USA: ACM, pp. 39–45, doi:10.1145/75427.75430, ISBN 978-0-89791-299-0, S2CID 28544302
// * https://dl.acm.org/doi/10.1145/75427.75430
// The sum of these minimal sums will be the cheapest way to give presents to the employees.

struct Vertex {
	Employee parent;
	Price minSum;
	Gift currentColor;
	Price delta;
	Gift nextColor;
	std::vector<size_t> coloradd; // the increase in the total of the chromatic sums TREE[I].MINSUM for all the sons I of a given vertex v when we insist on coloring vertex v with color K.
	Vertex(const Employee &emp, size_t colorCount) : parent(emp) {
		minSum = 0;
		currentColor = 0;
		delta = 0;
		nextColor = 0;
		coloradd.resize(colorCount);
	}
};

std::pair<Price, std::vector<Gift>> optimize_gifts(const std::vector<Employee> &boss, const std::vector<Price> &gift_price) {
}

#ifndef __PROGTEST__

const std::tuple<Price, std::vector<Employee>, std::vector<Price>> EXAMPLES[] = {
	{17, {1, 2, 3, 4, NO_EMPLOYEE}, {25, 4, 18, 3}},
	{16, {4, 4, 4, 4, NO_EMPLOYEE}, {25, 4, 18, 3}},
	{17, {4, 4, 3, 4, NO_EMPLOYEE}, {25, 4, 18, 3}},
	{24, {4, 4, 3, 4, NO_EMPLOYEE, 3, 3}, {25, 4, 18, 3}},
};

#define CHECK(cond, ...)                     \
	do {                                     \
		if (cond)                            \
			break;                           \
		printf("Test failed: " __VA_ARGS__); \
		printf("\n");                        \
		return false;                        \
	} while (0)

bool test(Price p, const std::vector<Employee> &boss, const std::vector<Price> &gp) {
	auto &&[sol_p, sol_g] = optimize_gifts(boss, gp);
	CHECK(sol_g.size() == boss.size(),
		  "Size of the solution: expected %zu but got %zu.", boss.size(), sol_g.size());

	Price real_p = 0;
	for (Gift g : sol_g)
		real_p += gp[g];
	CHECK(real_p == sol_p, "Sum of gift prices is %llu but reported price is %llu.", real_p, sol_p);

	if (0) {
		for (Employee e = 0; e < boss.size(); e++)
			printf(" (%zu)%zu", e, sol_g[e]);
		printf("\n");
	}

	for (Employee e = 0; e < boss.size(); e++)
		CHECK(boss[e] == NO_EMPLOYEE || sol_g[boss[e]] != sol_g[e],
			  "Employee %zu and their boss %zu has same gift %zu.", e, boss[e], sol_g[e]);

	CHECK(p == sol_p, "Wrong price: expected %llu got %llu.", p, sol_p);

	return true;
}
#undef CHECK

int main() {
	int ok = 0, fail = 0;
	for (auto &&[p, b, gp] : EXAMPLES)
		(test(p, b, gp) ? ok : fail)++;

	if (!fail)
		printf("Passed all %d tests!\n", ok);
	else
		printf("Failed %d of %d tests.", fail, fail + ok);
}

#endif

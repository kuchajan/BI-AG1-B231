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

class Graph {
private:
	std::vector<Employee> m_roots;						  // indexes of roots of trees
	std::vector<Vertex> m_vertices;						  // all vertices and important information for the minimal sum coloring of a tree algorithm
	std::map<Employee, std::vector<Employee>> m_children; // directed edges to children
	std::vector<std::pair<Gift, Price>> m_gifts;		  // available gifts/colors

	std::vector<Employee> getPostorder(Employee root) {
		std::vector<Employee> preorder;
		std::stack<Employee> toVisit;

		toVisit.push(root);
		while (!toVisit.empty()) {
			Employee visiting = toVisit.top();
			toVisit.pop();
			preorder.push_back(visiting);
			for (Employee child : m_children[visiting]) {
				toVisit.push(child);
			}
		}

		std::reverse(preorder.begin(), preorder.end());
		return preorder;
	}

	void colorTree(Employee root) {
		std::vector<Employee> order = getPostorder(root);

		for (size_t i = 0; i < order.size(); ++i) {
			Employee visiting = order[i];
			if (m_children[visiting].size() == 0) { // base case
				m_vertices[visiting].currentColor = 0;
				m_vertices[visiting].nextColor = 1;
				m_vertices[visiting].minSum = m_gifts[0].second;
				m_vertices[visiting].delta = m_gifts[1].second - m_gifts[0].second;
				continue;
			}

			for (size_t k = 0; k < m_children[visiting].size() + 2; ++k) {
				m_vertices[visiting].coloradd[k] = m_gifts[k].second;
			}

			size_t minTotal = 0;
			for (Employee emp : m_children[visiting]) {
				minTotal += m_vertices[emp].minSum;
				m_vertices[visiting].coloradd[m_vertices[emp].currentColor] += m_vertices[emp].delta;
			}
			size_t sum1 = 0;
			size_t sum2 = 0;
			bool sum1Inf = true;
			bool sum2Inf = true;
			size_t color1 = 0;
			size_t color2 = 0;
			for (size_t k = 0; k < m_children[visiting].size() + 2; ++k) {
				size_t value = m_vertices[visiting].coloradd[k];
				if (sum1Inf || value < sum1) {
					color2 = color1;
					sum2 = sum1;
					if (!sum1Inf) {
						sum2Inf = false;
					}
					color1 = k;
					sum1 = value;
					sum1Inf = false;
				} else if (sum2Inf || value < sum2) {
					color2 = k;
					sum2 = value;
					sum2Inf = false;
				}
			}
			m_vertices[visiting].currentColor = color1;
			m_vertices[visiting].nextColor = color2;
			m_vertices[visiting].minSum = sum1 + minTotal;
			m_vertices[visiting].delta = sum2 - sum1;
		}
	}

public:
	Graph(const std::vector<Employee> &boss, const std::vector<Price> &gift_price) {
		// Initialize graph
		for (Employee emp = 0; emp < boss.size(); ++emp) {
			m_vertices.emplace_back(boss[emp], gift_price.size());
			if (m_children.find(emp) == m_children.end()) {
				m_children.insert({emp, {}});
			}
			if (boss[emp] == NO_EMPLOYEE) {
				m_roots.push_back(emp);
				continue;
			}
			m_children[boss[emp]].push_back(emp);
		}
		// Initialize gifts
		for (Gift gift = 0; gift < gift_price.size(); ++gift) {
			m_gifts.push_back({gift, gift_price[gift]});
		}
		std::sort(m_gifts.begin(), m_gifts.end(), [](std::pair<Gift, Price> a, std::pair<Gift, Price> b) { return a.second < b.second; });
	}

	void colorGraph() {
		for (Employee root : m_roots) {
			colorTree(root);
		}
	}

	std::pair<Price, std::vector<Gift>> getResult() const {
		Price acc = 0;
		for (Employee emp : m_roots) {
			acc += m_vertices[emp].minSum;
		}

		std::vector<Gift> gifts;
		for (Vertex v : m_vertices) {
			gifts.push_back(m_gifts[v.currentColor].first);
		}

		return {acc, gifts};
	}
};

std::pair<Price, std::vector<Gift>> optimize_gifts(const std::vector<Employee> &boss, const std::vector<Price> &gift_price) {
	Graph g(boss, gift_price);
	g.colorGraph();
	return g.getResult();
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

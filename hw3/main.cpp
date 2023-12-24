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
// The minimal sum coloring for a tree is correct and heavily inspired from the following source
// * source: Kubicka, Ewa; Schwenk, Allen J. (1989), "An introduction to chromatic sums", Proceedings of the 17th ACM Computer Science Conference (CSC '89), New York, NY, USA: ACM, pp. 39–45, doi:10.1145/75427.75430, ISBN 978-0-89791-299-0, S2CID 28544302
// * https://dl.acm.org/doi/10.1145/75427.75430
// The sum of these minimal sums will be the cheapest way to give presents to the employees.

struct Vertex {
	Employee parent;
	Gift totalBestColor;

	// two colorings, in case it is better for the parent to use this node's first best color
	Gift firstBestColor;
	Price minSum;
	Gift secondBestColor;
	Price minSum2;

	Vertex(const Employee &emp) : parent(emp) {
		minSum = 0;
		firstBestColor = 0;
		minSum2 = 0;
		secondBestColor = 0;
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
		std::vector<Employee> order = getPostorder(root); // linear traversal of entire tree, leaves first, then their parents, etc.

		for (size_t i = 0; i < order.size(); ++i) {
			Employee visiting = order[i];
			if (m_children[visiting].size() == 0) { // base case
				// since the gift array is sorted, the best two colorings must be these
				m_vertices[visiting].firstBestColor = 0;
				m_vertices[visiting].secondBestColor = 1;
				m_vertices[visiting].minSum = m_gifts[0].second;
				m_vertices[visiting].minSum2 = m_gifts[1].second;
				continue;
			}

			std::vector<Price> allColoring; // what would the minimal sum of sub-tree be if we insist on coloring this vertex with a color
			for (size_t k = 0; k < m_gifts.size(); ++k) {
				Price p = m_gifts[k].second;
				for (Employee emp : m_children[visiting]) {
					p += (m_vertices[emp].firstBestColor != k) ? m_vertices[emp].minSum : m_vertices[emp].minSum2;
				}
				allColoring.push_back(p);
			}

			// get the two minimums of this sub-tree
			size_t pos1 = 0;
			size_t pos2 = 1;
			size_t min1 = allColoring[0];
			size_t min2 = allColoring[1];
			if (min2 < min1) {
				std::swap(pos1, pos2);
				std::swap(min1, min2);
			}
			for (size_t k = 2; k < m_gifts.size(); ++k) {
				if (allColoring[k] < min1) {
					min2 = min1;
					pos2 = pos1;
					min1 = allColoring[k];
					pos1 = k;
					continue;
				}
				if (allColoring[k] < min2) {
					min2 = allColoring[k];
					pos2 = k;
				}
			}
			m_vertices[visiting].firstBestColor = pos1;
			m_vertices[visiting].minSum = min1;
			m_vertices[visiting].secondBestColor = pos2;
			m_vertices[visiting].minSum2 = min2;
		}

		std::reverse(order.begin(), order.end()); // reverse the postorder to get preorder
		// trace back all results
		for (size_t i = 0; i < order.size(); ++i) {
			Employee visiting = order[i];
			if (m_vertices[visiting].parent == NO_EMPLOYEE) { // since the supreme boss has no bosses, the first best color must be the total best color
				m_vertices[visiting].totalBestColor = m_vertices[visiting].firstBestColor;
				continue;
			}
			// this employee has a boss, so if the boss's total best color is the same as the first best color of this employee, the employee must use the second best color
			// else, the total best color must be the first best color
			m_vertices[visiting].totalBestColor = (m_vertices[visiting].firstBestColor != m_vertices[m_vertices[visiting].parent].totalBestColor) ? m_vertices[visiting].firstBestColor : m_vertices[visiting].secondBestColor;
		}
	}

public:
	Graph(const std::vector<Employee> &boss, const std::vector<Price> &gift_price) {
		// Initialize graph
		for (Employee emp = 0; emp < boss.size(); ++emp) {
			m_vertices.emplace_back(boss[emp]);
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
			gifts.push_back(m_gifts[v.totalBestColor].first);
		}
		std::pair<Price, std::vector<Gift>> toRet = std::make_pair(acc, gifts);
		return toRet;
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

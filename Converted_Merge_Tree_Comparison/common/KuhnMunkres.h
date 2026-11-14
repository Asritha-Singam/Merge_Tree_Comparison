// Kuhn-Munkres (Hungarian) algorithm: minimum-weight perfect matching on a
// complete bipartite graph given as an n x n cost matrix.
//
// Replaces org.jgrapht.alg.matching.KuhnMunkresMinimalWeightBipartitePerfectMatching
// as used in MM(i, j): the Java code always builds a complete square bipartite
// graph (children + dummies on each side), so a dense matrix is sufficient.
//
// The matrix is a flat row-major array of plain doubles: cost[i * n + j] is the
// weight of edge (S[i], T[j]).
//
// Complexity: O(n^3) time, O(n) extra memory.
#ifndef KUHN_MUNKRES_H
#define KUHN_MUNKRES_H

#include <algorithm>
#include <limits>
#include <vector>

struct KuhnMunkresResult {
    double weight = 0.0;          // total cost of the matching (== getMatching().getWeight())
    std::vector<int> rowToCol;    // rowToCol[i] = column matched to row i
};

inline KuhnMunkresResult kuhnMunkresMinWeightPerfectMatching(const std::vector<double>& cost, int n) {
    KuhnMunkresResult result;
    result.rowToCol.assign(n, -1);
    if (n == 0) return result;

    const double INF = std::numeric_limits<double>::infinity();
    // 1-indexed potentials / assignment (standard e-maxx formulation).
    std::vector<double> u(n + 1, 0.0), v(n + 1, 0.0), minv(n + 1);
    std::vector<int> p(n + 1, 0), way(n + 1, 0);
    std::vector<char> used(n + 1);

    for (int i = 1; i <= n; ++i) {
        p[0] = i;
        int j0 = 0;
        std::fill(minv.begin(), minv.end(), INF);
        std::fill(used.begin(), used.end(), 0);
        do {
            used[j0] = 1;
            const int i0 = p[j0];
            const double* row = &cost[static_cast<size_t>(i0 - 1) * n];
            double delta = INF;
            int j1 = 0;
            for (int j = 1; j <= n; ++j) {
                if (used[j]) continue;
                const double cur = row[j - 1] - u[i0] - v[j];
                if (cur < minv[j]) { minv[j] = cur; way[j] = j0; }
                if (minv[j] < delta) { delta = minv[j]; j1 = j; }
            }
            for (int j = 0; j <= n; ++j) {
                if (used[j]) { u[p[j]] += delta; v[j] -= delta; }
                else         { minv[j] -= delta; }
            }
            j0 = j1;
        } while (p[j0] != 0);
        do {
            const int j1 = way[j0];
            p[j0] = p[j1];
            j0 = j1;
        } while (j0 != 0);
    }

    for (int j = 1; j <= n; ++j) {
        if (p[j] != 0) result.rowToCol[p[j] - 1] = j - 1;
    }
    for (int i = 0; i < n; ++i) {
        result.weight += cost[static_cast<size_t>(i) * n + result.rowToCol[i]];
    }
    return result;
}

#endif // KUHN_MUNKRES_H

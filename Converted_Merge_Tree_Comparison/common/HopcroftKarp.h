// Hopcroft-Karp maximum-cardinality bipartite matching.
//
// Replaces org.jgrapht.alg.matching.HopcroftKarpMaximumCardinalityBipartiteMatching
// as used in the bottleneck binary search: the Java code only reads
// matching.getMatching().getEdges().size(), i.e. the matching size.
//
// Usage:
//   HopcroftKarp hk(numLeft, numRight);
//   hk.addEdge(l, r);            // 0 <= l < numLeft, 0 <= r < numRight
//   int size = hk.maxMatching();
//
// Complexity: O(E * sqrt(V)).
#ifndef HOPCROFT_KARP_H
#define HOPCROFT_KARP_H

#include <queue>
#include <vector>

class HopcroftKarp {
public:
    HopcroftKarp(int numLeft, int numRight)
        : nL(numLeft), nR(numRight), adj(numLeft),
          matchL(numLeft, -1), matchR(numRight, -1), dist(numLeft, 0) {}

    void addEdge(int l, int r) { adj[l].push_back(r); }

    int maxMatching() {
        int result = 0;
        while (bfs()) {
            for (int l = 0; l < nL; ++l) {
                if (matchL[l] == -1 && dfs(l)) ++result;
            }
        }
        return result;
    }

    // Valid after maxMatching(): partner of each vertex, or -1.
    const std::vector<int>& leftMatches() const { return matchL; }
    const std::vector<int>& rightMatches() const { return matchR; }

private:
    static constexpr int INF = 1 << 30;
    int nL, nR;
    std::vector<std::vector<int>> adj;
    std::vector<int> matchL, matchR, dist;

    bool bfs() {
        std::queue<int> q;
        bool foundFree = false;
        for (int l = 0; l < nL; ++l) {
            if (matchL[l] == -1) { dist[l] = 0; q.push(l); }
            else                 { dist[l] = INF; }
        }
        while (!q.empty()) {
            const int l = q.front(); q.pop();
            for (int r : adj[l]) {
                const int l2 = matchR[r];
                if (l2 == -1) {
                    foundFree = true;
                } else if (dist[l2] == INF) {
                    dist[l2] = dist[l] + 1;
                    q.push(l2);
                }
            }
        }
        return foundFree;
    }

    bool dfs(int l) {
        for (int r : adj[l]) {
            const int l2 = matchR[r];
            if (l2 == -1 || (dist[l2] == dist[l] + 1 && dfs(l2))) {
                matchL[l] = r;
                matchR[r] = l;
                return true;
            }
        }
        dist[l] = INF;
        return false;
    }
};

#endif // HOPCROFT_KARP_H

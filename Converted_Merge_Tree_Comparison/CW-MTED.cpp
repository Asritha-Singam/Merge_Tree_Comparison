// CW-MTED: constrained edit distance between two merge (join) trees, using the
// CW cost (L-infinity based, deletion = persistence / 4).
// C++ port of CW-MTED.java (class ConstrainedEditDist).
//
// Build:   g++ -std=c++17 -O2 -static -o CW-MTED.exe CW-MTED.cpp
//          (-static avoids Git Bash loading an older, incompatible libstdc++ DLL)
// Usage:   CW-MTED <tree1.jt> <tree2.jt> <outputDir>
// Prints:  the edit distance on stdout (formatted like Java's Double.toString)
// Writes:  <outputDir>/forwardmapCW.txt and <outputDir>/backwardmapCW.txt,
//          one "tree1ID,tree2ID" line per matched node pair.
//
// Differences from the Java version (results are the same):
//  * Only primitive types are stored (int / double in flat arrays). The Java code
//    used Map<Pair<Integer>, Double> and Map<Integer, Integer>, which box every value.
//  * The Java code copied a whole node mapping (HashMap) into every DP cell. Here
//    each cell only stores which case was chosen (one int), and the final mapping
//    is rebuilt once at the end by following those choices.
//  * Stabilization is skipped: the Java code calls it with threshold 0, and its
//    test |a - b| < 0 is never true, so it always returns an unchanged copy.
//  * Map files are written in ascending post-order index. The Java order came
//    from HashMap iteration, which equals this order for small trees.

#include <algorithm>
#include <charconv>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <limits>
#include <string>
#include <unordered_map>
#include <vector>

#include "common/KuhnMunkres.h"

namespace {

// ---------------------------------------------------------------------------
// Merge tree, stored as flat arrays indexed by post-order index (0 .. n-1).
// ---------------------------------------------------------------------------
struct MergeTree {
    int n = 0;
    int root = -1;                    // post-order index of the root (= n - 1)
    std::vector<int> nodeID;          // ID from the .jt file
    std::vector<double> value;        // scalar value of the node
    std::vector<double> pairValue;    // scalar value of its persistence pair
    std::vector<int> childStart;      // children of i: childList[childStart[i] .. childStart[i+1])
    std::vector<int> childList;

    int numChildren(int i) const { return childStart[i + 1] - childStart[i]; }
    const int* childrenBegin(int i) const { return childList.data() + childStart[i]; }
    const int* childrenEnd(int i) const { return childList.data() + childStart[i + 1]; }
};

// Reads a .jt file:
//   <numNodes>
//   <id> <parent> <pair> <maxNode> <value> <numChildren> <child IDs...>   (per node)
// The root is the node whose parent ID does not appear in the file.
bool readJoinTree(const std::string& fileName, MergeTree& tree, std::string& error) {
    std::ifstream in(fileName);
    if (!in) { error = "cannot open " + fileName; return false; }

    int numNodes = 0;
    if (!(in >> numNodes) || numNodes <= 0) { error = "bad node count in " + fileName; return false; }

    // Raw data in file order ("slot" = position in the file).
    std::vector<int> id(numNodes), parent(numNodes), pairID(numNodes);
    std::vector<double> val(numNodes);
    std::vector<int> rawChildStart(numNodes + 1, 0), rawChildIDs;
    std::unordered_map<int, int> slotOf;
    slotOf.reserve(numNodes * 2);

    for (int s = 0; s < numNodes; ++s) {
        int maxNode = 0, numChildren = 0;
        if (!(in >> id[s] >> parent[s] >> pairID[s] >> maxNode >> val[s] >> numChildren) || numChildren < 0) {
            error = "bad node record " + std::to_string(s + 1) + " in " + fileName;
            return false;
        }
        for (int c = 0; c < numChildren; ++c) {
            int child;
            if (!(in >> child)) { error = "bad child list in " + fileName; return false; }
            rawChildIDs.push_back(child);
        }
        rawChildStart[s + 1] = static_cast<int>(rawChildIDs.size());
        slotOf[id[s]] = s;
    }

    // Resolve IDs to slots.
    int rootSlot = -1;
    for (int s = 0; s < numNodes; ++s) {
        if (slotOf.find(parent[s]) == slotOf.end()) rootSlot = s;
    }
    if (rootSlot < 0) { error = "no root found in " + fileName; return false; }
    std::vector<int> rawChildSlots(rawChildIDs.size());
    for (size_t k = 0; k < rawChildIDs.size(); ++k) {
        auto it = slotOf.find(rawChildIDs[k]);
        if (it == slotOf.end()) { error = "unknown child ID " + std::to_string(rawChildIDs[k]) + " in " + fileName; return false; }
        rawChildSlots[k] = it->second;
    }

    // Post-order numbering (children in file order, then the node), done
    // iteratively so deep trees cannot overflow the call stack.
    std::vector<int> postIndex(numNodes, -1), order;
    order.reserve(numNodes);
    {
        std::vector<std::pair<int, int>> stack;  // (slot, next child position)
        stack.push_back({rootSlot, rawChildStart[rootSlot]});
        while (!stack.empty()) {
            auto& top = stack.back();
            const int s = top.first;
            if (top.second < rawChildStart[s + 1]) {
                const int child = rawChildSlots[top.second++];
                stack.push_back({child, rawChildStart[child]});
            } else {
                postIndex[s] = static_cast<int>(order.size());
                order.push_back(s);
                stack.pop_back();
            }
        }
    }

    const int n = static_cast<int>(order.size());  // nodes reachable from the root
    tree.n = n;
    tree.root = n - 1;
    tree.nodeID.resize(n);
    tree.value.resize(n);
    tree.pairValue.resize(n);
    tree.childStart.assign(n + 1, 0);
    tree.childList.clear();
    for (int i = 0; i < n; ++i) {
        const int s = order[i];
        tree.nodeID[i] = id[s];
        tree.value[i] = val[s];
        auto pit = slotOf.find(pairID[s]);
        if (pit == slotOf.end()) { error = "unknown pair ID " + std::to_string(pairID[s]) + " in " + fileName; return false; }
        tree.pairValue[i] = val[pit->second];
        for (int k = rawChildStart[s]; k < rawChildStart[s + 1]; ++k) {
            tree.childList.push_back(postIndex[rawChildSlots[k]]);
        }
        tree.childStart[i + 1] = static_cast<int>(tree.childList.size());
    }
    return true;
}

// ---------------------------------------------------------------------------
// Node matching cost (JoinTreePair.matchingCost in the Java code, CW variant).
// ---------------------------------------------------------------------------
inline double deleteCost(const MergeTree& t, int i) {
    return std::fabs(t.value[i] - t.pairValue[i]) / 4.0;
}

inline double relabelCost(const MergeTree& t1, int i, const MergeTree& t2, int j) {
    const double linf12 = std::max(std::fabs(t1.value[i] - t2.value[j]), std::fabs(t1.pairValue[i] - t2.pairValue[j]));
    const double linf1 = std::fabs(t1.value[i] - t1.pairValue[i]) / 2.0;
    const double linf2 = std::fabs(t2.value[j] - t2.pairValue[j]) / 2.0;
    return std::min(linf12, linf1 + linf2) / 2.0;
}

// ---------------------------------------------------------------------------
// Constrained edit distance DP.
//
// Tables are (n1 + 1) x (n2 + 1); row/column 0 stands for index -1 (empty tree),
// so cell (i, j) is stored at (i + 1) * (n2 + 1) + (j + 1).
//
// Choice codes stored per cell (how the value was obtained):
//   CHOICE_MATCH (-1)  : case 3 (match i with j / match the child forests)
//   c >= 0             : case 1 with tree-2 child c
//   -(c + 2) (<= -2)   : case 2 with tree-1 child c
// ---------------------------------------------------------------------------
constexpr int CHOICE_MATCH = -1;
constexpr double TIE_EPS = 0.00001;  // same tolerance as the Java code

class ConstrainedEditDist {
public:
    ConstrainedEditDist(const MergeTree& a, const MergeTree& b)
        : t1(a), t2(b), cols(b.n + 1),
          DFv(static_cast<size_t>(a.n + 1) * (b.n + 1), 0.0),
          DTv(DFv.size(), 0.0),
          DFchoice(DFv.size(), CHOICE_MATCH),
          DTchoice(DFv.size(), CHOICE_MATCH) {}

    double compute() {
        // Deleting whole subtrees (index -1 on the other side).
        for (int i = 0; i < t1.n; ++i) {
            double cost = 0;
            for (const int* c = t1.childrenBegin(i); c != t1.childrenEnd(i); ++c) cost += DT(*c, -1);
            DF(i, -1) = cost;
            DT(i, -1) = cost + deleteCost(t1, i);
        }
        for (int j = 0; j < t2.n; ++j) {
            double cost = 0;
            for (const int* c = t2.childrenBegin(j); c != t2.childrenEnd(j); ++c) cost += DT(-1, *c);
            DF(-1, j) = cost;
            DT(-1, j) = cost + deleteCost(t2, j);
        }
        // Post-order guarantees every cell this one depends on is already filled.
        for (int i = 0; i < t1.n; ++i) {
            for (int j = 0; j < t2.n; ++j) {
                computeDF(i, j);
                computeDT(i, j);
            }
        }
        return DT(t1.root, t2.root);
    }

    // Rebuilds the node mapping of the optimal solution (forward[i] = j, or -1).
    std::vector<int> forwardMapping() {
        std::vector<int> forward(t1.n, -1);
        rebuildDT(t1.root, t2.root, forward);
        return forward;
    }

private:
    const MergeTree& t1;
    const MergeTree& t2;
    const size_t cols;
    std::vector<double> DFv, DTv;
    std::vector<int> DFchoice, DTchoice;

    // Reusable buffer for MM's cost matrix.
    std::vector<double> mmCost;

    size_t cell(int i, int j) const { return static_cast<size_t>(i + 1) * cols + (j + 1); }
    double& DF(int i, int j) { return DFv[cell(i, j)]; }
    double& DT(int i, int j) { return DTv[cell(i, j)]; }

    // Chooses between the three cases exactly as the Java code does.
    static int choose(double case1, double case2, double case3, int case1Child, int case2Child,
                      double& finalCost) {
        if ((case3 - TIE_EPS) <= case2 && (case3 - TIE_EPS) <= case1) {
            finalCost = case3;
            return CHOICE_MATCH;
        } else if (case1 < case2) {
            finalCost = case1;
            return case1Child;
        } else {
            finalCost = case2;
            return -(case2Child + 2);
        }
    }

    void computeDF(int i, int j) {
        // CASE 1: keep i, match its forest into one child subtree of j.
        double minCost = std::numeric_limits<double>::max();
        int case1Child = -1;
        for (const int* c = t2.childrenBegin(j); c != t2.childrenEnd(j); ++c) {
            const double cur = DF(i, *c) - DF(-1, *c);
            if (cur < minCost) { minCost = cur; case1Child = *c; }
        }
        const double case1 = DF(-1, j) + minCost;

        // CASE 2: symmetric, one child subtree of i.
        minCost = std::numeric_limits<double>::max();
        int case2Child = -1;
        for (const int* c = t1.childrenBegin(i); c != t1.childrenEnd(i); ++c) {
            const double cur = DF(*c, j) - DF(*c, -1);
            if (cur < minCost) { minCost = cur; case2Child = *c; }
        }
        const double case2 = DF(i, -1) + minCost;

        // CASE 3: match the two child forests.
        const double case3 = MM(i, j, nullptr);

        double finalCost;
        DFchoice[cell(i, j)] = choose(case1, case2, case3, case1Child, case2Child, finalCost);
        DF(i, j) = finalCost;
    }

    void computeDT(int i, int j) {
        double minCost = std::numeric_limits<double>::max();
        int case1Child = -1;
        for (const int* c = t2.childrenBegin(j); c != t2.childrenEnd(j); ++c) {
            const double cur = DT(i, *c) - DT(-1, *c);
            if (cur < minCost) { minCost = cur; case1Child = *c; }
        }
        const double case1 = DT(-1, j) + minCost;

        minCost = std::numeric_limits<double>::max();
        int case2Child = -1;
        for (const int* c = t1.childrenBegin(i); c != t1.childrenEnd(i); ++c) {
            const double cur = DT(*c, j) - DT(*c, -1);
            if (cur < minCost) { minCost = cur; case2Child = *c; }
        }
        const double case2 = DT(i, -1) + minCost;

        const double case3 = DF(i, j) + relabelCost(t1, i, t2, j);

        double finalCost;
        DTchoice[cell(i, j)] = choose(case1, case2, case3, case1Child, case2Child, finalCost);
        DT(i, j) = finalCost;
    }

    // Minimum-cost matching between the children of i and the children of j,
    // each side padded with dummy nodes (index -1) so the matrix is square.
    // If 'pairs' is given, the matched (child1, child2) index pairs are stored in it.
    double MM(int i, int j, std::vector<std::pair<int, int>>* pairs) {
        const int a = t1.numChildren(i);
        const int b = t2.numChildren(j);
        if (a == 0 && b == 0) return 0.0;

        const int n = a + b;
        const int* c1 = t1.childrenBegin(i);
        const int* c2 = t2.childrenBegin(j);
        mmCost.resize(static_cast<size_t>(n) * n);
        for (int k = 0; k < n; ++k) {
            const int idx1 = (k < a) ? c1[k] : -1;
            for (int l = 0; l < n; ++l) {
                const int idx2 = (l < b) ? c2[l] : -1;
                mmCost[static_cast<size_t>(k) * n + l] = DT(idx1, idx2);
            }
        }
        const KuhnMunkresResult result = kuhnMunkresMinWeightPerfectMatching(mmCost, n);
        if (pairs) {
            pairs->clear();
            for (int k = 0; k < n; ++k) {
                const int l = result.rowToCol[k];
                const int idx1 = (k < a) ? c1[k] : -1;
                const int idx2 = (l < b) ? c2[l] : -1;
                if (idx1 >= 0 || idx2 >= 0) pairs->push_back({idx1, idx2});
            }
        }
        return result.weight;
    }

    // Following the stored choices reproduces the mapping the Java code kept
    // in pairedDTForward / pairedDFForward.
    void rebuildDT(int i, int j, std::vector<int>& forward) {
        if (i < 0 || j < 0) return;  // deleting a subtree maps nothing
        const int choice = DTchoice[cell(i, j)];
        if (choice == CHOICE_MATCH) {
            rebuildDF(i, j, forward);
            forward[i] = j;
        } else if (choice >= 0) {
            rebuildDT(i, choice, forward);
        } else {
            rebuildDT(-choice - 2, j, forward);
        }
    }

    void rebuildDF(int i, int j, std::vector<int>& forward) {
        if (i < 0 || j < 0) return;
        const int choice = DFchoice[cell(i, j)];
        if (choice == CHOICE_MATCH) {
            // Re-run the (deterministic) matching instead of storing it per cell.
            std::vector<std::pair<int, int>> pairs;
            MM(i, j, &pairs);
            for (const auto& p : pairs) rebuildDT(p.first, p.second, forward);
        } else if (choice >= 0) {
            rebuildDF(i, choice, forward);
        } else {
            rebuildDF(-choice - 2, j, forward);
        }
    }
};

// ---------------------------------------------------------------------------
// Formats a double the way Java's Double.toString does (JDK 19+): shortest
// digits that round-trip; plain notation for 1e-3 <= |v| < 1e7, otherwise
// "d.dddE<exp>"; always at least one digit after the decimal point.
// ---------------------------------------------------------------------------
std::string javaDoubleToString(double v) {
    if (std::isnan(v)) return "NaN";
    if (std::isinf(v)) return v > 0 ? "Infinity" : "-Infinity";
    if (v == 0.0) return std::signbit(v) ? "-0.0" : "0.0";

    char buf[64];
    auto res = std::to_chars(buf, buf + sizeof buf, v, std::chars_format::scientific);
    std::string s(buf, res.ptr);  // e.g. "-1.2345e+02"

    std::string sign;
    if (s[0] == '-') { sign = "-"; s.erase(0, 1); }
    const size_t ePos = s.find('e');
    const int exp10 = std::stoi(s.substr(ePos + 1));
    std::string digits = s.substr(0, ePos);
    if (digits.size() > 1) digits.erase(1, 1);  // drop the '.'

    const double mag = std::fabs(v);
    std::string out;
    if (mag >= 1e-3 && mag < 1e7) {
        if (exp10 >= 0) {
            const size_t intLen = static_cast<size_t>(exp10) + 1;
            if (digits.size() <= intLen) {
                out = digits + std::string(intLen - digits.size(), '0') + ".0";
            } else {
                out = digits.substr(0, intLen) + "." + digits.substr(intLen);
            }
        } else {
            out = "0." + std::string(static_cast<size_t>(-exp10 - 1), '0') + digits;
        }
    } else {
        out = digits.substr(0, 1) + "." + (digits.size() > 1 ? digits.substr(1) : "0") + "E" + std::to_string(exp10);
    }
    return sign + out;
}

bool writeMapFile(const std::string& path, const std::vector<std::pair<int, int>>& idPairs) {
    // Binary mode: write "\n" line endings like Java does, also on Windows.
    std::ofstream out(path, std::ios::binary);
    if (!out) return false;
    for (const auto& p : idPairs) out << p.first << ',' << p.second << '\n';
    return static_cast<bool>(out);
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 4) {
        std::cerr << "Usage: " << argv[0] << " <tree1.jt> <tree2.jt> <outputDir>\n";
        return 1;
    }
    const std::string jt1FileName = argv[1];
    const std::string jt2FileName = argv[2];
    const std::string location = argv[3];

    MergeTree tree1, tree2;
    std::string error;
    if (!readJoinTree(jt1FileName, tree1, error) || !readJoinTree(jt2FileName, tree2, error)) {
        std::cerr << "Error: " << error << '\n';
        return 1;
    }

    ConstrainedEditDist ced(tree1, tree2);
    const double cost = ced.compute();

    // Mapping files, as node IDs.
    const std::vector<int> forward = ced.forwardMapping();
    std::vector<int> backward(tree2.n, -1);
    std::vector<std::pair<int, int>> forwardIDs, backwardIDs;
    for (int i = 0; i < tree1.n; ++i) {
        if (forward[i] < 0) continue;
        backward[forward[i]] = i;
        forwardIDs.push_back({tree1.nodeID[i], tree2.nodeID[forward[i]]});
    }
    for (int j = 0; j < tree2.n; ++j) {
        if (backward[j] < 0) continue;
        backwardIDs.push_back({tree1.nodeID[backward[j]], tree2.nodeID[j]});  // same "t1,t2" order as Java
    }
    // Java prints "Buggy code" to stdout when a map file cannot be written.
    if (!writeMapFile(location + "/forwardmapCW.txt", forwardIDs)) {
        std::cout << "Buggy code\n";
        std::cerr << "cannot write " << location << "/forwardmapCW.txt\n";
    }
    if (!writeMapFile(location + "/backwardmapCW.txt", backwardIDs)) {
        std::cout << "Buggy code\n";
        std::cerr << "cannot write " << location << "/backwardmapCW.txt\n";
    }

    std::cout << javaDoubleToString(cost) << '\n';
    return 0;
}

// CustomisedBottleneck: bottleneck distance between the persistence diagrams of
// two merge (join) trees.
// C++ port of CustomisedBottleneck.java (class ConstrainedEditDist, method
// bottleneckDist, which is the only thing its main() computes).
//
// Build:   g++ -std=c++17 -O2 -static -o CustomisedBottleneck.exe CustomisedBottleneck.cpp
//          (-static avoids Git Bash loading an older, incompatible libstdc++ DLL)
// Usage:   CustomisedBottleneck <tree1.jt> <tree2.jt> <outputDir>
// Prints:  the bottleneck distance on stdout (formatted like Java's Double.toString)
// Writes:  nothing. <outputDir> is accepted (and ignored) like in the Java code.
//
// Method (same as the Java code):
//  * Every leaf of a tree is a point (birth = leaf value, death = value of its
//    persistence pair).
//  * Candidate distances: |birth - death| / 2 for every leaf (cost of matching it
//    to the diagonal) and max(|birth1 - birth2|, |death1 - death2|) for every
//    (tree-1 leaf, tree-2 leaf) pair. They are de-duplicated and sorted.
//  * Binary search over the candidates: for a candidate w, build the bipartite
//    graph whose edges cost <= w (plus diagonal dummies) and test whether it has a
//    perfect matching (Hopcroft-Karp). The smallest feasible w is printed.
//
// Differences from the Java version (results are the same):
//  * Only the code main() runs is ported. The edit-distance code (DF, DT, MM,
//    compute), contour trees and the unused JoinTreePair are left out.
//  * Stabilization is skipped: Java calls it with threshold 0, its test
//    |a - b| < 0 is never true, and bottleneckDist reads the unstabilized nodes.
//    Its side effect, crashing on malformed trees, is reproduced by validateTree.
//  * JGraphT's HopcroftKarpMaximumCardinalityBipartiteMatching is replaced by
//    common/HopcroftKarp.h. Only the matching size is used, and the maximum
//    matching size is unique, so the result does not depend on which matching
//    either implementation finds.
//  * The candidate costs are computed once instead of in every search step.
//  * Where Java dies with an exception (bad number, missing node, no root, ...),
//    this prints an error naming that exception to stderr and exits with 1,
//    with nothing on stdout, like the Java run.

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "common/HopcroftKarp.h"

namespace {

// ---------------------------------------------------------------------------
// Number parsing that accepts what java.util.Scanner's nextInt / nextDouble
// accept for these files (default locale group separators such as "1,000" are
// not supported).
// ---------------------------------------------------------------------------
bool parseJavaInt(const std::string& tok, int& out) {
    const char* b = tok.data();
    const char* e = b + tok.size();
    bool plus = false;
    if (b != e && *b == '+') { ++b; plus = true; }  // from_chars does not take '+'
    if (b == e || !(std::isdigit(static_cast<unsigned char>(*b)) || (*b == '-' && !plus))) return false;
    auto res = std::from_chars(b, e, out);      // fails on overflow, like Scanner
    return res.ec == std::errc() && res.ptr == e;
}

bool parseJavaDouble(const std::string& tok, double& out) {
    std::string t = tok;
    bool negative = false;
    if (!t.empty() && (t[0] == '+' || t[0] == '-')) {
        negative = t[0] == '-';
        t.erase(0, 1);
    }
    if (t == "NaN") { out = std::nan(""); return true; }
    if (t == "Infinity") { out = negative ? -INFINITY : INFINITY; return true; }
    // Plain decimal only: digits, '.', exponent. This rejects "inf", "nan", hex, a second sign.
    if (t.empty() || !(std::isdigit(static_cast<unsigned char>(t[0])) || t[0] == '.')) return false;
    for (char c : t) {
        if (!(std::isdigit(static_cast<unsigned char>(c)) || c == '.' || c == 'e' || c == 'E' || c == '+' || c == '-')) {
            return false;
        }
    }
    double v = 0.0;
    auto res = std::from_chars(t.data(), t.data() + t.size(), v, std::chars_format::general);
    if (res.ec != std::errc() || res.ptr != t.data() + t.size()) return false;
    out = negative ? -v : v;
    return true;
}

// Reads whitespace-separated tokens, like Scanner.
class TokenReader {
public:
    explicit TokenReader(std::istream& in) : in_(in) {}

    // On failure, error is the exception Java's Scanner would throw.
    bool nextInt(int& out, std::string& error) {
        std::string tok;
        if (!(in_ >> tok)) { error = "java.util.NoSuchElementException"; return false; }
        if (!parseJavaInt(tok, out)) { error = "java.util.InputMismatchException: \"" + tok + "\""; return false; }
        return true;
    }
    bool nextDouble(double& out, std::string& error) {
        std::string tok;
        if (!(in_ >> tok)) { error = "java.util.NoSuchElementException"; return false; }
        if (!parseJavaDouble(tok, out)) { error = "java.util.InputMismatchException: \"" + tok + "\""; return false; }
        return true;
    }

private:
    std::istream& in_;
};

// ---------------------------------------------------------------------------
// Join tree, as read from a .jt file (JoinTree.readJoinTree in the Java code).
// ---------------------------------------------------------------------------
struct JTNode {
    int nodeID = 0;
    int parent = 0;
    int pair = 0;
    double value = 0.0;
    std::vector<int> childrenIDs;
};

struct JoinTree {
    std::vector<JTNode> nodes;                 // one entry per distinct ID, in first-insertion order
    std::unordered_map<int, int> indexOf;      // node ID -> position in nodes
};

// Reads a .jt file:
//   <numNodes>
//   <id> <parent> <pair> <maxNode> <value> <numChildren> <child IDs...>   (per node)
// Like Java's HashMap.put, a repeated ID replaces the earlier record but keeps
// its original insertion position.
bool readJoinTree(const std::string& fileName, JoinTree& tree, std::string& error) {
    std::ifstream in(fileName);
    if (!in) { error = "java.io.FileNotFoundException: " + fileName; return false; }
    TokenReader sc(in);

    int numNodes = 0;
    if (!sc.nextInt(numNodes, error)) return false;
    for (int i = 0; i < numNodes; ++i) {
        JTNode node;
        int maxNode = 0, numChildren = 0;
        if (!sc.nextInt(node.nodeID, error) || !sc.nextInt(node.parent, error) ||
            !sc.nextInt(node.pair, error) || !sc.nextInt(maxNode, error) ||
            !sc.nextDouble(node.value, error) || !sc.nextInt(numChildren, error)) {
            return false;
        }
        for (int j = 0; j < numChildren; ++j) {     // a negative count reads no children, like Java
            int child = 0;
            if (!sc.nextInt(child, error)) return false;
            node.childrenIDs.push_back(child);
        }
        auto it = tree.indexOf.find(node.nodeID);
        if (it == tree.indexOf.end()) {
            tree.indexOf[node.nodeID] = static_cast<int>(tree.nodes.size());
            tree.nodes.push_back(std::move(node));
        } else {
            tree.nodes[it->second] = std::move(node);
        }
    }
    return true;
}

// Java picks the root as the LAST node, in HashMap<Integer, JTNode> iteration
// order, whose parent ID is not in the map. That order is by bucket
// (spread(hash) & (capacity - 1), with Integer.hashCode = the value), and by
// insertion order inside a bucket. Returns -1 if there is no such node.
int findRoot(const JoinTree& tree) {
    const size_t n = tree.nodes.size();
    size_t capacity = 16;
    while (n > capacity / 4 * 3) capacity *= 2;  // HashMap resizes when size > 0.75 * capacity

    int root = -1;
    size_t rootBucket = 0;
    for (size_t k = 0; k < n; ++k) {             // k ascending = insertion order
        const JTNode& node = tree.nodes[k];
        if (tree.indexOf.count(node.parent)) continue;
        const uint32_t h = static_cast<uint32_t>(node.nodeID);
        const size_t bucket = (h ^ (h >> 16)) & (capacity - 1);
        if (root < 0 || bucket >= rootBucket) {
            root = static_cast<int>(k);
            rootBucket = bucket;
        }
    }
    return root;
}

// Java's JoinTree.stabilize (called by main before bottleneckDist) walks the tree
// recursively from the root and then looks up every node's results. It throws
//  * NullPointerException if there is no root, if a child ID does not exist, or
//    if some node cannot be reached from the root;
//  * StackOverflowError if a cycle can be reached from the root.
// This reproduces those failures without the (otherwise no-op) stabilization.
// Very deep trees can also overflow Java's stack; that limit is not reproduced.
bool validateTree(const JoinTree& tree, std::string& error) {
    const int root = findRoot(tree);
    if (root < 0) { error = "java.lang.NullPointerException (no root)"; return false; }

    const size_t n = tree.nodes.size();
    enum : char { UNSEEN, ON_PATH, DONE };
    std::vector<char> state(n, UNSEEN);
    std::vector<std::pair<int, size_t>> stack;   // (node, next child position)
    stack.push_back({root, 0});
    state[root] = ON_PATH;
    while (!stack.empty()) {
        auto& top = stack.back();
        const JTNode& node = tree.nodes[top.first];
        if (top.second == node.childrenIDs.size()) {
            state[top.first] = DONE;
            stack.pop_back();
            continue;
        }
        const int childID = node.childrenIDs[top.second++];
        auto it = tree.indexOf.find(childID);
        if (it == tree.indexOf.end()) {
            error = "java.lang.NullPointerException (child " + std::to_string(childID) + " of node " +
                    std::to_string(node.nodeID) + " does not exist)";
            return false;
        }
        const int child = it->second;
        if (state[child] == ON_PATH) {
            error = "java.lang.StackOverflowError (cycle through node " + std::to_string(childID) + ")";
            return false;
        }
        if (state[child] == UNSEEN) {            // a DONE node (reached twice) needs no new visit
            state[child] = ON_PATH;
            stack.push_back({child, 0});
        }
    }
    for (size_t k = 0; k < n; ++k) {
        if (state[k] == UNSEEN) {
            error = "java.lang.NullPointerException (node " + std::to_string(tree.nodes[k].nodeID) +
                    " is not reachable from the root)";
            return false;
        }
    }
    return true;
}

// ---------------------------------------------------------------------------
// Bottleneck distance (ConstrainedEditDist.bottleneckDist in the Java code).
// ---------------------------------------------------------------------------
struct Point {
    double birth;  // leaf value
    double death;  // value of the leaf's persistence pair
};

// Leaves (nodes without children) as diagram points.
bool leafPoints(const JoinTree& tree, std::vector<Point>& points, std::string& error) {
    for (const JTNode& node : tree.nodes) {
        if (!node.childrenIDs.empty()) continue;
        auto it = tree.indexOf.find(node.pair);
        if (it == tree.indexOf.end()) {
            error = "java.lang.NullPointerException (pair " + std::to_string(node.pair) + " of node " +
                    std::to_string(node.nodeID) + " does not exist)";
            return false;
        }
        points.push_back({node.value, tree.nodes[it->second].value});
    }
    return true;
}

// Math.max: NaN if either argument is NaN (std::max is not).
inline double javaMax(double a, double b) {
    if (std::isnan(a) || std::isnan(b)) return std::nan("");
    return a > b ? a : b;
}

inline double diagonalCost(const Point& p) { return std::fabs(p.birth - p.death) / 2.0; }

inline double pointCost(const Point& p1, const Point& p2) {
    return javaMax(std::fabs(p2.birth - p1.birth), std::fabs(p2.death - p1.death));
}

// Returns false (Java: IndexOutOfBoundsException) only if both trees have no leaves.
bool bottleneckDist(const std::vector<Point>& S, const std::vector<Point>& T, double& result, std::string& error) {
    const int n1 = static_cast<int>(S.size());
    const int n2 = static_cast<int>(T.size());

    // All candidate costs. Java's Collections.sort on Doubles puts NaN last, and
    // its HashSet keeps a single NaN; costs are never -0.0 (they come from fabs).
    std::vector<double> diag1(n1), diag2(n2), cross(static_cast<size_t>(n1) * n2);
    std::vector<double> weights;
    weights.reserve(n1 + n2 + cross.size());
    for (int i = 0; i < n1; ++i) weights.push_back(diag1[i] = diagonalCost(S[i]));
    for (int j = 0; j < n2; ++j) weights.push_back(diag2[j] = diagonalCost(T[j]));
    for (int i = 0; i < n1; ++i) {
        for (int j = 0; j < n2; ++j) {
            weights.push_back(cross[static_cast<size_t>(i) * n2 + j] = pointCost(S[i], T[j]));
        }
    }
    std::sort(weights.begin(), weights.end(), [](double a, double b) {
        if (std::isnan(a)) return false;
        if (std::isnan(b)) return true;
        return a < b;
    });
    weights.erase(std::unique(weights.begin(), weights.end(), [](double a, double b) {
        return a == b || (std::isnan(a) && std::isnan(b));
    }), weights.end());
    if (weights.empty()) { error = "java.lang.IndexOutOfBoundsException (no leaves)"; return false; }

    // Vertices: left  = [tree-1 leaves 0..n1-1 | dummies n1..n1+n2-1]
    //           right = [tree-2 leaves 0..n2-1 | dummies n2..n2+n1-1]
    // An edge is left out only when its cost is > w; a NaN cost (or w) never is,
    // exactly like the Java test "weight > currentWeight".
    const int size = n1 + n2;
    double currentWeight = 0;
    for (int left = 0, right = static_cast<int>(weights.size()); left <= right;) {
        const int mid = (left + right) / 2;
        currentWeight = weights[mid];
        const double w = currentWeight;

        HopcroftKarp hk(size, size);
        for (int i = 0; i < n1; ++i) {                       // tree-1 leaf -> diagonal
            if (diag1[i] > w) continue;
            for (int r = n2; r < size; ++r) hk.addEdge(i, r);
        }
        for (int j = 0; j < n2; ++j) {                       // tree-2 leaf -> diagonal
            if (diag2[j] > w) continue;
            for (int l = n1; l < size; ++l) hk.addEdge(l, j);
        }
        for (int l = n1; l < size; ++l) {                    // diagonal -> diagonal, always
            for (int r = n2; r < size; ++r) hk.addEdge(l, r);
        }
        for (int i = 0; i < n1; ++i) {                       // tree-1 leaf -> tree-2 leaf
            for (int j = 0; j < n2; ++j) {
                if (cross[static_cast<size_t>(i) * n2 + j] > w) continue;
                hk.addEdge(i, j);
            }
        }
        const int matchedPairsSize = hk.maxMatching();

        if (left == right) break;
        if (matchedPairsSize == size) {
            right = mid;
        } else {
            left = mid + 1;
        }
    }
    result = currentWeight;
    return true;
}

// ---------------------------------------------------------------------------
// Formats a double the way Java's Double.toString does (JDK 19+): shortest
// digits that round-trip; plain notation for 1e-3 <= |v| < 1e7, otherwise
// "d.dddE<exp>"; always at least one digit after the decimal point.
// (Same as in CO-MTED.cpp / CW-MTED.cpp.)
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

}  // namespace

int main(int argc, char** argv) {
    if (argc < 4) {
        // Java reads args[0..2] and throws ArrayIndexOutOfBoundsException.
        std::cerr << "Usage: " << argv[0] << " <tree1.jt> <tree2.jt> <outputDir>\n";
        return 1;
    }
    const std::string jt1FileName = argv[1];
    const std::string jt2FileName = argv[2];
    // argv[3] (CompID in Java) is not used: the bottleneck writes no files.

    // Same order of steps (and so of failures) as Java's main: read + stabilize
    // tree 1, read + stabilize tree 2, then the leaves inside bottleneckDist.
    JoinTree tree1, tree2;
    std::vector<Point> S, T;
    std::string error;
    double distance = 0;
    if (!readJoinTree(jt1FileName, tree1, error) || !validateTree(tree1, error) ||
        !readJoinTree(jt2FileName, tree2, error) || !validateTree(tree2, error) ||
        !leafPoints(tree1, S, error) || !leafPoints(tree2, T, error) ||
        !bottleneckDist(S, T, distance, error)) {
        std::cerr << "Error: " << error << '\n';
        return 1;
    }

    std::cout << javaDoubleToString(distance) << '\n';
    return 0;
}

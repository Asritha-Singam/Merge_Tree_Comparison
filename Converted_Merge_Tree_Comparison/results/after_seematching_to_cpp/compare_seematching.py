"""Compare SeeMatching.java (root folder) with SeeMatching.exe (C++, converted folder).

Run from anywhere:
    python compare_seematching.py

What it does:
 1. Compiles the root SeeMatching.java once into a temporary folder (the root
    folder is only read, never changed).
 2. Runs Java and C++ on the same tree pairs as the other result folders:
      examples: every pair of Examples/Inputs/*.jt   (256 pairs)
      w1:       tv_0 vs tv_1 .. tv_250              (250 pairs)
 3. Compares WHICH pairs are matched (Java prints them in an unstable order,
    so order is ignored). If the matchings differ, it computes the total cost
    of both, to tell a tie (same cost: both correct) from a real difference.
 4. Writes, next to this script:
      <set>/SM_java.txt     Java output per pair
      <set>/SM_cpp.txt      C++ output per pair
      <set>/comparison.txt  same / tie / DIFFERENT per pair
      summary.txt
"""
import os
import re
import shutil
import subprocess
import sys
import tempfile
import time
from collections import Counter
from concurrent.futures import ThreadPoolExecutor

import numpy as np
from scipy.optimize import linear_sum_assignment

HERE = os.path.dirname(os.path.abspath(__file__))
CONVERTED = os.path.abspath(os.path.join(HERE, "..", ".."))
ROOT = os.path.dirname(CONVERTED)
EXE = os.path.join(CONVERTED, "SeeMatching.exe")
JAVA_SRC = os.path.join(ROOT, "SeeMatching.java")
JAVA_LIBS = os.path.join(ROOT, "CustomisedWasserstein")   # jgrapht/jheaps classes

SETS = {
    "examples": os.path.join(ROOT, "Examples", "Inputs"),
    "w1": os.path.join(ROOT, "test(W1)", "test_trees"),
}


def make_pairs():
    ex = SETS["examples"]
    trees = sorted(f for f in os.listdir(ex) if f.endswith(".jt"))
    pairs = [("examples", a, b) for a in trees for b in trees]
    pairs += [("w1", "tv_0.jt", "tv_%d.jt" % k) for k in range(1, 251)]
    return pairs


def compile_java(build):
    # The public class is comparator.ConstrainedEditDist, so javac needs that file name.
    src_dir = os.path.join(build, "src")
    os.makedirs(src_dir)
    src = os.path.join(src_dir, "ConstrainedEditDist.java")
    shutil.copy(JAVA_SRC, src)
    out = os.path.join(build, "classes")
    subprocess.run(["javac", "-nowarn", "-cp", JAVA_LIBS, "-d", out, src],
                   check=True, capture_output=True, text=True)
    return os.pathsep.join([out, JAVA_LIBS])


# ---------------------------------------------------------------- matching cost
def parse(out):
    return Counter(tuple(map(int, m)) for m in re.findall(r"\((-?\d+)\[\] : (-?\d+)\[\]\)", out))


def leaves(path):
    tok = open(path).read().split()
    n, i, nodes = int(tok[0]), 1, {}
    for _ in range(n):
        nid, pair, val, nc = int(tok[i]), int(tok[i + 2]), float(tok[i + 4]), int(tok[i + 5])
        nodes[nid] = (pair, val, nc)
        i += 6 + max(nc, 0)
    return {nid: (v, nodes[p][1]) for nid, (p, v, nc) in nodes.items() if nc == 0}


def matching_cost(path1, path2, matching):
    """Total cost of a matching, with the same cost formula as the Java code.
    The output does not say WHICH dummy a leaf went to (all print as -1), so the
    cheapest possible dummy assignment is used (that is what the optimum uses)."""
    L1, L2 = leaves(path1), leaves(path2)
    mids1 = [(b + d) / 2 for b, d in L1.values()]
    mids2 = [(b + d) / 2 for b, d in L2.values()]
    total, to_diag1, to_diag2 = 0.0, [], []
    for (a, b), cnt in matching.items():
        if a != -1 and b != -1:
            (b1, d1), (b2, d2) = L1[a], L2[b]
            total += cnt * max(abs(b2 - b1), abs(d2 - d1)) ** 2
        elif a != -1:
            to_diag1 += [L1[a]] * cnt
        elif b != -1:
            to_diag2 += [L2[b]] * cnt

    def best(points, mids):
        if not points:
            return 0.0
        M = np.array([[max(abs(p[0] - m), abs(p[1] - m)) ** 2 for m in mids] for p in points])
        r, c = linear_sum_assignment(M)
        return float(M[r, c].sum())

    return total + best(to_diag1, mids1) + best(to_diag2, mids2)


# ---------------------------------------------------------------- main
def main():
    pairs = make_pairs()
    start = time.time()
    with tempfile.TemporaryDirectory() as build:
        cp = compile_java(build)

        def run(job):
            s, a, b = job
            pa, pb = os.path.join(SETS[s], a), os.path.join(SETS[s], b)
            j = subprocess.run(["java", "-cp", cp, "comparator.ConstrainedEditDist", pa, pb, "./"],
                               capture_output=True, text=True)
            c = subprocess.run([EXE, pa, pb, "./"], capture_output=True, text=True)
            return s, a, b, pa, pb, j, c

        results = list(ThreadPoolExecutor(max_workers=os.cpu_count() or 4).map(run, pairs))

    java_ver = subprocess.run(["java", "-version"], capture_output=True, text=True).stderr.splitlines()[0]
    summary = [
        "run      after_seematching_to_cpp",
        "date     " + time.strftime("%Y%m%d-%H%M%S"),
        "java     " + java_ver,
        "java     " + JAVA_SRC + "  (compiled as comparator.ConstrainedEditDist)",
        "native   SM = " + EXE,
        "compare  set of matched pairs (order ignored); if different, total cost of both",
        "",
    ]
    for s in SETS:
        rows = [r for r in results if r[0] == s]
        d = os.path.join(HERE, s)
        os.makedirs(d, exist_ok=True)
        counts = Counter()
        with open(os.path.join(d, "SM_java.txt"), "w") as fj, \
             open(os.path.join(d, "SM_cpp.txt"), "w") as fc, \
             open(os.path.join(d, "comparison.txt"), "w") as fx:
            fj.write("# SM java  %s\n# tree1\ttree2\toutput\n" % JAVA_SRC)
            fc.write("# SM cpp  %s\n# tree1\ttree2\toutput\n" % EXE)
            fx.write("# tree1\ttree2\tresult\tjava_cost\tcpp_cost\n")
            for _, a, b, pa, pb, j, c in rows:
                fj.write("%s\t%s\t%s\n" % (a, b, j.stdout.strip() or "ERROR " + j.stderr.strip()[:200]))
                fc.write("%s\t%s\t%s\n" % (a, b, c.stdout.strip() or "ERROR " + c.stderr.strip()[:200]))
                mj, mc = parse(j.stdout), parse(c.stdout)
                if not mj or not mc:
                    res, cj, cc = "ERROR", "", ""
                elif mj == mc:
                    res, cj, cc = "same", "", ""
                else:
                    cj, cc = matching_cost(pa, pb, mj), matching_cost(pa, pb, mc)
                    res = "tie" if abs(cj - cc) <= 1e-9 * max(1.0, abs(cj)) else "DIFFERENT"
                counts[res] += 1
                fx.write("%s\t%s\t%s\t%s\t%s\n" % (a, b, res, cj, cc))
        summary.append("[%s]  inputs: %s  pairs: %d" % (s, SETS[s], len(rows)))
        summary.append("  same matching %d | tie (different matching, same cost) %d | DIFFERENT %d | ERROR %d"
                       % (counts["same"], counts["tie"], counts["DIFFERENT"], counts["ERROR"]))
        summary.append("")
    summary.append("total time %ds" % round(time.time() - start))
    text = "\n".join(summary) + "\n"
    open(os.path.join(HERE, "summary.txt"), "w").write(text)
    print(text)


if __name__ == "__main__":
    sys.exit(main())

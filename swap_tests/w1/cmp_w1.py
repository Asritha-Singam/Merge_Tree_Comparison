"""Compares test(W1)'s CustomisedWasserstein.jar with the root CustomisedWasserstein.java on tv_*.jt pairs."""
import os
import random
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor

REPO = sys.argv[1]
TT = os.path.join(REPO, "test(W1)", "test_trees")
JAR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "w.jar")
SRC = os.path.join(REPO, "test(W1)", "ConstrainedEditDist.java")
CP = os.path.join(REPO, "CustomisedWasserstein")

pairs = [("tv_0.jt", "tv_%d.jt" % i) for i in range(1, 251)]  # exactly what test.py runs
rng = random.Random(11)
pairs += [("tv_%d.jt" % rng.randrange(1001), "tv_%d.jt" % rng.randrange(1001)) for _ in range(150)]


def run(cmd):
    p = subprocess.run(cmd, cwd=TT, capture_output=True, text=True, timeout=600)
    return p.returncode, p.stdout.strip()


def one(pr):
    a, b = pr
    return pr, run(["java", "-jar", JAR, a, b, "./"]), run(["java", "-cp", CP, SRC, a, b, "./"])


same = close = bad = 0
maxrel = 0.0
with ThreadPoolExecutor(10) as ex:
    for (a, b), (rj, oj), (rs, os_) in ex.map(one, pairs):
        if rj or rs:
            bad += 1
            print("NONZERO", a, b, rj, rs, oj[:60], os_[:60])
        elif oj == os_:
            same += 1
        else:
            x, y = float(oj), float(os_)
            rel = abs(x - y) / max(abs(x), abs(y), 1e-300)
            maxrel = max(maxrel, rel)
            close += 1
            if rel > 1e-12:
                print("DIFF", a, b, oj, os_)
print("pairs %d | identical %d | differ only in last digits %d (max rel %.2e) | errors %d"
      % (len(pairs), same, close, maxrel, bad))


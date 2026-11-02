"""Runs each program as the original JAR, as a .java file in source mode, and as the
same .java compiled once to classes, on many tree pairs, and stores the results.

usage: python run_tests.py <repo dir> <test dir> <phase: main|errors|determinism>
"""
import glob
import itertools
import json
import os
import random
import re
import shutil
import subprocess
import sys
import time
from concurrent.futures import ThreadPoolExecutor

REPO, T, PHASE = sys.argv[1], sys.argv[2], sys.argv[3]
CWDIR = os.path.join(REPO, "CustomisedWasserstein")  # extracted jgrapht/jheaps classes
JARS = os.path.join(T, "jars")
WORKERS = 10

PROGS = {
    "CW": ("CW-MTED.jar", "CW-MTED.java", ["forwardmapCW.txt", "backwardmapCW.txt"]),
    "CO": ("CO-MTED.jar", "CO-MTED.java", ["forwardmapCO.txt", "backwardmapCO.txt"]),
    "BN": ("CustomisedBottleneck.jar", "CustomisedBottleneck.java", []),
    "WS": ("CustomisedWasserstein.jar", "CustomisedWasserstein.java", []),
    "SM": ("SeeMatching.jar", "SeeMatching.java", []),
}


def compile_classes():
    for p, (_, src, _) in PROGS.items():
        out = os.path.join(T, "cls", p)
        os.makedirs(out, exist_ok=True)
        # javac (unlike source mode) wants the public class in ConstrainedEditDist.java; same bytes
        srcdir = os.path.join(T, "cls_src", p)
        os.makedirs(srcdir, exist_ok=True)
        shutil.copyfile(os.path.join(REPO, src), os.path.join(srcdir, "ConstrainedEditDist.java"))
        subprocess.run(["javac", "-nowarn", "-cp", CWDIR, "-d", out, os.path.join(srcdir, "ConstrainedEditDist.java")],
                       check=True, capture_output=True)


def command(prog, variant, progargs):
    jar, src, _ = PROGS[prog]
    if variant == "jar":
        return ["java", "-jar", os.path.join(JARS, jar)] + progargs
    if variant == "src":
        return ["java", "-cp", CWDIR, os.path.join(REPO, src)] + progargs
    if variant == "cls":
        return ["java", "-cp", os.path.join(T, "cls", prog) + os.pathsep + CWDIR,
                "comparator.ConstrainedEditDist"] + progargs
    raise ValueError(variant)


EXC = re.compile(r'Exception in thread "main" ([\w.$]+)|^error: (.*)$|^([\w.$]+(Exception|Error))', re.M)


def run_one(job):
    prog, variant, case, args, outdir = job
    os.makedirs(outdir, exist_ok=True)
    # normal calls pass both trees + the output folder; the one-arg error case passes only one tree
    cmd = command(prog, variant, list(args) + [outdir] if len(args) == 2 else list(args))
    t0 = time.time()
    try:
        p = subprocess.run(cmd, capture_output=True, text=True, timeout=900)
        rc, so, se = p.returncode, p.stdout, p.stderr
    except subprocess.TimeoutExpired:
        rc, so, se = "TIMEOUT", "", ""
    m = EXC.search(se)
    exc = next((g for g in m.groups() if g), None) if m else None
    maps = {}
    for f in PROGS[prog][2]:
        fp = os.path.join(outdir, f)
        maps[f] = open(fp).read() if os.path.exists(fp) else None
    shutil.rmtree(outdir, ignore_errors=True)
    return dict(prog=prog, variant=variant, case=case, rc=rc, stdout=so.strip(), exc=exc,
                maps=maps, secs=round(time.time() - t0, 2))


def pairs_main():
    rng = random.Random(7)
    ex = sorted(glob.glob(os.path.join(REPO, "Examples", "Inputs", "*.jt")))
    tv = sorted(glob.glob(os.path.join(REPO, "test(W1)", "test_trees", "tv_*.jt")))
    edge = sorted(glob.glob(os.path.join(T, "inputs", "edge", "*.jt")))
    rnd = sorted(glob.glob(os.path.join(T, "inputs", "random", "*.jt")))
    pairs = [("examples", a, b) for a, b in itertools.product(ex, ex)]
    for i in rng.sample(range(len(tv) - 1), 40):
        pairs.append(("tv_adjacent", tv[i], tv[i + 1]))
    for _ in range(20):
        pairs.append(("tv_random", rng.choice(tv), rng.choice(tv)))
    pairs += [("edge", a, b) for a, b in itertools.product(edge, edge)]
    for _ in range(140):
        pairs.append(("random", rng.choice(rnd), rng.choice(rnd)))
    for f in rng.sample(rnd, 10):
        pairs.append(("random_same_file", f, f))
    for _ in range(30):
        pairs.append(("cross", rng.choice(edge), rng.choice(ex + rnd)))
    return pairs


def main():
    if PHASE == "main":
        compile_classes()
        pairs = pairs_main()
        variants = ["jar", "src", "cls"]
        repeat = 1
    elif PHASE == "errors":
        good = os.path.join(T, "inputs", "edge", "e03_star5_distinct.jt")
        pairs = []
        for f in sorted(glob.glob(os.path.join(T, "inputs", "error", "*.jt"))):
            pairs += [("error", f, good), ("error", good, f)]
        pairs.append(("error_missing_file", os.path.join(T, "no_such_file.jt"), good))
        pairs.append(("error_one_arg", good))
        variants = ["jar", "src"]
        repeat = 1
    else:  # determinism: same command repeated
        rnd = sorted(glob.glob(os.path.join(T, "inputs", "random", "r_ints_*.jt")))
        edge = os.path.join(T, "inputs", "edge")
        pairs = [("det", os.path.join(edge, "e04_star8_equal_leaves.jt"), os.path.join(edge, "e03_star5_distinct.jt")),
                 ("det", os.path.join(edge, "e16_integer_ties.jt"), os.path.join(edge, "e06_all_values_equal.jt")),
                 ("det", os.path.join(REPO, "Examples", "Inputs", "Tree1.jt"), os.path.join(REPO, "Examples", "Inputs", "Tree3.jt"))]
        rng = random.Random(3)
        pairs += [("det", rng.choice(rnd), rng.choice(rnd)) for _ in range(9)]
        variants = ["jar", "src", "cls"]
        repeat = 4

    jobs = []
    for k, pr in enumerate(pairs):
        case = {"set": pr[0], "a": pr[1], "b": pr[2] if len(pr) > 2 else None, "idx": k}
        args = pr[1:]
        for prog in PROGS:
            for v in variants:
                for r in range(repeat):
                    out = os.path.join(T, "runs", PHASE, "%s_%s_%d_%d" % (prog, v, k, r))
                    jobs.append((prog, v, dict(case, rep=r), args, out))
    print(len(pairs), "pairs,", len(jobs), "runs", flush=True)
    t0 = time.time()
    with open(os.path.join(T, "results_%s.jsonl" % PHASE), "w") as fh, ThreadPoolExecutor(WORKERS) as ex:
        for i, res in enumerate(ex.map(run_one, jobs)):
            fh.write(json.dumps(res) + "\n")
            if (i + 1) % 500 == 0:
                print(i + 1, "done", int(time.time() - t0), "s", flush=True)
    print("finished in", int(time.time() - t0), "s", flush=True)


main()

"""Run every Java program on the test inputs and save all outputs as .txt files.

Programs (all in Merge_Tree_Comparison unless noted):
  CW  CW-MTED.java
  CO  CO-MTED.java
  BN  CustomisedBottleneck.java
  WS  CustomisedWasserstein.java
  SM  SeeMatching.java
  W1  test(W1)/test_trees/CustomisedWasserstein.java

Input sets:
  examples  every ordered pair of the .jt files in Examples/Inputs (16 files -> 256 pairs)
  w1        tv_0.jt against tv_1.jt .. tv_250.jt (the pairs test.py uses)

Usage (from test(W1)):
  python run_all.py                  # both sets, all programs
  python run_all.py examples         # one set
  python run_all.py w1 --programs W1 CO
  python run_all.py --workers 4
  python run_all.py --name "After .jar replaced by java"
  python run_all.py --name "After CO-MTED converted to cpp" \
      --exe CO=../Converted_Merge_Tree_Comparison/CO-MTED.exe \
      --out ../Converted_Merge_Tree_Comparison/results

--exe PROG=PATH runs a native executable for that program instead of its .java
(same arguments: tree1.jt tree2.jt outputDir). --out sets where results go
(default: test(W1)/results). --name names the run folder (default: a timestamp).

Each run writes a new folder <out>/<name>/ (an existing folder is never overwritten):
  <set>/<PROG>.txt          one line per pair: tree1  tree2  output
                            (SM prints several lines per pair, so SM.txt uses a block per pair)
  <set>/files/<PROG>/<tree1>__<tree2>/
                            files the program wrote (forwardmapCW.txt, backwardmapCO.txt, ...)
  <set>/errors.txt          runs that failed or timed out, with stderr
  summary.txt               settings, Java version, counts per program and set
"""
import argparse
import itertools
import os
import shutil
import subprocess
import sys
import time
from concurrent.futures import ThreadPoolExecutor

HERE = os.path.dirname(os.path.abspath(__file__))           # test(W1)
ROOT = os.path.dirname(HERE)                                 # Merge_Tree_Comparison
LIBS = os.path.join(ROOT, "CustomisedWasserstein")          # jgrapht / jheaps classes

PROGRAMS = {
    "CW": os.path.join(ROOT, "CW-MTED.java"),
    "CO": os.path.join(ROOT, "CO-MTED.java"),
    "BN": os.path.join(ROOT, "CustomisedBottleneck.java"),
    "WS": os.path.join(ROOT, "CustomisedWasserstein.java"),
    "SM": os.path.join(ROOT, "SeeMatching.java"),
    "W1": os.path.join(HERE, "test_trees", "CustomisedWasserstein.java"),
}
MULTILINE = {"SM"}
TIMEOUT = 300  # seconds; inputs with NaN/Infinity hang forever in the original code


def example_pairs():
    d = os.path.join(ROOT, "Examples", "Inputs")
    files = sorted(f for f in os.listdir(d) if f.endswith(".jt"))
    return d, list(itertools.product(files, files))


def w1_pairs():
    d = os.path.join(HERE, "test_trees")
    return d, [("tv_0.jt", "tv_%d.jt" % i) for i in range(1, 251)]


SETS = {"examples": example_pairs, "w1": w1_pairs}


def compile_all(names, build):
    """Compile each .java once (all define comparator.ConstrainedEditDist, so each gets its own folder)."""
    for n in names:
        out = os.path.join(build, n)
        src = os.path.join(build, "src_" + n)
        os.makedirs(src, exist_ok=True)
        # javac wants the public class's name as the file name; the copy is byte-identical
        shutil.copyfile(PROGRAMS[n], os.path.join(src, "ConstrainedEditDist.java"))
        p = subprocess.run(["javac", "-nowarn", "-encoding", "UTF-8", "-cp", LIBS, "-d", out,
                            os.path.join(src, "ConstrainedEditDist.java")],
                           capture_output=True, text=True)
        if p.returncode:
            sys.exit("javac failed for %s:\n%s" % (PROGRAMS[n], p.stderr))


EXES = {}  # program -> native executable, from --exe


def run_one(job):
    prog, build, indir, a, b, outdir = job
    os.makedirs(outdir, exist_ok=True)
    if prog in EXES:
        cmd = [EXES[prog]]
    else:
        cmd = ["java", "-cp", os.pathsep.join([os.path.join(build, prog), LIBS]),
               "comparator.ConstrainedEditDist"]
    cmd += [os.path.join(indir, a), os.path.join(indir, b), outdir + os.sep]
    t = time.time()
    try:
        p = subprocess.run(cmd, capture_output=True, text=True, timeout=TIMEOUT, cwd=outdir)
        rc, out, err = p.returncode, p.stdout.strip(), p.stderr.strip()
    except subprocess.TimeoutExpired:
        rc, out, err = "TIMEOUT", "", "killed after %ds" % TIMEOUT
    if not os.listdir(outdir):
        os.rmdir(outdir)  # keep files/ only for programs that write something
    return prog, a, b, rc, out, err, time.time() - t


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("sets", nargs="*", metavar="SET", help="examples and/or w1 (default: both)")
    ap.add_argument("--programs", nargs="+", choices=list(PROGRAMS), default=list(PROGRAMS))
    ap.add_argument("--workers", type=int, default=max(1, (os.cpu_count() or 2) - 1))
    ap.add_argument("--name", help="name of the run folder (default: timestamp)")
    ap.add_argument("--out", default=os.path.join(HERE, "results"), help="folder that holds run folders")
    ap.add_argument("--exe", action="append", default=[], metavar="PROG=PATH",
                    help="run a native executable for PROG instead of its .java")
    args = ap.parse_args()
    args.sets = args.sets or list(SETS)
    for s in args.sets:
        if s not in SETS:
            ap.error("unknown set %r (choose from %s)" % (s, ", ".join(SETS)))
    for e in args.exe:
        prog, _, path = e.partition("=")
        if prog not in PROGRAMS or not os.path.isfile(path):
            ap.error("bad --exe %r (need PROG=PATH with PROG in %s and an existing file)" % (e, ", ".join(PROGRAMS)))
        EXES[prog] = os.path.abspath(path)
        PROGRAMS[prog] = EXES[prog]

    stamp = time.strftime("%Y%m%d-%H%M%S")
    res = os.path.join(os.path.abspath(args.out), args.name or stamp)
    if os.path.exists(res):
        sys.exit("%s already exists; pick another --name or delete it" % res)
    build = os.path.join(res, "_build")
    os.makedirs(res)
    print("Results:", res)

    compile_all([p for p in args.programs if p not in EXES], build)
    jver = subprocess.run(["java", "-version"], capture_output=True, text=True).stderr.splitlines()[0]

    summary = ["run      %s" % (args.name or stamp), "date     %s" % stamp, "java     %s" % jver,
               "workers  %d" % args.workers, "programs %s" % " ".join(args.programs)]
    summary += ["native   %s = %s" % (p, x) for p, x in EXES.items()] + [""]
    start = time.time()

    for s in args.sets:
        indir, pairs = SETS[s]()
        sdir = os.path.join(res, s)
        jobs = [(prog, build, indir, a, b,
                 os.path.join(sdir, "files", prog, "%s__%s" % (a[:-3], b[:-3])))
                for prog in args.programs for a, b in pairs]
        print("%s: %d pairs x %d programs = %d runs" % (s, len(pairs), len(args.programs), len(jobs)))

        results = {p: [] for p in args.programs}
        done = 0
        with ThreadPoolExecutor(args.workers) as ex:
            for r in ex.map(run_one, jobs):
                results[r[0]].append(r)
                done += 1
                if done % 50 == 0 or done == len(jobs):
                    print("  %d/%d" % (done, len(jobs)), flush=True)

        errors = []
        summary.append("[%s]  inputs: %s  pairs: %d" % (s, indir, len(pairs)))
        for prog in args.programs:
            rows = results[prog]
            with open(os.path.join(sdir, prog + ".txt"), "w", encoding="utf-8", newline="\n") as f:
                f.write("# %s  %s\n" % (prog, PROGRAMS[prog]))
                if prog in MULTILINE:
                    for _, a, b, rc, out, _, _ in rows:
                        f.write("### %s %s%s\n%s\n\n" % (a, b, "" if rc == 0 else "  [exit %s]" % rc, out))
                else:
                    f.write("# tree1\ttree2\toutput\n")
                    for _, a, b, rc, out, _, _ in rows:
                        f.write("%s\t%s\t%s\n" % (a, b, out if rc == 0 else "ERROR(exit %s)" % rc))
            bad = [r for r in rows if r[3] != 0]
            errors += bad
            summary.append("  %-3s ok %4d  failed %3d  avg %.2fs" % (
                prog, len(rows) - len(bad), len(bad), sum(r[6] for r in rows) / max(len(rows), 1)))

        with open(os.path.join(sdir, "errors.txt"), "w", encoding="utf-8", newline="\n") as f:
            if not errors:
                f.write("no errors\n")
            for prog, a, b, rc, out, err, _ in errors:
                f.write("=== %s %s %s  exit %s\n%s\n%s\n\n" % (prog, a, b, rc, out, err))
        summary.append("")

    summary.append("total time %.0fs" % (time.time() - start))
    shutil.rmtree(build, ignore_errors=True)
    with open(os.path.join(res, "summary.txt"), "w", encoding="utf-8", newline="\n") as f:
        f.write("\n".join(summary) + "\n")
    print("\n".join(summary))


if __name__ == "__main__":
    main()

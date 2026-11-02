"""Summarises results_<phase>.jsonl. usage: python analyze.py <test dir> <phase>"""
import collections
import json
import os
import sys

T, PHASE = sys.argv[1], sys.argv[2]
rows = [json.loads(l) for l in open(os.path.join(T, "results_%s.jsonl" % PHASE))]


def util_parse(s):
    """Exactly what Utilities.SeeMatching does with the program's stdout."""
    for ch in "[]():,":
        s = s.replace(ch, "")
    lst = s.split()
    return [(int(lst[i]), int(lst[i + 1])) for i in range(0, len(lst) - 1, 2)]


def num(s):
    try:
        return float(s.splitlines()[-1])
    except Exception:
        return None


def name(case):
    b = os.path.basename
    return "%s | %s | %s" % (case["set"], b(case["a"]), b(case["b"]) if case["b"] else "-")


def rel(x, y):
    if x is None or y is None:
        return float("inf")
    if x == y or (x != x and y != y):
        return 0.0
    return abs(x - y) / max(abs(x), abs(y), 1e-300)


groups = collections.defaultdict(dict)
for r in rows:
    groups[(r["prog"], r["case"]["idx"], r["case"].get("rep", 0))][r["variant"]] = r

if PHASE == "determinism":
    stab = collections.defaultdict(lambda: collections.defaultdict(set))
    for r in rows:
        stab[(r["prog"], r["variant"])][r["case"]["idx"]].add(r["stdout"] + json.dumps(r["maps"], sort_keys=True))
    print("Determinism (same command run 4 times; stable = all 4 outputs identical)")
    for (prog, v), cases in sorted(stab.items()):
        n = len(cases)
        ok = sum(1 for s in cases.values() if len(s) == 1)
        print("  %-3s %-4s stable %d/%d" % (prog, v, ok, n))
    sys.exit()

if PHASE == "errors":
    print("Error cases: jar vs src (exit code, exception type, stdout)")
    for (prog, idx, _), vs in sorted(groups.items()):
        j, s = vs["jar"], vs["src"]
        same = (j["rc"] == s["rc"]) and (j["exc"] == s["exc"]) and (j["stdout"] == s["stdout"])
        print("  %-3s %-55s jar rc=%s %-40s src rc=%s %-40s %s" % (
            prog, name(j["case"])[:55], j["rc"], (j["exc"] or "") + (" out=" + j["stdout"][:12] if j["stdout"] else ""),
            s["rc"], (s["exc"] or "") + (" out=" + s["stdout"][:12] if s["stdout"] else ""), "SAME" if same else "DIFF"))
    sys.exit()

for prog in ["CW", "CO", "BN", "WS", "SM"]:
    cases = [(k, v) for k, v in groups.items() if k[0] == prog]
    print("=" * 100)
    print("%s: %d pairs" % (prog, len(cases)))
    secs = collections.defaultdict(list)
    for _, vs in cases:
        for v, r in vs.items():
            secs[v].append(r["secs"])
    print("  avg seconds/run: " + ", ".join("%s %.2f" % (v, sum(x) / len(x)) for v, x in sorted(secs.items())))
    fails = [(name(vs["jar"]["case"]), v, r["rc"], r["exc"]) for _, vs in cases for v, r in vs.items() if r["rc"] != 0]
    print("  runs with non-zero exit: %d" % len(fails))
    for f in fails[:8]:
        print("     ", f)
    for other in ["src", "cls"]:
        exact = sum(1 for _, vs in cases if vs["jar"]["stdout"] == vs[other]["stdout"] and vs["jar"]["rc"] == vs[other]["rc"])
        line = "  jar vs %s: identical stdout %d/%d" % (other, exact, len(cases))
        if prog in ("CW", "CO", "BN", "WS"):
            diffs = [(rel(num(vs["jar"]["stdout"]), num(vs[other]["stdout"])), name(vs["jar"]["case"]),
                      vs["jar"]["stdout"], vs[other]["stdout"]) for _, vs in cases
                     if vs["jar"]["stdout"] != vs[other]["stdout"]]
            worst = max((d[0] for d in diffs), default=0.0)
            line += " | differing: %d, max relative diff %.3g" % (len(diffs), worst)
            bysets = collections.Counter(d[1].split(" | ")[0] for d in diffs)
            if bysets:
                line += " | by set " + dict(bysets).__repr__()
            print(line)
            for d in sorted(diffs, reverse=True)[:5]:
                print("      rel %.2g  %s  jar=%s %s=%s" % (d[0], d[1], d[2], other, d[3]))
        if prog in ("CW", "CO"):
            mexact = sum(1 for _, vs in cases if vs["jar"]["maps"] == vs[other]["maps"])
            msorted = sum(1 for _, vs in cases if all(
                sorted((vs["jar"]["maps"][f] or "").splitlines()) == sorted((vs[other]["maps"][f] or "").splitlines())
                for f in vs["jar"]["maps"]))
            print("      map files: identical %d/%d, same lines ignoring order %d/%d" % (mexact, len(cases), msorted, len(cases)))
            bad = [name(vs["jar"]["case"]) for _, vs in cases if vs["jar"]["maps"] != vs[other]["maps"]]
            for b in bad[:5]:
                print("        maps differ:", b)
        if prog == "SM":
            mset = rr = 0
            bad = []
            for _, vs in cases:
                pj, po = util_parse(vs["jar"]["stdout"]), util_parse(vs[other]["stdout"])
                if sorted(pj) == sorted(po):
                    mset += 1
                rrj = [p for p in pj if p[0] != -1 and p[1] != -1]
                rro = [p for p in po if p[0] != -1 and p[1] != -1]
                if rrj == rro:
                    rr += 1
                else:
                    bad.append((name(vs["jar"]["case"]), rrj, rro, sorted(pj) == sorted(po)))
            print(line)
            print("      same matched pairs (order ignored): %d/%d" % (mset, len(cases)))
            print("      same order of real-real pairs (what decides the WM*.png colours): %d/%d" % (rr, len(cases)))
            for b in bad[:6]:
                print("        %s\n           jar %s\n           %s %s  (same set: %s)" % (b[0], b[1], other, b[2], b[3]))
            if not bad:
                pass
    if prog == "SM":
        over6 = sum(1 for _, vs in cases if vs["jar"]["case"]["a"] != vs["jar"]["case"]["b"] and
                    len([p for p in util_parse(vs["jar"]["stdout"]) if p[0] != -1 and p[1] != -1]) > 6)
        print("  pairs with >6 real-real matches (Utilities.SeeMatching runs out of its 6 colours, in both): %d" % over6)

"""End-to-end Driver.py runs in a repo copy, and comparison of two copies.

usage: python e2e.py run <repo copy>
       python e2e.py compare <before copy> <after copy>
"""
import hashlib
import os
import subprocess
import sys
import time

MAKEFILES = ["ExampleMakeFile_PairWise", "ExampleMakeFile_Sequential", "ExampleMakeFile_1vM", "E2E_AllPairs"]
PY = r"C:\Users\saias\AppData\Local\Programs\Python\Python313\python.exe"
SKIP_EXT = {".cpp", ".h", ".py", ".pyd", ".so", ".jar", ".class", ".java", ".pyc", ".txt_", ".md"}


def run(repo):
    env = dict(os.environ)
    env["PATH"] = os.path.dirname(PY) + os.pathsep + r"C:\msys64\ucrt64\bin" + os.pathsep + env["PATH"]
    with open(os.path.join(repo, "e2e_log.txt"), "w") as log:
        for mk in MAKEFILES:
            t0 = time.time()
            p = subprocess.run([PY, "Driver.py", "Examples/" + mk], cwd=repo, env=env, capture_output=True, text=True)
            tail = "\n".join(l for l in p.stderr.splitlines() if "warning" not in l.lower() and "Note:" not in l)[-1500:]
            log.write("=== %s exit=%s secs=%d\n%s\n" % (mk, p.returncode, time.time() - t0, tail))
            log.flush()


def files(root):
    out = {}
    for d, dirs, fs in os.walk(root):
        dirs[:] = [x for x in dirs if x not in (".git", "__pycache__", "org", "comparator", "test(W1)")]
        for f in fs:
            p = os.path.join(d, f)
            rel = os.path.relpath(p, root)
            if os.path.splitext(f)[1] in SKIP_EXT or f == "e2e_log.txt":
                continue
            out[rel] = p
    return out


def compare(a, b):
    from PIL import Image, ImageChops
    fa, fb = files(a), files(b)
    print("files: before %d, after %d" % (len(fa), len(fb)))
    only_a = sorted(set(fa) - set(fb))
    only_b = sorted(set(fb) - set(fa))
    print("only in before:", only_a[:20])
    print("only in after:", only_b[:20])
    same = textdiff = pngsame_pixels = pngdiff = 0
    diffs = []
    for rel in sorted(set(fa) & set(fb)):
        da, db = open(fa[rel], "rb").read(), open(fb[rel], "rb").read()
        if da == db:
            same += 1
            continue
        if rel.lower().endswith(".png"):
            ia, ib = Image.open(fa[rel]).convert("RGB"), Image.open(fb[rel]).convert("RGB")
            if ia.size == ib.size and ImageChops.difference(ia, ib).getbbox() is None:
                pngsame_pixels += 1
                continue
            pngdiff += 1
            diffs.append((rel, "PNG pixels differ (size %s vs %s)" % (ia.size, ib.size)))
        else:
            textdiff += 1
            la, lb = da.decode(errors="replace").splitlines(), db.decode(errors="replace").splitlines()
            first = next((i for i in range(max(len(la), len(lb)))
                          if (la[i] if i < len(la) else None) != (lb[i] if i < len(lb) else None)), None)
            show = lambda l, i: l[i][:110] if i is not None and i < len(l) else "<none>"
            diffs.append((rel, "line %s:\n        before: %s\n        after:  %s" % (first, show(la, first), show(lb, first))))
    print("byte-identical: %d | PNG identical pixels (bytes differ): %d | PNG pixels differ: %d | other files differ: %d"
          % (same, pngsame_pixels, pngdiff, textdiff))
    for rel, why in diffs:
        print("  DIFF", rel, "-", why)


if sys.argv[1] == "run":
    run(sys.argv[2])
else:
    compare(sys.argv[2], sys.argv[3])

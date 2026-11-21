"""Makes a Wasserstein matching picture (WM*.png) for EVERY pair of example trees.

Run from this folder:
    python make_matching_pictures.py

It uses the same drawing code as Driver.py (Utilities.SeeMatching), which calls
SeeMatching.exe. Matched features get the same colour in both trees.

Output, sorted by whether the picture has colours:
    SeeMatching_Pictures/coloured/WM<tree1><tree2>.png
    SeeMatching_Pictures/no_colour/WM<tree1><tree2>.png
    SeeMatching_Pictures/list.txt   (pair, number of coloured matches)

"no_colour" means every feature was matched to the diagonal ("disappears"),
usually because the two trees have very different value ranges
(e.g. TreeA goes 0..100, TreeB goes 0..-1).
"""
import os
import re
import shutil
import subprocess

import Utilities

INPUTS = "./Examples/Inputs"
OUT = "SeeMatching_Pictures"


def main():
    if not os.path.exists("SeeMatching.exe"):
        raise SystemExit("SeeMatching.exe not found: run this from Converted_Merge_Tree_Comparison (and run build.sh first)")
    trees = sorted(f[:-3] for f in os.listdir(INPUTS) if f.endswith(".jt"))
    for sub in ("coloured", "no_colour"):
        os.makedirs(os.path.join(OUT, sub), exist_ok=True)

    lines = ["# tree1\ttree2\tcoloured_matches\tpicture"]
    counts = {"coloured": 0, "no_colour": 0}
    for t1 in trees:
        for t2 in trees:
            if t1 == t2:
                continue            # Utilities.SeeMatching skips a tree against itself
            f1, f2 = INPUTS + "/" + t1, INPUTS + "/" + t2
            out = subprocess.run([os.path.abspath("SeeMatching.exe"), f1 + ".jt", f2 + ".jt", "./"],
                                 capture_output=True, text=True).stdout
            pairs = re.findall(r"\((-?\d+)\[\] : (-?\d+)\[\]\)", out)
            n = sum(1 for a, b in pairs if a != "-1" and b != "-1")

            Utilities.SeeMatching(f1, f2)                   # writes ./Examples/WM<t1><t2>.png
            name = "WM" + t1 + t2 + ".png"
            sub = "coloured" if n else "no_colour"
            shutil.move(os.path.join("Examples", name), os.path.join(OUT, sub, name))
            counts[sub] += 1
            lines.append("%s\t%s\t%d\t%s/%s" % (t1, t2, n, sub, name))

    with open(os.path.join(OUT, "list.txt"), "w") as f:
        f.write("\n".join(lines) + "\n")
    print("coloured: %d   no colour: %d   ->  %s" % (counts["coloured"], counts["no_colour"], OUT))


if __name__ == "__main__":
    main()

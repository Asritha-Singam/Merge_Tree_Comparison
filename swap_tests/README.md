# JAR vs .java tests

These tests check that replacing each JAR in `Merge_Tree_Comparison` with a `.java` file (see `Merge_Tree_Comparison/Changes.md`) gives the same results.

## Contents

| Path | What it is |
|---|---|
| `inputs/edge/` | 18 edge-case trees: single node, two nodes, stars with equal or distinct leaves, all values equal, negative, huge and tiny values, mixed magnitudes, 40-leaf caterpillar, large shuffled IDs, CRLF/tabs/scientific notation, degree-1 chains, 100-leaf tree, integer ties, zero-persistence leaves, high-degree saddles |
| `inputs/random/` | 120 random trees in three kinds: `r_cont_*` (continuous values), `r_ints_*` (integers, many ties), `r_wild_*` (wide value ranges) |
| `inputs/error/` | 9 broken inputs: bad number, empty, truncated, missing child, missing pair, no root (cycle), zero nodes, NaN, Infinity |
| `jars/` | The 5 original JARs, restored from git history, used as the reference to compare against |
| `gen_inputs.py` | Generates `inputs/` |
| `run_tests.py` | Runs each program as the JAR, as the `.java` file (source mode) and as the `.java` compiled once, then writes `results_<phase>.jsonl` |
| `analyze.py` | Summarises a `results_<phase>.jsonl` |
| `e2e.py` | Runs the `Driver.py` example makefiles in a repo copy, then compares two copies file by file |
| `results/` | Results of the runs reported in `Changes.md`, plus `killed_hung.txt` (NaN/Infinity runs that hung and were stopped) |
| `w1/cmp_w1.py`, `w1/w.jar` | The comparison for `test(W1)`: its original JAR against `test(W1)/test_trees/CustomisedWasserstein.java` |

Programs in the results are labelled CW = `CW-MTED`, CO = `CO-MTED`, BN = `CustomisedBottleneck`, WS = `CustomisedWasserstein`, SM = `SeeMatching`.

## What was run

| Test | Inputs | Result |
|---|---|---|
| main | 820 pairs: all 256 pairs of the 16 `.jt` files in `Examples/Inputs` (8 trees and their normalised versions), 60 `tv_*.jt` pairs, all 324 edge × edge pairs, 150 random pairs (10 of them a file against itself), 30 edge × other pairs | BN identical 820/820. CW, CO and WS distances match to within ~1e-15. When several optimal matchings exist, the chosen one can differ, and the original JARs show the same variation when Java is launched differently. |
| errors | 9 broken files in both argument positions, a missing file, too few arguments | Same exit code and exception for the JAR and the `.java` in every case. NaN/Infinity hang CW/CO/WS/SM in both versions. |
| determinism | 12 pairs, each run 4 times per variant | Every program gives the same output on every repeat |
| e2e | `ExampleMakeFile_PairWise`, `_Sequential`, `_1vM`, and an all-pairs run on 8 trees with every measure | 820 of 821 output files byte-identical. One Wasserstein value differs only in its last digit (~1e-16). |
| w1 | 400 `tv_*.jt` pairs (the 250 `test.py` uses plus 150 random) | 399 identical, 1 differs only in its last digit |

## How to rerun

Run these from this folder (`swap_tests`) with Python 3 and JDK 11+. `..` is the repository root.

The main test also uses the 16 `.jt` files in `../Examples/Inputs` and the `tv_*.jt` files in `../test(W1)/test_trees`, both committed in the repository. If any of them is missing, the main test silently runs fewer than 820 pairs.

```
python gen_inputs.py inputs                  # optional: regenerate the inputs
python run_tests.py .. . main                # also: errors, determinism
python analyze.py . main                     # summarise a new run
python analyze.py results main               # summarise the saved run
python w1/cmp_w1.py ..                       # the test(W1) comparison
```

`run_tests.py` writes `results_<phase>.jsonl` into this folder, plus temporary `runs/`, `cls/` and `cls_src/` folders. The main phase is 12,300 Java runs and takes about 40 minutes with 10 parallel workers. Inputs containing NaN or Infinity hang forever (a bug in the original code). `run_tests.py` stops each such run after 15 minutes and records it as `TIMEOUT`, so the errors phase finishes on its own, slowly.

Rerun results should agree with `results/`, but are not guaranteed to be byte-identical: where several matchings are equally optimal, which one is chosen can vary.

`e2e.py run <copy>` needs a full copy of the repository, with the Python library built and Graphviz `dot` on `PATH`. Its interpreter path is hard-coded at the top of the file (`PY`). Run it once on a copy that still has the JARs and once on the current code, then use `python e2e.py compare <before copy> <after copy>`.

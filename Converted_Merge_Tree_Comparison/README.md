# Converted Merge Tree Comparison

This folder is a copy of the main project, but with the slow Java programs
replaced by C++ programs. The main (root) folder is **not changed**.

## What is the idea?

The project compares two merge trees and gives a number: how different they are.
There are four ways ("distances") to compare them:

| Distance     | Old way (root folder) | New way (this folder)       |
|--------------|-----------------------|-----------------------------|
| Bottleneck   | Java                  | C++ (`CustomisedBottleneck.exe`) |
| CO-MTED      | Java                  | C++ (`CO-MTED.exe`)         |
| CW-MTED      | Java                  | C++ (`CW-MTED.exe`)         |
| Wasserstein  | Java                  | C++ (`CustomisedWasserstein.exe`) |

The small helper that tells which points are matched (used to draw the
Wasserstein pictures `Examples/WM*.png`) is also C++ now: `SeeMatching.exe`
(was `SeeMatching.java`). So **no Java is used at all** in this folder.

The C++ programs give **the same numbers** as the Java programs (see
"Did we check it?" subheading below). They are faster, because Java has to start up
every time it is called.

## How it works (step by step)

1. You run `Driver.py` with a small text file (a "config") that says which
   trees to compare and which distances to use.
2. `Driver.py` uses `Utilities.py`, which calls `MergeTreeLibrary.pyd`.
3. `MergeTreeLibrary.pyd` is C++ code that Python can use. It reads the trees,
   writes them as `.jt` files, and then runs a program to get the distance.
4. **This is the part we changed.** `TreePair.cpp` (in both folders) picks
   the program: Java in the root folder, the C++ `.exe` here.
5. The results are saved as CSV files (`PairsOutput`, `SingleOutput`).

## What we changed in this folder

- **`TreePair.cpp`** – all four distances now run the `.exe` files in this
  folder instead of Java.
- **`CustomisedWasserstein.cpp`** – new: the C++ version of
  `CustomisedWasserstein.java`.
- **`SeeMatching.cpp`** – new: the C++ version of `SeeMatching.java`. It does
  the same matching as `CustomisedWasserstein.cpp`, but instead of the total
  cost it prints the matched pairs, in the same text form as Java:
- **`Utilities.py`** – the picture helper now runs `SeeMatching.exe` instead
  of `SeeMatching.java`. We also fixed a bug there: the old call could not
  start the program on Windows (the whole command was treated as one program
  name), so it got no matching and the `WM*.png` pictures had **no colours**.
  Now matched features get the same colour in both trees.
- **`Examples/`** – a copy of the example trees and config files from the root.
  This way, all new output files are written here, not into the root folder.
- **`build.py`** – one script that builds all the C++ parts (`build.sh` does
  the same, for Git Bash users).
- **`MergeTreeLibrary.pyd`** – rebuilt from the changed `TreePair.cpp`.

## How to build

Open a terminal (PowerShell, cmd or Git Bash) **in this folder** and run:

```sh
python build.py #for terminal or Powershell
sh build.sh     #for git bash
```

This makes `CustomisedBottleneck.exe`, `CustomisedWasserstein.exe`,
`SeeMatching.exe`, `CO-MTED.exe`, `CW-MTED.exe` and `MergeTreeLibrary.pyd`.
You need: `g++` (MSYS2) on PATH, Python, and `pip install pybind11`.

If it says **"MergeTreeLibrary.pyd is in use"**: some Python program still
has it open (for example a running `Driver.py`). Close it and build again.

## How to run

Always run from **this folder** (otherwise the `.exe` files are not found):

```sh
cd Converted_Merge_Tree_Comparison
python Driver.py Examples/ExampleMakeFile_PairWise
python Driver.py Examples/ExampleMakeFile_1vM
python Driver.py Examples/ExampleMakeFile_Sequential
```

The results appear in `Examples/RunPairwise`, `Examples/Run1vM` and
`Examples/RunSeqs`.

Also needed: Graphviz `dot` (for tree pictures) and the Python packages
`matplotlib`, `pillow`, `cairosvg`.

## Matching pictures for all example pairs

`Driver.py` only draws matching pictures for the pairs in a config (the 1vM
config draws just 4). To draw one for **every** pair of example trees:

```sh
python make_matching_pictures.py
```

This makes 240 pictures (16 trees x 15 others) in `SeeMatching_Pictures/`:
- `coloured/` (130 pictures): at least one feature of tree 1 is matched to a
  feature of tree 2. Matched features have the same colour in both trees.
- `no_colour/` (110 pictures): every feature was matched to the diagonal
  ("disappears"), so there is nothing to colour.
- `list.txt`: every pair and how many coloured matches it has.

Why do some pairs have no colours? Their trees have very different value
ranges. TreeA goes from 0 to 100, but TreeB, TreeC and TreeE go from 0 to -1.
Matching a feature across such different ranges costs far more than letting
both features "disappear", so the cheapest matching uses only the diagonal.
That is why, in the 1vM example (TreeA against B, C, D, E), only
`WMTreeATreeD.png` has colours. Good examples to look at:
`coloured/WMTreeBTreeE.png`, `coloured/WMTree1Tree3.png`.

## Did we check it?

### How we verified

- **Same inputs, both versions.** Every program was run on the same 506 tree
  pairs, once in Java and once in C++:
  - `examples`: every ordered pair of the 16 trees in `Examples/Inputs` (256 pairs)
  - `w1`: `tv_0` against `tv_1` … `tv_250` from `test(W1)/test_trees` (250 pairs)
- **Java baseline:** `test(W1)/run_all.py` ran all Java programs and saved
  their outputs in `test(W1)/results/after_jar_to_java/`.
- **C++ runs:** the same script with `--exe PROG=<file>.exe` runs the C++
  program instead of Java and saves the outputs in `results/after_<program>_to_cpp/`.
  SeeMatching is the exception: `results/after_seematching_to_cpp/compare_seematching.py`
  runs Java and C++ itself and compares them.
- **Compared pair by pair:** distances as numbers (`BN`, `CO`, `CW`, `WS`),
  map files and SeeMatching outputs as sets of matched pairs (order ignored).
  Where pairs differed, we checked that both answers have the same total cost
  (a tie, not an error).
- **Whole pipeline:** `Driver.py` was run on the example configs here and its
  `PairsOutput` / `SingleOutput` files compared with the root `Examples/Run*`.

### Where the outputs are

| Folder | What it holds |
|--------|---------------|
| `test(W1)/results/after_jar_to_java/` (root) | Java outputs of all programs: the baseline |
| `results/after_bottleneck_to_cpp/` | C++ Bottleneck outputs (`BN.txt`) |
| `results/after_comted_to_cpp/` | C++ CO-MTED outputs (`CO.txt`, map files in `files/CO/`) |
| `results/after_cwmted_to_cpp/` | C++ CW-MTED outputs (`CW.txt`, map files in `files/CW/`) |
| `results/after_wasserstein_to_cpp/` | C++ Wasserstein outputs (`WS.txt`) |
| `results/after_seematching_to_cpp/` | Java and C++ SeeMatching outputs (`SM_java.txt`, `SM_cpp.txt`), the result per pair (`comparison.txt`) and the script (`compare_seematching.py`) |
| root `walkthroughs/` | Pair-by-pair pairing differences for CO-MTED and CW-MTED, and step-by-step traces |

Each `results/` folder has `examples/` and `w1/` subfolders (one per input
set) and a `summary.txt` with the settings and counts. The folders made by
`run_all.py` also have an `errors.txt` per set listing failed runs; all of
them say `no errors`.

To repeat a C++ run (from `test(W1)`), for example for CO-MTED:
`python run_all.py --programs CO --exe CO=../Converted_Merge_Tree_Comparison/CO-MTED.exe --name my_run --out ../Converted_Merge_Tree_Comparison/results`

### Results

We ran the three example configs here and compared the results with the
old Java results in the root `Examples/` folder:

| Example     | Result |
|-------------|--------|
| Pairwise    | same numbers |
| 1 vs Many   | same numbers (3 Wasserstein values differ only in the last digit, e.g. `4194.4194` vs `4194.419400000001`) |
| Sequential  | same numbers |

We also compared each C++ program against Java on 506 tree pairs
(256 from `examples`, 250 from `w1`). The results are in `results/`.

| Program    | Result |
|------------|--------|
| Bottleneck | exactly the same on all 506 pairs |
| CO-MTED    | same on all pairs; on 20 `w1` pairs the very last decimal digit is different (difference smaller than 0.0000000000000003) |
| CW-MTED    | same on all pairs; on 27 `w1` pairs the very last decimal digit is different (difference smaller than 0.0000000000000002) |
| Wasserstein | same on all pairs; on 32 `examples` and 169 `w1` pairs the very last decimal digit is different (relative difference smaller than 0.000000000000001) |

That tiny last-digit difference is normal for computers: adding the same
decimal numbers in a slightly different order can change the final digit.
It does not change the real answer.

For Wasserstein, Java itself is not stable in that last digit. Java adds the
costs in an order that depends on its internal object "hash" numbers. We saw
this: the same pair (`tv_0` vs `tv_55`) gave `...750011` when Java was run as
`java CustomisedWasserstein.java`, but `...750014` when the test script ran it.
So the C++ and Java answers are as equal as two Java runs are.

For SeeMatching we compared **which pairs are matched** (the order in which
Java prints them is not stable, so order is ignored):

| Test set | Same matching | Different matching, but same total cost |
|----------|---------------|------------------------------------------|
| examples | 256 of 256    | 0 |
| w1       | 249 of 250    | 1 |

All outputs are saved in `results/after_seematching_to_cpp/` (Java output,
C++ output and the result for every pair). To run this check again:
`python results/after_seematching_to_cpp/compare_seematching.py`.

That one `w1` case (`tv_0` vs `tv_250`) is a "tie": two different matchings have exactly the same
lowest cost, so both are correct; Java and C++ just picked a different one.
(In the pictures this can only change which nodes get which colour.)

### CO-MTED and CW-MTED pairings (which node is matched to which)

The table above for CO-MTED and CW-MTED only compares the **distance number**.
These programs also write map files (`forwardmapCO.txt`, `forwardmapCW.txt`,
...) that say which node of tree 1 is matched to which node of tree 2. Those
were compared too, in the root `walkthroughs/` folder
(`CO-MTED_java_vs_cpp_pairing_diff.txt`, `CW-MTED_java_vs_cpp_pairing_diff.txt`).
"Same pairs" counts files with the same lines, in any order.

**CO-MTED**

| Test set | Same pairs | Different pairs, but same total cost |
|----------|------------|---------------------------------------|
| examples | 229 of 256 | 27 |
| w1       | 48 of 250  | 202 |

**CW-MTED**

| Test set | Same pairs | Different pairs, but same total cost |
|----------|------------|---------------------------------------|
| examples | 224 of 256 | 32 |
| w1       | 48 of 250  | 202 |

Every different case is a **tie**: the Java pairs and the C++ pairs both cost
exactly the distance and are both valid, so neither is wrong. There are just
several equally good answers, and the two programs pick a different one.
In `examples`, C++ always only leaves out some pairs that Java keeps (for
example `4,4` in Tree1 vs TreeC).

Why so many more than SeeMatching?
- **CO/CW map every node** (leaves and saddles), and build the answer from
  many small matching steps along the tree. A tie in any step can change the
  final map, and `w1` trees have 74 nodes, so there are many such steps.
- **CW's cost often ties exactly.** When two trees have very different value
  ranges, matching two nodes costs exactly the same as deleting one and
  inserting the other, so every mapping has the same total cost
  (see `walkthroughs/CW-MTED_Tree1_vs_TreeC_java_vs_cpp.md`, section 3).
- **SeeMatching hides most of its ties.** A leaf matched to the diagonal is
  printed as `-1`, without saying *which* diagonal point, so different choices
  look the same. CO/CW print a real node ID for every pair, so every tie shows.
- **Java CO is not fixed itself.** On ties its choice can change with how Java
  is started (Tree1 vs TreeC: `3,3 2,2 1,1` from bash, `3,3 4,4 2,2 1,1` from
  Python).

Step-by-step traces of one pair, showing exactly where Java and C++ split:
`walkthroughs/CW-MTED_Tree1_vs_TreeC_java_vs_cpp.md`.

### Kuhn-Munkres test (on its own)

Kuhn-Munkres finds the cheapest way to pair up two lists (for example, which
children of one node go with which children of another). We wrote our own in
C++ (`common/KuhnMunkres.h`), so we tested it on its own against JGraphT, the
Java library the original code uses.

We gave both 756 random cost tables (1x1 to 200x200):

| Kind of table | Tables | Same total cost | Same pairing |
|---|---|---|---|
| normal random costs | 378 | 378 | 378 |
| costs with many ties | 378 | 378 | 157 |
| **Total** | **756** | **756** | **535** |

- **Same total cost every time**, so our C++ is correct.
- **The pairing differs only when ties exist**: several pairings cost exactly
  the same, and Java and C++ pick a different one. Both are correct.
- This is why the full programs give the same distances as Java but sometimes
  different CO-MTED/CW-MTED pairings.

### Memory test (Java vs C++)

Peak memory (the most RAM the process used at any moment), measured on the
**same 506 test pairs** as all other checks (256 `examples` + 250 `w1`), for
every program, default Java settings. All 2,024 runs per language finished
and gave the same distance in Java and C++.

| Program | Set | Java MB (median / max) | C++ MB (median / max) | Java / C++ (median) |
|---|---|---|---|---|
| CO-MTED     | examples | 46.3 / 47.0  | 5.3 / 5.3 | 9x  |
| CO-MTED     | w1       | 109.5 / 122.2 | 5.3 / 5.3 | 21x |
| CW-MTED     | examples | 46.3 / 47.1  | 5.3 / 5.3 | 9x  |
| CW-MTED     | w1       | 113.6 / 132.2 | 5.3 / 5.3 | 21x |
| Bottleneck  | examples | 44.0 / 45.1  | 5.2 / 5.2 | 8x  |
| Bottleneck  | w1       | 69.5 / 83.3  | 5.3 / 5.3 | 13x |
| Wasserstein | examples | 44.2 / 44.9  | 5.2 / 5.2 | 8x  |
| Wasserstein | w1       | 54.1 / 59.8  | 5.3 / 5.3 | 10x |

- On the test pairs Java uses **8 to 21 times** more memory than C++.
- The test trees are small (4 to 84 nodes), so about 40 MB of Java's number is
  the Java runtime itself. C++ stays at about 5 MB for every pair.
- Java does not give memory back right away, so its peak is higher than what
  it strictly needs.
- Main reason for CO/CW-MTED: Java copies a whole node mapping (a `HashMap`)
  into every table cell; C++ stores one number per cell and rebuilds the
  mapping once at the end.

**Bigger trees (extra check).** To see how this grows, the same programs were
run on random trees made for this test (one pair per size, same distance in
Java and C++):

| Nodes per tree | CO-MTED Java / C++ | CW-MTED Java / C++ | Bottleneck Java / C++ | Wasserstein Java / C++ |
|---|---|---|---|---|
| 200 | 260 / 6 MB | 255 / 6 MB | 166 / 5 MB | 99 / 5 MB |
| 400 | 638 / 9 MB | 650 / 9 MB | 782 / 7 MB | 137 / 6 MB |
| 800 | 1369 / 20 MB | 1596 / 20 MB | 1926 / 12 MB | 286 / 10 MB |

At 800 nodes the gap is 29 to 160 times, and Java takes 6 to 12 s per pair
against 0.1 to 0.2 s for C++.

### Status of the C++ conversion plan

| Plan item | Status | Where |
|---|---|---|
| Write Kuhn-Munkres ourselves in C++ | Done | `common/KuhnMunkres.h`: O(n³) Hungarian on a flat cost array. Used by CO-MTED, CW-MTED, Wasserstein and SeeMatching. `common/HopcroftKarp.h` (also our own) is used by Bottleneck |
| Correct / validated against Java | Done | In the programs this shows up as the same distances on all 506 test pairs (some differ only in the last digit), while CO/CW-MTED pairings often differ, always by equal-cost ties ) |
| Efficient | Done | Flat array, O(n) extra memory, no objects |
| Memory advantage over Java | Done, measured | 8 to 21 times less memory on the 506 test pairs; 29 to 160 times on 800-node random trees (memory test above) |
| Flat-array tree representation | Done | `CO-MTED.cpp` lines 37-50: nodes in post-order, values and children in contiguous arrays (`childStart` / `childList`) |

## Small notes

- In `Driver.py`, the number of trees is read as **one digit only**, so use at
  most 9 trees in a config.
- The matching pictures have only **6 colours** (`colors` list in
  `Utilities.py`, `SeeMatching`). If more than 6 features are matched
  leaf-to-leaf, Python stops with an `IndexError`. Features matched to the
  diagonal (they "disappear") are not coloured.


## Future Prospects

Goal: compare **one very large real-world tree pair** by splitting that single
comparison over many machines.

- **Problem:** CO-MTED/CW-MTED fill an n1 x n2 table (24 bytes per cell).
  At 100,000 nodes per tree that is about 240 GB, too big for one machine.
- **CO-MTED / CW-MTED:**
  - Cell (i, j) only needs cells of the children of i or j, so all cells with
    the same height(i) + height(j) can be computed at the same time.
  - Split the table by subtrees of tree 1, one subtree per machine; machines
    only exchange the rows of their subtree roots.
  - If only the distance is needed, finished rows can be freed early.
- **Steps:** measure a real large tree first, then threads on one machine
  (e.g. OpenMP), then several machines (e.g. MPI).
- **Must stay true:** same distances and same node mappings as the current
  C++ code (checked with the scripts in `results/`).

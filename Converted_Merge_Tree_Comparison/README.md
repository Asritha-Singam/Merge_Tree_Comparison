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
"Did we check it?" below). They are faster, because Java has to start up
every time it is called.

## How it works (step by step)

1. You run `Driver.py` with a small text file (a "config") that says which
   trees to compare and which distances to use.
2. `Driver.py` uses `Utilities.py`, which calls `MergeTreeLibrary.pyd`.
3. `MergeTreeLibrary.pyd` is C++ code that Python can use. It reads the trees,
   writes them as `.jt` files, and then runs a program to get the distance.
4. **This is the part we changed.** In the root folder that program is Java.
   Here, in `TreePair.cpp`, it is the C++ `.exe` instead.
5. The results are saved as CSV files (`PairsOutput`, `SingleOutput`).

## What we changed in this folder

- **`TreePair.cpp`** – all four distances now run the `.exe` files in this
  folder instead of Java.
- **`CustomisedWasserstein.cpp`** – new: the C++ version of
  `CustomisedWasserstein.java`.
- **`SeeMatching.cpp`** – new: the C++ version of `SeeMatching.java`. It does
  the same matching as `CustomisedWasserstein.cpp`, but instead of the total
  cost it prints the matched pairs, in the same text form as Java:
  `[(9[] : 9[]), (3[] : -1[]), ...]` (`-1` = matched to the diagonal).
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

(In Git Bash you can also run `sh build.sh`. In PowerShell `sh` does not
exist, so use `python build.py` there.)

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

(When one tree picture is shorter than the other, the empty space is black.
That comes from the original drawing code, not from the colours.)

## How the Wasserstein program works (simple version)

1. Every leaf of a tree is a point: (leaf value, value of its pair).
2. Each point of tree 1 must be matched to a point of tree 2, **or** to a
   "diagonal" point (meaning: "this feature just disappears").
3. Every possible match has a cost (the biggest difference, squared).
4. The Hungarian (Kuhn-Munkres) method finds the matching with the
   **smallest total cost**. That total is the answer.

This is the same as the Java code, step by step. Two things are kept exactly
like Java on purpose, even if they look unusual:
- there is **no square root** at the end (Java does not take one);
- a point may be matched to the diagonal point of **any** point of its own
  tree, not only its own one.

## Did we check it?

Yes. We ran the three example configs here and compared the results with the
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

## Small notes

- `ExampleMakeFile_Regular` uses a tree called `Reg.jt` that does not exist in
  `Examples/Inputs`, so it does not run (same as in the root folder).
- In `Driver.py`, the number of trees is read as **one digit only**, so use at
  most 9 trees in a config.
- The matching pictures have only **6 colours** (`colors` list in
  `Utilities.py`, `SeeMatching`). If more than 6 features are matched
  leaf-to-leaf, Python stops with an `IndexError`. Features matched to the
  diagonal (they "disappear") are not coloured.
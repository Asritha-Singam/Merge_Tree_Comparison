# CW-MTED, Tree1 vs TreeC: why Java and C++ give different pairings

Both programs give distance **108.727273**, but the map files differ:

| | `forwardmapCW.txt` |
|---|---|
| Java, [CW-MTED.java](../CW-MTED.java) (`test(W1)/results/after_jar_to_java`) | `3,3` `4,4` `2,2` `1,1` |
| C++, [CW-MTED.cpp](../Converted_Merge_Tree_Comparison/CW-MTED.cpp) (`results/after_cwmted_to_cpp`) | `3,3` `2,2` `1,1` |

The C++ map leaves out `4,4`. This document follows both programs step by step and shows exactly where they split.

**In short:**
1. For these two trees, matching any Tree1 node with any TreeC node costs **exactly** the same as deleting the one and inserting the other (section 3). So every valid mapping costs 108.727273, and there are many equally optimal answers.
2. Both programs fill all 88 DP cells (DF and DT) with the **same values** and take the **same case** in every cell (section 5).
3. They differ only inside the Kuhn-Munkres matching step `MM`, which has several equal-cost answers and returns a different one in each language. This happens in three cells. One of them, `MM(T1:2, TC:2)`, is on the path to the final answer, and that is where `4,4` is kept (Java) or dropped (C++) (section 6).
4. Both final mappings cost exactly 108.727273 and are valid. Neither is wrong.

Raw traces (every cell, every matching):
* Java: [CW-MTED_java_Tree1_vs_TreeC_raw_trace.txt](CW-MTED_java_Tree1_vs_TreeC_raw_trace.txt)
* C++: [CW-MTED_cpp_Tree1_vs_TreeC_raw_trace.txt](CW-MTED_cpp_Tree1_vs_TreeC_raw_trace.txt)

They were made from copies of each program with logging added; the logic is unchanged. The traced C++ writes byte-identical map files to `results/after_cwmted_to_cpp`. The traced Java writes the same map as `after_jar_to_java` (see section 8 for how it had to be launched).

## How to read the traces

* `T1:4` is Tree1 node **ID** 4, and `TC:4` is TreeC node ID 4. `-` is the empty tree (index `-1`) and is also the dummy node in matchings.
* `DT(a,b)` is the cheapest cost to turn the subtree under `a` into the subtree under `b`. `DF(a,b)` is the same for the children only (the forest below `a` vs the forest below `b`).
* Each DF/DT line lists the three cases and which one won:
  * **case1:** `a` is matched somewhere inside one child of `b` (`b` is deleted).
  * **case2:** the same with the trees swapped (`a` is deleted).
  * **case3:** for DT, `a` is matched with `b`; for DF, the two child forests are matched with each other by `MM`.
* `MAX` means the node has no children, so that case cannot happen.
* In the Java trace, `map=` is the mapping Java stores for that cell (`pairedDFForward` / `pairedDTForward`), written Tree1 ID -> TreeC ID.
* In an MM block, `matched X <-> -` means child X is deleted, and `matched - <-> Y` means child Y is inserted.

## 1. Inputs

**Tree1** (values 0 to 100)

```text
1  (value 100)
└─ 2  (value 80)
   ├─ 3  (value 4)
   └─ 4  (value 60)
      ├─ 5  (value 30)
      └─ 6  (value 15)
         ├─ 8  (value 5)
         └─ 9  (value 0)
```

| post-order idx | node ID | value | pair ID | pair value | children | delete cost = \|value − pair value\| / 4 |
|---|---|---|---|---|---|---|
| 0 | 3 | 4 | 2 | 80 | — | 19 |
| 1 | 5 | 30 | 4 | 60 | — | 7.5 |
| 2 | 8 | 5 | 6 | 15 | — | 2.5 |
| 3 | 9 | 0 | 1 | 100 | — | 25 |
| 4 | 6 | 15 | 8 | 5 | 8, 9 | 2.5 |
| 5 | 4 | 60 | 5 | 30 | 5, 6 | 7.5 |
| 6 | 2 | 80 | 3 | 4 | 3, 4 | 19 |
| 7 | 1 | 100 | 9 | 0 | 2 | 25 |

**TreeC** (values −1 to 0)

```text
1  (value 1.11e-16)
└─ 2  (value -0.454545)
   ├─ 3  (value -0.909091)
   └─ 4  (value -1)
```

| post-order idx | node ID | value | pair ID | pair value | children | delete cost |
|---|---|---|---|---|---|---|
| 0 | 3 | −0.909091 | 2 | −0.454545 | — | 0.1136365 |
| 1 | 4 | −1 | 1 | 1.11e-16 | — | 0.25 |
| 2 | 2 | −0.454545 | 3 | −0.909091 | 3, 4 | 0.1136365 |
| 3 | 1 | 1.11e-16 | 4 | −1 | 2 | 0.25 |

Both programs number the nodes in the same post-order (first lines of each trace).

## 2. The CW cost functions

Java: `JoinTreePair.matchingCost` [CW-MTED.java:752-788](../CW-MTED.java#L752-L788). C++: [CW-MTED.cpp:148-157](../Converted_Merge_Tree_Comparison/CW-MTED.cpp#L148-L157).

* **delete** `x`: `|v − p| / 4`, where `p` is the value of `x`'s pair.
* **match** `x` with `y`: `min(linf12, linf1 + linf2) / 2`, where
  * `linf12 = max(|v1 − v2|, |p1 − p2|)` is the cost of moving one onto the other,
  * `linf1 = |v1 − p1| / 2` and `linf2 = |v2 − p2| / 2`.

## 3. Why every choice ties here

When `linf1 + linf2 ≤ linf12`, the match cost is `(linf1 + linf2) / 2 = |v1−p1|/4 + |v2−p2|/4`, which is **exactly delete(x) + delete(y)**.

Tree1's values are in [0, 100] and TreeC's are in [−1, 0], so `linf12` is always large and this holds for **every** pair of nodes. For example:

| match | linf12 | linf1 + linf2 | match cost | delete + insert |
|---|---|---|---|---|
| T1:4 ↔ TC:4 | max(\|60−(−1)\|, \|30−0\|) = 61 | 15 + 0.5 = 15.5 | **7.75** | 7.5 + 0.25 = **7.75** |
| T1:3 ↔ TC:3 | max(4.909, 80.45) = 80.45 | 38 + 0.227 = 38.227 | **19.1136365** | 19 + 0.1136365 = **19.1136365** |

So matching never saves or costs anything compared with deleting both nodes. Every valid mapping, including the empty one, costs the same as deleting all of Tree1 (108) and inserting all of TreeC (0.727273), which is **108.727273**. Any differences between the programs can only be in **which** of these equal-cost mappings they return.

## 4. Base cells (identical in both)

| cell | value | | cell | value |
|---|---|---|---|---|
| DT(T1:3,−) | 19 | | DT(−,TC:3) | 0.1136365 |
| DT(T1:5,−) | 7.5 | | DT(−,TC:4) | 0.25 |
| DT(T1:8,−) | 2.5 | | DT(−,TC:2) | 0.477273 |
| DT(T1:9,−) | 25 | | DT(−,TC:1) | 0.727273 |
| DT(T1:6,−) | 30 | | | |
| DT(T1:4,−) | 45 | | | |
| DT(T1:2,−) | 83 | | | |
| DT(T1:1,−) | 108 | | | |

## 5. The main loop: all 32 (i, j) cells are the same in both

The loop visits `i` = 0..7 (Tree1) and `j` = 0..3 (TreeC), computing `DF(i,j)` and then `DT(i,j)`.
* Java: [CW-MTED.java:213-218](../CW-MTED.java#L213-L218), `DF` [L389](../CW-MTED.java#L389), `DT` [L465](../CW-MTED.java#L465).
* C++: `computeDF` [L242](../Converted_Merge_Tree_Comparison/CW-MTED.cpp#L242), `computeDT` [L269](../Converted_Merge_Tree_Comparison/CW-MTED.cpp#L269).

I compared the 88 DF/DT lines of the two traces with each other. **Every value, every case winner and every `via` child is identical.**

The case choice uses the same rule in both: case 3 wins whenever `case3 − 0.00001 ≤ case1` and `≤ case2` (Java [L450](../CW-MTED.java#L450) / [L520](../CW-MTED.java#L520), C++ `choose` [L228](../Converted_Merge_Tree_Comparison/CW-MTED.cpp#L228)). Because of the ties in section 3, case 1, case 2 and case 3 are often equal here, and the rule always picks case 3. So in this example **every cell takes case 3**.

DT values (row = Tree1 node, column = TreeC node):

| DT | TC:3 | TC:4 | TC:2 | TC:1 |
|---|---|---|---|---|
| T1:3 | 19.1136365 | 19.25 | 19.477273 | 19.727273 |
| T1:5 | 7.6136365 | 7.75 | 7.977273 | 8.227273 |
| T1:8 | 2.6136365 | 2.75 | 2.977273 | 3.227273 |
| T1:9 | 25.1136365 | 25.25 | 25.477273 | 25.727273 |
| T1:6 | 30.1136365 | 30.25 | 30.477273 | 30.727273 |
| T1:4 | 45.1136365 | 45.25 | 45.477273 | 45.727273 |
| T1:2 | 83.1136365 | 83.25 | 83.477273 | 83.727273 |
| T1:1 | 108.1136365 | 108.25 | 108.477273 | **108.727273** |

Each cell equals `DT(T1:x,−) + DT(−,TC:y)`, as section 3 predicts.

## 6. Where they split: `MM`, the child matching

`MM(i, j)` matches the children of `i` with the children of `j`. Each side is padded with dummy nodes so the matrix is square, and the minimum-cost perfect matching is found with Kuhn-Munkres.
* Java [L543-604](../CW-MTED.java#L543-L604) uses JGraphT's `KuhnMunkresMinimalWeightBipartitePerfectMatching` [L587](../CW-MTED.java#L587).
* C++ [L296-322](../Converted_Merge_Tree_Comparison/CW-MTED.cpp#L296-L322) uses its own Hungarian algorithm in [common/KuhnMunkres.h](../Converted_Merge_Tree_Comparison/common/KuhnMunkres.h).

Both always return the **same minimum total**. When several matchings reach that minimum, each algorithm returns the first one its search happens to find, and those differ.

Of the 32 MM calls, three return different matchings. All three have the same shape: two children against two children, where every option costs the same.

| MM cell | Java matches | C++ matches | total (both) |
|---|---|---|---|
| MM(T1:6, TC:2) | T1:8↔TC:4, T1:9↔TC:3 | T1:9↔TC:3, **T1:8 deleted, TC:4 inserted** | 27.8636365 |
| MM(T1:4, TC:2) | T1:5↔TC:4, T1:6↔TC:3 | T1:6↔TC:3, **T1:5 deleted, TC:4 inserted** | 37.8636365 |
| **MM(T1:2, TC:2)** | T1:3↔TC:3, **T1:4↔TC:4** | T1:3↔TC:3, **T1:4 deleted, TC:4 inserted** | 64.3636365 |

Here is the cell that matters, `MM(T1:2, TC:2)`, with the same matrix in both programs:

```text
             TC:3        TC:4      -(dummy)  -(dummy)
T1:3      19.1136365    19.25      19.0      19.0
T1:4      45.1136365    45.25      45.0      45.0
-(dummy)   0.1136365     0.25       0.0       0.0
-(dummy)   0.1136365     0.25       0.0       0.0
```

Every way to fill this matrix costs 64.3636365:

| choice | sum |
|---|---|
| T1:3↔TC:3, T1:4↔TC:4 (Java) | 19.1136365 + 45.25 = 64.3636365 |
| T1:3↔TC:3, T1:4↔−, −↔TC:4 (C++) | 19.1136365 + 45.0 + 0.25 = 64.3636365 |
| T1:3↔TC:4, T1:4↔TC:3 | 19.25 + 45.1136365 = 64.3636365 |
| T1:3↔−, T1:4↔−, −↔TC:3, −↔TC:4 | 19 + 45 + 0.1136365 + 0.25 = 64.3636365 |
| … | all 64.3636365 |

The Java trace:

```text
  MM(T1:2,TC:2): ...
    HashSet iteration order handed to JGraphT: S=[ T1:3 T1:4 -@1 -@1 ] T=[ TC:3 TC:4 -@1 -@1 ]
    matched T1:3 <-> TC:3  weight 19.1136365  -> take map of DT(T1:3,TC:3) = { 3->3 }
    matched T1:4 <-> TC:4  weight 45.25  -> take map of DT(T1:4,TC:4) = { 4->4 }
    matched - <-> -  weight 0.0  (dummy-dummy, ignored)
    matched - <-> -  weight 0.0  (dummy-dummy, ignored)
    MM total = 64.3636365   DF map candidate = { 3->3 4->4 }
```

The C++ trace:

```text
  MM(T1:2,TC:2): cost matrix rows = children of T1:2 then dummies, cols = children of TC:2 then dummies
    matched T1:3 <-> TC:3  weight 19.1136365
    matched T1:4 <-> -  weight 45.0
    matched - <-> -  weight 0.0  (dummy-dummy, ignored)
    matched - <-> TC:4  weight 0.25
    MM total = 64.3636365
```

**Why each picks what it picks:**
* **C++** ([KuhnMunkres.h:35-50](../Converted_Merge_Tree_Comparison/common/KuhnMunkres.h#L35-L50)) adds the rows one at a time in order, scans columns left to right, and only switches to a column when it is *strictly* cheaper. For this matrix it ends with T1:3 on TC:3, T1:4 on a dummy column (deleted) and a dummy row on TC:4 (inserted). The result is the same on every run.
* **Java** passes `new HashSet<>(S)` and `new HashSet<>(T)` to JGraphT, so its search order is the HashSet iteration order. That depends on the identity hash codes of the node objects. In this run (all hash codes equal, see section 8), it ends with T1:3↔TC:3 and T1:4↔TC:4.

## 7. How the split becomes the final map

The two programs build the map differently.

**Java** stores a full map in every cell as it goes. In an MM, it copies the map of each matched child cell [L591-599](../CW-MTED.java#L591-L599). A DT cell that takes case 3 adds its own `i->j` [L520-527](../CW-MTED.java#L520-L527). The final answer is just the map stored in `DT(root,root)` [L221](../CW-MTED.java#L221):

```text
MM(T1:2,TC:2)  -> DF(T1:2,TC:2) map = { 3->3 4->4 }
DT(T1:2,TC:2)  CASE3 adds 2->2       -> { 3->3 4->4 2->2 }
MM(T1:1,TC:1)  matched T1:2<->TC:2   -> DF(T1:1,TC:1) map = { 3->3 4->4 2->2 }
DT(T1:1,TC:1)  CASE3 adds 1->1       -> { 3->3 4->4 2->2 1->1 }   = forwardmapCW.txt
```

**C++** stores only which case won in each cell. After `compute()` it walks back down from the roots [L327-353](../Converted_Merge_Tree_Comparison/CW-MTED.cpp#L327-L353), re-running `MM` where needed (same input, so the same answer):

```text
rebuildDT(T1:1,TC:1): CASE3 -> forward[T1:1] = TC:1
rebuildDF(T1:1,TC:1): CASE3 -> MM(T1:1,TC:1) re-run: T1:2<->TC:2
rebuildDT(T1:2,TC:2): CASE3 -> forward[T1:2] = TC:2
rebuildDF(T1:2,TC:2): CASE3 -> MM(T1:2,TC:2) re-run: T1:3<->TC:3, T1:4<->-, -<->TC:4
rebuildDT(T1:3,TC:3): CASE3 -> forward[T1:3] = TC:3
rebuildDT(T1:4,-)   : one side empty -> nothing mapped     <- this is where 4,4 is lost
rebuildDT(-,TC:4)   : one side empty -> nothing mapped
=> forward = 3->3, 2->2, 1->1                                  = forwardmapCW.txt
```

The other two MM cells that differ, `MM(T1:6,TC:2)` and `MM(T1:4,TC:2)`, are not on this path, so they do not affect the output.

## 8. Java's own answer is not fixed either

Java's choice in a tied MM depends on identity hash codes, and that has a visible effect.

The first traced Java run used a normal launch (`java -cp CustomisedWasserstein <copy>.java`). It printed the map **`4,3 2,2 1,1`**, which is a third mapping, different from both outputs above.

The cause is that `java File.java` compiles the source inside the same JVM before running it. A different source text means different compiler work, which shifts the identity hash codes the program later gets. That changes the HashSet order, and so the tie-break.

To get a trace that matches the real Java result, the traced copy was run with `-XX:+UnlockExperimentalVMOptions -XX:hashCode=2`, which gives every object the same hash code. With that flag, the traced copy and the original source both give `3,3 4,4 2,2 1,1`, the same as `after_jar_to_java`. The original source launched normally, and a precompiled class of it, give the same map.

All three mappings cost exactly 108.727273:

| mapping | from | cost | valid |
|---|---|---|---|
| `3,3 4,4 2,2 1,1` | Java (results) | 108.727273 | yes |
| `3,3 2,2 1,1` | C++ | 108.727273 | yes |
| `4,3 2,2 1,1` | Java, edited source, normal launch | 108.727273 | yes |

## 9. Summary

| step | same in both? |
|---|---|
| Reading the trees and post-order numbering | yes |
| Cost functions | yes |
| All 88 DF/DT values | yes |
| Which case wins in each cell | yes (always case 3 here) |
| MM totals | yes |
| **Which of several equal-cost matchings MM returns** | **no**, in 3 of 32 calls |
| Distance | yes, 108.727273 |
| Map | no: Java keeps `4,4` from `MM(T1:2,TC:2)`, C++ deletes T1:4 and inserts TC:4 instead |

The pairings differ because the two Kuhn-Munkres implementations break ties differently, on input where everything ties. To make C++ always give Java's map, it would have to copy JGraphT's search order, and that order itself depends on Java's hash codes.

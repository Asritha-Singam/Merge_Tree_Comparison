# How CW-MTED.java runs: Tree1 vs TreeC, step by step

This walkthrough follows [CW-MTED.java](../CW-MTED.java) as it compares `Examples/Inputs/Tree1.jt` with `Examples/Inputs/TreeC.jt`. It uses the existing raw trace [CW-MTED_java_Tree1_vs_TreeC_raw_trace.txt](CW-MTED_java_Tree1_vs_TreeC_raw_trace.txt) and explains what each part of the trace means in the code.

**Result:** distance **108.727273**, and `forwardmapCW.txt` = `3,3` `4,4` `2,2` `1,1`.

(For why the C++ port gives a different map with the same distance, see [CW-MTED_Tree1_vs_TreeC_java_vs_cpp.md](CW-MTED_Tree1_vs_TreeC_java_vs_cpp.md). This file covers only how the Java code works.)

---

## 0. The code at a glance

```text
main()                                   L37
 ├─ JoinTree.readJoinTree(file)          L866   read nodes, link children, find root
 ├─ JoinTree.stabilize(0.0)              L900   threshold 0 → nothing is simplified, just a copy
 ├─ new JoinTreePair(jt1, jt2)           L727   this object is the cost function
 ├─ new ConstrainedEditDist(root1, root2, pair)  L179
 │    └─ assignPostOrderIndices(...)     L607   number every node in post-order
 └─ compute(...)                         L196
      ├─ seed DT(-1,-1)=DF(-1,-1)=0
      ├─ for every i:  DF(i,-1), DT(i,-1)       delete costs (Tree1 side)
      ├─ for every j:  DF(-1,j), DT(-1,j)       insert costs (TreeC side)
      ├─ for i, for j: DF(i,j), DT(i,j)         the real DP
      │        DF uses MM(i,j)  (L543)  ← Kuhn–Munkres matching of children
      ├─ answer = DT(root1, root2)
      └─ write forwardmapCW.txt / backwardmapCW.txt from pairedDTForward/Backward
```

Four tables are filled, all `HashMap<Pair<Integer>, …>` keyed by **post-order index** pairs `(i, j)`:

| table | meaning |
|---|---|
| `DT(i,j)` | cost to edit the **whole subtree** under node `i` into the subtree under node `j` |
| `DF(i,j)` | cost to edit the **forest of children** of `i` into the forest of children of `j` |
| `pairedDTForward(i,j)` | which nodes were matched to reach that `DT` value (Tree1 idx → TreeC idx) |
| `pairedDFForward(i,j)` | same for `DF` |

Index `-1` means "empty tree". So `DT(i,-1)` = delete all of subtree `i`, and `DT(-1,j)` = insert all of subtree `j`.

`DF` and `DT` are **memoised recursive** functions: the first line of each checks `containsKey` and returns the stored value ([L391](../CW-MTED.java#L391), [L467](../CW-MTED.java#L467)). The loops in `compute` go in post-order, so children are always ready before their parents, and the recursion rarely goes deep.

---

## 1. Reading the input

`readJoinTree` reads each line as `id parent pair maxNode value numChildren children…`. The root is the node whose parent is not in the file (parent `-1`). Children keep the order in the file.

**Tree1**

```text
1  (100)            pair 9
└─ 2  (80)          pair 3
   ├─ 3  (4)        pair 2
   └─ 4  (60)       pair 5
      ├─ 5  (30)    pair 4
      └─ 6  (15)    pair 8
         ├─ 8  (5)  pair 6
         └─ 9  (0)  pair 1
```

**TreeC**

```text
1  (≈0)               pair 4
└─ 2  (-0.454545)     pair 3
   ├─ 3  (-0.909091)  pair 2
   └─ 4  (-1)         pair 1
```

`stabilize(0.0)`: a saddle is removed only if `|value − parentValue| < 0`, which is never true. So the stabilized tree is just a copy, and `stabilizedJTRoot` is node 1.

---

## 2. Post-order numbering (trace lines 6–19)

`assignPostOrderIndices` ([L607](../CW-MTED.java#L607)) visits children first, then gives the node the next number and appends it to `tree1` / `tree2`.

| Tree1 idx | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 |
|---|---|---|---|---|---|---|---|---|
| node ID | 3 | 5 | 8 | 9 | 6 | 4 | 2 | 1 |

| TreeC idx | 0 | 1 | 2 | 3 |
|---|---|---|---|---|
| node ID | 3 | 4 | 2 | 1 |

From now on the code works **only with indices**. The trace prints IDs (`T1:6`, `TC:2`) to make it readable. Remember this when you read the map tables: `pairedDTForward` really stores `4→2`, which the trace prints as `6->2`.

---

## 3. The cost function: `JoinTreePair.matchingCost` ([L752](../CW-MTED.java#L752))

Every node stands for a persistence pair (node value, its `pair` node's value).

* **Delete / insert a node** (`t2j == null` or `t1i == null`):
  `|value − pairValue| / 4`
* **Match (relabel) node a with node b**:
  `min( max(|va − vb|, |pa − pb|),  pers(a)/2 + pers(b)/2 ) / 2`
  where `pers = |value − pairValue|`.

Delete costs for this run:

| node | value, pair value | delete cost |
|---|---|---|
| T1:3 | 4, 80 | 19 |
| T1:5 | 30, 60 | 7.5 |
| T1:8 | 5, 15 | 2.5 |
| T1:9 | 0, 100 | 25 |
| T1:6 | 15, 5 | 2.5 |
| T1:4 | 60, 30 | 7.5 |
| T1:2 | 80, 4 | 19 |
| T1:1 | 100, 0 | 25 |
| TC:3 | −0.909091, −0.454545 | 0.1136365 |
| TC:4 | −1, ≈0 | 0.25 |
| TC:2 | −0.454545, −0.909091 | 0.1136365 |
| TC:1 | ≈0, −1 | 0.25 |

**Important for this example.** TreeC is tiny (values near 0) and Tree1 is large (0 to 100). So in the relabel formula the second term always wins, and

```text
relabel(a, b) = pers(a)/4 + pers(b)/4 = delete(a) + insert(b)
```

Example, T1:6 vs TC:2: `max(|15+0.4545|, |5+0.909|) = 15.4545`, `5 + 0.2273 = 5.2273`, min = 5.2273, /2 = **2.6136365** = 2.5 + 0.1136365.

So matching two nodes is never cheaper than deleting one and inserting the other. That is why every cell in the trace has ties, and the final answer equals "delete all of Tree1 + insert all of TreeC" = 108 + 0.727273.

---

## 4. Phase 1: delete and insert costs (trace lines 20–43)

The first two loops in `compute` ([L205–L212](../CW-MTED.java#L205-L212)) fill the border of the table.

**`DF(i,-1)`** ([L396](../CW-MTED.java#L396)) = sum of `DT(child,-1)` over the children of `i`.
**`DT(i,-1)`** ([L472](../CW-MTED.java#L472)) = `DF(i,-1)` + delete cost of `i`.

Worked example for T1:6 (children 8, 9):

```text
DF(T1:6,-) = DT(T1:8,-) + DT(T1:9,-) = 2.5 + 25 = 27.5
DT(T1:6,-) = 27.5 + delete(T1:6)     = 27.5 + 2.5 = 30
```

Going up the tree:

| node | DF(·,-) | DT(·,-) |
|---|---|---|
| T1:3, 5, 8, 9 (leaves) | 0 | 19, 7.5, 2.5, 25 |
| T1:6 | 27.5 | 30 |
| T1:4 | 7.5 + 30 = 37.5 | 45 |
| T1:2 | 19 + 45 = 64 | 83 |
| T1:1 | 83 | **108** |

The TreeC side works the same way with `DF(-1,j)` / `DT(-1,j)` ([L407](../CW-MTED.java#L407), [L480](../CW-MTED.java#L480)):

| node | DF(-,·) | DT(-,·) |
|---|---|---|
| TC:3, TC:4 | 0 | 0.1136365, 0.25 |
| TC:2 | 0.3636365 | 0.477273 |
| TC:1 | 0.477273 | **0.727273** |

All these cells get an **empty map**, because nothing is matched when you only delete or only insert.

---

## 5. Phase 2: the main DP loop (trace lines 44–291)

```java
for (int i = 0; i < tree1.size(); i++)
    for (int j = 0; j < tree2.size(); j++) {
        DF(i, j);
        DT(i, j);
    }
```

So the trace goes row by row: T1:3 against TC:3, TC:4, TC:2, TC:1, then T1:5 against all four, and so on. For each `(i, j)`, `DF` runs first (it calls `MM`), then `DT`.

### 5.1 The three cases

Both `DF` and `DT` take the minimum of three options.

**`DF(i,j)`** ([L419–L462](../CW-MTED.java#L419-L462)):

| case | formula | meaning |
|---|---|---|
| case1 | `DF(-,j) + min over children c of j: [DF(i,c) − DF(-,c)]` | insert all of j's forest, except that i's whole forest goes inside one child `c` |
| case2 | `DF(i,-) + min over children c of i: [DF(c,j) − DF(c,-)]` | the same, with the trees swapped |
| case3 | `MM(i,j)` | match the children of i with the children of j one-to-one (bipartite matching) |

**`DT(i,j)`** ([L489–L540](../CW-MTED.java#L489-L540)):

| case | formula | meaning |
|---|---|---|
| case1 | `DT(-,j) + min over c: [DT(i,c) − DT(-,c)]` | j is inserted; subtree i maps inside one child of j |
| case2 | `DT(i,-) + min over c: [DT(c,j) − DT(c,-)]` | i is deleted; one child of i maps to subtree j |
| case3 | `DF(i,j) + relabel(i,j)` | **i is matched with j**, and their forests are matched by DF |

If a node has no children, its case loop never runs, `minCost` stays `Double.MAX_VALUE`, and the trace prints `MAX`.

### 5.2 How the winner is chosen ([L450](../CW-MTED.java#L450), [L520](../CW-MTED.java#L520))

```java
if (case3 - 0.00001 <= case2 && case3 - 0.00001 <= case1)  → case3
else if (case1 < case2)                                     → case1
else                                                        → case2
```

Case3 wins whenever it is within 0.00001 of the best. In this example all three cases are equal in almost every cell (see section 3), so **case3 wins in every cell of the trace**. This is the reason the final map has so many matches: when there is a tie, the code prefers to match nodes.

### 5.3 How the maps are built

* **DT case3** copies `pairedDFForward(i,j)` and adds `i → j` ([L522–L524](../CW-MTED.java#L522-L524)).
* **DT case1/case2** copies the map of the cell it came from.
* **DF case3** stores nothing itself. `MM` already wrote `pairedDFForward(i,j)` ([L601](../CW-MTED.java#L601)). If case1 or case2 wins, that entry is overwritten.
* **MM** builds the DF map by joining the `pairedDTForward` maps of every matched child pair ([L597](../CW-MTED.java#L597)).

So maps flow **upward**. A match made low in the tree is carried into each parent cell through MM, and each DT case3 adds one more pair.

---

## 6. The MM function (Kuhn–Munkres), explained with the trace

`MM(i,j)` ([L543](../CW-MTED.java#L543)) matches the children of `i` with the children of `j` at minimum total cost.

1. `S` = children of i, `T` = children of j.
2. If both are empty (two leaves) → return 0 with an empty map. Trace: `MM(...): both leaves -> 0`.
3. Pad: add `|T|` dummy nodes to S and `|S|` dummy nodes to T, so both sides have size `|S|+|T|`.
4. Edge weight for every (s, t) is `DT(s.index, t.index)`. A dummy has index `-1`, so:
   * real ↔ real = `DT(s,t)`
   * real ↔ dummy = `DT(s,-1)` = delete s
   * dummy ↔ real = `DT(-1,t)` = insert t
   * dummy ↔ dummy = `DT(-1,-1)` = 0 (seeded at the start of `compute`)
5. Run JGraphT `KuhnMunkresMinimalWeightBipartitePerfectMatching`. The S/T sets are passed as `new HashSet<>(S)`, so their order depends on object hash codes. This is why Java runs can return different maps when there are ties (the trace was made with `-XX:hashCode=2` to get a fixed order).
6. For each matched edge, except dummy–dummy, add the child pair's `pairedDTForward` map into the DF map.

In the trace's cost-matrix blocks, rows are S and columns are T, and `-` is a dummy.

### Example A: leaf vs leaf, `(T1:3, TC:3)` (lines 44–46)

```text
MM(T1:3,TC:3): both leaves -> 0
DF = 0, map {}
DT: case1 = MAX (TC:3 has no children), case2 = MAX (T1:3 has no children)
    case3 = DF + relabel = 0 + 19.1136365   → CASE3, map { 3->3 }
```

A leaf–leaf DT can only be "match them", so the map is just that one pair.

### Example B: leaf vs internal node, `(T1:3, TC:2)` (lines 50–58)

T1:3 has 0 children, TC:2 has 2 (TC:3, TC:4). So S gets 2 dummies and T gets 0 dummies:

```text
        TC:3        TC:4
  -     0.1136365   0.25      ← insert TC:3 / insert TC:4
  -     0.1136365   0.25
```

The only choice is to insert both children: 0.3636365, empty map. Then
`DT(T1:3,TC:2) = 0.3636365 + relabel(T1:3,TC:2)` = 19.477273, map `{3->2}`.

Case1 for this DT is `DT(-,TC:2) + [DT(T1:3,TC:3) − DT(-,TC:3)]` = 0.477273 + 19 = 19.477273, which is the same value. Case3 wins the tie (section 5.2).

### Example C: internal node vs leaf, `(T1:6, TC:3)` (lines 132–140)

The mirror case. T1:6's children 8 and 9 must both be deleted: 2.5 + 25 = 27.5. Then `DT = 27.5 + relabel(T1:6, TC:3) = 30.1136365`, map `{6->3}`.

### Example D: first real matching, `(T1:6, TC:2)` (lines 150–162)

Both have 2 children, so the matrix is 4×4:

```text
          TC:3         TC:4        -       -
  T1:8    2.6136365    2.75        2.5     2.5
  T1:9    25.1136365   25.25       25.0    25.0
  -       0.1136365    0.25        0.0     0.0
  -       0.1136365    0.25        0.0     0.0
```

Some perfect matchings and their totals:

| choice | total |
|---|---|
| 8↔TC:4, 9↔TC:3 | 2.75 + 25.1136365 = **27.8636365** |
| 8↔TC:3, 9↔TC:4 | 2.6136365 + 25.25 = **27.8636365** |
| delete 8, 9, insert TC:3, TC:4 | 2.5 + 25 + 0.1136365 + 0.25 = **27.8636365** |

They are all equal (section 3). Kuhn–Munkres returned `8↔TC:4, 9↔TC:3`, so the DF map is
`DT(T1:8,TC:4).map ∪ DT(T1:9,TC:3).map = {8->4} ∪ {9->3}`.

```text
DF(T1:6,TC:2) = 27.8636365        map { 8->4, 9->3 }
DT(T1:6,TC:2) = 27.8636365 + relabel 2.6136365 = 30.477273
                                  map { 6->2, 8->4, 9->3 }
```

This map comes back later in `DT(T1:4,TC:1)`.

### Example E: `(T1:6, TC:1)` (lines 163–173)

TC:1 has a single child, TC:2:

```text
          TC:2        -      -
  T1:8    2.977273    2.5    2.5
  T1:9    25.477273   25.0   25.0
  -       0.477273    0.0    0.0
```

Matching chosen: delete T1:8 (2.5) and `T1:9 ↔ TC:2` (25.477273). It brings the map of `DT(T1:9,TC:2)` = `{9->2}`. Then DT adds `6->1`: map `{6->1, 9->2}`.

---

## 7. Climbing up to the root

Each level repeats the same pattern. Here are only the cells that lead to the final answer.

```text
DT(T1:3,TC:3) = 19.1136365     { 3->3 }                         line 46
DT(T1:4,TC:4) = 45.25          { 4->4 }                         line 191
        │                        (MM(T1:4,TC:4): TC:4 is a leaf, so delete 5 and 6;
        │                         then case3 adds 4->4)
        ▼
MM(T1:2,TC:2)                                                   lines 234–244
          TC:3        TC:4        -      -
  T1:3    19.1136365  19.25       19.0   19.0
  T1:4    45.1136365  45.25       45.0   45.0
  -       0.1136365   0.25        0.0    0.0
  -       0.1136365   0.25        0.0    0.0
  chosen: T1:3↔TC:3 (19.1136365) + T1:4↔TC:4 (45.25) = 64.3636365
  map = {3->3} ∪ {4->4}
        ▼
DF(T1:2,TC:2) = 64.3636365     { 3->3, 4->4 }                   line 245
DT(T1:2,TC:2) = 64.3636365 + relabel 19.1136365 = 83.477273
                               { 3->3, 4->4, 2->2 }             line 246
        ▼
MM(T1:1,TC:1)                                                   lines 283–289
          TC:2        -
  T1:2    83.477273   83.0
  -       0.477273    0.0
  chosen: T1:2↔TC:2 (83.477273), dummy↔dummy (0)
  (the alternative, 83.0 + 0.477273, is the same total)
  map = DT(T1:2,TC:2).map = {3->3, 4->4, 2->2}
        ▼
DF(T1:1,TC:1) = 83.477273      { 3->3, 4->4, 2->2 }             line 290
DT(T1:1,TC:1) = 83.477273 + relabel 25.25 = 108.727273
                               { 3->3, 4->4, 2->2, 1->1 }       line 291
```

Note: `{4->4}` reaches the root only because `MM(T1:2,TC:2)` picked `T1:4↔TC:4` and not `T1:4↔TC:3`. Both cost the same, so this choice decides whether `4,4` appears in the output.

Check the final number: `relabel(T1:1, TC:1) = 25 + 0.25`, and
`83.477273 + 25.25 = 108.727273 = 108 (delete all of Tree1) + 0.727273 (insert all of TreeC)`.

---

## 8. Full DT table (all 32 cells, all case3)

Rows are Tree1 nodes, columns are TreeC nodes. Each cell is `DT value` / `pairedDTForward` map (as IDs).

| | TC:3 | TC:4 | TC:2 | TC:1 |
|---|---|---|---|---|
| **T1:3** | 19.1136 `{3→3}` | 19.25 `{3→4}` | 19.4773 `{3→2}` | 19.7273 `{3→1}` |
| **T1:5** | 7.6136 `{5→3}` | 7.75 `{5→4}` | 7.9773 `{5→2}` | 8.2273 `{5→1}` |
| **T1:8** | 2.6136 `{8→3}` | 2.75 `{8→4}` | 2.9773 `{8→2}` | 3.2273 `{8→1}` |
| **T1:9** | 25.1136 `{9→3}` | 25.25 `{9→4}` | 25.4773 `{9→2}` | 25.7273 `{9→1}` |
| **T1:6** | 30.1136 `{6→3}` | 30.25 `{6→4}` | 30.4773 `{6→2, 8→4, 9→3}` | 30.7273 `{6→1, 9→2}` |
| **T1:4** | 45.1136 `{4→3}` | 45.25 `{4→4}` | 45.4773 `{4→2, 5→4, 6→3}` | 45.7273 `{4→1, 6→2, 8→4, 9→3}` |
| **T1:2** | 83.1136 `{2→3}` | 83.25 `{2→4}` | 83.4773 `{2→2, 3→3, 4→4}` | 83.7273 `{2→1, 3→2}` |
| **T1:1** | 108.1136 `{1→3}` | 108.25 `{1→4}` | 108.4773 `{1→2, 2→3}` | **108.7273** `{1→1, 2→2, 3→3, 4→4}` |

A pattern to notice: every value is `DT(i,-) + DT(-,j)`, which means "delete one whole subtree and insert the other". This is the tie from section 3 again. The DP still computes all three cases, but they always come out equal.

---

## 9. Writing the output ([L220–L268](../CW-MTED.java#L220-L268))

```java
double cost = DT(tree1Root.getIndex(), tree2Root.getIndex());   // DT(7, 3) = 108.727273
Map<Integer,Integer> forwardMapIndex = pairedDTForward.get(new Pair<>(7, 3));
```

The stored map uses **indices**: `{0→0, 5→1, 6→2, 7→3}`. The loop converts each index back to a node ID with `tree1.get(idx).getID()` and `tree2.get(idx).getID()`, and writes `ID1,ID2` lines.

`HashMap<Integer,…>` with small keys iterates in ascending key order (key 0, 5, 6, 7), so the file is:

```text
3,3      ← idx 0 → idx 0
4,4      ← idx 5 → idx 1
2,2      ← idx 6 → idx 2
1,1      ← idx 7 → idx 3
```

`backwardmapCW.txt` is written the same way from `pairedDTBackward`, which holds the same pairs keyed by the TreeC index. `main` then prints `108.727273`.

---

## 10. Summary

1. Number both trees in post-order. All tables are keyed by index pairs.
2. Fill the border: delete cost of every Tree1 subtree, insert cost of every TreeC subtree.
3. For each `(i, j)`, bottom-up:
   * `DF(i,j)`: best of "push forest into one child" (case1/2) or "match the children by Kuhn–Munkres" (case3 → `MM`).
   * `DT(i,j)`: best of "push subtree into one child" (case1/2) or "match i with j" (`DF + relabel`, case3).
   * Case3 wins all near-ties (0.00001 tolerance).
   * Maps are copied upward: MM joins the children's DT maps, and DT case3 adds `i→j`.
4. The answer is `DT(root1, root2)`, and its map is written to file.

For Tree1 vs TreeC, relabelling always costs exactly delete + insert. So every cell ties, case3 always wins, and which pairs end up in the map depends only on which equal-cost matching Kuhn–Munkres returns (HashSet order). The distance 108.727273 does not depend on that choice.

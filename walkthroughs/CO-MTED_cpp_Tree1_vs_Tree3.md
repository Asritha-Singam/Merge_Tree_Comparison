# CO-MTED (C++ port), Tree1 vs Tree3: how the code runs, step by step

This is a record of one real run of [CO-MTED.cpp](../Converted_Merge_Tree_Comparison/CO-MTED.cpp) (with [common/KuhnMunkres.h](../Converted_Merge_Tree_Comparison/common/KuhnMunkres.h)) on `Examples/Inputs/Tree1.jt` vs `Examples/Inputs/Tree3.jt`. It was produced by a copy of the C++ source with logging added at every step; the logic is unchanged. The copy prints the same result as `CO-MTED.exe`, distance **21.0**, and writes byte-identical map files. The forward map is `3,3 5,5 8,8 9,9 6,6 4,4 2,2 1,1`.

The C++ is structured differently from the Java, and the order of this document follows the C++:

| | Java (`CO-MTED.java`) | C++ (`CO-MTED.cpp`) |
|---|---|---|
| Tables | `HashMap<Pair, Double>`, filled by memoised recursive `DF()`/`DT()` calls with table lookups | flat `double` arrays; `computeDF`/`computeDT` read earlier cells directly |
| Per cell it stores | the value **and a full copy of the node mapping** | the value **and one int**: which case won, and which child (`DFchoice`/`DTchoice`) |
| Mapping | already complete in `pairedDTForward(root,root)` when the loop ends | rebuilt afterwards by `rebuildDT`/`rebuildDF`, which follow the stored choices from the roots down (phase D below) |
| Matching (MM) | JGraphT Kuhn-Munkres over a `HashSet`, so ties can break differently between runs | its own Hungarian algorithm, rows tried in order: the same answer every run |
| Map file order | HashMap iteration | ascending post-order index |

## How to read the trace

* `[L243]` and similar tags give the line in `CO-MTED.cpp` that is executing; `[KM:L48]` is a line in `common/KuhnMunkres.h`. In the text outside the trace blocks, line numbers are clickable.
* `|   ` indentation means a nested step: the indented lines happen *inside* the line above them.
* `T1:6` means Tree1 node **ID** 6, and `T3:4` means Tree3 node ID 4. `-` means "empty tree": index `-1` in the code (row/column 0 of each table), which is also the dummy node in matchings.
* `DT(a,b)` is the cheapest cost to turn the **subtree** under `a` into the subtree under `b`. `DF(a,b)` is the same for the **children only** (the forest below `a` vs the forest below `b`).
* `MAX` is `std::numeric_limits<double>::max()`. When a node has no children, the case 1 or case 2 loop never runs, so that case's cost stays MAX and can never win.

## 1. Inputs

**Tree1** (node IDs)

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

| post-order idx | Tree1 node ID | value | pair ID | pair value | children (IDs) | delete cost = \|value − pair value\| / 2 |
|---|---|---|---|---|---|---|
| 0 | **3** | 4 | 2 | 80 | — | 38 |
| 1 | **5** | 30 | 4 | 60 | — | 15 |
| 2 | **8** | 5 | 6 | 15 | — | 5 |
| 3 | **9** | 0 | 1 | 100 | — | 50 |
| 4 | **6** | 15 | 8 | 5 | 8, 9 | 5 |
| 5 | **4** | 60 | 5 | 30 | 5, 6 | 15 |
| 6 | **2** | 80 | 3 | 4 | 3, 4 | 38 |
| 7 | **1** | 100 | 9 | 0 | 2 | 50 |

**Tree3** (node IDs)

```text
1  (value 105)
└─ 2  (value 70)
   ├─ 3  (value 3)
   └─ 4  (value 60)
      ├─ 5  (value 30)
      └─ 6  (value 10)
         ├─ 8  (value 5)
         └─ 9  (value 0)
```

| post-order idx | Tree3 node ID | value | pair ID | pair value | children (IDs) | delete cost = \|value − pair value\| / 2 |
|---|---|---|---|---|---|---|
| 0 | **3** | 3 | 2 | 70 | — | 33.5 |
| 1 | **5** | 30 | 4 | 60 | — | 15 |
| 2 | **8** | 5 | 6 | 10 | — | 2.5 |
| 3 | **9** | 0 | 1 | 105 | — | 52.5 |
| 4 | **6** | 10 | 8 | 5 | 8, 9 | 2.5 |
| 5 | **4** | 60 | 5 | 30 | 5, 6 | 15 |
| 6 | **2** | 70 | 3 | 3 | 3, 4 | 33.5 |
| 7 | **1** | 105 | 9 | 0 | 2 | 52.5 |

The post-order index is the number the code actually uses: `i` for the first tree and `j` for the second. Children always get smaller indices than their parent, so when the loops reach a cell, every cell it reads has already been filled in.

## 2. The cost functions [L146-154](../Converted_Merge_Tree_Comparison/CO-MTED.cpp#L146-L154)

* **Delete a node**, `deleteCost` [L146-148](../Converted_Merge_Tree_Comparison/CO-MTED.cpp#L146-L148): `|value − pairValue| / 2`.
* **Match (relabel) two nodes**, `relabelCost` [L150-154](../Converted_Merge_Tree_Comparison/CO-MTED.cpp#L150-L154): `min(diff1, diff2) / 2`, where
  * `diff1 = |v1 − v2| + |pairValue1 − pairValue2|` [L151](../Converted_Merge_Tree_Comparison/CO-MTED.cpp#L151) is the cost of moving one node onto the other,
  * `diff2 = |pairValue1 − v1| + |pairValue2 − v2|` [L152](../Converted_Merge_Tree_Comparison/CO-MTED.cpp#L152) is the cost of deleting both instead.

## 3. `main` reads both trees [L403-414](../Converted_Merge_Tree_Comparison/CO-MTED.cpp#L403-L414), `readJoinTree` [L57-141](../Converted_Merge_Tree_Comparison/CO-MTED.cpp#L57-L141)

For each file, `readJoinTree` reads the raw records ([L71-84](../Converted_Merge_Tree_Comparison/CO-MTED.cpp#L71-L84)), finds the root ([L87-91](../Converted_Merge_Tree_Comparison/CO-MTED.cpp#L87-L91)), numbers the nodes in post-order with an explicit stack ([L101-118](../Converted_Merge_Tree_Comparison/CO-MTED.cpp#L101-L118)), and builds the arrays the DP uses ([L120-139](../Converted_Merge_Tree_Comparison/CO-MTED.cpp#L120-L139)). Unlike the Java, there is no `stabilize` step: the Java calls it with threshold 0, which never changes anything.

```text
[L403-410] main: jt1FileName = Examples/Inputs/Tree1.jt, jt2FileName = Examples/Inputs/Tree3.jt, location = C:/Users/saias/AppData/Local/Temp/claude/46f41248b91d1fcc/a0dd2236-3501-47b2-830d-55001e7fe0eb/scratchpad/cpp_full/out_Tree1_Tree3
[L412-414] read both trees:
[L57] readJoinTree("Examples/Inputs/Tree1.jt")
|   [L61-62] first number in the file: numNodes = 8
|   [L65-69] allocate raw arrays (one 'slot' per record, in file order)
|   [L71-84] slot 0: id=1 parent=-1 pair=9 maxNode=63000 value=100 numChildren=1 children IDs=[2]   slotOf[1] = 0
|   [L71-84] slot 1: id=2 parent=1 pair=3 maxNode=63000 value=80 numChildren=2 children IDs=[3,4]   slotOf[2] = 1
|   [L71-84] slot 2: id=3 parent=2 pair=2 maxNode=63000 value=4 numChildren=0 children IDs=[]   slotOf[3] = 2
|   [L71-84] slot 3: id=4 parent=2 pair=5 maxNode=63000 value=60 numChildren=2 children IDs=[5,6]   slotOf[4] = 3
|   [L71-84] slot 4: id=5 parent=4 pair=4 maxNode=63000 value=30 numChildren=0 children IDs=[]   slotOf[5] = 4
|   [L71-84] slot 5: id=6 parent=4 pair=8 maxNode=63000 value=15 numChildren=2 children IDs=[8,9]   slotOf[6] = 5
|   [L71-84] slot 6: id=8 parent=6 pair=6 maxNode=63000 value=5 numChildren=0 children IDs=[]   slotOf[8] = 6
|   [L71-84] slot 7: id=9 parent=6 pair=1 maxNode=63000 value=0 numChildren=0 children IDs=[]   slotOf[9] = 7
|   [L87-91] root = slot 0 (ID 1): its parent ID -1 is not a node in the file
|   [L92-97] child IDs converted to slots
|   [L101-118] iterative post-order walk from the root (children in file order, then the node):
|   |   [L105] push ID 1
|   |   [L109-111] top ID 1 has an unvisited child -> push ID 2
|   |   [L109-111] top ID 2 has an unvisited child -> push ID 3
|   |   [L112-115] top ID 3 has no unvisited children -> pop, post-order index 0
|   |   [L109-111] top ID 2 has an unvisited child -> push ID 4
|   |   [L109-111] top ID 4 has an unvisited child -> push ID 5
|   |   [L112-115] top ID 5 has no unvisited children -> pop, post-order index 1
|   |   [L109-111] top ID 4 has an unvisited child -> push ID 6
|   |   [L109-111] top ID 6 has an unvisited child -> push ID 8
|   |   [L112-115] top ID 8 has no unvisited children -> pop, post-order index 2
|   |   [L109-111] top ID 6 has an unvisited child -> push ID 9
|   |   [L112-115] top ID 9 has no unvisited children -> pop, post-order index 3
|   |   [L112-115] top ID 6 has no unvisited children -> pop, post-order index 4
|   |   [L112-115] top ID 4 has no unvisited children -> pop, post-order index 5
|   |   [L112-115] top ID 2 has no unvisited children -> pop, post-order index 6
|   |   [L112-115] top ID 1 has no unvisited children -> pop, post-order index 7
|   [L120-139] build the final arrays, indexed by post-order index (root = index 7):
|   |   index 0: nodeID=3 value=4 pairValue=80 (value of pair ID 2) children indices=[]
|   |   index 1: nodeID=5 value=30 pairValue=60 (value of pair ID 4) children indices=[]
|   |   index 2: nodeID=8 value=5 pairValue=15 (value of pair ID 6) children indices=[]
|   |   index 3: nodeID=9 value=0 pairValue=100 (value of pair ID 1) children indices=[]
|   |   index 4: nodeID=6 value=15 pairValue=5 (value of pair ID 8) children indices=[2,3]
|   |   index 5: nodeID=4 value=60 pairValue=30 (value of pair ID 5) children indices=[1,4]
|   |   index 6: nodeID=2 value=80 pairValue=4 (value of pair ID 3) children indices=[0,5]
|   |   index 7: nodeID=1 value=100 pairValue=0 (value of pair ID 9) children indices=[6]
[L57] readJoinTree("Examples/Inputs/Tree3.jt")
|   [L61-62] first number in the file: numNodes = 8
|   [L65-69] allocate raw arrays (one 'slot' per record, in file order)
|   [L71-84] slot 0: id=1 parent=-1 pair=9 maxNode=63000 value=105 numChildren=1 children IDs=[2]   slotOf[1] = 0
|   [L71-84] slot 1: id=2 parent=1 pair=3 maxNode=63000 value=70 numChildren=2 children IDs=[3,4]   slotOf[2] = 1
|   [L71-84] slot 2: id=3 parent=2 pair=2 maxNode=63000 value=3 numChildren=0 children IDs=[]   slotOf[3] = 2
|   [L71-84] slot 3: id=4 parent=2 pair=5 maxNode=63000 value=60 numChildren=2 children IDs=[5,6]   slotOf[4] = 3
|   [L71-84] slot 4: id=5 parent=4 pair=4 maxNode=63000 value=30 numChildren=0 children IDs=[]   slotOf[5] = 4
|   [L71-84] slot 5: id=6 parent=4 pair=8 maxNode=63000 value=10 numChildren=2 children IDs=[8,9]   slotOf[6] = 5
|   [L71-84] slot 6: id=8 parent=6 pair=6 maxNode=63000 value=5 numChildren=0 children IDs=[]   slotOf[8] = 6
|   [L71-84] slot 7: id=9 parent=6 pair=1 maxNode=63000 value=0 numChildren=0 children IDs=[]   slotOf[9] = 7
|   [L87-91] root = slot 0 (ID 1): its parent ID -1 is not a node in the file
|   [L92-97] child IDs converted to slots
|   [L101-118] iterative post-order walk from the root (children in file order, then the node):
|   |   [L105] push ID 1
|   |   [L109-111] top ID 1 has an unvisited child -> push ID 2
|   |   [L109-111] top ID 2 has an unvisited child -> push ID 3
|   |   [L112-115] top ID 3 has no unvisited children -> pop, post-order index 0
|   |   [L109-111] top ID 2 has an unvisited child -> push ID 4
|   |   [L109-111] top ID 4 has an unvisited child -> push ID 5
|   |   [L112-115] top ID 5 has no unvisited children -> pop, post-order index 1
|   |   [L109-111] top ID 4 has an unvisited child -> push ID 6
|   |   [L109-111] top ID 6 has an unvisited child -> push ID 8
|   |   [L112-115] top ID 8 has no unvisited children -> pop, post-order index 2
|   |   [L109-111] top ID 6 has an unvisited child -> push ID 9
|   |   [L112-115] top ID 9 has no unvisited children -> pop, post-order index 3
|   |   [L112-115] top ID 6 has no unvisited children -> pop, post-order index 4
|   |   [L112-115] top ID 4 has no unvisited children -> pop, post-order index 5
|   |   [L112-115] top ID 2 has no unvisited children -> pop, post-order index 6
|   |   [L112-115] top ID 1 has no unvisited children -> pop, post-order index 7
|   [L120-139] build the final arrays, indexed by post-order index (root = index 7):
|   |   index 0: nodeID=3 value=3 pairValue=70 (value of pair ID 2) children indices=[]
|   |   index 1: nodeID=5 value=30 pairValue=60 (value of pair ID 4) children indices=[]
|   |   index 2: nodeID=8 value=5 pairValue=10 (value of pair ID 6) children indices=[]
|   |   index 3: nodeID=9 value=0 pairValue=105 (value of pair ID 1) children indices=[]
|   |   index 4: nodeID=6 value=10 pairValue=5 (value of pair ID 8) children indices=[2,3]
|   |   index 5: nodeID=4 value=60 pairValue=30 (value of pair ID 5) children indices=[1,4]
|   |   index 6: nodeID=2 value=70 pairValue=3 (value of pair ID 3) children indices=[0,5]
|   |   index 7: nodeID=1 value=105 pairValue=0 (value of pair ID 9) children indices=[6]
```

## 4. The constructor [L419](../Converted_Merge_Tree_Comparison/CO-MTED.cpp#L419), [L172-177](../Converted_Merge_Tree_Comparison/CO-MTED.cpp#L172-L177)

```text
[L419] ConstrainedEditDist ced(tree1, tree2)
|   [L172-177] constructor: tables DF, DT (values) and DFchoice, DTchoice are (8+1) x (8+1) = 81 cells each; values start at 0, choices at CHOICE_MATCH (-1).
|              row/col 0 stands for index -1 (empty tree): cell(i,j) = (i+1)*9 + (j+1)  [L220]
[L420] cost = ced.compute()
```

## 5. Phase A: delete each Tree1 subtree [L181-186](../Converted_Merge_Tree_Comparison/CO-MTED.cpp#L181-L186)

For each node in post-order, `DF(i,-)` adds up `DT(child,-)` over its children, which is the cost of deleting the forest below it. `DT(i,-)` then adds the node's own delete cost. Case 1, case 2 and the dummy rows of every matching read these values later.

```text
--- [L181] i = 0  (T1:3), children: none ---
|   [L182] cost = 0
|   [L184] STORE DF(T1:3,-) = 0
|   [L185] deleteCost(T1:3) [L146-148] = |4 - 80| / 2 = 38
|   [L185] STORE DT(T1:3,-) = 0 + 38 = 38

--- [L181] i = 1  (T1:5), children: none ---
|   [L182] cost = 0
|   [L184] STORE DF(T1:5,-) = 0
|   [L185] deleteCost(T1:5) [L146-148] = |30 - 60| / 2 = 15
|   [L185] STORE DT(T1:5,-) = 0 + 15 = 15

--- [L181] i = 2  (T1:8), children: none ---
|   [L182] cost = 0
|   [L184] STORE DF(T1:8,-) = 0
|   [L185] deleteCost(T1:8) [L146-148] = |5 - 15| / 2 = 5
|   [L185] STORE DT(T1:8,-) = 0 + 5 = 5

--- [L181] i = 3  (T1:9), children: none ---
|   [L182] cost = 0
|   [L184] STORE DF(T1:9,-) = 0
|   [L185] deleteCost(T1:9) [L146-148] = |0 - 100| / 2 = 50
|   [L185] STORE DT(T1:9,-) = 0 + 50 = 50

--- [L181] i = 4  (T1:6), children: T1:8,T1:9 ---
|   [L182] cost = 0
|   [L183] cost += DT(T1:8,-) = 5  -> cost = 5
|   [L183] cost += DT(T1:9,-) = 50  -> cost = 55
|   [L184] STORE DF(T1:6,-) = 55
|   [L185] deleteCost(T1:6) [L146-148] = |15 - 5| / 2 = 5
|   [L185] STORE DT(T1:6,-) = 55 + 5 = 60

--- [L181] i = 5  (T1:4), children: T1:5,T1:6 ---
|   [L182] cost = 0
|   [L183] cost += DT(T1:5,-) = 15  -> cost = 15
|   [L183] cost += DT(T1:6,-) = 60  -> cost = 75
|   [L184] STORE DF(T1:4,-) = 75
|   [L185] deleteCost(T1:4) [L146-148] = |60 - 30| / 2 = 15
|   [L185] STORE DT(T1:4,-) = 75 + 15 = 90

--- [L181] i = 6  (T1:2), children: T1:3,T1:4 ---
|   [L182] cost = 0
|   [L183] cost += DT(T1:3,-) = 38  -> cost = 38
|   [L183] cost += DT(T1:4,-) = 90  -> cost = 128
|   [L184] STORE DF(T1:2,-) = 128
|   [L185] deleteCost(T1:2) [L146-148] = |80 - 4| / 2 = 38
|   [L185] STORE DT(T1:2,-) = 128 + 38 = 166

--- [L181] i = 7  (T1:1), children: T1:2 ---
|   [L182] cost = 0
|   [L183] cost += DT(T1:2,-) = 166  -> cost = 166
|   [L184] STORE DF(T1:1,-) = 166
|   [L185] deleteCost(T1:1) [L146-148] = |100 - 0| / 2 = 50
|   [L185] STORE DT(T1:1,-) = 166 + 50 = 216
```

## 6. Phase B: delete each Tree3 subtree [L187-192](../Converted_Merge_Tree_Comparison/CO-MTED.cpp#L187-L192)

The same as phase A on the other side.

```text
--- [L187] j = 0  (T3:3), children: none ---
|   [L188] cost = 0
|   [L190] STORE DF(-,T3:3) = 0
|   [L191] deleteCost(T3:3) [L146-148] = |3 - 70| / 2 = 33.5
|   [L191] STORE DT(-,T3:3) = 0 + 33.5 = 33.5

--- [L187] j = 1  (T3:5), children: none ---
|   [L188] cost = 0
|   [L190] STORE DF(-,T3:5) = 0
|   [L191] deleteCost(T3:5) [L146-148] = |30 - 60| / 2 = 15
|   [L191] STORE DT(-,T3:5) = 0 + 15 = 15

--- [L187] j = 2  (T3:8), children: none ---
|   [L188] cost = 0
|   [L190] STORE DF(-,T3:8) = 0
|   [L191] deleteCost(T3:8) [L146-148] = |5 - 10| / 2 = 2.5
|   [L191] STORE DT(-,T3:8) = 0 + 2.5 = 2.5

--- [L187] j = 3  (T3:9), children: none ---
|   [L188] cost = 0
|   [L190] STORE DF(-,T3:9) = 0
|   [L191] deleteCost(T3:9) [L146-148] = |0 - 105| / 2 = 52.5
|   [L191] STORE DT(-,T3:9) = 0 + 52.5 = 52.5

--- [L187] j = 4  (T3:6), children: T3:8,T3:9 ---
|   [L188] cost = 0
|   [L189] cost += DT(-,T3:8) = 2.5  -> cost = 2.5
|   [L189] cost += DT(-,T3:9) = 52.5  -> cost = 55
|   [L190] STORE DF(-,T3:6) = 55
|   [L191] deleteCost(T3:6) [L146-148] = |10 - 5| / 2 = 2.5
|   [L191] STORE DT(-,T3:6) = 55 + 2.5 = 57.5

--- [L187] j = 5  (T3:4), children: T3:5,T3:6 ---
|   [L188] cost = 0
|   [L189] cost += DT(-,T3:5) = 15  -> cost = 15
|   [L189] cost += DT(-,T3:6) = 57.5  -> cost = 72.5
|   [L190] STORE DF(-,T3:4) = 72.5
|   [L191] deleteCost(T3:4) [L146-148] = |60 - 30| / 2 = 15
|   [L191] STORE DT(-,T3:4) = 72.5 + 15 = 87.5

--- [L187] j = 6  (T3:2), children: T3:3,T3:4 ---
|   [L188] cost = 0
|   [L189] cost += DT(-,T3:3) = 33.5  -> cost = 33.5
|   [L189] cost += DT(-,T3:4) = 87.5  -> cost = 121
|   [L190] STORE DF(-,T3:2) = 121
|   [L191] deleteCost(T3:2) [L146-148] = |70 - 3| / 2 = 33.5
|   [L191] STORE DT(-,T3:2) = 121 + 33.5 = 154.5

--- [L187] j = 7  (T3:1), children: T3:2 ---
|   [L188] cost = 0
|   [L189] cost += DT(-,T3:2) = 154.5  -> cost = 154.5
|   [L190] STORE DF(-,T3:1) = 154.5
|   [L191] deleteCost(T3:1) [L146-148] = |105 - 0| / 2 = 52.5
|   [L191] STORE DT(-,T3:1) = 154.5 + 52.5 = 207
```

## 7. Phase C: the main double loop [L194-199](../Converted_Merge_Tree_Comparison/CO-MTED.cpp#L194-L199)

`for i in Tree1 post-order: for j in Tree3 post-order: computeDF(i,j); computeDT(i,j)`, which visits **64 cells**. Each cell runs:

1. `computeDF(i,j)` ([L239-264](../Converted_Merge_Tree_Comparison/CO-MTED.cpp#L239-L264)):
   * **case 1** [L241-247](../Converted_Merge_Tree_Comparison/CO-MTED.cpp#L241-L247): delete `j`; i's forest goes into one child of `j`. `case1 = DF(-,j) + min over children c of j of [DF(i,c) − DF(-,c)]`.
   * **case 2** [L250-256](../Converted_Merge_Tree_Comparison/CO-MTED.cpp#L250-L256): delete `i`; one child of `i` goes onto j's forest. `case2 = DF(i,-) + min over children c of i of [DF(c,j) − DF(c,-)]`.
   * **case 3** [L259](../Converted_Merge_Tree_Comparison/CO-MTED.cpp#L259): `MM(i,j)` [L293-320](../Converted_Merge_Tree_Comparison/CO-MTED.cpp#L293-L320), the cheapest one-to-one matching between the two lists of children, each padded with dummies. Its matrix entry for row k, column l is `DT(child k, child l)`.
   * **decide** with `choose` [L225-237](../Converted_Merge_Tree_Comparison/CO-MTED.cpp#L225-L237), then store the value and the choice code [L262-263](../Converted_Merge_Tree_Comparison/CO-MTED.cpp#L262-L263).
2. `computeDT(i,j)` ([L266-288](../Converted_Merge_Tree_Comparison/CO-MTED.cpp#L266-L288)):
   * **case 1** [L267-273](../Converted_Merge_Tree_Comparison/CO-MTED.cpp#L267-L273): delete `j`; subtree `i` goes into one child subtree of `j`.
   * **case 2** [L275-281](../Converted_Merge_Tree_Comparison/CO-MTED.cpp#L275-L281): delete `i`; one child subtree of `i` goes onto subtree `j`.
   * **case 3** [L283](../Converted_Merge_Tree_Comparison/CO-MTED.cpp#L283): match `i↔j`, so `DF(i,j) + relabelCost(i,j)`.
   * **decide** [L286](../Converted_Merge_Tree_Comparison/CO-MTED.cpp#L286), **store** [L286-287](../Converted_Merge_Tree_Comparison/CO-MTED.cpp#L286-L287).

**The decision rule** `choose` [L225-237](../Converted_Merge_Tree_Comparison/CO-MTED.cpp#L225-L237) has the same three tests as the Java:

* Case 3 wins if `case3 − 0.00001` is ≤ both other cases [L227](../Converted_Merge_Tree_Comparison/CO-MTED.cpp#L227).
* Otherwise case 1 wins if `case1 < case2` [L230](../Converted_Merge_Tree_Comparison/CO-MTED.cpp#L230).
* Otherwise case 2 wins.

**What is stored:** only the value, plus a choice code ([L162-167](../Converted_Merge_Tree_Comparison/CO-MTED.cpp#L162-L167)):

* `-1` (CHOICE_MATCH) means case 3.
* `c ≥ 0` means case 1, through child `c` of `j`.
* `-(c+2)` means case 2, through child `c` of `i`.

The matching found inside `MM` is **thrown away** at this point (`pairs = nullptr`). It is recomputed only for the cells the final answer actually uses (phase D).

### 7a. Results of all cells

Each entry shows the stored value, with the winning case in small print (C1, C2 or C3).

**DT(i,j)**, subtree vs subtree. Cells where both nodes have the same ID are in bold.

| Tree1 \ Tree3 | T3:3 | T3:5 | T3:8 | T3:9 | T3:6 | T3:4 | T3:2 | T3:1 |
|---|---|---|---|---|---|---|---|---|
| **T1:3** | **5.5** <sub>C3</sub> | 23 <sub>C3</sub> | 35.5 <sub>C3</sub> | 14.5 <sub>C3</sub> | 19.5 <sub>C1</sub> | 49.5 <sub>C1</sub> | 116.5 <sub>C1</sub> | 169 <sub>C1</sub> |
| **T1:5** | 18.5 <sub>C3</sub> | **0** <sub>C3</sub> | 17.5 <sub>C3</sub> | 37.5 <sub>C3</sub> | 42.5 <sub>C1</sub> | 72.5 <sub>C1</sub> | 139.5 <sub>C1</sub> | 192 <sub>C1</sub> |
| **T1:8** | 28.5 <sub>C3</sub> | 20 <sub>C3</sub> | **2.5** <sub>C3</sub> | 47.5 <sub>C3</sub> | 52.5 <sub>C1</sub> | 82.5 <sub>C1</sub> | 149.5 <sub>C1</sub> | 202 <sub>C1</sub> |
| **T1:9** | 16.5 <sub>C3</sub> | 35 <sub>C3</sub> | 47.5 <sub>C3</sub> | **2.5** <sub>C3</sub> | 7.5 <sub>C1</sub> | 37.5 <sub>C1</sub> | 104.5 <sub>C1</sub> | 157 <sub>C1</sub> |
| **T1:6** | 26.5 <sub>C2</sub> | 45 <sub>C2</sub> | 57.5 <sub>C2</sub> | 12.5 <sub>C2</sub> | **7.5** <sub>C3</sub> | 37.5 <sub>C1</sub> | 94.5 <sub>C3</sub> | 147 <sub>C3</sub> |
| **T1:4** | 56.5 <sub>C2</sub> | 75 <sub>C2</sub> | 87.5 <sub>C2</sub> | 42.5 <sub>C2</sub> | 37.5 <sub>C2</sub> | **7.5** <sub>C3</sub> | 74.5 <sub>C3</sub> | 127 <sub>C3</sub> |
| **T1:2** | 132.5 <sub>C2</sub> | 151 <sub>C2</sub> | 163.5 <sub>C2</sub> | 118.5 <sub>C2</sub> | 113.5 <sub>C3</sub> | 83.5 <sub>C3</sub> | **18.5** <sub>C3</sub> | 61 <sub>C3</sub> |
| **T1:1** | 182.5 <sub>C2</sub> | 201 <sub>C2</sub> | 213.5 <sub>C2</sub> | 168.5 <sub>C2</sub> | 163.5 <sub>C3</sub> | 133.5 <sub>C3</sub> | 67.5 <sub>C3</sub> | **21** <sub>C3</sub> |

**DF(i,j)**, children-forest vs children-forest:

| Tree1 \ Tree3 | T3:3 | T3:5 | T3:8 | T3:9 | T3:6 | T3:4 | T3:2 | T3:1 |
|---|---|---|---|---|---|---|---|---|
| **T1:3** | 0 <sub>C3</sub> | 0 <sub>C3</sub> | 0 <sub>C3</sub> | 0 <sub>C3</sub> | 55 <sub>C3</sub> | 72.5 <sub>C3</sub> | 121 <sub>C3</sub> | 154.5 <sub>C3</sub> |
| **T1:5** | 0 <sub>C3</sub> | 0 <sub>C3</sub> | 0 <sub>C3</sub> | 0 <sub>C3</sub> | 55 <sub>C3</sub> | 72.5 <sub>C3</sub> | 121 <sub>C3</sub> | 154.5 <sub>C3</sub> |
| **T1:8** | 0 <sub>C3</sub> | 0 <sub>C3</sub> | 0 <sub>C3</sub> | 0 <sub>C3</sub> | 55 <sub>C3</sub> | 72.5 <sub>C3</sub> | 121 <sub>C3</sub> | 154.5 <sub>C3</sub> |
| **T1:9** | 0 <sub>C3</sub> | 0 <sub>C3</sub> | 0 <sub>C3</sub> | 0 <sub>C3</sub> | 55 <sub>C3</sub> | 72.5 <sub>C3</sub> | 121 <sub>C3</sub> | 154.5 <sub>C3</sub> |
| **T1:6** | 55 <sub>C3</sub> | 55 <sub>C3</sub> | 55 <sub>C3</sub> | 55 <sub>C3</sub> | 5 <sub>C3</sub> | 22.5 <sub>C1</sub> | 66 <sub>C3</sub> | 99.5 <sub>C1</sub> |
| **T1:4** | 75 <sub>C3</sub> | 75 <sub>C3</sub> | 75 <sub>C3</sub> | 75 <sub>C3</sub> | 25 <sub>C2</sub> | 7.5 <sub>C3</sub> | 56 <sub>C3</sub> | 89.5 <sub>C1</sub> |
| **T1:2** | 128 <sub>C3</sub> | 128 <sub>C3</sub> | 128 <sub>C3</sub> | 128 <sub>C3</sub> | 78 <sub>C3</sub> | 60.5 <sub>C3</sub> | 13 <sub>C3</sub> | 46.5 <sub>C1</sub> |
| **T1:1** | 166 <sub>C3</sub> | 166 <sub>C3</sub> | 166 <sub>C3</sub> | 166 <sub>C3</sub> | 116 <sub>C2</sub> | 98.5 <sub>C2</sub> | 51 <sub>C2</sub> | 18.5 <sub>C3</sub> |

### 7b. Every cell, in the exact order the loop visits it

Click a cell to expand its complete trace. The summary line shows what the cell stores.

<details><summary><b>i=0, j=0: (T1:3, T3:3)</b> → DF = 0 (case 3), DT = 5.5 (case 3)</summary>

```text
--- [L194-195] i = 0, j = 0   cell (T1:3,T3:3) ---
[L196] computeDF(T1:3,T3:3)
|   T1:3 children: none   |   T3:3 children: none
|   [L241-247] CASE 1: delete T3:3, put T1:3's forest into one child of T3:3
|   |   [L241-242] minCost = MAX, case1Child = -1
|   |   [L243] T3:3 has no children: loop does not run, minCost stays MAX
|   |   [L247] case1 = DF(-,T3:3) + minCost = 0 + MAX = MAX
|   [L250-256] CASE 2: delete T1:3, put one child of T1:3 onto T3:3's forest
|   |   [L250-251] minCost = MAX, case2Child = -1
|   |   [L252] T1:3 has no children: loop does not run, minCost stays MAX
|   |   [L256] case2 = DF(T1:3,-) + minCost = 0 + MAX = MAX
|   [L259] CASE 3: case3 = MM(T1:3,T3:3)  (match the two child forests)
|   |   [L294-295] a = #children of T1:3 = 0, b = #children of T3:3 = 0
|   |   [L296] a == 0 && b == 0 -> return 0 (two empty forests)
|   [L259] case3 = 0
|   [L262] decide:
|   |   [L225] choose(case1 = MAX, case2 = MAX, case3 = 0)
|   |   [L227] case3 - 0.00001 <= case2 ? yes   case3 - 0.00001 <= case1 ? yes
|   |   [L228-229] -> CASE 3 wins, value 0, return CHOICE_MATCH (-1)
|   [L262-263] STORE DF(T1:3,T3:3) = 0   DFchoice = CHOICE_MATCH (case 3)
[L197] computeDT(T1:3,T3:3)
|   [L267-273] CASE 1: delete T3:3, put subtree T1:3 into one child subtree of T3:3
|   |   [L267-268] minCost = MAX, case1Child = -1
|   |   [L269] T3:3 has no children: loop does not run, minCost stays MAX
|   |   [L273] case1 = DT(-,T3:3) + minCost = 33.5 + MAX = MAX
|   [L275-281] CASE 2: delete T1:3, put one child subtree of T1:3 onto subtree T3:3
|   |   [L275-276] minCost = MAX, case2Child = -1
|   |   [L277] T1:3 has no children: loop does not run, minCost stays MAX
|   |   [L281] case2 = DT(T1:3,-) + minCost = 38 + MAX = MAX
|   [L283] CASE 3: match T1:3 <-> T3:3: case3 = DF(T1:3,T3:3) + relabelCost(T1:3,T3:3)
|   |   [L151] diff1 = |4 - 3| + |80 - 70| = 11   (move one node onto the other)
|   |   [L152] diff2 = |80 - 4| + |70 - 3| = 143   (delete both)
|   |   [L153] relabelCost = min(11, 143) / 2 = 5.5
|   [L283] case3 = 0 + 5.5 = 5.5
|   [L286] decide:
|   |   [L225] choose(case1 = MAX, case2 = MAX, case3 = 5.5)
|   |   [L227] case3 - 0.00001 <= case2 ? yes   case3 - 0.00001 <= case1 ? yes
|   |   [L228-229] -> CASE 3 wins, value 5.5, return CHOICE_MATCH (-1)
|   [L286-287] STORE DT(T1:3,T3:3) = 5.5   DTchoice = CHOICE_MATCH (case 3)
```

</details>

<details><summary><b>i=0, j=1: (T1:3, T3:5)</b> → DF = 0 (case 3), DT = 23 (case 3)</summary>

```text
--- [L194-195] i = 0, j = 1   cell (T1:3,T3:5) ---
[L196] computeDF(T1:3,T3:5)
|   T1:3 children: none   |   T3:5 children: none
|   [L241-247] CASE 1: delete T3:5, put T1:3's forest into one child of T3:5
|   |   [L241-242] minCost = MAX, case1Child = -1
|   |   [L243] T3:5 has no children: loop does not run, minCost stays MAX
|   |   [L247] case1 = DF(-,T3:5) + minCost = 0 + MAX = MAX
|   [L250-256] CASE 2: delete T1:3, put one child of T1:3 onto T3:5's forest
|   |   [L250-251] minCost = MAX, case2Child = -1
|   |   [L252] T1:3 has no children: loop does not run, minCost stays MAX
|   |   [L256] case2 = DF(T1:3,-) + minCost = 0 + MAX = MAX
|   [L259] CASE 3: case3 = MM(T1:3,T3:5)  (match the two child forests)
|   |   [L294-295] a = #children of T1:3 = 0, b = #children of T3:5 = 0
|   |   [L296] a == 0 && b == 0 -> return 0 (two empty forests)
|   [L259] case3 = 0
|   [L262] decide:
|   |   [L225] choose(case1 = MAX, case2 = MAX, case3 = 0)
|   |   [L227] case3 - 0.00001 <= case2 ? yes   case3 - 0.00001 <= case1 ? yes
|   |   [L228-229] -> CASE 3 wins, value 0, return CHOICE_MATCH (-1)
|   [L262-263] STORE DF(T1:3,T3:5) = 0   DFchoice = CHOICE_MATCH (case 3)
[L197] computeDT(T1:3,T3:5)
|   [L267-273] CASE 1: delete T3:5, put subtree T1:3 into one child subtree of T3:5
|   |   [L267-268] minCost = MAX, case1Child = -1
|   |   [L269] T3:5 has no children: loop does not run, minCost stays MAX
|   |   [L273] case1 = DT(-,T3:5) + minCost = 15 + MAX = MAX
|   [L275-281] CASE 2: delete T1:3, put one child subtree of T1:3 onto subtree T3:5
|   |   [L275-276] minCost = MAX, case2Child = -1
|   |   [L277] T1:3 has no children: loop does not run, minCost stays MAX
|   |   [L281] case2 = DT(T1:3,-) + minCost = 38 + MAX = MAX
|   [L283] CASE 3: match T1:3 <-> T3:5: case3 = DF(T1:3,T3:5) + relabelCost(T1:3,T3:5)
|   |   [L151] diff1 = |4 - 30| + |80 - 60| = 46   (move one node onto the other)
|   |   [L152] diff2 = |80 - 4| + |60 - 30| = 106   (delete both)
|   |   [L153] relabelCost = min(46, 106) / 2 = 23
|   [L283] case3 = 0 + 23 = 23
|   [L286] decide:
|   |   [L225] choose(case1 = MAX, case2 = MAX, case3 = 23)
|   |   [L227] case3 - 0.00001 <= case2 ? yes   case3 - 0.00001 <= case1 ? yes
|   |   [L228-229] -> CASE 3 wins, value 23, return CHOICE_MATCH (-1)
|   [L286-287] STORE DT(T1:3,T3:5) = 23   DTchoice = CHOICE_MATCH (case 3)
```

</details>

<details><summary><b>i=0, j=2: (T1:3, T3:8)</b> → DF = 0 (case 3), DT = 35.5 (case 3)</summary>

```text
--- [L194-195] i = 0, j = 2   cell (T1:3,T3:8) ---
[L196] computeDF(T1:3,T3:8)
|   T1:3 children: none   |   T3:8 children: none
|   [L241-247] CASE 1: delete T3:8, put T1:3's forest into one child of T3:8
|   |   [L241-242] minCost = MAX, case1Child = -1
|   |   [L243] T3:8 has no children: loop does not run, minCost stays MAX
|   |   [L247] case1 = DF(-,T3:8) + minCost = 0 + MAX = MAX
|   [L250-256] CASE 2: delete T1:3, put one child of T1:3 onto T3:8's forest
|   |   [L250-251] minCost = MAX, case2Child = -1
|   |   [L252] T1:3 has no children: loop does not run, minCost stays MAX
|   |   [L256] case2 = DF(T1:3,-) + minCost = 0 + MAX = MAX
|   [L259] CASE 3: case3 = MM(T1:3,T3:8)  (match the two child forests)
|   |   [L294-295] a = #children of T1:3 = 0, b = #children of T3:8 = 0
|   |   [L296] a == 0 && b == 0 -> return 0 (two empty forests)
|   [L259] case3 = 0
|   [L262] decide:
|   |   [L225] choose(case1 = MAX, case2 = MAX, case3 = 0)
|   |   [L227] case3 - 0.00001 <= case2 ? yes   case3 - 0.00001 <= case1 ? yes
|   |   [L228-229] -> CASE 3 wins, value 0, return CHOICE_MATCH (-1)
|   [L262-263] STORE DF(T1:3,T3:8) = 0   DFchoice = CHOICE_MATCH (case 3)
[L197] computeDT(T1:3,T3:8)
|   [L267-273] CASE 1: delete T3:8, put subtree T1:3 into one child subtree of T3:8
|   |   [L267-268] minCost = MAX, case1Child = -1
|   |   [L269] T3:8 has no children: loop does not run, minCost stays MAX
|   |   [L273] case1 = DT(-,T3:8) + minCost = 2.5 + MAX = MAX
|   [L275-281] CASE 2: delete T1:3, put one child subtree of T1:3 onto subtree T3:8
|   |   [L275-276] minCost = MAX, case2Child = -1
|   |   [L277] T1:3 has no children: loop does not run, minCost stays MAX
|   |   [L281] case2 = DT(T1:3,-) + minCost = 38 + MAX = MAX
|   [L283] CASE 3: match T1:3 <-> T3:8: case3 = DF(T1:3,T3:8) + relabelCost(T1:3,T3:8)
|   |   [L151] diff1 = |4 - 5| + |80 - 10| = 71   (move one node onto the other)
|   |   [L152] diff2 = |80 - 4| + |10 - 5| = 81   (delete both)
|   |   [L153] relabelCost = min(71, 81) / 2 = 35.5
|   [L283] case3 = 0 + 35.5 = 35.5
|   [L286] decide:
|   |   [L225] choose(case1 = MAX, case2 = MAX, case3 = 35.5)
|   |   [L227] case3 - 0.00001 <= case2 ? yes   case3 - 0.00001 <= case1 ? yes
|   |   [L228-229] -> CASE 3 wins, value 35.5, return CHOICE_MATCH (-1)
|   [L286-287] STORE DT(T1:3,T3:8) = 35.5   DTchoice = CHOICE_MATCH (case 3)
```

</details>

<details><summary><b>i=0, j=3: (T1:3, T3:9)</b> → DF = 0 (case 3), DT = 14.5 (case 3)</summary>

```text
--- [L194-195] i = 0, j = 3   cell (T1:3,T3:9) ---
[L196] computeDF(T1:3,T3:9)
|   T1:3 children: none   |   T3:9 children: none
|   [L241-247] CASE 1: delete T3:9, put T1:3's forest into one child of T3:9
|   |   [L241-242] minCost = MAX, case1Child = -1
|   |   [L243] T3:9 has no children: loop does not run, minCost stays MAX
|   |   [L247] case1 = DF(-,T3:9) + minCost = 0 + MAX = MAX
|   [L250-256] CASE 2: delete T1:3, put one child of T1:3 onto T3:9's forest
|   |   [L250-251] minCost = MAX, case2Child = -1
|   |   [L252] T1:3 has no children: loop does not run, minCost stays MAX
|   |   [L256] case2 = DF(T1:3,-) + minCost = 0 + MAX = MAX
|   [L259] CASE 3: case3 = MM(T1:3,T3:9)  (match the two child forests)
|   |   [L294-295] a = #children of T1:3 = 0, b = #children of T3:9 = 0
|   |   [L296] a == 0 && b == 0 -> return 0 (two empty forests)
|   [L259] case3 = 0
|   [L262] decide:
|   |   [L225] choose(case1 = MAX, case2 = MAX, case3 = 0)
|   |   [L227] case3 - 0.00001 <= case2 ? yes   case3 - 0.00001 <= case1 ? yes
|   |   [L228-229] -> CASE 3 wins, value 0, return CHOICE_MATCH (-1)
|   [L262-263] STORE DF(T1:3,T3:9) = 0   DFchoice = CHOICE_MATCH (case 3)
[L197] computeDT(T1:3,T3:9)
|   [L267-273] CASE 1: delete T3:9, put subtree T1:3 into one child subtree of T3:9
|   |   [L267-268] minCost = MAX, case1Child = -1
|   |   [L269] T3:9 has no children: loop does not run, minCost stays MAX
|   |   [L273] case1 = DT(-,T3:9) + minCost = 52.5 + MAX = MAX
|   [L275-281] CASE 2: delete T1:3, put one child subtree of T1:3 onto subtree T3:9
|   |   [L275-276] minCost = MAX, case2Child = -1
|   |   [L277] T1:3 has no children: loop does not run, minCost stays MAX
|   |   [L281] case2 = DT(T1:3,-) + minCost = 38 + MAX = MAX
|   [L283] CASE 3: match T1:3 <-> T3:9: case3 = DF(T1:3,T3:9) + relabelCost(T1:3,T3:9)
|   |   [L151] diff1 = |4 - 0| + |80 - 105| = 29   (move one node onto the other)
|   |   [L152] diff2 = |80 - 4| + |105 - 0| = 181   (delete both)
|   |   [L153] relabelCost = min(29, 181) / 2 = 14.5
|   [L283] case3 = 0 + 14.5 = 14.5
|   [L286] decide:
|   |   [L225] choose(case1 = MAX, case2 = MAX, case3 = 14.5)
|   |   [L227] case3 - 0.00001 <= case2 ? yes   case3 - 0.00001 <= case1 ? yes
|   |   [L228-229] -> CASE 3 wins, value 14.5, return CHOICE_MATCH (-1)
|   [L286-287] STORE DT(T1:3,T3:9) = 14.5   DTchoice = CHOICE_MATCH (case 3)
```

</details>

<details><summary><b>i=0, j=4: (T1:3, T3:6)</b> → DF = 55 (case 3), DT = 19.5 (case 1) · MM: <code>-<->T3:8 2.5 + -<->T3:9 52.5</code></summary>

```text
--- [L194-195] i = 0, j = 4   cell (T1:3,T3:6) ---
[L196] computeDF(T1:3,T3:6)
|   T1:3 children: none   |   T3:6 children: T3:8,T3:9
|   [L241-247] CASE 1: delete T3:6, put T1:3's forest into one child of T3:6
|   |   [L241-242] minCost = MAX, case1Child = -1
|   |   [L243-246] child T3:8: DF(T1:3,T3:8) - DF(-,T3:8) = 0 - 0 = 0  < min -> minCost = 0, case1Child = T3:8
|   |   [L243-246] child T3:9: DF(T1:3,T3:9) - DF(-,T3:9) = 0 - 0 = 0  not smaller
|   |   [L247] case1 = DF(-,T3:6) + minCost = 55 + 0 = 55
|   [L250-256] CASE 2: delete T1:3, put one child of T1:3 onto T3:6's forest
|   |   [L250-251] minCost = MAX, case2Child = -1
|   |   [L252] T1:3 has no children: loop does not run, minCost stays MAX
|   |   [L256] case2 = DF(T1:3,-) + minCost = 0 + MAX = MAX
|   [L259] CASE 3: case3 = MM(T1:3,T3:6)  (match the two child forests)
|   |   [L294-295] a = #children of T1:3 = 0, b = #children of T3:6 = 2
|   |   [L298] n = a + b = 2 (each side padded with dummies '-' up to n)
|   |   [L302-308] cost matrix, entry [k][l] = DT(row k, col l):
|   |                T3:8      T3:9      
|   |   -            2.5       52.5      
|   |   -            2.5       52.5      
|   |   [L309] kuhnMunkresMinWeightPerfectMatching(mmCost, 2):
|   |   |   [KM:L29-33] potentials u (rows) = 0, v (cols) = 0; p[col] = row assigned to col (0 = none). Rows/cols are 1-based here.
|   |   |   [KM:L35-39] add row r1(-): p[0] = 1, j0 = 0, minv = INF, used = none
|   |   |   |   [KM:L41-51] mark start used; its row i0 = r1(-). reduced costs cost-u-v for unused cols: c1:2.5-0-(0)=2.5->minv 2.5; c2:52.5-0-(0)=52.5->minv 52.5;
|   |   |   |              smallest minv: delta = 2.5 at c1(T3:8)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [2.5,0], v = [0,0]
|   |   |   |   [KM:L56-57] c1(T3:8) is free -> augmenting path found
|   |   |   |   [KM:L58-62] flip the path: c1(T3:8)<-r1(-);
|   |   |   [KM:L35-39] add row r2(-): p[0] = 2, j0 = 0, minv = INF, used = none
|   |   |   |   [KM:L41-51] mark start used; its row i0 = r2(-). reduced costs cost-u-v for unused cols: c1:2.5-0-(0)=2.5->minv 2.5; c2:52.5-0-(0)=52.5->minv 52.5;
|   |   |   |              smallest minv: delta = 2.5 at c1(T3:8)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [2.5,2.5], v = [0,0]
|   |   |   |   [KM:L56-57] c1(T3:8) is already taken by r1(-) -> keep searching from it
|   |   |   |   [KM:L41-51] mark c1(T3:8) used; its row i0 = r1(-). reduced costs cost-u-v for unused cols: c2:52.5-2.5-(0)=50->minv 50;
|   |   |   |              smallest minv: delta = 50 at c2(T3:9)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [52.5,52.5], v = [-50,0]
|   |   |   |   [KM:L56-57] c2(T3:9) is free -> augmenting path found
|   |   |   |   [KM:L58-62] flip the path: c2(T3:9)<-r2(-);
|   |   |   [KM:L65-70] KM RESULT: -<->T3:8 2.5 + -<->T3:9 52.5 = total 55
|   |   [L319] MM returns 55
|   [L259] case3 = 55
|   [L262] decide:
|   |   [L225] choose(case1 = 55, case2 = MAX, case3 = 55)
|   |   [L227] case3 - 0.00001 <= case2 ? yes   case3 - 0.00001 <= case1 ? yes
|   |   [L228-229] -> CASE 3 wins, value 55, return CHOICE_MATCH (-1)
|   [L262-263] STORE DF(T1:3,T3:6) = 55   DFchoice = CHOICE_MATCH (case 3)
[L197] computeDT(T1:3,T3:6)
|   [L267-273] CASE 1: delete T3:6, put subtree T1:3 into one child subtree of T3:6
|   |   [L267-268] minCost = MAX, case1Child = -1
|   |   [L269-272] child T3:8: DT(T1:3,T3:8) - DT(-,T3:8) = 35.5 - 2.5 = 33  < min -> minCost = 33, case1Child = T3:8
|   |   [L269-272] child T3:9: DT(T1:3,T3:9) - DT(-,T3:9) = 14.5 - 52.5 = -38  < min -> minCost = -38, case1Child = T3:9
|   |   [L273] case1 = DT(-,T3:6) + minCost = 57.5 + -38 = 19.5
|   [L275-281] CASE 2: delete T1:3, put one child subtree of T1:3 onto subtree T3:6
|   |   [L275-276] minCost = MAX, case2Child = -1
|   |   [L277] T1:3 has no children: loop does not run, minCost stays MAX
|   |   [L281] case2 = DT(T1:3,-) + minCost = 38 + MAX = MAX
|   [L283] CASE 3: match T1:3 <-> T3:6: case3 = DF(T1:3,T3:6) + relabelCost(T1:3,T3:6)
|   |   [L151] diff1 = |4 - 10| + |80 - 5| = 81   (move one node onto the other)
|   |   [L152] diff2 = |80 - 4| + |5 - 10| = 81   (delete both)
|   |   [L153] relabelCost = min(81, 81) / 2 = 40.5
|   [L283] case3 = 55 + 40.5 = 95.5
|   [L286] decide:
|   |   [L225] choose(case1 = 19.5, case2 = MAX, case3 = 95.5)
|   |   [L227] case3 - 0.00001 <= case2 ? yes   case3 - 0.00001 <= case1 ? no
|   |   [L230-232] case1 < case2 ? yes -> CASE 1 wins, value 19.5, return child index 3
|   [L286-287] STORE DT(T1:3,T3:6) = 19.5   DTchoice = 3 (case 1, T3 child T3:9)
```

</details>

<details><summary><b>i=0, j=5: (T1:3, T3:4)</b> → DF = 72.5 (case 3), DT = 49.5 (case 1) · MM: <code>-<->T3:5 15 + -<->T3:6 57.5</code></summary>

```text
--- [L194-195] i = 0, j = 5   cell (T1:3,T3:4) ---
[L196] computeDF(T1:3,T3:4)
|   T1:3 children: none   |   T3:4 children: T3:5,T3:6
|   [L241-247] CASE 1: delete T3:4, put T1:3's forest into one child of T3:4
|   |   [L241-242] minCost = MAX, case1Child = -1
|   |   [L243-246] child T3:5: DF(T1:3,T3:5) - DF(-,T3:5) = 0 - 0 = 0  < min -> minCost = 0, case1Child = T3:5
|   |   [L243-246] child T3:6: DF(T1:3,T3:6) - DF(-,T3:6) = 55 - 55 = 0  not smaller
|   |   [L247] case1 = DF(-,T3:4) + minCost = 72.5 + 0 = 72.5
|   [L250-256] CASE 2: delete T1:3, put one child of T1:3 onto T3:4's forest
|   |   [L250-251] minCost = MAX, case2Child = -1
|   |   [L252] T1:3 has no children: loop does not run, minCost stays MAX
|   |   [L256] case2 = DF(T1:3,-) + minCost = 0 + MAX = MAX
|   [L259] CASE 3: case3 = MM(T1:3,T3:4)  (match the two child forests)
|   |   [L294-295] a = #children of T1:3 = 0, b = #children of T3:4 = 2
|   |   [L298] n = a + b = 2 (each side padded with dummies '-' up to n)
|   |   [L302-308] cost matrix, entry [k][l] = DT(row k, col l):
|   |                T3:5      T3:6      
|   |   -            15        57.5      
|   |   -            15        57.5      
|   |   [L309] kuhnMunkresMinWeightPerfectMatching(mmCost, 2):
|   |   |   [KM:L29-33] potentials u (rows) = 0, v (cols) = 0; p[col] = row assigned to col (0 = none). Rows/cols are 1-based here.
|   |   |   [KM:L35-39] add row r1(-): p[0] = 1, j0 = 0, minv = INF, used = none
|   |   |   |   [KM:L41-51] mark start used; its row i0 = r1(-). reduced costs cost-u-v for unused cols: c1:15-0-(0)=15->minv 15; c2:57.5-0-(0)=57.5->minv 57.5;
|   |   |   |              smallest minv: delta = 15 at c1(T3:5)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [15,0], v = [0,0]
|   |   |   |   [KM:L56-57] c1(T3:5) is free -> augmenting path found
|   |   |   |   [KM:L58-62] flip the path: c1(T3:5)<-r1(-);
|   |   |   [KM:L35-39] add row r2(-): p[0] = 2, j0 = 0, minv = INF, used = none
|   |   |   |   [KM:L41-51] mark start used; its row i0 = r2(-). reduced costs cost-u-v for unused cols: c1:15-0-(0)=15->minv 15; c2:57.5-0-(0)=57.5->minv 57.5;
|   |   |   |              smallest minv: delta = 15 at c1(T3:5)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [15,15], v = [0,0]
|   |   |   |   [KM:L56-57] c1(T3:5) is already taken by r1(-) -> keep searching from it
|   |   |   |   [KM:L41-51] mark c1(T3:5) used; its row i0 = r1(-). reduced costs cost-u-v for unused cols: c2:57.5-15-(0)=42.5->minv 42.5;
|   |   |   |              smallest minv: delta = 42.5 at c2(T3:6)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [57.5,57.5], v = [-42.5,0]
|   |   |   |   [KM:L56-57] c2(T3:6) is free -> augmenting path found
|   |   |   |   [KM:L58-62] flip the path: c2(T3:6)<-r2(-);
|   |   |   [KM:L65-70] KM RESULT: -<->T3:5 15 + -<->T3:6 57.5 = total 72.5
|   |   [L319] MM returns 72.5
|   [L259] case3 = 72.5
|   [L262] decide:
|   |   [L225] choose(case1 = 72.5, case2 = MAX, case3 = 72.5)
|   |   [L227] case3 - 0.00001 <= case2 ? yes   case3 - 0.00001 <= case1 ? yes
|   |   [L228-229] -> CASE 3 wins, value 72.5, return CHOICE_MATCH (-1)
|   [L262-263] STORE DF(T1:3,T3:4) = 72.5   DFchoice = CHOICE_MATCH (case 3)
[L197] computeDT(T1:3,T3:4)
|   [L267-273] CASE 1: delete T3:4, put subtree T1:3 into one child subtree of T3:4
|   |   [L267-268] minCost = MAX, case1Child = -1
|   |   [L269-272] child T3:5: DT(T1:3,T3:5) - DT(-,T3:5) = 23 - 15 = 8  < min -> minCost = 8, case1Child = T3:5
|   |   [L269-272] child T3:6: DT(T1:3,T3:6) - DT(-,T3:6) = 19.5 - 57.5 = -38  < min -> minCost = -38, case1Child = T3:6
|   |   [L273] case1 = DT(-,T3:4) + minCost = 87.5 + -38 = 49.5
|   [L275-281] CASE 2: delete T1:3, put one child subtree of T1:3 onto subtree T3:4
|   |   [L275-276] minCost = MAX, case2Child = -1
|   |   [L277] T1:3 has no children: loop does not run, minCost stays MAX
|   |   [L281] case2 = DT(T1:3,-) + minCost = 38 + MAX = MAX
|   [L283] CASE 3: match T1:3 <-> T3:4: case3 = DF(T1:3,T3:4) + relabelCost(T1:3,T3:4)
|   |   [L151] diff1 = |4 - 60| + |80 - 30| = 106   (move one node onto the other)
|   |   [L152] diff2 = |80 - 4| + |30 - 60| = 106   (delete both)
|   |   [L153] relabelCost = min(106, 106) / 2 = 53
|   [L283] case3 = 72.5 + 53 = 125.5
|   [L286] decide:
|   |   [L225] choose(case1 = 49.5, case2 = MAX, case3 = 125.5)
|   |   [L227] case3 - 0.00001 <= case2 ? yes   case3 - 0.00001 <= case1 ? no
|   |   [L230-232] case1 < case2 ? yes -> CASE 1 wins, value 49.5, return child index 4
|   [L286-287] STORE DT(T1:3,T3:4) = 49.5   DTchoice = 4 (case 1, T3 child T3:6)
```

</details>

<details><summary><b>i=0, j=6: (T1:3, T3:2)</b> → DF = 121 (case 3), DT = 116.5 (case 1) · MM: <code>-<->T3:3 33.5 + -<->T3:4 87.5</code></summary>

```text
--- [L194-195] i = 0, j = 6   cell (T1:3,T3:2) ---
[L196] computeDF(T1:3,T3:2)
|   T1:3 children: none   |   T3:2 children: T3:3,T3:4
|   [L241-247] CASE 1: delete T3:2, put T1:3's forest into one child of T3:2
|   |   [L241-242] minCost = MAX, case1Child = -1
|   |   [L243-246] child T3:3: DF(T1:3,T3:3) - DF(-,T3:3) = 0 - 0 = 0  < min -> minCost = 0, case1Child = T3:3
|   |   [L243-246] child T3:4: DF(T1:3,T3:4) - DF(-,T3:4) = 72.5 - 72.5 = 0  not smaller
|   |   [L247] case1 = DF(-,T3:2) + minCost = 121 + 0 = 121
|   [L250-256] CASE 2: delete T1:3, put one child of T1:3 onto T3:2's forest
|   |   [L250-251] minCost = MAX, case2Child = -1
|   |   [L252] T1:3 has no children: loop does not run, minCost stays MAX
|   |   [L256] case2 = DF(T1:3,-) + minCost = 0 + MAX = MAX
|   [L259] CASE 3: case3 = MM(T1:3,T3:2)  (match the two child forests)
|   |   [L294-295] a = #children of T1:3 = 0, b = #children of T3:2 = 2
|   |   [L298] n = a + b = 2 (each side padded with dummies '-' up to n)
|   |   [L302-308] cost matrix, entry [k][l] = DT(row k, col l):
|   |                T3:3      T3:4      
|   |   -            33.5      87.5      
|   |   -            33.5      87.5      
|   |   [L309] kuhnMunkresMinWeightPerfectMatching(mmCost, 2):
|   |   |   [KM:L29-33] potentials u (rows) = 0, v (cols) = 0; p[col] = row assigned to col (0 = none). Rows/cols are 1-based here.
|   |   |   [KM:L35-39] add row r1(-): p[0] = 1, j0 = 0, minv = INF, used = none
|   |   |   |   [KM:L41-51] mark start used; its row i0 = r1(-). reduced costs cost-u-v for unused cols: c1:33.5-0-(0)=33.5->minv 33.5; c2:87.5-0-(0)=87.5->minv 87.5;
|   |   |   |              smallest minv: delta = 33.5 at c1(T3:3)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [33.5,0], v = [0,0]
|   |   |   |   [KM:L56-57] c1(T3:3) is free -> augmenting path found
|   |   |   |   [KM:L58-62] flip the path: c1(T3:3)<-r1(-);
|   |   |   [KM:L35-39] add row r2(-): p[0] = 2, j0 = 0, minv = INF, used = none
|   |   |   |   [KM:L41-51] mark start used; its row i0 = r2(-). reduced costs cost-u-v for unused cols: c1:33.5-0-(0)=33.5->minv 33.5; c2:87.5-0-(0)=87.5->minv 87.5;
|   |   |   |              smallest minv: delta = 33.5 at c1(T3:3)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [33.5,33.5], v = [0,0]
|   |   |   |   [KM:L56-57] c1(T3:3) is already taken by r1(-) -> keep searching from it
|   |   |   |   [KM:L41-51] mark c1(T3:3) used; its row i0 = r1(-). reduced costs cost-u-v for unused cols: c2:87.5-33.5-(0)=54->minv 54;
|   |   |   |              smallest minv: delta = 54 at c2(T3:4)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [87.5,87.5], v = [-54,0]
|   |   |   |   [KM:L56-57] c2(T3:4) is free -> augmenting path found
|   |   |   |   [KM:L58-62] flip the path: c2(T3:4)<-r2(-);
|   |   |   [KM:L65-70] KM RESULT: -<->T3:3 33.5 + -<->T3:4 87.5 = total 121
|   |   [L319] MM returns 121
|   [L259] case3 = 121
|   [L262] decide:
|   |   [L225] choose(case1 = 121, case2 = MAX, case3 = 121)
|   |   [L227] case3 - 0.00001 <= case2 ? yes   case3 - 0.00001 <= case1 ? yes
|   |   [L228-229] -> CASE 3 wins, value 121, return CHOICE_MATCH (-1)
|   [L262-263] STORE DF(T1:3,T3:2) = 121   DFchoice = CHOICE_MATCH (case 3)
[L197] computeDT(T1:3,T3:2)
|   [L267-273] CASE 1: delete T3:2, put subtree T1:3 into one child subtree of T3:2
|   |   [L267-268] minCost = MAX, case1Child = -1
|   |   [L269-272] child T3:3: DT(T1:3,T3:3) - DT(-,T3:3) = 5.5 - 33.5 = -28  < min -> minCost = -28, case1Child = T3:3
|   |   [L269-272] child T3:4: DT(T1:3,T3:4) - DT(-,T3:4) = 49.5 - 87.5 = -38  < min -> minCost = -38, case1Child = T3:4
|   |   [L273] case1 = DT(-,T3:2) + minCost = 154.5 + -38 = 116.5
|   [L275-281] CASE 2: delete T1:3, put one child subtree of T1:3 onto subtree T3:2
|   |   [L275-276] minCost = MAX, case2Child = -1
|   |   [L277] T1:3 has no children: loop does not run, minCost stays MAX
|   |   [L281] case2 = DT(T1:3,-) + minCost = 38 + MAX = MAX
|   [L283] CASE 3: match T1:3 <-> T3:2: case3 = DF(T1:3,T3:2) + relabelCost(T1:3,T3:2)
|   |   [L151] diff1 = |4 - 70| + |80 - 3| = 143   (move one node onto the other)
|   |   [L152] diff2 = |80 - 4| + |3 - 70| = 143   (delete both)
|   |   [L153] relabelCost = min(143, 143) / 2 = 71.5
|   [L283] case3 = 121 + 71.5 = 192.5
|   [L286] decide:
|   |   [L225] choose(case1 = 116.5, case2 = MAX, case3 = 192.5)
|   |   [L227] case3 - 0.00001 <= case2 ? yes   case3 - 0.00001 <= case1 ? no
|   |   [L230-232] case1 < case2 ? yes -> CASE 1 wins, value 116.5, return child index 5
|   [L286-287] STORE DT(T1:3,T3:2) = 116.5   DTchoice = 5 (case 1, T3 child T3:4)
```

</details>

<details><summary><b>i=0, j=7: (T1:3, T3:1)</b> → DF = 154.5 (case 3), DT = 169 (case 1) · MM: <code>-<->T3:2 154.5</code></summary>

```text
--- [L194-195] i = 0, j = 7   cell (T1:3,T3:1) ---
[L196] computeDF(T1:3,T3:1)
|   T1:3 children: none   |   T3:1 children: T3:2
|   [L241-247] CASE 1: delete T3:1, put T1:3's forest into one child of T3:1
|   |   [L241-242] minCost = MAX, case1Child = -1
|   |   [L243-246] child T3:2: DF(T1:3,T3:2) - DF(-,T3:2) = 121 - 121 = 0  < min -> minCost = 0, case1Child = T3:2
|   |   [L247] case1 = DF(-,T3:1) + minCost = 154.5 + 0 = 154.5
|   [L250-256] CASE 2: delete T1:3, put one child of T1:3 onto T3:1's forest
|   |   [L250-251] minCost = MAX, case2Child = -1
|   |   [L252] T1:3 has no children: loop does not run, minCost stays MAX
|   |   [L256] case2 = DF(T1:3,-) + minCost = 0 + MAX = MAX
|   [L259] CASE 3: case3 = MM(T1:3,T3:1)  (match the two child forests)
|   |   [L294-295] a = #children of T1:3 = 0, b = #children of T3:1 = 1
|   |   [L298] n = a + b = 1 (each side padded with dummies '-' up to n)
|   |   [L302-308] cost matrix, entry [k][l] = DT(row k, col l):
|   |                T3:2      
|   |   -            154.5     
|   |   [L309] kuhnMunkresMinWeightPerfectMatching(mmCost, 1):
|   |   |   [KM:L29-33] potentials u (rows) = 0, v (cols) = 0; p[col] = row assigned to col (0 = none). Rows/cols are 1-based here.
|   |   |   [KM:L35-39] add row r1(-): p[0] = 1, j0 = 0, minv = INF, used = none
|   |   |   |   [KM:L41-51] mark start used; its row i0 = r1(-). reduced costs cost-u-v for unused cols: c1:154.5-0-(0)=154.5->minv 154.5;
|   |   |   |              smallest minv: delta = 154.5 at c1(T3:2)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [154.5], v = [0]
|   |   |   |   [KM:L56-57] c1(T3:2) is free -> augmenting path found
|   |   |   |   [KM:L58-62] flip the path: c1(T3:2)<-r1(-);
|   |   |   [KM:L65-70] KM RESULT: -<->T3:2 154.5 = total 154.5
|   |   [L319] MM returns 154.5
|   [L259] case3 = 154.5
|   [L262] decide:
|   |   [L225] choose(case1 = 154.5, case2 = MAX, case3 = 154.5)
|   |   [L227] case3 - 0.00001 <= case2 ? yes   case3 - 0.00001 <= case1 ? yes
|   |   [L228-229] -> CASE 3 wins, value 154.5, return CHOICE_MATCH (-1)
|   [L262-263] STORE DF(T1:3,T3:1) = 154.5   DFchoice = CHOICE_MATCH (case 3)
[L197] computeDT(T1:3,T3:1)
|   [L267-273] CASE 1: delete T3:1, put subtree T1:3 into one child subtree of T3:1
|   |   [L267-268] minCost = MAX, case1Child = -1
|   |   [L269-272] child T3:2: DT(T1:3,T3:2) - DT(-,T3:2) = 116.5 - 154.5 = -38  < min -> minCost = -38, case1Child = T3:2
|   |   [L273] case1 = DT(-,T3:1) + minCost = 207 + -38 = 169
|   [L275-281] CASE 2: delete T1:3, put one child subtree of T1:3 onto subtree T3:1
|   |   [L275-276] minCost = MAX, case2Child = -1
|   |   [L277] T1:3 has no children: loop does not run, minCost stays MAX
|   |   [L281] case2 = DT(T1:3,-) + minCost = 38 + MAX = MAX
|   [L283] CASE 3: match T1:3 <-> T3:1: case3 = DF(T1:3,T3:1) + relabelCost(T1:3,T3:1)
|   |   [L151] diff1 = |4 - 105| + |80 - 0| = 181   (move one node onto the other)
|   |   [L152] diff2 = |80 - 4| + |0 - 105| = 181   (delete both)
|   |   [L153] relabelCost = min(181, 181) / 2 = 90.5
|   [L283] case3 = 154.5 + 90.5 = 245
|   [L286] decide:
|   |   [L225] choose(case1 = 169, case2 = MAX, case3 = 245)
|   |   [L227] case3 - 0.00001 <= case2 ? yes   case3 - 0.00001 <= case1 ? no
|   |   [L230-232] case1 < case2 ? yes -> CASE 1 wins, value 169, return child index 6
|   [L286-287] STORE DT(T1:3,T3:1) = 169   DTchoice = 6 (case 1, T3 child T3:2)
```

</details>

<details><summary><b>i=1, j=0: (T1:5, T3:3)</b> → DF = 0 (case 3), DT = 18.5 (case 3)</summary>

```text
--- [L194-195] i = 1, j = 0   cell (T1:5,T3:3) ---
[L196] computeDF(T1:5,T3:3)
|   T1:5 children: none   |   T3:3 children: none
|   [L241-247] CASE 1: delete T3:3, put T1:5's forest into one child of T3:3
|   |   [L241-242] minCost = MAX, case1Child = -1
|   |   [L243] T3:3 has no children: loop does not run, minCost stays MAX
|   |   [L247] case1 = DF(-,T3:3) + minCost = 0 + MAX = MAX
|   [L250-256] CASE 2: delete T1:5, put one child of T1:5 onto T3:3's forest
|   |   [L250-251] minCost = MAX, case2Child = -1
|   |   [L252] T1:5 has no children: loop does not run, minCost stays MAX
|   |   [L256] case2 = DF(T1:5,-) + minCost = 0 + MAX = MAX
|   [L259] CASE 3: case3 = MM(T1:5,T3:3)  (match the two child forests)
|   |   [L294-295] a = #children of T1:5 = 0, b = #children of T3:3 = 0
|   |   [L296] a == 0 && b == 0 -> return 0 (two empty forests)
|   [L259] case3 = 0
|   [L262] decide:
|   |   [L225] choose(case1 = MAX, case2 = MAX, case3 = 0)
|   |   [L227] case3 - 0.00001 <= case2 ? yes   case3 - 0.00001 <= case1 ? yes
|   |   [L228-229] -> CASE 3 wins, value 0, return CHOICE_MATCH (-1)
|   [L262-263] STORE DF(T1:5,T3:3) = 0   DFchoice = CHOICE_MATCH (case 3)
[L197] computeDT(T1:5,T3:3)
|   [L267-273] CASE 1: delete T3:3, put subtree T1:5 into one child subtree of T3:3
|   |   [L267-268] minCost = MAX, case1Child = -1
|   |   [L269] T3:3 has no children: loop does not run, minCost stays MAX
|   |   [L273] case1 = DT(-,T3:3) + minCost = 33.5 + MAX = MAX
|   [L275-281] CASE 2: delete T1:5, put one child subtree of T1:5 onto subtree T3:3
|   |   [L275-276] minCost = MAX, case2Child = -1
|   |   [L277] T1:5 has no children: loop does not run, minCost stays MAX
|   |   [L281] case2 = DT(T1:5,-) + minCost = 15 + MAX = MAX
|   [L283] CASE 3: match T1:5 <-> T3:3: case3 = DF(T1:5,T3:3) + relabelCost(T1:5,T3:3)
|   |   [L151] diff1 = |30 - 3| + |60 - 70| = 37   (move one node onto the other)
|   |   [L152] diff2 = |60 - 30| + |70 - 3| = 97   (delete both)
|   |   [L153] relabelCost = min(37, 97) / 2 = 18.5
|   [L283] case3 = 0 + 18.5 = 18.5
|   [L286] decide:
|   |   [L225] choose(case1 = MAX, case2 = MAX, case3 = 18.5)
|   |   [L227] case3 - 0.00001 <= case2 ? yes   case3 - 0.00001 <= case1 ? yes
|   |   [L228-229] -> CASE 3 wins, value 18.5, return CHOICE_MATCH (-1)
|   [L286-287] STORE DT(T1:5,T3:3) = 18.5   DTchoice = CHOICE_MATCH (case 3)
```

</details>

<details><summary><b>i=1, j=1: (T1:5, T3:5)</b> → DF = 0 (case 3), DT = 0 (case 3)</summary>

```text
--- [L194-195] i = 1, j = 1   cell (T1:5,T3:5) ---
[L196] computeDF(T1:5,T3:5)
|   T1:5 children: none   |   T3:5 children: none
|   [L241-247] CASE 1: delete T3:5, put T1:5's forest into one child of T3:5
|   |   [L241-242] minCost = MAX, case1Child = -1
|   |   [L243] T3:5 has no children: loop does not run, minCost stays MAX
|   |   [L247] case1 = DF(-,T3:5) + minCost = 0 + MAX = MAX
|   [L250-256] CASE 2: delete T1:5, put one child of T1:5 onto T3:5's forest
|   |   [L250-251] minCost = MAX, case2Child = -1
|   |   [L252] T1:5 has no children: loop does not run, minCost stays MAX
|   |   [L256] case2 = DF(T1:5,-) + minCost = 0 + MAX = MAX
|   [L259] CASE 3: case3 = MM(T1:5,T3:5)  (match the two child forests)
|   |   [L294-295] a = #children of T1:5 = 0, b = #children of T3:5 = 0
|   |   [L296] a == 0 && b == 0 -> return 0 (two empty forests)
|   [L259] case3 = 0
|   [L262] decide:
|   |   [L225] choose(case1 = MAX, case2 = MAX, case3 = 0)
|   |   [L227] case3 - 0.00001 <= case2 ? yes   case3 - 0.00001 <= case1 ? yes
|   |   [L228-229] -> CASE 3 wins, value 0, return CHOICE_MATCH (-1)
|   [L262-263] STORE DF(T1:5,T3:5) = 0   DFchoice = CHOICE_MATCH (case 3)
[L197] computeDT(T1:5,T3:5)
|   [L267-273] CASE 1: delete T3:5, put subtree T1:5 into one child subtree of T3:5
|   |   [L267-268] minCost = MAX, case1Child = -1
|   |   [L269] T3:5 has no children: loop does not run, minCost stays MAX
|   |   [L273] case1 = DT(-,T3:5) + minCost = 15 + MAX = MAX
|   [L275-281] CASE 2: delete T1:5, put one child subtree of T1:5 onto subtree T3:5
|   |   [L275-276] minCost = MAX, case2Child = -1
|   |   [L277] T1:5 has no children: loop does not run, minCost stays MAX
|   |   [L281] case2 = DT(T1:5,-) + minCost = 15 + MAX = MAX
|   [L283] CASE 3: match T1:5 <-> T3:5: case3 = DF(T1:5,T3:5) + relabelCost(T1:5,T3:5)
|   |   [L151] diff1 = |30 - 30| + |60 - 60| = 0   (move one node onto the other)
|   |   [L152] diff2 = |60 - 30| + |60 - 30| = 60   (delete both)
|   |   [L153] relabelCost = min(0, 60) / 2 = 0
|   [L283] case3 = 0 + 0 = 0
|   [L286] decide:
|   |   [L225] choose(case1 = MAX, case2 = MAX, case3 = 0)
|   |   [L227] case3 - 0.00001 <= case2 ? yes   case3 - 0.00001 <= case1 ? yes
|   |   [L228-229] -> CASE 3 wins, value 0, return CHOICE_MATCH (-1)
|   [L286-287] STORE DT(T1:5,T3:5) = 0   DTchoice = CHOICE_MATCH (case 3)
```

</details>

<details><summary><b>i=1, j=2: (T1:5, T3:8)</b> → DF = 0 (case 3), DT = 17.5 (case 3)</summary>

```text
--- [L194-195] i = 1, j = 2   cell (T1:5,T3:8) ---
[L196] computeDF(T1:5,T3:8)
|   T1:5 children: none   |   T3:8 children: none
|   [L241-247] CASE 1: delete T3:8, put T1:5's forest into one child of T3:8
|   |   [L241-242] minCost = MAX, case1Child = -1
|   |   [L243] T3:8 has no children: loop does not run, minCost stays MAX
|   |   [L247] case1 = DF(-,T3:8) + minCost = 0 + MAX = MAX
|   [L250-256] CASE 2: delete T1:5, put one child of T1:5 onto T3:8's forest
|   |   [L250-251] minCost = MAX, case2Child = -1
|   |   [L252] T1:5 has no children: loop does not run, minCost stays MAX
|   |   [L256] case2 = DF(T1:5,-) + minCost = 0 + MAX = MAX
|   [L259] CASE 3: case3 = MM(T1:5,T3:8)  (match the two child forests)
|   |   [L294-295] a = #children of T1:5 = 0, b = #children of T3:8 = 0
|   |   [L296] a == 0 && b == 0 -> return 0 (two empty forests)
|   [L259] case3 = 0
|   [L262] decide:
|   |   [L225] choose(case1 = MAX, case2 = MAX, case3 = 0)
|   |   [L227] case3 - 0.00001 <= case2 ? yes   case3 - 0.00001 <= case1 ? yes
|   |   [L228-229] -> CASE 3 wins, value 0, return CHOICE_MATCH (-1)
|   [L262-263] STORE DF(T1:5,T3:8) = 0   DFchoice = CHOICE_MATCH (case 3)
[L197] computeDT(T1:5,T3:8)
|   [L267-273] CASE 1: delete T3:8, put subtree T1:5 into one child subtree of T3:8
|   |   [L267-268] minCost = MAX, case1Child = -1
|   |   [L269] T3:8 has no children: loop does not run, minCost stays MAX
|   |   [L273] case1 = DT(-,T3:8) + minCost = 2.5 + MAX = MAX
|   [L275-281] CASE 2: delete T1:5, put one child subtree of T1:5 onto subtree T3:8
|   |   [L275-276] minCost = MAX, case2Child = -1
|   |   [L277] T1:5 has no children: loop does not run, minCost stays MAX
|   |   [L281] case2 = DT(T1:5,-) + minCost = 15 + MAX = MAX
|   [L283] CASE 3: match T1:5 <-> T3:8: case3 = DF(T1:5,T3:8) + relabelCost(T1:5,T3:8)
|   |   [L151] diff1 = |30 - 5| + |60 - 10| = 75   (move one node onto the other)
|   |   [L152] diff2 = |60 - 30| + |10 - 5| = 35   (delete both)
|   |   [L153] relabelCost = min(75, 35) / 2 = 17.5
|   [L283] case3 = 0 + 17.5 = 17.5
|   [L286] decide:
|   |   [L225] choose(case1 = MAX, case2 = MAX, case3 = 17.5)
|   |   [L227] case3 - 0.00001 <= case2 ? yes   case3 - 0.00001 <= case1 ? yes
|   |   [L228-229] -> CASE 3 wins, value 17.5, return CHOICE_MATCH (-1)
|   [L286-287] STORE DT(T1:5,T3:8) = 17.5   DTchoice = CHOICE_MATCH (case 3)
```

</details>

<details><summary><b>i=1, j=3: (T1:5, T3:9)</b> → DF = 0 (case 3), DT = 37.5 (case 3)</summary>

```text
--- [L194-195] i = 1, j = 3   cell (T1:5,T3:9) ---
[L196] computeDF(T1:5,T3:9)
|   T1:5 children: none   |   T3:9 children: none
|   [L241-247] CASE 1: delete T3:9, put T1:5's forest into one child of T3:9
|   |   [L241-242] minCost = MAX, case1Child = -1
|   |   [L243] T3:9 has no children: loop does not run, minCost stays MAX
|   |   [L247] case1 = DF(-,T3:9) + minCost = 0 + MAX = MAX
|   [L250-256] CASE 2: delete T1:5, put one child of T1:5 onto T3:9's forest
|   |   [L250-251] minCost = MAX, case2Child = -1
|   |   [L252] T1:5 has no children: loop does not run, minCost stays MAX
|   |   [L256] case2 = DF(T1:5,-) + minCost = 0 + MAX = MAX
|   [L259] CASE 3: case3 = MM(T1:5,T3:9)  (match the two child forests)
|   |   [L294-295] a = #children of T1:5 = 0, b = #children of T3:9 = 0
|   |   [L296] a == 0 && b == 0 -> return 0 (two empty forests)
|   [L259] case3 = 0
|   [L262] decide:
|   |   [L225] choose(case1 = MAX, case2 = MAX, case3 = 0)
|   |   [L227] case3 - 0.00001 <= case2 ? yes   case3 - 0.00001 <= case1 ? yes
|   |   [L228-229] -> CASE 3 wins, value 0, return CHOICE_MATCH (-1)
|   [L262-263] STORE DF(T1:5,T3:9) = 0   DFchoice = CHOICE_MATCH (case 3)
[L197] computeDT(T1:5,T3:9)
|   [L267-273] CASE 1: delete T3:9, put subtree T1:5 into one child subtree of T3:9
|   |   [L267-268] minCost = MAX, case1Child = -1
|   |   [L269] T3:9 has no children: loop does not run, minCost stays MAX
|   |   [L273] case1 = DT(-,T3:9) + minCost = 52.5 + MAX = MAX
|   [L275-281] CASE 2: delete T1:5, put one child subtree of T1:5 onto subtree T3:9
|   |   [L275-276] minCost = MAX, case2Child = -1
|   |   [L277] T1:5 has no children: loop does not run, minCost stays MAX
|   |   [L281] case2 = DT(T1:5,-) + minCost = 15 + MAX = MAX
|   [L283] CASE 3: match T1:5 <-> T3:9: case3 = DF(T1:5,T3:9) + relabelCost(T1:5,T3:9)
|   |   [L151] diff1 = |30 - 0| + |60 - 105| = 75   (move one node onto the other)
|   |   [L152] diff2 = |60 - 30| + |105 - 0| = 135   (delete both)
|   |   [L153] relabelCost = min(75, 135) / 2 = 37.5
|   [L283] case3 = 0 + 37.5 = 37.5
|   [L286] decide:
|   |   [L225] choose(case1 = MAX, case2 = MAX, case3 = 37.5)
|   |   [L227] case3 - 0.00001 <= case2 ? yes   case3 - 0.00001 <= case1 ? yes
|   |   [L228-229] -> CASE 3 wins, value 37.5, return CHOICE_MATCH (-1)
|   [L286-287] STORE DT(T1:5,T3:9) = 37.5   DTchoice = CHOICE_MATCH (case 3)
```

</details>

<details><summary><b>i=1, j=4: (T1:5, T3:6)</b> → DF = 55 (case 3), DT = 42.5 (case 1) · MM: <code>-<->T3:8 2.5 + -<->T3:9 52.5</code></summary>

```text
--- [L194-195] i = 1, j = 4   cell (T1:5,T3:6) ---
[L196] computeDF(T1:5,T3:6)
|   T1:5 children: none   |   T3:6 children: T3:8,T3:9
|   [L241-247] CASE 1: delete T3:6, put T1:5's forest into one child of T3:6
|   |   [L241-242] minCost = MAX, case1Child = -1
|   |   [L243-246] child T3:8: DF(T1:5,T3:8) - DF(-,T3:8) = 0 - 0 = 0  < min -> minCost = 0, case1Child = T3:8
|   |   [L243-246] child T3:9: DF(T1:5,T3:9) - DF(-,T3:9) = 0 - 0 = 0  not smaller
|   |   [L247] case1 = DF(-,T3:6) + minCost = 55 + 0 = 55
|   [L250-256] CASE 2: delete T1:5, put one child of T1:5 onto T3:6's forest
|   |   [L250-251] minCost = MAX, case2Child = -1
|   |   [L252] T1:5 has no children: loop does not run, minCost stays MAX
|   |   [L256] case2 = DF(T1:5,-) + minCost = 0 + MAX = MAX
|   [L259] CASE 3: case3 = MM(T1:5,T3:6)  (match the two child forests)
|   |   [L294-295] a = #children of T1:5 = 0, b = #children of T3:6 = 2
|   |   [L298] n = a + b = 2 (each side padded with dummies '-' up to n)
|   |   [L302-308] cost matrix, entry [k][l] = DT(row k, col l):
|   |                T3:8      T3:9      
|   |   -            2.5       52.5      
|   |   -            2.5       52.5      
|   |   [L309] kuhnMunkresMinWeightPerfectMatching(mmCost, 2):
|   |   |   [KM:L29-33] potentials u (rows) = 0, v (cols) = 0; p[col] = row assigned to col (0 = none). Rows/cols are 1-based here.
|   |   |   [KM:L35-39] add row r1(-): p[0] = 1, j0 = 0, minv = INF, used = none
|   |   |   |   [KM:L41-51] mark start used; its row i0 = r1(-). reduced costs cost-u-v for unused cols: c1:2.5-0-(0)=2.5->minv 2.5; c2:52.5-0-(0)=52.5->minv 52.5;
|   |   |   |              smallest minv: delta = 2.5 at c1(T3:8)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [2.5,0], v = [0,0]
|   |   |   |   [KM:L56-57] c1(T3:8) is free -> augmenting path found
|   |   |   |   [KM:L58-62] flip the path: c1(T3:8)<-r1(-);
|   |   |   [KM:L35-39] add row r2(-): p[0] = 2, j0 = 0, minv = INF, used = none
|   |   |   |   [KM:L41-51] mark start used; its row i0 = r2(-). reduced costs cost-u-v for unused cols: c1:2.5-0-(0)=2.5->minv 2.5; c2:52.5-0-(0)=52.5->minv 52.5;
|   |   |   |              smallest minv: delta = 2.5 at c1(T3:8)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [2.5,2.5], v = [0,0]
|   |   |   |   [KM:L56-57] c1(T3:8) is already taken by r1(-) -> keep searching from it
|   |   |   |   [KM:L41-51] mark c1(T3:8) used; its row i0 = r1(-). reduced costs cost-u-v for unused cols: c2:52.5-2.5-(0)=50->minv 50;
|   |   |   |              smallest minv: delta = 50 at c2(T3:9)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [52.5,52.5], v = [-50,0]
|   |   |   |   [KM:L56-57] c2(T3:9) is free -> augmenting path found
|   |   |   |   [KM:L58-62] flip the path: c2(T3:9)<-r2(-);
|   |   |   [KM:L65-70] KM RESULT: -<->T3:8 2.5 + -<->T3:9 52.5 = total 55
|   |   [L319] MM returns 55
|   [L259] case3 = 55
|   [L262] decide:
|   |   [L225] choose(case1 = 55, case2 = MAX, case3 = 55)
|   |   [L227] case3 - 0.00001 <= case2 ? yes   case3 - 0.00001 <= case1 ? yes
|   |   [L228-229] -> CASE 3 wins, value 55, return CHOICE_MATCH (-1)
|   [L262-263] STORE DF(T1:5,T3:6) = 55   DFchoice = CHOICE_MATCH (case 3)
[L197] computeDT(T1:5,T3:6)
|   [L267-273] CASE 1: delete T3:6, put subtree T1:5 into one child subtree of T3:6
|   |   [L267-268] minCost = MAX, case1Child = -1
|   |   [L269-272] child T3:8: DT(T1:5,T3:8) - DT(-,T3:8) = 17.5 - 2.5 = 15  < min -> minCost = 15, case1Child = T3:8
|   |   [L269-272] child T3:9: DT(T1:5,T3:9) - DT(-,T3:9) = 37.5 - 52.5 = -15  < min -> minCost = -15, case1Child = T3:9
|   |   [L273] case1 = DT(-,T3:6) + minCost = 57.5 + -15 = 42.5
|   [L275-281] CASE 2: delete T1:5, put one child subtree of T1:5 onto subtree T3:6
|   |   [L275-276] minCost = MAX, case2Child = -1
|   |   [L277] T1:5 has no children: loop does not run, minCost stays MAX
|   |   [L281] case2 = DT(T1:5,-) + minCost = 15 + MAX = MAX
|   [L283] CASE 3: match T1:5 <-> T3:6: case3 = DF(T1:5,T3:6) + relabelCost(T1:5,T3:6)
|   |   [L151] diff1 = |30 - 10| + |60 - 5| = 75   (move one node onto the other)
|   |   [L152] diff2 = |60 - 30| + |5 - 10| = 35   (delete both)
|   |   [L153] relabelCost = min(75, 35) / 2 = 17.5
|   [L283] case3 = 55 + 17.5 = 72.5
|   [L286] decide:
|   |   [L225] choose(case1 = 42.5, case2 = MAX, case3 = 72.5)
|   |   [L227] case3 - 0.00001 <= case2 ? yes   case3 - 0.00001 <= case1 ? no
|   |   [L230-232] case1 < case2 ? yes -> CASE 1 wins, value 42.5, return child index 3
|   [L286-287] STORE DT(T1:5,T3:6) = 42.5   DTchoice = 3 (case 1, T3 child T3:9)
```

</details>

<details><summary><b>i=1, j=5: (T1:5, T3:4)</b> → DF = 72.5 (case 3), DT = 72.5 (case 1) · MM: <code>-<->T3:5 15 + -<->T3:6 57.5</code></summary>

```text
--- [L194-195] i = 1, j = 5   cell (T1:5,T3:4) ---
[L196] computeDF(T1:5,T3:4)
|   T1:5 children: none   |   T3:4 children: T3:5,T3:6
|   [L241-247] CASE 1: delete T3:4, put T1:5's forest into one child of T3:4
|   |   [L241-242] minCost = MAX, case1Child = -1
|   |   [L243-246] child T3:5: DF(T1:5,T3:5) - DF(-,T3:5) = 0 - 0 = 0  < min -> minCost = 0, case1Child = T3:5
|   |   [L243-246] child T3:6: DF(T1:5,T3:6) - DF(-,T3:6) = 55 - 55 = 0  not smaller
|   |   [L247] case1 = DF(-,T3:4) + minCost = 72.5 + 0 = 72.5
|   [L250-256] CASE 2: delete T1:5, put one child of T1:5 onto T3:4's forest
|   |   [L250-251] minCost = MAX, case2Child = -1
|   |   [L252] T1:5 has no children: loop does not run, minCost stays MAX
|   |   [L256] case2 = DF(T1:5,-) + minCost = 0 + MAX = MAX
|   [L259] CASE 3: case3 = MM(T1:5,T3:4)  (match the two child forests)
|   |   [L294-295] a = #children of T1:5 = 0, b = #children of T3:4 = 2
|   |   [L298] n = a + b = 2 (each side padded with dummies '-' up to n)
|   |   [L302-308] cost matrix, entry [k][l] = DT(row k, col l):
|   |                T3:5      T3:6      
|   |   -            15        57.5      
|   |   -            15        57.5      
|   |   [L309] kuhnMunkresMinWeightPerfectMatching(mmCost, 2):
|   |   |   [KM:L29-33] potentials u (rows) = 0, v (cols) = 0; p[col] = row assigned to col (0 = none). Rows/cols are 1-based here.
|   |   |   [KM:L35-39] add row r1(-): p[0] = 1, j0 = 0, minv = INF, used = none
|   |   |   |   [KM:L41-51] mark start used; its row i0 = r1(-). reduced costs cost-u-v for unused cols: c1:15-0-(0)=15->minv 15; c2:57.5-0-(0)=57.5->minv 57.5;
|   |   |   |              smallest minv: delta = 15 at c1(T3:5)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [15,0], v = [0,0]
|   |   |   |   [KM:L56-57] c1(T3:5) is free -> augmenting path found
|   |   |   |   [KM:L58-62] flip the path: c1(T3:5)<-r1(-);
|   |   |   [KM:L35-39] add row r2(-): p[0] = 2, j0 = 0, minv = INF, used = none
|   |   |   |   [KM:L41-51] mark start used; its row i0 = r2(-). reduced costs cost-u-v for unused cols: c1:15-0-(0)=15->minv 15; c2:57.5-0-(0)=57.5->minv 57.5;
|   |   |   |              smallest minv: delta = 15 at c1(T3:5)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [15,15], v = [0,0]
|   |   |   |   [KM:L56-57] c1(T3:5) is already taken by r1(-) -> keep searching from it
|   |   |   |   [KM:L41-51] mark c1(T3:5) used; its row i0 = r1(-). reduced costs cost-u-v for unused cols: c2:57.5-15-(0)=42.5->minv 42.5;
|   |   |   |              smallest minv: delta = 42.5 at c2(T3:6)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [57.5,57.5], v = [-42.5,0]
|   |   |   |   [KM:L56-57] c2(T3:6) is free -> augmenting path found
|   |   |   |   [KM:L58-62] flip the path: c2(T3:6)<-r2(-);
|   |   |   [KM:L65-70] KM RESULT: -<->T3:5 15 + -<->T3:6 57.5 = total 72.5
|   |   [L319] MM returns 72.5
|   [L259] case3 = 72.5
|   [L262] decide:
|   |   [L225] choose(case1 = 72.5, case2 = MAX, case3 = 72.5)
|   |   [L227] case3 - 0.00001 <= case2 ? yes   case3 - 0.00001 <= case1 ? yes
|   |   [L228-229] -> CASE 3 wins, value 72.5, return CHOICE_MATCH (-1)
|   [L262-263] STORE DF(T1:5,T3:4) = 72.5   DFchoice = CHOICE_MATCH (case 3)
[L197] computeDT(T1:5,T3:4)
|   [L267-273] CASE 1: delete T3:4, put subtree T1:5 into one child subtree of T3:4
|   |   [L267-268] minCost = MAX, case1Child = -1
|   |   [L269-272] child T3:5: DT(T1:5,T3:5) - DT(-,T3:5) = 0 - 15 = -15  < min -> minCost = -15, case1Child = T3:5
|   |   [L269-272] child T3:6: DT(T1:5,T3:6) - DT(-,T3:6) = 42.5 - 57.5 = -15  not smaller
|   |   [L273] case1 = DT(-,T3:4) + minCost = 87.5 + -15 = 72.5
|   [L275-281] CASE 2: delete T1:5, put one child subtree of T1:5 onto subtree T3:4
|   |   [L275-276] minCost = MAX, case2Child = -1
|   |   [L277] T1:5 has no children: loop does not run, minCost stays MAX
|   |   [L281] case2 = DT(T1:5,-) + minCost = 15 + MAX = MAX
|   [L283] CASE 3: match T1:5 <-> T3:4: case3 = DF(T1:5,T3:4) + relabelCost(T1:5,T3:4)
|   |   [L151] diff1 = |30 - 60| + |60 - 30| = 60   (move one node onto the other)
|   |   [L152] diff2 = |60 - 30| + |30 - 60| = 60   (delete both)
|   |   [L153] relabelCost = min(60, 60) / 2 = 30
|   [L283] case3 = 72.5 + 30 = 102.5
|   [L286] decide:
|   |   [L225] choose(case1 = 72.5, case2 = MAX, case3 = 102.5)
|   |   [L227] case3 - 0.00001 <= case2 ? yes   case3 - 0.00001 <= case1 ? no
|   |   [L230-232] case1 < case2 ? yes -> CASE 1 wins, value 72.5, return child index 1
|   [L286-287] STORE DT(T1:5,T3:4) = 72.5   DTchoice = 1 (case 1, T3 child T3:5)
```

</details>

<details><summary><b>i=1, j=6: (T1:5, T3:2)</b> → DF = 121 (case 3), DT = 139.5 (case 1) · MM: <code>-<->T3:3 33.5 + -<->T3:4 87.5</code></summary>

```text
--- [L194-195] i = 1, j = 6   cell (T1:5,T3:2) ---
[L196] computeDF(T1:5,T3:2)
|   T1:5 children: none   |   T3:2 children: T3:3,T3:4
|   [L241-247] CASE 1: delete T3:2, put T1:5's forest into one child of T3:2
|   |   [L241-242] minCost = MAX, case1Child = -1
|   |   [L243-246] child T3:3: DF(T1:5,T3:3) - DF(-,T3:3) = 0 - 0 = 0  < min -> minCost = 0, case1Child = T3:3
|   |   [L243-246] child T3:4: DF(T1:5,T3:4) - DF(-,T3:4) = 72.5 - 72.5 = 0  not smaller
|   |   [L247] case1 = DF(-,T3:2) + minCost = 121 + 0 = 121
|   [L250-256] CASE 2: delete T1:5, put one child of T1:5 onto T3:2's forest
|   |   [L250-251] minCost = MAX, case2Child = -1
|   |   [L252] T1:5 has no children: loop does not run, minCost stays MAX
|   |   [L256] case2 = DF(T1:5,-) + minCost = 0 + MAX = MAX
|   [L259] CASE 3: case3 = MM(T1:5,T3:2)  (match the two child forests)
|   |   [L294-295] a = #children of T1:5 = 0, b = #children of T3:2 = 2
|   |   [L298] n = a + b = 2 (each side padded with dummies '-' up to n)
|   |   [L302-308] cost matrix, entry [k][l] = DT(row k, col l):
|   |                T3:3      T3:4      
|   |   -            33.5      87.5      
|   |   -            33.5      87.5      
|   |   [L309] kuhnMunkresMinWeightPerfectMatching(mmCost, 2):
|   |   |   [KM:L29-33] potentials u (rows) = 0, v (cols) = 0; p[col] = row assigned to col (0 = none). Rows/cols are 1-based here.
|   |   |   [KM:L35-39] add row r1(-): p[0] = 1, j0 = 0, minv = INF, used = none
|   |   |   |   [KM:L41-51] mark start used; its row i0 = r1(-). reduced costs cost-u-v for unused cols: c1:33.5-0-(0)=33.5->minv 33.5; c2:87.5-0-(0)=87.5->minv 87.5;
|   |   |   |              smallest minv: delta = 33.5 at c1(T3:3)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [33.5,0], v = [0,0]
|   |   |   |   [KM:L56-57] c1(T3:3) is free -> augmenting path found
|   |   |   |   [KM:L58-62] flip the path: c1(T3:3)<-r1(-);
|   |   |   [KM:L35-39] add row r2(-): p[0] = 2, j0 = 0, minv = INF, used = none
|   |   |   |   [KM:L41-51] mark start used; its row i0 = r2(-). reduced costs cost-u-v for unused cols: c1:33.5-0-(0)=33.5->minv 33.5; c2:87.5-0-(0)=87.5->minv 87.5;
|   |   |   |              smallest minv: delta = 33.5 at c1(T3:3)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [33.5,33.5], v = [0,0]
|   |   |   |   [KM:L56-57] c1(T3:3) is already taken by r1(-) -> keep searching from it
|   |   |   |   [KM:L41-51] mark c1(T3:3) used; its row i0 = r1(-). reduced costs cost-u-v for unused cols: c2:87.5-33.5-(0)=54->minv 54;
|   |   |   |              smallest minv: delta = 54 at c2(T3:4)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [87.5,87.5], v = [-54,0]
|   |   |   |   [KM:L56-57] c2(T3:4) is free -> augmenting path found
|   |   |   |   [KM:L58-62] flip the path: c2(T3:4)<-r2(-);
|   |   |   [KM:L65-70] KM RESULT: -<->T3:3 33.5 + -<->T3:4 87.5 = total 121
|   |   [L319] MM returns 121
|   [L259] case3 = 121
|   [L262] decide:
|   |   [L225] choose(case1 = 121, case2 = MAX, case3 = 121)
|   |   [L227] case3 - 0.00001 <= case2 ? yes   case3 - 0.00001 <= case1 ? yes
|   |   [L228-229] -> CASE 3 wins, value 121, return CHOICE_MATCH (-1)
|   [L262-263] STORE DF(T1:5,T3:2) = 121   DFchoice = CHOICE_MATCH (case 3)
[L197] computeDT(T1:5,T3:2)
|   [L267-273] CASE 1: delete T3:2, put subtree T1:5 into one child subtree of T3:2
|   |   [L267-268] minCost = MAX, case1Child = -1
|   |   [L269-272] child T3:3: DT(T1:5,T3:3) - DT(-,T3:3) = 18.5 - 33.5 = -15  < min -> minCost = -15, case1Child = T3:3
|   |   [L269-272] child T3:4: DT(T1:5,T3:4) - DT(-,T3:4) = 72.5 - 87.5 = -15  not smaller
|   |   [L273] case1 = DT(-,T3:2) + minCost = 154.5 + -15 = 139.5
|   [L275-281] CASE 2: delete T1:5, put one child subtree of T1:5 onto subtree T3:2
|   |   [L275-276] minCost = MAX, case2Child = -1
|   |   [L277] T1:5 has no children: loop does not run, minCost stays MAX
|   |   [L281] case2 = DT(T1:5,-) + minCost = 15 + MAX = MAX
|   [L283] CASE 3: match T1:5 <-> T3:2: case3 = DF(T1:5,T3:2) + relabelCost(T1:5,T3:2)
|   |   [L151] diff1 = |30 - 70| + |60 - 3| = 97   (move one node onto the other)
|   |   [L152] diff2 = |60 - 30| + |3 - 70| = 97   (delete both)
|   |   [L153] relabelCost = min(97, 97) / 2 = 48.5
|   [L283] case3 = 121 + 48.5 = 169.5
|   [L286] decide:
|   |   [L225] choose(case1 = 139.5, case2 = MAX, case3 = 169.5)
|   |   [L227] case3 - 0.00001 <= case2 ? yes   case3 - 0.00001 <= case1 ? no
|   |   [L230-232] case1 < case2 ? yes -> CASE 1 wins, value 139.5, return child index 0
|   [L286-287] STORE DT(T1:5,T3:2) = 139.5   DTchoice = 0 (case 1, T3 child T3:3)
```

</details>

<details><summary><b>i=1, j=7: (T1:5, T3:1)</b> → DF = 154.5 (case 3), DT = 192 (case 1) · MM: <code>-<->T3:2 154.5</code></summary>

```text
--- [L194-195] i = 1, j = 7   cell (T1:5,T3:1) ---
[L196] computeDF(T1:5,T3:1)
|   T1:5 children: none   |   T3:1 children: T3:2
|   [L241-247] CASE 1: delete T3:1, put T1:5's forest into one child of T3:1
|   |   [L241-242] minCost = MAX, case1Child = -1
|   |   [L243-246] child T3:2: DF(T1:5,T3:2) - DF(-,T3:2) = 121 - 121 = 0  < min -> minCost = 0, case1Child = T3:2
|   |   [L247] case1 = DF(-,T3:1) + minCost = 154.5 + 0 = 154.5
|   [L250-256] CASE 2: delete T1:5, put one child of T1:5 onto T3:1's forest
|   |   [L250-251] minCost = MAX, case2Child = -1
|   |   [L252] T1:5 has no children: loop does not run, minCost stays MAX
|   |   [L256] case2 = DF(T1:5,-) + minCost = 0 + MAX = MAX
|   [L259] CASE 3: case3 = MM(T1:5,T3:1)  (match the two child forests)
|   |   [L294-295] a = #children of T1:5 = 0, b = #children of T3:1 = 1
|   |   [L298] n = a + b = 1 (each side padded with dummies '-' up to n)
|   |   [L302-308] cost matrix, entry [k][l] = DT(row k, col l):
|   |                T3:2      
|   |   -            154.5     
|   |   [L309] kuhnMunkresMinWeightPerfectMatching(mmCost, 1):
|   |   |   [KM:L29-33] potentials u (rows) = 0, v (cols) = 0; p[col] = row assigned to col (0 = none). Rows/cols are 1-based here.
|   |   |   [KM:L35-39] add row r1(-): p[0] = 1, j0 = 0, minv = INF, used = none
|   |   |   |   [KM:L41-51] mark start used; its row i0 = r1(-). reduced costs cost-u-v for unused cols: c1:154.5-0-(0)=154.5->minv 154.5;
|   |   |   |              smallest minv: delta = 154.5 at c1(T3:2)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [154.5], v = [0]
|   |   |   |   [KM:L56-57] c1(T3:2) is free -> augmenting path found
|   |   |   |   [KM:L58-62] flip the path: c1(T3:2)<-r1(-);
|   |   |   [KM:L65-70] KM RESULT: -<->T3:2 154.5 = total 154.5
|   |   [L319] MM returns 154.5
|   [L259] case3 = 154.5
|   [L262] decide:
|   |   [L225] choose(case1 = 154.5, case2 = MAX, case3 = 154.5)
|   |   [L227] case3 - 0.00001 <= case2 ? yes   case3 - 0.00001 <= case1 ? yes
|   |   [L228-229] -> CASE 3 wins, value 154.5, return CHOICE_MATCH (-1)
|   [L262-263] STORE DF(T1:5,T3:1) = 154.5   DFchoice = CHOICE_MATCH (case 3)
[L197] computeDT(T1:5,T3:1)
|   [L267-273] CASE 1: delete T3:1, put subtree T1:5 into one child subtree of T3:1
|   |   [L267-268] minCost = MAX, case1Child = -1
|   |   [L269-272] child T3:2: DT(T1:5,T3:2) - DT(-,T3:2) = 139.5 - 154.5 = -15  < min -> minCost = -15, case1Child = T3:2
|   |   [L273] case1 = DT(-,T3:1) + minCost = 207 + -15 = 192
|   [L275-281] CASE 2: delete T1:5, put one child subtree of T1:5 onto subtree T3:1
|   |   [L275-276] minCost = MAX, case2Child = -1
|   |   [L277] T1:5 has no children: loop does not run, minCost stays MAX
|   |   [L281] case2 = DT(T1:5,-) + minCost = 15 + MAX = MAX
|   [L283] CASE 3: match T1:5 <-> T3:1: case3 = DF(T1:5,T3:1) + relabelCost(T1:5,T3:1)
|   |   [L151] diff1 = |30 - 105| + |60 - 0| = 135   (move one node onto the other)
|   |   [L152] diff2 = |60 - 30| + |0 - 105| = 135   (delete both)
|   |   [L153] relabelCost = min(135, 135) / 2 = 67.5
|   [L283] case3 = 154.5 + 67.5 = 222
|   [L286] decide:
|   |   [L225] choose(case1 = 192, case2 = MAX, case3 = 222)
|   |   [L227] case3 - 0.00001 <= case2 ? yes   case3 - 0.00001 <= case1 ? no
|   |   [L230-232] case1 < case2 ? yes -> CASE 1 wins, value 192, return child index 6
|   [L286-287] STORE DT(T1:5,T3:1) = 192   DTchoice = 6 (case 1, T3 child T3:2)
```

</details>

<details><summary><b>i=2, j=0: (T1:8, T3:3)</b> → DF = 0 (case 3), DT = 28.5 (case 3)</summary>

```text
--- [L194-195] i = 2, j = 0   cell (T1:8,T3:3) ---
[L196] computeDF(T1:8,T3:3)
|   T1:8 children: none   |   T3:3 children: none
|   [L241-247] CASE 1: delete T3:3, put T1:8's forest into one child of T3:3
|   |   [L241-242] minCost = MAX, case1Child = -1
|   |   [L243] T3:3 has no children: loop does not run, minCost stays MAX
|   |   [L247] case1 = DF(-,T3:3) + minCost = 0 + MAX = MAX
|   [L250-256] CASE 2: delete T1:8, put one child of T1:8 onto T3:3's forest
|   |   [L250-251] minCost = MAX, case2Child = -1
|   |   [L252] T1:8 has no children: loop does not run, minCost stays MAX
|   |   [L256] case2 = DF(T1:8,-) + minCost = 0 + MAX = MAX
|   [L259] CASE 3: case3 = MM(T1:8,T3:3)  (match the two child forests)
|   |   [L294-295] a = #children of T1:8 = 0, b = #children of T3:3 = 0
|   |   [L296] a == 0 && b == 0 -> return 0 (two empty forests)
|   [L259] case3 = 0
|   [L262] decide:
|   |   [L225] choose(case1 = MAX, case2 = MAX, case3 = 0)
|   |   [L227] case3 - 0.00001 <= case2 ? yes   case3 - 0.00001 <= case1 ? yes
|   |   [L228-229] -> CASE 3 wins, value 0, return CHOICE_MATCH (-1)
|   [L262-263] STORE DF(T1:8,T3:3) = 0   DFchoice = CHOICE_MATCH (case 3)
[L197] computeDT(T1:8,T3:3)
|   [L267-273] CASE 1: delete T3:3, put subtree T1:8 into one child subtree of T3:3
|   |   [L267-268] minCost = MAX, case1Child = -1
|   |   [L269] T3:3 has no children: loop does not run, minCost stays MAX
|   |   [L273] case1 = DT(-,T3:3) + minCost = 33.5 + MAX = MAX
|   [L275-281] CASE 2: delete T1:8, put one child subtree of T1:8 onto subtree T3:3
|   |   [L275-276] minCost = MAX, case2Child = -1
|   |   [L277] T1:8 has no children: loop does not run, minCost stays MAX
|   |   [L281] case2 = DT(T1:8,-) + minCost = 5 + MAX = MAX
|   [L283] CASE 3: match T1:8 <-> T3:3: case3 = DF(T1:8,T3:3) + relabelCost(T1:8,T3:3)
|   |   [L151] diff1 = |5 - 3| + |15 - 70| = 57   (move one node onto the other)
|   |   [L152] diff2 = |15 - 5| + |70 - 3| = 77   (delete both)
|   |   [L153] relabelCost = min(57, 77) / 2 = 28.5
|   [L283] case3 = 0 + 28.5 = 28.5
|   [L286] decide:
|   |   [L225] choose(case1 = MAX, case2 = MAX, case3 = 28.5)
|   |   [L227] case3 - 0.00001 <= case2 ? yes   case3 - 0.00001 <= case1 ? yes
|   |   [L228-229] -> CASE 3 wins, value 28.5, return CHOICE_MATCH (-1)
|   [L286-287] STORE DT(T1:8,T3:3) = 28.5   DTchoice = CHOICE_MATCH (case 3)
```

</details>

<details><summary><b>i=2, j=1: (T1:8, T3:5)</b> → DF = 0 (case 3), DT = 20 (case 3)</summary>

```text
--- [L194-195] i = 2, j = 1   cell (T1:8,T3:5) ---
[L196] computeDF(T1:8,T3:5)
|   T1:8 children: none   |   T3:5 children: none
|   [L241-247] CASE 1: delete T3:5, put T1:8's forest into one child of T3:5
|   |   [L241-242] minCost = MAX, case1Child = -1
|   |   [L243] T3:5 has no children: loop does not run, minCost stays MAX
|   |   [L247] case1 = DF(-,T3:5) + minCost = 0 + MAX = MAX
|   [L250-256] CASE 2: delete T1:8, put one child of T1:8 onto T3:5's forest
|   |   [L250-251] minCost = MAX, case2Child = -1
|   |   [L252] T1:8 has no children: loop does not run, minCost stays MAX
|   |   [L256] case2 = DF(T1:8,-) + minCost = 0 + MAX = MAX
|   [L259] CASE 3: case3 = MM(T1:8,T3:5)  (match the two child forests)
|   |   [L294-295] a = #children of T1:8 = 0, b = #children of T3:5 = 0
|   |   [L296] a == 0 && b == 0 -> return 0 (two empty forests)
|   [L259] case3 = 0
|   [L262] decide:
|   |   [L225] choose(case1 = MAX, case2 = MAX, case3 = 0)
|   |   [L227] case3 - 0.00001 <= case2 ? yes   case3 - 0.00001 <= case1 ? yes
|   |   [L228-229] -> CASE 3 wins, value 0, return CHOICE_MATCH (-1)
|   [L262-263] STORE DF(T1:8,T3:5) = 0   DFchoice = CHOICE_MATCH (case 3)
[L197] computeDT(T1:8,T3:5)
|   [L267-273] CASE 1: delete T3:5, put subtree T1:8 into one child subtree of T3:5
|   |   [L267-268] minCost = MAX, case1Child = -1
|   |   [L269] T3:5 has no children: loop does not run, minCost stays MAX
|   |   [L273] case1 = DT(-,T3:5) + minCost = 15 + MAX = MAX
|   [L275-281] CASE 2: delete T1:8, put one child subtree of T1:8 onto subtree T3:5
|   |   [L275-276] minCost = MAX, case2Child = -1
|   |   [L277] T1:8 has no children: loop does not run, minCost stays MAX
|   |   [L281] case2 = DT(T1:8,-) + minCost = 5 + MAX = MAX
|   [L283] CASE 3: match T1:8 <-> T3:5: case3 = DF(T1:8,T3:5) + relabelCost(T1:8,T3:5)
|   |   [L151] diff1 = |5 - 30| + |15 - 60| = 70   (move one node onto the other)
|   |   [L152] diff2 = |15 - 5| + |60 - 30| = 40   (delete both)
|   |   [L153] relabelCost = min(70, 40) / 2 = 20
|   [L283] case3 = 0 + 20 = 20
|   [L286] decide:
|   |   [L225] choose(case1 = MAX, case2 = MAX, case3 = 20)
|   |   [L227] case3 - 0.00001 <= case2 ? yes   case3 - 0.00001 <= case1 ? yes
|   |   [L228-229] -> CASE 3 wins, value 20, return CHOICE_MATCH (-1)
|   [L286-287] STORE DT(T1:8,T3:5) = 20   DTchoice = CHOICE_MATCH (case 3)
```

</details>

<details><summary><b>i=2, j=2: (T1:8, T3:8)</b> → DF = 0 (case 3), DT = 2.5 (case 3)</summary>

```text
--- [L194-195] i = 2, j = 2   cell (T1:8,T3:8) ---
[L196] computeDF(T1:8,T3:8)
|   T1:8 children: none   |   T3:8 children: none
|   [L241-247] CASE 1: delete T3:8, put T1:8's forest into one child of T3:8
|   |   [L241-242] minCost = MAX, case1Child = -1
|   |   [L243] T3:8 has no children: loop does not run, minCost stays MAX
|   |   [L247] case1 = DF(-,T3:8) + minCost = 0 + MAX = MAX
|   [L250-256] CASE 2: delete T1:8, put one child of T1:8 onto T3:8's forest
|   |   [L250-251] minCost = MAX, case2Child = -1
|   |   [L252] T1:8 has no children: loop does not run, minCost stays MAX
|   |   [L256] case2 = DF(T1:8,-) + minCost = 0 + MAX = MAX
|   [L259] CASE 3: case3 = MM(T1:8,T3:8)  (match the two child forests)
|   |   [L294-295] a = #children of T1:8 = 0, b = #children of T3:8 = 0
|   |   [L296] a == 0 && b == 0 -> return 0 (two empty forests)
|   [L259] case3 = 0
|   [L262] decide:
|   |   [L225] choose(case1 = MAX, case2 = MAX, case3 = 0)
|   |   [L227] case3 - 0.00001 <= case2 ? yes   case3 - 0.00001 <= case1 ? yes
|   |   [L228-229] -> CASE 3 wins, value 0, return CHOICE_MATCH (-1)
|   [L262-263] STORE DF(T1:8,T3:8) = 0   DFchoice = CHOICE_MATCH (case 3)
[L197] computeDT(T1:8,T3:8)
|   [L267-273] CASE 1: delete T3:8, put subtree T1:8 into one child subtree of T3:8
|   |   [L267-268] minCost = MAX, case1Child = -1
|   |   [L269] T3:8 has no children: loop does not run, minCost stays MAX
|   |   [L273] case1 = DT(-,T3:8) + minCost = 2.5 + MAX = MAX
|   [L275-281] CASE 2: delete T1:8, put one child subtree of T1:8 onto subtree T3:8
|   |   [L275-276] minCost = MAX, case2Child = -1
|   |   [L277] T1:8 has no children: loop does not run, minCost stays MAX
|   |   [L281] case2 = DT(T1:8,-) + minCost = 5 + MAX = MAX
|   [L283] CASE 3: match T1:8 <-> T3:8: case3 = DF(T1:8,T3:8) + relabelCost(T1:8,T3:8)
|   |   [L151] diff1 = |5 - 5| + |15 - 10| = 5   (move one node onto the other)
|   |   [L152] diff2 = |15 - 5| + |10 - 5| = 15   (delete both)
|   |   [L153] relabelCost = min(5, 15) / 2 = 2.5
|   [L283] case3 = 0 + 2.5 = 2.5
|   [L286] decide:
|   |   [L225] choose(case1 = MAX, case2 = MAX, case3 = 2.5)
|   |   [L227] case3 - 0.00001 <= case2 ? yes   case3 - 0.00001 <= case1 ? yes
|   |   [L228-229] -> CASE 3 wins, value 2.5, return CHOICE_MATCH (-1)
|   [L286-287] STORE DT(T1:8,T3:8) = 2.5   DTchoice = CHOICE_MATCH (case 3)
```

</details>

<details><summary><b>i=2, j=3: (T1:8, T3:9)</b> → DF = 0 (case 3), DT = 47.5 (case 3)</summary>

```text
--- [L194-195] i = 2, j = 3   cell (T1:8,T3:9) ---
[L196] computeDF(T1:8,T3:9)
|   T1:8 children: none   |   T3:9 children: none
|   [L241-247] CASE 1: delete T3:9, put T1:8's forest into one child of T3:9
|   |   [L241-242] minCost = MAX, case1Child = -1
|   |   [L243] T3:9 has no children: loop does not run, minCost stays MAX
|   |   [L247] case1 = DF(-,T3:9) + minCost = 0 + MAX = MAX
|   [L250-256] CASE 2: delete T1:8, put one child of T1:8 onto T3:9's forest
|   |   [L250-251] minCost = MAX, case2Child = -1
|   |   [L252] T1:8 has no children: loop does not run, minCost stays MAX
|   |   [L256] case2 = DF(T1:8,-) + minCost = 0 + MAX = MAX
|   [L259] CASE 3: case3 = MM(T1:8,T3:9)  (match the two child forests)
|   |   [L294-295] a = #children of T1:8 = 0, b = #children of T3:9 = 0
|   |   [L296] a == 0 && b == 0 -> return 0 (two empty forests)
|   [L259] case3 = 0
|   [L262] decide:
|   |   [L225] choose(case1 = MAX, case2 = MAX, case3 = 0)
|   |   [L227] case3 - 0.00001 <= case2 ? yes   case3 - 0.00001 <= case1 ? yes
|   |   [L228-229] -> CASE 3 wins, value 0, return CHOICE_MATCH (-1)
|   [L262-263] STORE DF(T1:8,T3:9) = 0   DFchoice = CHOICE_MATCH (case 3)
[L197] computeDT(T1:8,T3:9)
|   [L267-273] CASE 1: delete T3:9, put subtree T1:8 into one child subtree of T3:9
|   |   [L267-268] minCost = MAX, case1Child = -1
|   |   [L269] T3:9 has no children: loop does not run, minCost stays MAX
|   |   [L273] case1 = DT(-,T3:9) + minCost = 52.5 + MAX = MAX
|   [L275-281] CASE 2: delete T1:8, put one child subtree of T1:8 onto subtree T3:9
|   |   [L275-276] minCost = MAX, case2Child = -1
|   |   [L277] T1:8 has no children: loop does not run, minCost stays MAX
|   |   [L281] case2 = DT(T1:8,-) + minCost = 5 + MAX = MAX
|   [L283] CASE 3: match T1:8 <-> T3:9: case3 = DF(T1:8,T3:9) + relabelCost(T1:8,T3:9)
|   |   [L151] diff1 = |5 - 0| + |15 - 105| = 95   (move one node onto the other)
|   |   [L152] diff2 = |15 - 5| + |105 - 0| = 115   (delete both)
|   |   [L153] relabelCost = min(95, 115) / 2 = 47.5
|   [L283] case3 = 0 + 47.5 = 47.5
|   [L286] decide:
|   |   [L225] choose(case1 = MAX, case2 = MAX, case3 = 47.5)
|   |   [L227] case3 - 0.00001 <= case2 ? yes   case3 - 0.00001 <= case1 ? yes
|   |   [L228-229] -> CASE 3 wins, value 47.5, return CHOICE_MATCH (-1)
|   [L286-287] STORE DT(T1:8,T3:9) = 47.5   DTchoice = CHOICE_MATCH (case 3)
```

</details>

<details><summary><b>i=2, j=4: (T1:8, T3:6)</b> → DF = 55 (case 3), DT = 52.5 (case 1) · MM: <code>-<->T3:8 2.5 + -<->T3:9 52.5</code></summary>

```text
--- [L194-195] i = 2, j = 4   cell (T1:8,T3:6) ---
[L196] computeDF(T1:8,T3:6)
|   T1:8 children: none   |   T3:6 children: T3:8,T3:9
|   [L241-247] CASE 1: delete T3:6, put T1:8's forest into one child of T3:6
|   |   [L241-242] minCost = MAX, case1Child = -1
|   |   [L243-246] child T3:8: DF(T1:8,T3:8) - DF(-,T3:8) = 0 - 0 = 0  < min -> minCost = 0, case1Child = T3:8
|   |   [L243-246] child T3:9: DF(T1:8,T3:9) - DF(-,T3:9) = 0 - 0 = 0  not smaller
|   |   [L247] case1 = DF(-,T3:6) + minCost = 55 + 0 = 55
|   [L250-256] CASE 2: delete T1:8, put one child of T1:8 onto T3:6's forest
|   |   [L250-251] minCost = MAX, case2Child = -1
|   |   [L252] T1:8 has no children: loop does not run, minCost stays MAX
|   |   [L256] case2 = DF(T1:8,-) + minCost = 0 + MAX = MAX
|   [L259] CASE 3: case3 = MM(T1:8,T3:6)  (match the two child forests)
|   |   [L294-295] a = #children of T1:8 = 0, b = #children of T3:6 = 2
|   |   [L298] n = a + b = 2 (each side padded with dummies '-' up to n)
|   |   [L302-308] cost matrix, entry [k][l] = DT(row k, col l):
|   |                T3:8      T3:9      
|   |   -            2.5       52.5      
|   |   -            2.5       52.5      
|   |   [L309] kuhnMunkresMinWeightPerfectMatching(mmCost, 2):
|   |   |   [KM:L29-33] potentials u (rows) = 0, v (cols) = 0; p[col] = row assigned to col (0 = none). Rows/cols are 1-based here.
|   |   |   [KM:L35-39] add row r1(-): p[0] = 1, j0 = 0, minv = INF, used = none
|   |   |   |   [KM:L41-51] mark start used; its row i0 = r1(-). reduced costs cost-u-v for unused cols: c1:2.5-0-(0)=2.5->minv 2.5; c2:52.5-0-(0)=52.5->minv 52.5;
|   |   |   |              smallest minv: delta = 2.5 at c1(T3:8)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [2.5,0], v = [0,0]
|   |   |   |   [KM:L56-57] c1(T3:8) is free -> augmenting path found
|   |   |   |   [KM:L58-62] flip the path: c1(T3:8)<-r1(-);
|   |   |   [KM:L35-39] add row r2(-): p[0] = 2, j0 = 0, minv = INF, used = none
|   |   |   |   [KM:L41-51] mark start used; its row i0 = r2(-). reduced costs cost-u-v for unused cols: c1:2.5-0-(0)=2.5->minv 2.5; c2:52.5-0-(0)=52.5->minv 52.5;
|   |   |   |              smallest minv: delta = 2.5 at c1(T3:8)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [2.5,2.5], v = [0,0]
|   |   |   |   [KM:L56-57] c1(T3:8) is already taken by r1(-) -> keep searching from it
|   |   |   |   [KM:L41-51] mark c1(T3:8) used; its row i0 = r1(-). reduced costs cost-u-v for unused cols: c2:52.5-2.5-(0)=50->minv 50;
|   |   |   |              smallest minv: delta = 50 at c2(T3:9)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [52.5,52.5], v = [-50,0]
|   |   |   |   [KM:L56-57] c2(T3:9) is free -> augmenting path found
|   |   |   |   [KM:L58-62] flip the path: c2(T3:9)<-r2(-);
|   |   |   [KM:L65-70] KM RESULT: -<->T3:8 2.5 + -<->T3:9 52.5 = total 55
|   |   [L319] MM returns 55
|   [L259] case3 = 55
|   [L262] decide:
|   |   [L225] choose(case1 = 55, case2 = MAX, case3 = 55)
|   |   [L227] case3 - 0.00001 <= case2 ? yes   case3 - 0.00001 <= case1 ? yes
|   |   [L228-229] -> CASE 3 wins, value 55, return CHOICE_MATCH (-1)
|   [L262-263] STORE DF(T1:8,T3:6) = 55   DFchoice = CHOICE_MATCH (case 3)
[L197] computeDT(T1:8,T3:6)
|   [L267-273] CASE 1: delete T3:6, put subtree T1:8 into one child subtree of T3:6
|   |   [L267-268] minCost = MAX, case1Child = -1
|   |   [L269-272] child T3:8: DT(T1:8,T3:8) - DT(-,T3:8) = 2.5 - 2.5 = 0  < min -> minCost = 0, case1Child = T3:8
|   |   [L269-272] child T3:9: DT(T1:8,T3:9) - DT(-,T3:9) = 47.5 - 52.5 = -5  < min -> minCost = -5, case1Child = T3:9
|   |   [L273] case1 = DT(-,T3:6) + minCost = 57.5 + -5 = 52.5
|   [L275-281] CASE 2: delete T1:8, put one child subtree of T1:8 onto subtree T3:6
|   |   [L275-276] minCost = MAX, case2Child = -1
|   |   [L277] T1:8 has no children: loop does not run, minCost stays MAX
|   |   [L281] case2 = DT(T1:8,-) + minCost = 5 + MAX = MAX
|   [L283] CASE 3: match T1:8 <-> T3:6: case3 = DF(T1:8,T3:6) + relabelCost(T1:8,T3:6)
|   |   [L151] diff1 = |5 - 10| + |15 - 5| = 15   (move one node onto the other)
|   |   [L152] diff2 = |15 - 5| + |5 - 10| = 15   (delete both)
|   |   [L153] relabelCost = min(15, 15) / 2 = 7.5
|   [L283] case3 = 55 + 7.5 = 62.5
|   [L286] decide:
|   |   [L225] choose(case1 = 52.5, case2 = MAX, case3 = 62.5)
|   |   [L227] case3 - 0.00001 <= case2 ? yes   case3 - 0.00001 <= case1 ? no
|   |   [L230-232] case1 < case2 ? yes -> CASE 1 wins, value 52.5, return child index 3
|   [L286-287] STORE DT(T1:8,T3:6) = 52.5   DTchoice = 3 (case 1, T3 child T3:9)
```

</details>

<details><summary><b>i=2, j=5: (T1:8, T3:4)</b> → DF = 72.5 (case 3), DT = 82.5 (case 1) · MM: <code>-<->T3:5 15 + -<->T3:6 57.5</code></summary>

```text
--- [L194-195] i = 2, j = 5   cell (T1:8,T3:4) ---
[L196] computeDF(T1:8,T3:4)
|   T1:8 children: none   |   T3:4 children: T3:5,T3:6
|   [L241-247] CASE 1: delete T3:4, put T1:8's forest into one child of T3:4
|   |   [L241-242] minCost = MAX, case1Child = -1
|   |   [L243-246] child T3:5: DF(T1:8,T3:5) - DF(-,T3:5) = 0 - 0 = 0  < min -> minCost = 0, case1Child = T3:5
|   |   [L243-246] child T3:6: DF(T1:8,T3:6) - DF(-,T3:6) = 55 - 55 = 0  not smaller
|   |   [L247] case1 = DF(-,T3:4) + minCost = 72.5 + 0 = 72.5
|   [L250-256] CASE 2: delete T1:8, put one child of T1:8 onto T3:4's forest
|   |   [L250-251] minCost = MAX, case2Child = -1
|   |   [L252] T1:8 has no children: loop does not run, minCost stays MAX
|   |   [L256] case2 = DF(T1:8,-) + minCost = 0 + MAX = MAX
|   [L259] CASE 3: case3 = MM(T1:8,T3:4)  (match the two child forests)
|   |   [L294-295] a = #children of T1:8 = 0, b = #children of T3:4 = 2
|   |   [L298] n = a + b = 2 (each side padded with dummies '-' up to n)
|   |   [L302-308] cost matrix, entry [k][l] = DT(row k, col l):
|   |                T3:5      T3:6      
|   |   -            15        57.5      
|   |   -            15        57.5      
|   |   [L309] kuhnMunkresMinWeightPerfectMatching(mmCost, 2):
|   |   |   [KM:L29-33] potentials u (rows) = 0, v (cols) = 0; p[col] = row assigned to col (0 = none). Rows/cols are 1-based here.
|   |   |   [KM:L35-39] add row r1(-): p[0] = 1, j0 = 0, minv = INF, used = none
|   |   |   |   [KM:L41-51] mark start used; its row i0 = r1(-). reduced costs cost-u-v for unused cols: c1:15-0-(0)=15->minv 15; c2:57.5-0-(0)=57.5->minv 57.5;
|   |   |   |              smallest minv: delta = 15 at c1(T3:5)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [15,0], v = [0,0]
|   |   |   |   [KM:L56-57] c1(T3:5) is free -> augmenting path found
|   |   |   |   [KM:L58-62] flip the path: c1(T3:5)<-r1(-);
|   |   |   [KM:L35-39] add row r2(-): p[0] = 2, j0 = 0, minv = INF, used = none
|   |   |   |   [KM:L41-51] mark start used; its row i0 = r2(-). reduced costs cost-u-v for unused cols: c1:15-0-(0)=15->minv 15; c2:57.5-0-(0)=57.5->minv 57.5;
|   |   |   |              smallest minv: delta = 15 at c1(T3:5)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [15,15], v = [0,0]
|   |   |   |   [KM:L56-57] c1(T3:5) is already taken by r1(-) -> keep searching from it
|   |   |   |   [KM:L41-51] mark c1(T3:5) used; its row i0 = r1(-). reduced costs cost-u-v for unused cols: c2:57.5-15-(0)=42.5->minv 42.5;
|   |   |   |              smallest minv: delta = 42.5 at c2(T3:6)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [57.5,57.5], v = [-42.5,0]
|   |   |   |   [KM:L56-57] c2(T3:6) is free -> augmenting path found
|   |   |   |   [KM:L58-62] flip the path: c2(T3:6)<-r2(-);
|   |   |   [KM:L65-70] KM RESULT: -<->T3:5 15 + -<->T3:6 57.5 = total 72.5
|   |   [L319] MM returns 72.5
|   [L259] case3 = 72.5
|   [L262] decide:
|   |   [L225] choose(case1 = 72.5, case2 = MAX, case3 = 72.5)
|   |   [L227] case3 - 0.00001 <= case2 ? yes   case3 - 0.00001 <= case1 ? yes
|   |   [L228-229] -> CASE 3 wins, value 72.5, return CHOICE_MATCH (-1)
|   [L262-263] STORE DF(T1:8,T3:4) = 72.5   DFchoice = CHOICE_MATCH (case 3)
[L197] computeDT(T1:8,T3:4)
|   [L267-273] CASE 1: delete T3:4, put subtree T1:8 into one child subtree of T3:4
|   |   [L267-268] minCost = MAX, case1Child = -1
|   |   [L269-272] child T3:5: DT(T1:8,T3:5) - DT(-,T3:5) = 20 - 15 = 5  < min -> minCost = 5, case1Child = T3:5
|   |   [L269-272] child T3:6: DT(T1:8,T3:6) - DT(-,T3:6) = 52.5 - 57.5 = -5  < min -> minCost = -5, case1Child = T3:6
|   |   [L273] case1 = DT(-,T3:4) + minCost = 87.5 + -5 = 82.5
|   [L275-281] CASE 2: delete T1:8, put one child subtree of T1:8 onto subtree T3:4
|   |   [L275-276] minCost = MAX, case2Child = -1
|   |   [L277] T1:8 has no children: loop does not run, minCost stays MAX
|   |   [L281] case2 = DT(T1:8,-) + minCost = 5 + MAX = MAX
|   [L283] CASE 3: match T1:8 <-> T3:4: case3 = DF(T1:8,T3:4) + relabelCost(T1:8,T3:4)
|   |   [L151] diff1 = |5 - 60| + |15 - 30| = 70   (move one node onto the other)
|   |   [L152] diff2 = |15 - 5| + |30 - 60| = 40   (delete both)
|   |   [L153] relabelCost = min(70, 40) / 2 = 20
|   [L283] case3 = 72.5 + 20 = 92.5
|   [L286] decide:
|   |   [L225] choose(case1 = 82.5, case2 = MAX, case3 = 92.5)
|   |   [L227] case3 - 0.00001 <= case2 ? yes   case3 - 0.00001 <= case1 ? no
|   |   [L230-232] case1 < case2 ? yes -> CASE 1 wins, value 82.5, return child index 4
|   [L286-287] STORE DT(T1:8,T3:4) = 82.5   DTchoice = 4 (case 1, T3 child T3:6)
```

</details>

<details><summary><b>i=2, j=6: (T1:8, T3:2)</b> → DF = 121 (case 3), DT = 149.5 (case 1) · MM: <code>-<->T3:3 33.5 + -<->T3:4 87.5</code></summary>

```text
--- [L194-195] i = 2, j = 6   cell (T1:8,T3:2) ---
[L196] computeDF(T1:8,T3:2)
|   T1:8 children: none   |   T3:2 children: T3:3,T3:4
|   [L241-247] CASE 1: delete T3:2, put T1:8's forest into one child of T3:2
|   |   [L241-242] minCost = MAX, case1Child = -1
|   |   [L243-246] child T3:3: DF(T1:8,T3:3) - DF(-,T3:3) = 0 - 0 = 0  < min -> minCost = 0, case1Child = T3:3
|   |   [L243-246] child T3:4: DF(T1:8,T3:4) - DF(-,T3:4) = 72.5 - 72.5 = 0  not smaller
|   |   [L247] case1 = DF(-,T3:2) + minCost = 121 + 0 = 121
|   [L250-256] CASE 2: delete T1:8, put one child of T1:8 onto T3:2's forest
|   |   [L250-251] minCost = MAX, case2Child = -1
|   |   [L252] T1:8 has no children: loop does not run, minCost stays MAX
|   |   [L256] case2 = DF(T1:8,-) + minCost = 0 + MAX = MAX
|   [L259] CASE 3: case3 = MM(T1:8,T3:2)  (match the two child forests)
|   |   [L294-295] a = #children of T1:8 = 0, b = #children of T3:2 = 2
|   |   [L298] n = a + b = 2 (each side padded with dummies '-' up to n)
|   |   [L302-308] cost matrix, entry [k][l] = DT(row k, col l):
|   |                T3:3      T3:4      
|   |   -            33.5      87.5      
|   |   -            33.5      87.5      
|   |   [L309] kuhnMunkresMinWeightPerfectMatching(mmCost, 2):
|   |   |   [KM:L29-33] potentials u (rows) = 0, v (cols) = 0; p[col] = row assigned to col (0 = none). Rows/cols are 1-based here.
|   |   |   [KM:L35-39] add row r1(-): p[0] = 1, j0 = 0, minv = INF, used = none
|   |   |   |   [KM:L41-51] mark start used; its row i0 = r1(-). reduced costs cost-u-v for unused cols: c1:33.5-0-(0)=33.5->minv 33.5; c2:87.5-0-(0)=87.5->minv 87.5;
|   |   |   |              smallest minv: delta = 33.5 at c1(T3:3)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [33.5,0], v = [0,0]
|   |   |   |   [KM:L56-57] c1(T3:3) is free -> augmenting path found
|   |   |   |   [KM:L58-62] flip the path: c1(T3:3)<-r1(-);
|   |   |   [KM:L35-39] add row r2(-): p[0] = 2, j0 = 0, minv = INF, used = none
|   |   |   |   [KM:L41-51] mark start used; its row i0 = r2(-). reduced costs cost-u-v for unused cols: c1:33.5-0-(0)=33.5->minv 33.5; c2:87.5-0-(0)=87.5->minv 87.5;
|   |   |   |              smallest minv: delta = 33.5 at c1(T3:3)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [33.5,33.5], v = [0,0]
|   |   |   |   [KM:L56-57] c1(T3:3) is already taken by r1(-) -> keep searching from it
|   |   |   |   [KM:L41-51] mark c1(T3:3) used; its row i0 = r1(-). reduced costs cost-u-v for unused cols: c2:87.5-33.5-(0)=54->minv 54;
|   |   |   |              smallest minv: delta = 54 at c2(T3:4)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [87.5,87.5], v = [-54,0]
|   |   |   |   [KM:L56-57] c2(T3:4) is free -> augmenting path found
|   |   |   |   [KM:L58-62] flip the path: c2(T3:4)<-r2(-);
|   |   |   [KM:L65-70] KM RESULT: -<->T3:3 33.5 + -<->T3:4 87.5 = total 121
|   |   [L319] MM returns 121
|   [L259] case3 = 121
|   [L262] decide:
|   |   [L225] choose(case1 = 121, case2 = MAX, case3 = 121)
|   |   [L227] case3 - 0.00001 <= case2 ? yes   case3 - 0.00001 <= case1 ? yes
|   |   [L228-229] -> CASE 3 wins, value 121, return CHOICE_MATCH (-1)
|   [L262-263] STORE DF(T1:8,T3:2) = 121   DFchoice = CHOICE_MATCH (case 3)
[L197] computeDT(T1:8,T3:2)
|   [L267-273] CASE 1: delete T3:2, put subtree T1:8 into one child subtree of T3:2
|   |   [L267-268] minCost = MAX, case1Child = -1
|   |   [L269-272] child T3:3: DT(T1:8,T3:3) - DT(-,T3:3) = 28.5 - 33.5 = -5  < min -> minCost = -5, case1Child = T3:3
|   |   [L269-272] child T3:4: DT(T1:8,T3:4) - DT(-,T3:4) = 82.5 - 87.5 = -5  not smaller
|   |   [L273] case1 = DT(-,T3:2) + minCost = 154.5 + -5 = 149.5
|   [L275-281] CASE 2: delete T1:8, put one child subtree of T1:8 onto subtree T3:2
|   |   [L275-276] minCost = MAX, case2Child = -1
|   |   [L277] T1:8 has no children: loop does not run, minCost stays MAX
|   |   [L281] case2 = DT(T1:8,-) + minCost = 5 + MAX = MAX
|   [L283] CASE 3: match T1:8 <-> T3:2: case3 = DF(T1:8,T3:2) + relabelCost(T1:8,T3:2)
|   |   [L151] diff1 = |5 - 70| + |15 - 3| = 77   (move one node onto the other)
|   |   [L152] diff2 = |15 - 5| + |3 - 70| = 77   (delete both)
|   |   [L153] relabelCost = min(77, 77) / 2 = 38.5
|   [L283] case3 = 121 + 38.5 = 159.5
|   [L286] decide:
|   |   [L225] choose(case1 = 149.5, case2 = MAX, case3 = 159.5)
|   |   [L227] case3 - 0.00001 <= case2 ? yes   case3 - 0.00001 <= case1 ? no
|   |   [L230-232] case1 < case2 ? yes -> CASE 1 wins, value 149.5, return child index 0
|   [L286-287] STORE DT(T1:8,T3:2) = 149.5   DTchoice = 0 (case 1, T3 child T3:3)
```

</details>

<details><summary><b>i=2, j=7: (T1:8, T3:1)</b> → DF = 154.5 (case 3), DT = 202 (case 1) · MM: <code>-<->T3:2 154.5</code></summary>

```text
--- [L194-195] i = 2, j = 7   cell (T1:8,T3:1) ---
[L196] computeDF(T1:8,T3:1)
|   T1:8 children: none   |   T3:1 children: T3:2
|   [L241-247] CASE 1: delete T3:1, put T1:8's forest into one child of T3:1
|   |   [L241-242] minCost = MAX, case1Child = -1
|   |   [L243-246] child T3:2: DF(T1:8,T3:2) - DF(-,T3:2) = 121 - 121 = 0  < min -> minCost = 0, case1Child = T3:2
|   |   [L247] case1 = DF(-,T3:1) + minCost = 154.5 + 0 = 154.5
|   [L250-256] CASE 2: delete T1:8, put one child of T1:8 onto T3:1's forest
|   |   [L250-251] minCost = MAX, case2Child = -1
|   |   [L252] T1:8 has no children: loop does not run, minCost stays MAX
|   |   [L256] case2 = DF(T1:8,-) + minCost = 0 + MAX = MAX
|   [L259] CASE 3: case3 = MM(T1:8,T3:1)  (match the two child forests)
|   |   [L294-295] a = #children of T1:8 = 0, b = #children of T3:1 = 1
|   |   [L298] n = a + b = 1 (each side padded with dummies '-' up to n)
|   |   [L302-308] cost matrix, entry [k][l] = DT(row k, col l):
|   |                T3:2      
|   |   -            154.5     
|   |   [L309] kuhnMunkresMinWeightPerfectMatching(mmCost, 1):
|   |   |   [KM:L29-33] potentials u (rows) = 0, v (cols) = 0; p[col] = row assigned to col (0 = none). Rows/cols are 1-based here.
|   |   |   [KM:L35-39] add row r1(-): p[0] = 1, j0 = 0, minv = INF, used = none
|   |   |   |   [KM:L41-51] mark start used; its row i0 = r1(-). reduced costs cost-u-v for unused cols: c1:154.5-0-(0)=154.5->minv 154.5;
|   |   |   |              smallest minv: delta = 154.5 at c1(T3:2)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [154.5], v = [0]
|   |   |   |   [KM:L56-57] c1(T3:2) is free -> augmenting path found
|   |   |   |   [KM:L58-62] flip the path: c1(T3:2)<-r1(-);
|   |   |   [KM:L65-70] KM RESULT: -<->T3:2 154.5 = total 154.5
|   |   [L319] MM returns 154.5
|   [L259] case3 = 154.5
|   [L262] decide:
|   |   [L225] choose(case1 = 154.5, case2 = MAX, case3 = 154.5)
|   |   [L227] case3 - 0.00001 <= case2 ? yes   case3 - 0.00001 <= case1 ? yes
|   |   [L228-229] -> CASE 3 wins, value 154.5, return CHOICE_MATCH (-1)
|   [L262-263] STORE DF(T1:8,T3:1) = 154.5   DFchoice = CHOICE_MATCH (case 3)
[L197] computeDT(T1:8,T3:1)
|   [L267-273] CASE 1: delete T3:1, put subtree T1:8 into one child subtree of T3:1
|   |   [L267-268] minCost = MAX, case1Child = -1
|   |   [L269-272] child T3:2: DT(T1:8,T3:2) - DT(-,T3:2) = 149.5 - 154.5 = -5  < min -> minCost = -5, case1Child = T3:2
|   |   [L273] case1 = DT(-,T3:1) + minCost = 207 + -5 = 202
|   [L275-281] CASE 2: delete T1:8, put one child subtree of T1:8 onto subtree T3:1
|   |   [L275-276] minCost = MAX, case2Child = -1
|   |   [L277] T1:8 has no children: loop does not run, minCost stays MAX
|   |   [L281] case2 = DT(T1:8,-) + minCost = 5 + MAX = MAX
|   [L283] CASE 3: match T1:8 <-> T3:1: case3 = DF(T1:8,T3:1) + relabelCost(T1:8,T3:1)
|   |   [L151] diff1 = |5 - 105| + |15 - 0| = 115   (move one node onto the other)
|   |   [L152] diff2 = |15 - 5| + |0 - 105| = 115   (delete both)
|   |   [L153] relabelCost = min(115, 115) / 2 = 57.5
|   [L283] case3 = 154.5 + 57.5 = 212
|   [L286] decide:
|   |   [L225] choose(case1 = 202, case2 = MAX, case3 = 212)
|   |   [L227] case3 - 0.00001 <= case2 ? yes   case3 - 0.00001 <= case1 ? no
|   |   [L230-232] case1 < case2 ? yes -> CASE 1 wins, value 202, return child index 6
|   [L286-287] STORE DT(T1:8,T3:1) = 202   DTchoice = 6 (case 1, T3 child T3:2)
```

</details>

<details><summary><b>i=3, j=0: (T1:9, T3:3)</b> → DF = 0 (case 3), DT = 16.5 (case 3)</summary>

```text
--- [L194-195] i = 3, j = 0   cell (T1:9,T3:3) ---
[L196] computeDF(T1:9,T3:3)
|   T1:9 children: none   |   T3:3 children: none
|   [L241-247] CASE 1: delete T3:3, put T1:9's forest into one child of T3:3
|   |   [L241-242] minCost = MAX, case1Child = -1
|   |   [L243] T3:3 has no children: loop does not run, minCost stays MAX
|   |   [L247] case1 = DF(-,T3:3) + minCost = 0 + MAX = MAX
|   [L250-256] CASE 2: delete T1:9, put one child of T1:9 onto T3:3's forest
|   |   [L250-251] minCost = MAX, case2Child = -1
|   |   [L252] T1:9 has no children: loop does not run, minCost stays MAX
|   |   [L256] case2 = DF(T1:9,-) + minCost = 0 + MAX = MAX
|   [L259] CASE 3: case3 = MM(T1:9,T3:3)  (match the two child forests)
|   |   [L294-295] a = #children of T1:9 = 0, b = #children of T3:3 = 0
|   |   [L296] a == 0 && b == 0 -> return 0 (two empty forests)
|   [L259] case3 = 0
|   [L262] decide:
|   |   [L225] choose(case1 = MAX, case2 = MAX, case3 = 0)
|   |   [L227] case3 - 0.00001 <= case2 ? yes   case3 - 0.00001 <= case1 ? yes
|   |   [L228-229] -> CASE 3 wins, value 0, return CHOICE_MATCH (-1)
|   [L262-263] STORE DF(T1:9,T3:3) = 0   DFchoice = CHOICE_MATCH (case 3)
[L197] computeDT(T1:9,T3:3)
|   [L267-273] CASE 1: delete T3:3, put subtree T1:9 into one child subtree of T3:3
|   |   [L267-268] minCost = MAX, case1Child = -1
|   |   [L269] T3:3 has no children: loop does not run, minCost stays MAX
|   |   [L273] case1 = DT(-,T3:3) + minCost = 33.5 + MAX = MAX
|   [L275-281] CASE 2: delete T1:9, put one child subtree of T1:9 onto subtree T3:3
|   |   [L275-276] minCost = MAX, case2Child = -1
|   |   [L277] T1:9 has no children: loop does not run, minCost stays MAX
|   |   [L281] case2 = DT(T1:9,-) + minCost = 50 + MAX = MAX
|   [L283] CASE 3: match T1:9 <-> T3:3: case3 = DF(T1:9,T3:3) + relabelCost(T1:9,T3:3)
|   |   [L151] diff1 = |0 - 3| + |100 - 70| = 33   (move one node onto the other)
|   |   [L152] diff2 = |100 - 0| + |70 - 3| = 167   (delete both)
|   |   [L153] relabelCost = min(33, 167) / 2 = 16.5
|   [L283] case3 = 0 + 16.5 = 16.5
|   [L286] decide:
|   |   [L225] choose(case1 = MAX, case2 = MAX, case3 = 16.5)
|   |   [L227] case3 - 0.00001 <= case2 ? yes   case3 - 0.00001 <= case1 ? yes
|   |   [L228-229] -> CASE 3 wins, value 16.5, return CHOICE_MATCH (-1)
|   [L286-287] STORE DT(T1:9,T3:3) = 16.5   DTchoice = CHOICE_MATCH (case 3)
```

</details>

<details><summary><b>i=3, j=1: (T1:9, T3:5)</b> → DF = 0 (case 3), DT = 35 (case 3)</summary>

```text
--- [L194-195] i = 3, j = 1   cell (T1:9,T3:5) ---
[L196] computeDF(T1:9,T3:5)
|   T1:9 children: none   |   T3:5 children: none
|   [L241-247] CASE 1: delete T3:5, put T1:9's forest into one child of T3:5
|   |   [L241-242] minCost = MAX, case1Child = -1
|   |   [L243] T3:5 has no children: loop does not run, minCost stays MAX
|   |   [L247] case1 = DF(-,T3:5) + minCost = 0 + MAX = MAX
|   [L250-256] CASE 2: delete T1:9, put one child of T1:9 onto T3:5's forest
|   |   [L250-251] minCost = MAX, case2Child = -1
|   |   [L252] T1:9 has no children: loop does not run, minCost stays MAX
|   |   [L256] case2 = DF(T1:9,-) + minCost = 0 + MAX = MAX
|   [L259] CASE 3: case3 = MM(T1:9,T3:5)  (match the two child forests)
|   |   [L294-295] a = #children of T1:9 = 0, b = #children of T3:5 = 0
|   |   [L296] a == 0 && b == 0 -> return 0 (two empty forests)
|   [L259] case3 = 0
|   [L262] decide:
|   |   [L225] choose(case1 = MAX, case2 = MAX, case3 = 0)
|   |   [L227] case3 - 0.00001 <= case2 ? yes   case3 - 0.00001 <= case1 ? yes
|   |   [L228-229] -> CASE 3 wins, value 0, return CHOICE_MATCH (-1)
|   [L262-263] STORE DF(T1:9,T3:5) = 0   DFchoice = CHOICE_MATCH (case 3)
[L197] computeDT(T1:9,T3:5)
|   [L267-273] CASE 1: delete T3:5, put subtree T1:9 into one child subtree of T3:5
|   |   [L267-268] minCost = MAX, case1Child = -1
|   |   [L269] T3:5 has no children: loop does not run, minCost stays MAX
|   |   [L273] case1 = DT(-,T3:5) + minCost = 15 + MAX = MAX
|   [L275-281] CASE 2: delete T1:9, put one child subtree of T1:9 onto subtree T3:5
|   |   [L275-276] minCost = MAX, case2Child = -1
|   |   [L277] T1:9 has no children: loop does not run, minCost stays MAX
|   |   [L281] case2 = DT(T1:9,-) + minCost = 50 + MAX = MAX
|   [L283] CASE 3: match T1:9 <-> T3:5: case3 = DF(T1:9,T3:5) + relabelCost(T1:9,T3:5)
|   |   [L151] diff1 = |0 - 30| + |100 - 60| = 70   (move one node onto the other)
|   |   [L152] diff2 = |100 - 0| + |60 - 30| = 130   (delete both)
|   |   [L153] relabelCost = min(70, 130) / 2 = 35
|   [L283] case3 = 0 + 35 = 35
|   [L286] decide:
|   |   [L225] choose(case1 = MAX, case2 = MAX, case3 = 35)
|   |   [L227] case3 - 0.00001 <= case2 ? yes   case3 - 0.00001 <= case1 ? yes
|   |   [L228-229] -> CASE 3 wins, value 35, return CHOICE_MATCH (-1)
|   [L286-287] STORE DT(T1:9,T3:5) = 35   DTchoice = CHOICE_MATCH (case 3)
```

</details>

<details><summary><b>i=3, j=2: (T1:9, T3:8)</b> → DF = 0 (case 3), DT = 47.5 (case 3)</summary>

```text
--- [L194-195] i = 3, j = 2   cell (T1:9,T3:8) ---
[L196] computeDF(T1:9,T3:8)
|   T1:9 children: none   |   T3:8 children: none
|   [L241-247] CASE 1: delete T3:8, put T1:9's forest into one child of T3:8
|   |   [L241-242] minCost = MAX, case1Child = -1
|   |   [L243] T3:8 has no children: loop does not run, minCost stays MAX
|   |   [L247] case1 = DF(-,T3:8) + minCost = 0 + MAX = MAX
|   [L250-256] CASE 2: delete T1:9, put one child of T1:9 onto T3:8's forest
|   |   [L250-251] minCost = MAX, case2Child = -1
|   |   [L252] T1:9 has no children: loop does not run, minCost stays MAX
|   |   [L256] case2 = DF(T1:9,-) + minCost = 0 + MAX = MAX
|   [L259] CASE 3: case3 = MM(T1:9,T3:8)  (match the two child forests)
|   |   [L294-295] a = #children of T1:9 = 0, b = #children of T3:8 = 0
|   |   [L296] a == 0 && b == 0 -> return 0 (two empty forests)
|   [L259] case3 = 0
|   [L262] decide:
|   |   [L225] choose(case1 = MAX, case2 = MAX, case3 = 0)
|   |   [L227] case3 - 0.00001 <= case2 ? yes   case3 - 0.00001 <= case1 ? yes
|   |   [L228-229] -> CASE 3 wins, value 0, return CHOICE_MATCH (-1)
|   [L262-263] STORE DF(T1:9,T3:8) = 0   DFchoice = CHOICE_MATCH (case 3)
[L197] computeDT(T1:9,T3:8)
|   [L267-273] CASE 1: delete T3:8, put subtree T1:9 into one child subtree of T3:8
|   |   [L267-268] minCost = MAX, case1Child = -1
|   |   [L269] T3:8 has no children: loop does not run, minCost stays MAX
|   |   [L273] case1 = DT(-,T3:8) + minCost = 2.5 + MAX = MAX
|   [L275-281] CASE 2: delete T1:9, put one child subtree of T1:9 onto subtree T3:8
|   |   [L275-276] minCost = MAX, case2Child = -1
|   |   [L277] T1:9 has no children: loop does not run, minCost stays MAX
|   |   [L281] case2 = DT(T1:9,-) + minCost = 50 + MAX = MAX
|   [L283] CASE 3: match T1:9 <-> T3:8: case3 = DF(T1:9,T3:8) + relabelCost(T1:9,T3:8)
|   |   [L151] diff1 = |0 - 5| + |100 - 10| = 95   (move one node onto the other)
|   |   [L152] diff2 = |100 - 0| + |10 - 5| = 105   (delete both)
|   |   [L153] relabelCost = min(95, 105) / 2 = 47.5
|   [L283] case3 = 0 + 47.5 = 47.5
|   [L286] decide:
|   |   [L225] choose(case1 = MAX, case2 = MAX, case3 = 47.5)
|   |   [L227] case3 - 0.00001 <= case2 ? yes   case3 - 0.00001 <= case1 ? yes
|   |   [L228-229] -> CASE 3 wins, value 47.5, return CHOICE_MATCH (-1)
|   [L286-287] STORE DT(T1:9,T3:8) = 47.5   DTchoice = CHOICE_MATCH (case 3)
```

</details>

<details><summary><b>i=3, j=3: (T1:9, T3:9)</b> → DF = 0 (case 3), DT = 2.5 (case 3)</summary>

```text
--- [L194-195] i = 3, j = 3   cell (T1:9,T3:9) ---
[L196] computeDF(T1:9,T3:9)
|   T1:9 children: none   |   T3:9 children: none
|   [L241-247] CASE 1: delete T3:9, put T1:9's forest into one child of T3:9
|   |   [L241-242] minCost = MAX, case1Child = -1
|   |   [L243] T3:9 has no children: loop does not run, minCost stays MAX
|   |   [L247] case1 = DF(-,T3:9) + minCost = 0 + MAX = MAX
|   [L250-256] CASE 2: delete T1:9, put one child of T1:9 onto T3:9's forest
|   |   [L250-251] minCost = MAX, case2Child = -1
|   |   [L252] T1:9 has no children: loop does not run, minCost stays MAX
|   |   [L256] case2 = DF(T1:9,-) + minCost = 0 + MAX = MAX
|   [L259] CASE 3: case3 = MM(T1:9,T3:9)  (match the two child forests)
|   |   [L294-295] a = #children of T1:9 = 0, b = #children of T3:9 = 0
|   |   [L296] a == 0 && b == 0 -> return 0 (two empty forests)
|   [L259] case3 = 0
|   [L262] decide:
|   |   [L225] choose(case1 = MAX, case2 = MAX, case3 = 0)
|   |   [L227] case3 - 0.00001 <= case2 ? yes   case3 - 0.00001 <= case1 ? yes
|   |   [L228-229] -> CASE 3 wins, value 0, return CHOICE_MATCH (-1)
|   [L262-263] STORE DF(T1:9,T3:9) = 0   DFchoice = CHOICE_MATCH (case 3)
[L197] computeDT(T1:9,T3:9)
|   [L267-273] CASE 1: delete T3:9, put subtree T1:9 into one child subtree of T3:9
|   |   [L267-268] minCost = MAX, case1Child = -1
|   |   [L269] T3:9 has no children: loop does not run, minCost stays MAX
|   |   [L273] case1 = DT(-,T3:9) + minCost = 52.5 + MAX = MAX
|   [L275-281] CASE 2: delete T1:9, put one child subtree of T1:9 onto subtree T3:9
|   |   [L275-276] minCost = MAX, case2Child = -1
|   |   [L277] T1:9 has no children: loop does not run, minCost stays MAX
|   |   [L281] case2 = DT(T1:9,-) + minCost = 50 + MAX = MAX
|   [L283] CASE 3: match T1:9 <-> T3:9: case3 = DF(T1:9,T3:9) + relabelCost(T1:9,T3:9)
|   |   [L151] diff1 = |0 - 0| + |100 - 105| = 5   (move one node onto the other)
|   |   [L152] diff2 = |100 - 0| + |105 - 0| = 205   (delete both)
|   |   [L153] relabelCost = min(5, 205) / 2 = 2.5
|   [L283] case3 = 0 + 2.5 = 2.5
|   [L286] decide:
|   |   [L225] choose(case1 = MAX, case2 = MAX, case3 = 2.5)
|   |   [L227] case3 - 0.00001 <= case2 ? yes   case3 - 0.00001 <= case1 ? yes
|   |   [L228-229] -> CASE 3 wins, value 2.5, return CHOICE_MATCH (-1)
|   [L286-287] STORE DT(T1:9,T3:9) = 2.5   DTchoice = CHOICE_MATCH (case 3)
```

</details>

<details><summary><b>i=3, j=4: (T1:9, T3:6)</b> → DF = 55 (case 3), DT = 7.5 (case 1) · MM: <code>-<->T3:8 2.5 + -<->T3:9 52.5</code></summary>

```text
--- [L194-195] i = 3, j = 4   cell (T1:9,T3:6) ---
[L196] computeDF(T1:9,T3:6)
|   T1:9 children: none   |   T3:6 children: T3:8,T3:9
|   [L241-247] CASE 1: delete T3:6, put T1:9's forest into one child of T3:6
|   |   [L241-242] minCost = MAX, case1Child = -1
|   |   [L243-246] child T3:8: DF(T1:9,T3:8) - DF(-,T3:8) = 0 - 0 = 0  < min -> minCost = 0, case1Child = T3:8
|   |   [L243-246] child T3:9: DF(T1:9,T3:9) - DF(-,T3:9) = 0 - 0 = 0  not smaller
|   |   [L247] case1 = DF(-,T3:6) + minCost = 55 + 0 = 55
|   [L250-256] CASE 2: delete T1:9, put one child of T1:9 onto T3:6's forest
|   |   [L250-251] minCost = MAX, case2Child = -1
|   |   [L252] T1:9 has no children: loop does not run, minCost stays MAX
|   |   [L256] case2 = DF(T1:9,-) + minCost = 0 + MAX = MAX
|   [L259] CASE 3: case3 = MM(T1:9,T3:6)  (match the two child forests)
|   |   [L294-295] a = #children of T1:9 = 0, b = #children of T3:6 = 2
|   |   [L298] n = a + b = 2 (each side padded with dummies '-' up to n)
|   |   [L302-308] cost matrix, entry [k][l] = DT(row k, col l):
|   |                T3:8      T3:9      
|   |   -            2.5       52.5      
|   |   -            2.5       52.5      
|   |   [L309] kuhnMunkresMinWeightPerfectMatching(mmCost, 2):
|   |   |   [KM:L29-33] potentials u (rows) = 0, v (cols) = 0; p[col] = row assigned to col (0 = none). Rows/cols are 1-based here.
|   |   |   [KM:L35-39] add row r1(-): p[0] = 1, j0 = 0, minv = INF, used = none
|   |   |   |   [KM:L41-51] mark start used; its row i0 = r1(-). reduced costs cost-u-v for unused cols: c1:2.5-0-(0)=2.5->minv 2.5; c2:52.5-0-(0)=52.5->minv 52.5;
|   |   |   |              smallest minv: delta = 2.5 at c1(T3:8)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [2.5,0], v = [0,0]
|   |   |   |   [KM:L56-57] c1(T3:8) is free -> augmenting path found
|   |   |   |   [KM:L58-62] flip the path: c1(T3:8)<-r1(-);
|   |   |   [KM:L35-39] add row r2(-): p[0] = 2, j0 = 0, minv = INF, used = none
|   |   |   |   [KM:L41-51] mark start used; its row i0 = r2(-). reduced costs cost-u-v for unused cols: c1:2.5-0-(0)=2.5->minv 2.5; c2:52.5-0-(0)=52.5->minv 52.5;
|   |   |   |              smallest minv: delta = 2.5 at c1(T3:8)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [2.5,2.5], v = [0,0]
|   |   |   |   [KM:L56-57] c1(T3:8) is already taken by r1(-) -> keep searching from it
|   |   |   |   [KM:L41-51] mark c1(T3:8) used; its row i0 = r1(-). reduced costs cost-u-v for unused cols: c2:52.5-2.5-(0)=50->minv 50;
|   |   |   |              smallest minv: delta = 50 at c2(T3:9)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [52.5,52.5], v = [-50,0]
|   |   |   |   [KM:L56-57] c2(T3:9) is free -> augmenting path found
|   |   |   |   [KM:L58-62] flip the path: c2(T3:9)<-r2(-);
|   |   |   [KM:L65-70] KM RESULT: -<->T3:8 2.5 + -<->T3:9 52.5 = total 55
|   |   [L319] MM returns 55
|   [L259] case3 = 55
|   [L262] decide:
|   |   [L225] choose(case1 = 55, case2 = MAX, case3 = 55)
|   |   [L227] case3 - 0.00001 <= case2 ? yes   case3 - 0.00001 <= case1 ? yes
|   |   [L228-229] -> CASE 3 wins, value 55, return CHOICE_MATCH (-1)
|   [L262-263] STORE DF(T1:9,T3:6) = 55   DFchoice = CHOICE_MATCH (case 3)
[L197] computeDT(T1:9,T3:6)
|   [L267-273] CASE 1: delete T3:6, put subtree T1:9 into one child subtree of T3:6
|   |   [L267-268] minCost = MAX, case1Child = -1
|   |   [L269-272] child T3:8: DT(T1:9,T3:8) - DT(-,T3:8) = 47.5 - 2.5 = 45  < min -> minCost = 45, case1Child = T3:8
|   |   [L269-272] child T3:9: DT(T1:9,T3:9) - DT(-,T3:9) = 2.5 - 52.5 = -50  < min -> minCost = -50, case1Child = T3:9
|   |   [L273] case1 = DT(-,T3:6) + minCost = 57.5 + -50 = 7.5
|   [L275-281] CASE 2: delete T1:9, put one child subtree of T1:9 onto subtree T3:6
|   |   [L275-276] minCost = MAX, case2Child = -1
|   |   [L277] T1:9 has no children: loop does not run, minCost stays MAX
|   |   [L281] case2 = DT(T1:9,-) + minCost = 50 + MAX = MAX
|   [L283] CASE 3: match T1:9 <-> T3:6: case3 = DF(T1:9,T3:6) + relabelCost(T1:9,T3:6)
|   |   [L151] diff1 = |0 - 10| + |100 - 5| = 105   (move one node onto the other)
|   |   [L152] diff2 = |100 - 0| + |5 - 10| = 105   (delete both)
|   |   [L153] relabelCost = min(105, 105) / 2 = 52.5
|   [L283] case3 = 55 + 52.5 = 107.5
|   [L286] decide:
|   |   [L225] choose(case1 = 7.5, case2 = MAX, case3 = 107.5)
|   |   [L227] case3 - 0.00001 <= case2 ? yes   case3 - 0.00001 <= case1 ? no
|   |   [L230-232] case1 < case2 ? yes -> CASE 1 wins, value 7.5, return child index 3
|   [L286-287] STORE DT(T1:9,T3:6) = 7.5   DTchoice = 3 (case 1, T3 child T3:9)
```

</details>

<details><summary><b>i=3, j=5: (T1:9, T3:4)</b> → DF = 72.5 (case 3), DT = 37.5 (case 1) · MM: <code>-<->T3:5 15 + -<->T3:6 57.5</code></summary>

```text
--- [L194-195] i = 3, j = 5   cell (T1:9,T3:4) ---
[L196] computeDF(T1:9,T3:4)
|   T1:9 children: none   |   T3:4 children: T3:5,T3:6
|   [L241-247] CASE 1: delete T3:4, put T1:9's forest into one child of T3:4
|   |   [L241-242] minCost = MAX, case1Child = -1
|   |   [L243-246] child T3:5: DF(T1:9,T3:5) - DF(-,T3:5) = 0 - 0 = 0  < min -> minCost = 0, case1Child = T3:5
|   |   [L243-246] child T3:6: DF(T1:9,T3:6) - DF(-,T3:6) = 55 - 55 = 0  not smaller
|   |   [L247] case1 = DF(-,T3:4) + minCost = 72.5 + 0 = 72.5
|   [L250-256] CASE 2: delete T1:9, put one child of T1:9 onto T3:4's forest
|   |   [L250-251] minCost = MAX, case2Child = -1
|   |   [L252] T1:9 has no children: loop does not run, minCost stays MAX
|   |   [L256] case2 = DF(T1:9,-) + minCost = 0 + MAX = MAX
|   [L259] CASE 3: case3 = MM(T1:9,T3:4)  (match the two child forests)
|   |   [L294-295] a = #children of T1:9 = 0, b = #children of T3:4 = 2
|   |   [L298] n = a + b = 2 (each side padded with dummies '-' up to n)
|   |   [L302-308] cost matrix, entry [k][l] = DT(row k, col l):
|   |                T3:5      T3:6      
|   |   -            15        57.5      
|   |   -            15        57.5      
|   |   [L309] kuhnMunkresMinWeightPerfectMatching(mmCost, 2):
|   |   |   [KM:L29-33] potentials u (rows) = 0, v (cols) = 0; p[col] = row assigned to col (0 = none). Rows/cols are 1-based here.
|   |   |   [KM:L35-39] add row r1(-): p[0] = 1, j0 = 0, minv = INF, used = none
|   |   |   |   [KM:L41-51] mark start used; its row i0 = r1(-). reduced costs cost-u-v for unused cols: c1:15-0-(0)=15->minv 15; c2:57.5-0-(0)=57.5->minv 57.5;
|   |   |   |              smallest minv: delta = 15 at c1(T3:5)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [15,0], v = [0,0]
|   |   |   |   [KM:L56-57] c1(T3:5) is free -> augmenting path found
|   |   |   |   [KM:L58-62] flip the path: c1(T3:5)<-r1(-);
|   |   |   [KM:L35-39] add row r2(-): p[0] = 2, j0 = 0, minv = INF, used = none
|   |   |   |   [KM:L41-51] mark start used; its row i0 = r2(-). reduced costs cost-u-v for unused cols: c1:15-0-(0)=15->minv 15; c2:57.5-0-(0)=57.5->minv 57.5;
|   |   |   |              smallest minv: delta = 15 at c1(T3:5)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [15,15], v = [0,0]
|   |   |   |   [KM:L56-57] c1(T3:5) is already taken by r1(-) -> keep searching from it
|   |   |   |   [KM:L41-51] mark c1(T3:5) used; its row i0 = r1(-). reduced costs cost-u-v for unused cols: c2:57.5-15-(0)=42.5->minv 42.5;
|   |   |   |              smallest minv: delta = 42.5 at c2(T3:6)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [57.5,57.5], v = [-42.5,0]
|   |   |   |   [KM:L56-57] c2(T3:6) is free -> augmenting path found
|   |   |   |   [KM:L58-62] flip the path: c2(T3:6)<-r2(-);
|   |   |   [KM:L65-70] KM RESULT: -<->T3:5 15 + -<->T3:6 57.5 = total 72.5
|   |   [L319] MM returns 72.5
|   [L259] case3 = 72.5
|   [L262] decide:
|   |   [L225] choose(case1 = 72.5, case2 = MAX, case3 = 72.5)
|   |   [L227] case3 - 0.00001 <= case2 ? yes   case3 - 0.00001 <= case1 ? yes
|   |   [L228-229] -> CASE 3 wins, value 72.5, return CHOICE_MATCH (-1)
|   [L262-263] STORE DF(T1:9,T3:4) = 72.5   DFchoice = CHOICE_MATCH (case 3)
[L197] computeDT(T1:9,T3:4)
|   [L267-273] CASE 1: delete T3:4, put subtree T1:9 into one child subtree of T3:4
|   |   [L267-268] minCost = MAX, case1Child = -1
|   |   [L269-272] child T3:5: DT(T1:9,T3:5) - DT(-,T3:5) = 35 - 15 = 20  < min -> minCost = 20, case1Child = T3:5
|   |   [L269-272] child T3:6: DT(T1:9,T3:6) - DT(-,T3:6) = 7.5 - 57.5 = -50  < min -> minCost = -50, case1Child = T3:6
|   |   [L273] case1 = DT(-,T3:4) + minCost = 87.5 + -50 = 37.5
|   [L275-281] CASE 2: delete T1:9, put one child subtree of T1:9 onto subtree T3:4
|   |   [L275-276] minCost = MAX, case2Child = -1
|   |   [L277] T1:9 has no children: loop does not run, minCost stays MAX
|   |   [L281] case2 = DT(T1:9,-) + minCost = 50 + MAX = MAX
|   [L283] CASE 3: match T1:9 <-> T3:4: case3 = DF(T1:9,T3:4) + relabelCost(T1:9,T3:4)
|   |   [L151] diff1 = |0 - 60| + |100 - 30| = 130   (move one node onto the other)
|   |   [L152] diff2 = |100 - 0| + |30 - 60| = 130   (delete both)
|   |   [L153] relabelCost = min(130, 130) / 2 = 65
|   [L283] case3 = 72.5 + 65 = 137.5
|   [L286] decide:
|   |   [L225] choose(case1 = 37.5, case2 = MAX, case3 = 137.5)
|   |   [L227] case3 - 0.00001 <= case2 ? yes   case3 - 0.00001 <= case1 ? no
|   |   [L230-232] case1 < case2 ? yes -> CASE 1 wins, value 37.5, return child index 4
|   [L286-287] STORE DT(T1:9,T3:4) = 37.5   DTchoice = 4 (case 1, T3 child T3:6)
```

</details>

<details><summary><b>i=3, j=6: (T1:9, T3:2)</b> → DF = 121 (case 3), DT = 104.5 (case 1) · MM: <code>-<->T3:3 33.5 + -<->T3:4 87.5</code></summary>

```text
--- [L194-195] i = 3, j = 6   cell (T1:9,T3:2) ---
[L196] computeDF(T1:9,T3:2)
|   T1:9 children: none   |   T3:2 children: T3:3,T3:4
|   [L241-247] CASE 1: delete T3:2, put T1:9's forest into one child of T3:2
|   |   [L241-242] minCost = MAX, case1Child = -1
|   |   [L243-246] child T3:3: DF(T1:9,T3:3) - DF(-,T3:3) = 0 - 0 = 0  < min -> minCost = 0, case1Child = T3:3
|   |   [L243-246] child T3:4: DF(T1:9,T3:4) - DF(-,T3:4) = 72.5 - 72.5 = 0  not smaller
|   |   [L247] case1 = DF(-,T3:2) + minCost = 121 + 0 = 121
|   [L250-256] CASE 2: delete T1:9, put one child of T1:9 onto T3:2's forest
|   |   [L250-251] minCost = MAX, case2Child = -1
|   |   [L252] T1:9 has no children: loop does not run, minCost stays MAX
|   |   [L256] case2 = DF(T1:9,-) + minCost = 0 + MAX = MAX
|   [L259] CASE 3: case3 = MM(T1:9,T3:2)  (match the two child forests)
|   |   [L294-295] a = #children of T1:9 = 0, b = #children of T3:2 = 2
|   |   [L298] n = a + b = 2 (each side padded with dummies '-' up to n)
|   |   [L302-308] cost matrix, entry [k][l] = DT(row k, col l):
|   |                T3:3      T3:4      
|   |   -            33.5      87.5      
|   |   -            33.5      87.5      
|   |   [L309] kuhnMunkresMinWeightPerfectMatching(mmCost, 2):
|   |   |   [KM:L29-33] potentials u (rows) = 0, v (cols) = 0; p[col] = row assigned to col (0 = none). Rows/cols are 1-based here.
|   |   |   [KM:L35-39] add row r1(-): p[0] = 1, j0 = 0, minv = INF, used = none
|   |   |   |   [KM:L41-51] mark start used; its row i0 = r1(-). reduced costs cost-u-v for unused cols: c1:33.5-0-(0)=33.5->minv 33.5; c2:87.5-0-(0)=87.5->minv 87.5;
|   |   |   |              smallest minv: delta = 33.5 at c1(T3:3)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [33.5,0], v = [0,0]
|   |   |   |   [KM:L56-57] c1(T3:3) is free -> augmenting path found
|   |   |   |   [KM:L58-62] flip the path: c1(T3:3)<-r1(-);
|   |   |   [KM:L35-39] add row r2(-): p[0] = 2, j0 = 0, minv = INF, used = none
|   |   |   |   [KM:L41-51] mark start used; its row i0 = r2(-). reduced costs cost-u-v for unused cols: c1:33.5-0-(0)=33.5->minv 33.5; c2:87.5-0-(0)=87.5->minv 87.5;
|   |   |   |              smallest minv: delta = 33.5 at c1(T3:3)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [33.5,33.5], v = [0,0]
|   |   |   |   [KM:L56-57] c1(T3:3) is already taken by r1(-) -> keep searching from it
|   |   |   |   [KM:L41-51] mark c1(T3:3) used; its row i0 = r1(-). reduced costs cost-u-v for unused cols: c2:87.5-33.5-(0)=54->minv 54;
|   |   |   |              smallest minv: delta = 54 at c2(T3:4)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [87.5,87.5], v = [-54,0]
|   |   |   |   [KM:L56-57] c2(T3:4) is free -> augmenting path found
|   |   |   |   [KM:L58-62] flip the path: c2(T3:4)<-r2(-);
|   |   |   [KM:L65-70] KM RESULT: -<->T3:3 33.5 + -<->T3:4 87.5 = total 121
|   |   [L319] MM returns 121
|   [L259] case3 = 121
|   [L262] decide:
|   |   [L225] choose(case1 = 121, case2 = MAX, case3 = 121)
|   |   [L227] case3 - 0.00001 <= case2 ? yes   case3 - 0.00001 <= case1 ? yes
|   |   [L228-229] -> CASE 3 wins, value 121, return CHOICE_MATCH (-1)
|   [L262-263] STORE DF(T1:9,T3:2) = 121   DFchoice = CHOICE_MATCH (case 3)
[L197] computeDT(T1:9,T3:2)
|   [L267-273] CASE 1: delete T3:2, put subtree T1:9 into one child subtree of T3:2
|   |   [L267-268] minCost = MAX, case1Child = -1
|   |   [L269-272] child T3:3: DT(T1:9,T3:3) - DT(-,T3:3) = 16.5 - 33.5 = -17  < min -> minCost = -17, case1Child = T3:3
|   |   [L269-272] child T3:4: DT(T1:9,T3:4) - DT(-,T3:4) = 37.5 - 87.5 = -50  < min -> minCost = -50, case1Child = T3:4
|   |   [L273] case1 = DT(-,T3:2) + minCost = 154.5 + -50 = 104.5
|   [L275-281] CASE 2: delete T1:9, put one child subtree of T1:9 onto subtree T3:2
|   |   [L275-276] minCost = MAX, case2Child = -1
|   |   [L277] T1:9 has no children: loop does not run, minCost stays MAX
|   |   [L281] case2 = DT(T1:9,-) + minCost = 50 + MAX = MAX
|   [L283] CASE 3: match T1:9 <-> T3:2: case3 = DF(T1:9,T3:2) + relabelCost(T1:9,T3:2)
|   |   [L151] diff1 = |0 - 70| + |100 - 3| = 167   (move one node onto the other)
|   |   [L152] diff2 = |100 - 0| + |3 - 70| = 167   (delete both)
|   |   [L153] relabelCost = min(167, 167) / 2 = 83.5
|   [L283] case3 = 121 + 83.5 = 204.5
|   [L286] decide:
|   |   [L225] choose(case1 = 104.5, case2 = MAX, case3 = 204.5)
|   |   [L227] case3 - 0.00001 <= case2 ? yes   case3 - 0.00001 <= case1 ? no
|   |   [L230-232] case1 < case2 ? yes -> CASE 1 wins, value 104.5, return child index 5
|   [L286-287] STORE DT(T1:9,T3:2) = 104.5   DTchoice = 5 (case 1, T3 child T3:4)
```

</details>

<details><summary><b>i=3, j=7: (T1:9, T3:1)</b> → DF = 154.5 (case 3), DT = 157 (case 1) · MM: <code>-<->T3:2 154.5</code></summary>

```text
--- [L194-195] i = 3, j = 7   cell (T1:9,T3:1) ---
[L196] computeDF(T1:9,T3:1)
|   T1:9 children: none   |   T3:1 children: T3:2
|   [L241-247] CASE 1: delete T3:1, put T1:9's forest into one child of T3:1
|   |   [L241-242] minCost = MAX, case1Child = -1
|   |   [L243-246] child T3:2: DF(T1:9,T3:2) - DF(-,T3:2) = 121 - 121 = 0  < min -> minCost = 0, case1Child = T3:2
|   |   [L247] case1 = DF(-,T3:1) + minCost = 154.5 + 0 = 154.5
|   [L250-256] CASE 2: delete T1:9, put one child of T1:9 onto T3:1's forest
|   |   [L250-251] minCost = MAX, case2Child = -1
|   |   [L252] T1:9 has no children: loop does not run, minCost stays MAX
|   |   [L256] case2 = DF(T1:9,-) + minCost = 0 + MAX = MAX
|   [L259] CASE 3: case3 = MM(T1:9,T3:1)  (match the two child forests)
|   |   [L294-295] a = #children of T1:9 = 0, b = #children of T3:1 = 1
|   |   [L298] n = a + b = 1 (each side padded with dummies '-' up to n)
|   |   [L302-308] cost matrix, entry [k][l] = DT(row k, col l):
|   |                T3:2      
|   |   -            154.5     
|   |   [L309] kuhnMunkresMinWeightPerfectMatching(mmCost, 1):
|   |   |   [KM:L29-33] potentials u (rows) = 0, v (cols) = 0; p[col] = row assigned to col (0 = none). Rows/cols are 1-based here.
|   |   |   [KM:L35-39] add row r1(-): p[0] = 1, j0 = 0, minv = INF, used = none
|   |   |   |   [KM:L41-51] mark start used; its row i0 = r1(-). reduced costs cost-u-v for unused cols: c1:154.5-0-(0)=154.5->minv 154.5;
|   |   |   |              smallest minv: delta = 154.5 at c1(T3:2)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [154.5], v = [0]
|   |   |   |   [KM:L56-57] c1(T3:2) is free -> augmenting path found
|   |   |   |   [KM:L58-62] flip the path: c1(T3:2)<-r1(-);
|   |   |   [KM:L65-70] KM RESULT: -<->T3:2 154.5 = total 154.5
|   |   [L319] MM returns 154.5
|   [L259] case3 = 154.5
|   [L262] decide:
|   |   [L225] choose(case1 = 154.5, case2 = MAX, case3 = 154.5)
|   |   [L227] case3 - 0.00001 <= case2 ? yes   case3 - 0.00001 <= case1 ? yes
|   |   [L228-229] -> CASE 3 wins, value 154.5, return CHOICE_MATCH (-1)
|   [L262-263] STORE DF(T1:9,T3:1) = 154.5   DFchoice = CHOICE_MATCH (case 3)
[L197] computeDT(T1:9,T3:1)
|   [L267-273] CASE 1: delete T3:1, put subtree T1:9 into one child subtree of T3:1
|   |   [L267-268] minCost = MAX, case1Child = -1
|   |   [L269-272] child T3:2: DT(T1:9,T3:2) - DT(-,T3:2) = 104.5 - 154.5 = -50  < min -> minCost = -50, case1Child = T3:2
|   |   [L273] case1 = DT(-,T3:1) + minCost = 207 + -50 = 157
|   [L275-281] CASE 2: delete T1:9, put one child subtree of T1:9 onto subtree T3:1
|   |   [L275-276] minCost = MAX, case2Child = -1
|   |   [L277] T1:9 has no children: loop does not run, minCost stays MAX
|   |   [L281] case2 = DT(T1:9,-) + minCost = 50 + MAX = MAX
|   [L283] CASE 3: match T1:9 <-> T3:1: case3 = DF(T1:9,T3:1) + relabelCost(T1:9,T3:1)
|   |   [L151] diff1 = |0 - 105| + |100 - 0| = 205   (move one node onto the other)
|   |   [L152] diff2 = |100 - 0| + |0 - 105| = 205   (delete both)
|   |   [L153] relabelCost = min(205, 205) / 2 = 102.5
|   [L283] case3 = 154.5 + 102.5 = 257
|   [L286] decide:
|   |   [L225] choose(case1 = 157, case2 = MAX, case3 = 257)
|   |   [L227] case3 - 0.00001 <= case2 ? yes   case3 - 0.00001 <= case1 ? no
|   |   [L230-232] case1 < case2 ? yes -> CASE 1 wins, value 157, return child index 6
|   [L286-287] STORE DT(T1:9,T3:1) = 157   DTchoice = 6 (case 1, T3 child T3:2)
```

</details>

<details><summary><b>i=4, j=0: (T1:6, T3:3)</b> → DF = 55 (case 3), DT = 26.5 (case 2) · MM: <code>T1:8<->- 5 + T1:9<->- 50</code></summary>

```text
--- [L194-195] i = 4, j = 0   cell (T1:6,T3:3) ---
[L196] computeDF(T1:6,T3:3)
|   T1:6 children: T1:8,T1:9   |   T3:3 children: none
|   [L241-247] CASE 1: delete T3:3, put T1:6's forest into one child of T3:3
|   |   [L241-242] minCost = MAX, case1Child = -1
|   |   [L243] T3:3 has no children: loop does not run, minCost stays MAX
|   |   [L247] case1 = DF(-,T3:3) + minCost = 0 + MAX = MAX
|   [L250-256] CASE 2: delete T1:6, put one child of T1:6 onto T3:3's forest
|   |   [L250-251] minCost = MAX, case2Child = -1
|   |   [L252-255] child T1:8: DF(T1:8,T3:3) - DF(T1:8,-) = 0 - 0 = 0  < min -> minCost = 0, case2Child = T1:8
|   |   [L252-255] child T1:9: DF(T1:9,T3:3) - DF(T1:9,-) = 0 - 0 = 0  not smaller
|   |   [L256] case2 = DF(T1:6,-) + minCost = 55 + 0 = 55
|   [L259] CASE 3: case3 = MM(T1:6,T3:3)  (match the two child forests)
|   |   [L294-295] a = #children of T1:6 = 2, b = #children of T3:3 = 0
|   |   [L298] n = a + b = 2 (each side padded with dummies '-' up to n)
|   |   [L302-308] cost matrix, entry [k][l] = DT(row k, col l):
|   |                -         -         
|   |   T1:8         5         5         
|   |   T1:9         50        50        
|   |   [L309] kuhnMunkresMinWeightPerfectMatching(mmCost, 2):
|   |   |   [KM:L29-33] potentials u (rows) = 0, v (cols) = 0; p[col] = row assigned to col (0 = none). Rows/cols are 1-based here.
|   |   |   [KM:L35-39] add row r1(T1:8): p[0] = 1, j0 = 0, minv = INF, used = none
|   |   |   |   [KM:L41-51] mark start used; its row i0 = r1(T1:8). reduced costs cost-u-v for unused cols: c1:5-0-(0)=5->minv 5; c2:5-0-(0)=5->minv 5;
|   |   |   |              smallest minv: delta = 5 at c1(-)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [5,0], v = [0,0]
|   |   |   |   [KM:L56-57] c1(-) is free -> augmenting path found
|   |   |   |   [KM:L58-62] flip the path: c1(-)<-r1(T1:8);
|   |   |   [KM:L35-39] add row r2(T1:9): p[0] = 2, j0 = 0, minv = INF, used = none
|   |   |   |   [KM:L41-51] mark start used; its row i0 = r2(T1:9). reduced costs cost-u-v for unused cols: c1:50-0-(0)=50->minv 50; c2:50-0-(0)=50->minv 50;
|   |   |   |              smallest minv: delta = 50 at c1(-)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [5,50], v = [0,0]
|   |   |   |   [KM:L56-57] c1(-) is already taken by r1(T1:8) -> keep searching from it
|   |   |   |   [KM:L41-51] mark c1(-) used; its row i0 = r1(T1:8). reduced costs cost-u-v for unused cols: c2:5-5-(0)=0->minv 0;
|   |   |   |              smallest minv: delta = 0 at c2(-)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [5,50], v = [0,0]
|   |   |   |   [KM:L56-57] c2(-) is free -> augmenting path found
|   |   |   |   [KM:L58-62] flip the path: c2(-)<-r2(T1:9);
|   |   |   [KM:L65-70] KM RESULT: T1:8<->- 5 + T1:9<->- 50 = total 55
|   |   [L319] MM returns 55
|   [L259] case3 = 55
|   [L262] decide:
|   |   [L225] choose(case1 = MAX, case2 = 55, case3 = 55)
|   |   [L227] case3 - 0.00001 <= case2 ? yes   case3 - 0.00001 <= case1 ? yes
|   |   [L228-229] -> CASE 3 wins, value 55, return CHOICE_MATCH (-1)
|   [L262-263] STORE DF(T1:6,T3:3) = 55   DFchoice = CHOICE_MATCH (case 3)
[L197] computeDT(T1:6,T3:3)
|   [L267-273] CASE 1: delete T3:3, put subtree T1:6 into one child subtree of T3:3
|   |   [L267-268] minCost = MAX, case1Child = -1
|   |   [L269] T3:3 has no children: loop does not run, minCost stays MAX
|   |   [L273] case1 = DT(-,T3:3) + minCost = 33.5 + MAX = MAX
|   [L275-281] CASE 2: delete T1:6, put one child subtree of T1:6 onto subtree T3:3
|   |   [L275-276] minCost = MAX, case2Child = -1
|   |   [L277-280] child T1:8: DT(T1:8,T3:3) - DT(T1:8,-) = 28.5 - 5 = 23.5  < min -> minCost = 23.5, case2Child = T1:8
|   |   [L277-280] child T1:9: DT(T1:9,T3:3) - DT(T1:9,-) = 16.5 - 50 = -33.5  < min -> minCost = -33.5, case2Child = T1:9
|   |   [L281] case2 = DT(T1:6,-) + minCost = 60 + -33.5 = 26.5
|   [L283] CASE 3: match T1:6 <-> T3:3: case3 = DF(T1:6,T3:3) + relabelCost(T1:6,T3:3)
|   |   [L151] diff1 = |15 - 3| + |5 - 70| = 77   (move one node onto the other)
|   |   [L152] diff2 = |5 - 15| + |70 - 3| = 77   (delete both)
|   |   [L153] relabelCost = min(77, 77) / 2 = 38.5
|   [L283] case3 = 55 + 38.5 = 93.5
|   [L286] decide:
|   |   [L225] choose(case1 = MAX, case2 = 26.5, case3 = 93.5)
|   |   [L227] case3 - 0.00001 <= case2 ? no   case3 - 0.00001 <= case1 ? yes
|   |   [L230,233-235] case1 < case2 ? no -> CASE 2 wins, value 26.5, return -(3+2) = -5
|   [L286-287] STORE DT(T1:6,T3:3) = 26.5   DTchoice = -5 (case 2, T1 child T1:9)
```

</details>

<details><summary><b>i=4, j=1: (T1:6, T3:5)</b> → DF = 55 (case 3), DT = 45 (case 2) · MM: <code>T1:8<->- 5 + T1:9<->- 50</code></summary>

```text
--- [L194-195] i = 4, j = 1   cell (T1:6,T3:5) ---
[L196] computeDF(T1:6,T3:5)
|   T1:6 children: T1:8,T1:9   |   T3:5 children: none
|   [L241-247] CASE 1: delete T3:5, put T1:6's forest into one child of T3:5
|   |   [L241-242] minCost = MAX, case1Child = -1
|   |   [L243] T3:5 has no children: loop does not run, minCost stays MAX
|   |   [L247] case1 = DF(-,T3:5) + minCost = 0 + MAX = MAX
|   [L250-256] CASE 2: delete T1:6, put one child of T1:6 onto T3:5's forest
|   |   [L250-251] minCost = MAX, case2Child = -1
|   |   [L252-255] child T1:8: DF(T1:8,T3:5) - DF(T1:8,-) = 0 - 0 = 0  < min -> minCost = 0, case2Child = T1:8
|   |   [L252-255] child T1:9: DF(T1:9,T3:5) - DF(T1:9,-) = 0 - 0 = 0  not smaller
|   |   [L256] case2 = DF(T1:6,-) + minCost = 55 + 0 = 55
|   [L259] CASE 3: case3 = MM(T1:6,T3:5)  (match the two child forests)
|   |   [L294-295] a = #children of T1:6 = 2, b = #children of T3:5 = 0
|   |   [L298] n = a + b = 2 (each side padded with dummies '-' up to n)
|   |   [L302-308] cost matrix, entry [k][l] = DT(row k, col l):
|   |                -         -         
|   |   T1:8         5         5         
|   |   T1:9         50        50        
|   |   [L309] kuhnMunkresMinWeightPerfectMatching(mmCost, 2):
|   |   |   [KM:L29-33] potentials u (rows) = 0, v (cols) = 0; p[col] = row assigned to col (0 = none). Rows/cols are 1-based here.
|   |   |   [KM:L35-39] add row r1(T1:8): p[0] = 1, j0 = 0, minv = INF, used = none
|   |   |   |   [KM:L41-51] mark start used; its row i0 = r1(T1:8). reduced costs cost-u-v for unused cols: c1:5-0-(0)=5->minv 5; c2:5-0-(0)=5->minv 5;
|   |   |   |              smallest minv: delta = 5 at c1(-)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [5,0], v = [0,0]
|   |   |   |   [KM:L56-57] c1(-) is free -> augmenting path found
|   |   |   |   [KM:L58-62] flip the path: c1(-)<-r1(T1:8);
|   |   |   [KM:L35-39] add row r2(T1:9): p[0] = 2, j0 = 0, minv = INF, used = none
|   |   |   |   [KM:L41-51] mark start used; its row i0 = r2(T1:9). reduced costs cost-u-v for unused cols: c1:50-0-(0)=50->minv 50; c2:50-0-(0)=50->minv 50;
|   |   |   |              smallest minv: delta = 50 at c1(-)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [5,50], v = [0,0]
|   |   |   |   [KM:L56-57] c1(-) is already taken by r1(T1:8) -> keep searching from it
|   |   |   |   [KM:L41-51] mark c1(-) used; its row i0 = r1(T1:8). reduced costs cost-u-v for unused cols: c2:5-5-(0)=0->minv 0;
|   |   |   |              smallest minv: delta = 0 at c2(-)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [5,50], v = [0,0]
|   |   |   |   [KM:L56-57] c2(-) is free -> augmenting path found
|   |   |   |   [KM:L58-62] flip the path: c2(-)<-r2(T1:9);
|   |   |   [KM:L65-70] KM RESULT: T1:8<->- 5 + T1:9<->- 50 = total 55
|   |   [L319] MM returns 55
|   [L259] case3 = 55
|   [L262] decide:
|   |   [L225] choose(case1 = MAX, case2 = 55, case3 = 55)
|   |   [L227] case3 - 0.00001 <= case2 ? yes   case3 - 0.00001 <= case1 ? yes
|   |   [L228-229] -> CASE 3 wins, value 55, return CHOICE_MATCH (-1)
|   [L262-263] STORE DF(T1:6,T3:5) = 55   DFchoice = CHOICE_MATCH (case 3)
[L197] computeDT(T1:6,T3:5)
|   [L267-273] CASE 1: delete T3:5, put subtree T1:6 into one child subtree of T3:5
|   |   [L267-268] minCost = MAX, case1Child = -1
|   |   [L269] T3:5 has no children: loop does not run, minCost stays MAX
|   |   [L273] case1 = DT(-,T3:5) + minCost = 15 + MAX = MAX
|   [L275-281] CASE 2: delete T1:6, put one child subtree of T1:6 onto subtree T3:5
|   |   [L275-276] minCost = MAX, case2Child = -1
|   |   [L277-280] child T1:8: DT(T1:8,T3:5) - DT(T1:8,-) = 20 - 5 = 15  < min -> minCost = 15, case2Child = T1:8
|   |   [L277-280] child T1:9: DT(T1:9,T3:5) - DT(T1:9,-) = 35 - 50 = -15  < min -> minCost = -15, case2Child = T1:9
|   |   [L281] case2 = DT(T1:6,-) + minCost = 60 + -15 = 45
|   [L283] CASE 3: match T1:6 <-> T3:5: case3 = DF(T1:6,T3:5) + relabelCost(T1:6,T3:5)
|   |   [L151] diff1 = |15 - 30| + |5 - 60| = 70   (move one node onto the other)
|   |   [L152] diff2 = |5 - 15| + |60 - 30| = 40   (delete both)
|   |   [L153] relabelCost = min(70, 40) / 2 = 20
|   [L283] case3 = 55 + 20 = 75
|   [L286] decide:
|   |   [L225] choose(case1 = MAX, case2 = 45, case3 = 75)
|   |   [L227] case3 - 0.00001 <= case2 ? no   case3 - 0.00001 <= case1 ? yes
|   |   [L230,233-235] case1 < case2 ? no -> CASE 2 wins, value 45, return -(3+2) = -5
|   [L286-287] STORE DT(T1:6,T3:5) = 45   DTchoice = -5 (case 2, T1 child T1:9)
```

</details>

<details><summary><b>i=4, j=2: (T1:6, T3:8)</b> → DF = 55 (case 3), DT = 57.5 (case 2) · MM: <code>T1:8<->- 5 + T1:9<->- 50</code></summary>

```text
--- [L194-195] i = 4, j = 2   cell (T1:6,T3:8) ---
[L196] computeDF(T1:6,T3:8)
|   T1:6 children: T1:8,T1:9   |   T3:8 children: none
|   [L241-247] CASE 1: delete T3:8, put T1:6's forest into one child of T3:8
|   |   [L241-242] minCost = MAX, case1Child = -1
|   |   [L243] T3:8 has no children: loop does not run, minCost stays MAX
|   |   [L247] case1 = DF(-,T3:8) + minCost = 0 + MAX = MAX
|   [L250-256] CASE 2: delete T1:6, put one child of T1:6 onto T3:8's forest
|   |   [L250-251] minCost = MAX, case2Child = -1
|   |   [L252-255] child T1:8: DF(T1:8,T3:8) - DF(T1:8,-) = 0 - 0 = 0  < min -> minCost = 0, case2Child = T1:8
|   |   [L252-255] child T1:9: DF(T1:9,T3:8) - DF(T1:9,-) = 0 - 0 = 0  not smaller
|   |   [L256] case2 = DF(T1:6,-) + minCost = 55 + 0 = 55
|   [L259] CASE 3: case3 = MM(T1:6,T3:8)  (match the two child forests)
|   |   [L294-295] a = #children of T1:6 = 2, b = #children of T3:8 = 0
|   |   [L298] n = a + b = 2 (each side padded with dummies '-' up to n)
|   |   [L302-308] cost matrix, entry [k][l] = DT(row k, col l):
|   |                -         -         
|   |   T1:8         5         5         
|   |   T1:9         50        50        
|   |   [L309] kuhnMunkresMinWeightPerfectMatching(mmCost, 2):
|   |   |   [KM:L29-33] potentials u (rows) = 0, v (cols) = 0; p[col] = row assigned to col (0 = none). Rows/cols are 1-based here.
|   |   |   [KM:L35-39] add row r1(T1:8): p[0] = 1, j0 = 0, minv = INF, used = none
|   |   |   |   [KM:L41-51] mark start used; its row i0 = r1(T1:8). reduced costs cost-u-v for unused cols: c1:5-0-(0)=5->minv 5; c2:5-0-(0)=5->minv 5;
|   |   |   |              smallest minv: delta = 5 at c1(-)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [5,0], v = [0,0]
|   |   |   |   [KM:L56-57] c1(-) is free -> augmenting path found
|   |   |   |   [KM:L58-62] flip the path: c1(-)<-r1(T1:8);
|   |   |   [KM:L35-39] add row r2(T1:9): p[0] = 2, j0 = 0, minv = INF, used = none
|   |   |   |   [KM:L41-51] mark start used; its row i0 = r2(T1:9). reduced costs cost-u-v for unused cols: c1:50-0-(0)=50->minv 50; c2:50-0-(0)=50->minv 50;
|   |   |   |              smallest minv: delta = 50 at c1(-)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [5,50], v = [0,0]
|   |   |   |   [KM:L56-57] c1(-) is already taken by r1(T1:8) -> keep searching from it
|   |   |   |   [KM:L41-51] mark c1(-) used; its row i0 = r1(T1:8). reduced costs cost-u-v for unused cols: c2:5-5-(0)=0->minv 0;
|   |   |   |              smallest minv: delta = 0 at c2(-)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [5,50], v = [0,0]
|   |   |   |   [KM:L56-57] c2(-) is free -> augmenting path found
|   |   |   |   [KM:L58-62] flip the path: c2(-)<-r2(T1:9);
|   |   |   [KM:L65-70] KM RESULT: T1:8<->- 5 + T1:9<->- 50 = total 55
|   |   [L319] MM returns 55
|   [L259] case3 = 55
|   [L262] decide:
|   |   [L225] choose(case1 = MAX, case2 = 55, case3 = 55)
|   |   [L227] case3 - 0.00001 <= case2 ? yes   case3 - 0.00001 <= case1 ? yes
|   |   [L228-229] -> CASE 3 wins, value 55, return CHOICE_MATCH (-1)
|   [L262-263] STORE DF(T1:6,T3:8) = 55   DFchoice = CHOICE_MATCH (case 3)
[L197] computeDT(T1:6,T3:8)
|   [L267-273] CASE 1: delete T3:8, put subtree T1:6 into one child subtree of T3:8
|   |   [L267-268] minCost = MAX, case1Child = -1
|   |   [L269] T3:8 has no children: loop does not run, minCost stays MAX
|   |   [L273] case1 = DT(-,T3:8) + minCost = 2.5 + MAX = MAX
|   [L275-281] CASE 2: delete T1:6, put one child subtree of T1:6 onto subtree T3:8
|   |   [L275-276] minCost = MAX, case2Child = -1
|   |   [L277-280] child T1:8: DT(T1:8,T3:8) - DT(T1:8,-) = 2.5 - 5 = -2.5  < min -> minCost = -2.5, case2Child = T1:8
|   |   [L277-280] child T1:9: DT(T1:9,T3:8) - DT(T1:9,-) = 47.5 - 50 = -2.5  not smaller
|   |   [L281] case2 = DT(T1:6,-) + minCost = 60 + -2.5 = 57.5
|   [L283] CASE 3: match T1:6 <-> T3:8: case3 = DF(T1:6,T3:8) + relabelCost(T1:6,T3:8)
|   |   [L151] diff1 = |15 - 5| + |5 - 10| = 15   (move one node onto the other)
|   |   [L152] diff2 = |5 - 15| + |10 - 5| = 15   (delete both)
|   |   [L153] relabelCost = min(15, 15) / 2 = 7.5
|   [L283] case3 = 55 + 7.5 = 62.5
|   [L286] decide:
|   |   [L225] choose(case1 = MAX, case2 = 57.5, case3 = 62.5)
|   |   [L227] case3 - 0.00001 <= case2 ? no   case3 - 0.00001 <= case1 ? yes
|   |   [L230,233-235] case1 < case2 ? no -> CASE 2 wins, value 57.5, return -(2+2) = -4
|   [L286-287] STORE DT(T1:6,T3:8) = 57.5   DTchoice = -4 (case 2, T1 child T1:8)
```

</details>

<details><summary><b>i=4, j=3: (T1:6, T3:9)</b> → DF = 55 (case 3), DT = 12.5 (case 2) · MM: <code>T1:8<->- 5 + T1:9<->- 50</code></summary>

```text
--- [L194-195] i = 4, j = 3   cell (T1:6,T3:9) ---
[L196] computeDF(T1:6,T3:9)
|   T1:6 children: T1:8,T1:9   |   T3:9 children: none
|   [L241-247] CASE 1: delete T3:9, put T1:6's forest into one child of T3:9
|   |   [L241-242] minCost = MAX, case1Child = -1
|   |   [L243] T3:9 has no children: loop does not run, minCost stays MAX
|   |   [L247] case1 = DF(-,T3:9) + minCost = 0 + MAX = MAX
|   [L250-256] CASE 2: delete T1:6, put one child of T1:6 onto T3:9's forest
|   |   [L250-251] minCost = MAX, case2Child = -1
|   |   [L252-255] child T1:8: DF(T1:8,T3:9) - DF(T1:8,-) = 0 - 0 = 0  < min -> minCost = 0, case2Child = T1:8
|   |   [L252-255] child T1:9: DF(T1:9,T3:9) - DF(T1:9,-) = 0 - 0 = 0  not smaller
|   |   [L256] case2 = DF(T1:6,-) + minCost = 55 + 0 = 55
|   [L259] CASE 3: case3 = MM(T1:6,T3:9)  (match the two child forests)
|   |   [L294-295] a = #children of T1:6 = 2, b = #children of T3:9 = 0
|   |   [L298] n = a + b = 2 (each side padded with dummies '-' up to n)
|   |   [L302-308] cost matrix, entry [k][l] = DT(row k, col l):
|   |                -         -         
|   |   T1:8         5         5         
|   |   T1:9         50        50        
|   |   [L309] kuhnMunkresMinWeightPerfectMatching(mmCost, 2):
|   |   |   [KM:L29-33] potentials u (rows) = 0, v (cols) = 0; p[col] = row assigned to col (0 = none). Rows/cols are 1-based here.
|   |   |   [KM:L35-39] add row r1(T1:8): p[0] = 1, j0 = 0, minv = INF, used = none
|   |   |   |   [KM:L41-51] mark start used; its row i0 = r1(T1:8). reduced costs cost-u-v for unused cols: c1:5-0-(0)=5->minv 5; c2:5-0-(0)=5->minv 5;
|   |   |   |              smallest minv: delta = 5 at c1(-)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [5,0], v = [0,0]
|   |   |   |   [KM:L56-57] c1(-) is free -> augmenting path found
|   |   |   |   [KM:L58-62] flip the path: c1(-)<-r1(T1:8);
|   |   |   [KM:L35-39] add row r2(T1:9): p[0] = 2, j0 = 0, minv = INF, used = none
|   |   |   |   [KM:L41-51] mark start used; its row i0 = r2(T1:9). reduced costs cost-u-v for unused cols: c1:50-0-(0)=50->minv 50; c2:50-0-(0)=50->minv 50;
|   |   |   |              smallest minv: delta = 50 at c1(-)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [5,50], v = [0,0]
|   |   |   |   [KM:L56-57] c1(-) is already taken by r1(T1:8) -> keep searching from it
|   |   |   |   [KM:L41-51] mark c1(-) used; its row i0 = r1(T1:8). reduced costs cost-u-v for unused cols: c2:5-5-(0)=0->minv 0;
|   |   |   |              smallest minv: delta = 0 at c2(-)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [5,50], v = [0,0]
|   |   |   |   [KM:L56-57] c2(-) is free -> augmenting path found
|   |   |   |   [KM:L58-62] flip the path: c2(-)<-r2(T1:9);
|   |   |   [KM:L65-70] KM RESULT: T1:8<->- 5 + T1:9<->- 50 = total 55
|   |   [L319] MM returns 55
|   [L259] case3 = 55
|   [L262] decide:
|   |   [L225] choose(case1 = MAX, case2 = 55, case3 = 55)
|   |   [L227] case3 - 0.00001 <= case2 ? yes   case3 - 0.00001 <= case1 ? yes
|   |   [L228-229] -> CASE 3 wins, value 55, return CHOICE_MATCH (-1)
|   [L262-263] STORE DF(T1:6,T3:9) = 55   DFchoice = CHOICE_MATCH (case 3)
[L197] computeDT(T1:6,T3:9)
|   [L267-273] CASE 1: delete T3:9, put subtree T1:6 into one child subtree of T3:9
|   |   [L267-268] minCost = MAX, case1Child = -1
|   |   [L269] T3:9 has no children: loop does not run, minCost stays MAX
|   |   [L273] case1 = DT(-,T3:9) + minCost = 52.5 + MAX = MAX
|   [L275-281] CASE 2: delete T1:6, put one child subtree of T1:6 onto subtree T3:9
|   |   [L275-276] minCost = MAX, case2Child = -1
|   |   [L277-280] child T1:8: DT(T1:8,T3:9) - DT(T1:8,-) = 47.5 - 5 = 42.5  < min -> minCost = 42.5, case2Child = T1:8
|   |   [L277-280] child T1:9: DT(T1:9,T3:9) - DT(T1:9,-) = 2.5 - 50 = -47.5  < min -> minCost = -47.5, case2Child = T1:9
|   |   [L281] case2 = DT(T1:6,-) + minCost = 60 + -47.5 = 12.5
|   [L283] CASE 3: match T1:6 <-> T3:9: case3 = DF(T1:6,T3:9) + relabelCost(T1:6,T3:9)
|   |   [L151] diff1 = |15 - 0| + |5 - 105| = 115   (move one node onto the other)
|   |   [L152] diff2 = |5 - 15| + |105 - 0| = 115   (delete both)
|   |   [L153] relabelCost = min(115, 115) / 2 = 57.5
|   [L283] case3 = 55 + 57.5 = 112.5
|   [L286] decide:
|   |   [L225] choose(case1 = MAX, case2 = 12.5, case3 = 112.5)
|   |   [L227] case3 - 0.00001 <= case2 ? no   case3 - 0.00001 <= case1 ? yes
|   |   [L230,233-235] case1 < case2 ? no -> CASE 2 wins, value 12.5, return -(3+2) = -5
|   [L286-287] STORE DT(T1:6,T3:9) = 12.5   DTchoice = -5 (case 2, T1 child T1:9)
```

</details>

<details><summary><b>i=4, j=4: (T1:6, T3:6)</b> → DF = 5 (case 3), DT = 7.5 (case 3) · MM: <code>T1:8<->T3:8 2.5 + T1:9<->T3:9 2.5 + -<->- 0 + -<->- 0</code></summary>

```text
--- [L194-195] i = 4, j = 4   cell (T1:6,T3:6) ---
[L196] computeDF(T1:6,T3:6)
|   T1:6 children: T1:8,T1:9   |   T3:6 children: T3:8,T3:9
|   [L241-247] CASE 1: delete T3:6, put T1:6's forest into one child of T3:6
|   |   [L241-242] minCost = MAX, case1Child = -1
|   |   [L243-246] child T3:8: DF(T1:6,T3:8) - DF(-,T3:8) = 55 - 0 = 55  < min -> minCost = 55, case1Child = T3:8
|   |   [L243-246] child T3:9: DF(T1:6,T3:9) - DF(-,T3:9) = 55 - 0 = 55  not smaller
|   |   [L247] case1 = DF(-,T3:6) + minCost = 55 + 55 = 110
|   [L250-256] CASE 2: delete T1:6, put one child of T1:6 onto T3:6's forest
|   |   [L250-251] minCost = MAX, case2Child = -1
|   |   [L252-255] child T1:8: DF(T1:8,T3:6) - DF(T1:8,-) = 55 - 0 = 55  < min -> minCost = 55, case2Child = T1:8
|   |   [L252-255] child T1:9: DF(T1:9,T3:6) - DF(T1:9,-) = 55 - 0 = 55  not smaller
|   |   [L256] case2 = DF(T1:6,-) + minCost = 55 + 55 = 110
|   [L259] CASE 3: case3 = MM(T1:6,T3:6)  (match the two child forests)
|   |   [L294-295] a = #children of T1:6 = 2, b = #children of T3:6 = 2
|   |   [L298] n = a + b = 4 (each side padded with dummies '-' up to n)
|   |   [L302-308] cost matrix, entry [k][l] = DT(row k, col l):
|   |                T3:8      T3:9      -         -         
|   |   T1:8         2.5       47.5      5         5         
|   |   T1:9         47.5      2.5       50        50        
|   |   -            2.5       52.5      0         0         
|   |   -            2.5       52.5      0         0         
|   |   [L309] kuhnMunkresMinWeightPerfectMatching(mmCost, 4):
|   |   |   [KM:L29-33] potentials u (rows) = 0, v (cols) = 0; p[col] = row assigned to col (0 = none). Rows/cols are 1-based here.
|   |   |   [KM:L35-39] add row r1(T1:8): p[0] = 1, j0 = 0, minv = INF, used = none
|   |   |   |   [KM:L41-51] mark start used; its row i0 = r1(T1:8). reduced costs cost-u-v for unused cols: c1:2.5-0-(0)=2.5->minv 2.5; c2:47.5-0-(0)=47.5->minv 47.5; c3:5-0-(0)=5->minv 5; c4:5-0-(0)=5->minv 5;
|   |   |   |              smallest minv: delta = 2.5 at c1(T3:8)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [2.5,0,0,0], v = [0,0,0,0]
|   |   |   |   [KM:L56-57] c1(T3:8) is free -> augmenting path found
|   |   |   |   [KM:L58-62] flip the path: c1(T3:8)<-r1(T1:8);
|   |   |   [KM:L35-39] add row r2(T1:9): p[0] = 2, j0 = 0, minv = INF, used = none
|   |   |   |   [KM:L41-51] mark start used; its row i0 = r2(T1:9). reduced costs cost-u-v for unused cols: c1:47.5-0-(0)=47.5->minv 47.5; c2:2.5-0-(0)=2.5->minv 2.5; c3:50-0-(0)=50->minv 50; c4:50-0-(0)=50->minv 50;
|   |   |   |              smallest minv: delta = 2.5 at c2(T3:9)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [2.5,2.5,0,0], v = [0,0,0,0]
|   |   |   |   [KM:L56-57] c2(T3:9) is free -> augmenting path found
|   |   |   |   [KM:L58-62] flip the path: c2(T3:9)<-r2(T1:9);
|   |   |   [KM:L35-39] add row r3(-): p[0] = 3, j0 = 0, minv = INF, used = none
|   |   |   |   [KM:L41-51] mark start used; its row i0 = r3(-). reduced costs cost-u-v for unused cols: c1:2.5-0-(0)=2.5->minv 2.5; c2:52.5-0-(0)=52.5->minv 52.5; c3:0-0-(0)=0->minv 0; c4:0-0-(0)=0->minv 0;
|   |   |   |              smallest minv: delta = 0 at c3(-)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [2.5,2.5,0,0], v = [0,0,0,0]
|   |   |   |   [KM:L56-57] c3(-) is free -> augmenting path found
|   |   |   |   [KM:L58-62] flip the path: c3(-)<-r3(-);
|   |   |   [KM:L35-39] add row r4(-): p[0] = 4, j0 = 0, minv = INF, used = none
|   |   |   |   [KM:L41-51] mark start used; its row i0 = r4(-). reduced costs cost-u-v for unused cols: c1:2.5-0-(0)=2.5->minv 2.5; c2:52.5-0-(0)=52.5->minv 52.5; c3:0-0-(0)=0->minv 0; c4:0-0-(0)=0->minv 0;
|   |   |   |              smallest minv: delta = 0 at c3(-)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [2.5,2.5,0,0], v = [0,0,0,0]
|   |   |   |   [KM:L56-57] c3(-) is already taken by r3(-) -> keep searching from it
|   |   |   |   [KM:L41-51] mark c3(-) used; its row i0 = r3(-). reduced costs cost-u-v for unused cols: c1:2.5-0-(0)=2.5->minv 2.5; c2:52.5-0-(0)=52.5->minv 52.5; c4:0-0-(0)=0->minv 0;
|   |   |   |              smallest minv: delta = 0 at c4(-)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [2.5,2.5,0,0], v = [0,0,0,0]
|   |   |   |   [KM:L56-57] c4(-) is free -> augmenting path found
|   |   |   |   [KM:L58-62] flip the path: c4(-)<-r4(-);
|   |   |   [KM:L65-70] KM RESULT: T1:8<->T3:8 2.5 + T1:9<->T3:9 2.5 + -<->- 0 + -<->- 0 = total 5
|   |   [L319] MM returns 5
|   [L259] case3 = 5
|   [L262] decide:
|   |   [L225] choose(case1 = 110, case2 = 110, case3 = 5)
|   |   [L227] case3 - 0.00001 <= case2 ? yes   case3 - 0.00001 <= case1 ? yes
|   |   [L228-229] -> CASE 3 wins, value 5, return CHOICE_MATCH (-1)
|   [L262-263] STORE DF(T1:6,T3:6) = 5   DFchoice = CHOICE_MATCH (case 3)
[L197] computeDT(T1:6,T3:6)
|   [L267-273] CASE 1: delete T3:6, put subtree T1:6 into one child subtree of T3:6
|   |   [L267-268] minCost = MAX, case1Child = -1
|   |   [L269-272] child T3:8: DT(T1:6,T3:8) - DT(-,T3:8) = 57.5 - 2.5 = 55  < min -> minCost = 55, case1Child = T3:8
|   |   [L269-272] child T3:9: DT(T1:6,T3:9) - DT(-,T3:9) = 12.5 - 52.5 = -40  < min -> minCost = -40, case1Child = T3:9
|   |   [L273] case1 = DT(-,T3:6) + minCost = 57.5 + -40 = 17.5
|   [L275-281] CASE 2: delete T1:6, put one child subtree of T1:6 onto subtree T3:6
|   |   [L275-276] minCost = MAX, case2Child = -1
|   |   [L277-280] child T1:8: DT(T1:8,T3:6) - DT(T1:8,-) = 52.5 - 5 = 47.5  < min -> minCost = 47.5, case2Child = T1:8
|   |   [L277-280] child T1:9: DT(T1:9,T3:6) - DT(T1:9,-) = 7.5 - 50 = -42.5  < min -> minCost = -42.5, case2Child = T1:9
|   |   [L281] case2 = DT(T1:6,-) + minCost = 60 + -42.5 = 17.5
|   [L283] CASE 3: match T1:6 <-> T3:6: case3 = DF(T1:6,T3:6) + relabelCost(T1:6,T3:6)
|   |   [L151] diff1 = |15 - 10| + |5 - 5| = 5   (move one node onto the other)
|   |   [L152] diff2 = |5 - 15| + |5 - 10| = 15   (delete both)
|   |   [L153] relabelCost = min(5, 15) / 2 = 2.5
|   [L283] case3 = 5 + 2.5 = 7.5
|   [L286] decide:
|   |   [L225] choose(case1 = 17.5, case2 = 17.5, case3 = 7.5)
|   |   [L227] case3 - 0.00001 <= case2 ? yes   case3 - 0.00001 <= case1 ? yes
|   |   [L228-229] -> CASE 3 wins, value 7.5, return CHOICE_MATCH (-1)
|   [L286-287] STORE DT(T1:6,T3:6) = 7.5   DTchoice = CHOICE_MATCH (case 3)
```

</details>

<details><summary><b>i=4, j=5: (T1:6, T3:4)</b> → DF = 22.5 (case 1), DT = 37.5 (case 1) · MM: <code>T1:8<->- 5 + T1:9<->T3:6 7.5 + -<->- 0 + -<->T3:5 15</code></summary>

```text
--- [L194-195] i = 4, j = 5   cell (T1:6,T3:4) ---
[L196] computeDF(T1:6,T3:4)
|   T1:6 children: T1:8,T1:9   |   T3:4 children: T3:5,T3:6
|   [L241-247] CASE 1: delete T3:4, put T1:6's forest into one child of T3:4
|   |   [L241-242] minCost = MAX, case1Child = -1
|   |   [L243-246] child T3:5: DF(T1:6,T3:5) - DF(-,T3:5) = 55 - 0 = 55  < min -> minCost = 55, case1Child = T3:5
|   |   [L243-246] child T3:6: DF(T1:6,T3:6) - DF(-,T3:6) = 5 - 55 = -50  < min -> minCost = -50, case1Child = T3:6
|   |   [L247] case1 = DF(-,T3:4) + minCost = 72.5 + -50 = 22.5
|   [L250-256] CASE 2: delete T1:6, put one child of T1:6 onto T3:4's forest
|   |   [L250-251] minCost = MAX, case2Child = -1
|   |   [L252-255] child T1:8: DF(T1:8,T3:4) - DF(T1:8,-) = 72.5 - 0 = 72.5  < min -> minCost = 72.5, case2Child = T1:8
|   |   [L252-255] child T1:9: DF(T1:9,T3:4) - DF(T1:9,-) = 72.5 - 0 = 72.5  not smaller
|   |   [L256] case2 = DF(T1:6,-) + minCost = 55 + 72.5 = 127.5
|   [L259] CASE 3: case3 = MM(T1:6,T3:4)  (match the two child forests)
|   |   [L294-295] a = #children of T1:6 = 2, b = #children of T3:4 = 2
|   |   [L298] n = a + b = 4 (each side padded with dummies '-' up to n)
|   |   [L302-308] cost matrix, entry [k][l] = DT(row k, col l):
|   |                T3:5      T3:6      -         -         
|   |   T1:8         20        52.5      5         5         
|   |   T1:9         35        7.5       50        50        
|   |   -            15        57.5      0         0         
|   |   -            15        57.5      0         0         
|   |   [L309] kuhnMunkresMinWeightPerfectMatching(mmCost, 4):
|   |   |   [KM:L29-33] potentials u (rows) = 0, v (cols) = 0; p[col] = row assigned to col (0 = none). Rows/cols are 1-based here.
|   |   |   [KM:L35-39] add row r1(T1:8): p[0] = 1, j0 = 0, minv = INF, used = none
|   |   |   |   [KM:L41-51] mark start used; its row i0 = r1(T1:8). reduced costs cost-u-v for unused cols: c1:20-0-(0)=20->minv 20; c2:52.5-0-(0)=52.5->minv 52.5; c3:5-0-(0)=5->minv 5; c4:5-0-(0)=5->minv 5;
|   |   |   |              smallest minv: delta = 5 at c3(-)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [5,0,0,0], v = [0,0,0,0]
|   |   |   |   [KM:L56-57] c3(-) is free -> augmenting path found
|   |   |   |   [KM:L58-62] flip the path: c3(-)<-r1(T1:8);
|   |   |   [KM:L35-39] add row r2(T1:9): p[0] = 2, j0 = 0, minv = INF, used = none
|   |   |   |   [KM:L41-51] mark start used; its row i0 = r2(T1:9). reduced costs cost-u-v for unused cols: c1:35-0-(0)=35->minv 35; c2:7.5-0-(0)=7.5->minv 7.5; c3:50-0-(0)=50->minv 50; c4:50-0-(0)=50->minv 50;
|   |   |   |              smallest minv: delta = 7.5 at c2(T3:6)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [5,7.5,0,0], v = [0,0,0,0]
|   |   |   |   [KM:L56-57] c2(T3:6) is free -> augmenting path found
|   |   |   |   [KM:L58-62] flip the path: c2(T3:6)<-r2(T1:9);
|   |   |   [KM:L35-39] add row r3(-): p[0] = 3, j0 = 0, minv = INF, used = none
|   |   |   |   [KM:L41-51] mark start used; its row i0 = r3(-). reduced costs cost-u-v for unused cols: c1:15-0-(0)=15->minv 15; c2:57.5-0-(0)=57.5->minv 57.5; c3:0-0-(0)=0->minv 0; c4:0-0-(0)=0->minv 0;
|   |   |   |              smallest minv: delta = 0 at c3(-)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [5,7.5,0,0], v = [0,0,0,0]
|   |   |   |   [KM:L56-57] c3(-) is already taken by r1(T1:8) -> keep searching from it
|   |   |   |   [KM:L41-51] mark c3(-) used; its row i0 = r1(T1:8). reduced costs cost-u-v for unused cols: c1:20-5-(0)=15->minv 15; c2:52.5-5-(0)=47.5->minv 47.5; c4:5-5-(0)=0->minv 0;
|   |   |   |              smallest minv: delta = 0 at c4(-)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [5,7.5,0,0], v = [0,0,0,0]
|   |   |   |   [KM:L56-57] c4(-) is free -> augmenting path found
|   |   |   |   [KM:L58-62] flip the path: c4(-)<-r3(-);
|   |   |   [KM:L35-39] add row r4(-): p[0] = 4, j0 = 0, minv = INF, used = none
|   |   |   |   [KM:L41-51] mark start used; its row i0 = r4(-). reduced costs cost-u-v for unused cols: c1:15-0-(0)=15->minv 15; c2:57.5-0-(0)=57.5->minv 57.5; c3:0-0-(0)=0->minv 0; c4:0-0-(0)=0->minv 0;
|   |   |   |              smallest minv: delta = 0 at c3(-)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [5,7.5,0,0], v = [0,0,0,0]
|   |   |   |   [KM:L56-57] c3(-) is already taken by r1(T1:8) -> keep searching from it
|   |   |   |   [KM:L41-51] mark c3(-) used; its row i0 = r1(T1:8). reduced costs cost-u-v for unused cols: c1:20-5-(0)=15->minv 15; c2:52.5-5-(0)=47.5->minv 47.5; c4:5-5-(0)=0->minv 0;
|   |   |   |              smallest minv: delta = 0 at c4(-)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [5,7.5,0,0], v = [0,0,0,0]
|   |   |   |   [KM:L56-57] c4(-) is already taken by r3(-) -> keep searching from it
|   |   |   |   [KM:L41-51] mark c4(-) used; its row i0 = r3(-). reduced costs cost-u-v for unused cols: c1:15-0-(0)=15->minv 15; c2:57.5-0-(0)=57.5->minv 47.5;
|   |   |   |              smallest minv: delta = 15 at c1(T3:5)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [20,7.5,15,15], v = [0,0,-15,-15]
|   |   |   |   [KM:L56-57] c1(T3:5) is free -> augmenting path found
|   |   |   |   [KM:L58-62] flip the path: c1(T3:5)<-r4(-);
|   |   |   [KM:L65-70] KM RESULT: T1:8<->- 5 + T1:9<->T3:6 7.5 + -<->- 0 + -<->T3:5 15 = total 27.5
|   |   [L319] MM returns 27.5
|   [L259] case3 = 27.5
|   [L262] decide:
|   |   [L225] choose(case1 = 22.5, case2 = 127.5, case3 = 27.5)
|   |   [L227] case3 - 0.00001 <= case2 ? yes   case3 - 0.00001 <= case1 ? no
|   |   [L230-232] case1 < case2 ? yes -> CASE 1 wins, value 22.5, return child index 4
|   [L262-263] STORE DF(T1:6,T3:4) = 22.5   DFchoice = 4 (case 1, T3 child T3:6)
[L197] computeDT(T1:6,T3:4)
|   [L267-273] CASE 1: delete T3:4, put subtree T1:6 into one child subtree of T3:4
|   |   [L267-268] minCost = MAX, case1Child = -1
|   |   [L269-272] child T3:5: DT(T1:6,T3:5) - DT(-,T3:5) = 45 - 15 = 30  < min -> minCost = 30, case1Child = T3:5
|   |   [L269-272] child T3:6: DT(T1:6,T3:6) - DT(-,T3:6) = 7.5 - 57.5 = -50  < min -> minCost = -50, case1Child = T3:6
|   |   [L273] case1 = DT(-,T3:4) + minCost = 87.5 + -50 = 37.5
|   [L275-281] CASE 2: delete T1:6, put one child subtree of T1:6 onto subtree T3:4
|   |   [L275-276] minCost = MAX, case2Child = -1
|   |   [L277-280] child T1:8: DT(T1:8,T3:4) - DT(T1:8,-) = 82.5 - 5 = 77.5  < min -> minCost = 77.5, case2Child = T1:8
|   |   [L277-280] child T1:9: DT(T1:9,T3:4) - DT(T1:9,-) = 37.5 - 50 = -12.5  < min -> minCost = -12.5, case2Child = T1:9
|   |   [L281] case2 = DT(T1:6,-) + minCost = 60 + -12.5 = 47.5
|   [L283] CASE 3: match T1:6 <-> T3:4: case3 = DF(T1:6,T3:4) + relabelCost(T1:6,T3:4)
|   |   [L151] diff1 = |15 - 60| + |5 - 30| = 70   (move one node onto the other)
|   |   [L152] diff2 = |5 - 15| + |30 - 60| = 40   (delete both)
|   |   [L153] relabelCost = min(70, 40) / 2 = 20
|   [L283] case3 = 22.5 + 20 = 42.5
|   [L286] decide:
|   |   [L225] choose(case1 = 37.5, case2 = 47.5, case3 = 42.5)
|   |   [L227] case3 - 0.00001 <= case2 ? yes   case3 - 0.00001 <= case1 ? no
|   |   [L230-232] case1 < case2 ? yes -> CASE 1 wins, value 37.5, return child index 4
|   [L286-287] STORE DT(T1:6,T3:4) = 37.5   DTchoice = 4 (case 1, T3 child T3:6)
```

</details>

<details><summary><b>i=4, j=6: (T1:6, T3:2)</b> → DF = 66 (case 3), DT = 94.5 (case 3) · MM: <code>T1:8<->T3:3 28.5 + T1:9<->T3:4 37.5 + -<->- 0 + -<->- 0</code></summary>

```text
--- [L194-195] i = 4, j = 6   cell (T1:6,T3:2) ---
[L196] computeDF(T1:6,T3:2)
|   T1:6 children: T1:8,T1:9   |   T3:2 children: T3:3,T3:4
|   [L241-247] CASE 1: delete T3:2, put T1:6's forest into one child of T3:2
|   |   [L241-242] minCost = MAX, case1Child = -1
|   |   [L243-246] child T3:3: DF(T1:6,T3:3) - DF(-,T3:3) = 55 - 0 = 55  < min -> minCost = 55, case1Child = T3:3
|   |   [L243-246] child T3:4: DF(T1:6,T3:4) - DF(-,T3:4) = 22.5 - 72.5 = -50  < min -> minCost = -50, case1Child = T3:4
|   |   [L247] case1 = DF(-,T3:2) + minCost = 121 + -50 = 71
|   [L250-256] CASE 2: delete T1:6, put one child of T1:6 onto T3:2's forest
|   |   [L250-251] minCost = MAX, case2Child = -1
|   |   [L252-255] child T1:8: DF(T1:8,T3:2) - DF(T1:8,-) = 121 - 0 = 121  < min -> minCost = 121, case2Child = T1:8
|   |   [L252-255] child T1:9: DF(T1:9,T3:2) - DF(T1:9,-) = 121 - 0 = 121  not smaller
|   |   [L256] case2 = DF(T1:6,-) + minCost = 55 + 121 = 176
|   [L259] CASE 3: case3 = MM(T1:6,T3:2)  (match the two child forests)
|   |   [L294-295] a = #children of T1:6 = 2, b = #children of T3:2 = 2
|   |   [L298] n = a + b = 4 (each side padded with dummies '-' up to n)
|   |   [L302-308] cost matrix, entry [k][l] = DT(row k, col l):
|   |                T3:3      T3:4      -         -         
|   |   T1:8         28.5      82.5      5         5         
|   |   T1:9         16.5      37.5      50        50        
|   |   -            33.5      87.5      0         0         
|   |   -            33.5      87.5      0         0         
|   |   [L309] kuhnMunkresMinWeightPerfectMatching(mmCost, 4):
|   |   |   [KM:L29-33] potentials u (rows) = 0, v (cols) = 0; p[col] = row assigned to col (0 = none). Rows/cols are 1-based here.
|   |   |   [KM:L35-39] add row r1(T1:8): p[0] = 1, j0 = 0, minv = INF, used = none
|   |   |   |   [KM:L41-51] mark start used; its row i0 = r1(T1:8). reduced costs cost-u-v for unused cols: c1:28.5-0-(0)=28.5->minv 28.5; c2:82.5-0-(0)=82.5->minv 82.5; c3:5-0-(0)=5->minv 5; c4:5-0-(0)=5->minv 5;
|   |   |   |              smallest minv: delta = 5 at c3(-)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [5,0,0,0], v = [0,0,0,0]
|   |   |   |   [KM:L56-57] c3(-) is free -> augmenting path found
|   |   |   |   [KM:L58-62] flip the path: c3(-)<-r1(T1:8);
|   |   |   [KM:L35-39] add row r2(T1:9): p[0] = 2, j0 = 0, minv = INF, used = none
|   |   |   |   [KM:L41-51] mark start used; its row i0 = r2(T1:9). reduced costs cost-u-v for unused cols: c1:16.5-0-(0)=16.5->minv 16.5; c2:37.5-0-(0)=37.5->minv 37.5; c3:50-0-(0)=50->minv 50; c4:50-0-(0)=50->minv 50;
|   |   |   |              smallest minv: delta = 16.5 at c1(T3:3)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [5,16.5,0,0], v = [0,0,0,0]
|   |   |   |   [KM:L56-57] c1(T3:3) is free -> augmenting path found
|   |   |   |   [KM:L58-62] flip the path: c1(T3:3)<-r2(T1:9);
|   |   |   [KM:L35-39] add row r3(-): p[0] = 3, j0 = 0, minv = INF, used = none
|   |   |   |   [KM:L41-51] mark start used; its row i0 = r3(-). reduced costs cost-u-v for unused cols: c1:33.5-0-(0)=33.5->minv 33.5; c2:87.5-0-(0)=87.5->minv 87.5; c3:0-0-(0)=0->minv 0; c4:0-0-(0)=0->minv 0;
|   |   |   |              smallest minv: delta = 0 at c3(-)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [5,16.5,0,0], v = [0,0,0,0]
|   |   |   |   [KM:L56-57] c3(-) is already taken by r1(T1:8) -> keep searching from it
|   |   |   |   [KM:L41-51] mark c3(-) used; its row i0 = r1(T1:8). reduced costs cost-u-v for unused cols: c1:28.5-5-(0)=23.5->minv 23.5; c2:82.5-5-(0)=77.5->minv 77.5; c4:5-5-(0)=0->minv 0;
|   |   |   |              smallest minv: delta = 0 at c4(-)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [5,16.5,0,0], v = [0,0,0,0]
|   |   |   |   [KM:L56-57] c4(-) is free -> augmenting path found
|   |   |   |   [KM:L58-62] flip the path: c4(-)<-r3(-);
|   |   |   [KM:L35-39] add row r4(-): p[0] = 4, j0 = 0, minv = INF, used = none
|   |   |   |   [KM:L41-51] mark start used; its row i0 = r4(-). reduced costs cost-u-v for unused cols: c1:33.5-0-(0)=33.5->minv 33.5; c2:87.5-0-(0)=87.5->minv 87.5; c3:0-0-(0)=0->minv 0; c4:0-0-(0)=0->minv 0;
|   |   |   |              smallest minv: delta = 0 at c3(-)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [5,16.5,0,0], v = [0,0,0,0]
|   |   |   |   [KM:L56-57] c3(-) is already taken by r1(T1:8) -> keep searching from it
|   |   |   |   [KM:L41-51] mark c3(-) used; its row i0 = r1(T1:8). reduced costs cost-u-v for unused cols: c1:28.5-5-(0)=23.5->minv 23.5; c2:82.5-5-(0)=77.5->minv 77.5; c4:5-5-(0)=0->minv 0;
|   |   |   |              smallest minv: delta = 0 at c4(-)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [5,16.5,0,0], v = [0,0,0,0]
|   |   |   |   [KM:L56-57] c4(-) is already taken by r3(-) -> keep searching from it
|   |   |   |   [KM:L41-51] mark c4(-) used; its row i0 = r3(-). reduced costs cost-u-v for unused cols: c1:33.5-0-(0)=33.5->minv 23.5; c2:87.5-0-(0)=87.5->minv 77.5;
|   |   |   |              smallest minv: delta = 23.5 at c1(T3:3)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [28.5,16.5,23.5,23.5], v = [0,0,-23.5,-23.5]
|   |   |   |   [KM:L56-57] c1(T3:3) is already taken by r2(T1:9) -> keep searching from it
|   |   |   |   [KM:L41-51] mark c1(T3:3) used; its row i0 = r2(T1:9). reduced costs cost-u-v for unused cols: c2:37.5-16.5-(0)=21->minv 21;
|   |   |   |              smallest minv: delta = 21 at c2(T3:4)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [49.5,37.5,44.5,44.5], v = [-21,0,-44.5,-44.5]
|   |   |   |   [KM:L56-57] c2(T3:4) is free -> augmenting path found
|   |   |   |   [KM:L58-62] flip the path: c2(T3:4)<-r2(T1:9); c1(T3:3)<-r1(T1:8); c3(-)<-r4(-);
|   |   |   [KM:L65-70] KM RESULT: T1:8<->T3:3 28.5 + T1:9<->T3:4 37.5 + -<->- 0 + -<->- 0 = total 66
|   |   [L319] MM returns 66
|   [L259] case3 = 66
|   [L262] decide:
|   |   [L225] choose(case1 = 71, case2 = 176, case3 = 66)
|   |   [L227] case3 - 0.00001 <= case2 ? yes   case3 - 0.00001 <= case1 ? yes
|   |   [L228-229] -> CASE 3 wins, value 66, return CHOICE_MATCH (-1)
|   [L262-263] STORE DF(T1:6,T3:2) = 66   DFchoice = CHOICE_MATCH (case 3)
[L197] computeDT(T1:6,T3:2)
|   [L267-273] CASE 1: delete T3:2, put subtree T1:6 into one child subtree of T3:2
|   |   [L267-268] minCost = MAX, case1Child = -1
|   |   [L269-272] child T3:3: DT(T1:6,T3:3) - DT(-,T3:3) = 26.5 - 33.5 = -7  < min -> minCost = -7, case1Child = T3:3
|   |   [L269-272] child T3:4: DT(T1:6,T3:4) - DT(-,T3:4) = 37.5 - 87.5 = -50  < min -> minCost = -50, case1Child = T3:4
|   |   [L273] case1 = DT(-,T3:2) + minCost = 154.5 + -50 = 104.5
|   [L275-281] CASE 2: delete T1:6, put one child subtree of T1:6 onto subtree T3:2
|   |   [L275-276] minCost = MAX, case2Child = -1
|   |   [L277-280] child T1:8: DT(T1:8,T3:2) - DT(T1:8,-) = 149.5 - 5 = 144.5  < min -> minCost = 144.5, case2Child = T1:8
|   |   [L277-280] child T1:9: DT(T1:9,T3:2) - DT(T1:9,-) = 104.5 - 50 = 54.5  < min -> minCost = 54.5, case2Child = T1:9
|   |   [L281] case2 = DT(T1:6,-) + minCost = 60 + 54.5 = 114.5
|   [L283] CASE 3: match T1:6 <-> T3:2: case3 = DF(T1:6,T3:2) + relabelCost(T1:6,T3:2)
|   |   [L151] diff1 = |15 - 70| + |5 - 3| = 57   (move one node onto the other)
|   |   [L152] diff2 = |5 - 15| + |3 - 70| = 77   (delete both)
|   |   [L153] relabelCost = min(57, 77) / 2 = 28.5
|   [L283] case3 = 66 + 28.5 = 94.5
|   [L286] decide:
|   |   [L225] choose(case1 = 104.5, case2 = 114.5, case3 = 94.5)
|   |   [L227] case3 - 0.00001 <= case2 ? yes   case3 - 0.00001 <= case1 ? yes
|   |   [L228-229] -> CASE 3 wins, value 94.5, return CHOICE_MATCH (-1)
|   [L286-287] STORE DT(T1:6,T3:2) = 94.5   DTchoice = CHOICE_MATCH (case 3)
```

</details>

<details><summary><b>i=4, j=7: (T1:6, T3:1)</b> → DF = 99.5 (case 1), DT = 147 (case 3) · MM: <code>T1:8<->- 5 + T1:9<->T3:2 104.5 + -<->- 0</code></summary>

```text
--- [L194-195] i = 4, j = 7   cell (T1:6,T3:1) ---
[L196] computeDF(T1:6,T3:1)
|   T1:6 children: T1:8,T1:9   |   T3:1 children: T3:2
|   [L241-247] CASE 1: delete T3:1, put T1:6's forest into one child of T3:1
|   |   [L241-242] minCost = MAX, case1Child = -1
|   |   [L243-246] child T3:2: DF(T1:6,T3:2) - DF(-,T3:2) = 66 - 121 = -55  < min -> minCost = -55, case1Child = T3:2
|   |   [L247] case1 = DF(-,T3:1) + minCost = 154.5 + -55 = 99.5
|   [L250-256] CASE 2: delete T1:6, put one child of T1:6 onto T3:1's forest
|   |   [L250-251] minCost = MAX, case2Child = -1
|   |   [L252-255] child T1:8: DF(T1:8,T3:1) - DF(T1:8,-) = 154.5 - 0 = 154.5  < min -> minCost = 154.5, case2Child = T1:8
|   |   [L252-255] child T1:9: DF(T1:9,T3:1) - DF(T1:9,-) = 154.5 - 0 = 154.5  not smaller
|   |   [L256] case2 = DF(T1:6,-) + minCost = 55 + 154.5 = 209.5
|   [L259] CASE 3: case3 = MM(T1:6,T3:1)  (match the two child forests)
|   |   [L294-295] a = #children of T1:6 = 2, b = #children of T3:1 = 1
|   |   [L298] n = a + b = 3 (each side padded with dummies '-' up to n)
|   |   [L302-308] cost matrix, entry [k][l] = DT(row k, col l):
|   |                T3:2      -         -         
|   |   T1:8         149.5     5         5         
|   |   T1:9         104.5     50        50        
|   |   -            154.5     0         0         
|   |   [L309] kuhnMunkresMinWeightPerfectMatching(mmCost, 3):
|   |   |   [KM:L29-33] potentials u (rows) = 0, v (cols) = 0; p[col] = row assigned to col (0 = none). Rows/cols are 1-based here.
|   |   |   [KM:L35-39] add row r1(T1:8): p[0] = 1, j0 = 0, minv = INF, used = none
|   |   |   |   [KM:L41-51] mark start used; its row i0 = r1(T1:8). reduced costs cost-u-v for unused cols: c1:149.5-0-(0)=149.5->minv 149.5; c2:5-0-(0)=5->minv 5; c3:5-0-(0)=5->minv 5;
|   |   |   |              smallest minv: delta = 5 at c2(-)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [5,0,0], v = [0,0,0]
|   |   |   |   [KM:L56-57] c2(-) is free -> augmenting path found
|   |   |   |   [KM:L58-62] flip the path: c2(-)<-r1(T1:8);
|   |   |   [KM:L35-39] add row r2(T1:9): p[0] = 2, j0 = 0, minv = INF, used = none
|   |   |   |   [KM:L41-51] mark start used; its row i0 = r2(T1:9). reduced costs cost-u-v for unused cols: c1:104.5-0-(0)=104.5->minv 104.5; c2:50-0-(0)=50->minv 50; c3:50-0-(0)=50->minv 50;
|   |   |   |              smallest minv: delta = 50 at c2(-)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [5,50,0], v = [0,0,0]
|   |   |   |   [KM:L56-57] c2(-) is already taken by r1(T1:8) -> keep searching from it
|   |   |   |   [KM:L41-51] mark c2(-) used; its row i0 = r1(T1:8). reduced costs cost-u-v for unused cols: c1:149.5-5-(0)=144.5->minv 54.5; c3:5-5-(0)=0->minv 0;
|   |   |   |              smallest minv: delta = 0 at c3(-)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [5,50,0], v = [0,0,0]
|   |   |   |   [KM:L56-57] c3(-) is free -> augmenting path found
|   |   |   |   [KM:L58-62] flip the path: c3(-)<-r2(T1:9);
|   |   |   [KM:L35-39] add row r3(-): p[0] = 3, j0 = 0, minv = INF, used = none
|   |   |   |   [KM:L41-51] mark start used; its row i0 = r3(-). reduced costs cost-u-v for unused cols: c1:154.5-0-(0)=154.5->minv 154.5; c2:0-0-(0)=0->minv 0; c3:0-0-(0)=0->minv 0;
|   |   |   |              smallest minv: delta = 0 at c2(-)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [5,50,0], v = [0,0,0]
|   |   |   |   [KM:L56-57] c2(-) is already taken by r1(T1:8) -> keep searching from it
|   |   |   |   [KM:L41-51] mark c2(-) used; its row i0 = r1(T1:8). reduced costs cost-u-v for unused cols: c1:149.5-5-(0)=144.5->minv 144.5; c3:5-5-(0)=0->minv 0;
|   |   |   |              smallest minv: delta = 0 at c3(-)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [5,50,0], v = [0,0,0]
|   |   |   |   [KM:L56-57] c3(-) is already taken by r2(T1:9) -> keep searching from it
|   |   |   |   [KM:L41-51] mark c3(-) used; its row i0 = r2(T1:9). reduced costs cost-u-v for unused cols: c1:104.5-50-(0)=54.5->minv 54.5;
|   |   |   |              smallest minv: delta = 54.5 at c1(T3:2)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [59.5,104.5,54.5], v = [0,-54.5,-54.5]
|   |   |   |   [KM:L56-57] c1(T3:2) is free -> augmenting path found
|   |   |   |   [KM:L58-62] flip the path: c1(T3:2)<-r2(T1:9); c3(-)<-r3(-);
|   |   |   [KM:L65-70] KM RESULT: T1:8<->- 5 + T1:9<->T3:2 104.5 + -<->- 0 = total 109.5
|   |   [L319] MM returns 109.5
|   [L259] case3 = 109.5
|   [L262] decide:
|   |   [L225] choose(case1 = 99.5, case2 = 209.5, case3 = 109.5)
|   |   [L227] case3 - 0.00001 <= case2 ? yes   case3 - 0.00001 <= case1 ? no
|   |   [L230-232] case1 < case2 ? yes -> CASE 1 wins, value 99.5, return child index 6
|   [L262-263] STORE DF(T1:6,T3:1) = 99.5   DFchoice = 6 (case 1, T3 child T3:2)
[L197] computeDT(T1:6,T3:1)
|   [L267-273] CASE 1: delete T3:1, put subtree T1:6 into one child subtree of T3:1
|   |   [L267-268] minCost = MAX, case1Child = -1
|   |   [L269-272] child T3:2: DT(T1:6,T3:2) - DT(-,T3:2) = 94.5 - 154.5 = -60  < min -> minCost = -60, case1Child = T3:2
|   |   [L273] case1 = DT(-,T3:1) + minCost = 207 + -60 = 147
|   [L275-281] CASE 2: delete T1:6, put one child subtree of T1:6 onto subtree T3:1
|   |   [L275-276] minCost = MAX, case2Child = -1
|   |   [L277-280] child T1:8: DT(T1:8,T3:1) - DT(T1:8,-) = 202 - 5 = 197  < min -> minCost = 197, case2Child = T1:8
|   |   [L277-280] child T1:9: DT(T1:9,T3:1) - DT(T1:9,-) = 157 - 50 = 107  < min -> minCost = 107, case2Child = T1:9
|   |   [L281] case2 = DT(T1:6,-) + minCost = 60 + 107 = 167
|   [L283] CASE 3: match T1:6 <-> T3:1: case3 = DF(T1:6,T3:1) + relabelCost(T1:6,T3:1)
|   |   [L151] diff1 = |15 - 105| + |5 - 0| = 95   (move one node onto the other)
|   |   [L152] diff2 = |5 - 15| + |0 - 105| = 115   (delete both)
|   |   [L153] relabelCost = min(95, 115) / 2 = 47.5
|   [L283] case3 = 99.5 + 47.5 = 147
|   [L286] decide:
|   |   [L225] choose(case1 = 147, case2 = 167, case3 = 147)
|   |   [L227] case3 - 0.00001 <= case2 ? yes   case3 - 0.00001 <= case1 ? yes
|   |   [L228-229] -> CASE 3 wins, value 147, return CHOICE_MATCH (-1)
|   [L286-287] STORE DT(T1:6,T3:1) = 147   DTchoice = CHOICE_MATCH (case 3)
```

</details>

<details><summary><b>i=5, j=0: (T1:4, T3:3)</b> → DF = 75 (case 3), DT = 56.5 (case 2) · MM: <code>T1:5<->- 15 + T1:6<->- 60</code></summary>

```text
--- [L194-195] i = 5, j = 0   cell (T1:4,T3:3) ---
[L196] computeDF(T1:4,T3:3)
|   T1:4 children: T1:5,T1:6   |   T3:3 children: none
|   [L241-247] CASE 1: delete T3:3, put T1:4's forest into one child of T3:3
|   |   [L241-242] minCost = MAX, case1Child = -1
|   |   [L243] T3:3 has no children: loop does not run, minCost stays MAX
|   |   [L247] case1 = DF(-,T3:3) + minCost = 0 + MAX = MAX
|   [L250-256] CASE 2: delete T1:4, put one child of T1:4 onto T3:3's forest
|   |   [L250-251] minCost = MAX, case2Child = -1
|   |   [L252-255] child T1:5: DF(T1:5,T3:3) - DF(T1:5,-) = 0 - 0 = 0  < min -> minCost = 0, case2Child = T1:5
|   |   [L252-255] child T1:6: DF(T1:6,T3:3) - DF(T1:6,-) = 55 - 55 = 0  not smaller
|   |   [L256] case2 = DF(T1:4,-) + minCost = 75 + 0 = 75
|   [L259] CASE 3: case3 = MM(T1:4,T3:3)  (match the two child forests)
|   |   [L294-295] a = #children of T1:4 = 2, b = #children of T3:3 = 0
|   |   [L298] n = a + b = 2 (each side padded with dummies '-' up to n)
|   |   [L302-308] cost matrix, entry [k][l] = DT(row k, col l):
|   |                -         -         
|   |   T1:5         15        15        
|   |   T1:6         60        60        
|   |   [L309] kuhnMunkresMinWeightPerfectMatching(mmCost, 2):
|   |   |   [KM:L29-33] potentials u (rows) = 0, v (cols) = 0; p[col] = row assigned to col (0 = none). Rows/cols are 1-based here.
|   |   |   [KM:L35-39] add row r1(T1:5): p[0] = 1, j0 = 0, minv = INF, used = none
|   |   |   |   [KM:L41-51] mark start used; its row i0 = r1(T1:5). reduced costs cost-u-v for unused cols: c1:15-0-(0)=15->minv 15; c2:15-0-(0)=15->minv 15;
|   |   |   |              smallest minv: delta = 15 at c1(-)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [15,0], v = [0,0]
|   |   |   |   [KM:L56-57] c1(-) is free -> augmenting path found
|   |   |   |   [KM:L58-62] flip the path: c1(-)<-r1(T1:5);
|   |   |   [KM:L35-39] add row r2(T1:6): p[0] = 2, j0 = 0, minv = INF, used = none
|   |   |   |   [KM:L41-51] mark start used; its row i0 = r2(T1:6). reduced costs cost-u-v for unused cols: c1:60-0-(0)=60->minv 60; c2:60-0-(0)=60->minv 60;
|   |   |   |              smallest minv: delta = 60 at c1(-)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [15,60], v = [0,0]
|   |   |   |   [KM:L56-57] c1(-) is already taken by r1(T1:5) -> keep searching from it
|   |   |   |   [KM:L41-51] mark c1(-) used; its row i0 = r1(T1:5). reduced costs cost-u-v for unused cols: c2:15-15-(0)=0->minv 0;
|   |   |   |              smallest minv: delta = 0 at c2(-)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [15,60], v = [0,0]
|   |   |   |   [KM:L56-57] c2(-) is free -> augmenting path found
|   |   |   |   [KM:L58-62] flip the path: c2(-)<-r2(T1:6);
|   |   |   [KM:L65-70] KM RESULT: T1:5<->- 15 + T1:6<->- 60 = total 75
|   |   [L319] MM returns 75
|   [L259] case3 = 75
|   [L262] decide:
|   |   [L225] choose(case1 = MAX, case2 = 75, case3 = 75)
|   |   [L227] case3 - 0.00001 <= case2 ? yes   case3 - 0.00001 <= case1 ? yes
|   |   [L228-229] -> CASE 3 wins, value 75, return CHOICE_MATCH (-1)
|   [L262-263] STORE DF(T1:4,T3:3) = 75   DFchoice = CHOICE_MATCH (case 3)
[L197] computeDT(T1:4,T3:3)
|   [L267-273] CASE 1: delete T3:3, put subtree T1:4 into one child subtree of T3:3
|   |   [L267-268] minCost = MAX, case1Child = -1
|   |   [L269] T3:3 has no children: loop does not run, minCost stays MAX
|   |   [L273] case1 = DT(-,T3:3) + minCost = 33.5 + MAX = MAX
|   [L275-281] CASE 2: delete T1:4, put one child subtree of T1:4 onto subtree T3:3
|   |   [L275-276] minCost = MAX, case2Child = -1
|   |   [L277-280] child T1:5: DT(T1:5,T3:3) - DT(T1:5,-) = 18.5 - 15 = 3.5  < min -> minCost = 3.5, case2Child = T1:5
|   |   [L277-280] child T1:6: DT(T1:6,T3:3) - DT(T1:6,-) = 26.5 - 60 = -33.5  < min -> minCost = -33.5, case2Child = T1:6
|   |   [L281] case2 = DT(T1:4,-) + minCost = 90 + -33.5 = 56.5
|   [L283] CASE 3: match T1:4 <-> T3:3: case3 = DF(T1:4,T3:3) + relabelCost(T1:4,T3:3)
|   |   [L151] diff1 = |60 - 3| + |30 - 70| = 97   (move one node onto the other)
|   |   [L152] diff2 = |30 - 60| + |70 - 3| = 97   (delete both)
|   |   [L153] relabelCost = min(97, 97) / 2 = 48.5
|   [L283] case3 = 75 + 48.5 = 123.5
|   [L286] decide:
|   |   [L225] choose(case1 = MAX, case2 = 56.5, case3 = 123.5)
|   |   [L227] case3 - 0.00001 <= case2 ? no   case3 - 0.00001 <= case1 ? yes
|   |   [L230,233-235] case1 < case2 ? no -> CASE 2 wins, value 56.5, return -(4+2) = -6
|   [L286-287] STORE DT(T1:4,T3:3) = 56.5   DTchoice = -6 (case 2, T1 child T1:6)
```

</details>

<details><summary><b>i=5, j=1: (T1:4, T3:5)</b> → DF = 75 (case 3), DT = 75 (case 2) · MM: <code>T1:5<->- 15 + T1:6<->- 60</code></summary>

```text
--- [L194-195] i = 5, j = 1   cell (T1:4,T3:5) ---
[L196] computeDF(T1:4,T3:5)
|   T1:4 children: T1:5,T1:6   |   T3:5 children: none
|   [L241-247] CASE 1: delete T3:5, put T1:4's forest into one child of T3:5
|   |   [L241-242] minCost = MAX, case1Child = -1
|   |   [L243] T3:5 has no children: loop does not run, minCost stays MAX
|   |   [L247] case1 = DF(-,T3:5) + minCost = 0 + MAX = MAX
|   [L250-256] CASE 2: delete T1:4, put one child of T1:4 onto T3:5's forest
|   |   [L250-251] minCost = MAX, case2Child = -1
|   |   [L252-255] child T1:5: DF(T1:5,T3:5) - DF(T1:5,-) = 0 - 0 = 0  < min -> minCost = 0, case2Child = T1:5
|   |   [L252-255] child T1:6: DF(T1:6,T3:5) - DF(T1:6,-) = 55 - 55 = 0  not smaller
|   |   [L256] case2 = DF(T1:4,-) + minCost = 75 + 0 = 75
|   [L259] CASE 3: case3 = MM(T1:4,T3:5)  (match the two child forests)
|   |   [L294-295] a = #children of T1:4 = 2, b = #children of T3:5 = 0
|   |   [L298] n = a + b = 2 (each side padded with dummies '-' up to n)
|   |   [L302-308] cost matrix, entry [k][l] = DT(row k, col l):
|   |                -         -         
|   |   T1:5         15        15        
|   |   T1:6         60        60        
|   |   [L309] kuhnMunkresMinWeightPerfectMatching(mmCost, 2):
|   |   |   [KM:L29-33] potentials u (rows) = 0, v (cols) = 0; p[col] = row assigned to col (0 = none). Rows/cols are 1-based here.
|   |   |   [KM:L35-39] add row r1(T1:5): p[0] = 1, j0 = 0, minv = INF, used = none
|   |   |   |   [KM:L41-51] mark start used; its row i0 = r1(T1:5). reduced costs cost-u-v for unused cols: c1:15-0-(0)=15->minv 15; c2:15-0-(0)=15->minv 15;
|   |   |   |              smallest minv: delta = 15 at c1(-)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [15,0], v = [0,0]
|   |   |   |   [KM:L56-57] c1(-) is free -> augmenting path found
|   |   |   |   [KM:L58-62] flip the path: c1(-)<-r1(T1:5);
|   |   |   [KM:L35-39] add row r2(T1:6): p[0] = 2, j0 = 0, minv = INF, used = none
|   |   |   |   [KM:L41-51] mark start used; its row i0 = r2(T1:6). reduced costs cost-u-v for unused cols: c1:60-0-(0)=60->minv 60; c2:60-0-(0)=60->minv 60;
|   |   |   |              smallest minv: delta = 60 at c1(-)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [15,60], v = [0,0]
|   |   |   |   [KM:L56-57] c1(-) is already taken by r1(T1:5) -> keep searching from it
|   |   |   |   [KM:L41-51] mark c1(-) used; its row i0 = r1(T1:5). reduced costs cost-u-v for unused cols: c2:15-15-(0)=0->minv 0;
|   |   |   |              smallest minv: delta = 0 at c2(-)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [15,60], v = [0,0]
|   |   |   |   [KM:L56-57] c2(-) is free -> augmenting path found
|   |   |   |   [KM:L58-62] flip the path: c2(-)<-r2(T1:6);
|   |   |   [KM:L65-70] KM RESULT: T1:5<->- 15 + T1:6<->- 60 = total 75
|   |   [L319] MM returns 75
|   [L259] case3 = 75
|   [L262] decide:
|   |   [L225] choose(case1 = MAX, case2 = 75, case3 = 75)
|   |   [L227] case3 - 0.00001 <= case2 ? yes   case3 - 0.00001 <= case1 ? yes
|   |   [L228-229] -> CASE 3 wins, value 75, return CHOICE_MATCH (-1)
|   [L262-263] STORE DF(T1:4,T3:5) = 75   DFchoice = CHOICE_MATCH (case 3)
[L197] computeDT(T1:4,T3:5)
|   [L267-273] CASE 1: delete T3:5, put subtree T1:4 into one child subtree of T3:5
|   |   [L267-268] minCost = MAX, case1Child = -1
|   |   [L269] T3:5 has no children: loop does not run, minCost stays MAX
|   |   [L273] case1 = DT(-,T3:5) + minCost = 15 + MAX = MAX
|   [L275-281] CASE 2: delete T1:4, put one child subtree of T1:4 onto subtree T3:5
|   |   [L275-276] minCost = MAX, case2Child = -1
|   |   [L277-280] child T1:5: DT(T1:5,T3:5) - DT(T1:5,-) = 0 - 15 = -15  < min -> minCost = -15, case2Child = T1:5
|   |   [L277-280] child T1:6: DT(T1:6,T3:5) - DT(T1:6,-) = 45 - 60 = -15  not smaller
|   |   [L281] case2 = DT(T1:4,-) + minCost = 90 + -15 = 75
|   [L283] CASE 3: match T1:4 <-> T3:5: case3 = DF(T1:4,T3:5) + relabelCost(T1:4,T3:5)
|   |   [L151] diff1 = |60 - 30| + |30 - 60| = 60   (move one node onto the other)
|   |   [L152] diff2 = |30 - 60| + |60 - 30| = 60   (delete both)
|   |   [L153] relabelCost = min(60, 60) / 2 = 30
|   [L283] case3 = 75 + 30 = 105
|   [L286] decide:
|   |   [L225] choose(case1 = MAX, case2 = 75, case3 = 105)
|   |   [L227] case3 - 0.00001 <= case2 ? no   case3 - 0.00001 <= case1 ? yes
|   |   [L230,233-235] case1 < case2 ? no -> CASE 2 wins, value 75, return -(1+2) = -3
|   [L286-287] STORE DT(T1:4,T3:5) = 75   DTchoice = -3 (case 2, T1 child T1:5)
```

</details>

<details><summary><b>i=5, j=2: (T1:4, T3:8)</b> → DF = 75 (case 3), DT = 87.5 (case 2) · MM: <code>T1:5<->- 15 + T1:6<->- 60</code></summary>

```text
--- [L194-195] i = 5, j = 2   cell (T1:4,T3:8) ---
[L196] computeDF(T1:4,T3:8)
|   T1:4 children: T1:5,T1:6   |   T3:8 children: none
|   [L241-247] CASE 1: delete T3:8, put T1:4's forest into one child of T3:8
|   |   [L241-242] minCost = MAX, case1Child = -1
|   |   [L243] T3:8 has no children: loop does not run, minCost stays MAX
|   |   [L247] case1 = DF(-,T3:8) + minCost = 0 + MAX = MAX
|   [L250-256] CASE 2: delete T1:4, put one child of T1:4 onto T3:8's forest
|   |   [L250-251] minCost = MAX, case2Child = -1
|   |   [L252-255] child T1:5: DF(T1:5,T3:8) - DF(T1:5,-) = 0 - 0 = 0  < min -> minCost = 0, case2Child = T1:5
|   |   [L252-255] child T1:6: DF(T1:6,T3:8) - DF(T1:6,-) = 55 - 55 = 0  not smaller
|   |   [L256] case2 = DF(T1:4,-) + minCost = 75 + 0 = 75
|   [L259] CASE 3: case3 = MM(T1:4,T3:8)  (match the two child forests)
|   |   [L294-295] a = #children of T1:4 = 2, b = #children of T3:8 = 0
|   |   [L298] n = a + b = 2 (each side padded with dummies '-' up to n)
|   |   [L302-308] cost matrix, entry [k][l] = DT(row k, col l):
|   |                -         -         
|   |   T1:5         15        15        
|   |   T1:6         60        60        
|   |   [L309] kuhnMunkresMinWeightPerfectMatching(mmCost, 2):
|   |   |   [KM:L29-33] potentials u (rows) = 0, v (cols) = 0; p[col] = row assigned to col (0 = none). Rows/cols are 1-based here.
|   |   |   [KM:L35-39] add row r1(T1:5): p[0] = 1, j0 = 0, minv = INF, used = none
|   |   |   |   [KM:L41-51] mark start used; its row i0 = r1(T1:5). reduced costs cost-u-v for unused cols: c1:15-0-(0)=15->minv 15; c2:15-0-(0)=15->minv 15;
|   |   |   |              smallest minv: delta = 15 at c1(-)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [15,0], v = [0,0]
|   |   |   |   [KM:L56-57] c1(-) is free -> augmenting path found
|   |   |   |   [KM:L58-62] flip the path: c1(-)<-r1(T1:5);
|   |   |   [KM:L35-39] add row r2(T1:6): p[0] = 2, j0 = 0, minv = INF, used = none
|   |   |   |   [KM:L41-51] mark start used; its row i0 = r2(T1:6). reduced costs cost-u-v for unused cols: c1:60-0-(0)=60->minv 60; c2:60-0-(0)=60->minv 60;
|   |   |   |              smallest minv: delta = 60 at c1(-)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [15,60], v = [0,0]
|   |   |   |   [KM:L56-57] c1(-) is already taken by r1(T1:5) -> keep searching from it
|   |   |   |   [KM:L41-51] mark c1(-) used; its row i0 = r1(T1:5). reduced costs cost-u-v for unused cols: c2:15-15-(0)=0->minv 0;
|   |   |   |              smallest minv: delta = 0 at c2(-)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [15,60], v = [0,0]
|   |   |   |   [KM:L56-57] c2(-) is free -> augmenting path found
|   |   |   |   [KM:L58-62] flip the path: c2(-)<-r2(T1:6);
|   |   |   [KM:L65-70] KM RESULT: T1:5<->- 15 + T1:6<->- 60 = total 75
|   |   [L319] MM returns 75
|   [L259] case3 = 75
|   [L262] decide:
|   |   [L225] choose(case1 = MAX, case2 = 75, case3 = 75)
|   |   [L227] case3 - 0.00001 <= case2 ? yes   case3 - 0.00001 <= case1 ? yes
|   |   [L228-229] -> CASE 3 wins, value 75, return CHOICE_MATCH (-1)
|   [L262-263] STORE DF(T1:4,T3:8) = 75   DFchoice = CHOICE_MATCH (case 3)
[L197] computeDT(T1:4,T3:8)
|   [L267-273] CASE 1: delete T3:8, put subtree T1:4 into one child subtree of T3:8
|   |   [L267-268] minCost = MAX, case1Child = -1
|   |   [L269] T3:8 has no children: loop does not run, minCost stays MAX
|   |   [L273] case1 = DT(-,T3:8) + minCost = 2.5 + MAX = MAX
|   [L275-281] CASE 2: delete T1:4, put one child subtree of T1:4 onto subtree T3:8
|   |   [L275-276] minCost = MAX, case2Child = -1
|   |   [L277-280] child T1:5: DT(T1:5,T3:8) - DT(T1:5,-) = 17.5 - 15 = 2.5  < min -> minCost = 2.5, case2Child = T1:5
|   |   [L277-280] child T1:6: DT(T1:6,T3:8) - DT(T1:6,-) = 57.5 - 60 = -2.5  < min -> minCost = -2.5, case2Child = T1:6
|   |   [L281] case2 = DT(T1:4,-) + minCost = 90 + -2.5 = 87.5
|   [L283] CASE 3: match T1:4 <-> T3:8: case3 = DF(T1:4,T3:8) + relabelCost(T1:4,T3:8)
|   |   [L151] diff1 = |60 - 5| + |30 - 10| = 75   (move one node onto the other)
|   |   [L152] diff2 = |30 - 60| + |10 - 5| = 35   (delete both)
|   |   [L153] relabelCost = min(75, 35) / 2 = 17.5
|   [L283] case3 = 75 + 17.5 = 92.5
|   [L286] decide:
|   |   [L225] choose(case1 = MAX, case2 = 87.5, case3 = 92.5)
|   |   [L227] case3 - 0.00001 <= case2 ? no   case3 - 0.00001 <= case1 ? yes
|   |   [L230,233-235] case1 < case2 ? no -> CASE 2 wins, value 87.5, return -(4+2) = -6
|   [L286-287] STORE DT(T1:4,T3:8) = 87.5   DTchoice = -6 (case 2, T1 child T1:6)
```

</details>

<details><summary><b>i=5, j=3: (T1:4, T3:9)</b> → DF = 75 (case 3), DT = 42.5 (case 2) · MM: <code>T1:5<->- 15 + T1:6<->- 60</code></summary>

```text
--- [L194-195] i = 5, j = 3   cell (T1:4,T3:9) ---
[L196] computeDF(T1:4,T3:9)
|   T1:4 children: T1:5,T1:6   |   T3:9 children: none
|   [L241-247] CASE 1: delete T3:9, put T1:4's forest into one child of T3:9
|   |   [L241-242] minCost = MAX, case1Child = -1
|   |   [L243] T3:9 has no children: loop does not run, minCost stays MAX
|   |   [L247] case1 = DF(-,T3:9) + minCost = 0 + MAX = MAX
|   [L250-256] CASE 2: delete T1:4, put one child of T1:4 onto T3:9's forest
|   |   [L250-251] minCost = MAX, case2Child = -1
|   |   [L252-255] child T1:5: DF(T1:5,T3:9) - DF(T1:5,-) = 0 - 0 = 0  < min -> minCost = 0, case2Child = T1:5
|   |   [L252-255] child T1:6: DF(T1:6,T3:9) - DF(T1:6,-) = 55 - 55 = 0  not smaller
|   |   [L256] case2 = DF(T1:4,-) + minCost = 75 + 0 = 75
|   [L259] CASE 3: case3 = MM(T1:4,T3:9)  (match the two child forests)
|   |   [L294-295] a = #children of T1:4 = 2, b = #children of T3:9 = 0
|   |   [L298] n = a + b = 2 (each side padded with dummies '-' up to n)
|   |   [L302-308] cost matrix, entry [k][l] = DT(row k, col l):
|   |                -         -         
|   |   T1:5         15        15        
|   |   T1:6         60        60        
|   |   [L309] kuhnMunkresMinWeightPerfectMatching(mmCost, 2):
|   |   |   [KM:L29-33] potentials u (rows) = 0, v (cols) = 0; p[col] = row assigned to col (0 = none). Rows/cols are 1-based here.
|   |   |   [KM:L35-39] add row r1(T1:5): p[0] = 1, j0 = 0, minv = INF, used = none
|   |   |   |   [KM:L41-51] mark start used; its row i0 = r1(T1:5). reduced costs cost-u-v for unused cols: c1:15-0-(0)=15->minv 15; c2:15-0-(0)=15->minv 15;
|   |   |   |              smallest minv: delta = 15 at c1(-)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [15,0], v = [0,0]
|   |   |   |   [KM:L56-57] c1(-) is free -> augmenting path found
|   |   |   |   [KM:L58-62] flip the path: c1(-)<-r1(T1:5);
|   |   |   [KM:L35-39] add row r2(T1:6): p[0] = 2, j0 = 0, minv = INF, used = none
|   |   |   |   [KM:L41-51] mark start used; its row i0 = r2(T1:6). reduced costs cost-u-v for unused cols: c1:60-0-(0)=60->minv 60; c2:60-0-(0)=60->minv 60;
|   |   |   |              smallest minv: delta = 60 at c1(-)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [15,60], v = [0,0]
|   |   |   |   [KM:L56-57] c1(-) is already taken by r1(T1:5) -> keep searching from it
|   |   |   |   [KM:L41-51] mark c1(-) used; its row i0 = r1(T1:5). reduced costs cost-u-v for unused cols: c2:15-15-(0)=0->minv 0;
|   |   |   |              smallest minv: delta = 0 at c2(-)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [15,60], v = [0,0]
|   |   |   |   [KM:L56-57] c2(-) is free -> augmenting path found
|   |   |   |   [KM:L58-62] flip the path: c2(-)<-r2(T1:6);
|   |   |   [KM:L65-70] KM RESULT: T1:5<->- 15 + T1:6<->- 60 = total 75
|   |   [L319] MM returns 75
|   [L259] case3 = 75
|   [L262] decide:
|   |   [L225] choose(case1 = MAX, case2 = 75, case3 = 75)
|   |   [L227] case3 - 0.00001 <= case2 ? yes   case3 - 0.00001 <= case1 ? yes
|   |   [L228-229] -> CASE 3 wins, value 75, return CHOICE_MATCH (-1)
|   [L262-263] STORE DF(T1:4,T3:9) = 75   DFchoice = CHOICE_MATCH (case 3)
[L197] computeDT(T1:4,T3:9)
|   [L267-273] CASE 1: delete T3:9, put subtree T1:4 into one child subtree of T3:9
|   |   [L267-268] minCost = MAX, case1Child = -1
|   |   [L269] T3:9 has no children: loop does not run, minCost stays MAX
|   |   [L273] case1 = DT(-,T3:9) + minCost = 52.5 + MAX = MAX
|   [L275-281] CASE 2: delete T1:4, put one child subtree of T1:4 onto subtree T3:9
|   |   [L275-276] minCost = MAX, case2Child = -1
|   |   [L277-280] child T1:5: DT(T1:5,T3:9) - DT(T1:5,-) = 37.5 - 15 = 22.5  < min -> minCost = 22.5, case2Child = T1:5
|   |   [L277-280] child T1:6: DT(T1:6,T3:9) - DT(T1:6,-) = 12.5 - 60 = -47.5  < min -> minCost = -47.5, case2Child = T1:6
|   |   [L281] case2 = DT(T1:4,-) + minCost = 90 + -47.5 = 42.5
|   [L283] CASE 3: match T1:4 <-> T3:9: case3 = DF(T1:4,T3:9) + relabelCost(T1:4,T3:9)
|   |   [L151] diff1 = |60 - 0| + |30 - 105| = 135   (move one node onto the other)
|   |   [L152] diff2 = |30 - 60| + |105 - 0| = 135   (delete both)
|   |   [L153] relabelCost = min(135, 135) / 2 = 67.5
|   [L283] case3 = 75 + 67.5 = 142.5
|   [L286] decide:
|   |   [L225] choose(case1 = MAX, case2 = 42.5, case3 = 142.5)
|   |   [L227] case3 - 0.00001 <= case2 ? no   case3 - 0.00001 <= case1 ? yes
|   |   [L230,233-235] case1 < case2 ? no -> CASE 2 wins, value 42.5, return -(4+2) = -6
|   [L286-287] STORE DT(T1:4,T3:9) = 42.5   DTchoice = -6 (case 2, T1 child T1:6)
```

</details>

<details><summary><b>i=5, j=4: (T1:4, T3:6)</b> → DF = 25 (case 2), DT = 37.5 (case 2) · MM: <code>T1:5<->- 15 + T1:6<->T3:9 12.5 + -<->- 0 + -<->T3:8 2.5</code></summary>

```text
--- [L194-195] i = 5, j = 4   cell (T1:4,T3:6) ---
[L196] computeDF(T1:4,T3:6)
|   T1:4 children: T1:5,T1:6   |   T3:6 children: T3:8,T3:9
|   [L241-247] CASE 1: delete T3:6, put T1:4's forest into one child of T3:6
|   |   [L241-242] minCost = MAX, case1Child = -1
|   |   [L243-246] child T3:8: DF(T1:4,T3:8) - DF(-,T3:8) = 75 - 0 = 75  < min -> minCost = 75, case1Child = T3:8
|   |   [L243-246] child T3:9: DF(T1:4,T3:9) - DF(-,T3:9) = 75 - 0 = 75  not smaller
|   |   [L247] case1 = DF(-,T3:6) + minCost = 55 + 75 = 130
|   [L250-256] CASE 2: delete T1:4, put one child of T1:4 onto T3:6's forest
|   |   [L250-251] minCost = MAX, case2Child = -1
|   |   [L252-255] child T1:5: DF(T1:5,T3:6) - DF(T1:5,-) = 55 - 0 = 55  < min -> minCost = 55, case2Child = T1:5
|   |   [L252-255] child T1:6: DF(T1:6,T3:6) - DF(T1:6,-) = 5 - 55 = -50  < min -> minCost = -50, case2Child = T1:6
|   |   [L256] case2 = DF(T1:4,-) + minCost = 75 + -50 = 25
|   [L259] CASE 3: case3 = MM(T1:4,T3:6)  (match the two child forests)
|   |   [L294-295] a = #children of T1:4 = 2, b = #children of T3:6 = 2
|   |   [L298] n = a + b = 4 (each side padded with dummies '-' up to n)
|   |   [L302-308] cost matrix, entry [k][l] = DT(row k, col l):
|   |                T3:8      T3:9      -         -         
|   |   T1:5         17.5      37.5      15        15        
|   |   T1:6         57.5      12.5      60        60        
|   |   -            2.5       52.5      0         0         
|   |   -            2.5       52.5      0         0         
|   |   [L309] kuhnMunkresMinWeightPerfectMatching(mmCost, 4):
|   |   |   [KM:L29-33] potentials u (rows) = 0, v (cols) = 0; p[col] = row assigned to col (0 = none). Rows/cols are 1-based here.
|   |   |   [KM:L35-39] add row r1(T1:5): p[0] = 1, j0 = 0, minv = INF, used = none
|   |   |   |   [KM:L41-51] mark start used; its row i0 = r1(T1:5). reduced costs cost-u-v for unused cols: c1:17.5-0-(0)=17.5->minv 17.5; c2:37.5-0-(0)=37.5->minv 37.5; c3:15-0-(0)=15->minv 15; c4:15-0-(0)=15->minv 15;
|   |   |   |              smallest minv: delta = 15 at c3(-)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [15,0,0,0], v = [0,0,0,0]
|   |   |   |   [KM:L56-57] c3(-) is free -> augmenting path found
|   |   |   |   [KM:L58-62] flip the path: c3(-)<-r1(T1:5);
|   |   |   [KM:L35-39] add row r2(T1:6): p[0] = 2, j0 = 0, minv = INF, used = none
|   |   |   |   [KM:L41-51] mark start used; its row i0 = r2(T1:6). reduced costs cost-u-v for unused cols: c1:57.5-0-(0)=57.5->minv 57.5; c2:12.5-0-(0)=12.5->minv 12.5; c3:60-0-(0)=60->minv 60; c4:60-0-(0)=60->minv 60;
|   |   |   |              smallest minv: delta = 12.5 at c2(T3:9)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [15,12.5,0,0], v = [0,0,0,0]
|   |   |   |   [KM:L56-57] c2(T3:9) is free -> augmenting path found
|   |   |   |   [KM:L58-62] flip the path: c2(T3:9)<-r2(T1:6);
|   |   |   [KM:L35-39] add row r3(-): p[0] = 3, j0 = 0, minv = INF, used = none
|   |   |   |   [KM:L41-51] mark start used; its row i0 = r3(-). reduced costs cost-u-v for unused cols: c1:2.5-0-(0)=2.5->minv 2.5; c2:52.5-0-(0)=52.5->minv 52.5; c3:0-0-(0)=0->minv 0; c4:0-0-(0)=0->minv 0;
|   |   |   |              smallest minv: delta = 0 at c3(-)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [15,12.5,0,0], v = [0,0,0,0]
|   |   |   |   [KM:L56-57] c3(-) is already taken by r1(T1:5) -> keep searching from it
|   |   |   |   [KM:L41-51] mark c3(-) used; its row i0 = r1(T1:5). reduced costs cost-u-v for unused cols: c1:17.5-15-(0)=2.5->minv 2.5; c2:37.5-15-(0)=22.5->minv 22.5; c4:15-15-(0)=0->minv 0;
|   |   |   |              smallest minv: delta = 0 at c4(-)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [15,12.5,0,0], v = [0,0,0,0]
|   |   |   |   [KM:L56-57] c4(-) is free -> augmenting path found
|   |   |   |   [KM:L58-62] flip the path: c4(-)<-r3(-);
|   |   |   [KM:L35-39] add row r4(-): p[0] = 4, j0 = 0, minv = INF, used = none
|   |   |   |   [KM:L41-51] mark start used; its row i0 = r4(-). reduced costs cost-u-v for unused cols: c1:2.5-0-(0)=2.5->minv 2.5; c2:52.5-0-(0)=52.5->minv 52.5; c3:0-0-(0)=0->minv 0; c4:0-0-(0)=0->minv 0;
|   |   |   |              smallest minv: delta = 0 at c3(-)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [15,12.5,0,0], v = [0,0,0,0]
|   |   |   |   [KM:L56-57] c3(-) is already taken by r1(T1:5) -> keep searching from it
|   |   |   |   [KM:L41-51] mark c3(-) used; its row i0 = r1(T1:5). reduced costs cost-u-v for unused cols: c1:17.5-15-(0)=2.5->minv 2.5; c2:37.5-15-(0)=22.5->minv 22.5; c4:15-15-(0)=0->minv 0;
|   |   |   |              smallest minv: delta = 0 at c4(-)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [15,12.5,0,0], v = [0,0,0,0]
|   |   |   |   [KM:L56-57] c4(-) is already taken by r3(-) -> keep searching from it
|   |   |   |   [KM:L41-51] mark c4(-) used; its row i0 = r3(-). reduced costs cost-u-v for unused cols: c1:2.5-0-(0)=2.5->minv 2.5; c2:52.5-0-(0)=52.5->minv 22.5;
|   |   |   |              smallest minv: delta = 2.5 at c1(T3:8)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [17.5,12.5,2.5,2.5], v = [0,0,-2.5,-2.5]
|   |   |   |   [KM:L56-57] c1(T3:8) is free -> augmenting path found
|   |   |   |   [KM:L58-62] flip the path: c1(T3:8)<-r4(-);
|   |   |   [KM:L65-70] KM RESULT: T1:5<->- 15 + T1:6<->T3:9 12.5 + -<->- 0 + -<->T3:8 2.5 = total 30
|   |   [L319] MM returns 30
|   [L259] case3 = 30
|   [L262] decide:
|   |   [L225] choose(case1 = 130, case2 = 25, case3 = 30)
|   |   [L227] case3 - 0.00001 <= case2 ? no   case3 - 0.00001 <= case1 ? yes
|   |   [L230,233-235] case1 < case2 ? no -> CASE 2 wins, value 25, return -(4+2) = -6
|   [L262-263] STORE DF(T1:4,T3:6) = 25   DFchoice = -6 (case 2, T1 child T1:6)
[L197] computeDT(T1:4,T3:6)
|   [L267-273] CASE 1: delete T3:6, put subtree T1:4 into one child subtree of T3:6
|   |   [L267-268] minCost = MAX, case1Child = -1
|   |   [L269-272] child T3:8: DT(T1:4,T3:8) - DT(-,T3:8) = 87.5 - 2.5 = 85  < min -> minCost = 85, case1Child = T3:8
|   |   [L269-272] child T3:9: DT(T1:4,T3:9) - DT(-,T3:9) = 42.5 - 52.5 = -10  < min -> minCost = -10, case1Child = T3:9
|   |   [L273] case1 = DT(-,T3:6) + minCost = 57.5 + -10 = 47.5
|   [L275-281] CASE 2: delete T1:4, put one child subtree of T1:4 onto subtree T3:6
|   |   [L275-276] minCost = MAX, case2Child = -1
|   |   [L277-280] child T1:5: DT(T1:5,T3:6) - DT(T1:5,-) = 42.5 - 15 = 27.5  < min -> minCost = 27.5, case2Child = T1:5
|   |   [L277-280] child T1:6: DT(T1:6,T3:6) - DT(T1:6,-) = 7.5 - 60 = -52.5  < min -> minCost = -52.5, case2Child = T1:6
|   |   [L281] case2 = DT(T1:4,-) + minCost = 90 + -52.5 = 37.5
|   [L283] CASE 3: match T1:4 <-> T3:6: case3 = DF(T1:4,T3:6) + relabelCost(T1:4,T3:6)
|   |   [L151] diff1 = |60 - 10| + |30 - 5| = 75   (move one node onto the other)
|   |   [L152] diff2 = |30 - 60| + |5 - 10| = 35   (delete both)
|   |   [L153] relabelCost = min(75, 35) / 2 = 17.5
|   [L283] case3 = 25 + 17.5 = 42.5
|   [L286] decide:
|   |   [L225] choose(case1 = 47.5, case2 = 37.5, case3 = 42.5)
|   |   [L227] case3 - 0.00001 <= case2 ? no   case3 - 0.00001 <= case1 ? yes
|   |   [L230,233-235] case1 < case2 ? no -> CASE 2 wins, value 37.5, return -(4+2) = -6
|   [L286-287] STORE DT(T1:4,T3:6) = 37.5   DTchoice = -6 (case 2, T1 child T1:6)
```

</details>

<details><summary><b>i=5, j=5: (T1:4, T3:4)</b> → DF = 7.5 (case 3), DT = 7.5 (case 3) · MM: <code>T1:5<->T3:5 0 + T1:6<->T3:6 7.5 + -<->- 0 + -<->- 0</code></summary>

```text
--- [L194-195] i = 5, j = 5   cell (T1:4,T3:4) ---
[L196] computeDF(T1:4,T3:4)
|   T1:4 children: T1:5,T1:6   |   T3:4 children: T3:5,T3:6
|   [L241-247] CASE 1: delete T3:4, put T1:4's forest into one child of T3:4
|   |   [L241-242] minCost = MAX, case1Child = -1
|   |   [L243-246] child T3:5: DF(T1:4,T3:5) - DF(-,T3:5) = 75 - 0 = 75  < min -> minCost = 75, case1Child = T3:5
|   |   [L243-246] child T3:6: DF(T1:4,T3:6) - DF(-,T3:6) = 25 - 55 = -30  < min -> minCost = -30, case1Child = T3:6
|   |   [L247] case1 = DF(-,T3:4) + minCost = 72.5 + -30 = 42.5
|   [L250-256] CASE 2: delete T1:4, put one child of T1:4 onto T3:4's forest
|   |   [L250-251] minCost = MAX, case2Child = -1
|   |   [L252-255] child T1:5: DF(T1:5,T3:4) - DF(T1:5,-) = 72.5 - 0 = 72.5  < min -> minCost = 72.5, case2Child = T1:5
|   |   [L252-255] child T1:6: DF(T1:6,T3:4) - DF(T1:6,-) = 22.5 - 55 = -32.5  < min -> minCost = -32.5, case2Child = T1:6
|   |   [L256] case2 = DF(T1:4,-) + minCost = 75 + -32.5 = 42.5
|   [L259] CASE 3: case3 = MM(T1:4,T3:4)  (match the two child forests)
|   |   [L294-295] a = #children of T1:4 = 2, b = #children of T3:4 = 2
|   |   [L298] n = a + b = 4 (each side padded with dummies '-' up to n)
|   |   [L302-308] cost matrix, entry [k][l] = DT(row k, col l):
|   |                T3:5      T3:6      -         -         
|   |   T1:5         0         42.5      15        15        
|   |   T1:6         45        7.5       60        60        
|   |   -            15        57.5      0         0         
|   |   -            15        57.5      0         0         
|   |   [L309] kuhnMunkresMinWeightPerfectMatching(mmCost, 4):
|   |   |   [KM:L29-33] potentials u (rows) = 0, v (cols) = 0; p[col] = row assigned to col (0 = none). Rows/cols are 1-based here.
|   |   |   [KM:L35-39] add row r1(T1:5): p[0] = 1, j0 = 0, minv = INF, used = none
|   |   |   |   [KM:L41-51] mark start used; its row i0 = r1(T1:5). reduced costs cost-u-v for unused cols: c1:0-0-(0)=0->minv 0; c2:42.5-0-(0)=42.5->minv 42.5; c3:15-0-(0)=15->minv 15; c4:15-0-(0)=15->minv 15;
|   |   |   |              smallest minv: delta = 0 at c1(T3:5)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [0,0,0,0], v = [0,0,0,0]
|   |   |   |   [KM:L56-57] c1(T3:5) is free -> augmenting path found
|   |   |   |   [KM:L58-62] flip the path: c1(T3:5)<-r1(T1:5);
|   |   |   [KM:L35-39] add row r2(T1:6): p[0] = 2, j0 = 0, minv = INF, used = none
|   |   |   |   [KM:L41-51] mark start used; its row i0 = r2(T1:6). reduced costs cost-u-v for unused cols: c1:45-0-(0)=45->minv 45; c2:7.5-0-(0)=7.5->minv 7.5; c3:60-0-(0)=60->minv 60; c4:60-0-(0)=60->minv 60;
|   |   |   |              smallest minv: delta = 7.5 at c2(T3:6)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [0,7.5,0,0], v = [0,0,0,0]
|   |   |   |   [KM:L56-57] c2(T3:6) is free -> augmenting path found
|   |   |   |   [KM:L58-62] flip the path: c2(T3:6)<-r2(T1:6);
|   |   |   [KM:L35-39] add row r3(-): p[0] = 3, j0 = 0, minv = INF, used = none
|   |   |   |   [KM:L41-51] mark start used; its row i0 = r3(-). reduced costs cost-u-v for unused cols: c1:15-0-(0)=15->minv 15; c2:57.5-0-(0)=57.5->minv 57.5; c3:0-0-(0)=0->minv 0; c4:0-0-(0)=0->minv 0;
|   |   |   |              smallest minv: delta = 0 at c3(-)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [0,7.5,0,0], v = [0,0,0,0]
|   |   |   |   [KM:L56-57] c3(-) is free -> augmenting path found
|   |   |   |   [KM:L58-62] flip the path: c3(-)<-r3(-);
|   |   |   [KM:L35-39] add row r4(-): p[0] = 4, j0 = 0, minv = INF, used = none
|   |   |   |   [KM:L41-51] mark start used; its row i0 = r4(-). reduced costs cost-u-v for unused cols: c1:15-0-(0)=15->minv 15; c2:57.5-0-(0)=57.5->minv 57.5; c3:0-0-(0)=0->minv 0; c4:0-0-(0)=0->minv 0;
|   |   |   |              smallest minv: delta = 0 at c3(-)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [0,7.5,0,0], v = [0,0,0,0]
|   |   |   |   [KM:L56-57] c3(-) is already taken by r3(-) -> keep searching from it
|   |   |   |   [KM:L41-51] mark c3(-) used; its row i0 = r3(-). reduced costs cost-u-v for unused cols: c1:15-0-(0)=15->minv 15; c2:57.5-0-(0)=57.5->minv 57.5; c4:0-0-(0)=0->minv 0;
|   |   |   |              smallest minv: delta = 0 at c4(-)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [0,7.5,0,0], v = [0,0,0,0]
|   |   |   |   [KM:L56-57] c4(-) is free -> augmenting path found
|   |   |   |   [KM:L58-62] flip the path: c4(-)<-r4(-);
|   |   |   [KM:L65-70] KM RESULT: T1:5<->T3:5 0 + T1:6<->T3:6 7.5 + -<->- 0 + -<->- 0 = total 7.5
|   |   [L319] MM returns 7.5
|   [L259] case3 = 7.5
|   [L262] decide:
|   |   [L225] choose(case1 = 42.5, case2 = 42.5, case3 = 7.5)
|   |   [L227] case3 - 0.00001 <= case2 ? yes   case3 - 0.00001 <= case1 ? yes
|   |   [L228-229] -> CASE 3 wins, value 7.5, return CHOICE_MATCH (-1)
|   [L262-263] STORE DF(T1:4,T3:4) = 7.5   DFchoice = CHOICE_MATCH (case 3)
[L197] computeDT(T1:4,T3:4)
|   [L267-273] CASE 1: delete T3:4, put subtree T1:4 into one child subtree of T3:4
|   |   [L267-268] minCost = MAX, case1Child = -1
|   |   [L269-272] child T3:5: DT(T1:4,T3:5) - DT(-,T3:5) = 75 - 15 = 60  < min -> minCost = 60, case1Child = T3:5
|   |   [L269-272] child T3:6: DT(T1:4,T3:6) - DT(-,T3:6) = 37.5 - 57.5 = -20  < min -> minCost = -20, case1Child = T3:6
|   |   [L273] case1 = DT(-,T3:4) + minCost = 87.5 + -20 = 67.5
|   [L275-281] CASE 2: delete T1:4, put one child subtree of T1:4 onto subtree T3:4
|   |   [L275-276] minCost = MAX, case2Child = -1
|   |   [L277-280] child T1:5: DT(T1:5,T3:4) - DT(T1:5,-) = 72.5 - 15 = 57.5  < min -> minCost = 57.5, case2Child = T1:5
|   |   [L277-280] child T1:6: DT(T1:6,T3:4) - DT(T1:6,-) = 37.5 - 60 = -22.5  < min -> minCost = -22.5, case2Child = T1:6
|   |   [L281] case2 = DT(T1:4,-) + minCost = 90 + -22.5 = 67.5
|   [L283] CASE 3: match T1:4 <-> T3:4: case3 = DF(T1:4,T3:4) + relabelCost(T1:4,T3:4)
|   |   [L151] diff1 = |60 - 60| + |30 - 30| = 0   (move one node onto the other)
|   |   [L152] diff2 = |30 - 60| + |30 - 60| = 60   (delete both)
|   |   [L153] relabelCost = min(0, 60) / 2 = 0
|   [L283] case3 = 7.5 + 0 = 7.5
|   [L286] decide:
|   |   [L225] choose(case1 = 67.5, case2 = 67.5, case3 = 7.5)
|   |   [L227] case3 - 0.00001 <= case2 ? yes   case3 - 0.00001 <= case1 ? yes
|   |   [L228-229] -> CASE 3 wins, value 7.5, return CHOICE_MATCH (-1)
|   [L286-287] STORE DT(T1:4,T3:4) = 7.5   DTchoice = CHOICE_MATCH (case 3)
```

</details>

<details><summary><b>i=5, j=6: (T1:4, T3:2)</b> → DF = 56 (case 3), DT = 74.5 (case 3) · MM: <code>T1:5<->T3:3 18.5 + T1:6<->T3:4 37.5 + -<->- 0 + -<->- 0</code></summary>

```text
--- [L194-195] i = 5, j = 6   cell (T1:4,T3:2) ---
[L196] computeDF(T1:4,T3:2)
|   T1:4 children: T1:5,T1:6   |   T3:2 children: T3:3,T3:4
|   [L241-247] CASE 1: delete T3:2, put T1:4's forest into one child of T3:2
|   |   [L241-242] minCost = MAX, case1Child = -1
|   |   [L243-246] child T3:3: DF(T1:4,T3:3) - DF(-,T3:3) = 75 - 0 = 75  < min -> minCost = 75, case1Child = T3:3
|   |   [L243-246] child T3:4: DF(T1:4,T3:4) - DF(-,T3:4) = 7.5 - 72.5 = -65  < min -> minCost = -65, case1Child = T3:4
|   |   [L247] case1 = DF(-,T3:2) + minCost = 121 + -65 = 56
|   [L250-256] CASE 2: delete T1:4, put one child of T1:4 onto T3:2's forest
|   |   [L250-251] minCost = MAX, case2Child = -1
|   |   [L252-255] child T1:5: DF(T1:5,T3:2) - DF(T1:5,-) = 121 - 0 = 121  < min -> minCost = 121, case2Child = T1:5
|   |   [L252-255] child T1:6: DF(T1:6,T3:2) - DF(T1:6,-) = 66 - 55 = 11  < min -> minCost = 11, case2Child = T1:6
|   |   [L256] case2 = DF(T1:4,-) + minCost = 75 + 11 = 86
|   [L259] CASE 3: case3 = MM(T1:4,T3:2)  (match the two child forests)
|   |   [L294-295] a = #children of T1:4 = 2, b = #children of T3:2 = 2
|   |   [L298] n = a + b = 4 (each side padded with dummies '-' up to n)
|   |   [L302-308] cost matrix, entry [k][l] = DT(row k, col l):
|   |                T3:3      T3:4      -         -         
|   |   T1:5         18.5      72.5      15        15        
|   |   T1:6         26.5      37.5      60        60        
|   |   -            33.5      87.5      0         0         
|   |   -            33.5      87.5      0         0         
|   |   [L309] kuhnMunkresMinWeightPerfectMatching(mmCost, 4):
|   |   |   [KM:L29-33] potentials u (rows) = 0, v (cols) = 0; p[col] = row assigned to col (0 = none). Rows/cols are 1-based here.
|   |   |   [KM:L35-39] add row r1(T1:5): p[0] = 1, j0 = 0, minv = INF, used = none
|   |   |   |   [KM:L41-51] mark start used; its row i0 = r1(T1:5). reduced costs cost-u-v for unused cols: c1:18.5-0-(0)=18.5->minv 18.5; c2:72.5-0-(0)=72.5->minv 72.5; c3:15-0-(0)=15->minv 15; c4:15-0-(0)=15->minv 15;
|   |   |   |              smallest minv: delta = 15 at c3(-)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [15,0,0,0], v = [0,0,0,0]
|   |   |   |   [KM:L56-57] c3(-) is free -> augmenting path found
|   |   |   |   [KM:L58-62] flip the path: c3(-)<-r1(T1:5);
|   |   |   [KM:L35-39] add row r2(T1:6): p[0] = 2, j0 = 0, minv = INF, used = none
|   |   |   |   [KM:L41-51] mark start used; its row i0 = r2(T1:6). reduced costs cost-u-v for unused cols: c1:26.5-0-(0)=26.5->minv 26.5; c2:37.5-0-(0)=37.5->minv 37.5; c3:60-0-(0)=60->minv 60; c4:60-0-(0)=60->minv 60;
|   |   |   |              smallest minv: delta = 26.5 at c1(T3:3)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [15,26.5,0,0], v = [0,0,0,0]
|   |   |   |   [KM:L56-57] c1(T3:3) is free -> augmenting path found
|   |   |   |   [KM:L58-62] flip the path: c1(T3:3)<-r2(T1:6);
|   |   |   [KM:L35-39] add row r3(-): p[0] = 3, j0 = 0, minv = INF, used = none
|   |   |   |   [KM:L41-51] mark start used; its row i0 = r3(-). reduced costs cost-u-v for unused cols: c1:33.5-0-(0)=33.5->minv 33.5; c2:87.5-0-(0)=87.5->minv 87.5; c3:0-0-(0)=0->minv 0; c4:0-0-(0)=0->minv 0;
|   |   |   |              smallest minv: delta = 0 at c3(-)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [15,26.5,0,0], v = [0,0,0,0]
|   |   |   |   [KM:L56-57] c3(-) is already taken by r1(T1:5) -> keep searching from it
|   |   |   |   [KM:L41-51] mark c3(-) used; its row i0 = r1(T1:5). reduced costs cost-u-v for unused cols: c1:18.5-15-(0)=3.5->minv 3.5; c2:72.5-15-(0)=57.5->minv 57.5; c4:15-15-(0)=0->minv 0;
|   |   |   |              smallest minv: delta = 0 at c4(-)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [15,26.5,0,0], v = [0,0,0,0]
|   |   |   |   [KM:L56-57] c4(-) is free -> augmenting path found
|   |   |   |   [KM:L58-62] flip the path: c4(-)<-r3(-);
|   |   |   [KM:L35-39] add row r4(-): p[0] = 4, j0 = 0, minv = INF, used = none
|   |   |   |   [KM:L41-51] mark start used; its row i0 = r4(-). reduced costs cost-u-v for unused cols: c1:33.5-0-(0)=33.5->minv 33.5; c2:87.5-0-(0)=87.5->minv 87.5; c3:0-0-(0)=0->minv 0; c4:0-0-(0)=0->minv 0;
|   |   |   |              smallest minv: delta = 0 at c3(-)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [15,26.5,0,0], v = [0,0,0,0]
|   |   |   |   [KM:L56-57] c3(-) is already taken by r1(T1:5) -> keep searching from it
|   |   |   |   [KM:L41-51] mark c3(-) used; its row i0 = r1(T1:5). reduced costs cost-u-v for unused cols: c1:18.5-15-(0)=3.5->minv 3.5; c2:72.5-15-(0)=57.5->minv 57.5; c4:15-15-(0)=0->minv 0;
|   |   |   |              smallest minv: delta = 0 at c4(-)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [15,26.5,0,0], v = [0,0,0,0]
|   |   |   |   [KM:L56-57] c4(-) is already taken by r3(-) -> keep searching from it
|   |   |   |   [KM:L41-51] mark c4(-) used; its row i0 = r3(-). reduced costs cost-u-v for unused cols: c1:33.5-0-(0)=33.5->minv 3.5; c2:87.5-0-(0)=87.5->minv 57.5;
|   |   |   |              smallest minv: delta = 3.5 at c1(T3:3)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [18.5,26.5,3.5,3.5], v = [0,0,-3.5,-3.5]
|   |   |   |   [KM:L56-57] c1(T3:3) is already taken by r2(T1:6) -> keep searching from it
|   |   |   |   [KM:L41-51] mark c1(T3:3) used; its row i0 = r2(T1:6). reduced costs cost-u-v for unused cols: c2:37.5-26.5-(0)=11->minv 11;
|   |   |   |              smallest minv: delta = 11 at c2(T3:4)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [29.5,37.5,14.5,14.5], v = [-11,0,-14.5,-14.5]
|   |   |   |   [KM:L56-57] c2(T3:4) is free -> augmenting path found
|   |   |   |   [KM:L58-62] flip the path: c2(T3:4)<-r2(T1:6); c1(T3:3)<-r1(T1:5); c3(-)<-r4(-);
|   |   |   [KM:L65-70] KM RESULT: T1:5<->T3:3 18.5 + T1:6<->T3:4 37.5 + -<->- 0 + -<->- 0 = total 56
|   |   [L319] MM returns 56
|   [L259] case3 = 56
|   [L262] decide:
|   |   [L225] choose(case1 = 56, case2 = 86, case3 = 56)
|   |   [L227] case3 - 0.00001 <= case2 ? yes   case3 - 0.00001 <= case1 ? yes
|   |   [L228-229] -> CASE 3 wins, value 56, return CHOICE_MATCH (-1)
|   [L262-263] STORE DF(T1:4,T3:2) = 56   DFchoice = CHOICE_MATCH (case 3)
[L197] computeDT(T1:4,T3:2)
|   [L267-273] CASE 1: delete T3:2, put subtree T1:4 into one child subtree of T3:2
|   |   [L267-268] minCost = MAX, case1Child = -1
|   |   [L269-272] child T3:3: DT(T1:4,T3:3) - DT(-,T3:3) = 56.5 - 33.5 = 23  < min -> minCost = 23, case1Child = T3:3
|   |   [L269-272] child T3:4: DT(T1:4,T3:4) - DT(-,T3:4) = 7.5 - 87.5 = -80  < min -> minCost = -80, case1Child = T3:4
|   |   [L273] case1 = DT(-,T3:2) + minCost = 154.5 + -80 = 74.5
|   [L275-281] CASE 2: delete T1:4, put one child subtree of T1:4 onto subtree T3:2
|   |   [L275-276] minCost = MAX, case2Child = -1
|   |   [L277-280] child T1:5: DT(T1:5,T3:2) - DT(T1:5,-) = 139.5 - 15 = 124.5  < min -> minCost = 124.5, case2Child = T1:5
|   |   [L277-280] child T1:6: DT(T1:6,T3:2) - DT(T1:6,-) = 94.5 - 60 = 34.5  < min -> minCost = 34.5, case2Child = T1:6
|   |   [L281] case2 = DT(T1:4,-) + minCost = 90 + 34.5 = 124.5
|   [L283] CASE 3: match T1:4 <-> T3:2: case3 = DF(T1:4,T3:2) + relabelCost(T1:4,T3:2)
|   |   [L151] diff1 = |60 - 70| + |30 - 3| = 37   (move one node onto the other)
|   |   [L152] diff2 = |30 - 60| + |3 - 70| = 97   (delete both)
|   |   [L153] relabelCost = min(37, 97) / 2 = 18.5
|   [L283] case3 = 56 + 18.5 = 74.5
|   [L286] decide:
|   |   [L225] choose(case1 = 74.5, case2 = 124.5, case3 = 74.5)
|   |   [L227] case3 - 0.00001 <= case2 ? yes   case3 - 0.00001 <= case1 ? yes
|   |   [L228-229] -> CASE 3 wins, value 74.5, return CHOICE_MATCH (-1)
|   [L286-287] STORE DT(T1:4,T3:2) = 74.5   DTchoice = CHOICE_MATCH (case 3)
```

</details>

<details><summary><b>i=5, j=7: (T1:4, T3:1)</b> → DF = 89.5 (case 1), DT = 127 (case 3) · MM: <code>T1:5<->- 15 + T1:6<->T3:2 94.5 + -<->- 0</code></summary>

```text
--- [L194-195] i = 5, j = 7   cell (T1:4,T3:1) ---
[L196] computeDF(T1:4,T3:1)
|   T1:4 children: T1:5,T1:6   |   T3:1 children: T3:2
|   [L241-247] CASE 1: delete T3:1, put T1:4's forest into one child of T3:1
|   |   [L241-242] minCost = MAX, case1Child = -1
|   |   [L243-246] child T3:2: DF(T1:4,T3:2) - DF(-,T3:2) = 56 - 121 = -65  < min -> minCost = -65, case1Child = T3:2
|   |   [L247] case1 = DF(-,T3:1) + minCost = 154.5 + -65 = 89.5
|   [L250-256] CASE 2: delete T1:4, put one child of T1:4 onto T3:1's forest
|   |   [L250-251] minCost = MAX, case2Child = -1
|   |   [L252-255] child T1:5: DF(T1:5,T3:1) - DF(T1:5,-) = 154.5 - 0 = 154.5  < min -> minCost = 154.5, case2Child = T1:5
|   |   [L252-255] child T1:6: DF(T1:6,T3:1) - DF(T1:6,-) = 99.5 - 55 = 44.5  < min -> minCost = 44.5, case2Child = T1:6
|   |   [L256] case2 = DF(T1:4,-) + minCost = 75 + 44.5 = 119.5
|   [L259] CASE 3: case3 = MM(T1:4,T3:1)  (match the two child forests)
|   |   [L294-295] a = #children of T1:4 = 2, b = #children of T3:1 = 1
|   |   [L298] n = a + b = 3 (each side padded with dummies '-' up to n)
|   |   [L302-308] cost matrix, entry [k][l] = DT(row k, col l):
|   |                T3:2      -         -         
|   |   T1:5         139.5     15        15        
|   |   T1:6         94.5      60        60        
|   |   -            154.5     0         0         
|   |   [L309] kuhnMunkresMinWeightPerfectMatching(mmCost, 3):
|   |   |   [KM:L29-33] potentials u (rows) = 0, v (cols) = 0; p[col] = row assigned to col (0 = none). Rows/cols are 1-based here.
|   |   |   [KM:L35-39] add row r1(T1:5): p[0] = 1, j0 = 0, minv = INF, used = none
|   |   |   |   [KM:L41-51] mark start used; its row i0 = r1(T1:5). reduced costs cost-u-v for unused cols: c1:139.5-0-(0)=139.5->minv 139.5; c2:15-0-(0)=15->minv 15; c3:15-0-(0)=15->minv 15;
|   |   |   |              smallest minv: delta = 15 at c2(-)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [15,0,0], v = [0,0,0]
|   |   |   |   [KM:L56-57] c2(-) is free -> augmenting path found
|   |   |   |   [KM:L58-62] flip the path: c2(-)<-r1(T1:5);
|   |   |   [KM:L35-39] add row r2(T1:6): p[0] = 2, j0 = 0, minv = INF, used = none
|   |   |   |   [KM:L41-51] mark start used; its row i0 = r2(T1:6). reduced costs cost-u-v for unused cols: c1:94.5-0-(0)=94.5->minv 94.5; c2:60-0-(0)=60->minv 60; c3:60-0-(0)=60->minv 60;
|   |   |   |              smallest minv: delta = 60 at c2(-)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [15,60,0], v = [0,0,0]
|   |   |   |   [KM:L56-57] c2(-) is already taken by r1(T1:5) -> keep searching from it
|   |   |   |   [KM:L41-51] mark c2(-) used; its row i0 = r1(T1:5). reduced costs cost-u-v for unused cols: c1:139.5-15-(0)=124.5->minv 34.5; c3:15-15-(0)=0->minv 0;
|   |   |   |              smallest minv: delta = 0 at c3(-)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [15,60,0], v = [0,0,0]
|   |   |   |   [KM:L56-57] c3(-) is free -> augmenting path found
|   |   |   |   [KM:L58-62] flip the path: c3(-)<-r2(T1:6);
|   |   |   [KM:L35-39] add row r3(-): p[0] = 3, j0 = 0, minv = INF, used = none
|   |   |   |   [KM:L41-51] mark start used; its row i0 = r3(-). reduced costs cost-u-v for unused cols: c1:154.5-0-(0)=154.5->minv 154.5; c2:0-0-(0)=0->minv 0; c3:0-0-(0)=0->minv 0;
|   |   |   |              smallest minv: delta = 0 at c2(-)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [15,60,0], v = [0,0,0]
|   |   |   |   [KM:L56-57] c2(-) is already taken by r1(T1:5) -> keep searching from it
|   |   |   |   [KM:L41-51] mark c2(-) used; its row i0 = r1(T1:5). reduced costs cost-u-v for unused cols: c1:139.5-15-(0)=124.5->minv 124.5; c3:15-15-(0)=0->minv 0;
|   |   |   |              smallest minv: delta = 0 at c3(-)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [15,60,0], v = [0,0,0]
|   |   |   |   [KM:L56-57] c3(-) is already taken by r2(T1:6) -> keep searching from it
|   |   |   |   [KM:L41-51] mark c3(-) used; its row i0 = r2(T1:6). reduced costs cost-u-v for unused cols: c1:94.5-60-(0)=34.5->minv 34.5;
|   |   |   |              smallest minv: delta = 34.5 at c1(T3:2)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [49.5,94.5,34.5], v = [0,-34.5,-34.5]
|   |   |   |   [KM:L56-57] c1(T3:2) is free -> augmenting path found
|   |   |   |   [KM:L58-62] flip the path: c1(T3:2)<-r2(T1:6); c3(-)<-r3(-);
|   |   |   [KM:L65-70] KM RESULT: T1:5<->- 15 + T1:6<->T3:2 94.5 + -<->- 0 = total 109.5
|   |   [L319] MM returns 109.5
|   [L259] case3 = 109.5
|   [L262] decide:
|   |   [L225] choose(case1 = 89.5, case2 = 119.5, case3 = 109.5)
|   |   [L227] case3 - 0.00001 <= case2 ? yes   case3 - 0.00001 <= case1 ? no
|   |   [L230-232] case1 < case2 ? yes -> CASE 1 wins, value 89.5, return child index 6
|   [L262-263] STORE DF(T1:4,T3:1) = 89.5   DFchoice = 6 (case 1, T3 child T3:2)
[L197] computeDT(T1:4,T3:1)
|   [L267-273] CASE 1: delete T3:1, put subtree T1:4 into one child subtree of T3:1
|   |   [L267-268] minCost = MAX, case1Child = -1
|   |   [L269-272] child T3:2: DT(T1:4,T3:2) - DT(-,T3:2) = 74.5 - 154.5 = -80  < min -> minCost = -80, case1Child = T3:2
|   |   [L273] case1 = DT(-,T3:1) + minCost = 207 + -80 = 127
|   [L275-281] CASE 2: delete T1:4, put one child subtree of T1:4 onto subtree T3:1
|   |   [L275-276] minCost = MAX, case2Child = -1
|   |   [L277-280] child T1:5: DT(T1:5,T3:1) - DT(T1:5,-) = 192 - 15 = 177  < min -> minCost = 177, case2Child = T1:5
|   |   [L277-280] child T1:6: DT(T1:6,T3:1) - DT(T1:6,-) = 147 - 60 = 87  < min -> minCost = 87, case2Child = T1:6
|   |   [L281] case2 = DT(T1:4,-) + minCost = 90 + 87 = 177
|   [L283] CASE 3: match T1:4 <-> T3:1: case3 = DF(T1:4,T3:1) + relabelCost(T1:4,T3:1)
|   |   [L151] diff1 = |60 - 105| + |30 - 0| = 75   (move one node onto the other)
|   |   [L152] diff2 = |30 - 60| + |0 - 105| = 135   (delete both)
|   |   [L153] relabelCost = min(75, 135) / 2 = 37.5
|   [L283] case3 = 89.5 + 37.5 = 127
|   [L286] decide:
|   |   [L225] choose(case1 = 127, case2 = 177, case3 = 127)
|   |   [L227] case3 - 0.00001 <= case2 ? yes   case3 - 0.00001 <= case1 ? yes
|   |   [L228-229] -> CASE 3 wins, value 127, return CHOICE_MATCH (-1)
|   [L286-287] STORE DT(T1:4,T3:1) = 127   DTchoice = CHOICE_MATCH (case 3)
```

</details>

<details><summary><b>i=6, j=0: (T1:2, T3:3)</b> → DF = 128 (case 3), DT = 132.5 (case 2) · MM: <code>T1:3<->- 38 + T1:4<->- 90</code></summary>

```text
--- [L194-195] i = 6, j = 0   cell (T1:2,T3:3) ---
[L196] computeDF(T1:2,T3:3)
|   T1:2 children: T1:3,T1:4   |   T3:3 children: none
|   [L241-247] CASE 1: delete T3:3, put T1:2's forest into one child of T3:3
|   |   [L241-242] minCost = MAX, case1Child = -1
|   |   [L243] T3:3 has no children: loop does not run, minCost stays MAX
|   |   [L247] case1 = DF(-,T3:3) + minCost = 0 + MAX = MAX
|   [L250-256] CASE 2: delete T1:2, put one child of T1:2 onto T3:3's forest
|   |   [L250-251] minCost = MAX, case2Child = -1
|   |   [L252-255] child T1:3: DF(T1:3,T3:3) - DF(T1:3,-) = 0 - 0 = 0  < min -> minCost = 0, case2Child = T1:3
|   |   [L252-255] child T1:4: DF(T1:4,T3:3) - DF(T1:4,-) = 75 - 75 = 0  not smaller
|   |   [L256] case2 = DF(T1:2,-) + minCost = 128 + 0 = 128
|   [L259] CASE 3: case3 = MM(T1:2,T3:3)  (match the two child forests)
|   |   [L294-295] a = #children of T1:2 = 2, b = #children of T3:3 = 0
|   |   [L298] n = a + b = 2 (each side padded with dummies '-' up to n)
|   |   [L302-308] cost matrix, entry [k][l] = DT(row k, col l):
|   |                -         -         
|   |   T1:3         38        38        
|   |   T1:4         90        90        
|   |   [L309] kuhnMunkresMinWeightPerfectMatching(mmCost, 2):
|   |   |   [KM:L29-33] potentials u (rows) = 0, v (cols) = 0; p[col] = row assigned to col (0 = none). Rows/cols are 1-based here.
|   |   |   [KM:L35-39] add row r1(T1:3): p[0] = 1, j0 = 0, minv = INF, used = none
|   |   |   |   [KM:L41-51] mark start used; its row i0 = r1(T1:3). reduced costs cost-u-v for unused cols: c1:38-0-(0)=38->minv 38; c2:38-0-(0)=38->minv 38;
|   |   |   |              smallest minv: delta = 38 at c1(-)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [38,0], v = [0,0]
|   |   |   |   [KM:L56-57] c1(-) is free -> augmenting path found
|   |   |   |   [KM:L58-62] flip the path: c1(-)<-r1(T1:3);
|   |   |   [KM:L35-39] add row r2(T1:4): p[0] = 2, j0 = 0, minv = INF, used = none
|   |   |   |   [KM:L41-51] mark start used; its row i0 = r2(T1:4). reduced costs cost-u-v for unused cols: c1:90-0-(0)=90->minv 90; c2:90-0-(0)=90->minv 90;
|   |   |   |              smallest minv: delta = 90 at c1(-)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [38,90], v = [0,0]
|   |   |   |   [KM:L56-57] c1(-) is already taken by r1(T1:3) -> keep searching from it
|   |   |   |   [KM:L41-51] mark c1(-) used; its row i0 = r1(T1:3). reduced costs cost-u-v for unused cols: c2:38-38-(0)=0->minv 0;
|   |   |   |              smallest minv: delta = 0 at c2(-)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [38,90], v = [0,0]
|   |   |   |   [KM:L56-57] c2(-) is free -> augmenting path found
|   |   |   |   [KM:L58-62] flip the path: c2(-)<-r2(T1:4);
|   |   |   [KM:L65-70] KM RESULT: T1:3<->- 38 + T1:4<->- 90 = total 128
|   |   [L319] MM returns 128
|   [L259] case3 = 128
|   [L262] decide:
|   |   [L225] choose(case1 = MAX, case2 = 128, case3 = 128)
|   |   [L227] case3 - 0.00001 <= case2 ? yes   case3 - 0.00001 <= case1 ? yes
|   |   [L228-229] -> CASE 3 wins, value 128, return CHOICE_MATCH (-1)
|   [L262-263] STORE DF(T1:2,T3:3) = 128   DFchoice = CHOICE_MATCH (case 3)
[L197] computeDT(T1:2,T3:3)
|   [L267-273] CASE 1: delete T3:3, put subtree T1:2 into one child subtree of T3:3
|   |   [L267-268] minCost = MAX, case1Child = -1
|   |   [L269] T3:3 has no children: loop does not run, minCost stays MAX
|   |   [L273] case1 = DT(-,T3:3) + minCost = 33.5 + MAX = MAX
|   [L275-281] CASE 2: delete T1:2, put one child subtree of T1:2 onto subtree T3:3
|   |   [L275-276] minCost = MAX, case2Child = -1
|   |   [L277-280] child T1:3: DT(T1:3,T3:3) - DT(T1:3,-) = 5.5 - 38 = -32.5  < min -> minCost = -32.5, case2Child = T1:3
|   |   [L277-280] child T1:4: DT(T1:4,T3:3) - DT(T1:4,-) = 56.5 - 90 = -33.5  < min -> minCost = -33.5, case2Child = T1:4
|   |   [L281] case2 = DT(T1:2,-) + minCost = 166 + -33.5 = 132.5
|   [L283] CASE 3: match T1:2 <-> T3:3: case3 = DF(T1:2,T3:3) + relabelCost(T1:2,T3:3)
|   |   [L151] diff1 = |80 - 3| + |4 - 70| = 143   (move one node onto the other)
|   |   [L152] diff2 = |4 - 80| + |70 - 3| = 143   (delete both)
|   |   [L153] relabelCost = min(143, 143) / 2 = 71.5
|   [L283] case3 = 128 + 71.5 = 199.5
|   [L286] decide:
|   |   [L225] choose(case1 = MAX, case2 = 132.5, case3 = 199.5)
|   |   [L227] case3 - 0.00001 <= case2 ? no   case3 - 0.00001 <= case1 ? yes
|   |   [L230,233-235] case1 < case2 ? no -> CASE 2 wins, value 132.5, return -(5+2) = -7
|   [L286-287] STORE DT(T1:2,T3:3) = 132.5   DTchoice = -7 (case 2, T1 child T1:4)
```

</details>

<details><summary><b>i=6, j=1: (T1:2, T3:5)</b> → DF = 128 (case 3), DT = 151 (case 2) · MM: <code>T1:3<->- 38 + T1:4<->- 90</code></summary>

```text
--- [L194-195] i = 6, j = 1   cell (T1:2,T3:5) ---
[L196] computeDF(T1:2,T3:5)
|   T1:2 children: T1:3,T1:4   |   T3:5 children: none
|   [L241-247] CASE 1: delete T3:5, put T1:2's forest into one child of T3:5
|   |   [L241-242] minCost = MAX, case1Child = -1
|   |   [L243] T3:5 has no children: loop does not run, minCost stays MAX
|   |   [L247] case1 = DF(-,T3:5) + minCost = 0 + MAX = MAX
|   [L250-256] CASE 2: delete T1:2, put one child of T1:2 onto T3:5's forest
|   |   [L250-251] minCost = MAX, case2Child = -1
|   |   [L252-255] child T1:3: DF(T1:3,T3:5) - DF(T1:3,-) = 0 - 0 = 0  < min -> minCost = 0, case2Child = T1:3
|   |   [L252-255] child T1:4: DF(T1:4,T3:5) - DF(T1:4,-) = 75 - 75 = 0  not smaller
|   |   [L256] case2 = DF(T1:2,-) + minCost = 128 + 0 = 128
|   [L259] CASE 3: case3 = MM(T1:2,T3:5)  (match the two child forests)
|   |   [L294-295] a = #children of T1:2 = 2, b = #children of T3:5 = 0
|   |   [L298] n = a + b = 2 (each side padded with dummies '-' up to n)
|   |   [L302-308] cost matrix, entry [k][l] = DT(row k, col l):
|   |                -         -         
|   |   T1:3         38        38        
|   |   T1:4         90        90        
|   |   [L309] kuhnMunkresMinWeightPerfectMatching(mmCost, 2):
|   |   |   [KM:L29-33] potentials u (rows) = 0, v (cols) = 0; p[col] = row assigned to col (0 = none). Rows/cols are 1-based here.
|   |   |   [KM:L35-39] add row r1(T1:3): p[0] = 1, j0 = 0, minv = INF, used = none
|   |   |   |   [KM:L41-51] mark start used; its row i0 = r1(T1:3). reduced costs cost-u-v for unused cols: c1:38-0-(0)=38->minv 38; c2:38-0-(0)=38->minv 38;
|   |   |   |              smallest minv: delta = 38 at c1(-)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [38,0], v = [0,0]
|   |   |   |   [KM:L56-57] c1(-) is free -> augmenting path found
|   |   |   |   [KM:L58-62] flip the path: c1(-)<-r1(T1:3);
|   |   |   [KM:L35-39] add row r2(T1:4): p[0] = 2, j0 = 0, minv = INF, used = none
|   |   |   |   [KM:L41-51] mark start used; its row i0 = r2(T1:4). reduced costs cost-u-v for unused cols: c1:90-0-(0)=90->minv 90; c2:90-0-(0)=90->minv 90;
|   |   |   |              smallest minv: delta = 90 at c1(-)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [38,90], v = [0,0]
|   |   |   |   [KM:L56-57] c1(-) is already taken by r1(T1:3) -> keep searching from it
|   |   |   |   [KM:L41-51] mark c1(-) used; its row i0 = r1(T1:3). reduced costs cost-u-v for unused cols: c2:38-38-(0)=0->minv 0;
|   |   |   |              smallest minv: delta = 0 at c2(-)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [38,90], v = [0,0]
|   |   |   |   [KM:L56-57] c2(-) is free -> augmenting path found
|   |   |   |   [KM:L58-62] flip the path: c2(-)<-r2(T1:4);
|   |   |   [KM:L65-70] KM RESULT: T1:3<->- 38 + T1:4<->- 90 = total 128
|   |   [L319] MM returns 128
|   [L259] case3 = 128
|   [L262] decide:
|   |   [L225] choose(case1 = MAX, case2 = 128, case3 = 128)
|   |   [L227] case3 - 0.00001 <= case2 ? yes   case3 - 0.00001 <= case1 ? yes
|   |   [L228-229] -> CASE 3 wins, value 128, return CHOICE_MATCH (-1)
|   [L262-263] STORE DF(T1:2,T3:5) = 128   DFchoice = CHOICE_MATCH (case 3)
[L197] computeDT(T1:2,T3:5)
|   [L267-273] CASE 1: delete T3:5, put subtree T1:2 into one child subtree of T3:5
|   |   [L267-268] minCost = MAX, case1Child = -1
|   |   [L269] T3:5 has no children: loop does not run, minCost stays MAX
|   |   [L273] case1 = DT(-,T3:5) + minCost = 15 + MAX = MAX
|   [L275-281] CASE 2: delete T1:2, put one child subtree of T1:2 onto subtree T3:5
|   |   [L275-276] minCost = MAX, case2Child = -1
|   |   [L277-280] child T1:3: DT(T1:3,T3:5) - DT(T1:3,-) = 23 - 38 = -15  < min -> minCost = -15, case2Child = T1:3
|   |   [L277-280] child T1:4: DT(T1:4,T3:5) - DT(T1:4,-) = 75 - 90 = -15  not smaller
|   |   [L281] case2 = DT(T1:2,-) + minCost = 166 + -15 = 151
|   [L283] CASE 3: match T1:2 <-> T3:5: case3 = DF(T1:2,T3:5) + relabelCost(T1:2,T3:5)
|   |   [L151] diff1 = |80 - 30| + |4 - 60| = 106   (move one node onto the other)
|   |   [L152] diff2 = |4 - 80| + |60 - 30| = 106   (delete both)
|   |   [L153] relabelCost = min(106, 106) / 2 = 53
|   [L283] case3 = 128 + 53 = 181
|   [L286] decide:
|   |   [L225] choose(case1 = MAX, case2 = 151, case3 = 181)
|   |   [L227] case3 - 0.00001 <= case2 ? no   case3 - 0.00001 <= case1 ? yes
|   |   [L230,233-235] case1 < case2 ? no -> CASE 2 wins, value 151, return -(0+2) = -2
|   [L286-287] STORE DT(T1:2,T3:5) = 151   DTchoice = -2 (case 2, T1 child T1:3)
```

</details>

<details><summary><b>i=6, j=2: (T1:2, T3:8)</b> → DF = 128 (case 3), DT = 163.5 (case 2) · MM: <code>T1:3<->- 38 + T1:4<->- 90</code></summary>

```text
--- [L194-195] i = 6, j = 2   cell (T1:2,T3:8) ---
[L196] computeDF(T1:2,T3:8)
|   T1:2 children: T1:3,T1:4   |   T3:8 children: none
|   [L241-247] CASE 1: delete T3:8, put T1:2's forest into one child of T3:8
|   |   [L241-242] minCost = MAX, case1Child = -1
|   |   [L243] T3:8 has no children: loop does not run, minCost stays MAX
|   |   [L247] case1 = DF(-,T3:8) + minCost = 0 + MAX = MAX
|   [L250-256] CASE 2: delete T1:2, put one child of T1:2 onto T3:8's forest
|   |   [L250-251] minCost = MAX, case2Child = -1
|   |   [L252-255] child T1:3: DF(T1:3,T3:8) - DF(T1:3,-) = 0 - 0 = 0  < min -> minCost = 0, case2Child = T1:3
|   |   [L252-255] child T1:4: DF(T1:4,T3:8) - DF(T1:4,-) = 75 - 75 = 0  not smaller
|   |   [L256] case2 = DF(T1:2,-) + minCost = 128 + 0 = 128
|   [L259] CASE 3: case3 = MM(T1:2,T3:8)  (match the two child forests)
|   |   [L294-295] a = #children of T1:2 = 2, b = #children of T3:8 = 0
|   |   [L298] n = a + b = 2 (each side padded with dummies '-' up to n)
|   |   [L302-308] cost matrix, entry [k][l] = DT(row k, col l):
|   |                -         -         
|   |   T1:3         38        38        
|   |   T1:4         90        90        
|   |   [L309] kuhnMunkresMinWeightPerfectMatching(mmCost, 2):
|   |   |   [KM:L29-33] potentials u (rows) = 0, v (cols) = 0; p[col] = row assigned to col (0 = none). Rows/cols are 1-based here.
|   |   |   [KM:L35-39] add row r1(T1:3): p[0] = 1, j0 = 0, minv = INF, used = none
|   |   |   |   [KM:L41-51] mark start used; its row i0 = r1(T1:3). reduced costs cost-u-v for unused cols: c1:38-0-(0)=38->minv 38; c2:38-0-(0)=38->minv 38;
|   |   |   |              smallest minv: delta = 38 at c1(-)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [38,0], v = [0,0]
|   |   |   |   [KM:L56-57] c1(-) is free -> augmenting path found
|   |   |   |   [KM:L58-62] flip the path: c1(-)<-r1(T1:3);
|   |   |   [KM:L35-39] add row r2(T1:4): p[0] = 2, j0 = 0, minv = INF, used = none
|   |   |   |   [KM:L41-51] mark start used; its row i0 = r2(T1:4). reduced costs cost-u-v for unused cols: c1:90-0-(0)=90->minv 90; c2:90-0-(0)=90->minv 90;
|   |   |   |              smallest minv: delta = 90 at c1(-)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [38,90], v = [0,0]
|   |   |   |   [KM:L56-57] c1(-) is already taken by r1(T1:3) -> keep searching from it
|   |   |   |   [KM:L41-51] mark c1(-) used; its row i0 = r1(T1:3). reduced costs cost-u-v for unused cols: c2:38-38-(0)=0->minv 0;
|   |   |   |              smallest minv: delta = 0 at c2(-)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [38,90], v = [0,0]
|   |   |   |   [KM:L56-57] c2(-) is free -> augmenting path found
|   |   |   |   [KM:L58-62] flip the path: c2(-)<-r2(T1:4);
|   |   |   [KM:L65-70] KM RESULT: T1:3<->- 38 + T1:4<->- 90 = total 128
|   |   [L319] MM returns 128
|   [L259] case3 = 128
|   [L262] decide:
|   |   [L225] choose(case1 = MAX, case2 = 128, case3 = 128)
|   |   [L227] case3 - 0.00001 <= case2 ? yes   case3 - 0.00001 <= case1 ? yes
|   |   [L228-229] -> CASE 3 wins, value 128, return CHOICE_MATCH (-1)
|   [L262-263] STORE DF(T1:2,T3:8) = 128   DFchoice = CHOICE_MATCH (case 3)
[L197] computeDT(T1:2,T3:8)
|   [L267-273] CASE 1: delete T3:8, put subtree T1:2 into one child subtree of T3:8
|   |   [L267-268] minCost = MAX, case1Child = -1
|   |   [L269] T3:8 has no children: loop does not run, minCost stays MAX
|   |   [L273] case1 = DT(-,T3:8) + minCost = 2.5 + MAX = MAX
|   [L275-281] CASE 2: delete T1:2, put one child subtree of T1:2 onto subtree T3:8
|   |   [L275-276] minCost = MAX, case2Child = -1
|   |   [L277-280] child T1:3: DT(T1:3,T3:8) - DT(T1:3,-) = 35.5 - 38 = -2.5  < min -> minCost = -2.5, case2Child = T1:3
|   |   [L277-280] child T1:4: DT(T1:4,T3:8) - DT(T1:4,-) = 87.5 - 90 = -2.5  not smaller
|   |   [L281] case2 = DT(T1:2,-) + minCost = 166 + -2.5 = 163.5
|   [L283] CASE 3: match T1:2 <-> T3:8: case3 = DF(T1:2,T3:8) + relabelCost(T1:2,T3:8)
|   |   [L151] diff1 = |80 - 5| + |4 - 10| = 81   (move one node onto the other)
|   |   [L152] diff2 = |4 - 80| + |10 - 5| = 81   (delete both)
|   |   [L153] relabelCost = min(81, 81) / 2 = 40.5
|   [L283] case3 = 128 + 40.5 = 168.5
|   [L286] decide:
|   |   [L225] choose(case1 = MAX, case2 = 163.5, case3 = 168.5)
|   |   [L227] case3 - 0.00001 <= case2 ? no   case3 - 0.00001 <= case1 ? yes
|   |   [L230,233-235] case1 < case2 ? no -> CASE 2 wins, value 163.5, return -(0+2) = -2
|   [L286-287] STORE DT(T1:2,T3:8) = 163.5   DTchoice = -2 (case 2, T1 child T1:3)
```

</details>

<details><summary><b>i=6, j=3: (T1:2, T3:9)</b> → DF = 128 (case 3), DT = 118.5 (case 2) · MM: <code>T1:3<->- 38 + T1:4<->- 90</code></summary>

```text
--- [L194-195] i = 6, j = 3   cell (T1:2,T3:9) ---
[L196] computeDF(T1:2,T3:9)
|   T1:2 children: T1:3,T1:4   |   T3:9 children: none
|   [L241-247] CASE 1: delete T3:9, put T1:2's forest into one child of T3:9
|   |   [L241-242] minCost = MAX, case1Child = -1
|   |   [L243] T3:9 has no children: loop does not run, minCost stays MAX
|   |   [L247] case1 = DF(-,T3:9) + minCost = 0 + MAX = MAX
|   [L250-256] CASE 2: delete T1:2, put one child of T1:2 onto T3:9's forest
|   |   [L250-251] minCost = MAX, case2Child = -1
|   |   [L252-255] child T1:3: DF(T1:3,T3:9) - DF(T1:3,-) = 0 - 0 = 0  < min -> minCost = 0, case2Child = T1:3
|   |   [L252-255] child T1:4: DF(T1:4,T3:9) - DF(T1:4,-) = 75 - 75 = 0  not smaller
|   |   [L256] case2 = DF(T1:2,-) + minCost = 128 + 0 = 128
|   [L259] CASE 3: case3 = MM(T1:2,T3:9)  (match the two child forests)
|   |   [L294-295] a = #children of T1:2 = 2, b = #children of T3:9 = 0
|   |   [L298] n = a + b = 2 (each side padded with dummies '-' up to n)
|   |   [L302-308] cost matrix, entry [k][l] = DT(row k, col l):
|   |                -         -         
|   |   T1:3         38        38        
|   |   T1:4         90        90        
|   |   [L309] kuhnMunkresMinWeightPerfectMatching(mmCost, 2):
|   |   |   [KM:L29-33] potentials u (rows) = 0, v (cols) = 0; p[col] = row assigned to col (0 = none). Rows/cols are 1-based here.
|   |   |   [KM:L35-39] add row r1(T1:3): p[0] = 1, j0 = 0, minv = INF, used = none
|   |   |   |   [KM:L41-51] mark start used; its row i0 = r1(T1:3). reduced costs cost-u-v for unused cols: c1:38-0-(0)=38->minv 38; c2:38-0-(0)=38->minv 38;
|   |   |   |              smallest minv: delta = 38 at c1(-)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [38,0], v = [0,0]
|   |   |   |   [KM:L56-57] c1(-) is free -> augmenting path found
|   |   |   |   [KM:L58-62] flip the path: c1(-)<-r1(T1:3);
|   |   |   [KM:L35-39] add row r2(T1:4): p[0] = 2, j0 = 0, minv = INF, used = none
|   |   |   |   [KM:L41-51] mark start used; its row i0 = r2(T1:4). reduced costs cost-u-v for unused cols: c1:90-0-(0)=90->minv 90; c2:90-0-(0)=90->minv 90;
|   |   |   |              smallest minv: delta = 90 at c1(-)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [38,90], v = [0,0]
|   |   |   |   [KM:L56-57] c1(-) is already taken by r1(T1:3) -> keep searching from it
|   |   |   |   [KM:L41-51] mark c1(-) used; its row i0 = r1(T1:3). reduced costs cost-u-v for unused cols: c2:38-38-(0)=0->minv 0;
|   |   |   |              smallest minv: delta = 0 at c2(-)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [38,90], v = [0,0]
|   |   |   |   [KM:L56-57] c2(-) is free -> augmenting path found
|   |   |   |   [KM:L58-62] flip the path: c2(-)<-r2(T1:4);
|   |   |   [KM:L65-70] KM RESULT: T1:3<->- 38 + T1:4<->- 90 = total 128
|   |   [L319] MM returns 128
|   [L259] case3 = 128
|   [L262] decide:
|   |   [L225] choose(case1 = MAX, case2 = 128, case3 = 128)
|   |   [L227] case3 - 0.00001 <= case2 ? yes   case3 - 0.00001 <= case1 ? yes
|   |   [L228-229] -> CASE 3 wins, value 128, return CHOICE_MATCH (-1)
|   [L262-263] STORE DF(T1:2,T3:9) = 128   DFchoice = CHOICE_MATCH (case 3)
[L197] computeDT(T1:2,T3:9)
|   [L267-273] CASE 1: delete T3:9, put subtree T1:2 into one child subtree of T3:9
|   |   [L267-268] minCost = MAX, case1Child = -1
|   |   [L269] T3:9 has no children: loop does not run, minCost stays MAX
|   |   [L273] case1 = DT(-,T3:9) + minCost = 52.5 + MAX = MAX
|   [L275-281] CASE 2: delete T1:2, put one child subtree of T1:2 onto subtree T3:9
|   |   [L275-276] minCost = MAX, case2Child = -1
|   |   [L277-280] child T1:3: DT(T1:3,T3:9) - DT(T1:3,-) = 14.5 - 38 = -23.5  < min -> minCost = -23.5, case2Child = T1:3
|   |   [L277-280] child T1:4: DT(T1:4,T3:9) - DT(T1:4,-) = 42.5 - 90 = -47.5  < min -> minCost = -47.5, case2Child = T1:4
|   |   [L281] case2 = DT(T1:2,-) + minCost = 166 + -47.5 = 118.5
|   [L283] CASE 3: match T1:2 <-> T3:9: case3 = DF(T1:2,T3:9) + relabelCost(T1:2,T3:9)
|   |   [L151] diff1 = |80 - 0| + |4 - 105| = 181   (move one node onto the other)
|   |   [L152] diff2 = |4 - 80| + |105 - 0| = 181   (delete both)
|   |   [L153] relabelCost = min(181, 181) / 2 = 90.5
|   [L283] case3 = 128 + 90.5 = 218.5
|   [L286] decide:
|   |   [L225] choose(case1 = MAX, case2 = 118.5, case3 = 218.5)
|   |   [L227] case3 - 0.00001 <= case2 ? no   case3 - 0.00001 <= case1 ? yes
|   |   [L230,233-235] case1 < case2 ? no -> CASE 2 wins, value 118.5, return -(5+2) = -7
|   [L286-287] STORE DT(T1:2,T3:9) = 118.5   DTchoice = -7 (case 2, T1 child T1:4)
```

</details>

<details><summary><b>i=6, j=4: (T1:2, T3:6)</b> → DF = 78 (case 3), DT = 113.5 (case 3) · MM: <code>T1:3<->T3:8 35.5 + T1:4<->T3:9 42.5 + -<->- 0 + -<->- 0</code></summary>

```text
--- [L194-195] i = 6, j = 4   cell (T1:2,T3:6) ---
[L196] computeDF(T1:2,T3:6)
|   T1:2 children: T1:3,T1:4   |   T3:6 children: T3:8,T3:9
|   [L241-247] CASE 1: delete T3:6, put T1:2's forest into one child of T3:6
|   |   [L241-242] minCost = MAX, case1Child = -1
|   |   [L243-246] child T3:8: DF(T1:2,T3:8) - DF(-,T3:8) = 128 - 0 = 128  < min -> minCost = 128, case1Child = T3:8
|   |   [L243-246] child T3:9: DF(T1:2,T3:9) - DF(-,T3:9) = 128 - 0 = 128  not smaller
|   |   [L247] case1 = DF(-,T3:6) + minCost = 55 + 128 = 183
|   [L250-256] CASE 2: delete T1:2, put one child of T1:2 onto T3:6's forest
|   |   [L250-251] minCost = MAX, case2Child = -1
|   |   [L252-255] child T1:3: DF(T1:3,T3:6) - DF(T1:3,-) = 55 - 0 = 55  < min -> minCost = 55, case2Child = T1:3
|   |   [L252-255] child T1:4: DF(T1:4,T3:6) - DF(T1:4,-) = 25 - 75 = -50  < min -> minCost = -50, case2Child = T1:4
|   |   [L256] case2 = DF(T1:2,-) + minCost = 128 + -50 = 78
|   [L259] CASE 3: case3 = MM(T1:2,T3:6)  (match the two child forests)
|   |   [L294-295] a = #children of T1:2 = 2, b = #children of T3:6 = 2
|   |   [L298] n = a + b = 4 (each side padded with dummies '-' up to n)
|   |   [L302-308] cost matrix, entry [k][l] = DT(row k, col l):
|   |                T3:8      T3:9      -         -         
|   |   T1:3         35.5      14.5      38        38        
|   |   T1:4         87.5      42.5      90        90        
|   |   -            2.5       52.5      0         0         
|   |   -            2.5       52.5      0         0         
|   |   [L309] kuhnMunkresMinWeightPerfectMatching(mmCost, 4):
|   |   |   [KM:L29-33] potentials u (rows) = 0, v (cols) = 0; p[col] = row assigned to col (0 = none). Rows/cols are 1-based here.
|   |   |   [KM:L35-39] add row r1(T1:3): p[0] = 1, j0 = 0, minv = INF, used = none
|   |   |   |   [KM:L41-51] mark start used; its row i0 = r1(T1:3). reduced costs cost-u-v for unused cols: c1:35.5-0-(0)=35.5->minv 35.5; c2:14.5-0-(0)=14.5->minv 14.5; c3:38-0-(0)=38->minv 38; c4:38-0-(0)=38->minv 38;
|   |   |   |              smallest minv: delta = 14.5 at c2(T3:9)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [14.5,0,0,0], v = [0,0,0,0]
|   |   |   |   [KM:L56-57] c2(T3:9) is free -> augmenting path found
|   |   |   |   [KM:L58-62] flip the path: c2(T3:9)<-r1(T1:3);
|   |   |   [KM:L35-39] add row r2(T1:4): p[0] = 2, j0 = 0, minv = INF, used = none
|   |   |   |   [KM:L41-51] mark start used; its row i0 = r2(T1:4). reduced costs cost-u-v for unused cols: c1:87.5-0-(0)=87.5->minv 87.5; c2:42.5-0-(0)=42.5->minv 42.5; c3:90-0-(0)=90->minv 90; c4:90-0-(0)=90->minv 90;
|   |   |   |              smallest minv: delta = 42.5 at c2(T3:9)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [14.5,42.5,0,0], v = [0,0,0,0]
|   |   |   |   [KM:L56-57] c2(T3:9) is already taken by r1(T1:3) -> keep searching from it
|   |   |   |   [KM:L41-51] mark c2(T3:9) used; its row i0 = r1(T1:3). reduced costs cost-u-v for unused cols: c1:35.5-14.5-(0)=21->minv 21; c3:38-14.5-(0)=23.5->minv 23.5; c4:38-14.5-(0)=23.5->minv 23.5;
|   |   |   |              smallest minv: delta = 21 at c1(T3:8)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [35.5,63.5,0,0], v = [0,-21,0,0]
|   |   |   |   [KM:L56-57] c1(T3:8) is free -> augmenting path found
|   |   |   |   [KM:L58-62] flip the path: c1(T3:8)<-r1(T1:3); c2(T3:9)<-r2(T1:4);
|   |   |   [KM:L35-39] add row r3(-): p[0] = 3, j0 = 0, minv = INF, used = none
|   |   |   |   [KM:L41-51] mark start used; its row i0 = r3(-). reduced costs cost-u-v for unused cols: c1:2.5-0-(0)=2.5->minv 2.5; c2:52.5-0-(-21)=73.5->minv 73.5; c3:0-0-(0)=0->minv 0; c4:0-0-(0)=0->minv 0;
|   |   |   |              smallest minv: delta = 0 at c3(-)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [35.5,63.5,0,0], v = [0,-21,0,0]
|   |   |   |   [KM:L56-57] c3(-) is free -> augmenting path found
|   |   |   |   [KM:L58-62] flip the path: c3(-)<-r3(-);
|   |   |   [KM:L35-39] add row r4(-): p[0] = 4, j0 = 0, minv = INF, used = none
|   |   |   |   [KM:L41-51] mark start used; its row i0 = r4(-). reduced costs cost-u-v for unused cols: c1:2.5-0-(0)=2.5->minv 2.5; c2:52.5-0-(-21)=73.5->minv 73.5; c3:0-0-(0)=0->minv 0; c4:0-0-(0)=0->minv 0;
|   |   |   |              smallest minv: delta = 0 at c3(-)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [35.5,63.5,0,0], v = [0,-21,0,0]
|   |   |   |   [KM:L56-57] c3(-) is already taken by r3(-) -> keep searching from it
|   |   |   |   [KM:L41-51] mark c3(-) used; its row i0 = r3(-). reduced costs cost-u-v for unused cols: c1:2.5-0-(0)=2.5->minv 2.5; c2:52.5-0-(-21)=73.5->minv 73.5; c4:0-0-(0)=0->minv 0;
|   |   |   |              smallest minv: delta = 0 at c4(-)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [35.5,63.5,0,0], v = [0,-21,0,0]
|   |   |   |   [KM:L56-57] c4(-) is free -> augmenting path found
|   |   |   |   [KM:L58-62] flip the path: c4(-)<-r4(-);
|   |   |   [KM:L65-70] KM RESULT: T1:3<->T3:8 35.5 + T1:4<->T3:9 42.5 + -<->- 0 + -<->- 0 = total 78
|   |   [L319] MM returns 78
|   [L259] case3 = 78
|   [L262] decide:
|   |   [L225] choose(case1 = 183, case2 = 78, case3 = 78)
|   |   [L227] case3 - 0.00001 <= case2 ? yes   case3 - 0.00001 <= case1 ? yes
|   |   [L228-229] -> CASE 3 wins, value 78, return CHOICE_MATCH (-1)
|   [L262-263] STORE DF(T1:2,T3:6) = 78   DFchoice = CHOICE_MATCH (case 3)
[L197] computeDT(T1:2,T3:6)
|   [L267-273] CASE 1: delete T3:6, put subtree T1:2 into one child subtree of T3:6
|   |   [L267-268] minCost = MAX, case1Child = -1
|   |   [L269-272] child T3:8: DT(T1:2,T3:8) - DT(-,T3:8) = 163.5 - 2.5 = 161  < min -> minCost = 161, case1Child = T3:8
|   |   [L269-272] child T3:9: DT(T1:2,T3:9) - DT(-,T3:9) = 118.5 - 52.5 = 66  < min -> minCost = 66, case1Child = T3:9
|   |   [L273] case1 = DT(-,T3:6) + minCost = 57.5 + 66 = 123.5
|   [L275-281] CASE 2: delete T1:2, put one child subtree of T1:2 onto subtree T3:6
|   |   [L275-276] minCost = MAX, case2Child = -1
|   |   [L277-280] child T1:3: DT(T1:3,T3:6) - DT(T1:3,-) = 19.5 - 38 = -18.5  < min -> minCost = -18.5, case2Child = T1:3
|   |   [L277-280] child T1:4: DT(T1:4,T3:6) - DT(T1:4,-) = 37.5 - 90 = -52.5  < min -> minCost = -52.5, case2Child = T1:4
|   |   [L281] case2 = DT(T1:2,-) + minCost = 166 + -52.5 = 113.5
|   [L283] CASE 3: match T1:2 <-> T3:6: case3 = DF(T1:2,T3:6) + relabelCost(T1:2,T3:6)
|   |   [L151] diff1 = |80 - 10| + |4 - 5| = 71   (move one node onto the other)
|   |   [L152] diff2 = |4 - 80| + |5 - 10| = 81   (delete both)
|   |   [L153] relabelCost = min(71, 81) / 2 = 35.5
|   [L283] case3 = 78 + 35.5 = 113.5
|   [L286] decide:
|   |   [L225] choose(case1 = 123.5, case2 = 113.5, case3 = 113.5)
|   |   [L227] case3 - 0.00001 <= case2 ? yes   case3 - 0.00001 <= case1 ? yes
|   |   [L228-229] -> CASE 3 wins, value 113.5, return CHOICE_MATCH (-1)
|   [L286-287] STORE DT(T1:2,T3:6) = 113.5   DTchoice = CHOICE_MATCH (case 3)
```

</details>

<details><summary><b>i=6, j=5: (T1:2, T3:4)</b> → DF = 60.5 (case 3), DT = 83.5 (case 3) · MM: <code>T1:3<->T3:5 23 + T1:4<->T3:6 37.5 + -<->- 0 + -<->- 0</code></summary>

```text
--- [L194-195] i = 6, j = 5   cell (T1:2,T3:4) ---
[L196] computeDF(T1:2,T3:4)
|   T1:2 children: T1:3,T1:4   |   T3:4 children: T3:5,T3:6
|   [L241-247] CASE 1: delete T3:4, put T1:2's forest into one child of T3:4
|   |   [L241-242] minCost = MAX, case1Child = -1
|   |   [L243-246] child T3:5: DF(T1:2,T3:5) - DF(-,T3:5) = 128 - 0 = 128  < min -> minCost = 128, case1Child = T3:5
|   |   [L243-246] child T3:6: DF(T1:2,T3:6) - DF(-,T3:6) = 78 - 55 = 23  < min -> minCost = 23, case1Child = T3:6
|   |   [L247] case1 = DF(-,T3:4) + minCost = 72.5 + 23 = 95.5
|   [L250-256] CASE 2: delete T1:2, put one child of T1:2 onto T3:4's forest
|   |   [L250-251] minCost = MAX, case2Child = -1
|   |   [L252-255] child T1:3: DF(T1:3,T3:4) - DF(T1:3,-) = 72.5 - 0 = 72.5  < min -> minCost = 72.5, case2Child = T1:3
|   |   [L252-255] child T1:4: DF(T1:4,T3:4) - DF(T1:4,-) = 7.5 - 75 = -67.5  < min -> minCost = -67.5, case2Child = T1:4
|   |   [L256] case2 = DF(T1:2,-) + minCost = 128 + -67.5 = 60.5
|   [L259] CASE 3: case3 = MM(T1:2,T3:4)  (match the two child forests)
|   |   [L294-295] a = #children of T1:2 = 2, b = #children of T3:4 = 2
|   |   [L298] n = a + b = 4 (each side padded with dummies '-' up to n)
|   |   [L302-308] cost matrix, entry [k][l] = DT(row k, col l):
|   |                T3:5      T3:6      -         -         
|   |   T1:3         23        19.5      38        38        
|   |   T1:4         75        37.5      90        90        
|   |   -            15        57.5      0         0         
|   |   -            15        57.5      0         0         
|   |   [L309] kuhnMunkresMinWeightPerfectMatching(mmCost, 4):
|   |   |   [KM:L29-33] potentials u (rows) = 0, v (cols) = 0; p[col] = row assigned to col (0 = none). Rows/cols are 1-based here.
|   |   |   [KM:L35-39] add row r1(T1:3): p[0] = 1, j0 = 0, minv = INF, used = none
|   |   |   |   [KM:L41-51] mark start used; its row i0 = r1(T1:3). reduced costs cost-u-v for unused cols: c1:23-0-(0)=23->minv 23; c2:19.5-0-(0)=19.5->minv 19.5; c3:38-0-(0)=38->minv 38; c4:38-0-(0)=38->minv 38;
|   |   |   |              smallest minv: delta = 19.5 at c2(T3:6)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [19.5,0,0,0], v = [0,0,0,0]
|   |   |   |   [KM:L56-57] c2(T3:6) is free -> augmenting path found
|   |   |   |   [KM:L58-62] flip the path: c2(T3:6)<-r1(T1:3);
|   |   |   [KM:L35-39] add row r2(T1:4): p[0] = 2, j0 = 0, minv = INF, used = none
|   |   |   |   [KM:L41-51] mark start used; its row i0 = r2(T1:4). reduced costs cost-u-v for unused cols: c1:75-0-(0)=75->minv 75; c2:37.5-0-(0)=37.5->minv 37.5; c3:90-0-(0)=90->minv 90; c4:90-0-(0)=90->minv 90;
|   |   |   |              smallest minv: delta = 37.5 at c2(T3:6)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [19.5,37.5,0,0], v = [0,0,0,0]
|   |   |   |   [KM:L56-57] c2(T3:6) is already taken by r1(T1:3) -> keep searching from it
|   |   |   |   [KM:L41-51] mark c2(T3:6) used; its row i0 = r1(T1:3). reduced costs cost-u-v for unused cols: c1:23-19.5-(0)=3.5->minv 3.5; c3:38-19.5-(0)=18.5->minv 18.5; c4:38-19.5-(0)=18.5->minv 18.5;
|   |   |   |              smallest minv: delta = 3.5 at c1(T3:5)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [23,41,0,0], v = [0,-3.5,0,0]
|   |   |   |   [KM:L56-57] c1(T3:5) is free -> augmenting path found
|   |   |   |   [KM:L58-62] flip the path: c1(T3:5)<-r1(T1:3); c2(T3:6)<-r2(T1:4);
|   |   |   [KM:L35-39] add row r3(-): p[0] = 3, j0 = 0, minv = INF, used = none
|   |   |   |   [KM:L41-51] mark start used; its row i0 = r3(-). reduced costs cost-u-v for unused cols: c1:15-0-(0)=15->minv 15; c2:57.5-0-(-3.5)=61->minv 61; c3:0-0-(0)=0->minv 0; c4:0-0-(0)=0->minv 0;
|   |   |   |              smallest minv: delta = 0 at c3(-)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [23,41,0,0], v = [0,-3.5,0,0]
|   |   |   |   [KM:L56-57] c3(-) is free -> augmenting path found
|   |   |   |   [KM:L58-62] flip the path: c3(-)<-r3(-);
|   |   |   [KM:L35-39] add row r4(-): p[0] = 4, j0 = 0, minv = INF, used = none
|   |   |   |   [KM:L41-51] mark start used; its row i0 = r4(-). reduced costs cost-u-v for unused cols: c1:15-0-(0)=15->minv 15; c2:57.5-0-(-3.5)=61->minv 61; c3:0-0-(0)=0->minv 0; c4:0-0-(0)=0->minv 0;
|   |   |   |              smallest minv: delta = 0 at c3(-)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [23,41,0,0], v = [0,-3.5,0,0]
|   |   |   |   [KM:L56-57] c3(-) is already taken by r3(-) -> keep searching from it
|   |   |   |   [KM:L41-51] mark c3(-) used; its row i0 = r3(-). reduced costs cost-u-v for unused cols: c1:15-0-(0)=15->minv 15; c2:57.5-0-(-3.5)=61->minv 61; c4:0-0-(0)=0->minv 0;
|   |   |   |              smallest minv: delta = 0 at c4(-)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [23,41,0,0], v = [0,-3.5,0,0]
|   |   |   |   [KM:L56-57] c4(-) is free -> augmenting path found
|   |   |   |   [KM:L58-62] flip the path: c4(-)<-r4(-);
|   |   |   [KM:L65-70] KM RESULT: T1:3<->T3:5 23 + T1:4<->T3:6 37.5 + -<->- 0 + -<->- 0 = total 60.5
|   |   [L319] MM returns 60.5
|   [L259] case3 = 60.5
|   [L262] decide:
|   |   [L225] choose(case1 = 95.5, case2 = 60.5, case3 = 60.5)
|   |   [L227] case3 - 0.00001 <= case2 ? yes   case3 - 0.00001 <= case1 ? yes
|   |   [L228-229] -> CASE 3 wins, value 60.5, return CHOICE_MATCH (-1)
|   [L262-263] STORE DF(T1:2,T3:4) = 60.5   DFchoice = CHOICE_MATCH (case 3)
[L197] computeDT(T1:2,T3:4)
|   [L267-273] CASE 1: delete T3:4, put subtree T1:2 into one child subtree of T3:4
|   |   [L267-268] minCost = MAX, case1Child = -1
|   |   [L269-272] child T3:5: DT(T1:2,T3:5) - DT(-,T3:5) = 151 - 15 = 136  < min -> minCost = 136, case1Child = T3:5
|   |   [L269-272] child T3:6: DT(T1:2,T3:6) - DT(-,T3:6) = 113.5 - 57.5 = 56  < min -> minCost = 56, case1Child = T3:6
|   |   [L273] case1 = DT(-,T3:4) + minCost = 87.5 + 56 = 143.5
|   [L275-281] CASE 2: delete T1:2, put one child subtree of T1:2 onto subtree T3:4
|   |   [L275-276] minCost = MAX, case2Child = -1
|   |   [L277-280] child T1:3: DT(T1:3,T3:4) - DT(T1:3,-) = 49.5 - 38 = 11.5  < min -> minCost = 11.5, case2Child = T1:3
|   |   [L277-280] child T1:4: DT(T1:4,T3:4) - DT(T1:4,-) = 7.5 - 90 = -82.5  < min -> minCost = -82.5, case2Child = T1:4
|   |   [L281] case2 = DT(T1:2,-) + minCost = 166 + -82.5 = 83.5
|   [L283] CASE 3: match T1:2 <-> T3:4: case3 = DF(T1:2,T3:4) + relabelCost(T1:2,T3:4)
|   |   [L151] diff1 = |80 - 60| + |4 - 30| = 46   (move one node onto the other)
|   |   [L152] diff2 = |4 - 80| + |30 - 60| = 106   (delete both)
|   |   [L153] relabelCost = min(46, 106) / 2 = 23
|   [L283] case3 = 60.5 + 23 = 83.5
|   [L286] decide:
|   |   [L225] choose(case1 = 143.5, case2 = 83.5, case3 = 83.5)
|   |   [L227] case3 - 0.00001 <= case2 ? yes   case3 - 0.00001 <= case1 ? yes
|   |   [L228-229] -> CASE 3 wins, value 83.5, return CHOICE_MATCH (-1)
|   [L286-287] STORE DT(T1:2,T3:4) = 83.5   DTchoice = CHOICE_MATCH (case 3)
```

</details>

<details><summary><b>i=6, j=6: (T1:2, T3:2)</b> → DF = 13 (case 3), DT = 18.5 (case 3) · MM: <code>T1:3<->T3:3 5.5 + T1:4<->T3:4 7.5 + -<->- 0 + -<->- 0</code></summary>

```text
--- [L194-195] i = 6, j = 6   cell (T1:2,T3:2) ---
[L196] computeDF(T1:2,T3:2)
|   T1:2 children: T1:3,T1:4   |   T3:2 children: T3:3,T3:4
|   [L241-247] CASE 1: delete T3:2, put T1:2's forest into one child of T3:2
|   |   [L241-242] minCost = MAX, case1Child = -1
|   |   [L243-246] child T3:3: DF(T1:2,T3:3) - DF(-,T3:3) = 128 - 0 = 128  < min -> minCost = 128, case1Child = T3:3
|   |   [L243-246] child T3:4: DF(T1:2,T3:4) - DF(-,T3:4) = 60.5 - 72.5 = -12  < min -> minCost = -12, case1Child = T3:4
|   |   [L247] case1 = DF(-,T3:2) + minCost = 121 + -12 = 109
|   [L250-256] CASE 2: delete T1:2, put one child of T1:2 onto T3:2's forest
|   |   [L250-251] minCost = MAX, case2Child = -1
|   |   [L252-255] child T1:3: DF(T1:3,T3:2) - DF(T1:3,-) = 121 - 0 = 121  < min -> minCost = 121, case2Child = T1:3
|   |   [L252-255] child T1:4: DF(T1:4,T3:2) - DF(T1:4,-) = 56 - 75 = -19  < min -> minCost = -19, case2Child = T1:4
|   |   [L256] case2 = DF(T1:2,-) + minCost = 128 + -19 = 109
|   [L259] CASE 3: case3 = MM(T1:2,T3:2)  (match the two child forests)
|   |   [L294-295] a = #children of T1:2 = 2, b = #children of T3:2 = 2
|   |   [L298] n = a + b = 4 (each side padded with dummies '-' up to n)
|   |   [L302-308] cost matrix, entry [k][l] = DT(row k, col l):
|   |                T3:3      T3:4      -         -         
|   |   T1:3         5.5       49.5      38        38        
|   |   T1:4         56.5      7.5       90        90        
|   |   -            33.5      87.5      0         0         
|   |   -            33.5      87.5      0         0         
|   |   [L309] kuhnMunkresMinWeightPerfectMatching(mmCost, 4):
|   |   |   [KM:L29-33] potentials u (rows) = 0, v (cols) = 0; p[col] = row assigned to col (0 = none). Rows/cols are 1-based here.
|   |   |   [KM:L35-39] add row r1(T1:3): p[0] = 1, j0 = 0, minv = INF, used = none
|   |   |   |   [KM:L41-51] mark start used; its row i0 = r1(T1:3). reduced costs cost-u-v for unused cols: c1:5.5-0-(0)=5.5->minv 5.5; c2:49.5-0-(0)=49.5->minv 49.5; c3:38-0-(0)=38->minv 38; c4:38-0-(0)=38->minv 38;
|   |   |   |              smallest minv: delta = 5.5 at c1(T3:3)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [5.5,0,0,0], v = [0,0,0,0]
|   |   |   |   [KM:L56-57] c1(T3:3) is free -> augmenting path found
|   |   |   |   [KM:L58-62] flip the path: c1(T3:3)<-r1(T1:3);
|   |   |   [KM:L35-39] add row r2(T1:4): p[0] = 2, j0 = 0, minv = INF, used = none
|   |   |   |   [KM:L41-51] mark start used; its row i0 = r2(T1:4). reduced costs cost-u-v for unused cols: c1:56.5-0-(0)=56.5->minv 56.5; c2:7.5-0-(0)=7.5->minv 7.5; c3:90-0-(0)=90->minv 90; c4:90-0-(0)=90->minv 90;
|   |   |   |              smallest minv: delta = 7.5 at c2(T3:4)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [5.5,7.5,0,0], v = [0,0,0,0]
|   |   |   |   [KM:L56-57] c2(T3:4) is free -> augmenting path found
|   |   |   |   [KM:L58-62] flip the path: c2(T3:4)<-r2(T1:4);
|   |   |   [KM:L35-39] add row r3(-): p[0] = 3, j0 = 0, minv = INF, used = none
|   |   |   |   [KM:L41-51] mark start used; its row i0 = r3(-). reduced costs cost-u-v for unused cols: c1:33.5-0-(0)=33.5->minv 33.5; c2:87.5-0-(0)=87.5->minv 87.5; c3:0-0-(0)=0->minv 0; c4:0-0-(0)=0->minv 0;
|   |   |   |              smallest minv: delta = 0 at c3(-)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [5.5,7.5,0,0], v = [0,0,0,0]
|   |   |   |   [KM:L56-57] c3(-) is free -> augmenting path found
|   |   |   |   [KM:L58-62] flip the path: c3(-)<-r3(-);
|   |   |   [KM:L35-39] add row r4(-): p[0] = 4, j0 = 0, minv = INF, used = none
|   |   |   |   [KM:L41-51] mark start used; its row i0 = r4(-). reduced costs cost-u-v for unused cols: c1:33.5-0-(0)=33.5->minv 33.5; c2:87.5-0-(0)=87.5->minv 87.5; c3:0-0-(0)=0->minv 0; c4:0-0-(0)=0->minv 0;
|   |   |   |              smallest minv: delta = 0 at c3(-)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [5.5,7.5,0,0], v = [0,0,0,0]
|   |   |   |   [KM:L56-57] c3(-) is already taken by r3(-) -> keep searching from it
|   |   |   |   [KM:L41-51] mark c3(-) used; its row i0 = r3(-). reduced costs cost-u-v for unused cols: c1:33.5-0-(0)=33.5->minv 33.5; c2:87.5-0-(0)=87.5->minv 87.5; c4:0-0-(0)=0->minv 0;
|   |   |   |              smallest minv: delta = 0 at c4(-)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [5.5,7.5,0,0], v = [0,0,0,0]
|   |   |   |   [KM:L56-57] c4(-) is free -> augmenting path found
|   |   |   |   [KM:L58-62] flip the path: c4(-)<-r4(-);
|   |   |   [KM:L65-70] KM RESULT: T1:3<->T3:3 5.5 + T1:4<->T3:4 7.5 + -<->- 0 + -<->- 0 = total 13
|   |   [L319] MM returns 13
|   [L259] case3 = 13
|   [L262] decide:
|   |   [L225] choose(case1 = 109, case2 = 109, case3 = 13)
|   |   [L227] case3 - 0.00001 <= case2 ? yes   case3 - 0.00001 <= case1 ? yes
|   |   [L228-229] -> CASE 3 wins, value 13, return CHOICE_MATCH (-1)
|   [L262-263] STORE DF(T1:2,T3:2) = 13   DFchoice = CHOICE_MATCH (case 3)
[L197] computeDT(T1:2,T3:2)
|   [L267-273] CASE 1: delete T3:2, put subtree T1:2 into one child subtree of T3:2
|   |   [L267-268] minCost = MAX, case1Child = -1
|   |   [L269-272] child T3:3: DT(T1:2,T3:3) - DT(-,T3:3) = 132.5 - 33.5 = 99  < min -> minCost = 99, case1Child = T3:3
|   |   [L269-272] child T3:4: DT(T1:2,T3:4) - DT(-,T3:4) = 83.5 - 87.5 = -4  < min -> minCost = -4, case1Child = T3:4
|   |   [L273] case1 = DT(-,T3:2) + minCost = 154.5 + -4 = 150.5
|   [L275-281] CASE 2: delete T1:2, put one child subtree of T1:2 onto subtree T3:2
|   |   [L275-276] minCost = MAX, case2Child = -1
|   |   [L277-280] child T1:3: DT(T1:3,T3:2) - DT(T1:3,-) = 116.5 - 38 = 78.5  < min -> minCost = 78.5, case2Child = T1:3
|   |   [L277-280] child T1:4: DT(T1:4,T3:2) - DT(T1:4,-) = 74.5 - 90 = -15.5  < min -> minCost = -15.5, case2Child = T1:4
|   |   [L281] case2 = DT(T1:2,-) + minCost = 166 + -15.5 = 150.5
|   [L283] CASE 3: match T1:2 <-> T3:2: case3 = DF(T1:2,T3:2) + relabelCost(T1:2,T3:2)
|   |   [L151] diff1 = |80 - 70| + |4 - 3| = 11   (move one node onto the other)
|   |   [L152] diff2 = |4 - 80| + |3 - 70| = 143   (delete both)
|   |   [L153] relabelCost = min(11, 143) / 2 = 5.5
|   [L283] case3 = 13 + 5.5 = 18.5
|   [L286] decide:
|   |   [L225] choose(case1 = 150.5, case2 = 150.5, case3 = 18.5)
|   |   [L227] case3 - 0.00001 <= case2 ? yes   case3 - 0.00001 <= case1 ? yes
|   |   [L228-229] -> CASE 3 wins, value 18.5, return CHOICE_MATCH (-1)
|   [L286-287] STORE DT(T1:2,T3:2) = 18.5   DTchoice = CHOICE_MATCH (case 3)
```

</details>

<details><summary><b>i=6, j=7: (T1:2, T3:1)</b> → DF = 46.5 (case 1), DT = 61 (case 3) · MM: <code>T1:3<->- 38 + T1:4<->T3:2 74.5 + -<->- 0</code></summary>

```text
--- [L194-195] i = 6, j = 7   cell (T1:2,T3:1) ---
[L196] computeDF(T1:2,T3:1)
|   T1:2 children: T1:3,T1:4   |   T3:1 children: T3:2
|   [L241-247] CASE 1: delete T3:1, put T1:2's forest into one child of T3:1
|   |   [L241-242] minCost = MAX, case1Child = -1
|   |   [L243-246] child T3:2: DF(T1:2,T3:2) - DF(-,T3:2) = 13 - 121 = -108  < min -> minCost = -108, case1Child = T3:2
|   |   [L247] case1 = DF(-,T3:1) + minCost = 154.5 + -108 = 46.5
|   [L250-256] CASE 2: delete T1:2, put one child of T1:2 onto T3:1's forest
|   |   [L250-251] minCost = MAX, case2Child = -1
|   |   [L252-255] child T1:3: DF(T1:3,T3:1) - DF(T1:3,-) = 154.5 - 0 = 154.5  < min -> minCost = 154.5, case2Child = T1:3
|   |   [L252-255] child T1:4: DF(T1:4,T3:1) - DF(T1:4,-) = 89.5 - 75 = 14.5  < min -> minCost = 14.5, case2Child = T1:4
|   |   [L256] case2 = DF(T1:2,-) + minCost = 128 + 14.5 = 142.5
|   [L259] CASE 3: case3 = MM(T1:2,T3:1)  (match the two child forests)
|   |   [L294-295] a = #children of T1:2 = 2, b = #children of T3:1 = 1
|   |   [L298] n = a + b = 3 (each side padded with dummies '-' up to n)
|   |   [L302-308] cost matrix, entry [k][l] = DT(row k, col l):
|   |                T3:2      -         -         
|   |   T1:3         116.5     38        38        
|   |   T1:4         74.5      90        90        
|   |   -            154.5     0         0         
|   |   [L309] kuhnMunkresMinWeightPerfectMatching(mmCost, 3):
|   |   |   [KM:L29-33] potentials u (rows) = 0, v (cols) = 0; p[col] = row assigned to col (0 = none). Rows/cols are 1-based here.
|   |   |   [KM:L35-39] add row r1(T1:3): p[0] = 1, j0 = 0, minv = INF, used = none
|   |   |   |   [KM:L41-51] mark start used; its row i0 = r1(T1:3). reduced costs cost-u-v for unused cols: c1:116.5-0-(0)=116.5->minv 116.5; c2:38-0-(0)=38->minv 38; c3:38-0-(0)=38->minv 38;
|   |   |   |              smallest minv: delta = 38 at c2(-)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [38,0,0], v = [0,0,0]
|   |   |   |   [KM:L56-57] c2(-) is free -> augmenting path found
|   |   |   |   [KM:L58-62] flip the path: c2(-)<-r1(T1:3);
|   |   |   [KM:L35-39] add row r2(T1:4): p[0] = 2, j0 = 0, minv = INF, used = none
|   |   |   |   [KM:L41-51] mark start used; its row i0 = r2(T1:4). reduced costs cost-u-v for unused cols: c1:74.5-0-(0)=74.5->minv 74.5; c2:90-0-(0)=90->minv 90; c3:90-0-(0)=90->minv 90;
|   |   |   |              smallest minv: delta = 74.5 at c1(T3:2)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [38,74.5,0], v = [0,0,0]
|   |   |   |   [KM:L56-57] c1(T3:2) is free -> augmenting path found
|   |   |   |   [KM:L58-62] flip the path: c1(T3:2)<-r2(T1:4);
|   |   |   [KM:L35-39] add row r3(-): p[0] = 3, j0 = 0, minv = INF, used = none
|   |   |   |   [KM:L41-51] mark start used; its row i0 = r3(-). reduced costs cost-u-v for unused cols: c1:154.5-0-(0)=154.5->minv 154.5; c2:0-0-(0)=0->minv 0; c3:0-0-(0)=0->minv 0;
|   |   |   |              smallest minv: delta = 0 at c2(-)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [38,74.5,0], v = [0,0,0]
|   |   |   |   [KM:L56-57] c2(-) is already taken by r1(T1:3) -> keep searching from it
|   |   |   |   [KM:L41-51] mark c2(-) used; its row i0 = r1(T1:3). reduced costs cost-u-v for unused cols: c1:116.5-38-(0)=78.5->minv 78.5; c3:38-38-(0)=0->minv 0;
|   |   |   |              smallest minv: delta = 0 at c3(-)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [38,74.5,0], v = [0,0,0]
|   |   |   |   [KM:L56-57] c3(-) is free -> augmenting path found
|   |   |   |   [KM:L58-62] flip the path: c3(-)<-r3(-);
|   |   |   [KM:L65-70] KM RESULT: T1:3<->- 38 + T1:4<->T3:2 74.5 + -<->- 0 = total 112.5
|   |   [L319] MM returns 112.5
|   [L259] case3 = 112.5
|   [L262] decide:
|   |   [L225] choose(case1 = 46.5, case2 = 142.5, case3 = 112.5)
|   |   [L227] case3 - 0.00001 <= case2 ? yes   case3 - 0.00001 <= case1 ? no
|   |   [L230-232] case1 < case2 ? yes -> CASE 1 wins, value 46.5, return child index 6
|   [L262-263] STORE DF(T1:2,T3:1) = 46.5   DFchoice = 6 (case 1, T3 child T3:2)
[L197] computeDT(T1:2,T3:1)
|   [L267-273] CASE 1: delete T3:1, put subtree T1:2 into one child subtree of T3:1
|   |   [L267-268] minCost = MAX, case1Child = -1
|   |   [L269-272] child T3:2: DT(T1:2,T3:2) - DT(-,T3:2) = 18.5 - 154.5 = -136  < min -> minCost = -136, case1Child = T3:2
|   |   [L273] case1 = DT(-,T3:1) + minCost = 207 + -136 = 71
|   [L275-281] CASE 2: delete T1:2, put one child subtree of T1:2 onto subtree T3:1
|   |   [L275-276] minCost = MAX, case2Child = -1
|   |   [L277-280] child T1:3: DT(T1:3,T3:1) - DT(T1:3,-) = 169 - 38 = 131  < min -> minCost = 131, case2Child = T1:3
|   |   [L277-280] child T1:4: DT(T1:4,T3:1) - DT(T1:4,-) = 127 - 90 = 37  < min -> minCost = 37, case2Child = T1:4
|   |   [L281] case2 = DT(T1:2,-) + minCost = 166 + 37 = 203
|   [L283] CASE 3: match T1:2 <-> T3:1: case3 = DF(T1:2,T3:1) + relabelCost(T1:2,T3:1)
|   |   [L151] diff1 = |80 - 105| + |4 - 0| = 29   (move one node onto the other)
|   |   [L152] diff2 = |4 - 80| + |0 - 105| = 181   (delete both)
|   |   [L153] relabelCost = min(29, 181) / 2 = 14.5
|   [L283] case3 = 46.5 + 14.5 = 61
|   [L286] decide:
|   |   [L225] choose(case1 = 71, case2 = 203, case3 = 61)
|   |   [L227] case3 - 0.00001 <= case2 ? yes   case3 - 0.00001 <= case1 ? yes
|   |   [L228-229] -> CASE 3 wins, value 61, return CHOICE_MATCH (-1)
|   [L286-287] STORE DT(T1:2,T3:1) = 61   DTchoice = CHOICE_MATCH (case 3)
```

</details>

<details><summary><b>i=7, j=0: (T1:1, T3:3)</b> → DF = 166 (case 3), DT = 182.5 (case 2) · MM: <code>T1:2<->- 166</code></summary>

```text
--- [L194-195] i = 7, j = 0   cell (T1:1,T3:3) ---
[L196] computeDF(T1:1,T3:3)
|   T1:1 children: T1:2   |   T3:3 children: none
|   [L241-247] CASE 1: delete T3:3, put T1:1's forest into one child of T3:3
|   |   [L241-242] minCost = MAX, case1Child = -1
|   |   [L243] T3:3 has no children: loop does not run, minCost stays MAX
|   |   [L247] case1 = DF(-,T3:3) + minCost = 0 + MAX = MAX
|   [L250-256] CASE 2: delete T1:1, put one child of T1:1 onto T3:3's forest
|   |   [L250-251] minCost = MAX, case2Child = -1
|   |   [L252-255] child T1:2: DF(T1:2,T3:3) - DF(T1:2,-) = 128 - 128 = 0  < min -> minCost = 0, case2Child = T1:2
|   |   [L256] case2 = DF(T1:1,-) + minCost = 166 + 0 = 166
|   [L259] CASE 3: case3 = MM(T1:1,T3:3)  (match the two child forests)
|   |   [L294-295] a = #children of T1:1 = 1, b = #children of T3:3 = 0
|   |   [L298] n = a + b = 1 (each side padded with dummies '-' up to n)
|   |   [L302-308] cost matrix, entry [k][l] = DT(row k, col l):
|   |                -         
|   |   T1:2         166       
|   |   [L309] kuhnMunkresMinWeightPerfectMatching(mmCost, 1):
|   |   |   [KM:L29-33] potentials u (rows) = 0, v (cols) = 0; p[col] = row assigned to col (0 = none). Rows/cols are 1-based here.
|   |   |   [KM:L35-39] add row r1(T1:2): p[0] = 1, j0 = 0, minv = INF, used = none
|   |   |   |   [KM:L41-51] mark start used; its row i0 = r1(T1:2). reduced costs cost-u-v for unused cols: c1:166-0-(0)=166->minv 166;
|   |   |   |              smallest minv: delta = 166 at c1(-)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [166], v = [0]
|   |   |   |   [KM:L56-57] c1(-) is free -> augmenting path found
|   |   |   |   [KM:L58-62] flip the path: c1(-)<-r1(T1:2);
|   |   |   [KM:L65-70] KM RESULT: T1:2<->- 166 = total 166
|   |   [L319] MM returns 166
|   [L259] case3 = 166
|   [L262] decide:
|   |   [L225] choose(case1 = MAX, case2 = 166, case3 = 166)
|   |   [L227] case3 - 0.00001 <= case2 ? yes   case3 - 0.00001 <= case1 ? yes
|   |   [L228-229] -> CASE 3 wins, value 166, return CHOICE_MATCH (-1)
|   [L262-263] STORE DF(T1:1,T3:3) = 166   DFchoice = CHOICE_MATCH (case 3)
[L197] computeDT(T1:1,T3:3)
|   [L267-273] CASE 1: delete T3:3, put subtree T1:1 into one child subtree of T3:3
|   |   [L267-268] minCost = MAX, case1Child = -1
|   |   [L269] T3:3 has no children: loop does not run, minCost stays MAX
|   |   [L273] case1 = DT(-,T3:3) + minCost = 33.5 + MAX = MAX
|   [L275-281] CASE 2: delete T1:1, put one child subtree of T1:1 onto subtree T3:3
|   |   [L275-276] minCost = MAX, case2Child = -1
|   |   [L277-280] child T1:2: DT(T1:2,T3:3) - DT(T1:2,-) = 132.5 - 166 = -33.5  < min -> minCost = -33.5, case2Child = T1:2
|   |   [L281] case2 = DT(T1:1,-) + minCost = 216 + -33.5 = 182.5
|   [L283] CASE 3: match T1:1 <-> T3:3: case3 = DF(T1:1,T3:3) + relabelCost(T1:1,T3:3)
|   |   [L151] diff1 = |100 - 3| + |0 - 70| = 167   (move one node onto the other)
|   |   [L152] diff2 = |0 - 100| + |70 - 3| = 167   (delete both)
|   |   [L153] relabelCost = min(167, 167) / 2 = 83.5
|   [L283] case3 = 166 + 83.5 = 249.5
|   [L286] decide:
|   |   [L225] choose(case1 = MAX, case2 = 182.5, case3 = 249.5)
|   |   [L227] case3 - 0.00001 <= case2 ? no   case3 - 0.00001 <= case1 ? yes
|   |   [L230,233-235] case1 < case2 ? no -> CASE 2 wins, value 182.5, return -(6+2) = -8
|   [L286-287] STORE DT(T1:1,T3:3) = 182.5   DTchoice = -8 (case 2, T1 child T1:2)
```

</details>

<details><summary><b>i=7, j=1: (T1:1, T3:5)</b> → DF = 166 (case 3), DT = 201 (case 2) · MM: <code>T1:2<->- 166</code></summary>

```text
--- [L194-195] i = 7, j = 1   cell (T1:1,T3:5) ---
[L196] computeDF(T1:1,T3:5)
|   T1:1 children: T1:2   |   T3:5 children: none
|   [L241-247] CASE 1: delete T3:5, put T1:1's forest into one child of T3:5
|   |   [L241-242] minCost = MAX, case1Child = -1
|   |   [L243] T3:5 has no children: loop does not run, minCost stays MAX
|   |   [L247] case1 = DF(-,T3:5) + minCost = 0 + MAX = MAX
|   [L250-256] CASE 2: delete T1:1, put one child of T1:1 onto T3:5's forest
|   |   [L250-251] minCost = MAX, case2Child = -1
|   |   [L252-255] child T1:2: DF(T1:2,T3:5) - DF(T1:2,-) = 128 - 128 = 0  < min -> minCost = 0, case2Child = T1:2
|   |   [L256] case2 = DF(T1:1,-) + minCost = 166 + 0 = 166
|   [L259] CASE 3: case3 = MM(T1:1,T3:5)  (match the two child forests)
|   |   [L294-295] a = #children of T1:1 = 1, b = #children of T3:5 = 0
|   |   [L298] n = a + b = 1 (each side padded with dummies '-' up to n)
|   |   [L302-308] cost matrix, entry [k][l] = DT(row k, col l):
|   |                -         
|   |   T1:2         166       
|   |   [L309] kuhnMunkresMinWeightPerfectMatching(mmCost, 1):
|   |   |   [KM:L29-33] potentials u (rows) = 0, v (cols) = 0; p[col] = row assigned to col (0 = none). Rows/cols are 1-based here.
|   |   |   [KM:L35-39] add row r1(T1:2): p[0] = 1, j0 = 0, minv = INF, used = none
|   |   |   |   [KM:L41-51] mark start used; its row i0 = r1(T1:2). reduced costs cost-u-v for unused cols: c1:166-0-(0)=166->minv 166;
|   |   |   |              smallest minv: delta = 166 at c1(-)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [166], v = [0]
|   |   |   |   [KM:L56-57] c1(-) is free -> augmenting path found
|   |   |   |   [KM:L58-62] flip the path: c1(-)<-r1(T1:2);
|   |   |   [KM:L65-70] KM RESULT: T1:2<->- 166 = total 166
|   |   [L319] MM returns 166
|   [L259] case3 = 166
|   [L262] decide:
|   |   [L225] choose(case1 = MAX, case2 = 166, case3 = 166)
|   |   [L227] case3 - 0.00001 <= case2 ? yes   case3 - 0.00001 <= case1 ? yes
|   |   [L228-229] -> CASE 3 wins, value 166, return CHOICE_MATCH (-1)
|   [L262-263] STORE DF(T1:1,T3:5) = 166   DFchoice = CHOICE_MATCH (case 3)
[L197] computeDT(T1:1,T3:5)
|   [L267-273] CASE 1: delete T3:5, put subtree T1:1 into one child subtree of T3:5
|   |   [L267-268] minCost = MAX, case1Child = -1
|   |   [L269] T3:5 has no children: loop does not run, minCost stays MAX
|   |   [L273] case1 = DT(-,T3:5) + minCost = 15 + MAX = MAX
|   [L275-281] CASE 2: delete T1:1, put one child subtree of T1:1 onto subtree T3:5
|   |   [L275-276] minCost = MAX, case2Child = -1
|   |   [L277-280] child T1:2: DT(T1:2,T3:5) - DT(T1:2,-) = 151 - 166 = -15  < min -> minCost = -15, case2Child = T1:2
|   |   [L281] case2 = DT(T1:1,-) + minCost = 216 + -15 = 201
|   [L283] CASE 3: match T1:1 <-> T3:5: case3 = DF(T1:1,T3:5) + relabelCost(T1:1,T3:5)
|   |   [L151] diff1 = |100 - 30| + |0 - 60| = 130   (move one node onto the other)
|   |   [L152] diff2 = |0 - 100| + |60 - 30| = 130   (delete both)
|   |   [L153] relabelCost = min(130, 130) / 2 = 65
|   [L283] case3 = 166 + 65 = 231
|   [L286] decide:
|   |   [L225] choose(case1 = MAX, case2 = 201, case3 = 231)
|   |   [L227] case3 - 0.00001 <= case2 ? no   case3 - 0.00001 <= case1 ? yes
|   |   [L230,233-235] case1 < case2 ? no -> CASE 2 wins, value 201, return -(6+2) = -8
|   [L286-287] STORE DT(T1:1,T3:5) = 201   DTchoice = -8 (case 2, T1 child T1:2)
```

</details>

<details><summary><b>i=7, j=2: (T1:1, T3:8)</b> → DF = 166 (case 3), DT = 213.5 (case 2) · MM: <code>T1:2<->- 166</code></summary>

```text
--- [L194-195] i = 7, j = 2   cell (T1:1,T3:8) ---
[L196] computeDF(T1:1,T3:8)
|   T1:1 children: T1:2   |   T3:8 children: none
|   [L241-247] CASE 1: delete T3:8, put T1:1's forest into one child of T3:8
|   |   [L241-242] minCost = MAX, case1Child = -1
|   |   [L243] T3:8 has no children: loop does not run, minCost stays MAX
|   |   [L247] case1 = DF(-,T3:8) + minCost = 0 + MAX = MAX
|   [L250-256] CASE 2: delete T1:1, put one child of T1:1 onto T3:8's forest
|   |   [L250-251] minCost = MAX, case2Child = -1
|   |   [L252-255] child T1:2: DF(T1:2,T3:8) - DF(T1:2,-) = 128 - 128 = 0  < min -> minCost = 0, case2Child = T1:2
|   |   [L256] case2 = DF(T1:1,-) + minCost = 166 + 0 = 166
|   [L259] CASE 3: case3 = MM(T1:1,T3:8)  (match the two child forests)
|   |   [L294-295] a = #children of T1:1 = 1, b = #children of T3:8 = 0
|   |   [L298] n = a + b = 1 (each side padded with dummies '-' up to n)
|   |   [L302-308] cost matrix, entry [k][l] = DT(row k, col l):
|   |                -         
|   |   T1:2         166       
|   |   [L309] kuhnMunkresMinWeightPerfectMatching(mmCost, 1):
|   |   |   [KM:L29-33] potentials u (rows) = 0, v (cols) = 0; p[col] = row assigned to col (0 = none). Rows/cols are 1-based here.
|   |   |   [KM:L35-39] add row r1(T1:2): p[0] = 1, j0 = 0, minv = INF, used = none
|   |   |   |   [KM:L41-51] mark start used; its row i0 = r1(T1:2). reduced costs cost-u-v for unused cols: c1:166-0-(0)=166->minv 166;
|   |   |   |              smallest minv: delta = 166 at c1(-)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [166], v = [0]
|   |   |   |   [KM:L56-57] c1(-) is free -> augmenting path found
|   |   |   |   [KM:L58-62] flip the path: c1(-)<-r1(T1:2);
|   |   |   [KM:L65-70] KM RESULT: T1:2<->- 166 = total 166
|   |   [L319] MM returns 166
|   [L259] case3 = 166
|   [L262] decide:
|   |   [L225] choose(case1 = MAX, case2 = 166, case3 = 166)
|   |   [L227] case3 - 0.00001 <= case2 ? yes   case3 - 0.00001 <= case1 ? yes
|   |   [L228-229] -> CASE 3 wins, value 166, return CHOICE_MATCH (-1)
|   [L262-263] STORE DF(T1:1,T3:8) = 166   DFchoice = CHOICE_MATCH (case 3)
[L197] computeDT(T1:1,T3:8)
|   [L267-273] CASE 1: delete T3:8, put subtree T1:1 into one child subtree of T3:8
|   |   [L267-268] minCost = MAX, case1Child = -1
|   |   [L269] T3:8 has no children: loop does not run, minCost stays MAX
|   |   [L273] case1 = DT(-,T3:8) + minCost = 2.5 + MAX = MAX
|   [L275-281] CASE 2: delete T1:1, put one child subtree of T1:1 onto subtree T3:8
|   |   [L275-276] minCost = MAX, case2Child = -1
|   |   [L277-280] child T1:2: DT(T1:2,T3:8) - DT(T1:2,-) = 163.5 - 166 = -2.5  < min -> minCost = -2.5, case2Child = T1:2
|   |   [L281] case2 = DT(T1:1,-) + minCost = 216 + -2.5 = 213.5
|   [L283] CASE 3: match T1:1 <-> T3:8: case3 = DF(T1:1,T3:8) + relabelCost(T1:1,T3:8)
|   |   [L151] diff1 = |100 - 5| + |0 - 10| = 105   (move one node onto the other)
|   |   [L152] diff2 = |0 - 100| + |10 - 5| = 105   (delete both)
|   |   [L153] relabelCost = min(105, 105) / 2 = 52.5
|   [L283] case3 = 166 + 52.5 = 218.5
|   [L286] decide:
|   |   [L225] choose(case1 = MAX, case2 = 213.5, case3 = 218.5)
|   |   [L227] case3 - 0.00001 <= case2 ? no   case3 - 0.00001 <= case1 ? yes
|   |   [L230,233-235] case1 < case2 ? no -> CASE 2 wins, value 213.5, return -(6+2) = -8
|   [L286-287] STORE DT(T1:1,T3:8) = 213.5   DTchoice = -8 (case 2, T1 child T1:2)
```

</details>

<details><summary><b>i=7, j=3: (T1:1, T3:9)</b> → DF = 166 (case 3), DT = 168.5 (case 2) · MM: <code>T1:2<->- 166</code></summary>

```text
--- [L194-195] i = 7, j = 3   cell (T1:1,T3:9) ---
[L196] computeDF(T1:1,T3:9)
|   T1:1 children: T1:2   |   T3:9 children: none
|   [L241-247] CASE 1: delete T3:9, put T1:1's forest into one child of T3:9
|   |   [L241-242] minCost = MAX, case1Child = -1
|   |   [L243] T3:9 has no children: loop does not run, minCost stays MAX
|   |   [L247] case1 = DF(-,T3:9) + minCost = 0 + MAX = MAX
|   [L250-256] CASE 2: delete T1:1, put one child of T1:1 onto T3:9's forest
|   |   [L250-251] minCost = MAX, case2Child = -1
|   |   [L252-255] child T1:2: DF(T1:2,T3:9) - DF(T1:2,-) = 128 - 128 = 0  < min -> minCost = 0, case2Child = T1:2
|   |   [L256] case2 = DF(T1:1,-) + minCost = 166 + 0 = 166
|   [L259] CASE 3: case3 = MM(T1:1,T3:9)  (match the two child forests)
|   |   [L294-295] a = #children of T1:1 = 1, b = #children of T3:9 = 0
|   |   [L298] n = a + b = 1 (each side padded with dummies '-' up to n)
|   |   [L302-308] cost matrix, entry [k][l] = DT(row k, col l):
|   |                -         
|   |   T1:2         166       
|   |   [L309] kuhnMunkresMinWeightPerfectMatching(mmCost, 1):
|   |   |   [KM:L29-33] potentials u (rows) = 0, v (cols) = 0; p[col] = row assigned to col (0 = none). Rows/cols are 1-based here.
|   |   |   [KM:L35-39] add row r1(T1:2): p[0] = 1, j0 = 0, minv = INF, used = none
|   |   |   |   [KM:L41-51] mark start used; its row i0 = r1(T1:2). reduced costs cost-u-v for unused cols: c1:166-0-(0)=166->minv 166;
|   |   |   |              smallest minv: delta = 166 at c1(-)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [166], v = [0]
|   |   |   |   [KM:L56-57] c1(-) is free -> augmenting path found
|   |   |   |   [KM:L58-62] flip the path: c1(-)<-r1(T1:2);
|   |   |   [KM:L65-70] KM RESULT: T1:2<->- 166 = total 166
|   |   [L319] MM returns 166
|   [L259] case3 = 166
|   [L262] decide:
|   |   [L225] choose(case1 = MAX, case2 = 166, case3 = 166)
|   |   [L227] case3 - 0.00001 <= case2 ? yes   case3 - 0.00001 <= case1 ? yes
|   |   [L228-229] -> CASE 3 wins, value 166, return CHOICE_MATCH (-1)
|   [L262-263] STORE DF(T1:1,T3:9) = 166   DFchoice = CHOICE_MATCH (case 3)
[L197] computeDT(T1:1,T3:9)
|   [L267-273] CASE 1: delete T3:9, put subtree T1:1 into one child subtree of T3:9
|   |   [L267-268] minCost = MAX, case1Child = -1
|   |   [L269] T3:9 has no children: loop does not run, minCost stays MAX
|   |   [L273] case1 = DT(-,T3:9) + minCost = 52.5 + MAX = MAX
|   [L275-281] CASE 2: delete T1:1, put one child subtree of T1:1 onto subtree T3:9
|   |   [L275-276] minCost = MAX, case2Child = -1
|   |   [L277-280] child T1:2: DT(T1:2,T3:9) - DT(T1:2,-) = 118.5 - 166 = -47.5  < min -> minCost = -47.5, case2Child = T1:2
|   |   [L281] case2 = DT(T1:1,-) + minCost = 216 + -47.5 = 168.5
|   [L283] CASE 3: match T1:1 <-> T3:9: case3 = DF(T1:1,T3:9) + relabelCost(T1:1,T3:9)
|   |   [L151] diff1 = |100 - 0| + |0 - 105| = 205   (move one node onto the other)
|   |   [L152] diff2 = |0 - 100| + |105 - 0| = 205   (delete both)
|   |   [L153] relabelCost = min(205, 205) / 2 = 102.5
|   [L283] case3 = 166 + 102.5 = 268.5
|   [L286] decide:
|   |   [L225] choose(case1 = MAX, case2 = 168.5, case3 = 268.5)
|   |   [L227] case3 - 0.00001 <= case2 ? no   case3 - 0.00001 <= case1 ? yes
|   |   [L230,233-235] case1 < case2 ? no -> CASE 2 wins, value 168.5, return -(6+2) = -8
|   [L286-287] STORE DT(T1:1,T3:9) = 168.5   DTchoice = -8 (case 2, T1 child T1:2)
```

</details>

<details><summary><b>i=7, j=4: (T1:1, T3:6)</b> → DF = 116 (case 2), DT = 163.5 (case 3) · MM: <code>T1:2<->T3:9 118.5 + -<->- 0 + -<->T3:8 2.5</code></summary>

```text
--- [L194-195] i = 7, j = 4   cell (T1:1,T3:6) ---
[L196] computeDF(T1:1,T3:6)
|   T1:1 children: T1:2   |   T3:6 children: T3:8,T3:9
|   [L241-247] CASE 1: delete T3:6, put T1:1's forest into one child of T3:6
|   |   [L241-242] minCost = MAX, case1Child = -1
|   |   [L243-246] child T3:8: DF(T1:1,T3:8) - DF(-,T3:8) = 166 - 0 = 166  < min -> minCost = 166, case1Child = T3:8
|   |   [L243-246] child T3:9: DF(T1:1,T3:9) - DF(-,T3:9) = 166 - 0 = 166  not smaller
|   |   [L247] case1 = DF(-,T3:6) + minCost = 55 + 166 = 221
|   [L250-256] CASE 2: delete T1:1, put one child of T1:1 onto T3:6's forest
|   |   [L250-251] minCost = MAX, case2Child = -1
|   |   [L252-255] child T1:2: DF(T1:2,T3:6) - DF(T1:2,-) = 78 - 128 = -50  < min -> minCost = -50, case2Child = T1:2
|   |   [L256] case2 = DF(T1:1,-) + minCost = 166 + -50 = 116
|   [L259] CASE 3: case3 = MM(T1:1,T3:6)  (match the two child forests)
|   |   [L294-295] a = #children of T1:1 = 1, b = #children of T3:6 = 2
|   |   [L298] n = a + b = 3 (each side padded with dummies '-' up to n)
|   |   [L302-308] cost matrix, entry [k][l] = DT(row k, col l):
|   |                T3:8      T3:9      -         
|   |   T1:2         163.5     118.5     166       
|   |   -            2.5       52.5      0         
|   |   -            2.5       52.5      0         
|   |   [L309] kuhnMunkresMinWeightPerfectMatching(mmCost, 3):
|   |   |   [KM:L29-33] potentials u (rows) = 0, v (cols) = 0; p[col] = row assigned to col (0 = none). Rows/cols are 1-based here.
|   |   |   [KM:L35-39] add row r1(T1:2): p[0] = 1, j0 = 0, minv = INF, used = none
|   |   |   |   [KM:L41-51] mark start used; its row i0 = r1(T1:2). reduced costs cost-u-v for unused cols: c1:163.5-0-(0)=163.5->minv 163.5; c2:118.5-0-(0)=118.5->minv 118.5; c3:166-0-(0)=166->minv 166;
|   |   |   |              smallest minv: delta = 118.5 at c2(T3:9)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [118.5,0,0], v = [0,0,0]
|   |   |   |   [KM:L56-57] c2(T3:9) is free -> augmenting path found
|   |   |   |   [KM:L58-62] flip the path: c2(T3:9)<-r1(T1:2);
|   |   |   [KM:L35-39] add row r2(-): p[0] = 2, j0 = 0, minv = INF, used = none
|   |   |   |   [KM:L41-51] mark start used; its row i0 = r2(-). reduced costs cost-u-v for unused cols: c1:2.5-0-(0)=2.5->minv 2.5; c2:52.5-0-(0)=52.5->minv 52.5; c3:0-0-(0)=0->minv 0;
|   |   |   |              smallest minv: delta = 0 at c3(-)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [118.5,0,0], v = [0,0,0]
|   |   |   |   [KM:L56-57] c3(-) is free -> augmenting path found
|   |   |   |   [KM:L58-62] flip the path: c3(-)<-r2(-);
|   |   |   [KM:L35-39] add row r3(-): p[0] = 3, j0 = 0, minv = INF, used = none
|   |   |   |   [KM:L41-51] mark start used; its row i0 = r3(-). reduced costs cost-u-v for unused cols: c1:2.5-0-(0)=2.5->minv 2.5; c2:52.5-0-(0)=52.5->minv 52.5; c3:0-0-(0)=0->minv 0;
|   |   |   |              smallest minv: delta = 0 at c3(-)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [118.5,0,0], v = [0,0,0]
|   |   |   |   [KM:L56-57] c3(-) is already taken by r2(-) -> keep searching from it
|   |   |   |   [KM:L41-51] mark c3(-) used; its row i0 = r2(-). reduced costs cost-u-v for unused cols: c1:2.5-0-(0)=2.5->minv 2.5; c2:52.5-0-(0)=52.5->minv 52.5;
|   |   |   |              smallest minv: delta = 2.5 at c1(T3:8)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [118.5,2.5,2.5], v = [0,0,-2.5]
|   |   |   |   [KM:L56-57] c1(T3:8) is free -> augmenting path found
|   |   |   |   [KM:L58-62] flip the path: c1(T3:8)<-r3(-);
|   |   |   [KM:L65-70] KM RESULT: T1:2<->T3:9 118.5 + -<->- 0 + -<->T3:8 2.5 = total 121
|   |   [L319] MM returns 121
|   [L259] case3 = 121
|   [L262] decide:
|   |   [L225] choose(case1 = 221, case2 = 116, case3 = 121)
|   |   [L227] case3 - 0.00001 <= case2 ? no   case3 - 0.00001 <= case1 ? yes
|   |   [L230,233-235] case1 < case2 ? no -> CASE 2 wins, value 116, return -(6+2) = -8
|   [L262-263] STORE DF(T1:1,T3:6) = 116   DFchoice = -8 (case 2, T1 child T1:2)
[L197] computeDT(T1:1,T3:6)
|   [L267-273] CASE 1: delete T3:6, put subtree T1:1 into one child subtree of T3:6
|   |   [L267-268] minCost = MAX, case1Child = -1
|   |   [L269-272] child T3:8: DT(T1:1,T3:8) - DT(-,T3:8) = 213.5 - 2.5 = 211  < min -> minCost = 211, case1Child = T3:8
|   |   [L269-272] child T3:9: DT(T1:1,T3:9) - DT(-,T3:9) = 168.5 - 52.5 = 116  < min -> minCost = 116, case1Child = T3:9
|   |   [L273] case1 = DT(-,T3:6) + minCost = 57.5 + 116 = 173.5
|   [L275-281] CASE 2: delete T1:1, put one child subtree of T1:1 onto subtree T3:6
|   |   [L275-276] minCost = MAX, case2Child = -1
|   |   [L277-280] child T1:2: DT(T1:2,T3:6) - DT(T1:2,-) = 113.5 - 166 = -52.5  < min -> minCost = -52.5, case2Child = T1:2
|   |   [L281] case2 = DT(T1:1,-) + minCost = 216 + -52.5 = 163.5
|   [L283] CASE 3: match T1:1 <-> T3:6: case3 = DF(T1:1,T3:6) + relabelCost(T1:1,T3:6)
|   |   [L151] diff1 = |100 - 10| + |0 - 5| = 95   (move one node onto the other)
|   |   [L152] diff2 = |0 - 100| + |5 - 10| = 105   (delete both)
|   |   [L153] relabelCost = min(95, 105) / 2 = 47.5
|   [L283] case3 = 116 + 47.5 = 163.5
|   [L286] decide:
|   |   [L225] choose(case1 = 173.5, case2 = 163.5, case3 = 163.5)
|   |   [L227] case3 - 0.00001 <= case2 ? yes   case3 - 0.00001 <= case1 ? yes
|   |   [L228-229] -> CASE 3 wins, value 163.5, return CHOICE_MATCH (-1)
|   [L286-287] STORE DT(T1:1,T3:6) = 163.5   DTchoice = CHOICE_MATCH (case 3)
```

</details>

<details><summary><b>i=7, j=5: (T1:1, T3:4)</b> → DF = 98.5 (case 2), DT = 133.5 (case 3) · MM: <code>T1:2<->T3:6 113.5 + -<->- 0 + -<->T3:5 15</code></summary>

```text
--- [L194-195] i = 7, j = 5   cell (T1:1,T3:4) ---
[L196] computeDF(T1:1,T3:4)
|   T1:1 children: T1:2   |   T3:4 children: T3:5,T3:6
|   [L241-247] CASE 1: delete T3:4, put T1:1's forest into one child of T3:4
|   |   [L241-242] minCost = MAX, case1Child = -1
|   |   [L243-246] child T3:5: DF(T1:1,T3:5) - DF(-,T3:5) = 166 - 0 = 166  < min -> minCost = 166, case1Child = T3:5
|   |   [L243-246] child T3:6: DF(T1:1,T3:6) - DF(-,T3:6) = 116 - 55 = 61  < min -> minCost = 61, case1Child = T3:6
|   |   [L247] case1 = DF(-,T3:4) + minCost = 72.5 + 61 = 133.5
|   [L250-256] CASE 2: delete T1:1, put one child of T1:1 onto T3:4's forest
|   |   [L250-251] minCost = MAX, case2Child = -1
|   |   [L252-255] child T1:2: DF(T1:2,T3:4) - DF(T1:2,-) = 60.5 - 128 = -67.5  < min -> minCost = -67.5, case2Child = T1:2
|   |   [L256] case2 = DF(T1:1,-) + minCost = 166 + -67.5 = 98.5
|   [L259] CASE 3: case3 = MM(T1:1,T3:4)  (match the two child forests)
|   |   [L294-295] a = #children of T1:1 = 1, b = #children of T3:4 = 2
|   |   [L298] n = a + b = 3 (each side padded with dummies '-' up to n)
|   |   [L302-308] cost matrix, entry [k][l] = DT(row k, col l):
|   |                T3:5      T3:6      -         
|   |   T1:2         151       113.5     166       
|   |   -            15        57.5      0         
|   |   -            15        57.5      0         
|   |   [L309] kuhnMunkresMinWeightPerfectMatching(mmCost, 3):
|   |   |   [KM:L29-33] potentials u (rows) = 0, v (cols) = 0; p[col] = row assigned to col (0 = none). Rows/cols are 1-based here.
|   |   |   [KM:L35-39] add row r1(T1:2): p[0] = 1, j0 = 0, minv = INF, used = none
|   |   |   |   [KM:L41-51] mark start used; its row i0 = r1(T1:2). reduced costs cost-u-v for unused cols: c1:151-0-(0)=151->minv 151; c2:113.5-0-(0)=113.5->minv 113.5; c3:166-0-(0)=166->minv 166;
|   |   |   |              smallest minv: delta = 113.5 at c2(T3:6)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [113.5,0,0], v = [0,0,0]
|   |   |   |   [KM:L56-57] c2(T3:6) is free -> augmenting path found
|   |   |   |   [KM:L58-62] flip the path: c2(T3:6)<-r1(T1:2);
|   |   |   [KM:L35-39] add row r2(-): p[0] = 2, j0 = 0, minv = INF, used = none
|   |   |   |   [KM:L41-51] mark start used; its row i0 = r2(-). reduced costs cost-u-v for unused cols: c1:15-0-(0)=15->minv 15; c2:57.5-0-(0)=57.5->minv 57.5; c3:0-0-(0)=0->minv 0;
|   |   |   |              smallest minv: delta = 0 at c3(-)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [113.5,0,0], v = [0,0,0]
|   |   |   |   [KM:L56-57] c3(-) is free -> augmenting path found
|   |   |   |   [KM:L58-62] flip the path: c3(-)<-r2(-);
|   |   |   [KM:L35-39] add row r3(-): p[0] = 3, j0 = 0, minv = INF, used = none
|   |   |   |   [KM:L41-51] mark start used; its row i0 = r3(-). reduced costs cost-u-v for unused cols: c1:15-0-(0)=15->minv 15; c2:57.5-0-(0)=57.5->minv 57.5; c3:0-0-(0)=0->minv 0;
|   |   |   |              smallest minv: delta = 0 at c3(-)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [113.5,0,0], v = [0,0,0]
|   |   |   |   [KM:L56-57] c3(-) is already taken by r2(-) -> keep searching from it
|   |   |   |   [KM:L41-51] mark c3(-) used; its row i0 = r2(-). reduced costs cost-u-v for unused cols: c1:15-0-(0)=15->minv 15; c2:57.5-0-(0)=57.5->minv 57.5;
|   |   |   |              smallest minv: delta = 15 at c1(T3:5)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [113.5,15,15], v = [0,0,-15]
|   |   |   |   [KM:L56-57] c1(T3:5) is free -> augmenting path found
|   |   |   |   [KM:L58-62] flip the path: c1(T3:5)<-r3(-);
|   |   |   [KM:L65-70] KM RESULT: T1:2<->T3:6 113.5 + -<->- 0 + -<->T3:5 15 = total 128.5
|   |   [L319] MM returns 128.5
|   [L259] case3 = 128.5
|   [L262] decide:
|   |   [L225] choose(case1 = 133.5, case2 = 98.5, case3 = 128.5)
|   |   [L227] case3 - 0.00001 <= case2 ? no   case3 - 0.00001 <= case1 ? yes
|   |   [L230,233-235] case1 < case2 ? no -> CASE 2 wins, value 98.5, return -(6+2) = -8
|   [L262-263] STORE DF(T1:1,T3:4) = 98.5   DFchoice = -8 (case 2, T1 child T1:2)
[L197] computeDT(T1:1,T3:4)
|   [L267-273] CASE 1: delete T3:4, put subtree T1:1 into one child subtree of T3:4
|   |   [L267-268] minCost = MAX, case1Child = -1
|   |   [L269-272] child T3:5: DT(T1:1,T3:5) - DT(-,T3:5) = 201 - 15 = 186  < min -> minCost = 186, case1Child = T3:5
|   |   [L269-272] child T3:6: DT(T1:1,T3:6) - DT(-,T3:6) = 163.5 - 57.5 = 106  < min -> minCost = 106, case1Child = T3:6
|   |   [L273] case1 = DT(-,T3:4) + minCost = 87.5 + 106 = 193.5
|   [L275-281] CASE 2: delete T1:1, put one child subtree of T1:1 onto subtree T3:4
|   |   [L275-276] minCost = MAX, case2Child = -1
|   |   [L277-280] child T1:2: DT(T1:2,T3:4) - DT(T1:2,-) = 83.5 - 166 = -82.5  < min -> minCost = -82.5, case2Child = T1:2
|   |   [L281] case2 = DT(T1:1,-) + minCost = 216 + -82.5 = 133.5
|   [L283] CASE 3: match T1:1 <-> T3:4: case3 = DF(T1:1,T3:4) + relabelCost(T1:1,T3:4)
|   |   [L151] diff1 = |100 - 60| + |0 - 30| = 70   (move one node onto the other)
|   |   [L152] diff2 = |0 - 100| + |30 - 60| = 130   (delete both)
|   |   [L153] relabelCost = min(70, 130) / 2 = 35
|   [L283] case3 = 98.5 + 35 = 133.5
|   [L286] decide:
|   |   [L225] choose(case1 = 193.5, case2 = 133.5, case3 = 133.5)
|   |   [L227] case3 - 0.00001 <= case2 ? yes   case3 - 0.00001 <= case1 ? yes
|   |   [L228-229] -> CASE 3 wins, value 133.5, return CHOICE_MATCH (-1)
|   [L286-287] STORE DT(T1:1,T3:4) = 133.5   DTchoice = CHOICE_MATCH (case 3)
```

</details>

<details><summary><b>i=7, j=6: (T1:1, T3:2)</b> → DF = 51 (case 2), DT = 67.5 (case 3) · MM: <code>T1:2<->T3:4 83.5 + -<->- 0 + -<->T3:3 33.5</code></summary>

```text
--- [L194-195] i = 7, j = 6   cell (T1:1,T3:2) ---
[L196] computeDF(T1:1,T3:2)
|   T1:1 children: T1:2   |   T3:2 children: T3:3,T3:4
|   [L241-247] CASE 1: delete T3:2, put T1:1's forest into one child of T3:2
|   |   [L241-242] minCost = MAX, case1Child = -1
|   |   [L243-246] child T3:3: DF(T1:1,T3:3) - DF(-,T3:3) = 166 - 0 = 166  < min -> minCost = 166, case1Child = T3:3
|   |   [L243-246] child T3:4: DF(T1:1,T3:4) - DF(-,T3:4) = 98.5 - 72.5 = 26  < min -> minCost = 26, case1Child = T3:4
|   |   [L247] case1 = DF(-,T3:2) + minCost = 121 + 26 = 147
|   [L250-256] CASE 2: delete T1:1, put one child of T1:1 onto T3:2's forest
|   |   [L250-251] minCost = MAX, case2Child = -1
|   |   [L252-255] child T1:2: DF(T1:2,T3:2) - DF(T1:2,-) = 13 - 128 = -115  < min -> minCost = -115, case2Child = T1:2
|   |   [L256] case2 = DF(T1:1,-) + minCost = 166 + -115 = 51
|   [L259] CASE 3: case3 = MM(T1:1,T3:2)  (match the two child forests)
|   |   [L294-295] a = #children of T1:1 = 1, b = #children of T3:2 = 2
|   |   [L298] n = a + b = 3 (each side padded with dummies '-' up to n)
|   |   [L302-308] cost matrix, entry [k][l] = DT(row k, col l):
|   |                T3:3      T3:4      -         
|   |   T1:2         132.5     83.5      166       
|   |   -            33.5      87.5      0         
|   |   -            33.5      87.5      0         
|   |   [L309] kuhnMunkresMinWeightPerfectMatching(mmCost, 3):
|   |   |   [KM:L29-33] potentials u (rows) = 0, v (cols) = 0; p[col] = row assigned to col (0 = none). Rows/cols are 1-based here.
|   |   |   [KM:L35-39] add row r1(T1:2): p[0] = 1, j0 = 0, minv = INF, used = none
|   |   |   |   [KM:L41-51] mark start used; its row i0 = r1(T1:2). reduced costs cost-u-v for unused cols: c1:132.5-0-(0)=132.5->minv 132.5; c2:83.5-0-(0)=83.5->minv 83.5; c3:166-0-(0)=166->minv 166;
|   |   |   |              smallest minv: delta = 83.5 at c2(T3:4)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [83.5,0,0], v = [0,0,0]
|   |   |   |   [KM:L56-57] c2(T3:4) is free -> augmenting path found
|   |   |   |   [KM:L58-62] flip the path: c2(T3:4)<-r1(T1:2);
|   |   |   [KM:L35-39] add row r2(-): p[0] = 2, j0 = 0, minv = INF, used = none
|   |   |   |   [KM:L41-51] mark start used; its row i0 = r2(-). reduced costs cost-u-v for unused cols: c1:33.5-0-(0)=33.5->minv 33.5; c2:87.5-0-(0)=87.5->minv 87.5; c3:0-0-(0)=0->minv 0;
|   |   |   |              smallest minv: delta = 0 at c3(-)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [83.5,0,0], v = [0,0,0]
|   |   |   |   [KM:L56-57] c3(-) is free -> augmenting path found
|   |   |   |   [KM:L58-62] flip the path: c3(-)<-r2(-);
|   |   |   [KM:L35-39] add row r3(-): p[0] = 3, j0 = 0, minv = INF, used = none
|   |   |   |   [KM:L41-51] mark start used; its row i0 = r3(-). reduced costs cost-u-v for unused cols: c1:33.5-0-(0)=33.5->minv 33.5; c2:87.5-0-(0)=87.5->minv 87.5; c3:0-0-(0)=0->minv 0;
|   |   |   |              smallest minv: delta = 0 at c3(-)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [83.5,0,0], v = [0,0,0]
|   |   |   |   [KM:L56-57] c3(-) is already taken by r2(-) -> keep searching from it
|   |   |   |   [KM:L41-51] mark c3(-) used; its row i0 = r2(-). reduced costs cost-u-v for unused cols: c1:33.5-0-(0)=33.5->minv 33.5; c2:87.5-0-(0)=87.5->minv 87.5;
|   |   |   |              smallest minv: delta = 33.5 at c1(T3:3)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [83.5,33.5,33.5], v = [0,0,-33.5]
|   |   |   |   [KM:L56-57] c1(T3:3) is free -> augmenting path found
|   |   |   |   [KM:L58-62] flip the path: c1(T3:3)<-r3(-);
|   |   |   [KM:L65-70] KM RESULT: T1:2<->T3:4 83.5 + -<->- 0 + -<->T3:3 33.5 = total 117
|   |   [L319] MM returns 117
|   [L259] case3 = 117
|   [L262] decide:
|   |   [L225] choose(case1 = 147, case2 = 51, case3 = 117)
|   |   [L227] case3 - 0.00001 <= case2 ? no   case3 - 0.00001 <= case1 ? yes
|   |   [L230,233-235] case1 < case2 ? no -> CASE 2 wins, value 51, return -(6+2) = -8
|   [L262-263] STORE DF(T1:1,T3:2) = 51   DFchoice = -8 (case 2, T1 child T1:2)
[L197] computeDT(T1:1,T3:2)
|   [L267-273] CASE 1: delete T3:2, put subtree T1:1 into one child subtree of T3:2
|   |   [L267-268] minCost = MAX, case1Child = -1
|   |   [L269-272] child T3:3: DT(T1:1,T3:3) - DT(-,T3:3) = 182.5 - 33.5 = 149  < min -> minCost = 149, case1Child = T3:3
|   |   [L269-272] child T3:4: DT(T1:1,T3:4) - DT(-,T3:4) = 133.5 - 87.5 = 46  < min -> minCost = 46, case1Child = T3:4
|   |   [L273] case1 = DT(-,T3:2) + minCost = 154.5 + 46 = 200.5
|   [L275-281] CASE 2: delete T1:1, put one child subtree of T1:1 onto subtree T3:2
|   |   [L275-276] minCost = MAX, case2Child = -1
|   |   [L277-280] child T1:2: DT(T1:2,T3:2) - DT(T1:2,-) = 18.5 - 166 = -147.5  < min -> minCost = -147.5, case2Child = T1:2
|   |   [L281] case2 = DT(T1:1,-) + minCost = 216 + -147.5 = 68.5
|   [L283] CASE 3: match T1:1 <-> T3:2: case3 = DF(T1:1,T3:2) + relabelCost(T1:1,T3:2)
|   |   [L151] diff1 = |100 - 70| + |0 - 3| = 33   (move one node onto the other)
|   |   [L152] diff2 = |0 - 100| + |3 - 70| = 167   (delete both)
|   |   [L153] relabelCost = min(33, 167) / 2 = 16.5
|   [L283] case3 = 51 + 16.5 = 67.5
|   [L286] decide:
|   |   [L225] choose(case1 = 200.5, case2 = 68.5, case3 = 67.5)
|   |   [L227] case3 - 0.00001 <= case2 ? yes   case3 - 0.00001 <= case1 ? yes
|   |   [L228-229] -> CASE 3 wins, value 67.5, return CHOICE_MATCH (-1)
|   [L286-287] STORE DT(T1:1,T3:2) = 67.5   DTchoice = CHOICE_MATCH (case 3)
```

</details>

<details><summary><b>i=7, j=7: (T1:1, T3:1)</b> → DF = 18.5 (case 3), DT = 21 (case 3) · MM: <code>T1:2<->T3:2 18.5 + -<->- 0</code></summary>

```text
--- [L194-195] i = 7, j = 7   cell (T1:1,T3:1) ---
[L196] computeDF(T1:1,T3:1)
|   T1:1 children: T1:2   |   T3:1 children: T3:2
|   [L241-247] CASE 1: delete T3:1, put T1:1's forest into one child of T3:1
|   |   [L241-242] minCost = MAX, case1Child = -1
|   |   [L243-246] child T3:2: DF(T1:1,T3:2) - DF(-,T3:2) = 51 - 121 = -70  < min -> minCost = -70, case1Child = T3:2
|   |   [L247] case1 = DF(-,T3:1) + minCost = 154.5 + -70 = 84.5
|   [L250-256] CASE 2: delete T1:1, put one child of T1:1 onto T3:1's forest
|   |   [L250-251] minCost = MAX, case2Child = -1
|   |   [L252-255] child T1:2: DF(T1:2,T3:1) - DF(T1:2,-) = 46.5 - 128 = -81.5  < min -> minCost = -81.5, case2Child = T1:2
|   |   [L256] case2 = DF(T1:1,-) + minCost = 166 + -81.5 = 84.5
|   [L259] CASE 3: case3 = MM(T1:1,T3:1)  (match the two child forests)
|   |   [L294-295] a = #children of T1:1 = 1, b = #children of T3:1 = 1
|   |   [L298] n = a + b = 2 (each side padded with dummies '-' up to n)
|   |   [L302-308] cost matrix, entry [k][l] = DT(row k, col l):
|   |                T3:2      -         
|   |   T1:2         18.5      166       
|   |   -            154.5     0         
|   |   [L309] kuhnMunkresMinWeightPerfectMatching(mmCost, 2):
|   |   |   [KM:L29-33] potentials u (rows) = 0, v (cols) = 0; p[col] = row assigned to col (0 = none). Rows/cols are 1-based here.
|   |   |   [KM:L35-39] add row r1(T1:2): p[0] = 1, j0 = 0, minv = INF, used = none
|   |   |   |   [KM:L41-51] mark start used; its row i0 = r1(T1:2). reduced costs cost-u-v for unused cols: c1:18.5-0-(0)=18.5->minv 18.5; c2:166-0-(0)=166->minv 166;
|   |   |   |              smallest minv: delta = 18.5 at c1(T3:2)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [18.5,0], v = [0,0]
|   |   |   |   [KM:L56-57] c1(T3:2) is free -> augmenting path found
|   |   |   |   [KM:L58-62] flip the path: c1(T3:2)<-r1(T1:2);
|   |   |   [KM:L35-39] add row r2(-): p[0] = 2, j0 = 0, minv = INF, used = none
|   |   |   |   [KM:L41-51] mark start used; its row i0 = r2(-). reduced costs cost-u-v for unused cols: c1:154.5-0-(0)=154.5->minv 154.5; c2:0-0-(0)=0->minv 0;
|   |   |   |              smallest minv: delta = 0 at c2(-)
|   |   |   |   [KM:L52-55] shift potentials by delta: u = [18.5,0], v = [0,0]
|   |   |   |   [KM:L56-57] c2(-) is free -> augmenting path found
|   |   |   |   [KM:L58-62] flip the path: c2(-)<-r2(-);
|   |   |   [KM:L65-70] KM RESULT: T1:2<->T3:2 18.5 + -<->- 0 = total 18.5
|   |   [L319] MM returns 18.5
|   [L259] case3 = 18.5
|   [L262] decide:
|   |   [L225] choose(case1 = 84.5, case2 = 84.5, case3 = 18.5)
|   |   [L227] case3 - 0.00001 <= case2 ? yes   case3 - 0.00001 <= case1 ? yes
|   |   [L228-229] -> CASE 3 wins, value 18.5, return CHOICE_MATCH (-1)
|   [L262-263] STORE DF(T1:1,T3:1) = 18.5   DFchoice = CHOICE_MATCH (case 3)
[L197] computeDT(T1:1,T3:1)
|   [L267-273] CASE 1: delete T3:1, put subtree T1:1 into one child subtree of T3:1
|   |   [L267-268] minCost = MAX, case1Child = -1
|   |   [L269-272] child T3:2: DT(T1:1,T3:2) - DT(-,T3:2) = 67.5 - 154.5 = -87  < min -> minCost = -87, case1Child = T3:2
|   |   [L273] case1 = DT(-,T3:1) + minCost = 207 + -87 = 120
|   [L275-281] CASE 2: delete T1:1, put one child subtree of T1:1 onto subtree T3:1
|   |   [L275-276] minCost = MAX, case2Child = -1
|   |   [L277-280] child T1:2: DT(T1:2,T3:1) - DT(T1:2,-) = 61 - 166 = -105  < min -> minCost = -105, case2Child = T1:2
|   |   [L281] case2 = DT(T1:1,-) + minCost = 216 + -105 = 111
|   [L283] CASE 3: match T1:1 <-> T3:1: case3 = DF(T1:1,T3:1) + relabelCost(T1:1,T3:1)
|   |   [L151] diff1 = |100 - 105| + |0 - 0| = 5   (move one node onto the other)
|   |   [L152] diff2 = |0 - 100| + |0 - 105| = 205   (delete both)
|   |   [L153] relabelCost = min(5, 205) / 2 = 2.5
|   [L283] case3 = 18.5 + 2.5 = 21
|   [L286] decide:
|   |   [L225] choose(case1 = 120, case2 = 111, case3 = 21)
|   |   [L227] case3 - 0.00001 <= case2 ? yes   case3 - 0.00001 <= case1 ? yes
|   |   [L228-229] -> CASE 3 wins, value 21, return CHOICE_MATCH (-1)
|   [L286-287] STORE DT(T1:1,T3:1) = 21   DTchoice = CHOICE_MATCH (case 3)
```

</details>

## 8. `compute()` returns [L200](../Converted_Merge_Tree_Comparison/CO-MTED.cpp#L200)

```text
[L200] compute() returns DT(T1:1,T3:1) = 21
```

## 9. Phase D: rebuild the mapping [L204-208](../Converted_Merge_Tree_Comparison/CO-MTED.cpp#L204-L208), [L324-350](../Converted_Merge_Tree_Comparison/CO-MTED.cpp#L324-L350)

This is where the C++ differs most from the Java. Nothing was matched during phase C; only the choice codes were stored. `rebuildDT` starts at the two roots and follows them downwards:

* `rebuildDT(i,j)` [L324-335](../Converted_Merge_Tree_Comparison/CO-MTED.cpp#L324-L335):
  * case 3: rebuild `DF(i,j)`, then record `forward[i] = j`.
  * case 1 or 2: move to the stored child and repeat.
  * an empty side `-`: a whole subtree was deleted, so nothing is recorded.
* `rebuildDF(i,j)` [L337-350](../Converted_Merge_Tree_Comparison/CO-MTED.cpp#L337-L350):
  * case 3: **re-run `MM(i,j)`**, this time keeping the pairs, then call `rebuildDT` on each matched pair of children.
  * case 1 or 2: move to the stored child.

Re-running `MM` gives the same matching as in phase C: it reads the same unchanged `DT` values and the solver is deterministic.

```text
[L205] forward[0..7] = -1 (nothing matched yet)
[L206] rebuildDT(T1:1,T3:1)
|   [L326] DTchoice(T1:1,T3:1) = CHOICE_MATCH (case 3)   (DT value 21)
|   [L327-328] case 3 -> rebuildDF(T1:1,T3:1)
|   |   [L339] DFchoice(T1:1,T3:1) = CHOICE_MATCH (case 3)   (DF value 18.5)
|   |   [L340-343] case 3 -> re-run MM(T1:1,T3:1) with a pairs list to see which children were matched:
|   |   |   [L294-295] a = #children of T1:1 = 1, b = #children of T3:1 = 1
|   |   |   [L298] n = a + b = 2 (each side padded with dummies '-' up to n)
|   |   |   [L302-308] cost matrix, entry [k][l] = DT(row k, col l):
|   |   |                T3:2      -         
|   |   |   T1:2         18.5      166       
|   |   |   -            154.5     0         
|   |   |   [L309] kuhnMunkresMinWeightPerfectMatching(mmCost, 2):
|   |   |   |   [KM:L29-33] potentials u (rows) = 0, v (cols) = 0; p[col] = row assigned to col (0 = none). Rows/cols are 1-based here.
|   |   |   |   [KM:L35-39] add row r1(T1:2): p[0] = 1, j0 = 0, minv = INF, used = none
|   |   |   |   |   [KM:L41-51] mark start used; its row i0 = r1(T1:2). reduced costs cost-u-v for unused cols: c1:18.5-0-(0)=18.5->minv 18.5; c2:166-0-(0)=166->minv 166;
|   |   |   |   |              smallest minv: delta = 18.5 at c1(T3:2)
|   |   |   |   |   [KM:L52-55] shift potentials by delta: u = [18.5,0], v = [0,0]
|   |   |   |   |   [KM:L56-57] c1(T3:2) is free -> augmenting path found
|   |   |   |   |   [KM:L58-62] flip the path: c1(T3:2)<-r1(T1:2);
|   |   |   |   [KM:L35-39] add row r2(-): p[0] = 2, j0 = 0, minv = INF, used = none
|   |   |   |   |   [KM:L41-51] mark start used; its row i0 = r2(-). reduced costs cost-u-v for unused cols: c1:154.5-0-(0)=154.5->minv 154.5; c2:0-0-(0)=0->minv 0;
|   |   |   |   |              smallest minv: delta = 0 at c2(-)
|   |   |   |   |   [KM:L52-55] shift potentials by delta: u = [18.5,0], v = [0,0]
|   |   |   |   |   [KM:L56-57] c2(-) is free -> augmenting path found
|   |   |   |   |   [KM:L58-62] flip the path: c2(-)<-r2(-);
|   |   |   |   [KM:L65-70] KM RESULT: T1:2<->T3:2 18.5 + -<->- 0 = total 18.5
|   |   |   [L310-318] pairs (dummy<->dummy dropped): (T1:2,T3:2)
|   |   |   [L319] MM returns 18.5
|   |   [L344] rebuildDT(T1:2,T3:2)
|   |   |   [L326] DTchoice(T1:2,T3:2) = CHOICE_MATCH (case 3)   (DT value 18.5)
|   |   |   [L327-328] case 3 -> rebuildDF(T1:2,T3:2)
|   |   |   |   [L339] DFchoice(T1:2,T3:2) = CHOICE_MATCH (case 3)   (DF value 13)
|   |   |   |   [L340-343] case 3 -> re-run MM(T1:2,T3:2) with a pairs list to see which children were matched:
|   |   |   |   |   [L294-295] a = #children of T1:2 = 2, b = #children of T3:2 = 2
|   |   |   |   |   [L298] n = a + b = 4 (each side padded with dummies '-' up to n)
|   |   |   |   |   [L302-308] cost matrix, entry [k][l] = DT(row k, col l):
|   |   |   |   |                T3:3      T3:4      -         -         
|   |   |   |   |   T1:3         5.5       49.5      38        38        
|   |   |   |   |   T1:4         56.5      7.5       90        90        
|   |   |   |   |   -            33.5      87.5      0         0         
|   |   |   |   |   -            33.5      87.5      0         0         
|   |   |   |   |   [L309] kuhnMunkresMinWeightPerfectMatching(mmCost, 4):
|   |   |   |   |   |   [KM:L29-33] potentials u (rows) = 0, v (cols) = 0; p[col] = row assigned to col (0 = none). Rows/cols are 1-based here.
|   |   |   |   |   |   [KM:L35-39] add row r1(T1:3): p[0] = 1, j0 = 0, minv = INF, used = none
|   |   |   |   |   |   |   [KM:L41-51] mark start used; its row i0 = r1(T1:3). reduced costs cost-u-v for unused cols: c1:5.5-0-(0)=5.5->minv 5.5; c2:49.5-0-(0)=49.5->minv 49.5; c3:38-0-(0)=38->minv 38; c4:38-0-(0)=38->minv 38;
|   |   |   |   |   |   |              smallest minv: delta = 5.5 at c1(T3:3)
|   |   |   |   |   |   |   [KM:L52-55] shift potentials by delta: u = [5.5,0,0,0], v = [0,0,0,0]
|   |   |   |   |   |   |   [KM:L56-57] c1(T3:3) is free -> augmenting path found
|   |   |   |   |   |   |   [KM:L58-62] flip the path: c1(T3:3)<-r1(T1:3);
|   |   |   |   |   |   [KM:L35-39] add row r2(T1:4): p[0] = 2, j0 = 0, minv = INF, used = none
|   |   |   |   |   |   |   [KM:L41-51] mark start used; its row i0 = r2(T1:4). reduced costs cost-u-v for unused cols: c1:56.5-0-(0)=56.5->minv 56.5; c2:7.5-0-(0)=7.5->minv 7.5; c3:90-0-(0)=90->minv 90; c4:90-0-(0)=90->minv 90;
|   |   |   |   |   |   |              smallest minv: delta = 7.5 at c2(T3:4)
|   |   |   |   |   |   |   [KM:L52-55] shift potentials by delta: u = [5.5,7.5,0,0], v = [0,0,0,0]
|   |   |   |   |   |   |   [KM:L56-57] c2(T3:4) is free -> augmenting path found
|   |   |   |   |   |   |   [KM:L58-62] flip the path: c2(T3:4)<-r2(T1:4);
|   |   |   |   |   |   [KM:L35-39] add row r3(-): p[0] = 3, j0 = 0, minv = INF, used = none
|   |   |   |   |   |   |   [KM:L41-51] mark start used; its row i0 = r3(-). reduced costs cost-u-v for unused cols: c1:33.5-0-(0)=33.5->minv 33.5; c2:87.5-0-(0)=87.5->minv 87.5; c3:0-0-(0)=0->minv 0; c4:0-0-(0)=0->minv 0;
|   |   |   |   |   |   |              smallest minv: delta = 0 at c3(-)
|   |   |   |   |   |   |   [KM:L52-55] shift potentials by delta: u = [5.5,7.5,0,0], v = [0,0,0,0]
|   |   |   |   |   |   |   [KM:L56-57] c3(-) is free -> augmenting path found
|   |   |   |   |   |   |   [KM:L58-62] flip the path: c3(-)<-r3(-);
|   |   |   |   |   |   [KM:L35-39] add row r4(-): p[0] = 4, j0 = 0, minv = INF, used = none
|   |   |   |   |   |   |   [KM:L41-51] mark start used; its row i0 = r4(-). reduced costs cost-u-v for unused cols: c1:33.5-0-(0)=33.5->minv 33.5; c2:87.5-0-(0)=87.5->minv 87.5; c3:0-0-(0)=0->minv 0; c4:0-0-(0)=0->minv 0;
|   |   |   |   |   |   |              smallest minv: delta = 0 at c3(-)
|   |   |   |   |   |   |   [KM:L52-55] shift potentials by delta: u = [5.5,7.5,0,0], v = [0,0,0,0]
|   |   |   |   |   |   |   [KM:L56-57] c3(-) is already taken by r3(-) -> keep searching from it
|   |   |   |   |   |   |   [KM:L41-51] mark c3(-) used; its row i0 = r3(-). reduced costs cost-u-v for unused cols: c1:33.5-0-(0)=33.5->minv 33.5; c2:87.5-0-(0)=87.5->minv 87.5; c4:0-0-(0)=0->minv 0;
|   |   |   |   |   |   |              smallest minv: delta = 0 at c4(-)
|   |   |   |   |   |   |   [KM:L52-55] shift potentials by delta: u = [5.5,7.5,0,0], v = [0,0,0,0]
|   |   |   |   |   |   |   [KM:L56-57] c4(-) is free -> augmenting path found
|   |   |   |   |   |   |   [KM:L58-62] flip the path: c4(-)<-r4(-);
|   |   |   |   |   |   [KM:L65-70] KM RESULT: T1:3<->T3:3 5.5 + T1:4<->T3:4 7.5 + -<->- 0 + -<->- 0 = total 13
|   |   |   |   |   [L310-318] pairs (dummy<->dummy dropped): (T1:3,T3:3) (T1:4,T3:4)
|   |   |   |   |   [L319] MM returns 13
|   |   |   |   [L344] rebuildDT(T1:3,T3:3)
|   |   |   |   |   [L326] DTchoice(T1:3,T3:3) = CHOICE_MATCH (case 3)   (DT value 5.5)
|   |   |   |   |   [L327-328] case 3 -> rebuildDF(T1:3,T3:3)
|   |   |   |   |   |   [L339] DFchoice(T1:3,T3:3) = CHOICE_MATCH (case 3)   (DF value 0)
|   |   |   |   |   |   [L340-343] case 3 -> re-run MM(T1:3,T3:3) with a pairs list to see which children were matched:
|   |   |   |   |   |   |   [L294-295] a = #children of T1:3 = 0, b = #children of T3:3 = 0
|   |   |   |   |   |   |   [L296] a == 0 && b == 0 -> return 0 (two empty forests)
|   |   |   |   |   [L329] forward[0] = 0   i.e. MATCH T1:3 -> T3:3
|   |   |   |   [L344] rebuildDT(T1:4,T3:4)
|   |   |   |   |   [L326] DTchoice(T1:4,T3:4) = CHOICE_MATCH (case 3)   (DT value 7.5)
|   |   |   |   |   [L327-328] case 3 -> rebuildDF(T1:4,T3:4)
|   |   |   |   |   |   [L339] DFchoice(T1:4,T3:4) = CHOICE_MATCH (case 3)   (DF value 7.5)
|   |   |   |   |   |   [L340-343] case 3 -> re-run MM(T1:4,T3:4) with a pairs list to see which children were matched:
|   |   |   |   |   |   |   [L294-295] a = #children of T1:4 = 2, b = #children of T3:4 = 2
|   |   |   |   |   |   |   [L298] n = a + b = 4 (each side padded with dummies '-' up to n)
|   |   |   |   |   |   |   [L302-308] cost matrix, entry [k][l] = DT(row k, col l):
|   |   |   |   |   |   |                T3:5      T3:6      -         -         
|   |   |   |   |   |   |   T1:5         0         42.5      15        15        
|   |   |   |   |   |   |   T1:6         45        7.5       60        60        
|   |   |   |   |   |   |   -            15        57.5      0         0         
|   |   |   |   |   |   |   -            15        57.5      0         0         
|   |   |   |   |   |   |   [L309] kuhnMunkresMinWeightPerfectMatching(mmCost, 4):
|   |   |   |   |   |   |   |   [KM:L29-33] potentials u (rows) = 0, v (cols) = 0; p[col] = row assigned to col (0 = none). Rows/cols are 1-based here.
|   |   |   |   |   |   |   |   [KM:L35-39] add row r1(T1:5): p[0] = 1, j0 = 0, minv = INF, used = none
|   |   |   |   |   |   |   |   |   [KM:L41-51] mark start used; its row i0 = r1(T1:5). reduced costs cost-u-v for unused cols: c1:0-0-(0)=0->minv 0; c2:42.5-0-(0)=42.5->minv 42.5; c3:15-0-(0)=15->minv 15; c4:15-0-(0)=15->minv 15;
|   |   |   |   |   |   |   |   |              smallest minv: delta = 0 at c1(T3:5)
|   |   |   |   |   |   |   |   |   [KM:L52-55] shift potentials by delta: u = [0,0,0,0], v = [0,0,0,0]
|   |   |   |   |   |   |   |   |   [KM:L56-57] c1(T3:5) is free -> augmenting path found
|   |   |   |   |   |   |   |   |   [KM:L58-62] flip the path: c1(T3:5)<-r1(T1:5);
|   |   |   |   |   |   |   |   [KM:L35-39] add row r2(T1:6): p[0] = 2, j0 = 0, minv = INF, used = none
|   |   |   |   |   |   |   |   |   [KM:L41-51] mark start used; its row i0 = r2(T1:6). reduced costs cost-u-v for unused cols: c1:45-0-(0)=45->minv 45; c2:7.5-0-(0)=7.5->minv 7.5; c3:60-0-(0)=60->minv 60; c4:60-0-(0)=60->minv 60;
|   |   |   |   |   |   |   |   |              smallest minv: delta = 7.5 at c2(T3:6)
|   |   |   |   |   |   |   |   |   [KM:L52-55] shift potentials by delta: u = [0,7.5,0,0], v = [0,0,0,0]
|   |   |   |   |   |   |   |   |   [KM:L56-57] c2(T3:6) is free -> augmenting path found
|   |   |   |   |   |   |   |   |   [KM:L58-62] flip the path: c2(T3:6)<-r2(T1:6);
|   |   |   |   |   |   |   |   [KM:L35-39] add row r3(-): p[0] = 3, j0 = 0, minv = INF, used = none
|   |   |   |   |   |   |   |   |   [KM:L41-51] mark start used; its row i0 = r3(-). reduced costs cost-u-v for unused cols: c1:15-0-(0)=15->minv 15; c2:57.5-0-(0)=57.5->minv 57.5; c3:0-0-(0)=0->minv 0; c4:0-0-(0)=0->minv 0;
|   |   |   |   |   |   |   |   |              smallest minv: delta = 0 at c3(-)
|   |   |   |   |   |   |   |   |   [KM:L52-55] shift potentials by delta: u = [0,7.5,0,0], v = [0,0,0,0]
|   |   |   |   |   |   |   |   |   [KM:L56-57] c3(-) is free -> augmenting path found
|   |   |   |   |   |   |   |   |   [KM:L58-62] flip the path: c3(-)<-r3(-);
|   |   |   |   |   |   |   |   [KM:L35-39] add row r4(-): p[0] = 4, j0 = 0, minv = INF, used = none
|   |   |   |   |   |   |   |   |   [KM:L41-51] mark start used; its row i0 = r4(-). reduced costs cost-u-v for unused cols: c1:15-0-(0)=15->minv 15; c2:57.5-0-(0)=57.5->minv 57.5; c3:0-0-(0)=0->minv 0; c4:0-0-(0)=0->minv 0;
|   |   |   |   |   |   |   |   |              smallest minv: delta = 0 at c3(-)
|   |   |   |   |   |   |   |   |   [KM:L52-55] shift potentials by delta: u = [0,7.5,0,0], v = [0,0,0,0]
|   |   |   |   |   |   |   |   |   [KM:L56-57] c3(-) is already taken by r3(-) -> keep searching from it
|   |   |   |   |   |   |   |   |   [KM:L41-51] mark c3(-) used; its row i0 = r3(-). reduced costs cost-u-v for unused cols: c1:15-0-(0)=15->minv 15; c2:57.5-0-(0)=57.5->minv 57.5; c4:0-0-(0)=0->minv 0;
|   |   |   |   |   |   |   |   |              smallest minv: delta = 0 at c4(-)
|   |   |   |   |   |   |   |   |   [KM:L52-55] shift potentials by delta: u = [0,7.5,0,0], v = [0,0,0,0]
|   |   |   |   |   |   |   |   |   [KM:L56-57] c4(-) is free -> augmenting path found
|   |   |   |   |   |   |   |   |   [KM:L58-62] flip the path: c4(-)<-r4(-);
|   |   |   |   |   |   |   |   [KM:L65-70] KM RESULT: T1:5<->T3:5 0 + T1:6<->T3:6 7.5 + -<->- 0 + -<->- 0 = total 7.5
|   |   |   |   |   |   |   [L310-318] pairs (dummy<->dummy dropped): (T1:5,T3:5) (T1:6,T3:6)
|   |   |   |   |   |   |   [L319] MM returns 7.5
|   |   |   |   |   |   [L344] rebuildDT(T1:5,T3:5)
|   |   |   |   |   |   |   [L326] DTchoice(T1:5,T3:5) = CHOICE_MATCH (case 3)   (DT value 0)
|   |   |   |   |   |   |   [L327-328] case 3 -> rebuildDF(T1:5,T3:5)
|   |   |   |   |   |   |   |   [L339] DFchoice(T1:5,T3:5) = CHOICE_MATCH (case 3)   (DF value 0)
|   |   |   |   |   |   |   |   [L340-343] case 3 -> re-run MM(T1:5,T3:5) with a pairs list to see which children were matched:
|   |   |   |   |   |   |   |   |   [L294-295] a = #children of T1:5 = 0, b = #children of T3:5 = 0
|   |   |   |   |   |   |   |   |   [L296] a == 0 && b == 0 -> return 0 (two empty forests)
|   |   |   |   |   |   |   [L329] forward[1] = 1   i.e. MATCH T1:5 -> T3:5
|   |   |   |   |   |   [L344] rebuildDT(T1:6,T3:6)
|   |   |   |   |   |   |   [L326] DTchoice(T1:6,T3:6) = CHOICE_MATCH (case 3)   (DT value 7.5)
|   |   |   |   |   |   |   [L327-328] case 3 -> rebuildDF(T1:6,T3:6)
|   |   |   |   |   |   |   |   [L339] DFchoice(T1:6,T3:6) = CHOICE_MATCH (case 3)   (DF value 5)
|   |   |   |   |   |   |   |   [L340-343] case 3 -> re-run MM(T1:6,T3:6) with a pairs list to see which children were matched:
|   |   |   |   |   |   |   |   |   [L294-295] a = #children of T1:6 = 2, b = #children of T3:6 = 2
|   |   |   |   |   |   |   |   |   [L298] n = a + b = 4 (each side padded with dummies '-' up to n)
|   |   |   |   |   |   |   |   |   [L302-308] cost matrix, entry [k][l] = DT(row k, col l):
|   |   |   |   |   |   |   |   |                T3:8      T3:9      -         -         
|   |   |   |   |   |   |   |   |   T1:8         2.5       47.5      5         5         
|   |   |   |   |   |   |   |   |   T1:9         47.5      2.5       50        50        
|   |   |   |   |   |   |   |   |   -            2.5       52.5      0         0         
|   |   |   |   |   |   |   |   |   -            2.5       52.5      0         0         
|   |   |   |   |   |   |   |   |   [L309] kuhnMunkresMinWeightPerfectMatching(mmCost, 4):
|   |   |   |   |   |   |   |   |   |   [KM:L29-33] potentials u (rows) = 0, v (cols) = 0; p[col] = row assigned to col (0 = none). Rows/cols are 1-based here.
|   |   |   |   |   |   |   |   |   |   [KM:L35-39] add row r1(T1:8): p[0] = 1, j0 = 0, minv = INF, used = none
|   |   |   |   |   |   |   |   |   |   |   [KM:L41-51] mark start used; its row i0 = r1(T1:8). reduced costs cost-u-v for unused cols: c1:2.5-0-(0)=2.5->minv 2.5; c2:47.5-0-(0)=47.5->minv 47.5; c3:5-0-(0)=5->minv 5; c4:5-0-(0)=5->minv 5;
|   |   |   |   |   |   |   |   |   |   |              smallest minv: delta = 2.5 at c1(T3:8)
|   |   |   |   |   |   |   |   |   |   |   [KM:L52-55] shift potentials by delta: u = [2.5,0,0,0], v = [0,0,0,0]
|   |   |   |   |   |   |   |   |   |   |   [KM:L56-57] c1(T3:8) is free -> augmenting path found
|   |   |   |   |   |   |   |   |   |   |   [KM:L58-62] flip the path: c1(T3:8)<-r1(T1:8);
|   |   |   |   |   |   |   |   |   |   [KM:L35-39] add row r2(T1:9): p[0] = 2, j0 = 0, minv = INF, used = none
|   |   |   |   |   |   |   |   |   |   |   [KM:L41-51] mark start used; its row i0 = r2(T1:9). reduced costs cost-u-v for unused cols: c1:47.5-0-(0)=47.5->minv 47.5; c2:2.5-0-(0)=2.5->minv 2.5; c3:50-0-(0)=50->minv 50; c4:50-0-(0)=50->minv 50;
|   |   |   |   |   |   |   |   |   |   |              smallest minv: delta = 2.5 at c2(T3:9)
|   |   |   |   |   |   |   |   |   |   |   [KM:L52-55] shift potentials by delta: u = [2.5,2.5,0,0], v = [0,0,0,0]
|   |   |   |   |   |   |   |   |   |   |   [KM:L56-57] c2(T3:9) is free -> augmenting path found
|   |   |   |   |   |   |   |   |   |   |   [KM:L58-62] flip the path: c2(T3:9)<-r2(T1:9);
|   |   |   |   |   |   |   |   |   |   [KM:L35-39] add row r3(-): p[0] = 3, j0 = 0, minv = INF, used = none
|   |   |   |   |   |   |   |   |   |   |   [KM:L41-51] mark start used; its row i0 = r3(-). reduced costs cost-u-v for unused cols: c1:2.5-0-(0)=2.5->minv 2.5; c2:52.5-0-(0)=52.5->minv 52.5; c3:0-0-(0)=0->minv 0; c4:0-0-(0)=0->minv 0;
|   |   |   |   |   |   |   |   |   |   |              smallest minv: delta = 0 at c3(-)
|   |   |   |   |   |   |   |   |   |   |   [KM:L52-55] shift potentials by delta: u = [2.5,2.5,0,0], v = [0,0,0,0]
|   |   |   |   |   |   |   |   |   |   |   [KM:L56-57] c3(-) is free -> augmenting path found
|   |   |   |   |   |   |   |   |   |   |   [KM:L58-62] flip the path: c3(-)<-r3(-);
|   |   |   |   |   |   |   |   |   |   [KM:L35-39] add row r4(-): p[0] = 4, j0 = 0, minv = INF, used = none
|   |   |   |   |   |   |   |   |   |   |   [KM:L41-51] mark start used; its row i0 = r4(-). reduced costs cost-u-v for unused cols: c1:2.5-0-(0)=2.5->minv 2.5; c2:52.5-0-(0)=52.5->minv 52.5; c3:0-0-(0)=0->minv 0; c4:0-0-(0)=0->minv 0;
|   |   |   |   |   |   |   |   |   |   |              smallest minv: delta = 0 at c3(-)
|   |   |   |   |   |   |   |   |   |   |   [KM:L52-55] shift potentials by delta: u = [2.5,2.5,0,0], v = [0,0,0,0]
|   |   |   |   |   |   |   |   |   |   |   [KM:L56-57] c3(-) is already taken by r3(-) -> keep searching from it
|   |   |   |   |   |   |   |   |   |   |   [KM:L41-51] mark c3(-) used; its row i0 = r3(-). reduced costs cost-u-v for unused cols: c1:2.5-0-(0)=2.5->minv 2.5; c2:52.5-0-(0)=52.5->minv 52.5; c4:0-0-(0)=0->minv 0;
|   |   |   |   |   |   |   |   |   |   |              smallest minv: delta = 0 at c4(-)
|   |   |   |   |   |   |   |   |   |   |   [KM:L52-55] shift potentials by delta: u = [2.5,2.5,0,0], v = [0,0,0,0]
|   |   |   |   |   |   |   |   |   |   |   [KM:L56-57] c4(-) is free -> augmenting path found
|   |   |   |   |   |   |   |   |   |   |   [KM:L58-62] flip the path: c4(-)<-r4(-);
|   |   |   |   |   |   |   |   |   |   [KM:L65-70] KM RESULT: T1:8<->T3:8 2.5 + T1:9<->T3:9 2.5 + -<->- 0 + -<->- 0 = total 5
|   |   |   |   |   |   |   |   |   [L310-318] pairs (dummy<->dummy dropped): (T1:8,T3:8) (T1:9,T3:9)
|   |   |   |   |   |   |   |   |   [L319] MM returns 5
|   |   |   |   |   |   |   |   [L344] rebuildDT(T1:8,T3:8)
|   |   |   |   |   |   |   |   |   [L326] DTchoice(T1:8,T3:8) = CHOICE_MATCH (case 3)   (DT value 2.5)
|   |   |   |   |   |   |   |   |   [L327-328] case 3 -> rebuildDF(T1:8,T3:8)
|   |   |   |   |   |   |   |   |   |   [L339] DFchoice(T1:8,T3:8) = CHOICE_MATCH (case 3)   (DF value 0)
|   |   |   |   |   |   |   |   |   |   [L340-343] case 3 -> re-run MM(T1:8,T3:8) with a pairs list to see which children were matched:
|   |   |   |   |   |   |   |   |   |   |   [L294-295] a = #children of T1:8 = 0, b = #children of T3:8 = 0
|   |   |   |   |   |   |   |   |   |   |   [L296] a == 0 && b == 0 -> return 0 (two empty forests)
|   |   |   |   |   |   |   |   |   [L329] forward[2] = 2   i.e. MATCH T1:8 -> T3:8
|   |   |   |   |   |   |   |   [L344] rebuildDT(T1:9,T3:9)
|   |   |   |   |   |   |   |   |   [L326] DTchoice(T1:9,T3:9) = CHOICE_MATCH (case 3)   (DT value 2.5)
|   |   |   |   |   |   |   |   |   [L327-328] case 3 -> rebuildDF(T1:9,T3:9)
|   |   |   |   |   |   |   |   |   |   [L339] DFchoice(T1:9,T3:9) = CHOICE_MATCH (case 3)   (DF value 0)
|   |   |   |   |   |   |   |   |   |   [L340-343] case 3 -> re-run MM(T1:9,T3:9) with a pairs list to see which children were matched:
|   |   |   |   |   |   |   |   |   |   |   [L294-295] a = #children of T1:9 = 0, b = #children of T3:9 = 0
|   |   |   |   |   |   |   |   |   |   |   [L296] a == 0 && b == 0 -> return 0 (two empty forests)
|   |   |   |   |   |   |   |   |   [L329] forward[3] = 3   i.e. MATCH T1:9 -> T3:9
|   |   |   |   |   |   |   [L329] forward[4] = 4   i.e. MATCH T1:6 -> T3:6
|   |   |   |   |   [L329] forward[5] = 5   i.e. MATCH T1:4 -> T3:4
|   |   |   [L329] forward[6] = 6   i.e. MATCH T1:2 -> T3:2
|   [L329] forward[7] = 7   i.e. MATCH T1:1 -> T3:1
[L207] return forward (index->index): 0->0 1->1 2->2 3->3 4->4 5->5 6->6 7->7
```

## 10. Phase E: write the files and print [L424-445](../Converted_Merge_Tree_Comparison/CO-MTED.cpp#L424-L445)

```text
[L426-430] for i = 0..7 (ascending T1 post-order index):
|   i = 0 -> j = 0: [L428] backward[0] = 0; [L429] forward line "3,3"
|   i = 1 -> j = 1: [L428] backward[1] = 1; [L429] forward line "5,5"
|   i = 2 -> j = 2: [L428] backward[2] = 2; [L429] forward line "8,8"
|   i = 3 -> j = 3: [L428] backward[3] = 3; [L429] forward line "9,9"
|   i = 4 -> j = 4: [L428] backward[4] = 4; [L429] forward line "6,6"
|   i = 5 -> j = 5: [L428] backward[5] = 5; [L429] forward line "4,4"
|   i = 6 -> j = 6: [L428] backward[6] = 6; [L429] forward line "2,2"
|   i = 7 -> j = 7: [L428] backward[7] = 7; [L429] forward line "1,1"
[L431-434] for j = 0..7 (ascending T3 post-order index):
|   j = 0 <- i = 0: [L433] backward line "3,3"
|   j = 1 <- i = 1: [L433] backward line "5,5"
|   j = 2 <- i = 2: [L433] backward line "8,8"
|   j = 3 <- i = 3: [L433] backward line "9,9"
|   j = 4 <- i = 4: [L433] backward line "6,6"
|   j = 5 <- i = 5: [L433] backward line "4,4"
|   j = 6 <- i = 6: [L433] backward line "2,2"
|   j = 7 <- i = 7: [L433] backward line "1,1"
[L436] writeMapFile(forwardmapCO.txt): 8 lines
[L440] writeMapFile(backwardmapCO.txt): 8 lines
[L445] print javaDoubleToString(21) = "21.0" to stdout
```

## 11. Checking the answer

Each pair in the final mapping costs its relabel cost [L150-154](../Converted_Merge_Tree_Comparison/CO-MTED.cpp#L150-L154). Each node outside the mapping costs its delete cost [L146-148](../Converted_Merge_Tree_Comparison/CO-MTED.cpp#L146-L148).

| Pair (T1 ↔ T3) | Tree1 (value, pair value) | Tree3 (value, pair value) | diff1 | diff2 | cost = min/2 |
|---|---|---|---|---|---|
| 3↔3 | 4, 80 | 3, 70 | 11 | 143 | **5.5** |
| 5↔5 | 30, 60 | 30, 60 | 0 | 60 | **0** |
| 8↔8 | 5, 15 | 5, 10 | 5 | 15 | **2.5** |
| 9↔9 | 0, 100 | 0, 105 | 5 | 205 | **2.5** |
| 6↔6 | 15, 5 | 10, 5 | 5 | 15 | **2.5** |
| 4↔4 | 60, 30 | 60, 30 | 0 | 60 | **0** |
| 2↔2 | 80, 4 | 70, 3 | 11 | 143 | **5.5** |
| 1↔1 | 100, 0 | 105, 0 | 5 | 205 | **2.5** |

**Matched pairs + deleted nodes = 21**, the same as the distance the program printed (21.0).
For comparison, deleting both trees completely would cost 216 + 207 = 423.


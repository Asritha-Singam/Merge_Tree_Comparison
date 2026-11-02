"""Generates edge-case, random and error-case .jt files for the JAR-vs-source tests.

.jt line format: nodeID parentID pairID maxNode value numChildren childIDs...
Convention used by the project (see Examples/Inputs/TreeA.jt): leaves are minima,
values grow towards the root, the root has a single child and is paired with the
global minimum leaf, every other saddle is paired with the leaf that dies there.
"""
import os
import random
import sys

OUT = sys.argv[1]
EDGE = os.path.join(OUT, "edge")
RAND = os.path.join(OUT, "random")
ERR = os.path.join(OUT, "error")
for d in (EDGE, RAND, ERR):
    os.makedirs(d, exist_ok=True)


class Node:
    def __init__(self, nid, value):
        self.id = nid
        self.value = value
        self.parent = -1
        self.pair = nid
        self.children = []
        self.maxnode = 0


def fmt(v):
    return repr(float(v))


def write(path, nodes, order=None, sep=" ", eol="\n", valfmt=fmt):
    nodes = list(nodes)
    if order is not None:
        nodes = order(nodes)
    lines = [str(len(nodes))]
    for n in nodes:
        parts = [str(n.id), str(n.parent), str(n.pair), str(n.maxnode), valfmt(n.value), str(len(n.children))]
        parts += [str(c.id) for c in n.children]
        lines.append(sep.join(parts))
    with open(path, "w", newline="") as f:
        f.write(eol.join(lines) + eol)


def merge_tree(leaf_values, rng, ids=None, saddle_gap=lambda r: r.uniform(0.5, 5.0), max_branch=2,
               chain_prob=0.0, int_values=False):
    """Builds a merge tree bottom-up with elder-rule pairing; returns the node list."""
    ids = list(ids) if ids is not None else list(range(10 ** 6))
    nxt = iter(ids)
    nodes = []
    comps = []  # (top node, min leaf)
    for v in leaf_values:
        n = Node(next(nxt), v)
        nodes.append(n)
        comps.append((n, n))
    while len(comps) > 1:
        k = min(len(comps), rng.choice([2] * 6 + list(range(2, max_branch + 1))))
        rng.shuffle(comps)
        group, comps = comps[:k], comps[k:]
        top = max(t.value for t, _ in group)
        gap = saddle_gap(rng)
        val = top + (round(gap) if int_values else gap)
        s = Node(next(nxt), val)
        nodes.append(s)
        for t, _ in group:
            t.parent = s.id
            s.children.append(t)
        group.sort(key=lambda g: g[1].value)
        survivor = group[0][1]
        dying = [g[1] for g in group[1:]]
        for leaf in dying:
            leaf.pair = s.id
        s.pair = dying[0].id
        comps.append((s, survivor))
        if chain_prob and rng.random() < chain_prob:
            c = Node(next(nxt), val + (1 if int_values else rng.uniform(0.1, 1.0)))
            nodes.append(c)
            s.parent = c.id
            c.children.append(s)
            comps[-1] = (c, survivor)  # degree-1 node, paired with itself
    top, gmin = comps[0]
    root = Node(next(nxt), top.value + (1 if int_values else rng.uniform(0.5, 5.0)))
    nodes.append(root)
    top.parent = root.id
    root.children.append(top)
    root.pair = gmin.id
    gmin.pair = root.id
    return nodes


def star(values, root_value):
    root = Node(0, root_value)
    leaves = [Node(i + 1, v) for i, v in enumerate(values)]
    for l in leaves:
        l.parent = 0
        l.pair = 0
        root.children.append(l)
    gmin = min(leaves, key=lambda l: l.value)
    root.pair = gmin.id
    return [root] + leaves


rng = random.Random(20261031)

# ---------------------------------------------------------------- edge cases
single = Node(0, 5.0)
write(os.path.join(EDGE, "e01_single_node.jt"), [single])

r, l = Node(0, 10.0), Node(1, 0.0)
r.children, l.parent, r.pair, l.pair = [l], 0, 1, 0
write(os.path.join(EDGE, "e02_two_nodes.jt"), [r, l])

write(os.path.join(EDGE, "e03_star5_distinct.jt"), star([1.0, 2.5, 4.0, 7.5, 9.0], 20.0))
write(os.path.join(EDGE, "e04_star8_equal_leaves.jt"), star([3.0] * 8, 10.0))
write(os.path.join(EDGE, "e05_star12_distinct.jt"), star([0.5 * i for i in range(12)], 30.0))

allsame = merge_tree([4.0] * 6, rng, saddle_gap=lambda r: 0.0)
write(os.path.join(EDGE, "e06_all_values_equal.jt"), allsame)

neg = merge_tree([rng.uniform(-100, -50) for _ in range(9)], rng)
write(os.path.join(EDGE, "e07_negative_values.jt"), neg)

huge = merge_tree([rng.uniform(1e14, 1e15) for _ in range(9)], rng, saddle_gap=lambda r: r.uniform(1e13, 1e14))
write(os.path.join(EDGE, "e08_huge_values.jt"), huge)

tiny = merge_tree([rng.uniform(1e-13, 1e-12) for _ in range(9)], rng, saddle_gap=lambda r: r.uniform(1e-14, 1e-13))
write(os.path.join(EDGE, "e09_tiny_values.jt"), tiny)

mixed = merge_tree([10 ** rng.uniform(-9, 9) for _ in range(9)], rng, saddle_gap=lambda r: 10 ** r.uniform(-9, 9))
write(os.path.join(EDGE, "e10_mixed_magnitudes.jt"), mixed)

# caterpillar: each saddle merges the chain so far with one new leaf
cat_leaves = [float(i) for i in range(40)]
cat = merge_tree(cat_leaves[:1], rng)
nodes = [Node(0, 0.0)]
spine, gmin, nid = nodes[0], nodes[0], 1
for i in range(1, 40):
    leaf = Node(nid, float(i)); nid += 1
    s = Node(nid, 40.0 + i); nid += 1
    spine.parent = s.id; leaf.parent = s.id
    s.children = [spine, leaf]
    leaf.pair, s.pair = s.id, leaf.id
    nodes += [leaf, s]
    spine = s
root = Node(nid, 100.0)
spine.parent = root.id
root.children = [spine]
root.pair, gmin.pair = gmin.id, root.id
nodes.append(root)
write(os.path.join(EDGE, "e11_caterpillar_40_leaves.jt"), nodes)

shuf_ids = rng.sample(range(10 ** 6, 10 ** 7), 40)
weird_ids = merge_tree([rng.uniform(0, 50) for _ in range(12)], rng, ids=shuf_ids)
write(os.path.join(EDGE, "e12_large_ids_shuffled_lines.jt"), weird_ids,
      order=lambda ns: rng.sample(ns, len(ns)))

base = star([1.0, 2.5, 4.0, 7.5, 9.0], 20.0)
write(os.path.join(EDGE, "e13_star5_crlf_tabs_sci.jt"), base, sep=" \t ", eol="\r\n",
      valfmt=lambda v: "%.3E" % v if v != int(v) else str(int(v)))

chain = merge_tree([rng.uniform(0, 30) for _ in range(10)], rng, chain_prob=0.6)
write(os.path.join(EDGE, "e14_degree1_chain_nodes.jt"), chain)

big = merge_tree([rng.uniform(0, 1000) for _ in range(100)], rng, max_branch=3)
write(os.path.join(EDGE, "e15_big_100_leaves.jt"), big)

ints = merge_tree([float(rng.randint(0, 3)) for _ in range(10)], rng, saddle_gap=lambda r: r.choice([0, 1]),
                  int_values=True)
write(os.path.join(EDGE, "e16_integer_ties.jt"), ints)

selfpair = star([2.0, 2.0, 5.0], 5.0)
write(os.path.join(EDGE, "e17_zero_persistence_leaves.jt"), selfpair)

wide = merge_tree([rng.uniform(0, 10) for _ in range(15)], rng, max_branch=5)
write(os.path.join(EDGE, "e18_high_degree_saddles.jt"), wide)

# --------------------------------------------------------------- random trees
fams = {
    "cont": dict(gen=lambda n: [rng.uniform(0, 100) for _ in range(n)], kw={}),
    "ints": dict(gen=lambda n: [float(rng.randint(0, 5)) for _ in range(n)],
                 kw=dict(saddle_gap=lambda r: r.choice([0, 0, 1, 2]), int_values=True)),
    "wild": dict(gen=lambda n: [rng.choice([-1, 1]) * 10 ** rng.uniform(-6, 6) for _ in range(n)],
                 kw=dict(saddle_gap=lambda r: 10 ** r.uniform(-6, 6), max_branch=4, chain_prob=0.2)),
}
for fam, spec in fams.items():
    for i in range(40):
        n = rng.choice([1, 2, 3] + list(range(4, 31)))
        ids = rng.sample(range(0, 5000), 4 * n + 10) if rng.random() < 0.5 else None
        t = merge_tree(spec["gen"](n), rng, ids=ids, **spec["kw"])
        order = (lambda ns: rng.sample(ns, len(ns))) if rng.random() < 0.3 else None
        write(os.path.join(RAND, "r_%s_%02d.jt" % (fam, i)), t, order=order)

# ---------------------------------------------------------------- error cases
ok = os.path.join(EDGE, "e03_star5_distinct.jt")
with open(os.path.join(ERR, "x_empty.jt"), "w") as f:
    pass
with open(os.path.join(ERR, "x_zero_nodes.jt"), "w") as f:
    f.write("0\n")
with open(os.path.join(ERR, "x_bad_number.jt"), "w") as f:
    f.write("2\n0 -1 1 0 abc 1 1\n1 0 0 0 0.0 0\n")
with open(os.path.join(ERR, "x_missing_pair.jt"), "w") as f:
    f.write("2\n0 -1 1 0 10.0 1 1\n1 0 77 0 0.0 0\n")
with open(os.path.join(ERR, "x_missing_child.jt"), "w") as f:
    f.write("2\n0 -1 1 0 10.0 2 1 99\n1 0 0 0 0.0 0\n")
with open(os.path.join(ERR, "x_truncated.jt"), "w") as f:
    f.write("5\n0 -1 1 0 10.0 1 1\n1 0 0 0 0.0 0\n")
with open(os.path.join(ERR, "x_no_root_cycle.jt"), "w") as f:
    f.write("2\n0 1 1 0 10.0 1 1\n1 0 0 0 0.0 1 0\n")
with open(os.path.join(ERR, "x_nan_value.jt"), "w") as f:
    f.write("3\n0 -1 2 0 NaN 1 1\n1 0 2 0 5.0 1 2\n2 1 0 0 0.0 0\n")
with open(os.path.join(ERR, "x_infinity_value.jt"), "w") as f:
    f.write("3\n0 -1 2 0 Infinity 1 1\n1 0 2 0 5.0 1 2\n2 1 0 0 0.0 0\n")

print("edge", len(os.listdir(EDGE)), "random", len(os.listdir(RAND)), "error", len(os.listdir(ERR)))

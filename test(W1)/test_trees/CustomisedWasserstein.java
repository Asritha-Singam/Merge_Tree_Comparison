package comparator;


import java.io.BufferedWriter;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileWriter;
import java.io.IOException;
//import java.io.PrintWriter;
import java.util.Collections;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.Scanner;
import java.util.Set;
import org.jgrapht.alg.matching.KuhnMunkresMinimalWeightBipartitePerfectMatching;
import org.jgrapht.alg.matching.HopcroftKarpMaximumCardinalityBipartiteMatching;

import org.jgrapht.graph.DefaultEdge;
import org.jgrapht.graph.DefaultWeightedEdge;
import org.jgrapht.graph.SimpleGraph;
import org.jgrapht.graph.SimpleWeightedGraph;
/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/**
 *
 * @author tbmasood
 */
public class ConstrainedEditDist {
        public static void main(String... args) throws FileNotFoundException, IOException {

        //Set main Dir where the split trees exist
        String mainDir = "~/Documents/project/3/Modified code/vgl_iisc-merge-tree-comparison-framework-c7919bc196b3/Examples/Inputs";


        //

        //String mainDir = "C:\\Users\\Nishit\\Documents\\Edit Distances\\trees\\";

        //Set file name of the two trees
        String jt1FileName = args[0];
        //System.out.println(args[0]);//debug
        String jt2FileName = args[1];
        String CompID = args[2];

        //dont worry about threshold
        double onePerThresholdValue = 1.6667 / 100.0;
        double percent = 0.0;
        double thresholdValue = onePerThresholdValue * percent;

        //read and stabilize two trees again don't worry about stabilization
        JoinTree joinTree1 = new JoinTree();
        joinTree1.readJoinTree(jt1FileName);
        joinTree1.stabilize(thresholdValue);

        JoinTree joinTree2 = new JoinTree();
        joinTree2.readJoinTree(jt2FileName);
        joinTree2.stabilize(thresholdValue);

        //construct the pair
        JoinTreePair joinTreePair = new JoinTreePair(joinTree1, joinTree2);


        //compute edit distance and mapping
        /*ConstrainedEditDist ConstrainedEditDist = new ConstrainedEditDist(
                    joinTree1.stabilizedJTRoot, joinTree2.stabilizedJTRoot, joinTreePair);
            Map<Integer, Integer> forwardMap = new HashMap<Integer, Integer>(), backwardMap = new HashMap<Integer, Integer>();
        //the compute function also outouts the mapping between the tree nodes both directions
            double cost = ConstrainedEditDist.compute(joinTree1.stabilizedJTRoot,
                    joinTree2.stabilizedJTRoot, forwardMap, backwardMap, CompID);

        */
        //output final cost
        //System.out.println(cost);

        ConstrainedEditDist.WassersteinDist(joinTree1.jTNodes, joinTree2.jTNodes);

        //auxiliary code to draw rhe trees and store them as images
        //set the output file paths appropriately

        /*JoinTreeRenderer jtrenderer1 = new JoinTreeRenderer(joinTree1,joinTree1.root);

        BufferedImage image1 = jtrenderer1.createImage(joinTree1.root);

        String treeimagefile1 = "/home/raghavendra/Desktop/tree1.png";
        ImageIO.write(image1, "png", new File(treeimagefile1));

        JoinTreeRenderer jtrenderer2 = new JoinTreeRenderer(joinTree2,joinTree2.root);

        BufferedImage image2 = jtrenderer2.createImage(joinTree2.root);

        String treeimagefile2 = "/home/raghavendra/Desktop/tree2.png";
        ImageIO.write(image2, "png", new File(treeimagefile2));*/

    }

    public static interface TreeNode {

        public int getIndex();

        public void setIndex(int i);

        public List<? extends TreeNode> getChildren();

        public TreeNode getParent();

        public int getID();
    }

    public static interface NodeMatchingCostComputer {

        public double matchingCost(TreeNode t1i, TreeNode t2j, int indexI, int indexJ);
    }

    public static class DummyTreeNode implements TreeNode {

        @Override
        public int getIndex() {
            return -1;
        }

        @Override
        public void setIndex(int i) {
            // do nothing
        }

        @Override
        public List<TreeNode> getChildren() {
            return null;
        }

        @Override
        public TreeNode getParent() {
            return null;
        }

        @Override
        public int getID() {
            return -1;
        }
    }

    public static class Pair<T> {

        T i, j;

        public Pair(T i, T j) {
            this.i = i;
            this.j = j;
        }

        @Override
        public boolean equals(Object obj) {
            if (obj instanceof Pair) {
                Pair other = (Pair) obj;
                return other.i.equals(i) && other.j.equals(j);
            }
            return super.equals(obj);
        }

        @Override
        public int hashCode() {
            int hash = 7;
            hash = 71 * hash + Objects.hashCode(this.i);
            hash = 71 * hash + Objects.hashCode(this.j);
            return hash;
        }
    }
    List<TreeNode> tree1, tree2;
    TreeNode tree1Root, tree2Root;
    Map<Pair<Integer>, Double> DF, DT;
    Map<Pair<Integer>, Map<Integer, Integer>> pairedDTForward;
    Map<Pair<Integer>, Map<Integer, Integer>> pairedDTBackward;
    Map<Pair<Integer>, Map<Integer, Integer>> pairedDFForward;
    Map<Pair<Integer>, Map<Integer, Integer>> pairedDFBackward;
    NodeMatchingCostComputer costComputer;

    public ConstrainedEditDist(TreeNode tree1Root, TreeNode tree2Root, NodeMatchingCostComputer costComputer) {
        this.tree1 = new ArrayList<>();
        this.tree1Root = tree1Root;
        this.tree2 = new ArrayList<>();
        this.tree2Root = tree2Root;
        this.costComputer = costComputer;
        DF = new HashMap<>();
        DT = new HashMap<>();
        pairedDTForward = new HashMap<>();
        pairedDTBackward = new HashMap<>();
        pairedDFForward = new HashMap<>();
        pairedDFBackward = new HashMap<>();

        assignPostOrderIndices(tree1Root, -1, tree1);
        assignPostOrderIndices(tree2Root, -1, tree2);
    }

    public double compute(TreeNode tree1Root, TreeNode tree2Root,
            Map<Integer, Integer> forwardMap, Map<Integer, Integer> backwardMap, String Location) {
        Pair<Integer> index = new Pair<>(-1, -1);
        DF.put(index, 0.0);
        DT.put(index, 0.0);
        pairedDTForward.put(index, new HashMap<Integer, Integer>());
        pairedDTBackward.put(index, new HashMap<Integer, Integer>());
        pairedDFForward.put(index, new HashMap<Integer, Integer>());
        pairedDFBackward.put(index, new HashMap<Integer, Integer>());
        for (int i = 0; i < tree1.size(); i++) {
            DF(i, -1);
            DT(i, -1);
        }
        for (int j = 0; j < tree2.size(); j++) {
            DF(-1, j);
            DT(-1, j);
        }
        for (int i = 0; i < tree1.size(); i++) {
            for (int j = 0; j < tree2.size(); j++) {
                DF(i, j);
                DT(i, j);
            }
        }

        double cost = DT(tree1Root.getIndex(), tree2Root.getIndex());
        Map<Integer, Integer> forwardMapIndex = pairedDTForward.get(
                new Pair<>(tree1Root.getIndex(), tree2Root.getIndex()));
        Map<Integer, Integer> backwardMapIndex = pairedDTBackward.get(
                new Pair<>(tree1Root.getIndex(), tree2Root.getIndex()));
        //System.out.println();
        //System.out.println("forward map");
        try {
                FileWriter fMap = new FileWriter(Location + "/forwardmapCO.txt");
                BufferedWriter out1 = new BufferedWriter(fMap);
                for (Map.Entry<Integer, Integer> entry : forwardMapIndex.entrySet()) {
                    TreeNode t1i = tree1.get(entry.getKey());
                    JoinTree.JTNode n1i = (JoinTree.JTNode) t1i;
                    TreeNode t2j = tree2.get(entry.getValue());
                    JoinTree.JTNode n2j = (JoinTree.JTNode) t2j;
                    forwardMap.put(t1i.getID(), t2j.getID());
                    out1.write(t1i.getID() + "," + t2j.getID() + "\n");
                    //System.out.println(entry.getKey() +"|" + t1i.getID() + "|" + n1i.value + "->"  + entry.getValue() + "|" + t2j.getID()+ "|" + n2j.value);
                    //System.out.println(t1i.getID() + "|" + n1i.value + "->"  + t2j.getID()+ "|" + n2j.value);
                }
                out1.close();
            }   catch (IOException e) {
                    System.out.println("Buggy code");
                    e.printStackTrace();
            }

        //System.out.println();
        //System.out.println("backward map");
        try {
                FileWriter bMap = new FileWriter(Location + "/backwardmapCO.txt");
                BufferedWriter out2 = new BufferedWriter(bMap);


                for (Map.Entry<Integer, Integer> entry : backwardMapIndex.entrySet()) {
                    TreeNode t1i = tree1.get(entry.getValue());
                    JoinTree.JTNode n1i = (JoinTree.JTNode) t1i;
                    TreeNode t2j = tree2.get(entry.getKey());
                    JoinTree.JTNode n2j = (JoinTree.JTNode) t2j;
                    backwardMap.put(t2j.getID(), t1i.getID());
                    out2.write(t1i.getID() + "," + t2j.getID() + "\n");
                    //System.out.println(entry.getKey() +"|" + t2j.getID() + "|" + n2j.value + "->"  + entry.getValue() + "|" + t1i.getID() + "|" + n1i.value);
                    //System.out.println(t2j.getID() + "|" + n2j.value + "->"   + t1i.getID() + "|" + n1i.value);
                }
                out2.close();
            }   catch (IOException e) {
                    System.out.println("Buggy code");
                    e.printStackTrace();
            }
        return cost;
    }

    public static void WassersteinDist(Map<Integer, JoinTree.JTNode> jTNodes1, Map<Integer, JoinTree.JTNode> jTNodes2){

      SimpleWeightedGraph<JoinTree.JTNode, DefaultWeightedEdge> weightedGraph = new SimpleWeightedGraph(DefaultWeightedEdge.class);
      List<JoinTree.JTNode> S = new ArrayList<>();
      int numT1Nodes = 0;
      for (JoinTree.JTNode node : jTNodes1.values()) {
        if (node.children.isEmpty()) {
          weightedGraph.addVertex(node);
          S.add(node);
          numT1Nodes++;
        }
      }
      List<JoinTree.JTNode> T = new ArrayList<>();
      int numT2Nodes = 0;
      for (JoinTree.JTNode node : jTNodes2.values()) {
        if (node.children.isEmpty()) {
          weightedGraph.addVertex(node);
          T.add(node);
          numT2Nodes++;
        }
      }
      //int maxSize = Math.max(numT1Nodes, numT2Nodes);
      //System.out.println(maxSize);
      JoinTree jt = new JoinTree();
      for (int i = 0; i < numT2Nodes; i++) {
        JoinTree.JTNode node = T.get(i);
        //System.out.println(node.value);
        double val = (node.value + jTNodes2.get(node.pair).value)/2;
        JoinTree.JTNode dummy = jt.new JTNode (-1, -1, -1, -1, val);
        weightedGraph.addVertex(dummy);
        S.add(dummy);
      }
      for (int i = 0; i < numT1Nodes; i++) {
        JoinTree.JTNode node = S.get(i);
        //System.out.println(node.value);
        double val = (node.value + jTNodes1.get(node.pair).value)/2;
        //System.out.println(node.value);//debug
        JoinTree.JTNode dummy = jt.new JTNode(-1, -1, -1, -1, val);
        weightedGraph.addVertex(dummy);
        T.add(dummy);
      }
      //System.out.println("Sizes: " + S.size() + "  " + T.size());
      //double sqrt2 = Math.sqrt(2);
      for (int i = numT2Nodes; i < T.size(); i++) {
        JoinTree.JTNode node2 = T.get(i);
        double f2 = node2.value;
        //System.out.println(f2);
        for (int j = 0; j < S.size(); j++) {
          JoinTree.JTNode node1 = S.get(j);
          if (j < numT1Nodes){
          double f1 = node1.value;
          double fp1 = jTNodes1.get(node1.pair).value;
          final double weight = Math.max(Math.abs(f1-f2),Math.abs(fp1-f2));
          weightedGraph.addEdge(node1, node2);
          DefaultWeightedEdge edge = weightedGraph.getEdge(node1, node2);
          weightedGraph.setEdgeWeight(edge, weight);
          //System.out.println(f2);
          //System.out.println(f1);
          //System.out.println(fp1);
          //System.out.println(edge);
          //System.out.println(weight);
        }

        if (j >= numT1Nodes){
          final double weight = 0;
          weightedGraph.addEdge(node1, node2);
          DefaultWeightedEdge edge = weightedGraph.getEdge(node1, node2);
          weightedGraph.setEdgeWeight(edge, weight);
          //System.out.println(edge);
          //System.out.println(weight);
      }
      }
    }
      for (int i = numT1Nodes; i < S.size(); i++) {
        JoinTree.JTNode node1 = S.get(i);
        double f1 = node1.value;
        for (int j = 0; j < T.size(); j++) {
          JoinTree.JTNode node2 = T.get(j);
          if (j < numT2Nodes){
          double f2 = node2.value;
          double fp2 = jTNodes2.get(node2.pair).value;
          final double weight = Math.max(Math.abs(f1-f2),Math.abs(f1-fp2));
          weightedGraph.addEdge(node1, node2);
          DefaultWeightedEdge edge = weightedGraph.getEdge(node1, node2);
          weightedGraph.setEdgeWeight(edge, weight);
          //System.out.println(f2);
          //System.out.println(f1);
          //System.out.println(fp2);
          //System.out.println(edge);
          //System.out.println(weight);
        }
         if(j >= numT2Nodes){
           final double weight = 0;
          weightedGraph.addEdge(node1, node2);
          DefaultWeightedEdge edge = weightedGraph.getEdge(node1, node2);
          weightedGraph.setEdgeWeight(edge, weight);
          //System.out.println(edge);
          //System.out.println(weight);
         }
      }
      }
      for (int i = 0; i < numT1Nodes; i++) {
        JoinTree.JTNode node1 = S.get(i);
        double f1 = node1.value;
        double fp1 = jTNodes1.get(node1.pair).value;
        for (int j = 0; j < numT2Nodes; j++) {
          JoinTree.JTNode node2 = T.get(j);
          double f2 = node2.value;
          double fp2 = jTNodes2.get(node2.pair).value;
          final double weight = Math.max(Math.abs(f2-f1),Math.abs(fp2 - fp1));  //cost function
          //System.out.println(weight);
          //System.out.printf("%8d %8d --> %f\n", node1.nodeID, node2.nodeID, weight);
          weightedGraph.addEdge(node1, node2);
          DefaultWeightedEdge edge = weightedGraph.getEdge(node1, node2);
          weightedGraph.setEdgeWeight(edge, weight);
          //System.out.println(f2);
          //System.out.println(f1);
          //System.out.println(fp1);
          //System.out.println(fp2);
          //System.out.println(edge);
          //System.out.println(weight);
        }
      }
      //System.out.println(weightedGraph);
      KuhnMunkresMinimalWeightBipartitePerfectMatching<JoinTree.JTNode, DefaultWeightedEdge> matching =
        new KuhnMunkresMinimalWeightBipartitePerfectMatching<>(weightedGraph, new HashSet<>(S), new HashSet<>(T));
      Set<DefaultWeightedEdge> matchedPairs = matching.getMatching().getEdges();
      //System.out.println(matchedPairs);
      double totalCost = 0;
      for (DefaultWeightedEdge edge : matchedPairs) {
        JoinTree.JTNode node1 = weightedGraph.getEdgeSource(edge);
        JoinTree.JTNode node2 = weightedGraph.getEdgeTarget(edge);
        double cost = 0;
        cost = weightedGraph.getEdgeWeight(edge);
        //System.out.println(edge);
        //System.out.println(cost);
        totalCost += cost;
        //System.out.printf("%8d %8d --> %f %f\n", weightedGraph.getEdgeSource(edge).nodeID,
        //	weightedGraph.getEdgeTarget(edge).nodeID, weightedGraph.getEdgeWeight(edge), cost);
      }
      //System.out.println("Total Cost: " + totalCost);
      /*add code here to calculate total cost.*/
      System.out.println(totalCost);
    }


    public double DF(int i, int j) {
        Pair<Integer> index = new Pair<>(i, j);
        if (DF.containsKey(index)) {
            //System.out.println("Found index while doing DF");
            return DF.get(index);
        }
        //System.out.println("Trying to compute DF(" + i + "," + j + ")");
        if (j == -1) {
            double cost = 0;
            TreeNode t1i = tree1.get(i);
            for (TreeNode child : t1i.getChildren()) {
                cost += DT(child.getIndex(), -1);
            }
            DF.put(index, cost);
            pairedDFForward.put(index, new HashMap<Integer, Integer>());
            pairedDFBackward.put(index, new HashMap<Integer, Integer>());
            return cost;
        }
        if (i == -1) {
            double cost = 0;
            TreeNode t2j = tree2.get(j);
            for (TreeNode child : t2j.getChildren()) {
                cost += DT(-1, child.getIndex());
            }
            DF.put(index, cost);
            pairedDFForward.put(index, new HashMap<Integer, Integer>());
            pairedDFBackward.put(index, new HashMap<Integer, Integer>());
            return cost;
        }

        TreeNode t1i = tree1.get(i);
        TreeNode t2j = tree2.get(j);

        //CASE 1
        double minCost = Double.MAX_VALUE;
        int case1Index = -1;
        for (TreeNode child : t2j.getChildren()) {
            double currCost = DF(i, child.getIndex()) - DF(-1, child.getIndex());
            if (currCost < minCost) {
                minCost = currCost;
                case1Index = child.getIndex();
            }
        }
        double case1Cost = DF(-1, j) + minCost;

        //CASE 2
        minCost = Double.MAX_VALUE;
        int case2Index = -1;
        for (TreeNode child : t1i.getChildren()) {
            double currCost = DF(child.getIndex(), j) - DF(child.getIndex(), -1);
            if (currCost < minCost) {
                minCost = currCost;
                case2Index = child.getIndex();
            }
        }
        double case2Cost = DF(i, -1) + minCost;

        //CASE 3
        double case3Cost = MM(i, j);

        double finalCost;
        if ((case3Cost - 0.00001) <= case2Cost && (case3Cost - 0.00001) <= case1Cost) {
            finalCost = case3Cost;
        } else if (case1Cost < case2Cost) {
            finalCost = case1Cost;
            pairedDFForward.put(index, new HashMap<>(pairedDFForward.get(new Pair<>(i, case1Index))));
            pairedDFBackward.put(index, new HashMap<>(pairedDFBackward.get(new Pair<>(i, case1Index))));
        } else {
            finalCost = case2Cost;
            pairedDFForward.put(index, new HashMap<>(pairedDFForward.get(new Pair<>(case2Index, j))));
            pairedDFBackward.put(index, new HashMap<>(pairedDFBackward.get(new Pair<>(case2Index, j))));
        }
        DF.put(index, finalCost);
        return finalCost;
    }

    public double DT(int i, int j) {
        Pair<Integer> index = new Pair<>(i, j);
        if (DT.containsKey(index)) {
            //System.out.println("Found index while doing DT");
            return DT.get(index);
        }
        //System.out.println("Trying to compute DT(" + i + "," + j + ")");
        if (j == -1) {
            TreeNode t1i = tree1.get(i);
            double cost = DF(i, -1) + costComputer.matchingCost(t1i, null, i, -1);
            DT.put(index, cost);
            pairedDTForward.put(index, new HashMap<Integer, Integer>());
            pairedDTBackward.put(index, new HashMap<Integer, Integer>());
            return cost;
        }
        if (i == -1) {
            TreeNode t2j = tree2.get(j);
            double cost = DF(-1, j) + costComputer.matchingCost(null, t2j, -1, j);
            DT.put(index, cost);
            pairedDTForward.put(index, new HashMap<Integer, Integer>());
            pairedDTBackward.put(index, new HashMap<Integer, Integer>());
            return cost;
        }

        TreeNode t1i = tree1.get(i);
        TreeNode t2j = tree2.get(j);

        //CASE 1
        double minCost = Double.MAX_VALUE;
        int case1Index = -1;
        for (TreeNode child : t2j.getChildren()) {
            double currCost = DT(i, child.getIndex()) - DT(-1, child.getIndex());
            if (currCost < minCost) {
                minCost = currCost;
                case1Index = child.getIndex();
            }
        }
        double case1Cost = DT(-1, j) + minCost;

        //CASE 2
        minCost = Double.MAX_VALUE;
        int case2Index = -1;
        for (TreeNode child : t1i.getChildren()) {
            double currCost = DT(child.getIndex(), j) - DT(child.getIndex(), -1);
            if (currCost < minCost) {
                minCost = currCost;
                case2Index = child.getIndex();
            }
        }
        double case2Cost = DT(i, -1) + minCost;

        //CASE 3
        double case3Cost = DF(i, j) + costComputer.matchingCost(t1i, t2j, i, j);

        double finalCost;
        if ((case3Cost - 0.00001) <= case2Cost && (case3Cost - 0.00001) <= case1Cost) {
            finalCost = case3Cost;
            Map<Integer, Integer> forwardMap = new HashMap<>(pairedDFForward.get(index));
            forwardMap.put(i, j);
            pairedDTForward.put(index, forwardMap);
            Map<Integer, Integer> backwardMap = new HashMap<>(pairedDFBackward.get(index));
            backwardMap.put(j, i);
            pairedDTBackward.put(index, backwardMap);
        } else if (case1Cost < case2Cost) {
            finalCost = case1Cost;
            pairedDTForward.put(index, new HashMap<>(pairedDTForward.get(new Pair<>(i, case1Index))));
            pairedDTBackward.put(index, new HashMap<>(pairedDTBackward.get(new Pair<>(i, case1Index))));
        } else {
            finalCost = case2Cost;
            pairedDTForward.put(index, new HashMap<>(pairedDTForward.get(new Pair<>(case2Index, j))));
            pairedDTBackward.put(index, new HashMap<>(pairedDTBackward.get(new Pair<>(case2Index, j))));
        }
        DT.put(index, finalCost);

        //System.out.println("cost (" + i + "," + j + ") = " + finalCost);
        return finalCost;
    }

    public double MM(int i, int j) {
        SimpleWeightedGraph<TreeNode, DefaultWeightedEdge> weightedGraph
                = new SimpleWeightedGraph(DefaultWeightedEdge.class);
        TreeNode t1i = tree1.get(i);
        TreeNode t2j = tree2.get(j);

        List<TreeNode> S = new ArrayList<>();
        int numChildren1i = 0;
        for (TreeNode child : t1i.getChildren()) {
            weightedGraph.addVertex(child);
            S.add(child);
            numChildren1i++;
        }
        List<TreeNode> T = new ArrayList<>();
        int numChildren2j = 0;
        for (TreeNode child : t2j.getChildren()) {
            weightedGraph.addVertex(child);
            T.add(child);
            numChildren2j++;
        }
        if (numChildren1i == 0 && numChildren2j == 0) {
            pairedDFForward.put(new Pair<>(i, j), new HashMap<Integer, Integer>());
            pairedDFBackward.put(new Pair<>(i, j), new HashMap<Integer, Integer>());
            return 0;
        }
        for (int k = 0; k < numChildren2j; k++) {
            TreeNode dummy = new DummyTreeNode();
            weightedGraph.addVertex(dummy);
            S.add(dummy);
        }
        for (int k = 0; k < numChildren1i; k++) {
            TreeNode dummy = new DummyTreeNode();
            weightedGraph.addVertex(dummy);
            T.add(dummy);
        }
        for (int k = 0; k < S.size(); k++) {
            TreeNode node1 = S.get(k);
            for (int l = 0; l < T.size(); l++) {
                TreeNode node2 = T.get(l);
                double weight = DT(node1.getIndex(), node2.getIndex());
                DefaultWeightedEdge edge = weightedGraph.addEdge(node1, node2);
                weightedGraph.setEdgeWeight(edge, weight);
            }
        }
        KuhnMunkresMinimalWeightBipartitePerfectMatching<TreeNode, DefaultWeightedEdge> matching
                = new KuhnMunkresMinimalWeightBipartitePerfectMatching<>(weightedGraph, new HashSet<>(S), new HashSet<>(T));
        Map<Integer, Integer> forwardMap = new HashMap<>();
        Map<Integer, Integer> backwardMap = new HashMap<>();
        for (DefaultWeightedEdge edge : matching.getMatching().getEdges()) {
            TreeNode n1 = weightedGraph.getEdgeSource(edge);
            TreeNode n2 = weightedGraph.getEdgeTarget(edge);
            if (n1.getIndex() < 0 && n2.getIndex() < 0) {
                continue;
            } else {
                forwardMap.putAll(pairedDTForward.get(new Pair<>(n1.getIndex(), n2.getIndex())));
                backwardMap.putAll(pairedDTBackward.get(new Pair<>(n1.getIndex(), n2.getIndex())));
            }
        }
        pairedDFForward.put(new Pair<>(i, j), forwardMap);
        pairedDFBackward.put(new Pair<>(i, j), backwardMap);
        double totalCost = matching.getMatching().getWeight();
        return totalCost;
    }

    private int assignPostOrderIndices(TreeNode root, int index, List<TreeNode> treeList) {
        int lastIndex = index;
        for (TreeNode child : root.getChildren()) {
            lastIndex = assignPostOrderIndices(child, lastIndex, treeList);
        }

        root.setIndex(lastIndex + 1);
        treeList.add(root);
        return lastIndex + 1;
    }

    public static class ContourTreePair implements NodeMatchingCostComputer {

        ContourTree tree1, tree2;

        public ContourTreePair(ContourTree tree1, ContourTree tree2) {
            this.tree1 = tree1;
            this.tree2 = tree2;
        }

        @Override
        public double matchingCost(TreeNode t1i, TreeNode t2j, int indexI, int indexJ) {
            if (t1i == null) {
                if (t2j == null) {
                    return 0;
                }
                ContourTree.Branch br = (ContourTree.Branch) t2j;
                return br.pers;
            } else if (t2j == null) {
                ContourTree.Branch br = (ContourTree.Branch) t1i;
                return br.pers;
            } else {
                ContourTree.Branch br1i = (ContourTree.Branch) t1i;
                ContourTree.CTNode in1 = tree1.rgNodes.get(tree1.nodeIndexIDMap.get(br1i.n1));
                ContourTree.CTNode in2 = tree1.rgNodes.get(tree1.nodeIndexIDMap.get(br1i.n2));

                ContourTree.Branch br2j = (ContourTree.Branch) t2j;
                ContourTree.CTNode jn1 = tree2.rgNodes.get(tree2.nodeIndexIDMap.get(br2j.n1));
                ContourTree.CTNode jn2 = tree2.rgNodes.get(tree2.nodeIndexIDMap.get(br2j.n2));

                double ilow = in1.val;
                double ihigh = in2.val;
                if (ilow > ihigh) {
                    ilow = in2.val;
                    ihigh = in1.val;
                }

                double jlow = jn1.val;
                double jhigh = jn2.val;
                if (jlow > jhigh) {
                    jlow = jn2.val;
                    jhigh = jn1.val;
                }

                if (ihigh < jlow || ilow > jhigh) {
                    // intervals don't intersect
                    return jhigh - jlow + ihigh - ilow;
                } else {
                    double intersectLow = Math.max(ilow, jlow);
                    double intersectHigh = Math.min(ihigh, jhigh);
                    double symmDiff = jhigh - jlow + ihigh - ilow - 2 * (intersectHigh - intersectLow);
                    return symmDiff;
                }
            }
        }
    }

    public static class ContourTreePairL1 implements NodeMatchingCostComputer {

        ContourTree tree1, tree2;

        public ContourTreePairL1(ContourTree tree1, ContourTree tree2) {
            this.tree1 = tree1;
            this.tree2 = tree2;
        }

        @Override
        public double matchingCost(TreeNode t1i, TreeNode t2j, int indexI, int indexJ) {
            if (t1i == null) {
                if (t2j == null) {
                    return 0;
                }
                ContourTree.Branch br = (ContourTree.Branch) t2j;
                return br.pers / 2;
            } else if (t2j == null) {
                ContourTree.Branch br = (ContourTree.Branch) t1i;
                return br.pers / 2;
            } else {
                ContourTree.Branch br1i = (ContourTree.Branch) t1i;
                ContourTree.CTNode in1 = tree1.rgNodes.get(tree1.nodeIndexIDMap.get(br1i.n1));
                ContourTree.CTNode in2 = tree1.rgNodes.get(tree1.nodeIndexIDMap.get(br1i.n2));

                ContourTree.Branch br2j = (ContourTree.Branch) t2j;
                ContourTree.CTNode jn1 = tree2.rgNodes.get(tree2.nodeIndexIDMap.get(br2j.n1));
                ContourTree.CTNode jn2 = tree2.rgNodes.get(tree2.nodeIndexIDMap.get(br2j.n2));

                double ilow = in1.val;
                double ihigh = in2.val;
                if (ilow > ihigh) {
                    ilow = in2.val;
                    ihigh = in1.val;
                }

                double jlow = jn1.val;
                double jhigh = jn2.val;
                if (jlow > jhigh) {
                    jlow = jn2.val;
                    jhigh = jn1.val;
                }
                double linf = Math.max(Math.abs(ihigh - jhigh), Math.abs(ilow - jlow));

                return Math.min(linf, (br1i.pers + br2j.pers) / 2.0);
            }
        }
    }

    public static class JoinTreePair implements NodeMatchingCostComputer {

        JoinTree tree1, tree2;

        public JoinTreePair(JoinTree tree1, JoinTree tree2) {
            this.tree1 = tree1;
            this.tree2 = tree2;
        }

        public double matchingCostOld(TreeNode t1i, TreeNode t2j) {
            if (t1i == null) {
                if (t2j == null) {
                    return 0;
                }
                JoinTree.JTNode n2j = (JoinTree.JTNode) t2j;
                JoinTree.JTNode pair = tree2.jTNodes.get(n2j.pair);
                return Math.abs(n2j.value - pair.value);
            } else if (t2j == null) {
                JoinTree.JTNode n1i = (JoinTree.JTNode) t1i;
                JoinTree.JTNode pair = tree1.jTNodes.get(n1i.pair);
                return Math.abs(n1i.value - pair.value);
            } else {
                JoinTree.JTNode n1i = (JoinTree.JTNode) t1i;
                JoinTree.JTNode n2j = (JoinTree.JTNode) t2j;
                return Math.abs(n1i.value - n2j.value);
            }
        }

        @Override
        public double matchingCost(TreeNode t1i, TreeNode t2j, int indexI, int indexJ) {
            if (t1i == null) {
                if (t2j == null) {
                    return 0;
                }

                double otherCost = 0.0;
                JoinTree.JTNode n2j = (JoinTree.JTNode) t2j;
                JoinTree.JTNode pair = tree2.jTNodes.get(n2j.pair);
                otherCost = Math.abs(n2j.value - pair.value) / 2.0;
                //System.out.println("other cost (" + indexI + "," + indexJ + ") = " + otherCost);
                return otherCost;

            } else if (t2j == null) {

                double otherCost = 0.0;
                JoinTree.JTNode n1i = (JoinTree.JTNode) t1i;
                JoinTree.JTNode pair = tree1.jTNodes.get(n1i.pair);
                otherCost = Math.abs(n1i.value - pair.value) / 2.0;
                //System.out.println("other cost (" + indexI + "," + indexJ + ") = " + otherCost);
                return otherCost;

            } else {

                double otherCost = 0.0;
                JoinTree.JTNode n1i = (JoinTree.JTNode) t1i;
                JoinTree.JTNode pair1i = tree1.jTNodes.get(n1i.pair);
                JoinTree.JTNode n2j = (JoinTree.JTNode) t2j;
                JoinTree.JTNode pair2j = tree2.jTNodes.get(n2j.pair);
                double diff1 = Math.abs(n1i.value - n2j.value) + Math.abs(pair1i.value - pair2j.value);
                double diff2 = Math.abs(pair1i.value - n1i.value) + Math.abs(pair2j.value - n2j.value);
                //double linf12 = Math.max(Math.abs(n1i.value - n2j.value), Math.abs(pair1i.value - pair2j.value));
                //double linf1 = Math.abs(n1i.value - pair1i.value) / 2.0;
                //double linf2 = Math.abs(n2j.value - pair2j.value) / 2.0;
                otherCost = Math.min(diff1, diff2) / 2.0;
                //System.out.println("other cost (" + indexI + "," + indexJ + ") = " + otherCost);
                return otherCost;

            }
        }
    }

    public enum Critcality {

        MINIMUM,
        MAXIMUM,
        SADDLE,
        UNKNOWN
    }

    public static class JoinTree {

        public class JTNode implements TreeNode {

            int nodeID;
            int pair;
            int parent;
            int maxNode;
            double value;
            List<JTNode> children;
            List<Integer> childrenIDs;
            Critcality critcality;
            int index = -1;
            boolean simplify = false;

            public JTNode(int nodeID, int pair, int parent, int maxNode, double value) {
                this.nodeID = nodeID;
                this.pair = pair;
                this.parent = parent;
                this.maxNode = maxNode;
                this.value = value;
                children = new ArrayList<>();
                childrenIDs = new ArrayList<>();
                this.critcality = Critcality.UNKNOWN;
            }

            @Override
            public int getIndex() {
                return index;
            }

            @Override
            public void setIndex(int i) {
                this.index = i;
            }

            @Override
            public List<? extends TreeNode> getChildren() {
                return children;
            }

            @Override
            public TreeNode getParent() {
                return jTNodes.get(parent);
            }

            @Override
            public String toString() {
                return "" + nodeID + children;
            }

            @Override
            public int getID() {
                return nodeID;
            }
        }
        Map<Integer, JTNode> jTNodes;
        Map<Integer, JTNode> stabJTNodes;
        JTNode root;
        List<JTNode> stabilizedJT;
        JTNode stabilizedJTRoot;

        public JoinTree() {
            jTNodes = new HashMap<>();
        }

        public void readJoinTree(String jtFileName) throws FileNotFoundException {
            jTNodes.clear();
            root = null;
            try (Scanner scanner = new Scanner(new File(jtFileName))) {
                int numNodes = scanner.nextInt();
                for (int i = 0; i < numNodes; i++) {
                    int id = scanner.nextInt();
                    int parent = scanner.nextInt();
                    int pair = scanner.nextInt();
                    int maxNode = scanner.nextInt();
                    double val = scanner.nextDouble();
                    JTNode node = new JTNode(id, pair, parent, maxNode, val);
                    int numChildren = scanner.nextInt();
                    for (int j = 0; j < numChildren; j++) {
                        node.childrenIDs.add(scanner.nextInt());
                    }
                    jTNodes.put(id, node);
                }
            }
            for (JTNode node : jTNodes.values()) {
                for (int childID : node.childrenIDs) {
                    node.children.add(jTNodes.get(childID));
                }
                if (!jTNodes.containsKey(node.parent)) {
                    node.critcality = Critcality.MINIMUM;
                    root = node;
                } else if (node.children.isEmpty()) {
                    node.critcality = Critcality.MAXIMUM;
                } else {
                    node.critcality = Critcality.SADDLE;
                }
            }
        }

        public void stabilize(double thres) {
            stabilizedJT = new ArrayList<>();
            stabJTNodes = new HashMap<>();
            Map<Integer, List<JTNode>> newChildrenLists = new HashMap<>();
            Map<Integer, List<JTNode>> mergedSaddlesLists = new HashMap<>();
            stabilizeJTNode(root, thres, newChildrenLists, mergedSaddlesLists);
            int currIndex = 0;
            Map<Integer, Integer> oldNewMap = new HashMap<>();
            for (JTNode origNode : jTNodes.values()) {
                if (!origNode.simplify) {
                    JTNode stabilizedNode = new JTNode(origNode.nodeID, origNode.pair,
                            origNode.parent, origNode.maxNode, origNode.value);
                    stabilizedNode.critcality = origNode.critcality;
                    oldNewMap.put(origNode.nodeID, currIndex);
                    stabJTNodes.put(stabilizedNode.nodeID, stabilizedNode);
                    if (origNode == root) {
                        stabilizedJTRoot = stabilizedNode;
                    }
                    currIndex++;
                    stabilizedJT.add(stabilizedNode);
                    if (origNode.critcality == Critcality.SADDLE) {
                        List<JTNode> mergedSaddles = mergedSaddlesLists.get(origNode.nodeID);
                        if (!mergedSaddles.isEmpty()) {
                            double maxPers = Math.abs(jTNodes.get(origNode.pair).value - origNode.value);
                            int maxID = origNode.nodeID;
                            for (JTNode saddle : mergedSaddles) {
                                int pair = saddle.pair;
                                double pairValue = jTNodes.get(pair).value;
                                double saddleValue = saddle.value;
                                double persistence = Math.abs(pairValue - saddleValue);
                                if (persistence > maxPers) {
                                    maxPers = persistence;
                                    maxID = saddle.nodeID;
                                }
                            }
                            JTNode highPersSaddle = jTNodes.get(maxID);
                            if (maxID != origNode.nodeID) {
                                //stabJTNodes.remove(stabilizedNode.nodeID);
                                //stabilizedNode.nodeID = highPersSaddle.nodeID;
                                stabilizedNode.pair = highPersSaddle.pair;
                                stabilizedNode.value = highPersSaddle.value;
                                //stabJTNodes.put(stabilizedNode.nodeID, stabilizedNode);
                            }
                        }
                    }
                }
            }
            /*for (JTNode node : stabJTNodes.values()) {
                if (node.critcality == Critcality.SADDLE || node.critcality == Critcality.MINIMUM) {
                    List<JTNode> mergedSaddles = mergedSaddlesLists.get(node.nodeID);
                    if (!mergedSaddles.isEmpty()) {
                        for (JTNode saddle : mergedSaddles) {
                            int pair = saddle.pair;
                            stabJTNodes.get(pair).pair = node.nodeID; //comment/uncomment based on wanting to pair max
                            //System.err.println("Updated: " + pair);
                        }
                    }
                }
            }*/
            for (JTNode stabilizedNode : stabilizedJT) {
                JTNode origNode = jTNodes.get(stabilizedNode.nodeID);
                List<Integer> childIndices = new ArrayList<>();
                List<JTNode> children = new ArrayList<>();
                for (JTNode child : newChildrenLists.get(origNode.nodeID)) {
                    if (!child.simplify) {
                        childIndices.add(child.nodeID);
                        JTNode simChild = stabilizedJT.get(oldNewMap.get(child.nodeID));
                        children.add(simChild);
                        simChild.parent = stabilizedNode.nodeID;
                    }
                }
                stabilizedNode.childrenIDs = childIndices;
                stabilizedNode.children = children;
            }
        }

        boolean stabilizeJTNode(JTNode root, double thres, Map<Integer, List<JTNode>> newChildrenLists,
                Map<Integer, List<JTNode>> mergedSaddlesLists) {
            List<JTNode> newChildren = new ArrayList<>();
            List<JTNode> mergedSaddles = new ArrayList<>();
            for (JTNode child : root.children) {
                if (stabilizeJTNode(child, thres, newChildrenLists, mergedSaddlesLists)) {
                    newChildren.addAll(newChildrenLists.get(child.nodeID));
                    mergedSaddles.addAll(mergedSaddlesLists.get(child.nodeID));
                } else {
                    newChildren.add(child);
                }
            }

            newChildrenLists.put(root.nodeID, newChildren);
            boolean shouldSimplify = false;
            if (root.critcality == Critcality.SADDLE) {
                double myVal = jTNodes.get(root.nodeID).value;
                double parentVal = jTNodes.get(root.parent).value;
                shouldSimplify = Math.abs(myVal - parentVal) < thres;
                if (jTNodes.get(root.parent).critcality == Critcality.MINIMUM) {
                    shouldSimplify = false;
                }
            }
            root.simplify = shouldSimplify;
            if (shouldSimplify) {
                mergedSaddles.add(root);
            }
            mergedSaddlesLists.put(root.nodeID, mergedSaddles);
            return shouldSimplify;
        }
    }

    public static class ContourTree {

        public class CTNode implements Comparable<CTNode> {

            int index;
            int id;
            double val;
            Critcality critType;
            List<CTEdge> upEdges, downEdges;
            boolean valid = true;
            int branchID;

            public CTNode(int index, int id, double val, Critcality critType) {
                this.index = index;
                this.id = id;
                this.val = val;
                this.critType = critType;
                this.upEdges = new ArrayList<>();
                this.downEdges = new ArrayList<>();
            }

            boolean addEdge(CTEdge edge, Map<Integer, CTNode> rgNodes) {
                if (id != edge.n1 && id != edge.n2) {
                    return false;
                }
                if (upEdges.contains(edge) || downEdges.contains(edge)) {
                    return false;
                }
                int otherNodeIndex = (id == edge.n1) ? edge.n2 : edge.n1;
                if (rgNodes.containsKey(otherNodeIndex)) {
                    CTNode otherNode = rgNodes.get(otherNodeIndex);
                    if (otherNode.id == id) {
                        System.err.println("Edge connecting node to itself?");
                        return false;
                    }
                    if (isLessThan(otherNode)) {
                        upEdges.add(edge);
                    } else {
                        downEdges.add(edge);
                    }
                    return true;
                } else {
                    return false;
                }
            }

            CTNode getOtherNode(CTEdge edge, Map<Integer, CTNode> rgNodes) {
                if (id != edge.n1 && id != edge.n2) {
                    return null;
                }
                int otherNodeIndex = (id == edge.n1) ? edge.n2 : edge.n1;
                if (rgNodes.containsKey(otherNodeIndex)) {
                    return rgNodes.get(otherNodeIndex);
                }
                return null;
            }

            boolean isLessThan(CTNode other) {
                if (index < other.index) {
                    return true;
                }
                return false;
            }

            boolean isJoinSaddle() {
                if (critType != Critcality.SADDLE) {
                    return false;
                }
                if (upEdges.size() > 1) {
                    return true;
                }
                return false;
            }

            boolean isSplitSaddle() {
                if (critType != Critcality.SADDLE) {
                    return false;
                }
                if (downEdges.size() > 1) {
                    return true;
                }
                return false;
            }

            boolean isMultiSaddle() {
                return (downEdges.size() > 1 && upEdges.size() > 1);
            }

            boolean isProblematic() {
                return (critType == Critcality.SADDLE && (upEdges.isEmpty()
                        || downEdges.isEmpty()))
                        || ((critType == Critcality.MINIMUM) && upEdges.isEmpty())
                        || ((critType == Critcality.MAXIMUM) && downEdges.isEmpty());
            }

            @Override
            public int compareTo(CTNode o) {
                if (this.id == o.id) {
                    return 0;
                }
                return isLessThan(o) ? -1 : 1;
            }
        }

        public class CTEdge {

            int index;
            int n1, n2;
            int brachID;
            int branchType;

            public CTEdge(int index, int n1, int n2) {
                this.index = index;
                this.n1 = n1;
                this.n2 = n2;
            }
        }

        public class Branch implements TreeNode {

            int id;
            int n1, n2;
            double pers;
            int parent;
            List<Integer> edges;
            List<Integer> childIndices;
            List<Branch> children;
            boolean isRoot = false;
            boolean isMaxBranch = false;
            int index = -1;
            boolean simplify = false;
            int origID;

            public Branch(int id, int n1, int n2, double pers, int parent,
                    List<Integer> edges, List<Integer> children) {
                this.id = id;
                this.n1 = n1;
                this.n2 = n2;
                this.pers = pers;
                this.parent = parent;
                this.edges = edges;
                this.childIndices = children;
                this.children = new ArrayList<>();
                origID = id;
            }

            @Override
            public int getIndex() {
                return index;
            }

            @Override
            public void setIndex(int i) {
                this.index = i;
            }

            @Override
            public List<? extends TreeNode> getChildren() {
                return children;
            }

            @Override
            public TreeNode getParent() {
                if (parent >= 0) {
                    return branches.get(parent);
                }
                return null;
            }

            CTNode getSaddle() {
                CTNode node1 = rgNodes.get(nodeIndexIDMap.get(n1));
                if (node1.isJoinSaddle() || node1.isSplitSaddle()) {
                    return node1;
                }
                CTNode node2 = rgNodes.get(nodeIndexIDMap.get(n2));
                if (node2.isJoinSaddle() || node2.isSplitSaddle()) {
                    return node2;
                }
                return null;
            }

            @Override
            public String toString() {
                StringBuilder sb = new StringBuilder();
                sb.append(getIndex());
                sb.append("(").append(origID).append(")");
                if (getChildren().isEmpty()) {
                    sb.append(" : []");
                } else {
                    sb.append(" : [");
                }
                for (Branch child : children) {
                    sb.append(child.getIndex());
                    sb.append("(").append(child.origID).append(")");
                    sb.append(",");
                }
                if (!getChildren().isEmpty()) {
                    sb.replace(sb.length() - 1, sb.length(), "]");
                }
                return sb.toString();
            }

            @Override
            public int getID() {
                return id;
            }
        }
        Map<Integer, CTNode> rgNodes;
        Map<Integer, Integer> nodeIndexIDMap;
        List<CTEdge> rgEdges;
        int rootBranch;
        int lastBranch;
        List<Branch> branches;
        List<Branch> simplifiedBranches;
        Branch simplifiedBDRoot;

        public ContourTree() {
            rgNodes = new HashMap<>();
            nodeIndexIDMap = new HashMap<>();
            rgEdges = new ArrayList<>();
            branches = new ArrayList<>();
        }

        public void readRGFile(String rgFileName) throws FileNotFoundException {
            rgNodes.clear();
            rgEdges.clear();
            nodeIndexIDMap.clear();
            try (Scanner scanner = new Scanner(new File(rgFileName))) {
                int numNodes = scanner.nextInt();
                int numEdges = scanner.nextInt();
                for (int i = 0; i < numNodes; i++) {
                    int id = scanner.nextInt();
                    double val = scanner.nextDouble();
                    String crit = scanner.next();
                    Critcality critType = Critcality.UNKNOWN;
                    switch (crit) {
                        case "MINIMA":
                            critType = Critcality.MINIMUM;
                            break;
                        case "MAXIMA":
                            critType = Critcality.MAXIMUM;
                            break;
                        case "SADDLE":
                            critType = Critcality.SADDLE;
                            break;
                    }
                    rgNodes.put(id, new CTNode(i, id, val, critType));
                    nodeIndexIDMap.put(i, id);
                }
                for (int i = 0; i < numEdges; i++) {
                    int n1 = scanner.nextInt();
                    int n2 = scanner.nextInt();
                    CTEdge edge = new CTEdge(i, n1, n2);
                    rgEdges.add(edge);
                }
                for (CTEdge rgEdge : rgEdges) {
                    rgNodes.get(rgEdge.n1).addEdge(rgEdge, rgNodes);
                    rgNodes.get(rgEdge.n2).addEdge(rgEdge, rgNodes);
                }
            }
            for (CTNode node : rgNodes.values()) {
                if (node.isProblematic()) {
                    System.out.println("Problematic " + node.id);
                }
            }
        }

        public void readBDFile(String bdFileName) throws FileNotFoundException {
            branches.clear();
            try (Scanner scanner = new Scanner(new File(bdFileName))) {
                int numBranches = scanner.nextInt();
                rootBranch = scanner.nextInt();
                for (int i = 0; i < numBranches; i++) {
                    int id = scanner.nextInt();
                    int n1 = scanner.nextInt();
                    int n2 = scanner.nextInt();
                    double pers = scanner.nextDouble();
                    int parent = scanner.nextInt();
                    int numEdges = scanner.nextInt();
                    List<Integer> edges = new ArrayList<>();
                    for (int j = 0; j < numEdges; j++) {
                        int edge = scanner.nextInt();
                        edges.add(edge);
                    }
                    int numChildren = scanner.nextInt();
                    List<Integer> children = new ArrayList<>();
                    for (int j = 0; j < numChildren; j++) {
                        children.add(scanner.nextInt());
                    }
                    Branch branch = new Branch(id, n1, n2, pers, parent, edges, children);
                    branches.add(branch);
                    branch.isMaxBranch = rgNodes.get(nodeIndexIDMap.get(n2)).critType == Critcality.MAXIMUM
                            || rgNodes.get(nodeIndexIDMap.get(n1)).critType == Critcality.MAXIMUM;
                }
                branches.get(rootBranch).isRoot = true;

                int numEdges = scanner.nextInt();
                for (int i = 0; i < numEdges; i++) {
                    int listSize = scanner.nextInt();
                    for (int j = 0; j < listSize; j++) {
                        scanner.nextInt();
                    }
                }

                int numNodes = scanner.nextInt();
                for (int i = 0; i < numNodes; i++) {
                    int nodeValid = scanner.nextInt();
                    CTNode node = rgNodes.get(nodeIndexIDMap.get(i));
                    node.valid = nodeValid == 1;
                }
                lastBranch = scanner.nextInt();

                for (int i = 0; i < numBranches; i++) {
                    Branch branch = branches.get(i);
                    for (int childIndex : branch.childIndices) {
                        branch.children.add(branches.get(childIndex));
                    }
                }
            }
        }

        public void simplifyBD(double thres) {
            simplifiedBranches = new ArrayList<>();
            Map<Integer, List<Branch>> newChildrenLists = new HashMap<>();
            convertToJoinTreeBD(branches.get(rootBranch), newChildrenLists);
            int id = 0;
            Map<Integer, Integer> newOldMap = new HashMap<>();
            Map<Integer, Integer> oldNewMap = new HashMap<>();
            for (int i = 0; i <= lastBranch; i++) {
                Branch branch = branches.get(i);
                if (!branch.simplify) {
                    Branch simplifiedBranch = new Branch(id, branch.n1, branch.n2, branch.pers, -1, null, null);
                    simplifiedBranch.isMaxBranch = true;
                    newOldMap.put(id, branch.id);
                    oldNewMap.put(branch.id, id);
                    simplifiedBranch.origID = branch.id;
                    if (branch.isRoot) {
                        simplifiedBDRoot = simplifiedBranch;
                        simplifiedBranch.isRoot = true;
                    }
                    id++;
                    simplifiedBranches.add(simplifiedBranch);
                }
            }
            for (Branch simplifiedBranch : simplifiedBranches) {
                Branch origBranch = branches.get(newOldMap.get(simplifiedBranch.id));
                List<Integer> childIndices = new ArrayList<>();
                List<Branch> children = new ArrayList<>();
                for (Branch child : newChildrenLists.get(origBranch.id)) {
                    if (oldNewMap.containsKey(child.id)) {
                        int childIndex = oldNewMap.get(child.id);
                        childIndices.add(childIndex);
                        Branch simChild = simplifiedBranches.get(childIndex);
                        children.add(simChild);
                        simChild.parent = simplifiedBranch.id;
                    }
                }

                simplifiedBranch.childIndices = childIndices;
                simplifiedBranch.children = children;
            }
            newChildrenLists.clear();
            List<Branch> branchesToBeAdded = new ArrayList<>();
            simplifyJoinTreeBD(simplifiedBDRoot, branchesToBeAdded, newChildrenLists, thres);
            newChildrenLists.get(simplifiedBDRoot.id).addAll(branchesToBeAdded);
            for (Branch simplifiedBranch : simplifiedBranches) {
                List<Integer> childIndices = new ArrayList<>();
                List<Branch> children = new ArrayList<>();
                for (Branch child : newChildrenLists.get(simplifiedBranch.id)) {
                    int childIndex = child.id;
                    childIndices.add(childIndex);
                    Branch simChild = simplifiedBranches.get(childIndex);
                    children.add(simChild);
                    simChild.parent = simplifiedBranch.id;
                }

                simplifiedBranch.childIndices = childIndices;
                simplifiedBranch.children = children;
            }
        }

        boolean convertToJoinTreeBD(Branch root, Map<Integer, List<Branch>> newChildrenLists) {
            List<Branch> newChildren = new ArrayList<>();
            for (Branch child : root.children) {
                if (convertToJoinTreeBD(child, newChildrenLists)) {
                    newChildren.addAll(newChildrenLists.get(child.id));
                } else {
                    newChildren.add(child);
                }
            }
            newChildrenLists.put(root.id, newChildren);
            boolean shouldSimplify = !root.isMaxBranch;
            root.simplify = shouldSimplify;
            return shouldSimplify;
        }

        boolean simplifyJoinTreeBD(Branch root, List<Branch> branchesToBeAdded,
                Map<Integer, List<Branch>> newChildrenLists, double thres) {
            List<Branch> newChildren = new ArrayList<>();
            List<Branch> listToTest = new ArrayList<>();
            for (Branch child : root.children) {
                List<Branch> childsBranches = new ArrayList<>();
                if (!simplifyJoinTreeBD(child, branchesToBeAdded, newChildrenLists, thres)) {
                    newChildren.add(child);
                } else {
                    listToTest.addAll(childsBranches);
                    branchesToBeAdded.add(child);
                }
            }
            for (Branch br : listToTest) {
                if (!br.isRoot) {
                    double myVal = br.getSaddle().val;
                    Branch parent = simplifiedBranches.get(root.parent);
                    if (!parent.isRoot) {
                        double parentVal = parent.getSaddle().val;
                        boolean shouldSimplify = Math.abs(myVal - parentVal) < thres;
                        if (shouldSimplify) {
                            branchesToBeAdded.add(br);
                        } else {
                            newChildren.add(br);
                        }
                    } else {
                        newChildren.add(br);
                    }
                } else {
                    newChildren.add(br);
                }
            }
            boolean shouldSimplify = false;
            if (!root.isRoot) {
                double myVal = root.getSaddle().val;
                Branch parent = simplifiedBranches.get(root.parent);
                if (!parent.isRoot) {
                    double parentVal = parent.getSaddle().val;
                    shouldSimplify = Math.abs(myVal - parentVal) < thres;
                }
            }
            if (shouldSimplify) {
                //branchesToBeAdded.add(root);
            }
            newChildrenLists.put(root.id, newChildren);
            return shouldSimplify;
        }
    }
}

#include "TreeSkeleton.h"
#include <iostream>
#include <algorithm>
#include <math.h>
#include <utility>
#include <fstream>

namespace mergeTrees {

    TreeSkeleton :: TreeSkeleton (std :: vector <nodeData> nodesFromFile, int nodeID, TreeSkeleton* Parent, std :: string filename) {

        nodeData current;
        for (nodeData x : nodesFromFile) {
            if (x.node_ID == nodeID) {
                current = x;
            }
        }

        this -> Name = filename;
        this -> parent_ID = current.parent_ID;
        this -> node_ID = current.node_ID;
        this -> childrenIDs = current.childrenIDs;
        this -> parent = Parent;
        this -> fVal = current.fVal;

        for (int x : this -> childrenIDs) {
            TreeSkeleton* stp = new TreeSkeleton(nodesFromFile, x, this, filename);
            this -> children.push_back(stp);
        }
    }

    TreeSkeleton :: TreeSkeleton (int nodeID, int parentID, std :: vector <TreeSkeleton*> children, TreeSkeleton* parent, double fVal) {
        this -> node_ID = nodeID;
        this -> parent_ID = parentID;
        this -> childrenIDs = childrenIDs;
        this -> parent = parent;
        this -> fVal = fVal;
        this -> children = children; 
    }

    void TreeSkeleton :: recursive_traversal () {
        std :: cout << node_ID << " " << fVal << " " << std :: endl;
        for (TreeSkeleton* child : this -> children) {
            child -> recursive_traversal();
        }
    }

    TreeSkeleton* TreeSkeleton :: locateRoot () {
        if (this -> parent == NULL)
            return this;
        else
            return this -> parent -> locateRoot();
    }

    int TreeSkeleton :: numberOfChildren() {
        return this -> children.size();
    }

    int TreeSkeleton :: number_of_nodes () {
        int sum = 0;
        for (TreeSkeleton* child : this -> children) {
            sum = sum + child -> number_of_nodes();
        }
        sum = sum + 1;
        return sum;
    }

    int TreeSkeleton :: depth () {
        if (this -> numberOfChildren() == 0) {
            return 1;
        }
        else {
            int max_depth = 0;
            for (TreeSkeleton* child : this -> children) {
                int temp_max = child -> depth();
                if (temp_max >= max_depth)
                    max_depth = temp_max;
            }
            return max_depth + 1;
        }
    }

    int TreeSkeleton :: countFeatures () {
        if (this -> numberOfChildren() == 0) {
            return 1;
        }
        else {
            int sum = 0;
            for (TreeSkeleton* child : this -> children) {
                sum = sum + child -> countFeatures();
            }
            return sum;
        }
    }

    int TreeSkeleton :: countSaddles () {
        return this -> number_of_nodes() - this -> countFeatures() - 1;
    }

    bool TreeSkeleton :: isJoinTree () {
        bool baseval = true;
        for (TreeSkeleton* child : this -> children) {
            bool isCT = child -> isJoinTree();
            baseval = baseval and isCT;
        }

        for (TreeSkeleton* child : this -> children) {
            if (this -> fVal < child -> fVal){
                bool isCT = false;
                baseval = baseval and isCT;
            }
        }
        return baseval; 
    }

    bool TreeSkeleton :: isSplitTree () {
        bool baseval = true;
        for (TreeSkeleton* child : this -> children) {
            bool isCT = child -> isSplitTree();
            baseval = baseval and isCT;
        }

        for (TreeSkeleton* child : this -> children) {
            if (this -> fVal > child -> fVal){
                bool isCT = false;
                baseval = baseval and isCT;
            }
        }
        return baseval; 
    }

    std :: string TreeSkeleton :: classify () {
        if (this -> isJoinTree())
            return "JT";
        else if (this -> isSplitTree())
            return "ST";
        else
            return "None";
    }

    TreeSkeleton* TreeSkeleton :: locateNode (int nodeID) {
        if (this -> node_ID == nodeID) {
            return this;
        }
        else {
            for (TreeSkeleton* child : this -> children) {
                if (child -> locateNode (nodeID) != NULL) {
                    return child -> locateNode (nodeID);
                }
            }
            return NULL;
        }
    }

    std :: vector <TreeSkeleton*> TreeSkeleton :: path_to_root (int nodeID) {
        TreeSkeleton* node =  this -> locateNode (nodeID);
        if (node != NULL) {
            if (node -> parent == NULL) {
                return std :: vector <TreeSkeleton*> (1, node);
            }
            else {
                std :: vector <TreeSkeleton*> expath = path_to_root(node -> parent -> node_ID);
                expath.insert(expath.begin(), node);
                return expath;
            }
        }
        else {
            return std :: vector <TreeSkeleton*> ();
        }
    }

    TreeSkeleton* TreeSkeleton :: lca (int nodeID1, int nodeID2) {

        TreeSkeleton* node1 = this -> locateNode(nodeID1);
        TreeSkeleton* node2 = this -> locateNode(nodeID2);

        std :: vector <TreeSkeleton*> path1 = this -> path_to_root (node1 -> node_ID);
        std :: vector <TreeSkeleton*> path2 = this -> path_to_root (node2 -> node_ID);
        
        std :: vector <int> nodes1 = std :: vector <int> ();
        std :: vector <int> nodes2 = std :: vector <int> ();

        for (TreeSkeleton* pp1 : path1)
            nodes1.push_back (pp1 -> node_ID);
        for (TreeSkeleton* pp2 : path2)
            nodes2.push_back (pp2 -> node_ID);

        std :: reverse(nodes1.begin(), nodes1.end());
        std :: reverse(nodes2.begin(), nodes2.end());
        
        int temp;

        if (nodes2.size() >= nodes1.size()) {
            for (int i = 0; i < nodes1.size(); i += 1) {
                if (i < nodes1.size()) {
                    if (nodes2[i] != nodes1[i]) {
                        temp = nodes1[i - 1];
                        break;
                    }
                    else {
                        temp = nodes1[i];
                    }
                }
                else {
                    temp = nodes1[i];
                }
            }
        }

        else if (nodes1.size() > nodes2.size()) {
            for (int j = 0; j < nodes2.size(); j += 1) {
                if (j < nodes2.size()) {
                    if (nodes1[j] != nodes2[j]) {
                        temp = nodes2[j - 1];
                        break;
                    }
                    else {
                        temp = nodes2[j];
                    }
                }
            }
        }
        return this -> locateNode(temp);
    }

    double TreeSkeleton :: intrinsic_distance (int nodeID1, int nodeID2) {
        TreeSkeleton* node1 = this -> locateNode (nodeID1);
        TreeSkeleton* node2 = this -> locateNode (nodeID2);

        TreeSkeleton* lcanode = this -> lca(nodeID1, nodeID2);

        double dn1 = abs(lcanode -> fVal - node1 -> fVal);
        double dn2 = abs(lcanode -> fVal - node2 -> fVal);

        double intdist = dn1 + dn2;

        return intdist;
    }

    std :: vector <TreeSkeleton*> TreeSkeleton :: listLeaves () {
        std :: vector <TreeSkeleton*> T1 = std :: vector <TreeSkeleton*> ();
        if (this -> numberOfChildren() == 0) {
            T1.push_back(this);
        }
        else {
            for (TreeSkeleton* child : this -> children) {
                std :: vector <TreeSkeleton*> T2 = child -> listLeaves();
                T1.insert(T1.end(), T2.begin(), T2.end());
            }
        }
        return T1;
    }

    void TreeSkeleton :: resetPersistencePairings () {
        this -> isPaired = false;
        for (TreeSkeleton* child : this -> children) {
            child -> resetPersistencePairings();
        }
    }

    std :: vector <TreeSkeleton*> TreeSkeleton :: extremeUnpairedLeaves () {
        std :: vector <TreeSkeleton*> Leaves = this -> listLeaves();
        std :: vector <TreeSkeleton*> unpairedLeaves = std :: vector <TreeSkeleton*> ();    
        for (TreeSkeleton* Leaf : Leaves) {
            if (Leaf -> isPaired == false) {
                unpairedLeaves.push_back(Leaf);
            }
        }
        return unpairedLeaves;
    }

    void TreeSkeleton :: generatePersistencePairings () {

        /*
        for (TreeSkeleton* node : this -> children) {
            node -> generatePersistencePairings();
        }

        if (this -> numberOfChildren() != 0 and this -> numberOfChildren() == 2) {
            std :: vector <TreeSkeleton*> lup = this -> extremeUnpairedLeaves();
            TreeSkeleton* l0 = lup[0];
            TreeSkeleton* l1 = lup[1];

            if (l0 -> fVal >= l1 -> fVal) {
                this -> persPairID = l0 -> node_ID;
                l0 -> persPairID = this -> node_ID;
                this -> isPaired = true;
                l0 -> isPaired = true;
            }

            else {
                this -> persPairID = l1 -> node_ID;
                l1 -> persPairID = this -> node_ID;
                this -> isPaired = true;
                l1 -> isPaired = true;
            }
        }

        else if (this -> numberOfChildren() != 0 and this -> numberOfChildren() == 1) {
            TreeSkeleton* lup = this -> extremeUnpairedLeaves()[0];
            lup -> persPairID = this -> node_ID;
            this -> persPairID = lup -> node_ID;
            lup -> isPaired = true;
            this -> isPaired = true;
        }
        */
        if (this -> isPaired == false) {
            for (TreeSkeleton* node : this -> children) {
                node -> generatePersistencePairings();
            }

            if (this -> numberOfChildren() != 0 and this -> numberOfChildren() != 1) {
                std :: vector <TreeSkeleton*> leavesPersisting = std :: vector <TreeSkeleton*> ();

                for (TreeSkeleton* child : this -> children) {
                    std :: vector <TreeSkeleton*> childLeafPersisting = child -> extremeUnpairedLeaves();
                    leavesPersisting.insert(leavesPersisting.end(), childLeafPersisting.begin(), childLeafPersisting.end());
                }

                float max_pers = 0;
                int pos = 0;
                int pos_marked;
                for (TreeSkeleton* persFeat : leavesPersisting) {
                    if (abs(this -> fVal - persFeat -> fVal) > max_pers) {
                        max_pers = abs(this -> fVal - persFeat -> fVal);
                        pos_marked = pos;
                    }
                    pos += 1;
                }

                leavesPersisting.erase(leavesPersisting.begin() + pos_marked);

                float second_max_pers = 0;
                int second_pos = 0;
                int second_pos_marked = 0;
                for (TreeSkeleton* persFeat : leavesPersisting) {
                    if (abs(this -> fVal - persFeat -> fVal) > second_max_pers) {
                        second_max_pers = abs(this -> fVal - persFeat -> fVal);
                        second_pos_marked = second_pos;
                    }
                    second_pos += 1;
                }

                this -> persPairID = leavesPersisting[second_pos_marked] -> node_ID;
                this -> isPaired = true;

                for (TreeSkeleton* persFeat : leavesPersisting) {
                    persFeat -> persPairID = this -> node_ID;
                    persFeat -> isPaired = true;
                }
            }

            else if (this -> numberOfChildren() != 0 and this -> numberOfChildren() == 1) {
                TreeSkeleton* lup = this -> extremeUnpairedLeaves()[0];
                lup -> persPairID = this -> node_ID;
                this -> persPairID = lup -> node_ID;
                lup -> isPaired = true;
                this -> isPaired = true;
            }
        }
    }

    std :: vector <std :: pair <double, double>> TreeSkeleton :: persistenceDiagram () {
        this -> generatePersistencePairings();
        std :: vector <std :: pair <double, double>> bdgm = std :: vector <std :: pair <double, double>> ();
        for (TreeSkeleton* Leaf : this -> listLeaves()) {
            std :: pair <double, double> TopPair;
            TopPair.first = Leaf -> fVal;
            if (this -> locateNode (Leaf -> persPairID) != NULL) {
                TopPair.second = this -> locateNode (Leaf -> persPairID) -> fVal;
            }
            bdgm.push_back(TopPair);
        }
        return bdgm;
    }

    double TreeSkeleton :: TotalPers (double p = -1) {
        std :: vector <std :: pair <double, double>> dgm = this -> persistenceDiagram();
        std :: vector <double> persistences = std :: vector <double> ();
        for (std :: pair <double, double> x : dgm) {
            persistences.push_back(abs(x.first - x.second));
        }
        if (p >= 1) {
            double sum = 0;
            for (double pers : persistences) {
                sum = sum + pow(pers, p);
            }
            sum = pow(sum, 1/p);
            return sum;
        }
        else {
            return *max_element(persistences.begin(), persistences.end());
        }
    }

    double TreeSkeleton :: normalised_variance () {
        std :: vector <std :: pair <double, double>> dgm = this -> persistenceDiagram();
        std :: vector <double> persistences = std :: vector <double> ();
        for (std :: pair <double, double> x : dgm) {
            persistences.push_back(abs(x.first - x.second));
        }

        double span = *max_element(persistences.begin(), persistences.end());
        int iterator = 0;
        for (double x : persistences) {
            persistences[iterator] = persistences[iterator] / span;
            iterator = iterator + 1;
        }

        double nmp = this -> normalised_mean_persistence();
        double variance_numer = 0;
        for (int i = 0; i < iterator; i += 1) {
            variance_numer = variance_numer + (persistences[i] - nmp) * (persistences[i] - nmp);
        }

        return variance_numer / persistences.size();
    }

    double TreeSkeleton :: normalised_mean_persistence () {
        std :: vector <std :: pair <double, double>> dgm = this -> persistenceDiagram();
        std :: vector <double> persistences = std :: vector <double> ();
        for (std :: pair <double, double> x : dgm) {
            persistences.push_back(abs(x.first - x.second));
        }

        double span = *max_element(persistences.begin(), persistences.end());
        int iterator = 0;
        int sum = 0;
        for (double x : persistences) {
            persistences[iterator] = persistences[iterator] / span;
            iterator += 1;
        }
        
        for (int i = 0; i < iterator; i += 1) {
            sum = sum + persistences[i];
        }
        return sum / persistences.size();
    }

    TreeSkeleton* TreeSkeleton :: transform (double scalingFactor, double translate) {
        std :: vector <TreeSkeleton*> childrenTransformed = std :: vector <TreeSkeleton*> ();
        for (TreeSkeleton* child : this -> children) {
            TreeSkeleton* transformed = child -> transform (scalingFactor, translate);
            childrenTransformed.push_back(transformed);
        }
        double transformedVal = (this -> fVal) * scalingFactor + translate; 
        TreeSkeleton* newRoot = new TreeSkeleton (this -> node_ID, this -> parent_ID, childrenTransformed, NULL, transformedVal);
        for (TreeSkeleton* child : newRoot -> children) {
            child -> parent = newRoot;
        }
        return newRoot;
    }

    TreeSkeleton* TreeSkeleton :: normalise () {
        double translate = this -> fVal;
        double scaling = abs(this -> fVal - ((this -> locateNode(this -> persPairID)) -> fVal));
        return this -> transform (1/scaling, -translate/scaling);
    }

    /*
    TreeSkeleton* TreeSkeleton :: subTree (int nodeID) {
        TreeSkeleton* nodeRequired = this -> locateNode (nodeID);
        TreeSkeleton* node = nodeRequired -> transform (1, 0);
        node -> parent_ID = -1;
        node -> parent = NULL;
        return node;
    }

    std :: vector <TreeSkeleton*> TreeSkeleton :: Forest (int nodeID) {
        TreeSkeleton* nodeRequired = this -> locateNode (nodeID);
        std :: vector <TreeSkeleton*> Forest = std :: vector <TreeSkeleton*> ();
        for (TreeSkeleton* child : nodeRequired -> children) {
            TreeSkeleton* rSubTree = this -> subTree (child -> node_ID);
            Forest.push_back(rSubTree);
        }
        return Forest;
    }

    TreeSkeleton* TreeSkeleton :: insertNode (int parentID, std :: vector <int> childrenIDs, barebonesNode NewNode) {
        std :: vector <TreeSkeleton*> childrenTBM = std :: vector <TreeSkeleton*> ();
        for (int ID : childrenIDs) {
            TreeSkeleton* child = this -> locateNode(ID);
            childrenTBM.push_back(child);
        }
        TreeSkeleton* NewParent = this -> locateNode (parentID);
        TreeSkeleton* nodeTBI = new TreeSkeleton(NewNode.node_ID, parentID, childrenTBM, NewParent, NewNode.fVal); 
        NewParent -> children.push_back(nodeTBI);

        for (TreeSkeleton* child : nodeTBI -> children) {
            child -> parent = nodeTBI;
            child -> parent_ID = nodeTBI -> node_ID;
        }
        
        std :: vector <TreeSkeleton*> commonChildren = std :: vector <TreeSkeleton*> ();

        for (TreeSkeleton* childNewNode : nodeTBI -> children) {
            int iter = 0;
            for (TreeSkeleton* childParent : NewParent -> children) {
                if (childNewNode == childParent) {
                    NewParent -> children.erase(NewParent -> children.begin() + iter);
                }
                iter ++;
            }
        }
        return NewParent -> locateRoot();
    }

    TreeSkeleton* TreeSkeleton :: deleteNode (int nodeID) {
        TreeSkeleton* nodeTBD = this -> locateNode(nodeID);
        TreeSkeleton* NewParent = nodeTBD -> parent;
        for (TreeSkeleton* child : nodeTBD -> children) {
            child -> parent = nodeTBD -> parent;
            child -> parent_ID = nodeTBD -> parent_ID;
            NewParent -> children.push_back(child);
        }

        int iter = 0;
        int pos_marked = 0;
        for (TreeSkeleton* child : nodeTBD -> parent -> children) {
            if (child == nodeTBD) {
                pos_marked = iter;
            }
            iter ++;
        }
        NewParent -> children.erase(NewParent -> children.begin() + pos_marked);
        return NewParent;
    }

    /*TreeSkeleton* TreeSkeleton :: purgeRegular () {
        for (TreeSkeleton* child : this -> children) {
            child = child -> purgeRegular();
        }
        if (this -> numberOfChildren() == 1 && this -> parent != NULL) {
            return deleteNode(this -> node_ID);
        }
    }
    */

    std :: vector <CEDTNodeData> TreeSkeleton :: genCEDTInput () {

        std :: vector <CEDTNodeData> Olist = std :: vector <CEDTNodeData> ();
        CEDTNodeData O1;
        O1.id = this -> node_ID;
        O1.parent_id = this -> parent_ID;
        O1.persistence_pair_id = this -> persPairID;
        O1.function_value = this -> fVal;
        O1.number_of_children = this -> children.size();
        std :: vector <int> ListChildren = std :: vector <int> ();
        for (TreeSkeleton* child : this -> children) {
            ListChildren.push_back(child -> node_ID);
        }
        O1.children_ids = ListChildren;
        Olist.push_back(O1);
        
        for (TreeSkeleton* child : this -> children) {
            std :: vector <CEDTNodeData> Ochild = child -> genCEDTInput();
            Olist.insert(Olist.end(), Ochild.begin(), Ochild.end());
        }
        
        return Olist;
    }

    void TreeSkeleton :: WriteToCEDTFile (std :: vector <CEDTNodeData> Data, std :: string FileName) {

        int n1 = Data.size();
        int MAX_ID = 63000;
        std :: ofstream CEDTOutputFile;
        CEDTOutputFile.open(FileName);
        CEDTOutputFile << n1 << std :: endl;
        for (int i = 0; i < n1; i += 1) {
            CEDTNodeData D = Data[i];
            CEDTOutputFile << D.id << " " << D.parent_id << " " << D.persistence_pair_id << " " << MAX_ID << " " << D.function_value << " " << D.number_of_children;
            for (int j = 0; j < D.number_of_children; j += 1) {
                CEDTOutputFile << " " << D.children_ids[j];
            }
            CEDTOutputFile << std :: endl;
        }
        CEDTOutputFile.close();
    }

    void TreeSkeleton :: genTreeFile (std :: vector <CEDTNodeData> Data, std :: string Filename) {
        std :: ofstream TreeFile;
        int n1 = Data.size();
        TreeFile.open(Filename);
        TreeFile << n1 << std :: endl;
        for (int i = 0; i < n1; i += 1) {
            CEDTNodeData D = Data[i];
            TreeFile << D.id << " " << D.parent_id << " " << D.number_of_children;
            for (int j = 0; j < D.number_of_children; j += 1) {
                TreeFile << " " << D.children_ids[j];
            }
            TreeFile << " " << D.function_value;
            TreeFile << std :: endl;
        }
        TreeFile.close();
    }

    int root_ID (std :: vector <nodeData> nodesFromFile) {
        int root;
        for (nodeData x : nodesFromFile) {
            if (x.parent_ID == -1) {
                root = x.node_ID;
            }
        }
        return root;
    }

    TreeSkeleton* generateTree (std :: string filename) {
        std :: fstream TreeFile(filename);
        int n_nodes;
        TreeFile >> n_nodes;
        std :: vector <nodeData> nodesFromFile = std :: vector <nodeData> ();
        for (int i = 0; i < n_nodes; i += 1) {
            int node_ID;
            int parent_ID;
            int n_children;
            double fVal = 0;
            std :: vector <int> childrenIDs = std :: vector <int> ();
            TreeFile >> node_ID >> parent_ID >> n_children;
            
            int tempy;
            for (int j = 0; j < n_children; j += 1) {
                TreeFile >> tempy;
                childrenIDs.push_back(tempy);
            }
            TreeFile >> fVal;
            nodeData tempx;
            tempx.node_ID = node_ID;
            tempx.parent_ID = parent_ID;
            tempx.n_children = n_children;
            tempx.fVal = fVal;
            tempx.childrenIDs = childrenIDs;
            nodesFromFile.push_back(tempx);
        }
        
        TreeSkeleton* T1 = new TreeSkeleton (nodesFromFile, mergeTrees :: root_ID(nodesFromFile), NULL, filename);
        TreeFile.close();
        return T1;
    }
}
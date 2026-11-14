#ifndef TREE_SKELETON_H
#define TREE_SKELETON_H

#include <string>
#include <vector>
#include "nodeData.h"

namespace mergeTrees {

    class TreeSkeleton {

        public:

            int node_ID;
            std :: vector <int> childrenIDs = std :: vector <int> ();
            std :: vector <TreeSkeleton*> children = std :: vector <TreeSkeleton*> ();
            TreeSkeleton* parent = NULL;
            int parent_ID;
            double fVal;
            bool isPaired = false;
            int persPairID;
            std :: string Name;

            TreeSkeleton (std :: vector <nodeData>, int, TreeSkeleton*, std :: string);
            TreeSkeleton (int, int, std :: vector <TreeSkeleton*>, TreeSkeleton*, double);
            TreeSkeleton* locateRoot ();

            void recursive_traversal();
            int number_of_nodes();
            int depth();
            int countFeatures();
            int countSaddles();
            bool isJoinTree();
            bool isSplitTree();
            std :: string classify();
            TreeSkeleton* locateNode (int);
            std :: vector <TreeSkeleton*> path_to_root (int);
            TreeSkeleton* lca (int, int);
            int numberOfChildren ();
            double intrinsic_distance (int, int);
            std :: vector <TreeSkeleton*> listLeaves ();
            void generatePersistencePairings ();
            std :: vector <TreeSkeleton*> extremeUnpairedLeaves ();
            std :: vector <std :: pair <double, double>> persistenceDiagram ();
            void resetPersistencePairings ();
            double TotalPers (double);
            double normalised_mean_persistence ();
            double normalised_variance ();

            TreeSkeleton* transform (double, double);
            TreeSkeleton* normalise ();

            /*
            TreeSkeleton* subTree (int);
            std :: vector <TreeSkeleton*> Forest (int);
            TreeSkeleton* insertNode (int, std :: vector<int>, barebonesNode);
            TreeSkeleton* deleteNode (int);
            TreeSkeleton* purgeRegular ();
            */

            std :: vector <CEDTNodeData> genCEDTInput();
            void WriteToCEDTFile(std :: vector <CEDTNodeData>, std :: string);
            void genTreeFile (std :: vector <CEDTNodeData>, std :: string);
    };

    TreeSkeleton* generateTree (std :: string filename);
    int root_ID (std :: vector <nodeData> nodesFromFile);
}
#endif
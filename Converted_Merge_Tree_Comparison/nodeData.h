#ifndef NODE_DATA_H
#define NODE_DATA_H

#include <vector>

namespace mergeTrees {
    struct nodeData {
        int node_ID;
        int parent_ID;
        int n_children;
        double fVal;
        std :: vector <int> childrenIDs = std :: vector <int> ();
    };

    struct barebonesNode {
        int node_ID;
        double fVal;
    };

    struct CEDTNodeData {
        int id;
        int parent_id;
        int persistence_pair_id;
        double function_value;
        int number_of_children;
        std :: vector <int> children_ids;
    };
}
#endif
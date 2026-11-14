#ifndef TREE_PAIR_H
#define TREE_PAIR_H

#include "TreeSkeleton.h"

namespace mergeTrees{

        //double intrinsic_interleaving_distance (int);
        double Bottleneck (std :: string , std :: string);
        double Wasserstein (std :: string , std :: string);
        double CWMTED (std :: string , std :: string, std :: string);
        double COMTED (std :: string , std :: string, std :: string);
        double TotalPers (std :: string, double);
        int count_features (std :: string);
        std :: vector <std :: pair <double, double>> persistence_pairs (std :: string);
        std :: vector <double> persistences (std :: string);

        double mean_pers (std :: string);
        double pers_vars (std :: string);
        double normalised_mean_pers (std :: string);
        double normalised_vars_pers (std :: string);

        int depth (std :: string);

        class TreePair {

                public:
                        TreeSkeleton* T1;
                        TreeSkeleton* T2;

                        TreePair (std :: string, std :: string);
                        double bottleneck ();
                        double Wasserstein ();
                        double CWMTED (std :: string);
                        double COMTED (std :: string);
        };
}
#endif

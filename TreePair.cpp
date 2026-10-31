#include "TreePair.h"

#include <map>
#include <iostream>
#include <algorithm>
#include <math.h>
#include <numeric>
#include <cstdio>
#include <memory>
#include <string>

// CW-MTED.jar, CO-MTED.jar, CustomisedBottleneck.jar and CustomisedWasserstein.jar
// are replaced by the same-named .java files in the home directory, run directly
// (Java source-file mode, JDK 11+). jgrapht/jheaps come from the extracted classes
// in CustomisedWasserstein/org.
#define MTED_JAVA(name) "java -cp CustomisedWasserstein " name ".java "

namespace mergeTrees {

    // Only for a fixed cyclical labelling right now.
    /*
    double TreePair :: intrinsic_interleaving_distance (int n) {

        std :: vector <TreeSkeleton*> Leaves1 = T1 -> listLeaves();
        std :: vector <TreeSkeleton*> Leaves2 = T2 -> listLeaves();

        int n1 = Leaves1.size();
        int n2 = Leaves2.size();

        if (n >= n1 and n >= n2) {
            std :: map <int, TreeSkeleton*> Labels1;
            std :: map <int, TreeSkeleton*> Labels2;

            for (int i = 0; i < n; i += 1) {
                Labels1[i] = Leaves1[i % n1];
                Labels2[i] = Leaves2[i % n2];
            }

            std :: vector <std :: vector <double>> DiffMatrix;

            for (int i = 0; i < n; i += 1) {
                std :: vector <double> rowdiff = std :: vector <double> ();
                for (int j = 0; j < n; j += 1) {
                    TreeSkeleton* TreeNode11 = Labels1[i];
                    TreeSkeleton* TreeNode12 = Labels1[j];
                    TreeSkeleton* TreeNodeLCA1 = T1 -> lca(TreeNode11 -> node_ID, TreeNode12 -> node_ID);

                    TreeSkeleton* TreeNode21 = Labels2[i];
                    TreeSkeleton* TreeNode22 = Labels2[j];
                    TreeSkeleton* TreeNodeLCA2 = T2 -> lca(TreeNode21 -> node_ID, TreeNode22 -> node_ID);

                    rowdiff.push_back(abs(TreeNodeLCA1 -> fVal - TreeNodeLCA2 -> fVal));
                }
                DiffMatrix.push_back(rowdiff);
            }

            std :: vector <double> row_sums = std :: vector <double> ();
            for (std :: vector <double> row : DiffMatrix) {
                double res1 = *max_element(row.begin(), row.end());
                row_sums.push_back(res1);
            }

            double max = *max_element (row_sums.begin(), row_sums.end());
            return max;
        }
        else {
            return -1;
        }
    }
    */

    std :: string pipeline (char* cmd) {

        std :: array<char, 128> buffer;
        std :: string result;

        std :: unique_ptr <FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);

        while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
            result += buffer.data();
        }

        return result;
    }

    double Bottleneck (std :: string TreeFile1, std :: string TreeFile2) {

        std :: string TreeName1 = TreeFile1;
        std :: string TreeName2 = TreeFile2;

        TreeSkeleton* T1 = generateTree(TreeFile1);
        TreeSkeleton* T2 = generateTree(TreeFile2);

        T1 -> generatePersistencePairings();
        T2 -> generatePersistencePairings();

        T1 -> WriteToCEDTFile(T1 -> genCEDTInput(), TreeFile1 + ".jt");
        T2 -> WriteToCEDTFile(T2 -> genCEDTInput(), TreeFile2 + ".jt");

        std :: string command = MTED_JAVA("CustomisedBottleneck") + TreeName1 + ".jt " + TreeName2 + ".jt ./";
        //std :: string Bottleneck = pipeline ("java -jar CustomisedBottleneck.jar Tree1.jt Tree3.jt ./");
        char *cmnd = &command[0];
        //std :: cout << cmnd;
        std :: string Bottleneck = pipeline(cmnd);
        return stod(Bottleneck);

    }

    double Wasserstein (std :: string TreeFile1, std :: string TreeFile2) {

        std :: string TreeName1 = TreeFile1;
        std :: string TreeName2 = TreeFile2;

        TreeSkeleton* T1 = generateTree(TreeFile1);
        TreeSkeleton* T2 = generateTree(TreeFile2);

        T1 -> generatePersistencePairings();
        T2 -> generatePersistencePairings();

        T1 -> WriteToCEDTFile(T1 -> genCEDTInput(), TreeFile1 + ".jt");
        T2 -> WriteToCEDTFile(T2 -> genCEDTInput(), TreeFile2 + ".jt");

        std :: string command = MTED_JAVA("CustomisedWasserstein") + TreeName1 + ".jt " + TreeName2 + ".jt ./";
        //std :: cout << command;
        char *cmnd = &command[0];
        std :: string Wasserstein = pipeline(cmnd);
        //std::cout << Wasserstein;
        return stod(Wasserstein);

    }

    double CWMTED (std :: string TreeFile1, std :: string TreeFile2, std :: string OutputLocation) {

        std :: string TreeName1 = TreeFile1;
        std :: string TreeName2 = TreeFile2;

        TreeSkeleton* T1 = generateTree(TreeFile1);
        TreeSkeleton* T2 = generateTree(TreeFile2);

        T1 -> generatePersistencePairings();
        T2 -> generatePersistencePairings();

        T1 -> WriteToCEDTFile(T1 -> genCEDTInput(), TreeFile1 + ".jt");
        T2 -> WriteToCEDTFile(T2 -> genCEDTInput(), TreeFile2 + ".jt");

        std :: string command = MTED_JAVA("CW-MTED") + TreeName1 + ".jt " + TreeName2 + ".jt " + OutputLocation;
        char *cmnd = &command[0];
        std :: string CWMEDT = pipeline(cmnd);
        return stod(CWMEDT);
    }

    double COMTED (std :: string TreeFile1, std :: string TreeFile2, std :: string OutputLocation) {

        std :: string TreeName1 = TreeFile1;
        std :: string TreeName2 = TreeFile2;

        TreeSkeleton* T1 = generateTree(TreeFile1);
        TreeSkeleton* T2 = generateTree(TreeFile2);

        T1 -> generatePersistencePairings();
        T2 -> generatePersistencePairings();

        T1 -> WriteToCEDTFile(T1 -> genCEDTInput(), TreeFile1 + ".jt");
        T2 -> WriteToCEDTFile(T2 -> genCEDTInput(), TreeFile2 + ".jt");

        std :: string command = MTED_JAVA("CO-MTED") + TreeName1 + ".jt " + TreeName2 + ".jt " + OutputLocation;
        char *cmnd = &command[0];
        std :: string COMTED = pipeline(cmnd);
        return stod(COMTED);
    }

    double TotalPers (std :: string TreeFile, double p) {
        TreeSkeleton* T = generateTree(TreeFile);
        return T -> TotalPers(p);
    }

    int count_features (std :: string TreeFile) {
        TreeSkeleton* T = generateTree(TreeFile);
        return T -> countFeatures();
    }

    std :: vector <std :: pair <double, double>> persistence_pairs (std :: string TreeFile) {
        TreeSkeleton* T = generateTree(TreeFile);
        T -> generatePersistencePairings();
        return T -> persistenceDiagram();
    }

    std :: vector <double> persistences (std :: string TreeFile) {
        TreeSkeleton* T = generateTree(TreeFile);
        std :: vector <std :: pair <double, double>> dgm = T -> persistenceDiagram();
        std :: vector <double> persistences = std :: vector <double> ();
        for (std :: pair <double, double> x : dgm) {
            persistences.push_back(abs(x.first - x.second));
        }
        sort (persistences.begin(), persistences.end());
        return persistences;
    }

    int depth (std :: string TreeFile) {
        TreeSkeleton* T = generateTree(TreeFile);
        int depthReturn = T -> depth();
        return depthReturn;
    }

    TreePair :: TreePair (std :: string FileName1, std :: string FileName2) {
        TreeSkeleton* T1 = generateTree(FileName1);
        TreeSkeleton* T2 = generateTree(FileName2);

        T1 -> generatePersistencePairings();
        T2 -> generatePersistencePairings();

        T1 -> WriteToCEDTFile(T1 -> genCEDTInput(), T1 -> Name + ".jt");
        T2 -> WriteToCEDTFile(T2 -> genCEDTInput(), T2 -> Name + ".jt");
    }

    double TreePair :: bottleneck () {

        std :: string TreeFile1 = T1 -> Name;
        std :: string TreeFile2 = T2 -> Name;

        std :: string command = MTED_JAVA("CustomisedBottleneck") + TreeFile1 + ".jt " + TreeFile2 + ".jt ./";
        char *cmnd = &command[0];
        std :: string Bottleneck = pipeline(cmnd);
        return stod(Bottleneck);
    }

    double TreePair :: Wasserstein () {

      std :: string TreeFile1 = T1 -> Name;
      std :: string TreeFile2 = T2 -> Name;

      std :: string command = MTED_JAVA("CustomisedWasserstein") + TreeFile1 + ".jt " + TreeFile2 + ".jt ./";
      char *cmnd = &command[0];
      std :: string Wasserstein = pipeline(cmnd);
      return stod(Wasserstein);

    }

    double TreePair :: CWMTED (std :: string OutputLocation) {
        std :: string TreeFile1 = T1 -> Name;
        std :: string TreeFile2 = T2 -> Name;

        std :: string command = MTED_JAVA("CW-MTED") + TreeFile1 + ".jt " + TreeFile2 + ".jt " + OutputLocation;
        char *cmnd = &command[0];
        std :: string CWEDT = pipeline(cmnd);
        return stod(CWEDT);
    }

    double TreePair :: COMTED (std :: string OutputLocation) {
        std :: string TreeFile1 = T1 -> Name;
        std :: string TreeFile2 = T2 -> Name;

        std :: string command = MTED_JAVA("CO-MTED") + TreeFile1 + ".jt " + TreeFile2 + ".jt " + OutputLocation;
        char *cmnd = &command[0];
        std :: string COEDT = pipeline(cmnd);
        return stod(COEDT);
    }
}

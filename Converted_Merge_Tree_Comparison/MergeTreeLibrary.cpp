#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "TreeSkeleton.h"
#include "TreePair.h"

namespace py = pybind11;

PYBIND11_MODULE(MergeTreeLibrary, m) {

    m.def("bottleneck", & mergeTrees :: Bottleneck);
    m.def("CWMTED", & mergeTrees :: CWMTED);
    m.def("COMTED", & mergeTrees :: COMTED);
    m.def("Wasserstein", & mergeTrees :: Wasserstein);
    m.def("TotalPers", & mergeTrees :: TotalPers);
    m.def("pers", & mergeTrees :: persistences);
    m.def("features", & mergeTrees :: count_features);


    py :: class_<mergeTrees :: TreeSkeleton> (m, "TreeSkeleton")
        .def("recursive_traversal", & mergeTrees :: TreeSkeleton :: recursive_traversal)
        .def("number_of_nodes", & mergeTrees :: TreeSkeleton :: number_of_nodes)
        .def("depth", & mergeTrees :: TreeSkeleton :: depth)
        .def("countFeatures", & mergeTrees :: TreeSkeleton :: countFeatures)
        .def("countSaddles", & mergeTrees :: TreeSkeleton :: countSaddles)
        .def("isJoinTree", & mergeTrees :: TreeSkeleton :: isJoinTree)
        .def("isSplitTree", & mergeTrees :: TreeSkeleton :: isSplitTree)
        .def("classify", & mergeTrees :: TreeSkeleton :: classify)
        .def("locateNode", & mergeTrees :: TreeSkeleton :: locateNode)
        .def("path_to_root", & mergeTrees :: TreeSkeleton :: path_to_root)
        .def("lca", & mergeTrees :: TreeSkeleton :: lca)
        .def("numberOfChildren", & mergeTrees :: TreeSkeleton :: numberOfChildren)
        .def("intrinsic_distance", & mergeTrees :: TreeSkeleton :: intrinsic_distance)
        .def("listLeaves", & mergeTrees :: TreeSkeleton :: listLeaves)
        .def("generatePersistencePairings", & mergeTrees :: TreeSkeleton :: generatePersistencePairings)
        .def("extremeUnpairedLeaves", & mergeTrees :: TreeSkeleton :: extremeUnpairedLeaves)
        .def("persistenceDiagram", & mergeTrees :: TreeSkeleton :: persistenceDiagram)
        .def("resetPersistencePairings", & mergeTrees :: TreeSkeleton :: resetPersistencePairings)
        .def("TotalPers", & mergeTrees :: TreeSkeleton :: TotalPers)
        .def("normalised_mean_persistence", & mergeTrees :: TreeSkeleton :: normalised_mean_persistence)
        .def("normalised_variance", & mergeTrees :: TreeSkeleton :: normalised_variance)

        .def("transform", & mergeTrees :: TreeSkeleton :: transform)
        .def("normalise", & mergeTrees :: TreeSkeleton :: normalise)
        //subtree
        //forest
        //insertNode
        //deleteNode
        //purgeregular
        .def("genCEDTInput", & mergeTrees :: TreeSkeleton :: genCEDTInput)
        .def("WriteToCEDTFile", & mergeTrees :: TreeSkeleton :: WriteToCEDTFile)
        .def("genTreeFile", & mergeTrees :: TreeSkeleton :: genTreeFile);

    m.def("generateTree", & mergeTrees :: generateTree);

    py :: class_<mergeTrees :: CEDTNodeData> (m, "CEDTNodeData");

    py :: class_<mergeTrees :: TreePair> (m, "TreePair")
        .def(py :: init <std :: string, std :: string>())
        .def("bottleneck", & mergeTrees :: TreePair :: bottleneck)
        .def("Wasserstein", & mergeTrees :: TreePair :: Wasserstein)
        .def("CW-MTED", & mergeTrees :: TreePair :: CWMTED)
        .def("CO-MTED", & mergeTrees :: TreePair :: COMTED);
}

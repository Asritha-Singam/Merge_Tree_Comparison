#!/bin/sh
# Builds everything in this folder (run it from Git Bash, inside this folder):
#   1. the five C++ programs (.exe)
#   2. MergeTreeLibrary.pyd, the C++ module that Driver.py imports
# Needs: g++ (MSYS2 UCRT64), Python with pybind11 installed (pip install pybind11).
set -e
cd "$(dirname "$0")"

echo "Building CustomisedBottleneck.exe, CustomisedWasserstein.exe, SeeMatching.exe, CO-MTED.exe, CW-MTED.exe ..."
g++ -std=c++17 -O2 -static -o CustomisedBottleneck.exe CustomisedBottleneck.cpp
g++ -std=c++17 -O2 -static -o CustomisedWasserstein.exe CustomisedWasserstein.cpp
g++ -std=c++17 -O2 -static -o SeeMatching.exe SeeMatching.cpp
g++ -std=c++17 -O2 -static -o CO-MTED.exe CO-MTED.cpp
g++ -std=c++17 -O2 -static -o CW-MTED.exe CW-MTED.cpp

echo "Building MergeTreeLibrary.pyd ..."
PY_INC=$(python -c "import sysconfig; print(sysconfig.get_paths()['include'])")
PY_LIBS=$(python -c "import sys, os; print(os.path.join(sys.base_prefix, 'libs'))")
PY_LIB=$(python -c "import sys; print('python%d%d' % sys.version_info[:2])")
PB_INC=$(python -c "import pybind11; print(pybind11.get_include())")

g++ -std=c++17 -O2 -shared -static \
    -I "$PY_INC" -I "$PB_INC" \
    MergeTreeLibrary.cpp TreePair.cpp TreeSkeleton.cpp \
    -L "$PY_LIBS" -l"$PY_LIB" \
    -o MergeTreeLibrary.pyd

echo "Done."

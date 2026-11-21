"""Builds everything in this folder. Works from PowerShell, cmd or Git Bash:

    python build.py

Makes:
  1. the C++ distance programs: CustomisedBottleneck.exe, CustomisedWasserstein.exe,
     SeeMatching.exe, CO-MTED.exe, CW-MTED.exe
  2. MergeTreeLibrary.pyd, the C++ module that Driver.py imports

Needs: g++ (MSYS2) on PATH, and pybind11 (pip install pybind11) for the same
Python that runs this script (and later Driver.py).
"""
import os
import shutil
import subprocess
import sys
import sysconfig

HERE = os.path.dirname(os.path.abspath(__file__))

PROGRAMS = ["CustomisedBottleneck", "CustomisedWasserstein", "SeeMatching", "CO-MTED", "CW-MTED"]


def run(cmd):
    print(">", " ".join(cmd))
    result = subprocess.run(cmd, cwd=HERE)
    if result.returncode != 0:
        sys.exit("Build failed (see the error above).")


def main():
    if shutil.which("g++") is None:
        sys.exit("g++ was not found. Install MSYS2 (UCRT64) and add C:\\msys64\\ucrt64\\bin to PATH.")
    try:
        import pybind11
    except ImportError:
        sys.exit("pybind11 is missing. Run:  %s -m pip install pybind11" % sys.executable)

    print("Building the distance programs ...")
    for name in PROGRAMS:
        run(["g++", "-std=c++17", "-O2", "-static", "-o", name + ".exe", name + ".cpp"])

    print("Building MergeTreeLibrary.pyd ...")
    pyd = os.path.join(HERE, "MergeTreeLibrary.pyd")
    if os.path.exists(pyd):
        try:
            open(pyd, "r+b").close()
        except PermissionError:
            sys.exit("MergeTreeLibrary.pyd is in use, so it cannot be replaced.\n"
                     "Close every Python program that uses it (a running Driver.py,\n"
                     "make_matching_pictures.py, a Python shell or notebook), then run this again.")
    py_include = sysconfig.get_paths()["include"]
    py_libs = os.path.join(sys.base_prefix, "libs")
    py_lib = "python%d%d" % sys.version_info[:2]
    run(["g++", "-std=c++17", "-O2", "-shared", "-static",
         "-I", py_include, "-I", pybind11.get_include(),
         "MergeTreeLibrary.cpp", "TreePair.cpp", "TreeSkeleton.cpp",
         "-L", py_libs, "-l" + py_lib,
         "-o", "MergeTreeLibrary.pyd"])

    print("Done.")


if __name__ == "__main__":
    main()

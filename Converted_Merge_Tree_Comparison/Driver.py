import MergeTreeLibrary
import sys
import os
import csv
import matplotlib.pyplot as plt
import math
import Utilities

ExperimentName = sys.argv[1]

with open(ExperimentName) as MetaInputFile:

    RunName = MetaInputFile.readline().strip()
    mode = MetaInputFile.readline().strip()

    if mode == "Pairwise":

        InputDir = MetaInputFile.readline().strip()

        number_of_trees = int(MetaInputFile.readline().strip()[0])

        List_Trees = list()
        for i in range(number_of_trees):
            TreeName = MetaInputFile.readline().strip()
            if ".jt" in TreeName:
                Utilities.GenTreeFromJt(InputDir + "/" + TreeName)
                TreeName = TreeName.replace(".jt", "")
            List_Trees.append(TreeName)

        OutputDir = MetaInputFile.readline().strip()
        if not os.path.exists(OutputDir):
            os.mkdir(OutputDir)

        paircomps = int(MetaInputFile.readline().strip())
        OutputList = list()
        MeasuresTBC = list()
        for i in range(paircomps):
            OutputLine = MetaInputFile.readline()
            MeasureInput = OutputLine.strip().split(";")
            MeasuresTBC.append(MeasureInput)

        for T1Name in List_Trees:
            for T2Name in List_Trees:
                OutputDict = Utilities.pairwiseCompare (InputDir, OutputDir, T1Name, T2Name, MeasuresTBC)
                OutputList.append(OutputDict)

        with open(OutputDir + "/PairsOutput", "w") as OutputFile:
            writer = csv.DictWriter(OutputFile, OutputList[0].keys())
            writer.writeheader()
            for row in OutputList:
                writer.writerow(row)

        SingleMeasuresTBC = list()
        singlecomps = int(MetaInputFile.readline().strip())
        for i in range(singlecomps):
            OutputLine = MetaInputFile.readline()
            MeasureInput = OutputLine.strip().split(";")
            SingleMeasuresTBC.append(MeasureInput)

        SingleOutputList = list()
        for T1Name in List_Trees:
            OutputDict = Utilities.soloCompare(InputDir, OutputDir, T1Name, SingleMeasuresTBC)
            SingleOutputList.append(OutputDict)

        with open(OutputDir + "/SingleOutput", "w") as OutputFile:
            writer = csv.DictWriter(OutputFile, SingleOutputList[0].keys())
            writer.writeheader()
            for row in SingleOutputList:
                writer.writerow(row)

    elif mode == "OneVRest":
        InputDir = MetaInputFile.readline().strip()
        T1Name = MetaInputFile.readline().strip()
        number_of_trees = int(MetaInputFile.readline().strip()[0])

        List_Trees = list()
        for i in range(number_of_trees):
            List_Trees.append(MetaInputFile.readline().strip())

        OutputDir = MetaInputFile.readline().strip()
        if not os.path.exists(OutputDir):
            os.mkdir(OutputDir)

        paircomps = int(MetaInputFile.readline().strip())
        OutputList = list()
        MeasuresTBC = list()
        for i in range(paircomps):
            OutputLine = MetaInputFile.readline()
            MeasureInput = OutputLine.strip().split(";")
            MeasuresTBC.append(MeasureInput)

        for T2Name in List_Trees:
                OutputDict = Utilities.pairwiseCompare(InputDir, OutputDir, T1Name, T2Name, MeasuresTBC)
                OutputList.append(OutputDict)

        with open(OutputDir + "/PairsOutput", "w") as OutputFile:
            writer = csv.DictWriter(OutputFile, OutputList[0].keys())
            writer.writeheader()
            for row in OutputList:
                writer.writerow(row)

        SingleMeasuresTBC = list()
        singlecomps = int(MetaInputFile.readline().strip())
        for i in range(singlecomps):
            OutputLine = MetaInputFile.readline()
            MeasureInput = OutputLine.strip().split(";")
            SingleMeasuresTBC.append(MeasureInput)

        SingleOutputList = list()
        for TName in List_Trees + list([T1Name]):

            OutputDict = Utilities.soloCompare(InputDir, OutputDir, TName, SingleMeasuresTBC)
            SingleOutputList.append(OutputDict)

        with open(OutputDir + "/SingleOutput", "w") as OutputFile:
            writer = csv.DictWriter(OutputFile, SingleOutputList[0].keys())
            writer.writeheader()
            for row in SingleOutputList:
                writer.writerow(row)

    elif mode == "Sequential":

        InputDir = MetaInputFile.readline().strip()
        number_of_trees = int(MetaInputFile.readline().strip()[0])

        List_Trees = list()
        for i in range(number_of_trees):
            List_Trees.append(MetaInputFile.readline().strip())

        OutputDir = MetaInputFile.readline().strip()
        if not os.path.exists(OutputDir):
            os.mkdir(OutputDir)

        paircomps = int(MetaInputFile.readline().strip())
        OutputList = list()
        MeasuresTBC = list()

        for i in range(paircomps):
            OutputLine = MetaInputFile.readline()
            MeasureInput = OutputLine.strip().split(";")
            MeasuresTBC.append(MeasureInput)

        for i in range(len(List_Trees) - 1):
            T1Name = List_Trees[i]
            T2Name = List_Trees[i+1]
            OutputDict = Utilities.pairwiseCompare(InputDir, OutputDir, T1Name, T2Name, MeasuresTBC)
            OutputList.append(OutputDict)

        with open(OutputDir + "/PairsOutput", "w") as OutputFile:
            writer = csv.DictWriter(OutputFile, OutputList[0].keys())
            writer.writeheader()
            for row in OutputList:
                writer.writerow(row)

        SingleMeasuresTBC = list()
        singlecomps = int(MetaInputFile.readline().strip())
        for i in range(singlecomps):
            OutputLine = MetaInputFile.readline()
            MeasureInput = OutputLine.strip().split(";")
            SingleMeasuresTBC.append(MeasureInput)

        SingleOutputList = list()
        for T1Name in List_Trees:
            OutputDict = Utilities.soloCompare(InputDir, OutputDir, T1Name, SingleMeasuresTBC)
            SingleOutputList.append(OutputDict)

        with open(OutputDir + "/SingleOutput", "w") as OutputFile:
            writer = csv.DictWriter(OutputFile, SingleOutputList[0].keys())
            writer.writeheader()
            for row in SingleOutputList:
                writer.writerow(row)

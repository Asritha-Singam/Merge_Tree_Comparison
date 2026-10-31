import MergeTreeLibrary
import sys
import os
import csv
import matplotlib.pyplot as plt
import math
import subprocess
from PIL import Image
import cairosvg
from PIL import ImageFont
from PIL import ImageDraw
def single_compute (MeasureName, FileName1, arg=-1):
    rval = float("inf")
    if MeasureName == "TotalPers":
        rval = MergeTreeLibrary.TotalPers(FileName1, arg)
        draw_tree(FileName1)

    elif MeasureName == "depth":
        T = MergeTreeLibrary.generateTree(FileName1)
        rval = T.depth()

    elif MeasureName == "feat":
        T = MergeTreeLibrary.generateTree(FileName1)
        rval = T.countFeatures()

    return rval

def pair_compute (MeasureName, FileName1, FileName2, CompIDFolderName):

    rval = float("inf")
    if not os.path.exists(CompIDFolderName):
        os.mkdir(CompIDFolderName)

    if MeasureName == "bottleneck":
        rval = MergeTreeLibrary.bottleneck(FileName1, FileName2)


    elif MeasureName == "Wasserstein":
        rval = MergeTreeLibrary.Wasserstein(FileName1, FileName2)
        SeeMatching(FileName1,FileName2)

    elif MeasureName == "COMTED":
        rval = MergeTreeLibrary.COMTED(FileName1, FileName2, CompIDFolderName)

    elif MeasureName == "CWMTED":
        rval = MergeTreeLibrary.CWMTED(FileName1, FileName2, CompIDFolderName)

    return rval

def output_normalisation (MeasureName, FileName1, FileName2):

    rval = 1

    if MeasureName == "bottleneck":
        rval = MergeTreeLibrary.TotalPers(FileName1, -1) * MergeTreeLibrary.TotalPers(FileName2, -1)

    elif MeasureName == "COMTED":
        rval = MergeTreeLibrary.TotalPers(FileName1, 1) + MergeTreeLibrary.TotalPers(FileName2, 1)

    elif MeasureName == "CWMTED":
        rval = MergeTreeLibrary.TotalPers(FileName1, 1) + MergeTreeLibrary.TotalPers(FileName1, 1)

    return rval

def single_normalisation (MeasureName, FileName1):
    if MeasureName == "TotalPers":
        rval = MergeTreeLibrary.TotalPers(FileName1, -1)
    return rval

def draw_plot (MeasureName, FileName1, FileName2, OutputFileName, MatchingFileName = None, normalised = False):

    if MeasureName == "bottleneck" or MeasureName == "wasserstein":
        T1 = MergeTreeLibrary.generateTree(FileName1)
        T2 = MergeTreeLibrary.generateTree(FileName2)

        T1.generatePersistencePairings()
        T2.generatePersistencePairings()

        NodeCoords1 = T1.persistenceDiagram()
        NodeCoords2 = T2.persistenceDiagram()

        XArrs1 = [row[0] for row in NodeCoords1]
        YArrs1 = [row[1] for row in NodeCoords1]

        XArrs2 = [row[0] for row in NodeCoords2]
        YArrs2 = [row[1] for row in NodeCoords2]

        fog = plt.figure(OutputFileName)
        axis = fog.add_subplot(111)

        axis.scatter(XArrs1, YArrs1, c = 'b', label = FileName1)
        axis.scatter(XArrs2, YArrs2, c = 'r', label = FileName2)

        plt.legend()
        plt.savefig(OutputFileName)
        plt.close()

    elif MeasureName == "pers_dist":
        T1 = MergeTreeLibrary.generateTree(FileName1)
        pers = T1.persistenceDiagram()
        listabsvals = list()
        for point in pers:
            listabsvals.append(abs(point[0] - point[1]))
        listabsvals.sort()
        if normalised == True:
            listabsvals = [row / T1.TotalPers(-1) for row in listabsvals]
        plt.figure(OutputFileName)
        plt.scatter(range(len(listabsvals)), listabsvals, c = "b")
        plt.savefig(OutputFileName)
        plt.close()

    elif MeasureName == "COMTED" or MeasureName == "CWMTED":

        TFName1 = FileName1.strip(".").replace("/", "_")
        TFName2 = FileName2.strip(".").replace("/", "_")

        s = "digraph { \n"

        s1 = ""
        with open(FileName1) as IF1:

            n = int(IF1.readline().strip())
            s1 = "subgraph cluster_1 {\n style=filled \n; color=lightgrey; \n label=" + TFName1 + ";"
            for i in range(n):
                ni = IF1.readline().strip().split(' ')
                if ni[1] != "-1":
                    s1 = s1 + TFName1 + "_" + ni[1] + " -> " + TFName1 + "_" + ni[0] + ";\n"
            s1 = s1 + "}"

        s2 = ""
        with open(FileName2) as IF2:
            n = int(IF2.readline().strip())
            s2 = "subgraph cluster_2 {\n style=filled \n; color=lightgrey; \n label=" + TFName2 +";"
            for i in range(n):
                ni = IF2.readline().strip().split(' ')
                if ni[1] != "-1":
                    s2 = s2 + TFName2 + "_" + ni[1] + " -> " + TFName2 + "_" + ni[0] + ";\n"
            s2 = s2 + "}"

        sM = ""
        with open(MatchingFileName) as MatchFile:
            for line in MatchFile.readlines():
                MapNode = line.strip().split(",")
                sM = sM + TFName1 + "_" + MapNode[0] + " -> " + TFName2 + "_" + MapNode[1] + "\n"

        s = s + s1 + s2 + sM
        s = s + "}"

        with open(OutputFileName, "w") as OF1:
            OF1.write(s)

def pairwiseCompare (InputDir, OutputDir, T1Name, T2Name, MeasuresTBC):

    OutputDict = dict()

    CompID = T1Name + T2Name
    OutputDict["ID"] = CompID

    Tree1 = MergeTreeLibrary.generateTree(InputDir + "/" + T1Name)
    Tree2 = MergeTreeLibrary.generateTree(InputDir + "/" + T2Name)

    Tree1.generatePersistencePairings()
    Tree2.generatePersistencePairings()

    for Measure in MeasuresTBC:
        MeasureName = Measure[0]
        MeasureArgs = list()
        if len(Measure) > 1:
            MeasureArgs = Measure[1].split(" ")

        raw_compute = pair_compute(MeasureName, InputDir + "/" + T1Name, InputDir + "/" + T2Name, OutputDir + "/" + CompID)
        OutputDict[MeasureName] = raw_compute

        innormval = float("inf")
        if "in_normal" in MeasureArgs:

            Tree1Normalised = Tree1.normalise()
            Tree2Normalised = Tree2.normalise()

            Tree1Normalised.generatePersistencePairings()
            Tree2Normalised.generatePersistencePairings()

            Tree1Normalised.genTreeFile(Tree1Normalised.genCEDTInput(), InputDir + "/" + T1Name + "normalised")
            Tree2Normalised.genTreeFile(Tree2Normalised.genCEDTInput(), InputDir + "/" + T2Name + "normalised")

            innormval = pair_compute (MeasureName, InputDir + "/" + T1Name + "normalised", InputDir + "/" + T2Name + "normalised", OutputDir + "/" + CompID + "/normalised")
            if innormval < float("inf"):
                OutputDict["InputNormalised" + MeasureName] = innormval

        if "out_normal" in MeasureArgs:
            OutputDict["OutputNormalised" + MeasureName] = raw_compute / output_normalisation(MeasureName, InputDir + "/" + T1Name, InputDir + "/" + T2Name)

        if "plots" in MeasureArgs:
            OutputFileName = OutputDir + "/" + CompID + "/" + MeasureName + "map"
            stringmap = ""
            if MeasureName == "CWMTED":
                stringmap = "CW"
            elif MeasureName == "COMTED":
                stringmap = "CO"
            draw_plot(MeasureName, InputDir + "/" + T1Name, InputDir + "/" + T2Name, OutputFileName, OutputDir + "/" + CompID + "/forwardmap" + stringmap + ".txt")

    return OutputDict

def soloCompare (InputDir, OutputDir, T1Name, SingleMeasuresTBC):
    OutputDict = dict()
    CompID = T1Name
    OutputDict["ID"] = CompID
    Tree1 = MergeTreeLibrary.generateTree(InputDir + "/" + T1Name)
    Tree1.generatePersistencePairings()

    for Measure in SingleMeasuresTBC:
        MeasureName = Measure[0]
        MeasureArgs = list()
        if len(Measure) > 1:
            MeasureArgs = Measure[1].split(" ")

        if MeasureName == "TotalPers":
            power = float(MeasureArgs[1])
            raw_compute = single_compute(MeasureName, InputDir + "/" + T1Name, power)
            OutputDict[MeasureName + str(power)] = raw_compute
            if "normal" in MeasureArgs:
                OutputDict["Normalised" + MeasureName + str(power)] = raw_compute / single_normalisation(MeasureName, InputDir + "/" + T1Name)

        elif MeasureName == "depth" or MeasureName == "feat":
            raw_compute = single_compute(MeasureName, InputDir + "/" + T1Name)
            OutputDict[MeasureName] = raw_compute

        elif MeasureName == "pers_dist":
            if "normal" in MeasureArgs:
                draw_plot("pers_dist", InputDir + "/" + T1Name, None, OutputDir + "/" + T1Name, None, True)
            else:
                draw_plot("pers_dist", InputDir + "/" + T1Name, None, OutputDir + "/" + T1Name, None, False)

    return OutputDict

def GenTreeFromJt (FileNameJT):
    with open(FileNameJT) as JTFile:
        with open(FileNameJT.replace(".jt", ""), "w") as TreeFile:
            list_lines = list()
            n = JTFile.readline()
            TreeFile.write(n)
            for i in range(int(n)):
                newline = list()
                nodeLine = JTFile.readline().strip().split()
                newline.append(nodeLine[0])
                newline.append(nodeLine[1])
                fVal = nodeLine[4]
                nodeChildren = nodeLine[5:]
                newline.extend(nodeChildren)
                newline.append(fVal + "\n")
                list_lines.append(newline)

            text_list_lines = list()
            for nodeData in list_lines:
                text_list_lines.append(" ".join(nodeData))

            TreeFile.writelines(text_list_lines)

def draw_tree(FileName):
    colors = ['red','blue','green','black', 'brown', 'yellow', 'sienna'] # More colors maybe needed on larger trees.
    c = 0
    per_colored = []
    FileName = FileName+ ".jt"
    Filedot = FileName.replace(".jt",".dot")
    file1 = open(Filedot,"a")
    file1.truncate(0)
    file2 = open(FileName,"r")
    file1.write("digraph {\n")
    data = file2.readlines()
    num_nodes = int(data[0].replace('\n',''))
    #print(num_nodes)
    lst = []# This contains the details of all the nodes
    for i in range(1 , num_nodes + 1 ):
        lst.append(data[i].replace('\n','').split())

    for i in lst:
        val = round(float(i[4]),5)
        num_child = int(i[5])
        for j in range(num_child):
            child = int(i[5+j+1])
            for k in range(num_nodes):
                if child == int(lst[k][0]):
                    child_val = round(float(lst[k][4]),5)
                    file1.write(str(val)+"->"+str(child_val)+"\n")
    # Now to add the colourings for the persistence pairs
    for i in lst:
        val = round(float(i[4]),5)
        per_ID = int(i[2])
        for j in lst:
            if per_ID == int(j[0]):
                per_value = round(float(j[4]),5)
        if val not in per_colored:
           file1.write(str(val)+"[color="+colors[c]+"]\n")
           file1.write(str(per_value)+"[color="+colors[c]+"]\n")
           per_colored.append(val)
           per_colored.append(per_value)
           c = c + 1
    file1.write("}")
    file1.close()
    file2.close()
    FileName = FileName.replace("/Inputs","")
    os.system("dot -Tsvg " + Filedot + " > " +  FileName.replace(".jt",".svg"))
    os.remove(Filedot)

def SeeMatching(FileName1,FileName2):
    if FileName1 != FileName2:
        matching = subprocess.run(["java -cp CustomisedWasserstein SeeMatching.java "+ FileName1 + ".jt " + FileName2 + ".jt ./"],shell=True, capture_output=True,text=True).stdout.strip()
        matching = matching.replace('[','')
        matching = matching.replace(']','')
        matching = matching.replace('(','')
        matching = matching.replace(')','')
        matching = matching.replace(':','')
        matching = matching.replace(',','')
        lst = matching.split()
        #Now to compute the pairs
        pairs = []
        for i in range(0,len(lst),2):
            pairs.append([int(lst[i]),int(lst[i+1])])
        pair1 = []
        pair2 = []
        for i in pairs:
            pair1.append(i[0])
            pair2.append(i[1])
        # Now the matchings are ready
        colors = ['red','blue','green', 'brown', 'yellow', 'sienna'] # More colors maybe needed on larger trees.
        FileName1 = FileName1 + ".jt"
        Filedot1 = FileName1.replace(".jt",".dot")
        file1 = open(Filedot1,"a")
        file1.truncate(0)
        file2 = open(FileName1,"r")
        file1.write("digraph {\n")
        data = file2.readlines()
        num_nodes = int(data[0].replace('\n',''))
        #print(num_nodes)
        lst1 = []# This contains the details of all the nodes
        for i in range(1 , num_nodes + 1 ):
            lst1.append(data[i].replace('\n','').split())

        for i in lst1:
            val = round(float(i[4]),5)
            num_child = int(i[5])
            for j in range(num_child):
                child = int(i[5+j+1])
                for k in range(num_nodes):
                    if child == int(lst1[k][0]):
                        child_val = round(float(lst1[k][4]),5)
                        file1.write(str(val)+"->"+str(child_val)+"\n")

        FileName2 = FileName2 + ".jt"
        Filedot2 = FileName2.replace(".jt",".dot")
        file3 = open(Filedot2,"a")
        file3.truncate(0)
        file4 = open(FileName2,"r")
        file3.write("digraph {\n")
        data = file4.readlines()
        num_nodes = int(data[0].replace('\n',''))
        #print(num_nodes)
        lst2 = []# This contains the details of all the nodes
        for i in range(1 , num_nodes + 1 ):
            lst2.append(data[i].replace('\n','').split())

        for i in lst2:
            val = round(float(i[4]),5)
            num_child = int(i[5])
            for j in range(num_child):
                child = int(i[5+j+1])
                for k in range(num_nodes):
                    if child == int(lst2[k][0]):
                        child_val = round(float(lst2[k][4]),5)
                        file3.write(str(val)+"->"+str(child_val)+"\n")
        # Now to colour the nodes on both the graphs appropriately depicting the bipartite the matching
        c = 0
        for i in range(len(pair1)):
            if pair1[i]!=-1 and pair2[i]!=-1:
                for j in range(len(lst1)):
                    if int(lst1[j][0])==pair1[i]:
                        val = round(float(lst1[j][4]),5)
                        perID = int(lst1[j][2])
                        for k in range(len(lst1)):
                            if int(lst1[k][0])==perID:
                                perval = round(float(lst1[k][4]),5)
                                file1.write(str(val)+"[color="+colors[c]+"]\n")
                                file1.write(str(perval)+"[color="+colors[c]+"]\n")
                for j in range(len(lst2)):
                    if int(lst2[j][0])==pair2[i]:
                        val = round(float(lst2[j][4]),5)
                        perID = int(lst2[j][2])
                        for k in range(len(lst2)):
                            if int(lst2[k][0])==perID:
                                perval = round(float(lst2[k][4]),5)
                                file3.write(str(val)+"[color="+colors[c]+"]\n")
                                file3.write(str(perval)+"[color="+colors[c]+"]\n")
                c = c + 1
        file1.write("}")
        file1.close()
        file2.close()
        file3.write("}")
        file3.close()
        file4.close()
        #FileName = FileName.replace("/Inputs","")
        os.system("dot -Tsvg " + Filedot1 + " > " +  FileName1.replace(".jt",".svg"))
        os.system("dot -Tsvg " + Filedot2 + " > " +  FileName2.replace(".jt",".svg"))
        cairosvg.svg2png(url=FileName1.replace(".jt",".svg"), write_to=FileName1.replace(".jt",".png"))
        cairosvg.svg2png(url=FileName2.replace(".jt",".svg"), write_to=FileName2.replace(".jt",".png"))
        os.remove(FileName1.replace(".jt",".svg"))
        os.remove(FileName2.replace(".jt",".svg"))
        # For combining the images

        images = [Image.open(x) for x in [FileName1.replace(".jt",".png"), FileName2.replace(".jt",".png")]]
        widths, heights = zip(*(i.size for i in images))
        total_width = sum(widths)
        max_height = max(heights)
        new_im = Image.new('RGB', (total_width, max_height))
        x_offset = 0
        for im in images:
          new_im.paste(im, (x_offset,0))
          x_offset += im.size[0]
        Tree1 = FileName1.replace("./Examples/Inputs/","")
        Tree1 = Tree1.replace(".jt","")
        Tree2 = FileName2.replace("./Examples/Inputs/","")
        Tree2 = Tree2.replace(".jt","")
        new_im.save("./Examples/" + "WM" + Tree1 + Tree2 + ".png")
        os.remove(Filedot1)
        os.remove(Filedot2)
        os.remove(FileName1.replace(".jt",".png"))
        os.remove(FileName2.replace(".jt",".png"))

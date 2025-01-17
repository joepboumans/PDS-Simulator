import matplotlib
import matplotlib.pyplot as plt

import pandas as pd
import glob
import re
import math
import os
from matplotlib.ticker import ScalarFormatter
from utils import *


def plot_boxplot(metric, title):
    fig, axs = plt.subplots(1,1)
    axs.boxplot(list(metric.values()))
    axs.set_xticks(range(1, len(metric.keys()) +1))
    axs.set_xticklabels(list(metric.keys()))
    axs.set_title(title)
    fig.tight_layout()

def plot_graph(metric, title):
    fig, axs = plt.subplots(1,1)
    for name, data in metric.items():
        for d in data:
            axs.plot(d, label=name)
    # axs.set_xticks(range(1, len(metric.keys()) +1))
    # axs.set_xticklabels(list(metric.keys()))
    axs.legend()
    axs.set_title(title)
    fig.tight_layout()

def add2metric(df, metric, df_name, pds_name):
    if pds_name in metric:
        metric[pds_name].extend(df[df_name].tolist())
    else:
        metric.update({pds_name : df[df_name].tolist()})
    return metric

def append2metric(df, metric, df_name, pds_name):
    if pds_name in metric:
        metric[pds_name].append(df[df_name].tolist())
    else:
        metric.update({pds_name : [df[df_name].tolist()]})
    return metric

def sum_single_results(data):
    l = {}
    for s in data:
        if s[1] in l:
            l[s[1]].extend([float(x) for x in s[0]])
        else:
            l[s[1]] = [float(x) for x in s[0]]
    return l

def plotMetricsByTupleType(tuple_name, csv):
    print(f"Plotting for {tuple_name}")
    results_amq = [x for x in csv if "AMQ" in x and not "FC" in x]
    results_fc = [x for x in csv if "FC" in x and not "AMQ" in x]
    results_amq_fc = [x for x in csv if "FC" in x and "AMQ" in x]

    # AMQ
    if results_amq:
        INSERTS = {}
        COLLISIONS = {}
        F1 = {}

        for res in results_amq:
            df = pd.read_csv(res)
            df.dropna()
            pds_name = res.split('/')[3] # results/<TupleType>/<PDS name>/
            sz = re.findall(r"_(\d+)", res)
            name = pds_name + " "+ " ".join(sz)

            F1 = add2metric(df, F1, 'F1', name)
            INSERTS = add2metric(df, INSERTS, 'Insertions', name)
            COLLISIONS = add2metric(df, COLLISIONS, 'Collisions', name)

        plot_boxplot(F1, 'F1')
        plot_boxplot(INSERTS, 'Insertions')
        plot_boxplot(COLLISIONS, 'Collisions')

    # FC
    if results_fc:
        AAE = {}
        ARE = {}
        F1 = {}

        for res in results_fc:
            df = pd.read_csv(res)
            df.dropna()
            pds_name = res.split('/')[3] # results/<TupleType>/<PDS name>/
            sz = re.findall(r"_(\d+)", res)
            name = pds_name + " "+ " ".join(sz)

            AAE = add2metric(df, AAE, 'Average Absolute Error', name)
            ARE = add2metric(df, ARE, 'Average Relative Error', name)
            F1 = add2metric(df, F1, 'F1', name)

        plot_boxplot(AAE, 'Average Absolute Error')
        plot_boxplot(ARE, 'Average Relative Error')
        plot_boxplot(F1, 'F1')


    # AMQ FC
    if results_amq_fc:
        AAE = {}
        ARE = {}
        F1_HH = {}
        INSERTS = {}
        COLLISIONS = {}
        F1_MEMBER = {}

        for res in results_amq_fc:
            df = pd.read_csv(res)
            df.dropna()
            pds_name = res.split('/')[3] # results/<TupleType>/<PDS name>/
            sz = re.findall(r"_(\d+)", res)
            name = pds_name + " "+ " ".join(sz)

            AAE = add2metric(df, AAE, 'Average Absolute Error', name)
            ARE = add2metric(df, ARE, 'Average Relative Error', name)
            F1_HH = add2metric(df, F1_HH, 'F1 Heavy Hitter', name)
            F1_MEMBER = add2metric(df, F1_MEMBER, 'F1 Member', name)
            INSERTS = add2metric(df, INSERTS, 'Insertions', name)
            COLLISIONS = add2metric(df, COLLISIONS, 'Collisions', name)

        plot_boxplot(AAE, 'Average Absolute Error')
        plot_boxplot(ARE, 'Average Relative Error')
        plot_boxplot(F1_HH, 'F1 Heavy Hitter')
        plot_boxplot(F1_MEMBER, 'F1 Member')
        plot_boxplot(INSERTS, 'Insertions')
        plot_boxplot(COLLISIONS, 'Collisions')

def plotEstimationByTupleType(tuple_name, csv):
    print(f"Plotting for EM results of {tuple_name}")
    results = [x for x in csv if "FC" in x ]

    # FC
    if results:
        ET = {}
        ETT = {}
        WMRE = {}
        WMRE_FINAL = {}
        CARD = {}

        for res in results:
            df = pd.read_csv(res)
            df.dropna()
            pds_name = res.split('/')[3] # results/<TupleType>/<PDS name>/
            sz = re.findall(r"_(\d+)", res)
            name = pds_name + " "+ " ".join(sz)

            ET = add2metric(df, ET, 'Estimation Time', name)
            ETT = add2metric(df.tail(1), ETT, 'Total Time', name)
            WMRE = append2metric(df, WMRE, 'Weighted Mean Relative Error', name)
            WMRE_FINAL = add2metric(df.tail(1), WMRE_FINAL, 'Weighted Mean Relative Error', name)

        print(ET)
        print(ETT)
        print(WMRE)
        print(WMRE_FINAL)

        plot_boxplot(ET, 'Estimation Time')
        plot_boxplot(ETT, 'Total Time')
        plot_graph(WMRE, 'Weighted Mean Relative Error')
        plot_boxplot(WMRE_FINAL, 'Weighted Mean Relative Error')


if __name__ == "__main__":
    set_test_rcs()
    # Get results
    csv_results = glob.glob("results/*/*/*/*.csv")

    tuple_lists = {"FlowTuple" : [], "FiveTuple" : [], "ScrTuple" : []}
    for k, l in tuple_lists.items():
        for csv in csv_results:
            if k in csv and not "em" in csv:
                l.append(csv)
        
    # for t, l in tuple_lists.items():
    #     plotMetricsByTupleType(t, l)

    tuple_lists = {"FlowTuple" : [], "FiveTuple" : [], "ScrTuple" : []}
    for k, l in tuple_lists.items():
        for csv in csv_results:
            if k in csv and "em" in csv:
                l.append(csv)
    print(tuple_lists)

    for t, l in tuple_lists.items():
        plotEstimationByTupleType(t, l)

    plt.show()

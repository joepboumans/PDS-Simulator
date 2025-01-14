import matplotlib
# matplotlib.use('qtagg')
import matplotlib.pyplot as plt

import pandas as pd
import glob
import re
import math
import os
from matplotlib.ticker import ScalarFormatter

if __name__ == "__main__":
    # Get results
    csv_results = glob.glob("results/*/*/*.csv")
    csv_em_results = [x for x in csv_results if "em" in x ]
    csv_results = [x for x in csv_results if not "em" in x ]

    tuple_lists = {"FlowTuple" : [], "FiveTuple" : [], "ScrTuple" : []}
    for k, l in tuple_lists.items():
        for csv in csv_results:
            if k in csv:
                l.append(csv)
        

    tuple_df = {"FlowTuple" : {}, "FiveTuple" : {}, "ScrTuple" : {}}
    for t, l in tuple_lists.items():
        for csv in l:
            df = pd.read_csv(csv)
            df.dropna()
            name = csv.split('/')[2] # results/<TupleType>/<PDS name>/
            df['Name'] = name
            dataset = csv.split('/')[3] # results/<TupleType>/<PDS name>/
            df['Dataset'] = dataset
            if not name in tuple_df[t]:
                df_pds = pd.DataFrame()
                df_pds = pd.concat([df_pds, df])
                tuple_df[t].update({name : df_pds})
                print(df_pds)
            else:
                tuple_df[t][name] = pd.concat([tuple_df[t][name], df])
                print(tuple_df[t][name])

    for t, l in tuple_df.items():
        for name, df in l.items():
            tuple_df[t][name].boxplot(by=['Name', 'Dataset'], rot=45)
    plt.show()
    # print(tuple_df)

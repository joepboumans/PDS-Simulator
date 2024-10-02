import matplotlib.pyplot as plt
import pandas as pd
import glob
import re
import math
import os


def sort_names(names):
    new_names = []
    for name in names:
        new_names.append(name.split("_"))
    new_names = sorted(new_names, key = lambda x: (x[0], x[1], x[2], int(x[3]), int(x[4])) if len(x) > 4 else (x[0], x[1], x[2], int(x[3])), reverse=True)
    new_names = ["_".join(name) for name in new_names]

    return new_names

plot_dir = "plots/"

def plot_pds_stats(df, title, n_stage, n_struct, sz, dataset):
    # Approximate Membership Query and Frequency Counter
    if "F1 Heavy Hitter" in df.columns or "F1 Member" in df.columns:
        fig, axs = plt.subplots(6, 1, figsize=(9,9))
        axs[0].set_title("F1 of Heavy Hitter and Membership", fontsize=10)
        df.plot(x = 'Epoch', y = [ "F1 Heavy Hitter", "F1 Member"], ax=axs[0])
        axs[1].set_title("Recordings", fontsize=10)
        df.plot(x = 'Epoch', y = ["Insertions"], ax=axs[1])
        axs[2].set_title("Flow Sizes Estimation", fontsize=10)
        df.plot(x = 'Epoch', y = ["Average Relative Error", "Average Absolute Error"], ax=axs[2])
        axs[3].set_title("Flow Size Distribution", fontsize=10)
        df.plot(x = 'Epoch', y = ["Weighted Mean Relative Error"], ax=axs[3])
        axs[4].set_title("Estimation Time", fontsize=10)
        df.plot(x = 'Epoch', y = ["Estimation Time"], ax=axs[4])
        axs[5].set_title("Average ET per iteration", fontsize=10)
        df.plot(x = 'Epoch', y = "Average ET", ax=axs[5])
        sz = sz.split("_")
        sz[-1] = str(math.floor(int(sz[-1])/1024))
        sz = " ".join(sz)
    # Approximate Membership Query
    elif "Insertions" in df.columns:
        fig, axs = plt.subplots(2, 1, figsize=(9,9))
        axs[0].set_title("Performance metrics", fontsize=10)
        df.plot(x = 'Epoch', y = ["Recall", "Precision", "F1"], ax=axs[0])
        axs[1].set_title("Recordings", fontsize=10)
        df.plot(x = 'Epoch', y = ["Insertions"], ax=axs[1])
        sz = int(int(sz)/1024)
    # Frequency Counter
    else:
        fig, axs = plt.subplots(5, 1, figsize=(9,9))
        axs[0].set_title("Heavy Hitters", fontsize=10)
        df.plot(x = 'Epoch', y = ["Recall", "Precision", "F1"], ax=axs[0])
        axs[1].set_title("Flow Sizes Estimation", fontsize=10)
        df.plot(x = 'Epoch', y = ["Average Relative Error", "Average Absolute Error"], ax=axs[1])
        axs[2].set_title("Flow Size Distribution", fontsize=10)
        df.plot(x = 'Epoch', y = ["Weighted Mean Relative Error"], ax=axs[2])
        axs[3].set_title("Estimation Time", fontsize=10)
        df.plot(x = 'Epoch', y = ["Estimation Time"], ax=axs[3])
        if "Iterations" in df:
            axs[4].set_title("Average ET per iteration", fontsize=10)
            df.plot(x = 'Epoch', y = "Average ET", ax=axs[4])
        sz = sz.split("_")
        sz[-1] = str(math.floor(int(sz[-1])/1024))
        sz = " ".join(sz)

    fig.suptitle(f"{title} {n_stage}.{n_struct} {sz} KB\n {dataset}")
    plt.subplots_adjust(wspace=0.5, hspace=0.5)
    # Store figures 
    if(not os.path.isdir(f"{plot_dir}/{title}")):
        print(f"Making dir {plot_dir}/{title}")
        os.makedirs(f"{plot_dir}/{title}")
    plt.savefig(f"{plot_dir}/{title}/{n_stage}_{n_struct}_{dataset}_{sz}.png", transparent=True)


def parse_total_results(dfs, columns):
    scores = {}
    for column in columns:
        score = pd.DataFrame()
        for name, df in dfs.items():
            reg_name = r"(?P<stage>\d+_\d+)_[\w-]+_(?P<sz>\d+_\d+)_(?P<name>\w+)"
            match = re.search(reg_name, name)
            if not match:
                print(f"[ERROR] Cannot parse name, skipping data {name}")
                continue;

            md = match.groupdict()
            name = f"{md['stage']}_{md['name']}_{md['sz']}"
            # name = match.group("name")

            idf = pd.DataFrame(df[column])
            idf = pd.DataFrame.rename(idf, columns={column:name})
            score = pd.concat([score.reset_index(drop=True), idf.reset_index(drop=True)], ignore_index=True, sort=False)

        clean_scores = pd.DataFrame(columns=score.columns)
        for c in score.columns:
            clean_scores[c] = score[c].dropna().reset_index(drop=True)
        score = clean_scores
        score = score.reindex(sort_names(score.columns), axis=1)
        scores[column]= score

    return scores

def plot_total_results(total_df, columns):
    scores = parse_total_results(total_df, columns)
    if not "Epoch" in scores:
        print("Epoch not found, not printing total results")
        return
    scores.pop('Epoch')
    # rows = len(scores) #math.ceil(len(scores)  / 3)
    # fig2, axes = plt.subplots(rows, 1, figsize=(10,15))
    # plt.subplots_adjust(wspace=0.5, hspace=0.5)

    for (param, score),i in zip(scores.items(), range(len(scores))):
        if param == "Epoch":
            continue
        fig2, ax = plt.subplots(figsize=(9,9))
        if param == "Average Relative Error" or param == "Average Absolute Error" or param == "Estimation Time" or param == "Weighted Mean Relative Error":
            ax.set_xscale('log')
        ax.set_title(param)
        score.plot.box(ax=ax, vert=False, grid=True)
        ax.set_yticklabels(ax.get_yticklabels(), fontsize=8)
        # plt.title(param)
        plt.tight_layout()
        plt.savefig(f"{plot_dir}{param}_overview.png", transparent=True)

if __name__ == "__main__":
    # Get results
    csv_results = glob.glob("results/*/*.csv")

    # total_df = {}
    amq_fc_df = {}
    amq_fc_columns = []
    amq_df = {}
    amq_columns = []
    fc_df = {}
    fc_columns = []
    # Parse results
    reg_string = r"\/(?P<data_name>[^_]+?)\/(?P<name>[\w-]+?)_(?P<n_stage>\d+)_(?P<n_struct>\d+)_(?P<sz>(?P<row>\d+)_(?P<col>\d+)_(?P<tot_sz>\d+)|\d+)"
    for result in csv_results:
        match = re.search(reg_string, result)
        if not match:
            print(f"[ERROR] Cannot parse name, skipping file {result}")
            continue;
        if match:
            data_info = match.groupdict()
# "Epoch,Average Relative Error,Average Absolute Error,Weighted Mean Relative Error,Recall,Precision,F1
# Epoch,total data,total pds,false pos,false neg,recall,precision,f1
            df = pd.read_csv(result) 
            if df.empty:
                print(f"{result} is empty")
                continue
            
            if data_info['row']:
                if "F1 Heavy Hitter" in df.columns or "F1 Member" in df.columns:
                    if "Iterations" in df:
                        df["Average ET"] = df["Estimation Time"] / df["Iterations"]
                    amq_fc_df[f"{data_info['n_stage']}_{data_info['n_struct']}_{data_info['name']}_{data_info['row']}_{data_info['col']}_{data_info['tot_sz']}_{data_info['data_name']}"] = df
                    amq_fc_columns = df.columns
                elif "Average Relative Error" in df.columns:
                    if "Iterations" in df:
                        df["Average ET"] = df["Estimation Time"] / df["Iterations"]
                    fc_df[f"{data_info['n_stage']}_{data_info['n_struct']}_{data_info['name']}_{data_info['row']}_{data_info['col']}_{data_info['tot_sz']}_{data_info['data_name']}"] = df
                    fc_columns = df.columns
                else:
                    amq_df[f"{data_info['n_stage']}_{data_info['n_struct']}_{data_info['name']}_{data_info['row']}_{data_info['col']}_{data_info['tot_sz']}_{data_info['data_name']}"] = df
                    amq_columns = df.columns
            else:
                if "F1 Heavy Hitter" in df.columns or "F1 Member" in df.columns:
                    if "Iterations" in df:
                        df["Average ET"] = df["Estimation Time"] / df["Iterations"]
                    amq_fc_df[f"{data_info['n_stage']}_{data_info['n_struct']}_{data_info['name']}_{data_info['sz']}_{data_info['data_name']}"] = df
                    amq_fc_columns = df.columns
                elif "Average Relative Error" in df.columns:
                    fc_df[f"{data_info['n_stage']}_{data_info['n_struct']}_{data_info['name']}_{data_info['sz']}_{data_info['data_name']}"] = df
                    fc_columns = df.columns
                else:
                    amq_df[f"{data_info['n_stage']}_{data_info['n_struct']}_{data_info['name']}_{data_info['sz']}_{data_info['data_name']}"] = df
                    amq_columns = df.columns
            # plot_pds_stats(df, data_info['data_name'], data_info['n_stage'], data_info['n_struct'], data_info['sz'], data_info['name'])

    if amq_fc_df:
        plot_total_results(amq_fc_df, amq_fc_columns)
    elif amq_df:
        plot_total_results(amq_df, amq_columns)
    elif fc_df:
        plot_total_results(fc_df, fc_columns)
    
    plt.show()

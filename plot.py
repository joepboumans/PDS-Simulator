import matplotlib.pyplot as plt
import pandas as pd
import glob
import re
import math


def sort_names(names):
    new_names = []
    for name in names:
        new_names.append(name.split("_"))
    new_names = sorted(new_names, key = lambda x: (x[0], x[1], x[2], int(x[3]), int(x[4])) if len(x) > 4 else (x[0], x[1], x[2], int(x[3])), reverse=True)
    new_names = ["_".join(name) for name in new_names]

    return new_names

plot_dir = "plots/"

def plot_pds_stats(df, title, n_stage, n_struct, sz, dataset):
    # Setup subplots
    fig, axs = plt.subplots(2, 1, figsize=(9,9))
    plt.subplots_adjust(wspace=0.5, hspace=0.5)
    sz = int(sz)/1024
    fig.suptitle(f"{n_stage}.{n_struct} {title} {sz}KB\n {dataset}")

    # Plot data
    if "F1" in df.columns:
        axs[0].set_title("Performance metrics", fontsize=10)
        df.plot(x = 'epoch', y = ["Recall", "Precision", "F1"], ax=axs[0])
    if "Insertions" in df.columns:
        axs[1].set_title("Recordings", fontsize=10)
        df.plot(x = 'epoch', y = ["Insertions"], ax=axs[1])

    # Store figures 
    plt.savefig(f"{plot_dir}{dataset}_{n_stage}_{n_struct}_{title}_{sz}.png", transparent=True)


def parse_total_results(dfs, columns):
    scores = {}
    for column in columns:
        score = pd.DataFrame()
        for name, df in dfs.items():
            reg_name = r"(?P<name>\d+_\d+_[\w-]+_\d+)"
            match = re.search(reg_name, name)
            if not match:
                print(f"[ERROR] Cannot parse name, skipping data {name}")
                continue;
            name = match.group("name")

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
    scores.pop('epoch')
    # rows = len(scores) #math.ceil(len(scores)  / 3)
    # fig2, axes = plt.subplots(rows, 1, figsize=(10,15))
    # plt.subplots_adjust(wspace=0.5, hspace=0.5)

    for (param, score),i in zip(scores.items(), range(len(scores))):
        if param == "epoch":
            continue
        fig2, ax = plt.subplots(figsize=(9,9))
        ax.set_title(param)
        score.plot.box(ax=ax, vert=False, grid=True)
        ax.set_yticklabels(ax.get_yticklabels(), fontsize=8)
        # plt.title(param)
        plt.tight_layout()
        plt.savefig(f"{plot_dir}overview.png", transparent=True)

if __name__ == "__main__":
    # Get results
    csv_results = glob.glob("results/*.csv")

    # total_df = {}
    amq_df = {}
    amq_columns = []
    fc_df = {}
    fc_columns = []
    # Parse results
    reg_string = r"\/(?P<data_name>[^_]+?)_(?P<name>[\w-]+?)_(?P<n_stage>\d+)_(?P<n_struct>\d+)_(?P<sz>(?P<row>\d+)_(?P<col>\d+)|\d+)"
    for result in csv_results:
        match = re.search(reg_string, result)
        if not match:
            print(f"[ERROR] Cannot parse name, skipping file {result}")
            continue;
        if match:
            data_info = match.groupdict()
# "epoch,Average Relative Error,Average Absolute Error,Weighted Mean Relative Error,Recall,Precision,F1
# epoch,total data,total pds,false pos,false neg,recall,precision,f1
            df = pd.read_csv(result) 
            # plot_pds_stats(df, data_info['name'], data_info['n_stage'], data_info['n_struct'], data_info['sz'], data_info['data_name'])
            
            if data_info['row']:
                if "Average Relative Error" in df.columns:
                    fc_df[f"{data_info['n_stage']}_{data_info['n_struct']}_{data_info['name']}_{data_info['row']}_{data_info['col']}_{data_info['data_name']}"] = df
                    fc_columns = df.columns
                else:
                    amq_df[f"{data_info['n_stage']}_{data_info['n_struct']}_{data_info['name']}_{data_info['row']}_{data_info['col']}_{data_info['data_name']}"] = df
                    amq_columns = df.columns
            else:
                if "Average Relative Error" in df.columns:
                    fc_df[f"{data_info['n_stage']}_{data_info['n_struct']}_{data_info['name']}_{data_info['sz']}_{data_info['data_name']}"] = df
                    fc_columns = df.columns
                else:
                    amq_df[f"{data_info['n_stage']}_{data_info['n_struct']}_{data_info['name']}_{data_info['sz']}_{data_info['data_name']}"] = df
                    amq_columns = df.columns

    if amq_df:
        plot_total_results(amq_df, amq_columns)
    if fc_df:
        plot_total_results(fc_df, fc_columns)
    plt.show()

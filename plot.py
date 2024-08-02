import matplotlib.pyplot as plt
import pandas as pd
import glob
import re

def plot_pds_stats(df, title, n, sz, dataset):
    fig, axs = plt.subplots(2, 1, figsize=(9,9))
    plt.subplots_adjust(wspace=0.5, hspace=0.5)
    sz = int(sz)/1024
    fig.suptitle(f"{title} {n} {sz}KB\n {dataset}")
    # fig.suptitle(dataset, fontsize=10)

    axs[0].set_title("Performance metrics", fontsize=10)
    df.plot(x = 'epoch', y = ["recall", "precision", "f1"], ax=axs[0])
    axs[1].set_title("Recordings", fontsize=10)
    df.plot(x = 'epoch', y = ["total data", "total pds", "false pos", "false neg"], ax=axs[1])

if __name__ == "__main__":
    csv_results = glob.glob("results/*.csv")
    reg_string = r"\/(?P<data_name>.+)_(?P<name>\w+)_(?P<n>\d+)_(?P<sz>\d+)"
    for result in csv_results:
        match = re.search(reg_string, result)
        if not match:
            print(f"[ERROR] Cannot parse name, skipping file {result}")
            continue;
        if match:
            data_info = match.groupdict()
            df = pd.read_csv(result)
            plot_pds_stats(df, data_info['name'], data_info['n'], data_info['sz'], data_info['data_name'])

        plt.show()
        exit()
# epoch,total data,total pds,false pos,false neg,recall,precision,f1
    df = pd.read_csv("results/data1_BloomFilter_1_262144.csv")
# df.plot(x = 'epoch', y = 'f1')
    df.plot(x = 'epoch', y = ["total data", "total pds", "false pos", "false neg"])
    df.plot.box(column = ["recall", "f1"])
# fig, ax = plt.subplots()
# ax.plot([1,2,3,4], [1,4,2,3])



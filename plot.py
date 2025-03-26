import struct
import os
from unicodedata import numeric
import pandas as pd
from pandas.core.generic import common
from pandas.core.series import fmt
import seaborn as sns
import matplotlib.pyplot as plt
from matplotlib.lines import Line2D
from utils import *
from statsmodels.nonparametric.smoothers_lowess import lowess


class Plotter():
    def __init__(self, data, em_data, ns_data):
        self.data = data
        self.em_data = em_data
        self.ns_data = ns_data
        
        # Create a consistent palette mapping for the unique DataStructName groups.
        unique_names = np.append(data['DataStructName'].unique(),em_data['DataStructName'].unique() )
        palette = sns.color_palette("deep", n_colors=len(unique_names))
        self.color_map = dict(zip(unique_names, palette))
        print(f"Using color map : {self.color_map.keys()}")

        # Create a line style mapping for TupleType.
        unique_tuple = sorted(data['TupleType'].unique())
        styles = ['-', '--', '-.', ':']
        self.line_style_map = {tt: styles[i % len(styles)] for i, tt in enumerate(unique_tuple)}

        # Find the maximum Total Time
        maxTime = self.em_data['Total Time'].max()
        extendedRows = []
        for dataStruct in self.em_data['DataStructName'].unique():
            dataStructSet = self.em_data[self.em_data['DataStructName'] == dataStruct]
            for dataName in self.em_data['DataSetName'].unique():
                dataSet = dataStructSet[dataStructSet['DataSetName'] == dataName]
                maxDataTotalTime = dataSet['Total Time'].max()
                if maxDataTotalTime == maxTime:
                    continue
    
                maxEpoch = dataSet['Epoch'].max()
                finalRow = dataSet[dataSet['Epoch'] == maxEpoch]
                finalRow['Total Time'] = maxTime
    
                extendedRows.append(finalRow)

        for row in extendedRows:
            self.em_data = pd.concat([self.em_data, row], ignore_index=True)

        # Interpolate EM data to create smoother time
        commonTimeFrame = np.linspace(0, maxTime, num=500)
        smoothedResults = []
        for dstruct in self.em_data['DataStructName'].unique():
            df_ds = self.em_data[self.em_data['DataStructName'] == dstruct].copy()
            for dset in df_ds['DataSetName'].unique():
                df_dsDs = df_ds[df_ds['DataSetName'] == dset].copy()
                df_dsDs = df_dsDs.set_index('Total Time')
                df_dsDs = df_dsDs.infer_objects(copy=False)

                df_interp = df_dsDs.reindex(df_dsDs.index.union(commonTimeFrame))
                numeric_cols = df_interp.select_dtypes(include=[np.number]).columns
                for col in ['Weighted Mean Relative Error', 'Cardinality', 'Entropy']:
                    df_interp[col] = pd.to_numeric(df_interp[col], errors='coerce')
                df_interp[numeric_cols] = df_interp[numeric_cols].interpolate(method='polynomial', order=5)

                for col in ['TupleType', 'DataStructType', 'DataStructName']:
                    df_interp[col] = df_interp[col].ffill()
                df_interp['DataStruct'] = dstruct
                df_interp['DataSetName'] = dset

                droppingIndex = df_dsDs.index.difference([df_dsDs.index[0], df_dsDs.index[-1]])
                df_interp = df_interp.drop(index=droppingIndex)
                print(df_interp)
                smoothedResults.append(df_interp)

        self.dfSmoothed = pd.concat(smoothedResults)
        numeric_cols = self.dfSmoothed.select_dtypes(include=[np.number]).columns
        # self.dfSmoothedMean = self.dfSmoothed.groupby(numeric_cols).mean()
        # for col in numeric_cols:

        



    def plotSmoothedOverTime(self, data, param, frac):
        # Loop over each combination of DataStructName and TupleType for LOWESS smoothing.
        for (ds, tt), group in data.groupby(['DataStructName', 'TupleType']):
            smoothed = lowess(
                group[param], 
                group.index, 
                frac=frac,
                it=6,
                # delta=1
            )
            plt.plot(smoothed[:, 0], smoothed[:, 1], 
                     label=f"{ds}, {tt}", 
                     linewidth=2, 
                     color=self.color_map[ds],
                     linestyle=self.line_style_map[tt])


    def plotWMRE(self):
        # Set the figure size
        plt.figure(figsize=(10, 6))

        # Scatterplot to show each data point
        # sns.scatterplot(
        #     x='Total Time',
        #     y='Weighted Mean Relative Error',
        #     hue='DataStructName',
        #     style='TupleType',
        #     data=self.em_data,
        #     palette=self.color_map,
        #     s=1
        # )
        sns.lineplot(
            x='Total Time',
            y='Weighted Mean Relative Error',
            hue='DataStructName',
            # style='DataSetName',
            data=self.dfSmoothed,
        )

        # self.plotSmoothedOverTime(self.dfSmoothed, 'Weighted Mean Relative Error', 0.15)
        # Determine the maximum Total Time for the shortest dataset
        shortest_max = self.em_data.groupby('DataStructName')['Total Time'].max().min()
        plt.xlim(self.em_data['Total Time'].min(), shortest_max)
        # Add labels and title
        plt.xlabel('Estimation Time (ms)')
        plt.ylabel('Weighted Mean Relative Error')
        plt.title('Weighted Mean Error over time')
        plt.legend(title='Data Structures')
        plt.tight_layout()

        plt.savefig('plots/WMRE_over_time.pdf', dpi=300, bbox_inches='tight')

    def plotEntropy(self):
        plt.figure(figsize=(10, 6))
        # Scatterplot to show each data point
        # sns.scatterplot(
        #     x='Total Time',
        #     y='Entropy',
        #     hue='DataStructName',
        #     style='TupleType',
        #     data=self.em_data,
        #     palette=self.color_map,
        #     s=1
        # )

        self.plotSmoothedOverTime(self.em_data, 'Entropy', 0.1)
        # Determine the maximum Total Time for the shortest dataset
        shortest_max = self.em_data.groupby('DataStructName')['Total Time'].max().min()
        plt.xlim(self.em_data['Total Time'].min(), shortest_max)

        # Add labels and title
        plt.xlabel('Epoch')
        plt.ylabel('Entropy')
        plt.title('Entropy')
        plt.legend(title='Data Structure Type')
        plt.tight_layout()
        plt.savefig('plots/entropy.pdf', dpi=300, bbox_inches='tight')

    def plotCard(self):
        plt.figure(figsize=(10, 6))
        # Scatterplot to show each data point
        # sns.scatterplot(
        #     x='Total Time',
        #     y='Cardinality',
        #     hue='DataStructName',
        #     style='TupleType',
        #     data=self.em_data,
        #     palette=self.color_map,
        #     s=1
        # )

        self.plotSmoothedOverTime(self.em_data, 'Cardinality', 0.15)
        # Add labels and title
        longest_max = self.em_data.groupby('DataStructName')['Total Time'].max().max()
        shortest_max = self.em_data.groupby('DataStructName')['Total Time'].max().min()
        plt.xlim(self.em_data['Total Time'].min(), shortest_max)
        plt.xlabel('Epoch')
        plt.ylabel('Cardinality')
        plt.title('Cardinality')
        plt.legend(title='Data Structure Type')
        plt.tight_layout()
        plt.savefig('plots/cardinality.pdf', dpi=300, bbox_inches='tight')

    def plotEstimationTime(self):
        filtered_data = self.em_data[self.em_data['Epoch'] != 0]
        print("Filtered data:")
        print(filtered_data.head())

        # Set the figure size
        plt.figure(figsize=(10, 6))
        # Create a line plot for Epoch vs Estimation Time
        sns.lineplot(
            x='Epoch',                          # X-axis: Epoch
            y='Estimation Time',                 # Y-axis: Estimation Time
            hue='DataStructName',               # Group by DataStructType
            style='TupleType',                  # Distinguish by TupleType
            data=filtered_data               # Data to plot
        )

        # Add labels and title
        plt.xlabel('Epoch')
        plt.ylabel('Estimation Time (ms)')
        plt.title('Epoch vs Estimation Time (Estimation Data)')
        plt.legend(title='PDS, Tuple type')
        plt.tight_layout()
        plt.savefig('plots/epoch_vs_estimation_time.pdf', dpi=300, bbox_inches='tight')

    def plotTotalEstimationTime(self):
        # Set up the figure and subplots
        fig, axes = plt.subplots(1, 2, figsize=(14, 6))  # 1 row, 2 columns
        filtered_data = self.em_data[self.em_data['Epoch'] != 0]

        # Left Subplot: Total Time
        sns.boxplot(
            x='DataStructName',                 # X-axis: Data Structure Type
            y='Total Time',                      # Y-axis: Total Time
            hue='TupleType',                    # Group by TupleType
            data=filtered_data,              # Data to plot
            ax=axes[0]                          # Assign to the left subplot
        )
        axes[0].set_xlabel('Data Structure Type')
        axes[0].set_ylabel('Total Time (ms)')
        axes[0].set_title('Distribution of Total Time')
        axes[0].legend(title='PDS, Tuple Type')

        # Right Subplot: Estimation Time
        sns.boxplot(
            x='DataStructName',                 # X-axis: Data Structure Type
            y='Estimation Time',                 # Y-axis: Estimation Time
            hue='TupleType',                    # Group by TupleType
            data=filtered_data,              # Data to plot
            ax=axes[1]                          # Assign to the right subplot
        )
        axes[1].set_xlabel('Data Structure Type')
        axes[1].set_ylabel('Estimation Time (ms)')
        axes[1].set_title('Distribution of Estimation Time')
        axes[1].legend(title='PDS, Tuple Type')

        # Adjust layout and show the plot
        plt.tight_layout()

        # Save the plots
        plt.savefig('plots/time_distribution_boxplot.pdf', dpi=300, bbox_inches='tight')

    def plotNS(self):
        # Set the figure size
        plt.figure(figsize=(10, 6))

        # Create a line plot for Epoch vs Weighted Mean Relative Error
        sns.lineplot(
            x="Flow Size",
            y="Frequency",
            hue="DataSetName",
            style="DataStructName",
            markers=True,
            data=self.ns_data
        )


        # Add labels and title
        plt.legend(title='Data Structure Type')
        plt.xscale("log")
        print(self.ns_data["Flow Size"].max().max())
        plt.xlim(0.9, self.ns_data["Flow Size"].max().max())
        plt.ylim(0.1, self.ns_data["Frequency"].max().max() * 1.1)
        plt.xlabel("Value")
        plt.yscale("log")
        plt.ylabel("Frequency")
        plt.title("Frequency Plot")
        plt.tight_layout()
        plt.savefig('plots/fsd.pdf', dpi=300, bbox_inches='tight')

    def plotNSdelta(self):
        # Set the figure size
        plt.figure(figsize=(10, 6))

        # Create a line plot for Epoch vs Weighted Mean Relative Error
        sns.lineplot(
            x="Flow Size",
            y="Delta",
            # hue="Epoch",
            # style="DataStructName",
            hue="DataStructName",
            markers=True,
            data=self.ns_data
        )

        # Add labels and title
        plt.legend(title='Data Structure Type')
        plt.xscale("log")
        plt.xlabel("Flow Size")
        # plt.yscale("log")
        plt.ylabel("Delta")
        plt.title("Frequency Plot")
        plt.tight_layout()
        plt.savefig('plots/delta_fsd.pdf', dpi=300, bbox_inches='tight')

    def plotF1Membership(self):
        # Remove FC's for F1 Membership
        amqData = self.data[self.data['DataStructName'] != "FCM-Sketches"]
        df_melted = amqData.melt(id_vars='DataStructName', var_name='Metric', value_name='Value', value_vars=['F1', 'Precision', 'Recall'])

        # Create a scatterplot plot for Epoch vs Weighted Mean Relative Error
        plt.figure(figsize=(10, 6))
        sns.boxplot(
            x='DataStructName',
            y='Value',      
            hue='Metric',
            data=df_melted,               # Data to plot
        )

        # Add labels and title
        plt.xlabel('Data Structurres')
        plt.ylabel('F1 Score')
        plt.title('Membership Scores')
        plt.legend(title='Data Structures')
        plt.tight_layout()

        plt.savefig('plots/F1Membership.pdf', dpi=300, bbox_inches='tight')

    def plotBandwidth(self):
        # Remove FC's for F1 Membership
        amqData = self.data[self.data['DataStructName'] != "FCM-Sketches"]
        df_melted = amqData.melt(id_vars='DataStructName', var_name='Metric', value_name='Value', value_vars=['Collisions', 'Insertions'])

        # Create a scatterplot plot for Epoch vs Weighted Mean Relative Error
        plt.figure(figsize=(10, 6))
        sns.boxplot(
            x='DataStructName',
            y='Value',      
            hue='Metric',
            data=df_melted,               # Data to plot
        )

        # Add labels and title
        plt.xlabel('Data Structurres')
        plt.ylabel('Insertions and Collisions')
        plt.title('Bandwidth Usage')
        plt.legend(title='Data Structures')
        plt.tight_layout()

        plt.savefig('plots/WMRE_over_time.pdf', dpi=300, bbox_inches='tight')

def read_binary_file(filename):
    flow_sizes = []
    
    with open(filename, 'rb') as f:
        while True:
            # Read the length of the flow size distribution (integer)
            length_bytes = f.read(4)
            if not length_bytes:
                break  # End of file

            length = struct.unpack('I', length_bytes)[0]
            # print(f"Found row with length of {length}")
            
            fmt_sz = struct.calcsize("=Id")
            # Read the flow size distribution (doubles)
            flow_data = f.read(length * fmt_sz)  # Each double is 8 bytes
            if len(flow_data) != length * fmt_sz:
                print(f"Not enough data in {filename}. Data length was {len(flow_data)}, needed {length * fmt_sz}")
                continue

            values_counts = list(struct.iter_unpack(f'=Id', flow_data))
            flow_sizes.append(values_counts)
    
    # print(flow_sizes)
    return flow_sizes

def main():
    # set_test_rcs()
    # Root directory
    root_dir = "results"

    # Lists to hold all dataframes
    dataframes = []  # For non-'em' CSV files
    em_dataframes = []  # For 'em' CSV files
    ns_dataframes = []

    # Loop through all directories and files
    for tuple_type in os.listdir(root_dir):
        tuple_dir = os.path.join(root_dir, tuple_type)
        if os.path.isdir(tuple_dir):
            for data_struct_type in os.listdir(tuple_dir):
                data_struct_dir = os.path.join(tuple_dir, data_struct_type)
                if os.path.isdir(data_struct_dir):
                    for data_struct_name in os.listdir(data_struct_dir):
                        data_struct_name_dir = os.path.join(data_struct_dir, data_struct_name)
                        if os.path.isdir(data_struct_name_dir):
                            for file_name in os.listdir(data_struct_name_dir):
                                if file_name.endswith(".csv"):
                                    file_path = os.path.join(data_struct_name_dir, file_name)
                                    
                                    # Load the CSV file
                                    data = pd.read_csv(file_path)
                                    
                                    # Add metadata columns for TupleType, DataStructType, and DataStructName
                                    data['TupleType'] = tuple_type
                                    data['DataStructType'] = data_struct_type
                                    data['DataStructName'] = data_struct_name

                                    # Extract dataset name from the filename
                                    dataset_name = file_name.split("_")[1] if file_name.startswith('em_') else file_name.split("_")[0]
                                    data['DataSetName'] = dataset_name.replace(".dat", "")
                                    
                                    # Separate 'em' files from the rest
                                    if file_name.startswith('em'):
                                        em_dataframes.append(data)
                                    else:
                                        dataframes.append(data)
                                elif file_name.endswith(".dat"):
                                    file_path = os.path.join(data_struct_name_dir, file_name)
                                    data = read_binary_file(file_path)

                                    data_pd = pd.DataFrame(
                                                        data[-1],
                                                        columns=["Flow Size", "Frequency"]
                                    )
                                    # data_pd = pd.DataFrame(
                                    #                     [(value, freq, i) for i, dataset in enumerate(data) for value, freq in dataset],
                                    #                     columns=["Flow Size", "Frequency", "Epoch"]
                                    # )

                                    # Extract dataset name from the filename
                                    dataset_name = file_name.split("_")[1] if file_name.startswith('ns_') else file_name.split("_")[0]
                                    data_pd['DataSetName'] = dataset_name.replace(".dat", "")

                                    # Add metadata columns for TupleType, DataStructType, and DataStructName
                                    data_pd['TupleType'] = tuple_type
                                    data_pd['DataStructType'] = data_struct_type
                                    data_pd['DataStructName'] = data_struct_name
                                    ns_dataframes.append(data_pd)

                        else:
                            file_path = data_struct_name_dir
                            if file_path.endswith('.dat'):
                                data = read_binary_file(file_path)

                                data_pd = pd.DataFrame(
                                                    data[0],
                                                    columns=["Flow Size", "Frequency"]
                                )

                                # Extract dataset name from the filename
                                dataset_name = file_path.split("_")[1]
                                data_pd['DataSetName'] = dataset_name.replace(".dat", "")

                                # Add metadata columns for TupleType, DataStructType, and DataStructName
                                data_pd['TupleType'] = tuple_type
                                data_pd['DataStructType'] = data_struct_type
                                data_pd['DataStructName'] = "TrueData"
                                ns_dataframes.append(data_pd)

    # Display the combined data
    combined_data = pd.concat(dataframes, ignore_index=True)
    combined_data = combined_data[combined_data['TupleType'] == "SrcTuple"]

    combined_data['F1'] = combined_data['F1'].fillna(combined_data['F1 Member'])
    combined_data.drop(columns=['F1 Member'], inplace=True)

    print("Non-Estimation Data:")
    print(combined_data.head())

    print("\nEstimation Data:")
    combined_em_data = pd.concat(em_dataframes, ignore_index=True)
    combined_em_data = combined_em_data[combined_em_data['TupleType'] == "SrcTuple"]
    print(combined_em_data.head())

    print("\nFSD Data:")
    combined_ns_data = pd.concat(ns_dataframes, ignore_index=True)
    # combined_ns_data = combined_ns_data[combined_ns_data['DataSetName'] == "equinix-nyc.20190117-130000.UTC"]
    combined_ns_data = combined_ns_data[combined_ns_data['TupleType'] == "SrcTuple"]

    true_df = combined_ns_data[combined_ns_data["DataStructName"] == "TrueData"][["Flow Size", "Frequency"]].rename(
        columns={"Frequency": "TrueFrequency"}
    )
    print(true_df.head())
    combined_ns_data = combined_ns_data.merge(true_df, on="Flow Size", how="left")
    combined_ns_data["Delta"] = combined_ns_data["Frequency"] - combined_ns_data["TrueFrequency"]
    combined_ns_data.loc[combined_ns_data["DataStructName"] == "TrueData", "Delta"] = 0
    print(combined_ns_data.head())
    # plotNS(combined_ns_data)
    # Group non-'em' data by TupleType, DataStructType, and DataStructName
    grouped_data = combined_data.groupby(['TupleType', 'DataStructType', 'DataStructName'])['F1'].mean().reset_index()
    print("Grouped Non-'em' Data:")
    print(grouped_data)

    print(combined_em_data['DataStructName'].unique())
    # combined_em_data['DataSetName'] = combined_em_data['DataSetName'].str.split("-").str[:2].str.join("-")
    combined_em_data = combined_em_data[combined_em_data['TupleType'] == "SrcTuple"]
    # filtered_data = filtered_data[filtered_data['DataSetName'].apply(lambda x: not 'nyc' in x)]

    plotter = Plotter(combined_data, combined_em_data, combined_ns_data)
    plotter.plotWMRE()
    # plotter.plotEntropy()
    # plotter.plotCard()
    # plotter.plotEstimationTime()
    # plotter.plotTotalEstimationTime()
    # plotter.plotF1Membership()
    # plotter.plotBandwidth()
    # plotter.plotNSdelta()
    plt.show()

if __name__ == "__main__":
    main()
